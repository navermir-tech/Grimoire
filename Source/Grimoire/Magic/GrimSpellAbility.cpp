#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimSpellProjectile.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimWand.h"
#include "Core/GrimPlayerState.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimSpellFXTypes.h"
#include "Core/GrimTargetingTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayPrediction.h"
#include "TimerManager.h"

namespace
{
	/** 조준 · 소프트 락온에 쓰는 사거리: Cone 은 원뿔 거리, 나머지는 Range */
	float GetAimRange(const UGrimSpellDefinition* Definition)
	{
		return Definition->Delivery == EGrimSpellDelivery::Cone && Definition->ConeRange > 0.f ? Definition->ConeRange : Definition->Range;
	}

	FCollisionObjectQueryParams MakeSpellObjectParams()
	{
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
		return ObjectParams;
	}

	bool IsBlockingType(const FHitResult& Hit)
	{
		const UPrimitiveComponent* Component = Hit.GetComponent();
		const ECollisionChannel Type = Component ? Component->GetCollisionObjectType() : ECC_WorldStatic;
		return Type == ECC_WorldStatic || Type == ECC_WorldDynamic;
	}

	const TCHAR* DeliveryName(EGrimSpellDelivery Delivery)
	{
		switch (Delivery)
		{
		case EGrimSpellDelivery::Bolt: return TEXT("Bolt");
		case EGrimSpellDelivery::Stream: return TEXT("Stream");
		case EGrimSpellDelivery::Cone: return TEXT("Cone");
		case EGrimSpellDelivery::Beam: return TEXT("Beam");
		case EGrimSpellDelivery::Lob: return TEXT("Lob");
		default: return TEXT("?");
		}
	}

#if !UE_BUILD_SHIPPING
	struct FDebugAim
	{
		TWeakObjectPtr<AActor> Target;
		double ExpireTime = 0.0;
	};
	TMap<TWeakObjectPtr<const APawn>, FDebugAim> GDebugAimTargets;
#endif
}

UGrimSpellAbility::UGrimSpellAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

const UGrimSpellDefinition* UGrimSpellAbility::GetSpellDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return nullptr;
	}
	if (const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
	{
		return Cast<UGrimSpellDefinition>(Spec->SourceObject.Get());
	}
	return nullptr;
}

AGrimWand* UGrimSpellAbility::GetWand(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AGrimPlayerState* PlayerState = ActorInfo ? Cast<AGrimPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	return Spellbook ? Spellbook->GetWand() : nullptr;
}

float UGrimSpellAbility::GetEffectiveManaCost(const UGrimSpellDefinition* Definition, const AGrimWand* Wand, float ProficiencyMultiplier)
{
	return Definition ? GrimMagicMath::CalcManaCost(Definition->ManaCost, Wand ? Wand->GetManaEfficiency() : 0.f) * FMath::Max(ProficiencyMultiplier, 0.f) : 0.f;
}

float UGrimSpellAbility::GetEffectiveCooldown(const UGrimSpellDefinition* Definition, const AGrimWand* Wand, float ProficiencyMultiplier)
{
	return Definition ? GrimMagicMath::CalcCooldown(Definition->Cooldown * FMath::Max(ProficiencyMultiplier, 0.f), Wand ? Wand->GetCastSpeed() : 1.f) : 0.f;
}

namespace GrimSpellAbilityCasterLocal // 유니티 빌드에서 다른 파일과 부딪히지 않게
{
	/** 이름으로 어트리뷰트 값을 읽는다(생존 갈래의 배율 — 셋이 없으면 Default). */
	float ReadAttributeByName(const UAbilitySystemComponent* ASC, const FName& Name, float Default)
	{
		if (!ASC || Name.IsNone())
		{
			return Default;
		}
		for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
		{
			const FStructProperty* Property = Set ? FindFProperty<FStructProperty>(Set->GetClass(), Name) : nullptr;
			if (Property && Property->Struct && Property->Struct->IsChildOf(FGameplayAttributeData::StaticStruct()))
			{
				return ASC->GetNumericAttribute(FGameplayAttribute(const_cast<FStructProperty*>(Property)));
			}
		}
		return Default;
	}
}

FGrimProficiencyMultipliers UGrimSpellAbility::GetCasterMultipliers(const UGrimSpellDefinition* Definition, const UAbilitySystemComponent* ASC)
{
	using namespace GrimSpellAbilityCasterLocal;
	const UGrimSpellbookComponent* Spellbook = UGrimSpellbookComponent::FindForASC(ASC);
	FGrimProficiencyMultipliers Result = Spellbook ? Spellbook->GetProficiencyMultipliers(Definition) : FGrimProficiencyMultipliers();

	const UGrimSpellbookSettings* Settings = UGrimSpellbookSettings::Get();
	Result.ManaCost *= FMath::Max(0.f, ReadAttributeByName(ASC, Settings->ManaCostScaleAttribute, 1.f));
	Result.Cooldown /= FMath::Max(KINDA_SMALL_NUMBER, ReadAttributeByName(ASC, Settings->CastSpeedScaleAttribute, 1.f));
	return Result;
}

bool UGrimSpellAbility::HasCastBlockingTag(const UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return false;
	}
	for (const FName& TagName : UGrimSpellbookSettings::Get()->CastBlockingTags)
	{
		// 태그가 아직 없으면(생존 갈래 전) 건너뛴다.
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag))
		{
			return true;
		}
	}
	return false;
}

FVector UGrimSpellAbility::GetTargetPointOf(const AActor* Actor)
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}
	if (const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Actor))
	{
		return Targetable->GetTargetPoint();
	}
	const FBox Box = Actor->GetComponentsBoundingBox(false);
	return Box.IsValid ? Box.GetCenter() : Actor->GetActorLocation();
}

#if !UE_BUILD_SHIPPING
void UGrimSpellAbility::SetDebugAimTarget(const APawn* Pawn, AActor* Target)
{
	if (!Pawn)
	{
		return;
	}
	FDebugAim& Entry = GDebugAimTargets.FindOrAdd(Pawn);
	Entry.Target = Target;
	Entry.ExpireTime = FPlatformTime::Seconds() + 10.0;
}
#endif

FGameplayCueParameters UGrimSpellAbility::MakeSpellCueParameters(const UGrimSpellDefinition* Definition, AActor* InstigatorPawn, const FVector& Location, const FVector& Normal)
{
	FGameplayCueParameters Parameters;
	Parameters.SourceObject = Definition;
	Parameters.Instigator = InstigatorPawn;
	Parameters.EffectCauser = InstigatorPawn;
	Parameters.Location = Location;
	Parameters.Normal = Normal;
	if (Definition && Definition->Element.IsValid())
	{
		Parameters.AggregatedSourceTags.AddTag(Definition->Element);
	}
	return Parameters;
}

void UGrimSpellAbility::ExecuteSpellCue(UAbilitySystemComponent* ASC, const FGameplayTag& CueTag, const UGrimSpellDefinition* Definition, AActor* InstigatorPawn, const FVector& Location, const FVector& Normal)
{
	if (!ASC || !CueTag.IsValid())
	{
		return;
	}
	ASC->ExecuteGameplayCue(CueTag, MakeSpellCueParameters(Definition, InstigatorPawn, Location, Normal));
}

FGameplayAbilityTargetDataHandle UGrimSpellAbility::MakeAimTargetData(const FGrimAimResult& Aim)
{
	FGameplayAbilityTargetData_SingleTargetHit* Data = new FGameplayAbilityTargetData_SingleTargetHit();
	FHitResult& Hit = Data->HitResult;
	Hit.TraceStart = Aim.ViewOrigin;
	Hit.TraceEnd = Aim.AimPoint;
	Hit.Location = Aim.AimPoint;
	Hit.ImpactPoint = Aim.AimPoint;
	Hit.bBlockingHit = Aim.HasTarget();
	Hit.HitObjectHandle = FActorInstanceHandle(Aim.Target.Get());
	Hit.Item = static_cast<int32>(Aim.Mode);
	return FGameplayAbilityTargetDataHandle(Data);
}

bool UGrimSpellAbility::ReadAimTargetData(const FGameplayAbilityTargetDataHandle& Data, FGrimAimResult& OutAim)
{
	const FGameplayAbilityTargetData* Raw = Data.Get(0);
	const FHitResult* Hit = Raw ? Raw->GetHitResult() : nullptr;
	if (!Hit)
	{
		return false;
	}
	OutAim = FGrimAimResult();
	OutAim.ViewOrigin = Hit->TraceStart;
	OutAim.AimPoint = Hit->Location;
	OutAim.Target = Hit->GetActor();
	const int32 Mode = Hit->Item;
	OutAim.Mode = (Mode >= 0 && Mode <= static_cast<int32>(EGrimAimMode::Zoom)) ? static_cast<EGrimAimMode>(Mode) : EGrimAimMode::Free;
	if (const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(OutAim.Target.Get()))
	{
		OutAim.TargetKind = Targetable->GetTargetKind();
	}
	return true;
}

bool UGrimSpellAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	if (!Definition)
	{
		return false;
	}

	// 맨손 시전 없음 (Q4 제안).
	if (!GetWand(ActorInfo))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()) + KINDA_SMALL_NUMBER < Definition->Circle)
	{
		return false;
	}

	// 잠 · 쓰러짐 · 기절 중에는 시전하지 않는다(생존 갈래의 상태 태그). TODO(통합): 감기(Status.Sick.Cold)의 「가끔 끊김」.
	if (HasCastBlockingTag(ASC))
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UGrimSpellAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	const AGrimWand* Wand = GetWand(ActorInfo);
	const float ProficiencyMana = GetCasterMultipliers(Definition, ActorInfo->AbilitySystemComponent.Get()).ManaCost;
	float Cost = GetEffectiveManaCost(Definition, Wand, ProficiencyMana);
	// 누르는 마법은 시작 비용 + 첫 판정 한 번 몫이 있어야 시작한다.
	if (Definition && Definition->bChanneled)
	{
		Cost += GrimMagicMath::CalcManaCost(Definition->ChannelManaPerSecond * Definition->StreamTickInterval, Wand ? Wand->GetManaEfficiency() : 0.f) * ProficiencyMana;
	}
	const float Mana = ActorInfo->AbilitySystemComponent->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute());
	if (!GrimMagicMath::CanAfford(Mana, Cost))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
		}
		return false;
	}
	return true;
}

void UGrimSpellAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	const float Cost = GetEffectiveManaCost(Definition, GetWand(ActorInfo), GetCasterMultipliers(Definition, ActorInfo->AbilitySystemComponent.Get()).ManaCost);
	if (Cost <= 0.f)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UGrimGE_SpellCost::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_ManaCost, -Cost);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

bool UGrimSpellAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	if (Definition && Definition->CooldownTag.IsValid() && ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(Definition->CooldownTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCooldownTag);
			OptionalRelevantTags->AddTag(Definition->CooldownTag);
		}
		return false;
	}
	return true;
}

void UGrimSpellAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	const float Duration = GetEffectiveCooldown(Definition, GetWand(ActorInfo), GetCasterMultipliers(Definition, ActorInfo->AbilitySystemComponent.Get()).Cooldown);
	if (!Definition || Duration <= 0.f || !Definition->CooldownTag.IsValid())
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UGrimGE_SpellCooldown::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Cooldown, Duration);
		Spec.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

const FGameplayTagContainer* UGrimSpellAbility::GetCooldownTags() const
{
	CooldownTagsScratch.Reset();
	if (const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo))
	{
		CooldownTagsScratch.AddTag(Definition->CooldownTag);
	}
	return &CooldownTagsScratch;
}

UGameplayEffect* UGrimSpellAbility::GetCostGameplayEffect() const
{
	return GetMutableDefault<UGrimGE_SpellCost>();
}

UGameplayEffect* UGrimSpellAbility::GetCooldownGameplayEffect() const
{
	return GetMutableDefault<UGrimGE_SpellCooldown>();
}

// ───────────────────────── 시전 흐름 ─────────────────────────

void UGrimSpellAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	CastAim = FGrimAimResult();
	bAimReady = false;
	bDelivered = false;
	bStreamTicked = false;
	bEndingAbility = false;
	PendingAdvance = 0.f;
	CastDamageSpec = FGameplayEffectSpecHandle();
	StreamLastStimulus.Reset();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!Definition || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	const bool bLocal = ActorInfo->IsLocallyControlled();
	const bool bAuthority = ActorInfo->IsNetAuthority();

	if (bLocal && bAuthority)
	{
		// 리슨 서버 호스트 · 단독: 바로 권한 쪽 시전.
		BeginAuthorityCast(ComputeLocalAim(Definition), 0.f);
		return;
	}

	if (bLocal)
	{
		// 원격 클라이언트의 시전자: 조준을 서버로 보내고, 내 화면에서는 바로 나간다.
		const FGrimAimResult Aim = ComputeLocalAim(Definition);
		ASC->CallServerSetReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey(), MakeAimTargetData(Aim), FGameplayTag(), ASC->ScopedPredictionKey);
		CastAim = Aim;
		bAimReady = true;

		APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
		const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, Aim);
		// 예측 큐 — 서버가 같은 예측 키로 실행한 것은 이 클라이언트에서 다시 돌지 않는다.
		ExecuteSpellCue(ASC, GrimCueTags::Spell_Cast, Definition, Pawn, Launch.Start, Launch.Direction);
		if (Definition->bChanneled)
		{
			K2_AddGameplayCueWithParams(GrimCueTags::Spell_Channel, MakeSpellCueParameters(Definition, Pawn, Launch.Start, Launch.Direction), true);
		}

		if (Definition->UsesProjectile())
		{
			if (Definition->CastWindup > 0.f)
			{
				Timers.SetTimer(FakeBoltTimer, this, &ThisClass::SpawnFakeBolt, Definition->CastWindup, false);
			}
			else
			{
				SpawnFakeBolt();
			}
		}

		// 한 번 쏘는 마법은 서버가 끝낸다(판정 전에 끝나면 서버가 못 쏜다). 끝 복제가 안 오면 스스로.
		if (!Definition->bChanneled)
		{
			Timers.SetTimer(SafetyTimer, this, &ThisClass::OnSafetyTimeout, Definition->CastWindup + 2.f, false);
		}
		UE_LOG(LogGrimMagic, Display, TEXT("[Client] %s 시전(%s) 대상=%s 조준=%s 가짜볼트=%d"), *Definition->GetName(), DeliveryName(Definition->Delivery),
			*GetNameSafe(Aim.Target.Get()), *Aim.AimPoint.ToCompactString(), Definition->UsesProjectile() ? 1 : 0);
		return;
	}

	if (bAuthority)
	{
		// 서버 쪽 원격 시전자: TargetData 를 기다린다(활성화 RPC 다음에 온다).
		const FPredictionKey Key = ActivationInfo.GetActivationPredictionKey();
		TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, Key).AddUObject(this, &ThisClass::OnAimTargetDataReady);
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, Key);
		if (!bAimReady && IsActive())
		{
			Timers.SetTimer(SafetyTimer, this, &ThisClass::OnSafetyTimeout, 3.f, false);
		}
	}
}

void UGrimSpellAbility::OnAimTargetDataReady(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	if (!ASC || !Definition || !IsActive() || bAimReady)
	{
		return;
	}

	FGrimAimResult Aim;
	const bool bRead = ReadAimTargetData(Data, Aim);
	// 여기까지 읽었으면 캐시를 비운다(Data 는 캐시를 가리키므로 읽은 뒤에).
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

	if (!bRead || !ValidateClientAim(Definition, Aim))
	{
		UE_LOG(LogGrimMagic, Warning, TEXT("[Server] %s 클라이언트 조준이 어긋나 서버 조준으로 바꾼다"), *Definition->GetName());
		Aim = ComputeViewAim(Definition);
	}

	// 지금은 클라이언트의 예측 키 창 안이다 — 여기서 실행하는 Cast · Channel 큐는 시전자에게 다시 가지 않는다.
	BeginAuthorityCast(Aim, GetOneWayLatency());
}

void UGrimSpellAbility::BeginAuthorityCast(const FGrimAimResult& Aim, float OneWayLatency)
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Definition || !ASC)
	{
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(SafetyTimer);

	CastAim = Aim;
	bAimReady = true;
	CastDamageSpec = MakeDamageSpec(Definition, GetWand(CurrentActorInfo));

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, Aim);
	ExecuteSpellCue(ASC, GrimCueTags::Spell_Cast, Definition, Pawn, Launch.Start, Launch.Direction);
	if (Definition->bChanneled)
	{
		K2_AddGameplayCueWithParams(GrimCueTags::Spell_Channel, MakeSpellCueParameters(Definition, Pawn, Launch.Start, Launch.Direction), true);
	}

	// 숙련도(Q26): 서버가 시전을 확정했다 — 시전 경험(같은 대상 반복 감쇠).
	if (UGrimSpellbookComponent* Spellbook = UGrimSpellbookComponent::FindForASC(ASC))
	{
		Spellbook->AwardCastXP(Definition, Aim.Target.Get());
	}

	const GrimSpellDelivery::FLagSplit Lag = GrimSpellDelivery::SplitLag(OneWayLatency, Definition->CastWindup, UGrimMagicSettings::Get()->MaxLagCompensation);
	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 시전(%s) 대상=%s 지연=%.3fs 대기=%.3fs 당김=%.3fs"), *Definition->GetName(), DeliveryName(Definition->Delivery),
		*GetNameSafe(Aim.Target.Get()), OneWayLatency, Lag.WaitSeconds, Lag.AdvanceSeconds);

	if (Definition->bChanneled)
	{
		const float Interval = FMath::Max(Definition->StreamTickInterval, 0.02f);
		if (Lag.WaitSeconds > 0.f)
		{
			Timers.SetTimer(StreamTimer, this, &ThisClass::StreamTick, Interval, true, Lag.WaitSeconds);
		}
		else
		{
			Timers.SetTimer(StreamTimer, this, &ThisClass::StreamTick, Interval, true);
			StreamTick();
		}
		return;
	}

	PendingAdvance = Lag.AdvanceSeconds;
	if (Lag.WaitSeconds > 0.f)
	{
		Timers.SetTimer(DeliverTimer, this, &ThisClass::DeliverNow, Lag.WaitSeconds, false);
	}
	else
	{
		DeliverNow();
	}
}

void UGrimSpellAbility::DeliverNow()
{
	if (bDelivered || !bAimReady || !IsActive())
	{
		return;
	}
	bDelivered = true;

	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	AGrimWand* Wand = GetWand(CurrentActorInfo);
	if (Definition && Wand)
	{
		switch (Definition->Delivery)
		{
		case EGrimSpellDelivery::Bolt:
		case EGrimSpellDelivery::Lob:
			DeliverBolt(Definition, Wand, CastAim, CastDamageSpec, PendingAdvance);
			break;
		case EGrimSpellDelivery::Cone:
			DeliverCone(Definition, CastAim, CastDamageSpec);
			break;
		case EGrimSpellDelivery::Beam:
			DeliverBeam(Definition, CastAim, CastDamageSpec);
			break;
		case EGrimSpellDelivery::Stream:
			// bChanneled 가 꺼진 Stream: 한 번만 판정
			StreamTick();
			break;
		}
	}

	if (!bEndingAbility)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGrimSpellAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	if (!Definition || !Definition->bChanneled || !IsActive() || !ActorInfo || !ActorInfo->IsLocallyControlled())
	{
		return;
	}
	// 새 예측 키 창: 예측해 띄운 Channel 큐를 이 클라이언트에서 바로 거둔다. 끝은 서버로 복제된다.
	FScopedPredictionWindow ScopedPrediction(ActorInfo->AbilitySystemComponent.Get(), true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGrimSpellAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive() && !bEndingAbility && ActorInfo)
	{
		bEndingAbility = true;
		const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
		FTimerManager& Timers = GetWorld()->GetTimerManager();

		if (Definition && !bWasCancelled)
		{
			if (ActorInfo->IsNetAuthority() && bAimReady)
			{
				// 짧게 눌러 판정 전에 끝나도 한 번은 나간다(양초는 한 번 스치면 꺼진다).
				if (Definition->bChanneled && !bStreamTicked)
				{
					StreamTick();
				}
				else if (!Definition->bChanneled && !bDelivered)
				{
					DeliverNow();
				}
			}
			if (!ActorInfo->IsNetAuthority() && Timers.IsTimerActive(FakeBoltTimer))
			{
				SpawnFakeBolt();
			}
		}

		Timers.ClearTimer(DeliverTimer);
		Timers.ClearTimer(StreamTimer);
		Timers.ClearTimer(SafetyTimer);
		Timers.ClearTimer(FakeBoltTimer);

		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get(); ASC && TargetDataDelegateHandle.IsValid())
		{
			ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
			ASC->ConsumeClientReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey());
			TargetDataDelegateHandle.Reset();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEndingAbility = false;
}

void UGrimSpellAbility::OnSafetyTimeout()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bAimReady);
	}
}

float UGrimSpellAbility::GetOneWayLatency() const
{
	const APlayerState* PlayerState = CurrentActorInfo ? Cast<APlayerState>(CurrentActorInfo->OwnerActor.Get()) : nullptr;
	if (!PlayerState || (CurrentActorInfo && CurrentActorInfo->IsLocallyControlled()))
	{
		return 0.f;
	}
	// 왕복 지연(ms)의 절반
	return PlayerState->GetPingInMilliseconds() * 0.5f / 1000.f;
}

// ───────────────────────── 조준 ─────────────────────────

FGrimAimResult UGrimSpellAbility::ComputeLocalAim(const UGrimSpellDefinition* Definition) const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());

#if !UE_BUILD_SHIPPING
	if (const FDebugAim* Debug = GDebugAimTargets.Find(Pawn); Debug && Debug->Target.IsValid() && FPlatformTime::Seconds() < Debug->ExpireTime)
	{
		FGrimAimResult Aim = ComputeViewAim(Definition);
		Aim.Mode = EGrimAimMode::Locked;
		Aim.Target = Debug->Target;
		Aim.AimPoint = GetTargetPointOf(Debug->Target.Get());
		return Aim;
	}
#endif

	if (const IGrimAimProvider* Provider = UGrimTargetingStatics::FindAimProvider(Pawn))
	{
		const float AimRange = GetAimRange(Definition);
		FGrimAimResult Aim = Provider->GetCurrentAim(AimRange);
		// 고정 락온(Locked)은 사거리 밖 대상도 돌려준다 — 사거리로 자르는 건 마법 쪽. 밖이면 대상을 놓고 카메라 정면으로.
		if (Aim.HasTarget() && Pawn && FVector::DistSquared(GetTargetPointOf(Aim.Target.Get()), Pawn->GetActorLocation()) > FMath::Square(AimRange))
		{
			return ComputeViewAim(Definition);
		}
		if (Aim.HasTarget() || !Aim.AimPoint.IsNearlyZero())
		{
			if (Aim.ViewOrigin.IsNearlyZero())
			{
				Aim.ViewOrigin = ComputeViewAim(Definition).ViewOrigin;
			}
			if (Aim.HasTarget() && Aim.AimPoint.IsNearlyZero())
			{
				Aim.AimPoint = GetTargetPointOf(Aim.Target.Get());
			}
			return Aim;
		}
	}
	return ComputeViewAim(Definition);
}

FGrimAimResult UGrimSpellAbility::ComputeViewAim(const UGrimSpellDefinition* Definition) const
{
	FGrimAimResult Aim;
	Aim.Mode = EGrimAimMode::Free;

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !Definition)
	{
		return Aim;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		Pawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * GetAimRange(Definition);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellAim), false, Pawn);
	FHitResult Hit;
	Aim.ViewOrigin = ViewLocation;
	Aim.AimPoint = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params) ? FVector(Hit.ImpactPoint) : TraceEnd;
	return Aim;
}

bool UGrimSpellAbility::FollowsTarget(const FGrimAimResult& Aim)
{
	// 줌인 조준은 대상이 조준점 아래에 있어도 따라가지 않는다 — 유도 없이 겨눈 곳으로 곧게(R24).
	return Aim.HasTarget() && Aim.Mode != EGrimAimMode::Zoom;
}

bool UGrimSpellAbility::ValidateClientAim(const UGrimSpellDefinition* Definition, FGrimAimResult& InOutAim) const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return false;
	}
	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();
	const float MaxDistance = GetAimRange(Definition) + Settings->AimRangeSlack;
	const FVector PawnLocation = Pawn->GetActorLocation();

	if (!GrimSpellDelivery::IsClientAimPlausible(InOutAim.ViewOrigin, InOutAim.AimPoint, PawnLocation, Pawn->GetBaseAimRotation().Vector(),
		Settings->MaxAimOriginError, Settings->MaxAimAngleError, MaxDistance))
	{
		return false;
	}

	if (const AActor* Target = InOutAim.Target.Get())
	{
		const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Target);
		const FVector TargetPoint = GetTargetPointOf(Target);
		const bool bTargetable = !Targetable || Targetable->CanBeTargetedBy(Pawn);
		if (!bTargetable || FVector::DistSquared(TargetPoint, PawnLocation) > FMath::Square(MaxDistance))
		{
			// 대상만 버리고 조준점은 쓴다.
			InOutAim.Target = nullptr;
			InOutAim.TargetKind = EGrimTargetKind::None;
			InOutAim.Mode = EGrimAimMode::Free;
		}
		else if (InOutAim.Mode != EGrimAimMode::Zoom)
		{
			InOutAim.AimPoint = TargetPoint; // 줌인 조준은 겨눈 「바로 그 지점」을 그대로 쓴다(R24)
		}
	}
	return true;
}

GrimSpellDelivery::FLaunch UGrimSpellAbility::ComputeLaunch(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim) const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const AGrimWand* Wand = GetWand(CurrentActorInfo);
	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();

	const FVector PawnLocation = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const FVector Muzzle = Wand ? Wand->GetMuzzleTransform().GetLocation() : PawnLocation;
	const FVector Chest = PawnLocation + FVector(0.f, 0.f, Settings->ChestHeight);
	// 대상이 있으면 지금 대상 위치(Windup 동안 움직였을 수 있다). 줌인 조준은 겨눈 지점 그대로(R24).
	const FVector AimPoint = FollowsTarget(Aim) ? GetTargetPointOf(Aim.Target.Get()) : Aim.AimPoint;

	FVector ViewDirection = (AimPoint - Aim.ViewOrigin).GetSafeNormal();
	if (ViewDirection.IsNearlyZero() && Pawn)
	{
		ViewDirection = Pawn->GetBaseAimRotation().Vector();
	}
	return GrimSpellDelivery::ComputeLaunch(Muzzle, Chest, AimPoint, ViewDirection, Settings->CloseAimDistance);
}

// ───────────────────────── 전달 ─────────────────────────

FGameplayEffectSpecHandle UGrimSpellAbility::MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const
{
	if (!Definition || Definition->Damage <= 0.f)
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> EffectClass = Definition->ImpactEffect ? Definition->ImpactEffect : TSubclassOf<UGameplayEffect>(UGrimGE_SpellDamage::StaticClass());
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (Spec.IsValid())
	{
		const float WandPower = Wand ? Wand->GetElementPower(Definition->Element) : 1.f;
		Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, GrimMagicMath::CalcDamage(Definition->Damage, WandPower, 1.f));
		if (Definition->Element.IsValid())
		{
			Spec.Data->AddDynamicAssetTag(Definition->Element);
		}
		if (Definition->SpellTag.IsValid())
		{
			Spec.Data->AddDynamicAssetTag(Definition->SpellTag);
		}
	}
	return Spec;
}

void UGrimSpellAbility::DeliverBolt(const UGrimSpellDefinition* Definition, AGrimWand* Wand, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec, float AdvanceSeconds)
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const TSubclassOf<AGrimSpellProjectile> ProjectileClass = Definition->ProjectileClass ? Definition->ProjectileClass : TSubclassOf<AGrimSpellProjectile>(AGrimSpellProjectile::StaticClass());
	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, Aim);

	const FTransform SpawnTransform(Launch.Direction.Rotation(), Launch.Start);
	AGrimSpellProjectile* Projectile = GetWorld()->SpawnActorDeferred<AGrimSpellProjectile>(ProjectileClass, SpawnTransform, Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}
	Projectile->InitSpell(Definition, GetAbilitySystemComponentFromActorInfo(), DamageSpec, Definition->HomingStrength > 0.f && FollowsTarget(Aim) ? Aim.Target.Get() : nullptr);
	Projectile->FinishSpawning(SpawnTransform);
	Projectile->CatchUp(AdvanceSeconds);
}

void UGrimSpellAbility::SpawnFakeBolt()
{
	GetWorld()->GetTimerManager().ClearTimer(FakeBoltTimer);
	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Definition || !Pawn || !bAimReady)
	{
		return;
	}

	const TSubclassOf<AGrimSpellProjectile> ProjectileClass = Definition->ProjectileClass ? Definition->ProjectileClass : TSubclassOf<AGrimSpellProjectile>(AGrimSpellProjectile::StaticClass());
	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, CastAim);
	const FTransform SpawnTransform(Launch.Direction.Rotation(), Launch.Start);
	AGrimSpellProjectile* Fake = GetWorld()->SpawnActorDeferred<AGrimSpellProjectile>(ProjectileClass, SpawnTransform, Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Fake)
	{
		return;
	}
	Fake->InitFake(Definition, Definition->HomingStrength > 0.f && FollowsTarget(CastAim) ? CastAim.Target.Get() : nullptr);
	Fake->FinishSpawning(SpawnTransform);
	UE_LOG(LogGrimMagic, Display, TEXT("[Client] %s 가짜 볼트 발사 %s → %s"), *Definition->GetName(), *Launch.Start.ToCompactString(), *Launch.Direction.ToCompactString());
}

void UGrimSpellAbility::DeliverCone(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, Aim);
	const float Range = Definition->ConeRange;
	if (Range <= 0.f)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellCone), false, Avatar);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Launch.Start, FQuat::Identity, MakeSpellObjectParams(), FCollisionShape::MakeSphere(Range), Params);

	struct FConeHit
	{
		AActor* Actor;
		FVector Point;
		float Distance;
	};
	TArray<FConeHit> Candidates;
	TSet<AActor*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == Avatar || Seen.Contains(Actor))
		{
			continue;
		}
		Seen.Add(Actor);
		// 받는 쪽(속성 사물)이나 ASC(생물 · 플레이어)가 있는 것만.
		if (!UGrimElementalStatics::FindReceiver(Actor) && !FindTargetASC(Actor))
		{
			continue;
		}
		const FVector Point = GetTargetPointOf(Actor);
		if (!GrimSpellDelivery::IsInCone(Launch.Start, Launch.Direction, Definition->ConeHalfAngle, Range, Point))
		{
			continue;
		}
		// 벽 너머는 안 맞는다.
		FCollisionQueryParams SightParams(SCENE_QUERY_STAT(GrimSpellConeSight), false, Avatar);
		SightParams.AddIgnoredActor(Actor);
		if (GetWorld()->LineTraceTestByChannel(Launch.Start, Point, ECC_Visibility, SightParams))
		{
			continue;
		}
		Candidates.Add({ Actor, Point, static_cast<float>(FVector::Dist(Launch.Start, Point)) });
	}

	Candidates.Sort([](const FConeHit& A, const FConeHit& B) { return A.Distance < B.Distance; });
	const int32 MaxTargets = UGrimMagicSettings::Get()->MaxConeTargets;
	for (int32 Index = 0; Index < Candidates.Num() && Index < MaxTargets; ++Index)
	{
		const FConeHit& Hit = Candidates[Index];
		const float Intensity = GrimSpellDelivery::ConeIntensityAt(Definition->ConeFalloff, Range, Hit.Distance);
		ApplySpellHit(Definition, SourceASC, DamageSpec, Avatar, Hit.Actor, Intensity);
		ExecuteSpellCue(SourceASC, GrimCueTags::Spell_Impact, Definition, Avatar, Hit.Point, -Launch.Direction);
		UE_LOG(LogGrimMagic, Display, TEXT("[Cone] %s → %s 거리 %.0fcm 세기 %.2f"), *Definition->GetName(), *Hit.Actor->GetName(), Hit.Distance, Intensity);
	}
}

void UGrimSpellAbility::DeliverBeam(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, Aim);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellBeam), false, Avatar);
	const FVector End = Launch.Start + Launch.Direction * Definition->Range;

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Launch.Start, End, FQuat::Identity, MakeSpellObjectParams(), FCollisionShape::MakeSphere(FMath::Max(Definition->Radius, 1.f)), Params);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Distance < B.Distance; });

	TSet<AActor*> Done;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || Done.Contains(HitActor))
		{
			continue;
		}
		Done.Add(HitActor);
		ApplySpellHit(Definition, SourceASC, DamageSpec, Avatar, HitActor);
		ExecuteSpellCue(SourceASC, GrimCueTags::Spell_Impact, Definition, Avatar, Hit.ImpactPoint, Hit.ImpactNormal);

		// 광선은 폰 · 물리체는 지나가고, 지형 · 사물에서 멈춘다.
		if (IsBlockingType(Hit))
		{
			break;
		}
	}
}

bool UGrimSpellAbility::TraceStreamArc(const UWorld* World, const UGrimSpellDefinition* Definition, const FVector& Start, const FVector& AimPoint, const AActor* IgnoreActor, TArray<FVector>& OutPoints, TArray<FHitResult>* OutHits)
{
	OutPoints.Reset();
	if (OutHits)
	{
		OutHits->Reset();
	}
	if (!World || !Definition || Definition->StreamLaunchSpeed <= 0.f)
	{
		return false;
	}

	const float GravityZ = World->GetGravityZ() * Definition->StreamGravityScale;
	FVector Direction;
	GrimSpellDelivery::SolveLaunchDirection(Start, AimPoint, Definition->StreamLaunchSpeed, GravityZ, Direction);

	TArray<FVector> Arc;
	GrimSpellDelivery::ComputeArcPoints(Start, Direction * Definition->StreamLaunchSpeed, GravityZ, Definition->GetStreamFlightTime(), Definition->StreamSegments, Arc);
	if (Arc.Num() < 2)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellStream), false, IgnoreActor);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(FMath::Max(Definition->Radius, 1.f));
	const FCollisionObjectQueryParams ObjectParams = MakeSpellObjectParams();
	TSet<const AActor*> Seen;

	OutPoints.Add(Arc[0]);
	for (int32 Index = 0; Index + 1 < Arc.Num(); ++Index)
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, Arc[Index], Arc[Index + 1], FQuat::Identity, ObjectParams, Shape, Params);
		Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Distance < B.Distance; });

		for (const FHitResult& Hit : Hits)
		{
			const AActor* HitActor = Hit.GetActor();
			if (OutHits && HitActor && !Seen.Contains(HitActor))
			{
				Seen.Add(HitActor);
				OutHits->Add(Hit);
			}
			if (IsBlockingType(Hit))
			{
				// 땅 · 사물에 닿으면 줄기가 거기서 튄다 — 물보라 반지름 안의 것도 맞는다.
				OutPoints.Add(Hit.Location);
				if (OutHits && Definition->StreamSplashRadius > 0.f)
				{
					TArray<FOverlapResult> Splash;
					World->OverlapMultiByObjectType(Splash, Hit.Location, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Definition->StreamSplashRadius), Params);
					for (const FOverlapResult& Overlap : Splash)
					{
						AActor* SplashActor = Overlap.GetActor();
						if (SplashActor && !Seen.Contains(SplashActor))
						{
							Seen.Add(SplashActor);
							OutHits->Add(FHitResult(SplashActor, Overlap.GetComponent(), GetTargetPointOf(SplashActor), Hit.ImpactNormal));
						}
					}
				}
				return true;
			}
		}
		OutPoints.Add(Arc[Index + 1]);
	}
	return false;
}

void UGrimSpellAbility::StreamTick()
{
	if (!IsActive() || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}
	const bool bFirstTick = !bStreamTicked;
	bStreamTicked = true;

	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const AGrimWand* Wand = GetWand(CurrentActorInfo);
	if (!Definition || !ASC || !Pawn || !Wand)
	{
		if (!bEndingAbility)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	// 초당 마나(Q19). 모자라면 멈춘다.
	const float Cost = GrimMagicMath::CalcManaCost(Definition->ChannelManaPerSecond * Definition->StreamTickInterval, Wand->GetManaEfficiency())
		* GetCasterMultipliers(Definition, ASC).ManaCost;
	if (Cost > 0.f)
	{
		const float Mana = ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute());
		if (!GrimMagicMath::CanAfford(Mana, Cost))
		{
			UE_LOG(LogGrimMagic, Display, TEXT("[Stream] %s 마나 부족으로 멈춤"), *Definition->GetName());
			if (!bEndingAbility)
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}
			return;
		}
		FGameplayEffectSpecHandle CostSpec = MakeOutgoingGameplayEffectSpec(UGrimGE_SpellCost::StaticClass(), GetAbilityLevel());
		if (CostSpec.IsValid())
		{
			CostSpec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_ManaCost, -Cost);
			ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
		}
	}

	// 조준: 시작 때 잡은 대상이 아직 앞쪽이면 계속 따라가고, 아니면 놓고 지금 시선으로.
	if (AActor* Target = CastAim.Target.Get())
	{
		const FVector ToTarget = (GetTargetPointOf(Target) - Pawn->GetActorLocation()).GetSafeNormal();
		const float CosKeep = FMath::Cos(FMath::DegreesToRadians(UGrimMagicSettings::Get()->StreamKeepTargetAngle));
		if (FVector::DotProduct(ToTarget, Pawn->GetBaseAimRotation().Vector()) < CosKeep)
		{
			CastAim.Target = nullptr;
			CastAim.TargetKind = EGrimTargetKind::None;
			CastAim.Mode = EGrimAimMode::Free;
		}
	}
	// 첫 판정은 시전 순간의 조준(TargetData) 그대로.
	if (!CastAim.HasTarget() && !bFirstTick)
	{
		CastAim = CurrentActorInfo->IsLocallyControlled() ? ComputeLocalAim(Definition) : ComputeViewAim(Definition);
	}

	const GrimSpellDelivery::FLaunch Launch = ComputeLaunch(Definition, CastAim);
	const FVector AimPoint = FollowsTarget(CastAim) ? GetTargetPointOf(CastAim.Target.Get()) : CastAim.AimPoint;

	TArray<FVector> Points;
	TArray<FHitResult> Hits;
	TraceStreamArc(GetWorld(), Definition, Launch.Start, AimPoint, Pawn, Points, &Hits);

	const double Now = GetWorld()->GetTimeSeconds();
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Pawn)
		{
			continue;
		}
		const double* Last = StreamLastStimulus.Find(HitActor);
		if (!GrimSpellDelivery::IsStimulusDue(Last, Now, Definition->StreamStimulusInterval))
		{
			continue;
		}
		StreamLastStimulus.Add(HitActor, Now);
		ApplySpellHit(Definition, ASC, CastDamageSpec, Pawn, HitActor);
		ExecuteSpellCue(ASC, GrimCueTags::Spell_Impact, Definition, Pawn, Hit.ImpactPoint, Hit.ImpactNormal);
		UE_LOG(LogGrimMagic, Display, TEXT("[Stream] %s → %s"), *Definition->GetName(), *HitActor->GetName());
	}
}

// ───────────────────────── 적용 ─────────────────────────

UAbilitySystemComponent* UGrimSpellAbility::FindTargetASC(AActor* Actor)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		return ASC;
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn->GetPlayerState());
	}
	return nullptr;
}

void UGrimSpellAbility::ApplySpellHit(const UGrimSpellDefinition* Definition, UAbilitySystemComponent* SourceASC, const FGameplayEffectSpecHandle& DamageSpec, AActor* Instigator, AActor* HitActor, float StimulusIntensity)
{
	if (!Definition || !HitActor || !HitActor->HasAuthority())
	{
		return;
	}
	// 숙련도(Q26): 반응 여부는 자극을 주기 전에 본다.
	const bool bMeaningfulHit = UGrimSpellbookComponent::IsMeaningfulHit(Definition, SourceASC, HitActor);

	if (UAbilitySystemComponent* TargetASC = FindTargetASC(HitActor); TargetASC && DamageSpec.IsValid())
	{
		// 플레이어끼리는 기본으로 피해가 없다(협동 — 속성 자극은 준다).
		const bool bBothPlayers = Cast<APlayerState>(TargetASC->GetOwnerActor()) && SourceASC && Cast<APlayerState>(SourceASC->GetOwnerActor());
		if (!bBothPlayers || UGrimMagicSettings::Get()->bFriendlyFire)
		{
			if (SourceASC)
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
			}
			else
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
			}
		}
	}

	FGrimElementalStimulus Stimulus = Definition->Stimulus;
	if (!Stimulus.Element.IsValid())
	{
		Stimulus.Element = Definition->Element;
	}
	if (StimulusIntensity >= 0.f)
	{
		Stimulus.Intensity = StimulusIntensity;
	}
	if (UGrimElementalStatics::ApplyStimulusToActor(HitActor, Stimulus, Instigator))
	{
		UE_LOG(LogGrimMagic, Verbose, TEXT("%s → %s 자극 %s %.2f"), *Definition->GetName(), *HitActor->GetName(), *Stimulus.Element.ToString(), Stimulus.Intensity);
	}

	if (bMeaningfulHit)
	{
		if (UGrimSpellbookComponent* Spellbook = UGrimSpellbookComponent::FindForASC(SourceASC))
		{
			Spellbook->AwardHitXP(Definition, HitActor);
		}
	}
}
