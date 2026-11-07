#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimCastFailure.h"
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
	TArray<FGrimSpellGradeEntry> Entries;
	if (const UGrimSpellbookComponent* Spellbook = GetSpellbook())
	{
		Spellbook->GetProficiencyEntries(Entries);
	}
	TArray<FName> GradeOrder;
	UGrimProgressionSettings::GetGradeOrder(GradeOrder);
	return FGrimCircleMath::EvaluatePromotion(UGrimProgressionSettings::GetCircleTable(), GetCircle(), Entries, GradeOrder);
}

const FGrimCircleStep* UGrimProgressionComponent::GetCircleStep(int32 Circle)
{
	return UGrimProgressionSettings::GetCircleTable().FindClamped(Circle);
}

void UGrimProgressionComponent::ReportPromotionFailure(const FGrimCircleCheck& Check) const
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || Check.bMet)
	{
		return;
	}
	if (Check.bAtMax)
	{
		GrimCastFailure::Report(ASC, EGrimCastFailReason::PromotionAtMax, Check.CurrentCircle);
	}
	else if (const FGrimCircleGradeCheck* Unmet = Check.FirstUnmetGrade())
	{
		GrimCastFailure::Report(ASC, EGrimCastFailReason::PromotionGrade, FGrimCircleMath::PackGradeParam(Unmet->GradeIndex, Unmet->Required, Unmet->Have));
	}
	else if (!Check.IsCurrentCircleMet())
	{
		GrimCastFailure::Report(ASC, EGrimCastFailReason::PromotionCurrentCircle, Check.CurrentCircle);
	}
}

namespace GrimProgressionLocal
{
	FString DescribeCheck(const FGrimCircleCheck& Check)
	{
		FString Text;
		for (const FGrimCircleGradeCheck& Grade : Check.Grades)
		{
			Text += FString::Printf(TEXT(" %s이상 %d/%d"), *Grade.Grade.ToString(), Grade.Have, Grade.Required);
		}
		Text += FString::Printf(TEXT(" 지금서클마법 %d/%d"), Check.CurrentCircleSpells, Check.RequiredCurrentCircleSpells);
		return Text;
	}
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
		UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 승급 실패: 서클 %d, 다음 %s,%s"), *GetNameSafe(GetOwner()), Check.CurrentCircle,
			Check.bAtMax ? TEXT("없음") : *FString::FromInt(Check.NextCircle), *GrimProgressionLocal::DescribeCheck(Check));
		ReportPromotionFailure(Check);
		NotifyResult(Check.CurrentCircle, false);
		return false;
	}

	const int32 NewCircle = Check.NextCircle;
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), NewCircle);

	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 서클 %d → %d%s"), *GetNameSafe(GetOwner()), Check.CurrentCircle, NewCircle,
		(!Check.bMet && bForce) ? TEXT(" [강제]") : TEXT(""));

	// 서클 보상 중 마법서 몫: 봉인된 책 풀기 · 새 슬롯 세트 채우기. 승급 알림이 마지막 토스트가 되게 먼저 한다.
	if (UGrimSpellbookComponent* Spellbook = GetSpellbook())
	{
		Spellbook->HandleCircleRaised(NewCircle);
	}
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

// ───────────────────────── 세이브 (Magic.Progression v1) ─────────────────────────

void UGrimProgressionComponent::WriteSaveData(FArchive& Ar)
{
	const UAbilitySystemComponent* ASC = GetASC();
	float Circle = ASC ? ASC->GetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute()) : 0.f;
	Ar << Circle;
}

void UGrimProgressionComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	float Circle = 0.f;
	Ar << Circle;
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || Ar.IsError() || Circle < 1.f)
	{
		return;
	}
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), Circle);
	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 세이브 읽음: 서클 %d"), *GetNameSafe(GetOwner()), FMath::RoundToInt(Circle));
}
