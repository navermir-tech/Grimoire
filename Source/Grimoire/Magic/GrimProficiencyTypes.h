#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectKey.h"
#include "GrimProficiencyTypes.generated.h"

/**
 * 숙련도 등급표의 한 행 (Q26). 원본은 Content/Grimoire/Data/ProficiencyGrades.csv.
 * 행 이름 = 등급 글자(D · C · B · A · S), 행 순서 = 등급 순서. 등급 수를 바꿔도 코드는 그대로다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimProficiencyGradeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 등급 이름(입문 … 마스터) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
	FText DisplayName;

	/** 이 등급에 닿는 누적 경험(필요 경험 배율 XPScale 을 곱하기 전) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float XPToReach = 0.f;

	/** 마나 비용 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float ManaCostMultiplier = 1.f;

	/** 쿨타임 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float CooldownMultiplier = 1.f;

	/** 마법별 추가 효과(계약 포획 확률 등)의 최대값에 곱하는 배율 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float ExtraEffectScale = 0.f;
};

/** 표에서 읽어 순서대로 늘어놓은 등급 하나(행 이름 포함). 순수 계산이 쓴다. */
struct GRIMOIRE_API FGrimProficiencyGrade
{
	FName Grade;
	FText DisplayName;
	float XPToReach = 0.f;
	float ManaCostMultiplier = 1.f;
	float CooldownMultiplier = 1.f;
	float ExtraEffectScale = 0.f;
};

/** 경험 곡선 — 마법 묶음(서클)마다 다르게. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimProficiencyCurve
{
	GENERATED_BODY()

	/** 필요 경험 배율: 등급 문턱 = XPToReach × XPScale. 높은 서클 마법은 크게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0.01"))
	float XPScale = 1.f;

	/** 서버가 시전을 확정할 때 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float XPPerCast = 1.f;

	/** 의미 있는 명중(적 · 생물 · 반응하는 사물) 하나마다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (ClampMin = "0"))
	float XPPerMeaningfulHit = 4.f;
};

/** 마법별 추가 효과(선택) — 예: 계약 마법의 포획 확률 보너스(R23, M3). 값 = MaxValue × 등급의 ExtraEffectScale. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimProficiencyExtraEffect
{
	GENERATED_BODY()

	/** 효과 이름 태그(예: Taming.CaptureBonus — 태그는 쓰는 시스템이 만든다). 비우면 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
	FGameplayTag EffectTag;

	/** S(ExtraEffectScale 1)일 때의 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
	float MaxValue = 0.f;
};

/**
 * 마법 묶음별 숙련도 프로필(선택). 마법 정의의 ProficiencyProfile 이 비면 서클별 기본 곡선(UGrimProficiencySettings)을 쓴다.
 */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimProficiencyProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 비우면 공용 등급표(설정의 CSV · 에셋) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimProficiencyGradeRow"))
	TSoftObjectPtr<UDataTable> GradeTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
	FGrimProficiencyCurve Curve;
};

/** 등급이 주는 배율 */
struct GRIMOIRE_API FGrimProficiencyMultipliers
{
	float ManaCost = 1.f;
	float Cooldown = 1.f;
	float ExtraEffectScale = 0.f;
};

/** 마법 하나의 숙련도를 UI · 어빌리티가 읽기 좋게 푼 것 */
struct GRIMOIRE_API FGrimProficiencyView
{
	/** 숙련도를 쌓지 않는 항목(기본 공격 등) · 표 없음이면 false */
	bool bTracked = false;
	int32 GradeIndex = INDEX_NONE;
	FName Grade;
	FText GradeName;
	float XP = 0.f;
	/** 지금 등급 · 다음 등급 문턱(XPScale 적용). 최고 등급이면 둘이 같다. */
	float CurrentGradeXP = 0.f;
	float NextGradeXP = 0.f;
	/** 다음 등급까지 0~1. 최고 등급이면 1. */
	float Progress = 0.f;
	bool bMaxGrade = false;
	FGrimProficiencyMultipliers Multipliers;
};

/**
 * 숙련도 순수 계산 (Q26). 액터 · 월드를 모른다 — 테스트가 직접 부른다.
 * 등급은 저장하지 않고 XP 와 표로 매번 계산한다 → 표를 바꾸면 기존 세이브도 새 기준을 따른다.
 */
struct GRIMOIRE_API FGrimProficiencyMath
{
	/** DataTable 행을 순서대로 읽는다(RowMap 은 CSV 행 순서를 지킨다). */
	static void ReadGrades(const UDataTable* Table, TArray<FGrimProficiencyGrade>& OutGrades);

	/** XP 가 닿은 가장 높은 등급. 표가 비면 INDEX_NONE, 첫 문턱 아래여도 0(첫 등급). */
	static int32 GetGradeIndex(float XP, TConstArrayView<FGrimProficiencyGrade> Grades, float XPScale = 1.f);

	/** 등급 Index 의 문턱(XPScale 적용). 범위 밖이면 0. */
	static float GetGradeThreshold(TConstArrayView<FGrimProficiencyGrade> Grades, int32 Index, float XPScale = 1.f);

	/** 등급의 배율. 범위 밖이면 기본(1 · 1 · 0). */
	static FGrimProficiencyMultipliers GetMultipliers(TConstArrayView<FGrimProficiencyGrade> Grades, int32 Index);

	/** XP 하나로 등급 · 문턱 · 막대 · 배율을 모두 푼다. */
	static FGrimProficiencyView MakeView(float XP, TConstArrayView<FGrimProficiencyGrade> Grades, float XPScale = 1.f);

	/**
	 * 한 번에 쌓이는 경험 = Base × RepeatDecay^RepeatCount × GlobalRate.
	 * RepeatCount = 같은 (마법, 대상)에 최근 창 안에서 이미 쌓은 횟수(처음 0).
	 */
	static float ComputeXPGain(float BaseXP, int32 RepeatCount, float RepeatDecay, float GlobalRate);

	/**
	 * 이미 배운 마법의 스킬북을 쓸 때의 경험 = (다음 문턱 − 지금 문턱) × Fraction + Flat.
	 * 최고 등급이거나 숙련도가 없으면 0(쓸 수 없다).
	 */
	static float ComputeSkillbookXP(const FGrimProficiencyView& View, float FractionOfGrade, float FlatXP);

	/** 추가 효과 값 = MaxValue × ExtraEffectScale */
	static float GetExtraEffectValue(float MaxValue, const FGrimProficiencyMultipliers& Multipliers)
	{
		return MaxValue * Multipliers.ExtraEffectScale;
	}
};

/** 같은 대상 반복을 세는 열쇠: (마법, 대상, 종류). 대상이 없으면 null. 시전 · 명중은 따로 센다(Kind 0 = 시전, 1 = 명중). */
struct GRIMOIRE_API FGrimRepeatKey
{
	TObjectKey<UObject> Spell;
	TObjectKey<UObject> Target;
	uint8 Kind = 0;

	friend bool operator==(const FGrimRepeatKey& A, const FGrimRepeatKey& B) { return A.Spell == B.Spell && A.Target == B.Target && A.Kind == B.Kind; }
	friend uint32 GetTypeHash(const FGrimRepeatKey& Key) { return HashCombine(HashCombine(GetTypeHash(Key.Spell), GetTypeHash(Key.Target)), ::GetTypeHash(Key.Kind)); }
};

/**
 * 같은 (마법, 대상) 반복 기록(서버, 순수). 마지막 기록에서 Window 초가 지나면 다시 0 부터 센다.
 * 양초 하나를 켰다 껐다 하는 노가다를 막는다.
 */
struct GRIMOIRE_API FGrimRepeatTracker
{
	/** 이번 기록 전에 이어진 횟수를 돌려주고 기록한다(처음이면 0). */
	int32 Register(const FGrimRepeatKey& Key, double Now, double WindowSeconds);

	/** 창이 지난 기록을 버린다. */
	void Prune(double Now, double WindowSeconds);

	int32 Num() const { return Entries.Num(); }

private:
	struct FEntry
	{
		int32 Count = 0;
		double LastTime = 0.0;
	};
	TMap<FGrimRepeatKey, FEntry> Entries;
};
