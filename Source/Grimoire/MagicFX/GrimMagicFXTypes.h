#pragma once

#include "CoreMinimal.h"
#include "GrimMagicFXTypes.generated.h"

class UMaterialInterface;

/** 오른팔 시전 동작 (docs/systems/spell-feel 「시전 동작」). 전달 방식마다 하나. */
UENUM(BlueprintType)
enum class EGrimCastMotion : uint8
{
	/** 정하지 않음 — 마법의 전달 방식(Delivery) · 속성으로 고른다 */
	Auto,
	/** 튕기기 — Bolt(섬광) */
	Flick,
	/** 뻗고 유지 — Stream(줄기) */
	Extend,
	/** 옆으로 쓸기 — Cone(원뿔 파동) */
	Sweep
};

/** 색 펄스(빛나는 도형 + 점광원)의 모양. 엔진 기본 도형을 쓴다(임시 연출). */
UENUM(BlueprintType)
enum class EGrimFXPulseShape : uint8
{
	None,
	/** 구 — 섬광 · 터짐 */
	Sphere,
	/** 납작한 구 — 표면에 퍼지는 물보라. 법선 방향으로 눌린다 */
	Disc,
	/** 원뿔 — 꼭짓점이 시작점, 방향으로 퍼지는 파동 */
	Cone
};

/**
 * 짧게 커지며 사라지는 색 펄스 하나. 파티클(엔진 기본 템플릿)은 색을 바꿀 수 없어서,
 * 「색이 곧 정보」(spell-feel)를 이 펄스의 속성색 도형 · 빛이 맡는다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimFXPulseSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	EGrimFXPulseShape Shape = EGrimFXPulseShape::None;

	/** 시작 · 끝 크기(cm). 구 · 원반은 지름, 원뿔은 X = 끝 지름 · Z = 길이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	FVector StartSize = FVector(5.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	FVector EndSize = FVector(30.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "0.01", Units = "s"))
	float Lifetime = 0.15f;

	/** 발광 세기(머티리얼 Glow) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "0"))
	float Glow = 6.f;

	/** 0 = 가운데가 밝은 덩어리(불꽃), 1 = 테두리가 밝은 껍질(공기 고리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "0", ClampMax = "1"))
	float RimMix = 0.f;

	/** 점광원 세기(0 = 빛 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "0"))
	float LightIntensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "0", Units = "cm"))
	float LightRadius = 300.f;

	bool IsEnabled() const { return Shape != EGrimFXPulseShape::None; }
};

/** 맞은 곳 자국(그을음 · 젖음). 시간이 지나면 흐려져 사라진다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimImpactDecalSpec
{
	GENERATED_BODY()

	/** 데칼 머티리얼(비우면 자국 없음). 파라미터 Color · Roughness · Opacity 를 받는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal")
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal")
	FLinearColor Color = FLinearColor(0.02f, 0.015f, 0.01f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 0.85f;

	/** 지름(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (ClampMin = "1", Units = "cm"))
	float Size = 60.f;

	/** 남아 있는 시간(흐려지는 시간 포함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (ClampMin = "0.1", Units = "s"))
	float Lifetime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal", meta = (ClampMin = "0", Units = "s"))
	float FadeTime = 4.f;
};

/** 그리모어가 손 위에 떠서 오르내리는 모양 (R21). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimBookFloatParams
{
	GENERATED_BODY()

	/** 위아래 흔들림 반폭(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Float", meta = (ClampMin = "0", Units = "cm"))
	float BobAmplitude = 1.5f;

	/** 한 번 오르내리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Float", meta = (ClampMin = "0.1", Units = "s"))
	float BobPeriod = 2.f;

	/** 살짝 기울기(도) — 흔들림과 어긋난 주기로 좌우 · 앞뒤가 번갈아 기운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Float", meta = (ClampMin = "0", Units = "deg"))
	float TiltDegrees = 3.f;
};

/** 그리모어를 거둘지(Q25) 판정에 넣는 값. 책이 매 틱 로컬에서 모은다. */
struct FGrimGrimoireStowQuery
{
	bool bHasWand = true;
	bool bSwimming = false;
	bool bBuildMode = false;
	bool bCarrying = false;
	bool bMounted = false;
	bool bAiming = false;
	/** 마지막 시전(또는 조준)부터 지난 시간 */
	float SecondsSinceCombat = TNumericLimits<float>::Max();
	/** 탈것 위에서 시전 뒤 책을 띄워 두는 시간 */
	float MountedCombatHoldSeconds = 4.f;
};

/** 시전 동작 한 프레임 — 애님 인스턴스가 만들고 프록시가 뼈에 입힌다. 방향은 컴포넌트 공간(앞 +Y · 위 +Z · 오른쪽 −X). */
struct FGrimSpellPoseFrame
{
	/** 오른팔을 조준 자세로 끄는 정도 (0~1) */
	float RightArmWeight = 0.f;
	FVector RightUpperArmDir = FVector(0.f, 1.f, 0.f);
	FVector RightForearmDir = FVector(0.f, 1.f, 0.f);

	/** 상체(spine_03) 비틀기 · 숙이기 (도) */
	float SpineYawDegrees = 0.f;
	float SpinePitchDegrees = 0.f;

	/** 왼팔 「책 받치기」 정도 (0~1) — 책을 거두면 0 */
	float LeftHoldWeight = 0.f;
	FVector LeftUpperArmDir = FVector(0.f, 0.f, -1.f);
	FVector LeftForearmDir = FVector(0.f, 1.f, 0.f);
};
