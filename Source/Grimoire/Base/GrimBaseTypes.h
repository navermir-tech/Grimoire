#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Items/GrimItemTypes.h"
#include "GrimBaseTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimBase, Log, All);

/** 건축 조각 범주(docs/systems/m4-plan 「격자 · 지지」). 앞의 여섯(Foundation ~ Pillar)이 격자 구조 조각이다. */
UENUM(BlueprintType)
enum class EGrimBuildCategory : uint8
{
	/** 거점 핵 — 자유 배치, 격자의 원점 */
	Core,
	Foundation,
	Wall,
	Door,
	Roof,
	Stair,
	Pillar,
	/** 시설(침대 · 모닥불 · 저장고 · 작업대 · 밭) — 격자 없이 자유 배치 */
	Station,
};

/** 조각 재료. 나무는 불이 붙어 탄다(R16 · R18). */
UENUM(BlueprintType)
enum class EGrimBuildMaterial : uint8
{
	None,
	Wood,
	Stone,
};

/** 장비 칸(Q21). 순서가 장비 컴포넌트의 칸 번호다 — 바꾸면 세이브(Base.Equipment)를 올린다. */
UENUM(BlueprintType)
enum class EGrimEquipSlot : uint8
{
	Wand,
	Head,
	Body,
	Cloak,
	Hands,
	Feet,
	Ring,
	Necklace,
	Count UMETA(Hidden),
};

/** 건축 조각 표(Content/Grimoire/Data/BuildPieces.csv) 한 행. 행 이름 = 조각 ID. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimBuildPieceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	EGrimBuildCategory Category = EGrimBuildCategory::Foundation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	EGrimBuildMaterial Material = EGrimBuildMaterial::Wood;

	/** 비용 "아이템ID:개수|…"(묶음 # 은 쓰지 않는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FString Cost;

	/** 최대 내구도(설정 StructureMaxHealthScale 이 곱해진다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	float MaxHealth = 100.f;

	/** 수리 재료(1개 = 최대 내구도 × RepairFractionPerMaterial). 비우면 고칠 수 없다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FName RepairItem;

	/** 시설 · 거점 핵의 반 크기(cm) — 고스트와 놓을 자리 검사. 구조 조각은 격자가 정한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FVector Extent = FVector(50.f, 50.f, 50.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FLinearColor Color = FLinearColor(0.5f, 0.35f, 0.2f);

	/** 시설 · 거점 핵이면 스폰할 클래스 경로(예: /Script/Grimoire.GrimBaseBed). 구조 조각은 비운다(AGrimBuildPiece) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FString SpawnClass;

	/** 건축 패널의 순서 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	int32 SortOrder = 0;
};

/** 작물 표(Content/Grimoire/Data/Crops.csv) 한 행. 행 이름 = 작물 ID. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCropRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FString DisplayName;

	/** 심는 씨앗 아이템 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FName SeedItem;

	/** 거두면 나오는 아이템 · 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FName HarvestItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 HarvestCount = 2;

	/** 거둘 때 돌려주는 씨앗 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 SeedReturn = 1;

	/** 조건이 다 맞을 때 다 자라는 데 걸리는 실제 초 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float GrowSeconds = 1200.f;

	/** 보이는 단계 수(다 자람 제외) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 Stages = 3;

	/** 마른 밭에서의 생장 배율(0 = 물이 없으면 멈춤) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float DryGrowthScale = 0.f;

	/** 밤(빛 마법 없음)의 생장 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float NightGrowthScale = 0.5f;

	/** 견디는 추위 · 더위 등급(넘으면 자라지 않는다 — 설산은 온실) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 MaxColdGrade = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	int32 MaxHeatGrade = 2;

	/** 한 번 젖으면 물기가 남는 초 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float MoistureSeconds = 900.f;

	/** 재(불에 탄 밭) 비료의 생장 보너스(0.2 = +20%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	float AshBonus = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
	FLinearColor Color = FLinearColor(0.3f, 0.7f, 0.2f);
};

/** 장비 표(Content/Grimoire/Data/Equipment.csv) 한 행. 행 이름 = 아이템 ID(Items.csv 에 Kind=Equipment 로 있어야 한다). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimEquipmentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EGrimEquipSlot Slot = EGrimEquipSlot::Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	float MaxDurability = 100.f;

	/** 수리 재료(R18 — 만든 재료 중 대표 하나) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName RepairItem;

	/** 닳는 양: 완드 = 시전 한 번, 방어구 = 피해 ArmorWearDamageUnit 당, 장신구 = AccessoryWearIntervalSeconds 당 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	float WearPerUse = 1.f;

	/** 체온 보정(정수 등급 — R25) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	float ColdResist = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	float HeatResist = 0.f;
};

struct GRIMOIRE_API FGrimBuildPieceDefinition
{
	FName Id;
	FGrimBuildPieceRow Row;
	TArray<FGrimRecipeInput> Cost;

	bool IsStructural() const { return Row.Category >= EGrimBuildCategory::Foundation && Row.Category <= EGrimBuildCategory::Pillar; }
	bool IsFreePlaced() const { return !IsStructural(); }
	bool IsFlammable() const { return Row.Material == EGrimBuildMaterial::Wood; }
	FText GetDisplayName() const { return FText::FromString(Row.DisplayName.IsEmpty() ? Id.ToString() : Row.DisplayName); }
	/** 비용 "나무 4 · 돌 1"(이름은 아이템 표) */
	FString DescribeCost(const FGrimItemDatabase& Items) const;
};

struct GRIMOIRE_API FGrimCropDefinition
{
	FName Id;
	FGrimCropRow Row;
	FText GetDisplayName() const { return FText::FromString(Row.DisplayName.IsEmpty() ? Id.ToString() : Row.DisplayName); }
};

struct GRIMOIRE_API FGrimEquipmentDefinition
{
	FName ItemId;
	FGrimEquipmentRow Row;
};

/** 건축 · 작물 · 장비 표 묶음. 검사도 여기서(Grimoire.Base.Table). */
struct GRIMOIRE_API FGrimBaseDatabase
{
	TMap<FName, FGrimBuildPieceDefinition> Pieces;
	/** 건축 패널 순서(SortOrder → ID) — 거점 핵 포함 */
	TArray<FName> PieceOrder;
	TMap<FName, FGrimCropDefinition> Crops;
	TMap<FName, FGrimEquipmentDefinition> Equipment;

	const FGrimBuildPieceDefinition* FindPiece(FName Id) const { return Pieces.Find(Id); }
	const FGrimCropDefinition* FindCrop(FName Id) const { return Crops.Find(Id); }
	const FGrimCropDefinition* FindCropBySeed(FName SeedItem) const;
	const FGrimEquipmentDefinition* FindEquipment(FName ItemId) const { return Equipment.Find(ItemId); }

	/** CSV 셋으로 만든다. Items 가 있으면 아이템 ID 를 맞춰 본다. 문제가 없으면 true. */
	static bool BuildFromCSV(const FString& PiecesCSV, const FString& CropsCSV, const FString& EquipmentCSV, const FGrimItemDatabase* Items,
		FGrimBaseDatabase& Out, TArray<FString>& OutProblems);
};

// ───────────────────────── 격자 (Q33) ─────────────────────────

/** 격자 위 자리. 구조 조각만 쓴다(시설은 자유 배치). 서버 RPC 인자 · 세이브에 쓴다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimBuildSlot
{
	GENERATED_BODY()

	UPROPERTY()
	EGrimBuildCategory Category = EGrimBuildCategory::Foundation;

	/** 칸(바닥 · 지붕 · 계단) · 모서리의 앞 칸 · 꼭짓점의 앞 칸 */
	UPROPERTY()
	int32 X = 0;

	UPROPERTY()
	int32 Y = 0;

	/** 층(0 = 땅 위) */
	UPROPERTY()
	int32 Level = 0;

	/** 모서리 방향(0 = 칸 X 와 X+1 사이, 1 = 칸 Y 와 Y+1 사이) 또는 계단이 올라가는 방향(0~3, 90° 단위) */
	UPROPERTY()
	int32 Dir = 0;

	bool operator==(const FGrimBuildSlot& Other) const
	{
		return Category == Other.Category && X == Other.X && Y == Other.Y && Level == Other.Level && Dir == Other.Dir;
	}

	FString ToString() const;

	/** 자리 겹침 열쇠: 벽 · 문은 같은 모서리를 나눠 쓰고, 바닥 · 지붕 · 계단 · 기둥은 저마다의 층이다 */
	uint64 GetOccupancyKey() const;

	static uint64 MakeKey(uint8 Layer, int32 X, int32 Y, int32 Level, int32 Dir);
};

/** 상자 하나(조각 모양 — 액터 기준 로컬). 아트 전에는 엔진 큐브로 그린다. */
struct GRIMOIRE_API FGrimShapeBox
{
	FVector Center = FVector::ZeroVector;
	FVector Extent = FVector(50.f);
	FRotator Rotation = FRotator::ZeroRotator;
	/** 문짝(열면 콜리전 · 보임이 꺼진다) */
	bool bDoorLeaf = false;
};

/** 격자 계산(순수 — 테스트가 직접 부른다). 원점 = 거점 핵 위치 · 방향. */
struct GRIMOIRE_API FGrimBuildGrid
{
	FVector Origin = FVector::ZeroVector;
	float Yaw = 0.f;
	float CellSize = 300.f;
	float WallHeight = 300.f;
	float SlabThickness = 20.f;
	float FloorTopOffset = 30.f;
	float FoundationDepth = 100.f;
	int32 MaxLevel = 3;

	float GetLevelHeight() const { return WallHeight + SlabThickness; }
	/** 층의 바닥 윗면 높이(원점 기준) */
	float GetFloorTop(int32 Level) const { return FloorTopOffset + Level * GetLevelHeight(); }

	FVector LocalToWorld(const FVector& Local) const;
	FVector WorldToLocal(const FVector& World) const;

	/** 조준점 → 자리. RotationQuarter 는 계단 방향. */
	FGrimBuildSlot Snap(EGrimBuildCategory Category, const FVector& AimWorld, int32 RotationQuarter) const;

	/** 자리의 기준점(로컬): 칸 중심 · 모서리 가운데 · 꼭짓점, 높이는 바닥 윗면(지붕은 벽 위) */
	FVector GetSlotAnchorLocal(const FGrimBuildSlot& Slot) const;

	/** 조각 액터를 놓을 월드 변환 */
	FTransform GetSlotTransform(const FGrimBuildSlot& Slot) const;

	/** 범주별 모양(액터 기준 로컬) */
	void GetShape(EGrimBuildCategory Category, TArray<FGrimShapeBox>& OutBoxes) const;
};

/** 지지 · 무너짐(순수). */
struct GRIMOIRE_API FGrimBuildSupport
{
	/**
	 * 자리 하나가 받쳐지나. Stable = 이미 받쳐진 조각의 열쇠 → 직접 받쳐졌나(지붕 내밀기는 직접 받쳐진 이웃 지붕만).
	 * bOutDirect = 이웃 지붕이 아니라 벽 · 기둥 · 바닥으로 받쳐짐.
	 */
	static bool HasSupport(const FGrimBuildSlot& Slot, const TMap<uint64, bool>& Stable, bool* bOutDirect = nullptr);

	/** 받쳐지는 것을 땅부터 차례로 모은다(열쇠 → 직접 받쳐졌나). Unsupported 가 있으면 받쳐지지 않은 번호를 넣는다. */
	static TMap<uint64, bool> ComputeStable(const TArray<FGrimBuildSlot>& Slots, TArray<int32>* OutUnsupported = nullptr);

	/** 받쳐지는 것을 땅부터 차례로 모아 남은(무너질) 자리의 번호를 돌려준다. */
	static TArray<int32> FindUnsupported(const TArray<FGrimBuildSlot>& Slots);
};

// ───────────────────────── 내구도 · 작물 · 옮기기 (순수) ─────────────────────────

struct GRIMOIRE_API FGrimDurabilityMath
{
	/** 장비의 지금 최대 내구도: 수리마다 LossPerRepair 씩 줄고 MinFraction 아래로는 안 준다 */
	static float GetEquipmentMax(float BaseMax, int32 RepairCount, float LossPerRepair, float MinFraction);

	/** 모자란 내구도를 다 채우는 데 드는 재료 수(0 이면 이미 가득) */
	static int32 GetMaterialsNeeded(float Current, float Max, float FractionPerMaterial);

	/** 재료 Materials 개로 고친 뒤의 내구도(최대를 넘지 않음) */
	static float Repair(float Current, float Max, int32 Materials, float FractionPerMaterial);

	/** 초당 노후(거점 안 · 밖) */
	static float GetDecayPerSecond(float Max, bool bInsideBase, float InsideFractionPerHour, float OutsideFractionPerHour);

	/** 방어구가 피해를 받아 닳는 양 */
	static float GetArmorWear(float Damage, float WearPerUse, float DamageUnit);

	/** 부서지거나 철거하면 돌려주는 개수(비용 × 비율, 내림) */
	static int32 GetRefundCount(int32 Cost, float Fraction);
};

/** 밭의 지금 조건 */
struct GRIMOIRE_API FGrimCropConditions
{
	bool bWet = false;
	bool bDaylight = true;
	/** 빛 마법(State.Illuminated) — 밤에도 낮처럼 */
	bool bIlluminated = false;
	int32 ColdGrade = 0;
	int32 HeatGrade = 0;
	bool bAsh = false;
};

struct GRIMOIRE_API FGrimCropMath
{
	/** 초당 생장(0 → 1 이면 다 자람) */
	static float GetGrowthRate(const FGrimCropRow& Crop, const FGrimCropConditions& Conditions);

	/** 보이는 단계: 0 ~ Stages-1, 다 자라면 Stages */
	static int32 GetStage(float Growth, int32 Stages);

	static bool IsMature(float Growth) { return Growth >= 1.f - KINDA_SMALL_NUMBER; }
};

/** 칸 사이 옮기기 · 제작(순수 — 가방 · 저장고 공통) */
struct GRIMOIRE_API FGrimSlotTransferMath
{
	/** From[FromIndex] 에서 Count 개(음수 = 전부)를 To 로 옮긴다. 들어간 만큼만 빠진다. 옮긴 개수. */
	static int32 Transfer(TArray<FGrimItemInstance>& From, int32 FromIndex, int32 Count, TArray<FGrimItemInstance>& To, const FGrimItemDatabase& Database);

	/** 레시피 재료가 칸에 다 있나(ID 재료 먼저, 묶음 재료 나중) */
	static bool HasInputs(const TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const TArray<FGrimRecipeInput>& Inputs);

	/** 재료를 뺀다(HasInputs 가 참일 때만). 뺐으면 true. */
	static bool ConsumeInputs(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const TArray<FGrimRecipeInput>& Inputs);
};

/** 퀘스트 이벤트 이름 */
namespace GrimBaseEvents
{
	GRIMOIRE_API extern const FName CorePlaced;      // Base.CorePlaced
	GRIMOIRE_API extern const FName SleptInBase;     // Base.SleptInBase(지붕 아래 침대에서 잠)
	GRIMOIRE_API FName PiecePlaced(EGrimBuildCategory Category);  // Base.Placed.<범주>
	GRIMOIRE_API FName Harvested(FName CropId);      // Crop.Harvested.<작물>
	GRIMOIRE_API FName Crafted(FName ItemId);        // Item.Crafted.<아이템>
}
