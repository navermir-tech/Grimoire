#include "Magic/GrimBasicAttackAbility.h"
#include "Magic/GrimBasicAttackProjectile.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimSpellFXTypes.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace GrimBasicAttackLocal // 유니티 빌드에서 다른 파일의 도우미와 부딪히지 않게
{
	const TCHAR* RoleName(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const bool bLocal = ActorInfo && ActorInfo->IsLocallyControlled();
		const bool bAuthority = ActorInfo && ActorInfo->IsNetAuthority();
		return bLocal && bAuthority ? TEXT("Host") : (bAuthority ? TEXT("Server") : TEXT("Client"));
	}

	double NowOf(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
		return World ? World->GetTimeSeconds() : 0.0;
	}
}

UGrimBasicAttackAbility::UGrimBasicAttackAbility()
{
	// 부모와 같다(InstancedPerActor · LocalPredicted) — 연타 상태를 인스턴스가 가진다.
}

int32 UGrimBasicAttackAbility::GetVisibleComboStep(const UAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->GetWorld())
	{
		return 0;
	}
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (const UGrimBasicAttackAbility* Instance = Cast<UGrimBasicAttackAbility>(Spec.GetPrimaryInstance()))
		{
			return FGrimBasicAttackMath::VisibleStep(Instance->Combo, ASC->GetWorld()->GetTimeSeconds(), UGrimBasicAttackSettings::Get()->ComboWindow);
		}
	}
	return 0;
}

void UGrimBasicAttackAbility::KeepComboAlive(const UAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->GetWorld())
	{
		return;
	}
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (UGrimBasicAttackAbility* Instance = Cast<UGrimBasicAttackAbility>(Spec.GetPrimaryInstance()))
		{
			FGrimBasicAttackMath::KeepAlive(Instance->Combo, ASC->GetWorld()->GetTimeSeconds(), UGrimBasicAttackSettings::Get()->ComboWindow);
			return;
		}
	}
}

bool UGrimBasicAttackAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	using namespace GrimBasicAttackLocal;
	if (!ActorInfo)
	{
		return false;
	}

	// 연타 리듬(쿨다운이 아님). 인스턴스가 상태를 가진다 — CDO 로 묻는 경우(주기 전 검사 등)는 건너뛴다.
	if (IsInstantiated())
	{
		const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
		const bool bRemoteOnServer = ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
		if (!FGrimBasicAttackMath::IsReady(NowOf(ActorInfo), Combo.LastAttackTime, Settings->AttackInterval, bRemoteOnServer ? Settings->ServerIntervalTolerance : 0.f))
		{
			if (bRemoteOnServer)
			{
				UE_LOG(LogGrimMagic, Display, TEXT("[Server] 기본 공격 거절 — 리듬보다 빠름(%.3fs)"), NowOf(ActorInfo) - Combo.LastAttackTime);
			}
			return false;
		}
	}

	// 부모: 정의 · 완드 · 서클 · 시전 차단 태그. 완드가 없으면 여기서 막히고, 같은 입력의 주먹질(UGrimFistAbility)이 나간다(Q4).
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGrimBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	using namespace GrimBasicAttackLocal;
	const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
	const int32 Step = FGrimBasicAttackMath::Register(Combo, NowOf(ActorInfo), Settings->ComboWindow, Settings->ComboLength);
	bCurrentHeavy = FGrimBasicAttackMath::IsHeavy(Step, Settings->ComboLength);
	UE_LOG(LogGrimMagic, Display, TEXT("[%s] 기본 공격 %d/%d%s"), RoleName(ActorInfo), Step, Settings->ComboLength, bCurrentHeavy ? TEXT(" 강타") : TEXT(""));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 원격 시전자: 가짜 볼트까지 쏘았으면 바로 끝낸다. 끝 RPC 는 조준(TargetData) RPC 뒤에 가므로 서버는 그때 이미 쏠 수 있고,
	// 판정 전에 끝이 와도 부모 EndAbility 가 한 번은 쏜다. 그래서 다음 발이 서버의 끝 복제(왕복 지연)를 기다리지 않는다.
	if (IsActive() && ActorInfo->IsLocallyControlled() && !ActorInfo->IsNetAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

FGameplayEffectSpecHandle UGrimBasicAttackAbility::MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const
{
	FGameplayEffectSpecHandle Spec = Super::MakeDamageSpec(Definition, Wand);
	if (bCurrentHeavy && Spec.IsValid())
	{
		const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
		const float Base = Spec.Data->GetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, false, 0.f);
		Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage,
			Base * FGrimBasicAttackMath::DamageMultiplier(Settings->ComboLength, Settings->ComboLength, Settings->HeavyDamageMultiplier));
	}
	return Spec;
}

void UGrimBasicAttackAbility::OnSpellProjectileSpawning(AGrimSpellProjectile* Projectile) const
{
	if (!bCurrentHeavy)
	{
		return;
	}
	if (AGrimBasicAttackProjectile* Bolt = Cast<AGrimBasicAttackProjectile>(Projectile))
	{
		const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
		Bolt->SetHeavy(Settings->HeavyKnockbackSpeed, Settings->HeavyKnockbackUpSpeed, Settings->HeavyPhysicsSpeed, Settings->HeavyBoltScale);
	}
}
