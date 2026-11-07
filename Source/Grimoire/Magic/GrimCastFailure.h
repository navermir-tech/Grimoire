#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayCueNotify_Static.h"
#include "NativeGameplayTags.h"
#include "GrimCastFailure.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UFXSystemAsset;
class UGameplayAbility;

/**
 * 마법이 걸리지 않은 까닭(Q64 답 2026-11-07 — 왜 안 되는지 플레이어에게 알린다). 복제는 uint8 로(ASC RPC).
 * None 은 「알리지 않는다」(기본 공격 연타 간격 · 잠 · 기절처럼 따로 보이는 것, 주먹질이 대신 나가는 기본 공격).
 */
UENUM(BlueprintType)
enum class EGrimCastFailReason : uint8
{
	None,
	/** 완드 칸이 비었다(Q71) — 주먹만 */
	NoWand,
	/** 완드가 부러졌다(Q22) — 수리하거나 새로 산다 */
	WandBroken,
	/** 그 마법의 서클이 모자라다 — Param = 필요한 서클 */
	CircleTooLow,
	/** 쿨타임 — Param = 남은 초(올림) */
	Cooldown,
	/** 마나 부족 — Param = 필요한 마나(올림) */
	NotEnoughMana,
	/** 계약(파크툼): 데리고 다님 · 보관함 · 거점 생물 상자가 모두 가득 */
	TameNoRoom,
	/** 계약(파크툼): 그 생물과 계약할 서클이 모자라다 — Param = 필요한 서클 */
	TameCircleTooLow,
	/** 계약(파크툼): 계약을 받지 않는 생물(길들일 수 없음 · 이미 주인 있음 · 죽음) */
	TameRefused,
	/** 작업대 「수리」 탭 · 조각 하나씩 고치기가 잠김 — 레스티투오를 배워야(Q121 · Q123). 연출 없음 */
	RepairLocked,
	/** 거점 핵 「전체 수리」가 잠김 — 레스티투오 막시마를 배워야(Q121). 연출 없음 */
	BaseRepairLocked,
	/** 승급 자격(Q122): 숙련도 등급 조건이 모자라다 — Param = FGrimCircleMath::PackGradeParam(등급 번호, 필요, 지금). 연출 없음 */
	PromotionGrade,
	/** 승급 자격(Q122): 조건을 채운 마법 가운데 지금 서클의 마법이 없다 — Param = 지금 서클. 연출 없음 */
	PromotionCurrentCircle,
	/** 이미 표의 가장 높은 서클 — 더 오를 곳이 없다. 연출 없음 */
	PromotionAtMax,
	/** 장비 재료 등급이 서클보다 높아 입을 수 없다(Q122 해금) — Param = 입을 수 있게 되는 서클. 연출 없음 */
	EquipTierTooLow,
};

/** 서버 판정이 보는 값(순수 분류 — 테스트가 직접 채운다) */
struct FGrimCastFailInputs
{
	bool bBlocked = false;
	/** 기본 공격 — 완드가 없으면 같은 입력의 주먹질이 나가므로 알리지 않는다 */
	bool bBasicAttack = false;
	bool bWandMissing = false;
	bool bWandBroken = false;
	int32 Circle = 1;
	int32 RequiredCircle = 1;
	bool bOnCooldown = false;
	float CooldownRemaining = 0.f;
	bool bCanAfford = true;
	float ManaCost = 0.f;
};

namespace GrimCastFailureTags
{
	/** 마법 실패 연출(피식 꺼짐 — 호그와트 레거시 참고). 서버가 Execute → 모든 기기 */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Magic_Fizzle);
}

/**
 * 마법 실패 알림 · 연출 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimCastFailureSettings] (docs/systems/magic 「마법 실패 알림」).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Cast Failure"))
class GRIMOIRE_API UGrimCastFailureSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimCastFailureSettings* Get() { return GetDefault<UGrimCastFailureSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 서버: 같은 까닭을 다시 알리기까지(연타 · 꾹 누름에 알림 · 연출이 쏟아지지 않게) */
	UPROPERTY(Config, EditAnywhere, Category = "Notice", meta = (ClampMin = "0", Units = "s"))
	float NoticeIntervalSeconds = 0.75f;

	/** 소유 클라이언트: 서버에 판정을 묻는 최소 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Notice", meta = (ClampMin = "0", Units = "s"))
	float ClientReportIntervalSeconds = 0.2f;

	/** 실패 연출에 쓰는 몸짓 몽타주(비면 몸짓 없이 이펙트만 — 애니메이션 에셋이 생기면 채운다). 슬롯은 몽타주가 정한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Fizzle")
	TSoftObjectPtr<UAnimMontage> FizzleMontage;

	UPROPERTY(Config, EditAnywhere, Category = "Fizzle", meta = (ClampMin = "0.1"))
	float FizzleMontagePlayRate = 1.f;

	/** 완드 끝(완드가 없으면 손 · 계약은 생물)에서 피식 꺼지는 작은 이펙트 — 임시로 엔진 기본 템플릿 */
	UPROPERTY(Config, EditAnywhere, Category = "Fizzle", meta = (AllowedClasses = "/Script/Niagara.NiagaraSystem,/Script/Engine.ParticleSystem"))
	TSoftObjectPtr<UFXSystemAsset> FizzleFX;

	UPROPERTY(Config, EditAnywhere, Category = "Fizzle", meta = (ClampMin = "0"))
	float FizzleFXScale = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Fizzle")
	FLinearColor FizzleColor = FLinearColor(0.55f, 0.55f, 0.6f);

	/** 이펙트를 이 초 뒤에 멈춘다 */
	UPROPERTY(Config, EditAnywhere, Category = "Fizzle", meta = (ClampMin = "0", Units = "s"))
	float FizzleFXSeconds = 0.5f;

	/** 쿨타임 실패에도 연출을 한다(끄면 알림 글만) */
	UPROPERTY(Config, EditAnywhere, Category = "Fizzle")
	bool bFizzleOnCooldown = true;
};

/** 마법 실패(Q64) — 서버가 까닭을 정하고 소유 클라이언트에 알린다(HUD 글) · 모두에게 연출 큐. */
namespace GrimCastFailure
{
	/** 순수 분류: 막힘(잠 · 기절) → 기본 공격의 완드 없음(주먹질) → 완드 없음 · 부러짐 → 서클 → 쿨타임 → 마나 */
	GRIMOIRE_API EGrimCastFailReason Classify(const FGrimCastFailInputs& Inputs, int32& OutParam);

	/** HUD 알림 글(한국어 게임 문구) */
	GRIMOIRE_API FText GetMessage(EGrimCastFailReason Reason, int32 Param);

	/** 실패 연출(피식)을 하는 까닭인가 — 마법이 아닌 잠김(수리 해금)은 글만 */
	GRIMOIRE_API bool ShouldFizzle(EGrimCastFailReason Reason);

	GRIMOIRE_API const TCHAR* ReasonName(EGrimCastFailReason Reason);

	/** 이 ASC 의 이 Spec 이 왜 안 되나(마법 어빌리티만 — 아니면 None). 서버 · 소유 클라이언트 모두 같은 규칙 */
	GRIMOIRE_API EGrimCastFailReason Judge(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle Handle, int32& OutParam);

	/**
	 * 서버: 알린다 — 소유 클라이언트에 까닭(Reliable), 연출 큐(GameplayCue.Magic.Fizzle — 모두에게). 같은 까닭은 NoticeIntervalSeconds 안에 한 번.
	 * Location 이 있으면 그곳에서(계약 — 생물), 없으면 완드 끝 · 손.
	 */
	GRIMOIRE_API void Report(UAbilitySystemComponent* ASC, EGrimCastFailReason Reason, int32 Param, const FVector* Location = nullptr);

	/** ASC 의 NotifyAbilityFailed 에서: 서버면 판정 · 알림, 소유 클라이언트면(미리 걸러) 서버에 묻는다 */
	GRIMOIRE_API void HandleAbilityFailed(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle Handle, const UGameplayAbility* Ability);
}

/**
 * GameplayCue.Magic.Fizzle — 마법 실패 연출 자리(Q64). 큐 에셋 GC_Magic_Fizzle(이 클래스의 블루프린트 자식)이
 * /Game/Grimoire/Magic/Cues 에 있어야 큐 관리자가 찾는다(Scripts/Magic/make_fizzle_cue.py).
 * 임시 연출: 작은 회색 파티클(FizzleFX) + 몸짓 몽타주(FizzleMontage, 비면 건너뜀).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Grim Magic Fizzle Cue"))
class GRIMOIRE_API UGrimCueNotify_Fizzle : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGrimCueNotify_Fizzle(const FObjectInitializer& ObjectInitializer);

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
