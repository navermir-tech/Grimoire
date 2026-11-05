#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "GrimAppearanceSettings.generated.h"

class UAnimationAsset;
class USkeletalMesh;

/**
 * 외형(커스터마이징) 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimAppearanceSettings].
 * 카탈로그 표는 Content/Grimoire/Data/Appearance 의 CSV(런타임에 읽는다). CatalogAsset 을 넣으면 그 DataAsset 을 쓴다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Appearance"))
class GRIMOIRE_API UGrimAppearanceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGrimAppearanceSettings();

	static const UGrimAppearanceSettings* Get() { return GetDefault<UGrimAppearanceSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 카탈로그를 한 번 읽어 둔다. 문제는 로그(Error)에 남기고 읽은 만큼 쓴다. */
	static const FGrimAppearanceCatalog& GetCatalog();

	/** 다시 읽게 한다(콘솔 Grim.Appearance.Reload) */
	static void ReloadCatalog();

	// ───── 표 (Content 폴더 기준) ─────

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString PartsCSV = TEXT("Grimoire/Data/Appearance/AppearanceParts.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString PiecesCSV = TEXT("Grimoire/Data/Appearance/AppearancePieces.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString PalettesCSV = TEXT("Grimoire/Data/Appearance/AppearancePalettes.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString MorphsCSV = TEXT("Grimoire/Data/Appearance/AppearanceMorphs.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString RulesCSV = TEXT("Grimoire/Data/Appearance/NpcAppearanceRules.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString NpcsCSV = TEXT("Grimoire/Data/Appearance/NpcAppearance.csv");

	/** 있으면 CSV 대신 이 DataAsset 의 표 */
	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	TSoftObjectPtr<UGrimAppearanceCatalogAsset> CatalogAsset;

	// ───── 몸 ─────

	/** 체형 A · B 의 몸 메시(같은 마네킹 스켈레톤). 지금은 UE 마네킹 Manny · Quinn */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	TArray<TSoftObjectPtr<USkeletalMesh>> BodyMeshes;

	/** 몸 메시 재질 칸마다 칠할 색(칸 수보다 짧으면 Skin) — 마네킹: 0 머리 · 다리, 1 몸통 */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	TArray<EGrimAppearanceChannel> BodyMaterialChannels;

	/** 몸 재질의 색 파라미터(마네킹 M_Mannequin = "Paint Tint"). 색 칸 이름 파라미터(Skin 등)도 함께 준다 */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	FName BodyTintParameter = TEXT("Paint Tint");

	/** 키 배율 범위(Height 0 → X, 255 → Y) */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	FVector2D HeightRange = FVector2D(0.94, 1.06);

	// ───── 조각 ─────

	/** 자리 표시 도형 재질(엔진 BasicShapeMaterial)의 색 파라미터 */
	UPROPERTY(Config, EditAnywhere, Category = "Parts")
	FName PlaceholderColorParameter = TEXT("Color");

	/** 서클 고리 수를 재질에도 준다(수놓은 고리로 바꿀 때) */
	UPROPERTY(Config, EditAnywhere, Category = "Parts")
	FName RingCountParameter = TEXT("RingCount");

	/** Small 소품(고리 · 단추)을 이 거리 밖에서 숨긴다. 0 = 숨기지 않음 */
	UPROPERTY(Config, EditAnywhere, Category = "Parts", meta = (ClampMin = "0", Units = "cm"))
	float SmallPieceCullDistance = 2500.f;

	// ───── NPC · 플레이어 ─────

	/** 마법탑 NPC · 줄 세우기 더미가 서 있는 애님 */
	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	TSoftObjectPtr<UAnimationAsset> NpcIdleAnimation;

	/** NpcAppearance 표에 없는 마법탑 NPC 의 규칙 */
	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FName DefaultTowerNpcRule = TEXT("Tower_Apprentice");

	/** 플레이어 시작 · 무작위 규칙 */
	UPROPERTY(Config, EditAnywhere, Category = "Player")
	FName PlayerRule = TEXT("Player_Start");

	/** 세이브에 외형이 없는 새 캐릭터면 들어오자마자 생성 화면을 연다(개발 중엔 꺼 둠 — 치트 GrimCreator) */
	UPROPERTY(Config, EditAnywhere, Category = "Player")
	bool bOpenCreatorForNewCharacter = false;
};
