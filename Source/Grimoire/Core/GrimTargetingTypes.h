#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "GrimTargetingTypes.generated.h"

/** 조준 대상의 종류. 소프트 락온 우선순위와 아군 제외에 쓴다 (R17). */
UENUM(BlueprintType)
enum class EGrimTargetKind : uint8
{
	None,
	Enemy,
	Creature,
	Object,
	Ally
};

/** 조준 방식 (R17 · R24). 투사체 유도 · 카메라 · UI 가 이것을 보고 달라진다. */
UENUM(BlueprintType)
enum class EGrimAimMode : uint8
{
	/** 대상 없음 — 화면 중앙(카메라 정면)으로 */
	Free,
	/** 소프트 락온 — 자동으로 잡힌 대상 */
	Soft,
	/** 고정 락온 — 플레이어가 고정한 대상 */
	Locked,
	/** 줌인 조준 — 플레이어가 직접 겨눈 지점(자동 대상 없음) */
	Zoom
};

/**
 * 시전 순간의 조준 결과. 조준 쪽(Targeting)이 만들고 마법 쪽(Magic)이 읽는다.
 * 두 시스템은 서로를 모르고 이 구조체와 IGrimAimProvider 로만 만난다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAimResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	EGrimAimMode Mode = EGrimAimMode::Free;

	/** 잡힌 대상. Free · Zoom 이면 비어 있다(Zoom 으로 겨눈 곳에 액터가 있으면 들어 있을 수 있다). */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	EGrimTargetKind TargetKind = EGrimTargetKind::None;

	/** 시선의 시작(카메라 위치) */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FVector ViewOrigin = FVector::ZeroVector;

	/** 겨눈 월드 지점. 대상이 있으면 대상의 조준점, 없으면 카메라 정면 트레이스가 닿은 곳(또는 사거리 끝). */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FVector AimPoint = FVector::ZeroVector;

	bool HasTarget() const { return Target.IsValid(); }
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimAimProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 지금의 조준을 알려 주는 쪽. 플레이어 컨트롤러(또는 그 컴포넌트)가 구현한다.
 * 로컬(소유 클라이언트)에서 부른다 — 서버는 이 결과를 TargetData 로 받아 검증한다.
 */
class GRIMOIRE_API IGrimAimProvider
{
	GENERATED_BODY()

public:
	/** MaxRange = 마법 사거리(cm). 그 안에서 고른 조준 결과. */
	virtual FGrimAimResult GetCurrentAim(float MaxRange) const = 0;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimTargetable : public UInterface
{
	GENERATED_BODY()
};

/** 소프트 락온에 잡힐 수 있는 것: 적 · 생물 · 상호작용 사물. 액터 자신 또는 그 컴포넌트 하나가 구현한다. */
class GRIMOIRE_API IGrimTargetable
{
	GENERATED_BODY()

public:
	/** 투사체가 노리는 월드 지점(보통 몸통 가운데) */
	virtual FVector GetTargetPoint() const = 0;

	virtual EGrimTargetKind GetTargetKind() const = 0;

	/** 지금 대상이 될 수 있는가(죽음 · 숨음 · 꺼진 사물 등은 false) */
	virtual bool CanBeTargetedBy(const AActor* Instigator) const { return true; }
};

UCLASS()
class GRIMOIRE_API UGrimTargetingStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 폰의 컨트롤러 → 그 컴포넌트 → 폰 → 그 컴포넌트 순으로 IGrimAimProvider 를 찾는다. 없으면 nullptr. */
	static IGrimAimProvider* FindAimProvider(const APawn* Pawn);

	/** 액터 또는 그 컴포넌트 중 IGrimTargetable 구현체. 없으면 nullptr. */
	static IGrimTargetable* FindTargetable(const AActor* Actor);
};
