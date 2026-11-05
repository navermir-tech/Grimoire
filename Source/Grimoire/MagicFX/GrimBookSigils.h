#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimBookSigils.generated.h"

/** 문양 조각 모양 — 엔진 기본 도형을 납작하게 눌러 쓴다(임시, 그림 텍스처 없음) */
UENUM(BlueprintType)
enum class EGrimSigilShape : uint8
{
	/** 네모 · 막대(Cube) */
	Square,
	/** 원 · 타원(Cylinder) */
	Disc,
	/** 세모(Cone 을 눕혀 누름) — 꼭짓점이 +X(책등 위쪽) */
	Triangle
};

/** 문양 조각 하나. 좌표는 책 면 가운데 기준(cm): X = 책등을 따라 위, Y = 면 가로 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSigilPiece
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	EGrimSigilShape Shape = EGrimSigilShape::Square;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	FVector2D Offset = FVector2D::ZeroVector;

	/** 크기(cm): X = 세로(세모는 높이), Y = 가로(세모는 밑변) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	FVector2D Size = FVector2D(2.f, 2.f);

	/** 면 위에서 돌리는 각(도). + = +Y 쪽으로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	float Yaw = 0.f;
};

/** 속성 문양 하나 (R21 — 책 면에는 마법마다가 아니라 속성 문양만) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimBookSigil
{
	GENERATED_BODY()

	/** 속성 태그 이름(Element.*). 하위 태그(Element.Water.Ice)가 따로 없으면 위 이름(Element.Water)의 문양을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	FName Element;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sigil")
	TArray<FGrimSigilPiece> Pieces;
};

/**
 * 그리모어 책 면의 속성 문양 11개 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimBookSigilSettings].
 * 시전(GC_Spell_Cast)마다 넘어간 면에 그 마법 속성의 문양이 그 마법 VisualSet 의 ElementColor 로 번쩍인다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Book Sigils"))
class GRIMOIRE_API UGrimBookSigilSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimBookSigilSettings* Get() { return GetDefault<UGrimBookSigilSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 이 이름의 문양(없으면 위 이름 → FallbackElement). 없으면 nullptr */
	const FGrimBookSigil* FindSigil(FName ElementTagName) const;

	UPROPERTY(Config, EditAnywhere, Category = "Sigil")
	TArray<FGrimBookSigil> Sigils;

	/** 속성을 모르거나 표에 없을 때 */
	UPROPERTY(Config, EditAnywhere, Category = "Sigil")
	FName FallbackElement = TEXT("Element.None");

	/** 조각 두께(cm) */
	UPROPERTY(Config, EditAnywhere, Category = "Look", meta = (ClampMin = "0.01", Units = "cm"))
	float PieceThickness = 0.12f;

	/** 번쩍임이 끝난 뒤 남는 빛 */
	UPROPERTY(Config, EditAnywhere, Category = "Look", meta = (ClampMin = "0"))
	float IdleGlow = 0.4f;

	/** 넘김 빛(책 FlipGlowPeak 곡선)에 곱하는 배율 */
	UPROPERTY(Config, EditAnywhere, Category = "Look", meta = (ClampMin = "0"))
	float FlashGlowScale = 1.f;
};

/** 문양의 순수 계산 (자동화 테스트 Grimoire.MagicFX.Sigil.*) */
namespace GrimBookSigilMath
{
	/** 책 하나에 둔 조각 칸 수 — 문양 하나의 조각이 이보다 많으면 뒤는 버린다 */
	constexpr int32 MaxPieces = 6;

	/**
	 * 태그 이름에 맞는 문양 번호: 같은 이름 → 점으로 끊긴 가장 긴 앞 이름(Element.Water 는 Element.Water.Ice 에도) → FallbackName.
	 * 없으면 INDEX_NONE. 「Element.Wat」 처럼 점 경계가 아닌 앞부분은 맞지 않는다.
	 */
	GRIMOIRE_API int32 FindSigilIndex(TConstArrayView<FGrimBookSigil> Sigils, FName ElementTagName, FName FallbackName);

	/**
	 * 조각 → 면 기준 변환(면 위 0 높이에 바닥이 닿는다). 엔진 기본 도형(100cm)의 스케일 · 회전.
	 * Square = Cube, Disc = Cylinder(축 Z), Triangle = Cone 을 눕혀(높이 축 → +X) 두께 쪽으로 누름.
	 */
	GRIMOIRE_API FTransform MakePieceTransform(const FGrimSigilPiece& Piece, float Thickness);
}
