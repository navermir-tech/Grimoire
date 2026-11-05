#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "GrimMagicFXSettings.generated.h"

class UGrimSpellVisualSet;
class AGrimGrimoireBook;
class UMaterialInterface;

/**
 * 연출 · 동작(MagicFX) 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimMagicFXSettings].
 * 프로젝트 설정 > Game > Grimoire Magic FX 에서도 고칠 수 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Magic FX"))
class GRIMOIRE_API UGrimMagicFXSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimMagicFXSettings* Get() { return GetDefault<UGrimMagicFXSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 마법 정의에 VisualSet 이 없을 때 속성(Element.*)으로 고르는 기본 연출 묶음 */
	UPROPERTY(Config, EditAnywhere, Category = "VisualSet", meta = (Categories = "Element"))
	TMap<FGameplayTag, TSoftObjectPtr<UGrimSpellVisualSet>> DefaultVisualSets;

	/** 속성도 모를 때 */
	UPROPERTY(Config, EditAnywhere, Category = "VisualSet")
	TSoftObjectPtr<UGrimSpellVisualSet> FallbackVisualSet;

	/** 색 펄스 · 줄기 구슬 머티리얼(파라미터 Color · Glow · Opacity · RimMix) */
	UPROPERTY(Config, EditAnywhere, Category = "Material")
	TSoftObjectPtr<UMaterialInterface> GlowMaterial;

	/** 그리모어 책 몸 · 책장 머티리얼(파라미터 Color · EmissiveColor · Glow). 없으면 엔진 BasicShapeMaterial */
	UPROPERTY(Config, EditAnywhere, Category = "Material")
	TSoftObjectPtr<UMaterialInterface> BookMaterial;

	/** 플레이어가 드는 그리모어 클래스. 비우면 AGrimGrimoireBook */
	UPROPERTY(Config, EditAnywhere, Category = "Grimoire")
	TSoftClassPtr<AGrimGrimoireBook> GrimoireBookClass;

	/** 누르고 실제 발사까지의 시간을 정의에서 읽는 필드 이름(갈래 A). 섬광 · 소리를 그만큼 늦춘다. 없으면 0 */
	UPROPERTY(Config, EditAnywhere, Category = "Cast")
	FName CastWindupProperty = TEXT("CastWindup");

	/** 줄기를 그릴 때 마법 정의에서 읽는 필드 이름(갈래 A 의 UGrimSpellDefinition). 없으면 VisualSet 기본값 */
	UPROPERTY(Config, EditAnywhere, Category = "Stream")
	FName StreamLaunchSpeedProperty = TEXT("StreamLaunchSpeed");

	UPROPERTY(Config, EditAnywhere, Category = "Stream")
	FName StreamGravityScaleProperty = TEXT("StreamGravityScale");

	/** 원뿔 파동 크기를 정의에서 읽는 필드 이름. 없으면 아래 기본값 */
	UPROPERTY(Config, EditAnywhere, Category = "Cone")
	FName ConeRangeProperty = TEXT("ConeRange");

	UPROPERTY(Config, EditAnywhere, Category = "Cone")
	FName ConeHalfAngleProperty = TEXT("ConeHalfAngle");

	UPROPERTY(Config, EditAnywhere, Category = "Cone", meta = (Units = "cm"))
	float DefaultConeRange = 600.f;

	UPROPERTY(Config, EditAnywhere, Category = "Cone", meta = (Units = "deg"))
	float DefaultConeHalfAngle = 30.f;
};
