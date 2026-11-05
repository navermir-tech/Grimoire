#include "Magic/GrimFistAbility.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimSpellSlotInput.h"
#include "Core/GrimElementalTypes.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace GrimFistLocal // 유니티 빌드에서 다른 파일의 도우미와 부딪히지 않게
{
	const TCHAR* FistRoleName(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const bool bLocal = ActorInfo && ActorInfo->IsLocallyControlled();
		const bool bAuthority = ActorInfo && ActorInfo->IsNetAuthority();
		return bLocal && bAuthority ? TEXT("Host") : (bAuthority ? TEXT("Server") : TEXT("Client"));
	}

	double FistNow(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
		return World ? World->GetTimeSeconds() : 0.0;
	}
}

UGrimFistAbility::UGrimFistAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UGrimFistAbility::EnsureGranted(UAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return false;
	}
	if (ASC->FindAbilitySpecFromClass(UGrimFistAbility::StaticClass()))
	{
		return true;
	}
	FGameplayAbilitySpec Spec(UGrimFistAbility::StaticClass(), 1, INDEX_NONE, ASC->GetOwner());
	Spec.GetDynamicSpecSourceTags().AddTag(GrimInputTags::Input_Spell_BasicAttack);
	return ASC->GiveAbility(Spec).IsValid();
}

bool UGrimFistAbility::CanUseBasicAttackSpell(const FGameplayAbilityActorInfo* ActorInfo)
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	// 기본 공격 Spec 은 서버가 Input.Spell.BasicAttack 태그를 붙인다(복제) — 그 태그를 가진 마법이 있으면 배운 것이다.
	return ASC && UGrimSpellAbility::GetWand(ActorInfo) && GrimSpellSlotInput::FindSpellForInput(ASC, GrimInputTags::Input_Spell_BasicAttack);
}

bool UGrimFistAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	using namespace GrimFistLocal;
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || UGrimSpellAbility::HasCastBlockingTag(ASC) || CanUseBasicAttackSpell(ActorInfo))
	{
		return false;
	}
	if (IsInstantiated())
	{
		const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
		const bool bRemoteOnServer = ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
		if (!FGrimBasicAttackMath::IsReady(FistNow(ActorInfo), Combo.LastAttackTime, Settings->AttackInterval, bRemoteOnServer ? Settings->ServerIntervalTolerance : 0.f))
		{
			return false;
		}
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGrimFistAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	using namespace GrimFistLocal;
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// 리듬만 센다(주먹질엔 강타 없음).
	FGrimBasicAttackMath::Register(Combo, FistNow(ActorInfo), 0.f, 1);
	UE_LOG(LogGrimMagic, Display, TEXT("[%s] 주먹질"), FistRoleName(ActorInfo));

	// 판정은 서버만(서버도 같은 활성화를 받아 따로 돈다).
	if (ActorInfo->IsNetAuthority())
	{
		PerformFist();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGrimFistAbility::PerformFist()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!Pawn || !SourceASC || !World)
	{
		return;
	}
	const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();

	FVector Forward = Pawn->GetBaseAimRotation().Vector();
	Forward.Z = 0.f;
	Forward = Forward.IsNearlyZero() ? Pawn->GetActorForwardVector() : Forward.GetSafeNormal();
	const FVector Start = Pawn->GetActorLocation();
	const FVector End = Start + Forward * Settings->FistRange;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimFist), false, Pawn);
	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Settings->FistRadius), Params);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Distance < B.Distance; });

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		UAbilitySystemComponent* TargetASC = HitActor ? UGrimSpellAbility::FindTargetASC(HitActor) : nullptr;
		// 받는 쪽(속성 사물)이나 ASC(생물 · 플레이어)가 있는 첫 대상 하나만.
		if (!HitActor || HitActor == Pawn || (!UGrimElementalStatics::FindReceiver(HitActor) && !TargetASC))
		{
			continue;
		}

		// 플레이어끼리는 기본으로 피해가 없다(협동 — 자극은 준다). 마법의 ApplySpellHit 과 같은 규칙.
		const bool bBothPlayers = TargetASC && Cast<APlayerState>(TargetASC->GetOwnerActor()) && Cast<APlayerState>(SourceASC->GetOwnerActor());
		if (TargetASC && TargetASC != SourceASC && (!bBothPlayers || UGrimMagicSettings::Get()->bFriendlyFire))
		{
			FGameplayEffectSpecHandle Damage = MakeOutgoingGameplayEffectSpec(UGrimGE_SpellDamage::StaticClass(), GetAbilityLevel());
			if (Damage.IsValid())
			{
				Damage.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, Settings->FistDamage);
				Damage.Data->AddDynamicAssetTag(GrimMagicTags::Element_None);
				SourceASC->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), TargetASC);
			}
		}

		FGrimElementalStimulus Stimulus;
		Stimulus.Element = GrimMagicTags::Element_None;
		Stimulus.Intensity = Settings->FistStimulusIntensity;
		Stimulus.Source = EGrimStimulusSource::Spell;
		UGrimElementalStatics::ApplyStimulusToActor(HitActor, Stimulus, Pawn);
		UE_LOG(LogGrimMagic, Display, TEXT("[Server] 주먹질 → %s"), *HitActor->GetName());
		return;
	}
}
