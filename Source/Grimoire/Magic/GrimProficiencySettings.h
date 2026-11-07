#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Magic/GrimProficiencyTypes.h"
#include "Magic/GrimCircleTypes.h"
#include "GrimProficiencySettings.generated.h"

class UDataTable;
class UGrimSpellDefinition;

/**
 * 숙련도 수치 (Q26) — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimProficiencySettings].
 * 등급 수 · 배율은 등급표(CSV), 경험 곡선은 여기(서클별) 또는 마법 정의의 ProficiencyProfile.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Proficiency"))
class GRIMOIRE_API UGrimProficiencySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimProficiencySettings* Get() { return GetDefault<UGrimProficiencySettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 공용 등급표(에셋이 지정돼 있으면 그것, 아니면 CSV). 한 번 읽어 둔다. 읽지 못하면 nullptr. */
	static const UDataTable* GetDefaultGradeTable();

	/** 공용 등급표를 다시 읽게 한다(콘솔 Grim.Magic.ReloadProficiency). */
	static void ResetDefaultGradeTable();

	/** CSV 문자열을 등급표로 읽는다(테스트 · 로더 공용). 문제는 OutProblems 에. */
	static UDataTable* CreateGradeTableFromCSV(const FString& CSV, TArray<FString>& OutProblems);

	/** 마법이 쓰는 등급들(프로필의 표 → 공용 표). 공용 표는 캐시한다. */
	static void GetGradesFor(const UGrimSpellDefinition* Spell, TArray<FGrimProficiencyGrade>& OutGrades);

	/** 마법이 쓰는 경험 곡선(프로필 → 서클별 기본 → 마지막 값). */
	static FGrimProficiencyCurve GetCurveFor(const UGrimSpellDefinition* Spell);

	/** 모든 경험에 곱한다(서버 옵션으로도 바꿀 수 있게 — 콘솔 Grim.Magic.XPRate). */
	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0"))
	float GlobalXPRate = 1.f;

	/** 같은 (마법, 대상) 반복 감쇠: 두 번째부터 이만큼씩 곱한다(0.5 → 1, 0.5, 0.25 …). */
	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0", ClampMax = "1"))
	float RepeatTargetDecay = 0.5f;

	/** 마지막 기록에서 이 초가 지나면 반복을 다시 0 부터 센다. */
	UPROPERTY(Config, EditAnywhere, Category = "XP", meta = (ClampMin = "0", Units = "s"))
	float RepeatWindowSeconds = 30.f;

	/** [0] = 1서클 마법의 기본 곡선. 서클이 표보다 높으면 마지막 값. 마법 정의의 ProficiencyProfile 이 있으면 그쪽. */
	UPROPERTY(Config, EditAnywhere, Category = "XP")
	TArray<FGrimProficiencyCurve> DefaultCurveByCircle;

	/** 등급표 원본 CSV. 프로젝트 Content 폴더 기준 */
	UPROPERTY(Config, EditAnywhere, Category = "Grades")
	FString GradeTableCSV = TEXT("Grimoire/Data/ProficiencyGrades.csv");

	/** 지정하면 CSV 대신 이 DataTable(DT_ProficiencyGrades)을 쓴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Grades", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimProficiencyGradeRow"))
	TSoftObjectPtr<UDataTable> GradeTableAsset;
};

/**
 * 서클 (R06 · Q122) — [/Script/Grimoire.GrimProgressionSettings]. 서클 표(Circles.csv)가 서클마다 주는 것(수치 보정 · 해금)과
 * 그 서클로 오르는 숙련도 자격을 갖는다. 승급은 서클만 올린다(최대 마나는 레벨 몫 — Q30). 표에 있는 서클까지만 오른다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Progression"))
class GRIMOIRE_API UGrimProgressionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimProgressionSettings* Get() { return GetDefault<UGrimProgressionSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 서클 표(에셋이 지정돼 있으면 그것, 아니면 CSV). 한 번 읽어 둔다. 읽지 못하면 빈 표 */
	static const FGrimCircleTable& GetCircleTable();

	/** 다시 읽게 한다(콘솔 Grim.Magic.ReloadCircles) */
	static void ResetCircleTable();

	/** CSV 문자열 → 표(테스트 · 로더 공용). 문제는 OutProblems 에 */
	static UDataTable* CreateCircleTableFromCSV(const FString& CSV, TArray<FString>& OutProblems);

	/** 공용 숙련도 등급표의 등급 순서(D, C, B, A, S) */
	static void GetGradeOrder(TArray<FName>& OutOrder);

	/** 서클 표 원본 CSV. 프로젝트 Content 폴더 기준 */
	UPROPERTY(Config, EditAnywhere, Category = "Circle")
	FString CircleTableCSV = TEXT("Grimoire/Data/Circles.csv");

	/** 지정하면 CSV 대신 이 DataTable(DT_Circles)을 쓴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Circle", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimCircleRow"))
	TSoftObjectPtr<UDataTable> CircleTableAsset;
};

/**
 * 마법서 설정 — [/Script/Grimoire.GrimSpellbookSettings]. 단축 슬롯 세트(호그와트 레거시처럼 4칸 세트 여러 개) · 스킬북 사용.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Spellbook"))
class GRIMOIRE_API UGrimSpellbookSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimSpellbookSettings* Get() { return GetDefault<UGrimSpellbookSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	int32 GetTotalSlots() const { return FMath::Max(SpellSlotsPerSet, 1) * FMath::Max(SpellSlotSets, 1); }

	/** 세트 하나의 슬롯 수(호그와트 레거시 = 4) */
	UPROPERTY(Config, EditAnywhere, Category = "Slots", meta = (ClampMin = "1", ClampMax = "8"))
	int32 SpellSlotsPerSet = 4;

	/** 세트 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Slots", meta = (ClampMin = "1", ClampMax = "8"))
	int32 SpellSlotSets = 4;

	/** 새로 배운 마법(숙련도를 쌓는 것)을 첫 빈 슬롯에 넣는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Slots")
	bool bAutoAssignSlots = true;

	/** 이미 배운 마법의 스킬북을 쓰면: 지금 등급 → 다음 등급 구간의 이 비율만큼 경험 */
	UPROPERTY(Config, EditAnywhere, Category = "Skillbook", meta = (ClampMin = "0", ClampMax = "1"))
	float SkillbookXPFractionOfGrade = 0.4f;

	/** 위에 더하는 고정 경험 */
	UPROPERTY(Config, EditAnywhere, Category = "Skillbook", meta = (ClampMin = "0"))
	float SkillbookFlatXP = 0.f;

	/** 시전자 마나 비용 배율 어트리뷰트 이름(생존 — 더위). 없으면 1 */
	UPROPERTY(Config, EditAnywhere, Category = "Caster")
	FName ManaCostScaleAttribute = TEXT("ManaCostScale");

	/** 시전 속도 배율 어트리뷰트 이름(생존 — 추위 · 피곤함, 쿨다운을 나눔). 없으면 1 */
	UPROPERTY(Config, EditAnywhere, Category = "Caster")
	FName CastSpeedScaleAttribute = TEXT("CastSpeedScale");

	/** 붙어 있으면 시전할 수 없는 태그 이름(아직 등록되지 않은 태그는 건너뛴다) */
	UPROPERTY(Config, EditAnywhere, Category = "Caster")
	TArray<FName> CastBlockingTags;
};
