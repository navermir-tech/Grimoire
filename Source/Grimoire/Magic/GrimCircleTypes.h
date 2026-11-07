#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GrimCircleTypes.generated.h"

/**
 * 서클 표 한 줄 (Q122 — 서클이 주는 것: ① 수치 보정 ② 해금, 그리고 그 서클로 오르는 승급 자격).
 * 원본은 Content/Grimoire/Data/Circles.csv. 행 이름 = 서클(1 부터 빈틈없이). 수치는 모두 여기 — 코드에 적지 않는다.
 * 생물(데리고 다님 · 보관함 칸)은 생물 설정(UGrimCreatureStorageSettings)이, 성장 트리 큰 노드 조건은 SkillTreeNodes.csv 가 정본이라 여기 두지 않는다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCircleRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 격 차이 위력: 내 서클이 마법의 서클보다 한 단계 높을 때마다 더하는 위력(0.03 = +3%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Bonus", meta = (ClampMin = "0"))
	float GapPowerPerCircle = 0.f;

	/** 격 차이 위력 상한(0.15 = +15%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Bonus", meta = (ClampMin = "0"))
	float GapPowerCap = 0.f;

	/** 격 차이 마나 효율: 한 단계마다 줄이는 마나 소모(0.03 = −3%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Bonus", meta = (ClampMin = "0", ClampMax = "1"))
	float GapManaPerCircle = 0.f;

	/** 격 차이 마나 효율 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Bonus", meta = (ClampMin = "0", ClampMax = "1"))
	float GapManaCap = 0.f;

	/** 각인대 인챈트 후보의 최고 단계(1 = Ⅰ) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Bonus", meta = (ClampMin = "1"))
	int32 MaxEnchantTier = 1;

	/** 열린 단축 슬롯 세트 수(마법서 설정 SpellSlotSets 를 넘지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Unlock", meta = (ClampMin = "1"))
	int32 SlotSets = 1;

	/** 동시에 유지하는 마법(장벽 · 장판 · 몸 둘레) 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Unlock", meta = (ClampMin = "1"))
	int32 MaxSustainedSpells = 1;

	/** 입을 수 있는 장비 재료 등급(Equipment.csv 의 Tier 이하) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Unlock", meta = (ClampMin = "1"))
	int32 EquipTier = 1;

	/** 이 서클에 새로 열리는 것의 ID(| 로 여럿, 예: Access.Dungeon). 한 번 열리면 위 서클에서도 열려 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Unlock")
	FString Unlocks;

	/** 새로 열리는 곳 · 것(화면 글) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Unlock")
	FText UnlockText;

	/** 이 서클로 오르는 숙련도 자격: 「등급:수」를 | 로(B:3|A:1 = B 이상 3개, 그중 A 이상 1개). 1서클은 비운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Promotion")
	FString PromotionGrades;

	/** 위 자격을 채운 마법 가운데 지금 서클(오르기 전)의 마법이 이만큼 있어야 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Promotion", meta = (ClampMin = "0"))
	int32 PromotionCurrentCircleSpells = 0;

	/** 시험 과제(화면 글) — 과제 자체는 Quests.csv 의 시험(TrialCircle) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circle|Promotion")
	FText PromotionTask;
};

/** 숙련도 자격 하나: 이 등급 이상인 마법이 Count 개 */
struct GRIMOIRE_API FGrimCircleGradeRequirement
{
	FName Grade;
	int32 Count = 0;
};

/** 표를 읽은 서클 하나 */
struct GRIMOIRE_API FGrimCircleStep
{
	int32 Circle = 1;
	float GapPowerPerCircle = 0.f;
	float GapPowerCap = 0.f;
	float GapManaPerCircle = 0.f;
	float GapManaCap = 0.f;
	int32 MaxEnchantTier = 1;
	int32 SlotSets = 1;
	int32 MaxSustainedSpells = 1;
	int32 EquipTier = 1;
	TArray<FName> Unlocks;
	FText UnlockText;
	TArray<FGrimCircleGradeRequirement> PromotionGrades;
	int32 PromotionCurrentCircleSpells = 0;
	FText PromotionTask;
};

/** 서클 표 전체. Steps[0] = 1서클 */
struct GRIMOIRE_API FGrimCircleTable
{
	TArray<FGrimCircleStep> Steps;

	bool IsEmpty() const { return Steps.IsEmpty(); }
	int32 GetMaxCircle() const { return Steps.Num(); }

	/** 그 서클의 줄. 표보다 높으면 마지막, 낮으면 첫 줄. 표가 비면 nullptr */
	const FGrimCircleStep* FindClamped(int32 Circle) const;

	/** 정확히 그 서클의 줄(없으면 nullptr) */
	const FGrimCircleStep* Find(int32 Circle) const { return Steps.IsValidIndex(Circle - 1) ? &Steps[Circle - 1] : nullptr; }
};

/** 마법 하나의 숙련도 요약 — 승급 판정이 본다 */
struct GRIMOIRE_API FGrimSpellGradeEntry
{
	int32 SpellCircle = 1;
	/** 등급표 순서 번호(D = 0 …) */
	int32 GradeIndex = 0;
};

/** 숙련도 자격 하나의 판정 */
struct GRIMOIRE_API FGrimCircleGradeCheck
{
	FName Grade;
	int32 GradeIndex = 0;
	int32 Required = 0;
	int32 Have = 0;
	bool IsMet() const { return Have >= Required; }
};

/** 다음 서클 승급 자격 판정 결과 (Q122 — 숙련도 + 시험) */
struct GRIMOIRE_API FGrimCircleCheck
{
	int32 CurrentCircle = 1;
	/** 표에 다음 서클이 없다 */
	bool bAtMax = true;
	int32 NextCircle = 0;
	TArray<FGrimCircleGradeCheck> Grades;
	/** 자격을 채운(가장 낮은 등급 조건) 마법 가운데 지금 서클 마법 수 / 필요 수 */
	int32 CurrentCircleSpells = 0;
	int32 RequiredCurrentCircleSpells = 0;
	FText Task;
	bool bMet = false;

	/** 처음으로 모자란 등급 조건(없으면 nullptr) */
	const FGrimCircleGradeCheck* FirstUnmetGrade() const;
	bool IsCurrentCircleMet() const { return CurrentCircleSpells >= RequiredCurrentCircleSpells; }
};

/** 서클 표 순수 계산 — 테스트 Grimoire.Magic.Circle.* */
struct GRIMOIRE_API FGrimCircleMath
{
	/** 표 → 서클 줄들. 서클이 1 부터 빈틈없는지, 등급 이름이 등급표에 있는지 본다(GradeOrder 가 비면 등급 이름은 보지 않는다). */
	static bool ReadTable(const UDataTable* Table, FGrimCircleTable& OutTable, TConstArrayView<FName> GradeOrder, TArray<FString>& OutProblems);

	/** 「B:3|A:1」 → 자격 목록. 잘못된 조각은 OutProblems 에 */
	static void ParseGrades(const FString& Text, TArray<FGrimCircleGradeRequirement>& OutGrades, TArray<FString>& OutProblems);

	/** 격 차이(내 서클 − 마법 서클, 음수면 0)에 따른 위력 보너스 0 ~ 상한(내 서클의 줄을 쓴다) */
	static float GetGapPowerBonus(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle);
	static float GetGapManaBonus(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle);

	/** 피해에 곱하는 값(1 + 보너스) · 마나 소모에 곱하는 값(1 − 보너스) */
	static float GetPowerMultiplier(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle) { return 1.f + GetGapPowerBonus(Table, CasterCircle, SpellCircle); }
	static float GetManaMultiplier(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle) { return FMath::Max(0.f, 1.f - GetGapManaBonus(Table, CasterCircle, SpellCircle)); }

	/** 해금 조회(표가 비면 Fallback) */
	static int32 GetSlotSets(const FGrimCircleTable& Table, int32 Circle, int32 Fallback);
	static int32 GetMaxSustainedSpells(const FGrimCircleTable& Table, int32 Circle, int32 Fallback);
	static int32 GetEquipTier(const FGrimCircleTable& Table, int32 Circle, int32 Fallback);
	static int32 GetMaxEnchantTier(const FGrimCircleTable& Table, int32 Circle, int32 Fallback);

	/** 그 장비 등급을 처음 입을 수 있는 서클(표에 없으면 INDEX_NONE) */
	static int32 GetCircleForEquipTier(const FGrimCircleTable& Table, int32 Tier);

	/** 이 서클로 그 등급 장비를 입을 수 있나. 못 입으면 OutRequiredCircle = 입게 되는 서클(표 끝까지 없으면 표의 마지막 서클 + 1). 표가 비면 늘 입는다 */
	static bool CanWearTier(const FGrimCircleTable& Table, int32 Circle, int32 Tier, int32& OutRequiredCircle);

	/** 그 ID 가 처음 열리는 서클(없으면 INDEX_NONE) · 이 서클에서 열려 있나(누적) */
	static int32 GetUnlockCircle(const FGrimCircleTable& Table, FName UnlockId);
	static bool IsUnlocked(const FGrimCircleTable& Table, int32 Circle, FName UnlockId);

	/**
	 * 다음 서클 승급 자격. GradeOrder = 등급표 순서(D, C, B, A, S). 등급 조건은 「그 등급 이상인 마법 수」로 각각 센다(누적 — A 는 B 이상에도 든다).
	 * 「지금 서클 마법」은 가장 낮은 등급 조건을 채운 마법 가운데 서클이 지금 서클 이상인 것.
	 */
	static FGrimCircleCheck EvaluatePromotion(const FGrimCircleTable& Table, int32 CurrentCircle, TConstArrayView<FGrimSpellGradeEntry> Spells, TConstArrayView<FName> GradeOrder);

	/** 실패 알림 Param 에 등급 조건 하나를 싣는다(등급 번호 · 필요 · 지금 — 각 0~99) */
	static int32 PackGradeParam(int32 GradeIndex, int32 Required, int32 Have);
	static void UnpackGradeParam(int32 Param, int32& OutGradeIndex, int32& OutRequired, int32& OutHave);
};
