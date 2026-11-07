#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GrimAbilitySystemComponent.generated.h"

/** 마법 실패 알림(Q64) — 까닭(EGrimCastFailReason) · 값. 소유 클라이언트(리슨 호스트는 서버)에서만 울린다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FGrimOnCastFailedNotice, uint8 /*Reason*/, int32 /*Param*/);

/**
 * 그리모리 공용 ASC. 플레이어는 AGrimPlayerState 가, 생물은 생물 자신이 소유한다.
 * 입력은 "입력 태그"로 받는다: 어빌리티를 줄 때 Spec.GetDynamicSpecSourceTags() 에 Input.* 태그를 넣으면
 * 그 태그가 눌렸을 때 발동된다. 캐릭터는 어떤 어빌리티가 있는지 몰라도 된다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGrimAbilitySystemComponent();

	/** 입력 태그가 눌림 — 로컬 컨트롤러에서 호출한다. 어빌리티 발동은 GAS 예측을 따른다. */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** 입력 태그가 떼어짐. */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// ───── 마법 실패 알림(Q64 — Magic/GrimCastFailure) ─────

	/** 발동 실패 — 서버는 판정해 알리고, 소유 클라이언트는 서버에 묻는다 */
	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;

	/** 소유 클라이언트 → 서버: 이 Spec 이 내 쪽에서 안 됐다 — 서버가 다시 판정한다 */
	UFUNCTION(Server, Unreliable)
	void Server_ReportCastFailed(FGameplayAbilitySpecHandle Handle);

	/** 서버 → 소유 클라이언트: 왜 안 됐는지(HUD 글) */
	UFUNCTION(Client, Reliable)
	void Client_CastFailed(uint8 Reason, int32 Param);

	FGrimOnCastFailedNotice OnCastFailedNotice;

	/** 서버: 같은 까닭이 간격 안에 다시 왔나(왔으면 true — 알리지 않는다). 아니면 기록한다 */
	bool ShouldThrottleCastFailure(uint8 Reason, float IntervalSeconds);

	/** 소유 클라이언트: 서버에 물어도 되나(간격) — 되면 기록한다 */
	bool ConsumeClientCastFailureReport(float IntervalSeconds);

private:
	double LastCastFailureTime = -1.0;
	uint8 LastCastFailureReason = 0;
	double LastClientReportTime = -1.0;
};
