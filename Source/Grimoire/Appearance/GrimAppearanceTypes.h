#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GrimAppearanceTypes.generated.h"

class UDataTable;
class USkeletalMesh;
class UStaticMesh;

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimAppearance, Log, All);

/** 외형 칸 (docs/spec/customization 「의상 조각」). 순서를 바꾸면 복제 형식이 바뀐다 — 저장은 이름이라 괜찮다. */
UENUM(BlueprintType)
enum class EGrimAppearanceSlot : uint8
{
	Face,
	Hair,
	Beard,
	Hat,
	Robe,
	Stole,
	Belt,
	Cloak,
	Gloves,
	Boots,
	Legs,
	Count UMETA(Hidden)
};

constexpr int32 GrimAppearanceSlotCount = static_cast<int32>(EGrimAppearanceSlot::Count);

/** 재질 색 칸 — 모든 조각 재질이 같은 이름의 벡터 파라미터를 읽는다(docs/systems/customization 「결정」 5). */
UENUM(BlueprintType)
enum class EGrimAppearanceChannel : uint8
{
	None,
	Primary,
	Secondary,
	Leather,
	Metal,
	Lining,
	Skin,
	Hair,
	Eye,
	/** 바지 · 속옷 층(고정 색) */
	Under,
	Count UMETA(Hidden)
};

constexpr int32 GrimAppearanceChannelCount = static_cast<int32>(EGrimAppearanceChannel::Count);

/** 팔레트 이름(AppearancePalettes.csv 의 Palette 칸) */
namespace GrimAppearancePalettes
{
	GRIMOIRE_API extern const FName Skin;
	GRIMOIRE_API extern const FName Eye;
	GRIMOIRE_API extern const FName Hair;
	GRIMOIRE_API extern const FName Cloth;
	/** 속성 색 — 행 순서 = 물 · 불 · 바람 · 땅 · 빛 · 어둠 (FGrimAppearance::Element − 1) */
	GRIMOIRE_API extern const FName Element;
	/** 가죽 · 놋쇠 · 속옷 층 — 행 이름으로 찾는다 */
	GRIMOIRE_API extern const FName Fixed;
}

// ───────────────────────── 표 행 (CSV · DataTable) ─────────────────────────

/** AppearanceParts.csv — 조각 하나. 행 이름 = 조각 ID. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAppearancePartRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	EGrimAppearanceSlot Slot = EGrimAppearanceSlot::Robe;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString DisplayName;

	/** 맞는 체형 "A|B". 비우면 둘 다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString BodyTypes;

	/** 체형 A · B 용 스켈레탈 조각(같은 마네킹 스켈레톤). 비우면 소품(AppearancePieces)만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> MeshA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> MeshB;

	/** 플레이어 생성 화면에 나오나(NPC 전용 직책 옷은 false) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool Creator = false;

	/** 모자: 쓰면 머리 모양 칸을 숨긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool HidesHair = false;

	/** 얼굴 조각: 기본 슬라이더 값 "EyeSize=0.2|JawWidth=-0.1" (−1~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString Morphs;
};

/** AppearancePieces.csv — 조각에 붙는 스태틱 소품 하나(자리 표시 도형도 이것). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAppearancePieceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FName Part;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 붙일 뼈. 없으면 메시 컴포넌트 원점 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FName Bone;

	/** 마네킹 기준 자세에서 그 뼈 위치로부터의 오프셋(메시 공간 cm — Z 위, Y 앞) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FVector Offset = FVector::ZeroVector;

	/** 메시 공간 회전 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	EGrimAppearanceChannel Channel = EGrimAppearanceChannel::Primary;

	/** "Circle" = 서클 수만큼 Step 간격으로 되풀이(스톨 고리). 비우면 하나 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString Repeat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FVector Step = FVector::ZeroVector;

	/** 얼굴 슬라이더의 자리 표시 크기 변화(PlaceholderScale)를 받나 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool Face = false;

	/** 작은 소품 — 설정 SmallPieceCullDistance 밖에서 숨긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool Small = false;

	/** 이 소품이 붙는 체형 "A|B". 비우면 둘 다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString BodyTypes;
};

/** AppearancePalettes.csv — 색 하나. 행 이름 = 색 ID, 팔레트 안 순서 = 표 순서. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAppearancePaletteRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FName Palette;

	/** "232A45" (sRGB) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString Hex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString DisplayName;

	/** 플레이어가 고를 수 있나(흰머리 등 NPC 용 색은 false) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool Creator = true;
};

/** AppearanceMorphs.csv — 얼굴 슬라이더 하나. 행 이름 = 슬라이더 ID, 순서 = 표 순서(복제 순서). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAppearanceMorphRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString DisplayName;

	/** 머리 메시의 모프 타깃 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FName MorphTarget;

	/** 자리 표시 머리(Face 소품)에 값 1 일 때 더할 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FVector PlaceholderScale = FVector::ZeroVector;
};

/**
 * NpcAppearanceRules.csv — 역할 규칙 하나. 칸마다 후보 "조각ID:가중치|…"("-" = 비움, 가중치 생략 = 1). 비운 칸 = 늘 비움.
 * 색 칸은 색 ID 목록("*" 또는 비움 = 그 팔레트 전부, 플레이어 규칙은 Creator 색만).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimNpcAppearanceRuleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString DisplayName;

	/** 플레이어 규칙(시작 · 무작위) — 플레이어가 고를 수 있는 조각 · 색만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool Player = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	float BodyA = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	float BodyB = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Face;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Hair;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Beard;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Hat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Robe;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Stole;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Belt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Cloak;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Gloves;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Boots;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Legs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Skin;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Eye;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString HairColor;
	/** 이 확률로 GreyHairColor 에서 고른다(나이 든 역할) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") float GreyHair = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString GreyHairColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString ClothPrimary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString ClothSecondary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") int32 CircleMin = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") int32 CircleMax = 1;

	/** 속성 이름 목록 "Fire|Water", "*" = 여섯 계열 전부, 비움 = 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") FString Elements;

	/** 얼굴 슬라이더를 얼굴 기본값에서 ± 이만큼 흔든다(0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") float MorphJitter = 0.3f;

	/** 키를 범위의 ± 이 비율만큼 흔든다(0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance") float HeightJitter = 0.5f;
};

/** NpcAppearance.csv — 이름 있는 NPC 하나. 행 이름 = NpcId(TowerNpcs.csv 와 같은 이름). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimNpcAppearanceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FName Rule;

	/** 0 = NpcId 해시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	int32 Seed = 0;

	/** 0 = 규칙대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	int32 Circle = 0;

	/** 비움 = 규칙대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FString Element;
};

/**
 * 외형 카탈로그를 에디터 DataAsset 으로 둘 때(설정 CatalogAsset). 표 여섯 개를 가리킨다 — 비어 있으면 CSV 를 쓴다.
 * 표의 행 구조체는 위 FGrim…Row 들.
 */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimAppearanceCatalogAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimAppearancePartRow"))
	TObjectPtr<UDataTable> Parts;

	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimAppearancePieceRow"))
	TObjectPtr<UDataTable> Pieces;

	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimAppearancePaletteRow"))
	TObjectPtr<UDataTable> Palettes;

	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimAppearanceMorphRow"))
	TObjectPtr<UDataTable> Morphs;

	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimNpcAppearanceRuleRow"))
	TObjectPtr<UDataTable> Rules;

	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (RequiredAssetDataTags = "RowStructure=/Script/Grimoire.GrimNpcAppearanceRow"))
	TObjectPtr<UDataTable> Npcs;
};

// ───────────────────────── 외형 구조체 ─────────────────────────

/**
 * 한 사람의 외형 전부 — 플레이어 · NPC 공용. 서버가 정하고 복제 · 저장한다(docs/systems/customization 「데이터 모델」).
 * 복제(NetSerialize)는 카탈로그 번호로 작게, 저장(Serialize)은 이름으로(카탈로그가 바뀌어도 읽히게).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimAppearance
{
	GENERATED_BODY()

	static constexpr uint8 SaveVersion = 1;
	static constexpr uint8 NeutralByte = 128;
	static constexpr int32 MaxCircle = 10;
	static constexpr int32 ElementCount = 6;

	/** 0 = 체형 A, 1 = 체형 B */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	uint8 BodyType = 0;

	/** 128 = 기준 키(설정 HeightRange 로 펼친다) */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	uint8 Height = NeutralByte;

	/** 칸(EGrimAppearanceSlot)마다 조각 ID. 비었으면 아직 외형이 정해지지 않은 것 */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	TArray<FName> Parts;

	/** 얼굴 슬라이더(128 = 0), 카탈로그 슬라이더 순서 */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	TArray<uint8> Morphs;

	UPROPERTY(BlueprintReadOnly, Category = "Appearance") uint8 SkinColor = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Appearance") uint8 EyeColor = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Appearance") uint8 HairColor = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Appearance") uint8 ClothPrimary = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Appearance") uint8 ClothSecondary = 0;

	/** 안감 · 완드 보석 속성 0 없음 · 1 물 · 2 불 · 3 바람 · 4 땅 · 5 빛 · 6 어둠 */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	uint8 Element = 0;

	/** 스톨 고리 수(0~10) */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	uint8 Circle = 0;

	/** NPC 생성 규칙 · 씨앗(플레이어 = None · 0) */
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FName Rule;

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	int32 Seed = 0;

	bool IsSet() const { return Parts.Num() > 0; }

	FName GetPart(EGrimAppearanceSlot Slot) const;
	void SetPart(EGrimAppearanceSlot Slot, FName PartId);

	/** −1 ~ 1 */
	float GetMorph(int32 Index) const;
	void SetMorph(int32 Index, float Value);

	/** −1 ~ 1 → 0 ~ 255 (128 = 0) */
	static uint8 QuantizeUnit(float Value);
	static float DequantizeUnit(uint8 Byte);

	/** 저장 형식(이름) — 읽은 뒤에는 FGrimAppearanceMath::Sanitize 를 부른다 */
	void Serialize(FArchive& Ar);

	/** 복제 형식(카탈로그 번호) */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/** 저장 형식 바이트의 CRC — 서버 · 클라이언트가 같은 외형을 보는지 비교 */
	uint32 GetHash() const;

	FString ToString() const;

	bool operator==(const FGrimAppearance& Other) const;
	bool operator!=(const FGrimAppearance& Other) const { return !(*this == Other); }
};

template<>
struct TStructOpsTypeTraits<FGrimAppearance> : public TStructOpsTypeTraitsBase2<FGrimAppearance>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true
	};
};

// ───────────────────────── 카탈로그 (런타임) ─────────────────────────

struct FGrimAppearancePiece
{
	FGrimAppearancePieceRow Row;
	FName Id;
	uint8 BodyMask = 3;
	bool bRepeatCircle = false;
};

struct FGrimAppearancePart
{
	FName Id;
	FGrimAppearancePartRow Row;
	/** bit0 = A, bit1 = B */
	uint8 BodyMask = 3;
	/** 얼굴 기본 슬라이더 값(카탈로그 슬라이더 순서, 없으면 0) */
	TArray<float> BaseMorphs;
	/** 이 조각의 소품(Pieces 의 번호) */
	TArray<int32> Pieces;

	bool FitsBody(uint8 BodyType) const { return (BodyMask & (1u << FMath::Min<uint8>(BodyType, 1))) != 0; }
};

struct FGrimAppearanceColor
{
	FName Id;
	FString DisplayName;
	FLinearColor Color = FLinearColor::White;
	bool bCreator = true;
};

struct FGrimAppearanceMorph
{
	FName Id;
	FGrimAppearanceMorphRow Row;
};

struct FGrimAppearanceCandidate
{
	/** None = 비움 */
	FName Part;
	float Weight = 1.f;
};

struct FGrimAppearanceRule
{
	FName Id;
	FGrimNpcAppearanceRuleRow Row;
	TArray<FGrimAppearanceCandidate> Slots[GrimAppearanceSlotCount];
	TArray<uint8> Skin, Eye, Hair, GreyHair, ClothPrimary, ClothSecondary, Elements;
};

struct FGrimNpcAppearanceBinding
{
	FName Rule;
	int32 Seed = 0;
	int32 Circle = 0;
	uint8 Element = 0;
	bool bElementOverride = false;
};

/** 표 여섯 개 → 정렬된 조각 · 팔레트 · 슬라이더 · 규칙. 서버 · 클라이언트가 같은 CSV 에서 같은 순서를 만든다(조각 · 규칙은 ID 문자열 정렬). */
struct GRIMOIRE_API FGrimAppearanceCatalog
{
	TArray<FGrimAppearancePart> Parts;
	TArray<FGrimAppearancePiece> Pieces;
	TMap<FName, TArray<FGrimAppearanceColor>> Palettes;
	TArray<FGrimAppearanceMorph> Morphs;
	TArray<FGrimAppearanceRule> Rules;
	TMap<FName, FGrimNpcAppearanceBinding> Npcs;

	/** CSV 글 여섯 개로 만든다(Parts · Pieces · Palettes · Morphs · Rules · Npcs). 문제는 OutProblems 에. */
	static void BuildFromCSV(const FString& PartsCSV, const FString& PiecesCSV, const FString& PalettesCSV, const FString& MorphsCSV,
		const FString& RulesCSV, const FString& NpcsCSV, FGrimAppearanceCatalog& Out, TArray<FString>& OutProblems);

	static void BuildFromTables(const UDataTable* PartsTable, const UDataTable* PiecesTable, const UDataTable* PalettesTable, const UDataTable* MorphsTable,
		const UDataTable* RulesTable, const UDataTable* NpcsTable, FGrimAppearanceCatalog& Out, TArray<FString>& OutProblems);

	const FGrimAppearancePart* FindPart(FName Id) const;
	int32 FindPartIndex(FName Id) const;
	const FGrimAppearanceRule* FindRule(FName Id) const;
	int32 FindRuleIndex(FName Id) const;
	int32 FindMorph(FName Id) const;
	const TArray<FGrimAppearanceColor>& GetPalette(FName Palette) const;
	int32 FindColorIndex(FName Palette, FName ColorId) const;
	FLinearColor GetColor(FName Palette, int32 Index, const FLinearColor& Fallback = FLinearColor::White) const;
	FLinearColor GetFixedColor(FName ColorId, const FLinearColor& Fallback = FLinearColor::White) const;

	/** 그 칸 · 체형에 맞는 조각 ID(카탈로그 순서). bCreatorOnly = 플레이어 생성 화면용 */
	TArray<FName> GetPartsForSlot(EGrimAppearanceSlot Slot, uint8 BodyType, bool bCreatorOnly) const;

	/** 팔레트에서 고를 수 있는 칸 번호 */
	TArray<uint8> GetColorChoices(FName Palette, bool bCreatorOnly) const;

	/** 복제 번호(0 = 없음) */
	uint16 PartToNet(FName Id) const;
	FName PartFromNet(uint16 Net) const;
	uint16 RuleToNet(FName Id) const;
	FName RuleFromNet(uint16 Net) const;

private:
	TMap<FName, int32> PartIndex;
	TMap<FName, int32> RuleIndex;
	TMap<FName, int32> MorphIndex;
};

/** 외형 계산 — 순수 함수(Grimoire.Appearance.* 테스트). */
struct GRIMOIRE_API FGrimAppearanceMath
{
	/** 칸 · 체형 · 범위가 맞게 고친다. bPlayer = 플레이어가 고를 수 있는 조각 · 색만, 규칙 · 씨앗 지움. 고친 게 있으면 true */
	static bool Sanitize(FGrimAppearance& InOut, const FGrimAppearanceCatalog& Catalog, bool bPlayer);

	/** 규칙 + 씨앗 → 외형. 같은 카탈로그 · 규칙 · 씨앗이면 어느 컴퓨터에서나 같다. 규칙이 없으면 빈 외형 */
	static FGrimAppearance GenerateFromRule(const FGrimAppearanceCatalog& Catalog, FName RuleId, int32 Seed);

	/** 이름 있는 NPC: NpcAppearance 표(규칙 · 씨앗 · 고정값). 표에 없으면 FallbackRule + 이름 해시 */
	static FGrimAppearance GenerateForNpc(const FGrimAppearanceCatalog& Catalog, FName NpcId, FName FallbackRule);

	/** 이름 → 플랫폼과 실행에 상관없는 씨앗(소문자 문자열 CRC) */
	static int32 MakeSeed(const FString& Text);

	/** 얼굴 조각을 바꿀 때: 슬라이더를 그 얼굴의 기본값으로 */
	static void ApplyFaceDefaults(FGrimAppearance& InOut, const FGrimAppearanceCatalog& Catalog);

	/** 칸마다 실제 색(팔레트 → 채널) */
	static void ResolveColors(const FGrimAppearance& Appearance, const FGrimAppearanceCatalog& Catalog, FLinearColor (&OutColors)[GrimAppearanceChannelCount]);

	/** "Fire" · "불" → 2, 모르면 0 */
	static uint8 ElementFromName(const FString& Name);
	static FString ElementName(uint8 Element);
	static FText ElementDisplayName(uint8 Element);

	/** "A|B" → 비트(비우면 3) */
	static uint8 ParseBodyMask(const FString& Text);

	static FString SlotName(EGrimAppearanceSlot Slot);
	static FText SlotDisplayName(EGrimAppearanceSlot Slot);
	static FName ChannelParameterName(EGrimAppearanceChannel Channel);
};
