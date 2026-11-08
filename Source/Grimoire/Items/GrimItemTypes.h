#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GrimItemTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimItems, Log, All);

/** 아이템 종류(docs/systems/building 「인벤토리 · 아이템」). 뒤에만 더한다(CSV 는 이름으로 읽는다). */
UENUM(BlueprintType)
enum class EGrimItemKind : uint8
{
	/** 재료(털 · 뿔 · 허브 · 소금 …) — 쓸 수 없고 요리 · 생산(M4)에 들어간다 */
	Material,
	/** 음식 — 쓰면 먹는다(배고픔 회복 · 날것이면 배탈 확률 · 버프) */
	Food,
	/** 스킬북 — 쓰면 읽는다(UGrimSpellbookComponent::UseSkillbook) */
	Skillbook,
	/** 도구 — M4(내구도 · 인챈트) */
	Tool,
	/** 장비(M4) — 쓰면 입는다(UGrimEquipmentComponent). 칸 · 내구도는 Equipment.csv */
	Equipment,
	/** 씨앗(M4) — 밭에 F 로 심는다(Crops.csv 의 SeedItem) */
	Seed,
};

/** 땅에 떨어진 모양(엔진 기본 도형 — 아트 전 임시) */
UENUM(BlueprintType)
enum class EGrimPickupShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
};

/**
 * 아이템 표(Content/Grimoire/Data/Items.csv) 한 행. 행 이름 = 아이템 ID.
 * 묶음(Groups, `|` 로 여럿)은 요리 재료(`#Meat`) · 퀘스트 이벤트(Item.Obtained.<묶음>)에 쓴다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EGrimItemKind Kind = EGrimItemKind::Material;

	/** 한 칸에 쌓이는 최대 개수(1 = 쌓이지 않음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 20;

	/** 무게(kg) — 무게 제한은 M4(자리만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float Weight = 0.f;

	/** 묶음 이름, `|` 로 여럿. 예: "Meat|Raw" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Groups;

	// ── 음식 ──

	/** 먹으면 배고픔을 이만큼 채운다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float HungerRestore = 0.f;

	/** 날것 — 배탈 확률이 있다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	bool bRaw = false;

	/** 날것일 때 배탈 확률(0~1). 음수면 생존 설정의 기본(UGrimSurvivalSettings Disease.RawFoodStomachChance) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float DiseaseChance = -1.f;

	/** 버프 지속(초). 0 이면 버프 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float BuffSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float BuffColdResist = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float BuffHeatResist = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float BuffDiseaseResist = 0.f;

	// ── 요리(R16 — 불 자극을 받으면 익는다) ──

	/** 익으면 바뀌는 아이템 ID. 비우면 익지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	FName CookResult;

	/** 불 세기 1 로 익는 데 걸리는 초 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	float CookSeconds = 0.f;

	// ── 스킬북 ──

	/** 가르치는 마법 ID(DA_Spell_<ID>) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skillbook")
	FName TeachesSpell;

	// ── 땅에 떨어진 모양 ──

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	EGrimPickupShape Shape = EGrimPickupShape::Cube;

	/** 모양 크기(cm, 가장 긴 변) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	float Size = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FLinearColor Color = FLinearColor(0.6f, 0.6f, 0.6f);
};

/** 요리 표(Content/Grimoire/Data/Recipes.csv) 한 행. 행 이름 = 레시피 ID. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FString DisplayName;

	/** 어느 시설에서(AGrimCookingPot::StationId) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Station = TEXT("CookingPot");

	/** 재료: "아이템ID:개수" 또는 "#묶음:개수", `|` 로 여럿. 예: "#Meat:1|Herb:1|Salt:1" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FString Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Output;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 OutputCount = 1;

	/** 불 세기 1 로 끓는 데 걸리는 초 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	float CookSeconds = 10.f;
};

/** 아이템 하나(정의 + 개수). 내구도 · 인챈트는 M4 — 자리만 두고 저장 · 복제한다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 0;

	/** 내구도(M4). 음수 = 내구도 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Durability = -1.f;

	/** 인챈트 ID(M4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FName> Enchantments;

	/** 수리한 횟수(M4 — 수리마다 최대 내구도가 준다, 같은 장비 합치기의 작업 횟수) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 RepairCount = 0;

	FGrimItemInstance() = default;
	FGrimItemInstance(FName InItemId, int32 InCount) : ItemId(InItemId), Count(InCount) {}

	bool IsEmpty() const { return ItemId.IsNone() || Count <= 0; }

	/** 같은 칸에 쌓일 수 있나(같은 ID, 내구도 · 인챈트 없음) */
	bool CanStackWith(const FGrimItemInstance& Other) const;

	void Clear() { *this = FGrimItemInstance(); }

	bool operator==(const FGrimItemInstance& Other) const
	{
		return ItemId == Other.ItemId && Count == Other.Count && Durability == Other.Durability && Enchantments == Other.Enchantments && RepairCount == Other.RepairCount;
	}
	bool operator!=(const FGrimItemInstance& Other) const { return !(*this == Other); }

	/**
	 * 세이브 형식(Items.Inventory v1 · 픽업 · 냄비 · 저장고 · 장비 공통): FString ID, int32 개수, float 내구도, int32 인챈트 수, FString × 수.
	 * 수리한 적이 있으면(M4) 인챈트 수의 RepairCountFlag 비트를 켜고 끝에 int32 수리 횟수를 붙인다 — 옛 세이브는 그대로 읽힌다.
	 */
	void Serialize(FArchive& Ar);
};

/** 읽은 정의(표 한 행 + 나눈 묶음) */
struct GRIMOIRE_API FGrimItemDefinition
{
	FName Id;
	FGrimItemRow Row;
	TArray<FName> Groups;

	bool IsInGroup(FName Group) const { return Groups.Contains(Group); }
	bool IsFood() const { return Row.Kind == EGrimItemKind::Food; }
	/** 「사용」으로 무언가 일어나는 종류: 음식(먹기) · 스킬북(읽기) · 장비(입기). UGrimInventoryComponent::UseItem 의 분기와 맞춘다. */
	bool IsUsable() const { return Row.Kind == EGrimItemKind::Food || Row.Kind == EGrimItemKind::Skillbook || Row.Kind == EGrimItemKind::Equipment; }
	bool IsCookable() const { return !Row.CookResult.IsNone() && Row.CookSeconds > 0.f; }
	int32 GetMaxStack() const { return FMath::Max(1, Row.MaxStack); }
	FText GetDisplayName() const;
};

/** 레시피 재료 하나: 아이템 ID 또는 묶음(#) */
struct GRIMOIRE_API FGrimRecipeInput
{
	FName Id;
	bool bGroup = false;
	int32 Count = 1;

	FString ToString() const { return FString::Printf(TEXT("%s%s:%d"), bGroup ? TEXT("#") : TEXT(""), *Id.ToString(), Count); }
};

struct GRIMOIRE_API FGrimRecipeDefinition
{
	FName Id;
	FGrimRecipeRow Row;
	TArray<FGrimRecipeInput> Inputs;
};

/** 재료 하나가 레시피에 얼마나 찼나(냄비 안내) */
struct GRIMOIRE_API FGrimRecipeProgress
{
	const FGrimRecipeDefinition* Recipe = nullptr;
	/** Inputs 와 같은 순서: 채운 개수 */
	TArray<int32> Filled;
	/** 레시피에 안 쓰이는 내용물이 있다 */
	bool bHasExtra = false;

	bool IsComplete() const;
};

/** 아이템 · 요리 표를 묶은 것. 표 검사도 여기서(Grimoire.Items.Table). */
struct GRIMOIRE_API FGrimItemDatabase
{
	TMap<FName, FGrimItemDefinition> Items;
	TArray<FGrimRecipeDefinition> Recipes;

	const FGrimItemDefinition* Find(FName ItemId) const { return Items.Find(ItemId); }

	/** 아이템이 재료 하나에 맞나(ID 같음 또는 묶음에 듦) */
	bool MatchesInput(FName ItemId, const FGrimRecipeInput& Input) const;

	/** 내용물로 본 레시피 진행. 재료 하나에는 ID 재료를 먼저, 묶음 재료를 나중에 채운다. */
	FGrimRecipeProgress EvaluateRecipe(const FGrimRecipeDefinition& Recipe, const TArray<FGrimItemInstance>& Contents) const;

	/** 시설의 레시피 중 내용물이 정확히 다 찬 것(남는 것 없음). 없으면 nullptr. */
	const FGrimRecipeDefinition* FindCompleteRecipe(FName Station, const TArray<FGrimItemInstance>& Contents) const;

	/** 문자열 "A:1|#B:2" 를 읽는다. 못 읽은 조각은 OutProblems 에. */
	static bool ParseInputs(const FString& Text, TArray<FGrimRecipeInput>& OutInputs, TArray<FString>& OutProblems);

	/** CSV 두 개로 만든다. 문제(모르는 결과 · 재료 · 빈 묶음 …)는 OutProblems. 문제가 없으면 true. */
	static bool BuildFromCSV(const FString& ItemsCSV, const FString& RecipesCSV, FGrimItemDatabase& Out, TArray<FString>& OutProblems);
};

/**
 * 가방 칸 계산(세계 없이 — 테스트가 직접 부른다). 칸 배열의 크기는 고정(빈 칸 = IsEmpty).
 */
struct GRIMOIRE_API FGrimInventoryModel
{
	/** 쌓을 수 있는 칸에 먼저, 그다음 빈 칸에. 넣은 개수를 돌려준다(가방이 차면 나머지는 못 넣음). */
	static int32 Add(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const FGrimItemInstance& Item);

	/** 이 아이템을 몇 개 넣을 수 있나 */
	static int32 GetAddableCount(const TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, FName ItemId);

	/** ID 로 Count 개를 뺀다(뒤 칸부터). 뺀 개수. */
	static int32 Remove(TArray<FGrimItemInstance>& Slots, FName ItemId, int32 Count);

	/** 한 칸에서 Count 개를 뺀다. 뺀 것(개수 포함)을 돌려준다. */
	static FGrimItemInstance RemoveAt(TArray<FGrimItemInstance>& Slots, int32 SlotIndex, int32 Count);

	/** From → To: 빈 칸이면 옮기고, 같은 것이면 쌓을 수 있는 만큼 합치고, 다르면 바꾼다. 바뀌었으면 true. */
	static bool Move(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, int32 From, int32 To);

	static int32 CountOf(const TArray<FGrimItemInstance>& Slots, FName ItemId);
};

/** 요리 진행(픽업 · 냄비 공통) */
struct GRIMOIRE_API FGrimCookingModel
{
	/** 불 세기 → 익는 빠르기(초당 진행 초). 세기 0 이면 0. 바람으로 키운 불(세기 2 · 3)은 빨라진다. */
	static float GetCookRate(float HeatIntensity, float ExtraRatePerIntensity);

	/** 진행을 더하고, 다 익었으면(Progress ≥ CookSeconds) true */
	static bool Advance(float& InOutProgress, float HeatIntensity, float DeltaSeconds, float CookSeconds, float ExtraRatePerIntensity);
};

/** 먹기 계산 */
struct GRIMOIRE_API FGrimFoodModel
{
	/** 날것의 배탈 확률(저항 반영 전). 날것이 아니면 0, 표의 값이 음수면 DefaultRawChance. */
	static float GetBaseDiseaseChance(const FGrimItemDefinition& Food, float DefaultRawChance);
};

/** 퀘스트 이벤트 이름(docs/systems/tutorial 「이벤트」) */
namespace GrimItemEvents
{
	GRIMOIRE_API extern const FName FoodEaten;            // Food.Eaten
	GRIMOIRE_API extern const FName CookingIngredientsAdded; // Cooking.IngredientsAdded
	GRIMOIRE_API extern const FName CookingCompleted;     // Cooking.Completed
	GRIMOIRE_API extern const FName FoodCooked;           // Cooking.Cooked(바닥 · 불 위에서 익음 — 튜토리얼에는 없음)

	/** Item.Obtained.<IdOrGroup> */
	GRIMOIRE_API FName Obtained(FName IdOrGroup);
}
