#include "Magic/GrimCastFailure.h"
#include "Magic/GrimBasicAttackAbility.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimWand.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Particles/ParticleSystem.h"

#define LOCTEXT_NAMESPACE "GrimCastFailure"

namespace GrimCastFailureTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Magic_Fizzle, "GameplayCue.Magic.Fizzle", "마법 실패 연출(Q64) — 완드 끝 · 손에서 피식 꺼짐, 몸짓 몽타주 자리");
}

EGrimCastFailReason GrimCastFailure::Classify(const FGrimCastFailInputs& Inputs, int32& OutParam)
{
	OutParam = 0;
	// 잠 · 기절 · 쓰러짐은 화면이 이미 말해 준다.
	if (Inputs.bBlocked)
	{
		return EGrimCastFailReason::None;
	}
	if (Inputs.bWandMissing || Inputs.bWandBroken)
	{
		// 기본 공격 입력은 같은 키의 주먹질이 대신 나간다(Q71) — 실패가 아니다.
		if (Inputs.bBasicAttack)
		{
			return EGrimCastFailReason::None;
		}
		return Inputs.bWandBroken ? EGrimCastFailReason::WandBroken : EGrimCastFailReason::NoWand;
	}
	if (Inputs.Circle < Inputs.RequiredCircle)
	{
		OutParam = Inputs.RequiredCircle;
		return EGrimCastFailReason::CircleTooLow;
	}
	if (Inputs.bOnCooldown)
	{
		OutParam = FMath::Max(1, FMath::CeilToInt(Inputs.CooldownRemaining));
		return EGrimCastFailReason::Cooldown;
	}
	if (!Inputs.bCanAfford)
	{
		OutParam = FMath::Max(1, FMath::CeilToInt(Inputs.ManaCost));
		return EGrimCastFailReason::NotEnoughMana;
	}
	// 기본 공격 연타 간격 같은 것 — 알리지 않는다.
	return EGrimCastFailReason::None;
}

FText GrimCastFailure::GetMessage(EGrimCastFailReason Reason, int32 Param)
{
	switch (Reason)
	{
	case EGrimCastFailReason::NoWand: return LOCTEXT("NoWand", "완드가 없다 — 주먹만 나간다. 마법탑에서 기본 완드를 살 수 있다");
	case EGrimCastFailReason::WandBroken: return LOCTEXT("WandBroken", "완드가 부러졌다 — 「수리」로 고치거나 마법탑에서 새 완드를 산다");
	case EGrimCastFailReason::CircleTooLow: return FText::Format(LOCTEXT("CircleTooLow", "{0}서클 마법이다 — 아직 다룰 수 없다"), Param);
	case EGrimCastFailReason::Cooldown: return FText::Format(LOCTEXT("Cooldown", "아직 다시 쓸 수 없다 — {0}초"), Param);
	case EGrimCastFailReason::NotEnoughMana: return FText::Format(LOCTEXT("NotEnoughMana", "마나가 모자라다 (필요 {0})"), Param);
	case EGrimCastFailReason::TameNoRoom: return LOCTEXT("TameNoRoom", "데리고 다님 · 보관함 · 생물 상자가 모두 가득 — 계약이 걸리지 않는다");
	case EGrimCastFailReason::TameCircleTooLow: return FText::Format(LOCTEXT("TameCircleTooLow", "이 생물과 계약하려면 {0}서클이 필요하다"), Param);
	case EGrimCastFailReason::TameRefused: return LOCTEXT("TameRefused", "이 생물은 계약을 받지 않는다");
	default: return FText::GetEmpty();
	}
}

bool GrimCastFailure::ShouldFizzle(EGrimCastFailReason Reason)
{
	if (Reason == EGrimCastFailReason::None)
	{
		return false;
	}
	return Reason != EGrimCastFailReason::Cooldown || UGrimCastFailureSettings::Get()->bFizzleOnCooldown;
}

const TCHAR* GrimCastFailure::ReasonName(EGrimCastFailReason Reason)
{
	switch (Reason)
	{
	case EGrimCastFailReason::None: return TEXT("None");
	case EGrimCastFailReason::NoWand: return TEXT("NoWand");
	case EGrimCastFailReason::WandBroken: return TEXT("WandBroken");
	case EGrimCastFailReason::CircleTooLow: return TEXT("CircleTooLow");
	case EGrimCastFailReason::Cooldown: return TEXT("Cooldown");
	case EGrimCastFailReason::NotEnoughMana: return TEXT("NotEnoughMana");
	case EGrimCastFailReason::TameNoRoom: return TEXT("TameNoRoom");
	case EGrimCastFailReason::TameCircleTooLow: return TEXT("TameCircleTooLow");
	case EGrimCastFailReason::TameRefused: return TEXT("TameRefused");
	default: return TEXT("?");
	}
}

EGrimCastFailReason GrimCastFailure::Judge(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle Handle, int32& OutParam)
{
	OutParam = 0;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	const UGrimSpellAbility* Ability = Spec ? Cast<UGrimSpellAbility>(Spec->Ability) : nullptr;
	const FGameplayAbilityActorInfo* ActorInfo = ASC ? ASC->AbilityActorInfo.Get() : nullptr;
	const UGrimSpellDefinition* Definition = Ability && ActorInfo ? UGrimSpellAbility::GetSpellDefinition(Handle, ActorInfo) : nullptr;
	if (!Definition)
	{
		return EGrimCastFailReason::None;
	}

	FGrimCastFailInputs Inputs;
	Inputs.bBlocked = UGrimSpellAbility::HasCastBlockingTag(ASC);
	Inputs.bBasicAttack = Ability->IsA<UGrimBasicAttackAbility>();
	Inputs.bWandBroken = UGrimSpellAbility::IsWandBroken(ASC);
	Inputs.bWandMissing = !Inputs.bWandBroken && !UGrimSpellAbility::GetWand(ActorInfo);
	Inputs.Circle = FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()));
	Inputs.RequiredCircle = Definition->Circle;
	Inputs.bOnCooldown = !Ability->CheckCooldown(Handle, ActorInfo, nullptr);
	if (Inputs.bOnCooldown && Definition->CooldownTag.IsValid())
	{
		for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Definition->CooldownTag))))
		{
			Inputs.CooldownRemaining = FMath::Max(Inputs.CooldownRemaining, Remaining);
		}
	}
	Inputs.bCanAfford = Ability->CheckCost(Handle, ActorInfo, nullptr);
	if (!Inputs.bCanAfford)
	{
		Inputs.ManaCost = UGrimSpellAbility::GetEffectiveManaCost(Definition, UGrimSpellAbility::GetWand(ActorInfo), UGrimSpellAbility::GetCasterMultipliers(Definition, ASC).ManaCost);
	}
	return Classify(Inputs, OutParam);
}

namespace GrimCastFailureLocal // 유니티 빌드 이름 충돌 방지
{
	/** 연출 자리: 쥔 완드 끝, 없으면 손 높이 앞 */
	FVector FizzleLocation(UAbilitySystemComponent* ASC)
	{
		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
		if (const AGrimWand* Wand = UGrimSpellAbility::GetWand(ActorInfo))
		{
			return Wand->GetMuzzleTransform().GetLocation();
		}
		const AActor* Avatar = ASC->GetAvatarActor();
		return Avatar ? Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 45.f + FVector(0.f, 0.f, 25.f) : FVector::ZeroVector;
	}
}

void GrimCastFailure::Report(UAbilitySystemComponent* ASC, EGrimCastFailReason Reason, int32 Param, const FVector* Location)
{
	if (Reason == EGrimCastFailReason::None || !ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}
	const UGrimCastFailureSettings* Settings = UGrimCastFailureSettings::Get();
	UGrimAbilitySystemComponent* GrimASC = Cast<UGrimAbilitySystemComponent>(ASC);
	if (GrimASC && GrimASC->ShouldThrottleCastFailure(static_cast<uint8>(Reason), Settings->NoticeIntervalSeconds))
	{
		return;
	}
	UE_LOG(LogGrimMagic, Display, TEXT("[Server] 마법 실패 %s %s 값=%d"), *GetNameSafe(ASC->GetOwner()), ReasonName(Reason), Param);

	// 1) 소유 클라이언트에 까닭(리슨 호스트 자신이면 바로 돈다).
	if (GrimASC)
	{
		GrimASC->Client_CastFailed(static_cast<uint8>(Reason), Param);
	}

	// 2) 연출 — 모든 기기(다른 플레이어도 피식 꺼지는 것을 본다).
	if (ShouldFizzle(Reason))
	{
		AActor* Avatar = ASC->GetAvatarActor();
		FGameplayCueParameters Parameters;
		Parameters.Instigator = Avatar;
		Parameters.EffectCauser = Avatar;
		Parameters.Location = Location ? *Location : GrimCastFailureLocal::FizzleLocation(ASC);
		Parameters.Normal = Avatar ? Avatar->GetActorForwardVector() : FVector::ForwardVector;
		Parameters.RawMagnitude = static_cast<float>(Reason);
		ASC->ExecuteGameplayCue(GrimCastFailureTags::GameplayCue_Magic_Fizzle, Parameters);
	}
}

void GrimCastFailure::HandleAbilityFailed(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle Handle, const UGameplayAbility* Ability)
{
	// 마법 어빌리티만(주먹질 · 선택 어빌리티의 실패는 알릴 것이 아니다).
	if (!ASC || !Cast<UGrimSpellAbility>(Ability))
	{
		return;
	}
	int32 Param = 0;
	const EGrimCastFailReason Reason = Judge(ASC, Handle, Param);
	if (ASC->IsOwnerActorAuthoritative())
	{
		Report(ASC, Reason, Param);
		return;
	}
	// 소유 클라이언트: 내 복제 값으로 미리 걸러(연타 간격 등 None 은 묻지 않는다) 서버에 판정을 맡긴다 — 알림 글은 서버 판정으로만.
	UGrimAbilitySystemComponent* GrimASC = Cast<UGrimAbilitySystemComponent>(ASC);
	if (Reason != EGrimCastFailReason::None && GrimASC && GrimASC->ConsumeClientCastFailureReport(UGrimCastFailureSettings::Get()->ClientReportIntervalSeconds))
	{
		GrimASC->Server_ReportCastFailed(Handle);
	}
}

// ───── 큐 ─────

UGrimCueNotify_Fizzle::UGrimCueNotify_Fizzle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCueTag = GrimCastFailureTags::GameplayCue_Magic_Fizzle;
}

bool UGrimCueNotify_Fizzle::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const UGrimCastFailureSettings* Settings = UGrimCastFailureSettings::Get();
	AActor* Avatar = MyTarget ? MyTarget : Parameters.Instigator.Get();
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	const EGrimCastFailReason Reason = static_cast<EGrimCastFailReason>(FMath::Clamp(FMath::RoundToInt(Parameters.RawMagnitude), 0, 255));

	// 임시 연출: 작은 회색 파티클(엔진 기본 템플릿). 이펙트 갈래가 진짜 「피식」을 만들면 FizzleFX 만 바꾼다.
	UFXSystemAsset* FX = Settings->FizzleFX.LoadSynchronous();
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Parameters.Normal.IsNearlyZero() ? FVector::UpVector : FVector(Parameters.Normal)).Rotator();
	const bool bSpawned = FX && UGrimMagicFXStatics::SpawnFX(World, FX, Parameters.Location, Rotation, Settings->FizzleFXScale, Settings->FizzleColor, true, Settings->FizzleFXSeconds) != nullptr;

	// 몸짓 자리(호그와트 레거시의 완드 털기). 몽타주가 비면 건너뛴다 — 애니메이션 에셋이 생기면 ini 에 넣는다.
	bool bMontage = false;
	if (UAnimMontage* Montage = Settings->FizzleMontage.LoadSynchronous())
	{
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			bMontage = Character->PlayAnimMontage(Montage, Settings->FizzleMontagePlayRate) > 0.f;
		}
	}
	UE_LOG(LogGrimMagic, Display, TEXT("[%s] 마법 실패 연출 %s %s at %s 이펙트=%d 몽타주=%d"), World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"),
		*GetNameSafe(Avatar), GrimCastFailure::ReasonName(Reason), *Parameters.Location.ToCompactString(), bSpawned ? 1 : 0, bMontage ? 1 : 0);
	return true;
}

#undef LOCTEXT_NAMESPACE
