#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellProjectile.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimWand.h"
#include "Core/GrimPlayerState.h"
#include "Core/GrimElementalTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimMagic, Log, All);

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

float UGrimSpellAbility::GetEffectiveManaCost(const UGrimSpellDefinition* Definition, const AGrimWand* Wand)
{
	return Definition ? GrimMagicMath::CalcManaCost(Definition->ManaCost, Wand ? Wand->GetManaEfficiency() : 0.f) : 0.f;
}

float UGrimSpellAbility::GetEffectiveCooldown(const UGrimSpellDefinition* Definition, const AGrimWand* Wand)
{
	return Definition ? GrimMagicMath::CalcCooldown(Definition->Cooldown, Wand ? Wand->GetCastSpeed() : 1.f) : 0.f;
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

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UGrimSpellAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const float Cost = GetEffectiveManaCost(GetSpellDefinition(Handle, ActorInfo), GetWand(ActorInfo));
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
	const float Cost = GetEffectiveManaCost(GetSpellDefinition(Handle, ActorInfo), GetWand(ActorInfo));
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
	const float Duration = GetEffectiveCooldown(Definition, GetWand(ActorInfo));
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

void UGrimSpellAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UGrimSpellDefinition* Definition = GetSpellDefinition(Handle, ActorInfo);
	if (Definition && Definition->CastTime > 0.f)
	{
		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, Definition->CastTime);
		Wait->OnFinish.AddDynamic(this, &ThisClass::OnCastTimeFinished);
		Wait->ReadyForActivation();
		return;
	}

	Deliver();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGrimSpellAbility::OnCastTimeFinished()
{
	Deliver();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGrimSpellAbility::Deliver()
{
	// 투사체 · 판정은 서버만. 클라이언트는 비용 · 쿨다운만 예측한다.
	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	const UGrimSpellDefinition* Definition = GetSpellDefinition(CurrentSpecHandle, CurrentActorInfo);
	AGrimWand* Wand = GetWand(CurrentActorInfo);
	if (!Definition || !Wand)
	{
		return;
	}

	const FVector Muzzle = Wand->GetMuzzleTransform().GetLocation();
	const FVector AimPoint = GetAimPoint(Definition, Muzzle);
	const FGameplayEffectSpecHandle DamageSpec = MakeDamageSpec(Definition, Wand);

	switch (Definition->Delivery)
	{
	case EGrimSpellDelivery::Projectile:
		DeliverProjectile(Definition, Wand, Muzzle, AimPoint, DamageSpec);
		break;
	case EGrimSpellDelivery::Beam:
		DeliverBeam(Definition, Muzzle, AimPoint, DamageSpec);
		break;
	}
}

FVector UGrimSpellAbility::GetAimPoint(const UGrimSpellDefinition* Definition, const FVector& Muzzle) const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return Muzzle;
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

	const FVector Direction = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + Direction * Definition->Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellAim), false, Pawn);
	FHitResult Hit;
	FVector AimPoint = TraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		AimPoint = Hit.ImpactPoint;
	}

	// 카메라와 완드 사이에 맞은 점(머즐 뒤)이면 시선 방향 그대로 쏜다.
	if (FVector::DotProduct(AimPoint - Muzzle, Direction) <= 0.f)
	{
		AimPoint = Muzzle + Direction * Definition->Range;
	}
	return AimPoint;
}

FGameplayEffectSpecHandle UGrimSpellAbility::MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const
{
	if (Definition->Damage <= 0.f)
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

void UGrimSpellAbility::DeliverProjectile(const UGrimSpellDefinition* Definition, AGrimWand* Wand, const FVector& Muzzle, const FVector& AimPoint, const FGameplayEffectSpecHandle& DamageSpec)
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const TSubclassOf<AGrimSpellProjectile> ProjectileClass = Definition->ProjectileClass ? Definition->ProjectileClass : TSubclassOf<AGrimSpellProjectile>(AGrimSpellProjectile::StaticClass());

	const FTransform SpawnTransform((AimPoint - Muzzle).Rotation(), Muzzle);
	AGrimSpellProjectile* Projectile = GetWorld()->SpawnActorDeferred<AGrimSpellProjectile>(ProjectileClass, SpawnTransform, Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}
	Projectile->InitSpell(Definition, GetAbilitySystemComponentFromActorInfo(), DamageSpec);
	Projectile->FinishSpawning(SpawnTransform);
}

void UGrimSpellAbility::DeliverBeam(const UGrimSpellDefinition* Definition, const FVector& Muzzle, const FVector& AimPoint, const FGameplayEffectSpecHandle& DamageSpec)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSpellBeam), false, Avatar);
	const FVector End = Muzzle + (AimPoint - Muzzle).GetSafeNormal() * Definition->Range;

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Muzzle, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(FMath::Max(Definition->Radius, 1.f)), Params);
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
		ApplySpellHit(Definition, GetAbilitySystemComponentFromActorInfo(), DamageSpec, Avatar, HitActor);

		// 광선은 폰 · 물리체는 지나가고, 지형 · 사물에서 멈춘다.
		const UPrimitiveComponent* Component = Hit.GetComponent();
		const ECollisionChannel Type = Component ? Component->GetCollisionObjectType() : ECC_WorldStatic;
		if (Type == ECC_WorldStatic || Type == ECC_WorldDynamic)
		{
			break;
		}
	}
}

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

void UGrimSpellAbility::ApplySpellHit(const UGrimSpellDefinition* Definition, UAbilitySystemComponent* SourceASC, const FGameplayEffectSpecHandle& DamageSpec, AActor* Instigator, AActor* HitActor)
{
	if (!Definition || !HitActor || !HitActor->HasAuthority())
	{
		return;
	}

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
	if (UGrimElementalStatics::ApplyStimulusToActor(HitActor, Stimulus, Instigator))
	{
		UE_LOG(LogGrimMagic, Verbose, TEXT("%s → %s 자극 %s %.2f"), *Definition->GetName(), *HitActor->GetName(), *Stimulus.Element.ToString(), Stimulus.Intensity);
	}
}
