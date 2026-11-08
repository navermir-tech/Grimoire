#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "GrimAppearanceSettings.generated.h"

class UAnimationAsset;
class USkeletalMesh;

/**
 * 캐릭터 생성 화면 · 마법탑 거울의 카메라(Q102) — 캐릭터 앞 정면. 얼굴 칸을 고르면 얼굴 가까이, 옷 · 몸 칸이면 전신. 마우스 끌기로 돌려 본다.
 * 거리 · 높이는 cm, 화면 왼쪽 판에 가리지 않게 캐릭터를 화면 오른쪽으로 비키는 몫이 LateralOffset(카메라 기준 왼쪽으로 겨눔).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatorCameraTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "10", ClampMax = "120"))
	float FieldOfView = 50.f;

	/** 얼굴: 머리 뼈(HeadBone)에서 이만큼 떨어져 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face", meta = (ClampMin = "10", Units = "cm"))
	float FaceDistance = 120.f;

	/** 머리 뼈 높이에 더한다 — 마네킹의 head 뼈는 두개골 아래(목 위)라 눈 높이로 올린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face", meta = (Units = "cm"))
	float FaceHeightOffset = 14.f;

	/** 머리 뼈가 없으면 액터 중심에서 이 높이를 얼굴로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face", meta = (Units = "cm"))
	float FaceFallbackHeight = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face", meta = (Units = "cm"))
	float FaceLateralOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Face")
	FName HeadBone = TEXT("head");

	/** 전신: 액터 중심(허리쯤)에서 이만큼 떨어져 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "10", Units = "cm"))
	float BodyDistance = 430.f;

	/** 전신을 겨누는 높이(액터 중심 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (Units = "cm"))
	float BodyHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (Units = "cm"))
	float BodyLateralOffset = 70.f;

	/** 처음 · 기본으로 내려다보는 각(도, 음수 = 위에서) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orbit", meta = (ClampMin = "-60", ClampMax = "60"))
	float DefaultPitch = -5.f;

	/** 마우스 끌기: 화면 1픽셀에 도는 각(도) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orbit", meta = (ClampMin = "0"))
	float DragYawPerPixel = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orbit", meta = (ClampMin = "0"))
	float DragPitchPerPixel = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orbit", meta = (ClampMin = "-89", ClampMax = "0"))
	float MinPitch = -35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orbit", meta = (ClampMin = "0", ClampMax = "89"))
	float MaxPitch = 25.f;

	/** 얼굴 ↔ 전신으로 옮겨 가는 빠르기(VInterpTo 속도 — 클수록 빠름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend", meta = (ClampMin = "0"))
	float InterpSpeed = 8.f;

	/** 열고 닫을 때 평소 카메라와 섞는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend", meta = (ClampMin = "0", Units = "s"))
	float ViewBlendSeconds = 0.35f;
};

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

	/** 생성 화면 · 거울 카메라(Q102) */
	UPROPERTY(Config, EditAnywhere, Category = "Creator")
	FGrimCreatorCameraTuning CreatorCamera;
};
