#include "Core/GrimAbilitySystemComponent.h"
#include "Magic/GrimCastFailure.h"
#include "Magic/GrimSpellDelivery.h"
#include "Engine/World.h"

UGrimAbilitySystemComponent::UGrimAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UGrimAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// 순회 중에 발동하면 목록이 바뀔 수 있으므로 핸들을 모은 뒤 발동한다.
	TArray<FGameplayAbilitySpecHandle> ToActivate;
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = true;
			if (Spec.IsActive())
			{
				AbilitySpecInputPressed(Spec);
			}
			else
			{
				ToActivate.Add(Spec.Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : ToActivate)
	{
		TryActivateAbility(Handle);
	}
}

void UGrimAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = false;
			if (Spec.IsActive())
			{
				AbilitySpecInputReleased(Spec);
			}
		}
	}
}

// ───── 마법 실패 알림(Q64) ─────

void UGrimAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);
	GrimCastFailure::HandleAbilityFailed(this, Handle, Ability);
}

void UGrimAbilitySystemComponent::Server_ReportCastFailed_Implementation(FGameplayAbilitySpecHandle Handle)
{
	// 클라이언트 말은 믿지 않는다 — 내 Spec 인지 보고 서버가 다시 판정한다.
	if (!FindAbilitySpecFromHandle(Handle))
	{
		return;
	}
	int32 Param = 0;
	const EGrimCastFailReason Reason = GrimCastFailure::Judge(this, Handle, Param);
	GrimCastFailure::Report(this, Reason, Param);
}

void UGrimAbilitySystemComponent::Client_CastFailed_Implementation(uint8 Reason, int32 Param)
{
	const UWorld* World = GetWorld();
	UE_LOG(LogGrimMagic, Display, TEXT("[%s] 마법 실패 알림 %s: %s"), World && World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"),
		GrimCastFailure::ReasonName(static_cast<EGrimCastFailReason>(Reason)), *GrimCastFailure::GetMessage(static_cast<EGrimCastFailReason>(Reason), Param).ToString());
	OnCastFailedNotice.Broadcast(Reason, Param);
}

bool UGrimAbilitySystemComponent::ShouldThrottleCastFailure(uint8 Reason, float IntervalSeconds)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Reason == LastCastFailureReason && LastCastFailureTime >= 0.0 && Now - LastCastFailureTime < IntervalSeconds)
	{
		return true;
	}
	LastCastFailureReason = Reason;
	LastCastFailureTime = Now;
	return false;
}

bool UGrimAbilitySystemComponent::ConsumeClientCastFailureReport(float IntervalSeconds)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (LastClientReportTime >= 0.0 && Now - LastClientReportTime < IntervalSeconds)
	{
		return false;
	}
	LastClientReportTime = Now;
	return true;
}
