#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Progression/GrimProgressionTypes.h"
#include "GrimProgressionSettings.generated.h"

class UDataTable;

/** 경험치 출처 하나의 규칙 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimXPSourceRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (Categories = "Progression.XPSource"))
	FGameplayTag Source;

	/** 게임이 스스로 주는 출처(스킬북 · 승급 · 지역 · 규칙 발견)가 쓰는 기본 경험 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "0"))
	float DefaultXP = 0.f;

	/** 같은 출처 반복: 두 번째부터 이만큼씩 곱한다(1 = 감쇠 없음 — 퀘스트 · 첫 발견) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "0", ClampMax = "1"))
	float RepeatDecay = 1.f;

	/** 마지막 기록에서 이 초가 지나면 반복을 0 부터 다시 센다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "0", Units = "s"))
	float RepeatWindowSeconds = 60.f;

	/** 감쇠가 아무리 쌓여도 이 배율 밑으로는 안 내려간다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP", meta = (ClampMin = "0", ClampMax = "1"))
	float MinMultiplier = 0.1f;
};

/**
 * 레벨 · 경험치 (R27) — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimLevelSettings].
 * 곡선(레벨마다 누적 경험 · 최대 HP · 최대 마나 · 포인트)은 레벨 표(CSV). 레벨은 서클과 무관하다(Q30).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Level"))
class GRIMOIRE_API UGrimLevelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimLevelSettings* Get() { return GetDefault<UGrimLevelSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 공용 레벨 곡선(에셋 → CSV). 한 번 읽어 둔다. */
	static const FGrimLevelCurve& GetCurve();

	/** 다시 읽게 한다(콘솔 Grim.Level.Reload) */
	static void ResetCurve();

	/** CSV 문자열 → 레벨 표(테스트 · 로더 공용) */
	static UDataTable* CreateLevelTableFromCSV(const FString& CSV, TArray<FString>& OutProblems);

	/** 출처 규칙. 없으면 기본 규칙(Default*). */
	FGrimXPSourceRule GetSourceRule(const FGameplayTag& Source) const;

	/** 레벨 상한(오픈 베타 30 · 정식 50 — 스펙 제안). 표보다 크면 표 끝. */
	UPROPERTY(Config, EditAnywhere, Category = "Level", meta = (ClampMin = "1"))
	int32 LevelCap = 30;

	/** 모든 경험에 곱한다(서버 옵션) */
	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0"))
	float GlobalXPRate = 1.f;

	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (TitleProperty = "Source"))
	TArray<FGrimXPSourceRule> Sources;

	/** 규칙이 없는 출처의 반복 감쇠 */
	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0", ClampMax = "1"))
	float DefaultRepeatDecay = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0", Units = "s"))
	float DefaultRepeatWindowSeconds = 60.f;

	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0", ClampMax = "1"))
	float DefaultMinMultiplier = 0.1f;

	/** 서버가 플레이어의 기후 지역을 보는 간격(새 지역 첫 방문) */
	UPROPERTY(Config, EditAnywhere, Category = "Discovery", meta = (ClampMin = "0.1", Units = "s"))
	float RegionCheckInterval = 2.f;

	/** 처음 서 있던 지역도 경험을 주나(끄면 조용히 「발견」만) */
	UPROPERTY(Config, EditAnywhere, Category = "Discovery")
	bool bAwardStartingRegion = false;

	/** 레벨 곡선 원본 CSV. 프로젝트 Content 폴더 기준 */
	UPROPERTY(Config, EditAnywhere, Category = "Table")
	FString LevelTableCSV = TEXT("Grimoire/Data/LevelCurve.csv");

	/** 지정하면 CSV 대신 이 DataTable 을 쓴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Table", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimLevelRow"))
	TSoftObjectPtr<UDataTable> LevelTableAsset;
};

/** 트리 하나의 표시 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSkillTreeDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FLinearColor Color = FLinearColor(0.7f, 0.65f, 0.55f);
};

/** 세부 갈래 하나의 표시 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSkillBranchDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FLinearColor Color = FLinearColor(0.7f, 0.65f, 0.55f);
};

/**
 * 속성 성장 트리 (R27) — [/Script/Grimoire.GrimSkillTreeSettings]. 노드 · 비용 · 선행 · 효과는 트리 표(CSV), 여기엔 표시와 표 경로.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Skill Tree"))
class GRIMOIRE_API UGrimSkillTreeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimSkillTreeSettings* Get() { return GetDefault<UGrimSkillTreeSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 공용 트리 정의(에셋 → CSV). 한 번 읽어 둔다. */
	static const FGrimSkillTreeDefinition& GetDefinition();

	/** 다시 읽게 한다(콘솔 Grim.Tree.Reload) */
	static void ResetDefinition();

	static UDataTable* CreateNodeTableFromCSV(const FString& CSV, TArray<FString>& OutProblems);

	const FGrimSkillTreeDisplay* FindTree(FName Id) const;
	const FGrimSkillBranchDisplay* FindBranch(FName Id) const;

	/** 표시 순서(설정 순서, 설정에 없는 트리는 표 순서로 뒤에) */
	TArray<FName> GetTreeOrder(const FGrimSkillTreeDefinition& Definition) const;

	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TArray<FGrimSkillTreeDisplay> Trees;

	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TArray<FGrimSkillBranchDisplay> Branches;

	/** 그리모어 「성장 트리」 격자 한 칸(픽셀) */
	UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "20"))
	float GridCellWidth = 62.f;

	UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "20"))
	float GridCellHeight = 78.f;

	/** 노드 상자 크기(픽셀) */
	UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "20"))
	FVector2D NodeSize = FVector2D(58.f, 46.f);

	/** 트리 표 원본 CSV. 프로젝트 Content 폴더 기준 */
	UPROPERTY(Config, EditAnywhere, Category = "Table")
	FString NodeTableCSV = TEXT("Grimoire/Data/SkillTreeNodes.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Table", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimSkillTreeNodeRow"))
	TSoftObjectPtr<UDataTable> NodeTableAsset;
};
