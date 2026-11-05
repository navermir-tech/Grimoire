#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDelivery.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"

UGrimProgressionComponent::UGrimProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UAbilitySystemComponent* UGrimProgressionComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

UGrimSpellbookComponent* UGrimProgressionComponent::GetSpellbook() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetSpellbook() : nullptr;
}

int32 UGrimProgressionComponent::GetCircle() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC ? FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute())) : 0;
}

FGrimCircleCheck UGrimProgressionComponent::EvaluateNextCircle() const
{
	const UGrimSpellbookComponent* Spellbook = GetSpellbook();
	return UGrimProgressionSettings::Evaluate(UGrimProgressionSettings::Get()->CircleRequirements, GetCircle(),
		Spellbook ? Spellbook->GetTrackedSpellCount() : 0, Spellbook ? Spellbook->GetProficiencyGradeSum() : 0);
}

bool UGrimProgressionComponent::TryCircleUp(bool bForce)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const FGrimCircleCheck Check = EvaluateNextCircle();
	if (Check.bAtMax || (!Check.bMet && !bForce))
	{
		UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 승급 실패: 서클 %d, 다음 %s, 마법 %d/%d, 등급 합 %d/%d"), *GetNameSafe(GetOwner()), Check.CurrentCircle,
			Check.bAtMax ? TEXT("없음") : *FString::FromInt(Check.NextCircle), Check.KnownSpells, Check.RequiredKnownSpells, Check.GradeSum, Check.RequiredGradeSum);
		NotifyResult(Check.CurrentCircle, false);
		return false;
	}

	const int32 NewCircle = Check.NextCircle;
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), NewCircle);
	// TODO(레벨 묶음): 최대 마나는 서클에 묶지 않는다 — 캐릭터 레벨(HP · 마나 · 성장 트리 포인트)이 정한다. 지금은 값을 그대로 둔다.

	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 서클 %d → %d%s"), *GetNameSafe(GetOwner()), Check.CurrentCircle, NewCircle,
		(!Check.bMet && bForce) ? TEXT(" [강제]") : TEXT(""));
	NotifyResult(NewCircle, true);
	return true;
}

void UGrimProgressionComponent::Server_RequestCircleUp_Implementation(bool bForce)
{
#if UE_BUILD_SHIPPING
	bForce = false;
#endif
	TryCircleUp(bForce);
}

void UGrimProgressionComponent::RequestCircleUp(bool bForce)
{
	if (GetOwner()->HasAuthority())
	{
		TryCircleUp(bForce);
	}
	else
	{
		Server_RequestCircleUp(bForce);
	}
}

void UGrimProgressionComponent::NotifyResult(int32 Circle, bool bSuccess)
{
	OnCircleUpResult.Broadcast(this, Circle, bSuccess);
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PC && !PC->IsLocalController())
	{
		Client_CircleUpResult(Circle, bSuccess);
	}
}

void UGrimProgressionComponent::Client_CircleUpResult_Implementation(int32 Circle, bool bSuccess)
{
	OnCircleUpResult.Broadcast(this, Circle, bSuccess);
}
