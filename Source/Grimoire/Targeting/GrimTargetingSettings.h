#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "GrimTargetingSettings.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimTargeting, Log, All);

/** 소프트 락온 원뿔의 기준 (호그와트 레거시의 Camera Relative Targeting 옵션). */
UENUM(BlueprintType)
enum class EGrimTargetingReference : uint8
{
	/** 카메라 정면 원뿔 — 화면 중앙에 가까운 대상 */
	Camera,
	/** 이동 입력(왼쪽 스틱 · WASD) 방향 원뿔. 입력이 없으면 캐릭터가 보는 쪽 */
	Movement
};

/**
 * 소프트 락온 점수 가중치 (docs/systems/magic 「소프트 락온」).
 * 점수 = Center × (1 − 각도/반각) + Distance × (1 − 거리/사거리) + Kind × 종류 가중 + Keep × (현재 대상이면 1)
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTargetScoreWeights
{
	GENERATED_BODY()

	/** 화면 중앙(원뿔 축)에 가까울수록 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Score")
	float Center = 1.f;

	/** 가까울수록 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Score")
	float Distance = 0.5f;

	/** 종류 가중(아래 *Kind 값)에 곱한다 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Score")
	float Kind = 1.f;

	/** 지금 대상 유지 가산(자주 바뀌지 않게) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Score")
	float Keep = 0.3f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Kind")
	float EnemyKind = 1.f;

	/** 적이 아닌 생물 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Kind")
	float CreatureKind = 0.4f;

	/** 지금 고른 마법의 속성에 반응하는 사물(불씨 → 꺼진 양초) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Kind")
	float ReactiveObjectKind = 0.5f;

	/** 그 밖의 사물 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Kind")
	float ObjectKind = 0.1f;
};

/** 속성 → 표식 색 한 줄 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementColor
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Marker", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** sRGB */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Marker")
	FColor Color = FColor::White;
};

/**
 * 조준 · 소프트 락온 · 고정 락온 설정 (R17 · Q17 · Q18).
 * 값은 DefaultGame.ini 의 [/Script/Grimoire.GrimTargetingSettings] 로 덮어쓴다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Targeting"))
class GRIMOIRE_API UGrimTargetingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGrimTargetingSettings();

	// --- 옵션 기본값 (플레이어 옵션 화면이 생기면 거기서 덮어쓴다) ---

	/** 소프트 락온 켜짐(Q18 — 끌 수 있다). 꺼도 고정 락온은 쓸 수 있다 */
	UPROPERTY(Config, EditAnywhere, Category = "Options")
	bool bSoftLockEnabledByDefault = true;

	UPROPERTY(Config, EditAnywhere, Category = "Options")
	EGrimTargetingReference DefaultReference = EGrimTargetingReference::Camera;

	// --- 선정 ---

	/** 후보를 다시 고르는 간격(초). 매 프레임이 아니다 */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "0.01", Units = "s"))
	float SelectionInterval = 0.05f;

	/** 원뿔 반각(도) */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "1", ClampMax = "89"))
	float ConeHalfAngleDegrees = 25.f;

	/** 지금 대상은 반각을 이만큼 더 넓게 본다 — 원뿔 가장자리에서 깜빡이지 않게 */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "0"))
	float CurrentTargetExtraHalfAngleDegrees = 5.f;

	/** 선택된 마법이 없거나 사거리가 0 일 때의 선정 거리(cm) */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "100", Units = "cm"))
	float DefaultSelectionRange = 2500.f;

	/** 원뿔을 통과한 후보 상한(점수 순으로 자른다) */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "1"))
	int32 MaxCandidates = 32;

	/** 시야(가림) 트레이스는 점수 상위 이만큼만 */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "1"))
	int32 MaxVisibilityTraces = 8;

	/** 지금 대상이 이 시간보다 오래 가려지면 풀린다(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Selection", meta = (ClampMin = "0", Units = "s"))
	float OcclusionGraceSeconds = 0.3f;

	/** 보이는 적이 하나라도 있으면 사물보다 먼저 잡는다(전투 중 적 우선) */
	UPROPERTY(Config, EditAnywhere, Category = "Selection")
	bool bEnemiesBeforeObjects = true;

	UPROPERTY(Config, EditAnywhere, Category = "Selection")
	FGrimTargetScoreWeights Weights;

	/** 후보를 모으는 구 오버랩의 오브젝트 종류. 대상은 이 중 하나로 충돌(쿼리)이 켜져 있어야 한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Selection")
	TArray<TEnumAsByte<ECollisionChannel>> CandidateObjectChannels;

	/** 시야 트레이스 채널 */
	UPROPERTY(Config, EditAnywhere, Category = "Selection")
	TEnumAsByte<ECollisionChannel> VisibilityChannel = ECC_Visibility;

	// --- 고정 락온 (Q17) ---

	/** 고정 대상이 이보다 멀어지면 풀린다(cm). 마법 사거리가 더 길면 그쪽 */
	UPROPERTY(Config, EditAnywhere, Category = "Lock", meta = (ClampMin = "100", Units = "cm"))
	float LockedMaxRange = 3000.f;

	/** 고정 대상이 이 시간보다 오래 가려지면 풀린다(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Lock", meta = (ClampMin = "0", Units = "s"))
	float LockedOcclusionGraceSeconds = 1.f;

	/** 카메라가 고정 대상을 따라가는 보간 속도(RInterpTo) */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Camera", meta = (ClampMin = "0"))
	float LockCameraInterpSpeed = 8.f;

	/** 대상을 바라보는 피치에 더하는 값(도). 음수면 조금 내려다봐 대상이 화면 위쪽에 온다 */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Camera")
	float LockCameraPitchOffset = -5.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lock|Camera")
	float LockCameraMinPitch = -60.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lock|Camera")
	float LockCameraMaxPitch = 40.f;

	/** 고정 중 시점 입력(마우스 · 오른쪽 스틱)을 이만큼 모으면 그 방향 옆 대상으로 넘긴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Switch", meta = (ClampMin = "0.01"))
	float LockSwitchInputThreshold = 6.f;

	/** 시점 입력이 이 시간 동안 없으면 모은 값을 버린다(초) — 천천히 흘린 입력은 넘김이 아니다 */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Switch", meta = (ClampMin = "0", Units = "s"))
	float LockSwitchInputResetSeconds = 0.2f;

	/** 넘긴 뒤 다음 넘김까지(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Switch", meta = (ClampMin = "0", Units = "s"))
	float LockSwitchCooldownSeconds = 0.35f;

	/** 입력 방향과 화면상 후보 방향의 허용 각도(도) */
	UPROPERTY(Config, EditAnywhere, Category = "Lock|Switch", meta = (ClampMin = "1", ClampMax = "180"))
	float LockSwitchMaxAngleDegrees = 60.f;

	// --- 대상 표식 (임시: 디버그 선 그리기. 정식 위젯은 HUD 작업 때) ---

	/** 대상 조준점에서 표식 링까지의 높이(cm) */
	UPROPERTY(Config, EditAnywhere, Category = "Marker", meta = (Units = "cm"))
	float MarkerHeightOffset = 0.f;

	/** 표식 링 반지름(cm) = MarkerRadius + 카메라 거리 × MarkerRadiusPerDistance */
	UPROPERTY(Config, EditAnywhere, Category = "Marker", meta = (ClampMin = "1", Units = "cm"))
	float MarkerRadius = 20.f;

	UPROPERTY(Config, EditAnywhere, Category = "Marker", meta = (ClampMin = "0"))
	float MarkerRadiusPerDistance = 0.012f;

	UPROPERTY(Config, EditAnywhere, Category = "Marker", meta = (ClampMin = "0"))
	float MarkerThickness = 1.5f;

	/** 고정 락온이면 바깥에 링을 하나 더 — 이 배율 */
	UPROPERTY(Config, EditAnywhere, Category = "Marker", meta = (ClampMin = "1"))
	float LockedMarkerRadiusScale = 1.4f;

	/** 속성색이 없을 때(sRGB) */
	UPROPERTY(Config, EditAnywhere, Category = "Marker")
	FColor MarkerDefaultColor = FColor::White;

	/** 속성(Element.*) → 표식 색. 트레일 · 피격과 같은 속성색(spell-feel) */
	UPROPERTY(Config, EditAnywhere, Category = "Marker")
	TArray<FGrimElementColor> ElementColors;

	/** 속성의 표식 색(부모 태그까지 찾는다). 없으면 MarkerDefaultColor */
	FColor GetElementColor(const FGameplayTag& Element) const;
};
