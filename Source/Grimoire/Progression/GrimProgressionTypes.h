#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GrimProgressionTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimProgression, Log, All);

// ───────────────────────── 레벨 (R27) ─────────────────────────

/**
 * 레벨 곡선 표 한 줄 — Content/Grimoire/Data/LevelCurve.csv. 행 이름 = 레벨(1 부터 빈틈없이).
 * 값은 그 레벨에 「올라설 때」 받는 것(레벨 1 줄은 0).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLevelRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 레벨이 되는 누적 경험치(레벨 1 = 0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float TotalXP = 0.f;

	/** 이 레벨에 오를 때 더해지는 최대 HP */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float MaxHealth = 0.f;

	/** 이 레벨에 오를 때 더해지는 최대 마나 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float MaxMana = 0.f;

	/** 이 레벨에 오를 때 받는 성장 트리 포인트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	int32 SkillPoints = 0;
};

/** 레벨 하나(표를 읽은 것). Steps[0] = 레벨 1. */
struct GRIMOIRE_API FGrimLevelStep
{
	int32 Level = 1;
	float TotalXP = 0.f;
	float MaxHealth = 0.f;
	float MaxMana = 0.f;
	int32 SkillPoints = 0;
};

struct GRIMOIRE_API FGrimLevelCurve
{
	TArray<FGrimLevelStep> Steps;
};

/** 지금 레벨 안에서 어디쯤인가(UI 막대) */
struct GRIMOIRE_API FGrimLevelProgress
{
	int32 Level = 1;
	int32 MaxLevel = 1;
	float TotalXP = 0.f;
	/** 이 레벨 문턱 · 다음 레벨 문턱(누적) */
	float LevelStartXP = 0.f;
	float NextLevelXP = 0.f;
	/** 0~1. 상한이면 1 */
	float Fraction = 0.f;
	bool bAtCap = false;
};

/** 레벨 순수 계산 — 테스트 Grimoire.Progression.LevelCurve */
struct GRIMOIRE_API FGrimLevelMath
{
	/** 표 → 곡선. 레벨이 1 부터 빈틈없는지, 누적 경험이 늘기만 하는지 본다(문제는 OutProblems, 그래도 읽을 수 있는 만큼 읽는다). */
	static bool ReadCurve(const UDataTable* Table, FGrimLevelCurve& OutCurve, TArray<FString>& OutProblems);

	/** 실제 상한 = min(표의 마지막 레벨, Cap). Cap ≤ 0 이면 표 끝. 표가 비면 1. */
	static int32 GetMaxLevel(const FGrimLevelCurve& Curve, int32 Cap);

	/** 누적 경험 → 레벨(상한까지) */
	static int32 GetLevelForXP(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap);

	/** 레벨이 되는 누적 경험(범위 밖은 끝 값) */
	static float GetXPForLevel(const FGrimLevelCurve& Curve, int32 Level);

	/** 경험은 상한 레벨 문턱에서 멈춘다(상한을 올리면 이어서 쌓인다) */
	static float ClampXP(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap);

	static FGrimLevelProgress GetProgress(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap);

	/** 레벨 2 ~ Level 에서 받은 것의 합 */
	static int32 GetSkillPointsAtLevel(const FGrimLevelCurve& Curve, int32 Level);
	static float GetMaxHealthBonusAtLevel(const FGrimLevelCurve& Curve, int32 Level);
	static float GetMaxManaBonusAtLevel(const FGrimLevelCurve& Curve, int32 Level);

	/** 같은 출처 반복 감쇠: max(Min, Decay^RepeatCount) × Amount × Rate */
	static float ComputeXPGain(float Amount, int32 RepeatCount, float Decay, float MinMultiplier, float GlobalRate);
};

/** 출처별 반복 횟수(서버 전용, 저장 안 함). 마지막 기록에서 Window 초가 지나면 다시 0. */
struct GRIMOIRE_API FGrimXPRepeatTracker
{
	/** 이번이 몇 번째 반복인가(처음 = 0)를 돌려주고 기록한다. */
	int32 Register(const FGameplayTag& Source, double Now, double WindowSeconds);

	void Reset() { Entries.Reset(); }

private:
	struct FEntry
	{
		int32 Count = 0;
		double LastTime = 0.0;
	};
	TMap<FGameplayTag, FEntry> Entries;
};

// ───────────────────────── 성장 트리 (R27) ─────────────────────────

/** 노드 종류(스펙 「노드 종류」) — UI 모양 · 문서용. 효과는 Effects 가 정한다. */
UENUM(BlueprintType)
enum class EGrimSkillNodeKind : uint8
{
	Power,
	Efficiency,
	Status,
	Interaction,
	Survival,
	/** 특성(큰 노드) — 갈래 끝, 마법 모양을 바꾼다 */
	Trait
};

/**
 * 성장 트리 표 한 줄 — Content/Grimoire/Data/SkillTreeNodes.csv. 행 이름 = 노드 id(예: Fire.Power).
 *  - Prerequisites: 선행 노드 id 를 | 로 이어 쓴다(모두 찍어야 열린다). 같은 트리 안에서만.
 *  - Effects: 「키=값」을 ; 로 이어 쓴다. 값이 * 로 시작하면 곱하기(Scale 어트리뷰트), 아니면 더하기.
 *    키가 어트리뷰트 이름(MaxHealth · MaxMana · HeatResist · ColdResist · DiseaseResist · MoveSpeedScale · CastSpeedScale · ManaCostScale)이면 GE 로,
 *    아니면 태그 Progression.Node.<키> 와 합계값(UGrimSkillTreeComponent::GetEffectValue)으로만 남는다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSkillTreeNodeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 트리 id: Normal · Elemental · Light · Dark (UGrimSkillTreeSettings::Trees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FName Tree;

	/** 세부 갈래 id(UGrimSkillTreeSettings::Branches) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FName Branch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	EGrimSkillNodeKind Kind = EGrimSkillNodeKind::Power;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FText Description;

	/** UI 격자 칸(0.5 단위 가능). X = 왼쪽부터, Y = 위(안쪽)부터 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	float GridX = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	float GridY = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FString Prerequisites;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree", meta = (ClampMin = "0"))
	int32 Cost = 1;

	/** 이 서클 이상이어야 찍는다(큰 노드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree", meta = (ClampMin = "1"))
	int32 RequiredCircle = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillTree")
	FString Effects;
};

/** 노드 효과 하나 */
struct GRIMOIRE_API FGrimSkillEffect
{
	FName Key;
	bool bMultiply = false;
	float Value = 0.f;
};

/** 키 하나의 합: 더하기는 합, 곱하기는 곱 */
struct GRIMOIRE_API FGrimSkillEffectTotal
{
	float Add = 0.f;
	float Mul = 1.f;
};

struct GRIMOIRE_API FGrimSkillNode
{
	FName Id;
	FName Tree;
	FName Branch;
	EGrimSkillNodeKind Kind = EGrimSkillNodeKind::Power;
	FText DisplayName;
	FText Description;
	FVector2D Grid = FVector2D::ZeroVector;
	TArray<FName> Prerequisites;
	int32 Cost = 1;
	int32 RequiredCircle = 1;
	TArray<FGrimSkillEffect> Effects;
};

/** 표를 읽은 트리 전체(4 트리의 모든 노드) */
struct GRIMOIRE_API FGrimSkillTreeDefinition
{
	TArray<FGrimSkillNode> Nodes;
	TMap<FName, int32> IndexById;

	const FGrimSkillNode* Find(FName Id) const
	{
		const int32* Index = IndexById.Find(Id);
		return Index ? &Nodes[*Index] : nullptr;
	}

	/** 표에 나온 순서대로 트리 id */
	TArray<FName> GetTreeIds() const;
};

/** 찍기 판정 */
UENUM(BlueprintType)
enum class EGrimSkillUnlockResult : uint8
{
	Unlocked,
	AlreadyUnlocked,
	UnknownNode,
	MissingPrerequisite,
	NotEnoughPoints,
	CircleTooLow,
	/** 서버가 아님 · 컴포넌트 없음 */
	Invalid
};

/** 성장 트리 순수 계산 — 테스트 Grimoire.Progression.* */
struct GRIMOIRE_API FGrimSkillTreeMath
{
	/** "MaxMana=+10;Fire.Power=+5;ManaCostScale=*0.97" → 효과들. 틀린 조각은 OutProblem 에 적고 false. */
	static bool ParseEffects(const FString& Text, TArray<FGrimSkillEffect>& OutEffects, FString* OutProblem = nullptr);

	/** "A|B" → [A, B] */
	static TArray<FName> ParsePrerequisites(const FString& Text);

	/** 표 → 정의. 선행 노드가 없거나 다른 트리이거나 순환이면 문제로 적는다. */
	static bool BuildDefinition(const UDataTable* Table, FGrimSkillTreeDefinition& OutDefinition, TArray<FString>& OutProblems);

	/** 찍은 노드들의 비용 합(Tree 를 주면 그 트리만) */
	static int32 GetSpentPoints(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName Tree = NAME_None);

	static EGrimSkillUnlockResult CanUnlock(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName NodeId, int32 AvailablePoints, int32 Circle);

	/** 찍은 노드들의 효과 합 */
	static void SumEffects(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, TMap<FName, FGrimSkillEffectTotal>& OutTotals);

	/** 트리(빈 이름 = 전체)의 노드 수 · 비용 합 */
	static int32 GetNodeCount(const FGrimSkillTreeDefinition& Definition, FName Tree = NAME_None);
	static int32 GetTotalCost(const FGrimSkillTreeDefinition& Definition, FName Tree = NAME_None);

	/**
	 * 포인트 Points 로 찍을 수 있는 노드 수의 상한 추정 — 열린 노드 중 가장 싼 것부터 찍는다(서클 조건은 무시).
	 * 「상한까지 올려도 30~40% 만」을 노드 수로 볼 때 쓴다. OutSpent 에 쓴 포인트.
	 */
	static int32 GetMaxNodesForPoints(const FGrimSkillTreeDefinition& Definition, int32 Points, int32* OutSpent = nullptr);

	/** 트리 하나를 비울 때 남는 노드(다른 트리의 노드) */
	static TArray<FName> RemoveTree(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName Tree);
};
