#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GrimDressingTypes.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimDressing, Log, All);

/** 바뀐 상태를 언제 처음(표의 InitialState)으로 되돌리나 — 잠정(Q153: 자원은 다음 아침에 다시 난다). */
UENUM(BlueprintType)
enum class EGrimPropReset : uint8
{
	/** 되돌리지 않는다(모닥불 · 책장) */
	Never,
	/** 해가 뜨면(UGrimTimeOfDaySubsystem::OnNightEnd) — 등잔이 꺼지고 꽃이 다시 시들고 솜 · 마른 풀이 다시 난다 */
	Dawn,
	/** 처음과 달라진 뒤 ResetSeconds 초 */
	Seconds
};

/** 덧붙은 메시(솜 · 재 자국)를 언제 보이나. */
UENUM(BlueprintType)
enum class EGrimPropAccentShow : uint8
{
	Always,
	/** AltState 가 없을 때만(솜꽃의 솜) */
	Default,
	/** AltState 가 있을 때만(마른 풀이 탄 뒤의 재 자국) */
	Alt
};

/** 고리 이펙트(나비 떼)를 언제 켜나. LoopFXHideState 가 있으면 언제든 끈다. */
UENUM(BlueprintType)
enum class EGrimPropFXWhen : uint8
{
	Always,
	Day,
	Night,
	/** AltState 가 있을 때만(핀 꽃에 나비) */
	Alt
};

/** 채집물 소유 — 잠정(Q153: 자원은 세계 공유, 수집품은 사람별 한 번). */
UENUM(BlueprintType)
enum class EGrimHarvestOwnership : uint8
{
	/** 한 사람이 거두면 모두에게 사라진다 */
	Shared,
	/** 사람마다 한 번씩 — 그 사람에게만 사라진다 */
	PerPlayer
};

/**
 * 반응 소품 한 종 (ReactiveProps.csv 한 행 — 행 이름 = PropId, 예: D01_WindCotton).
 * 반응 자체는 ElementalReactions.csv(재질 · 상태 규칙)가 정하고, 이 행은 생김새 · 보상 · 되돌리기만 정한다.
 * 상태 → 생김새: AltState 가 있으면 Alt* 모습. 불(State.Burning)은 공통 액터의 불꽃 · 빛.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimReactivePropRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString Note;

	/** 안내 · 도감에 쓰는 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString DisplayName;

	// ── 몸 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 비우면 메시 기본 재질 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TSoftObjectPtr<UMaterialInterface> MeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector MeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector MeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FRotator MeshRotation = FRotator::ZeroRotator;

	/** 그리기 끝 거리(cm, 0 = 끝 없음) — 성능 규칙 10 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0"))
	float CullDistance = 12000.f;

	/** 폰을 막는가(등잔 · 모닥불 돌). 풀 · 꽃 · 종이는 겹치기만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	bool bBlocksPawn = false;

	/** 소프트 락온에 잡히는가 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	bool bTargetable = true;

	/** 마법 · 비 · 불 번짐이 닿는 상자(반 크기 cm). 보이는 메시는 충돌이 없다(팩 풀 메시는 충돌이 없기도 하다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector HitExtent = FVector(30.f, 30.f, 30.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector HitOffset = FVector(0.f, 0.f, 30.f);

	// ── 속성 ──
	/** 반응 규칙의 재질 조건(예: (Material.Seedhead)) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental", meta = (Categories = "Material"))
	TArray<FGameplayTag> Materials;

	/** 처음 상태(예: State.Wilted). 되돌리기도 이것으로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental", meta = (Categories = "State"))
	FGameplayTag InitialState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental", meta = (ClampMin = "0"))
	float InitialIntensity = 1.f;

	// ── 바뀐 모습 ──
	/** 이 상태가 있으면 Alt* 모습(예: State.Spent · State.Bloomed · State.Charred) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt", meta = (Categories = "State"))
	FGameplayTag AltState;

	/** 비우면 몸 메시 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	TSoftObjectPtr<UStaticMesh> AltMesh;

	/** 비우면 몸 재질 그대로(MeshMaterial) — 시든 꽃이 피면 색이 돌아온다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	TSoftObjectPtr<UMaterialInterface> AltMaterial;

	/** 0 = 몸 크기 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	FVector AltScale = FVector::ZeroVector;

	/** 몸 회전 대신(시든 꽃이 고개를 든다). 바뀌지 않게 하려면 MeshRotation 과 같게 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	FRotator AltRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	bool bAltUsesRotation = false;

	/** 바뀐 모습에서 몸을 숨긴다(다 탄 마른 풀) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alt")
	bool bAltHidesBody = false;

	// ── 덧붙은 메시 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accent")
	TSoftObjectPtr<UStaticMesh> AccentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accent")
	TSoftObjectPtr<UMaterialInterface> AccentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accent")
	FVector AccentScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accent")
	FVector AccentOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accent")
	EGrimPropAccentShow AccentShow = EGrimPropAccentShow::Always;

	// ── 이펙트 ──
	/** 늘 도는 이펙트(나비 떼). 환경 이펙트는 EFT_Grim_Ambient 를 쓴다(성능 규칙 24) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UNiagaraSystem> LoopFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FVector LoopFXOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	EGrimPropFXWhen LoopFXWhen = EGrimPropFXWhen::Always;

	/** 이 상태가 있으면 고리 이펙트를 끈다(바람에 흩어진 나비) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (Categories = "State"))
	FGameplayTag LoopFXHideState;

	/** 이 상태가 새로 생길 때 한 번 터지는 이펙트(솜이 흩날림) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UNiagaraSystem> BurstFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (Categories = "State"))
	FGameplayTag BurstState;

	// ── 불 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FVector FlameOffset = FVector(0.f, 0.f, 30.f);

	/** 불꽃 세기 1 일 때 원뿔 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FVector FlameScale = FVector(0.05f, 0.05f, 0.1f);

	/** 불꽃 세기 1 당 빛(cd). 0 = 빛 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0"))
	float LightCandela = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0"))
	float LightRadius = 500.f;

	/** 빛을 끄는 거리(cm) — 성능 규칙 23(등잔 · 모닥불은 30 m) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0"))
	float LightMaxDrawDistance = 3000.f;

	// ── 보상 ──
	/** 이 상태가 새로 생기면(서버) 보상을 굴려 픽업으로 놓는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (Categories = "State"))
	FGameplayTag RewardState;

	/** 「아이템:최소-최대:무게|…」 — 하나만 고른다. 예: Stone:1-2:5|Ash:1:3 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FString Rewards;

	/** 빈손 무게(셋 중 하나는 빈손이어도 된다 — 반복 규칙 6) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	float NothingWeight = 0.f;

	/** 보상을 놓을 자리(소품 기준, 바닥은 아래로 찾는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FVector RewardOffset = FVector(60.f, 0.f, 40.f);

	// ── 되돌리기 ──
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset")
	EGrimPropReset ResetRule = EGrimPropReset::Never;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset", meta = (ClampMin = "0"))
	float ResetSeconds = 0.f;

	// ── 움직임 ──
	/** 이 상태가 새로 생기면(서버) 바람 방향 쪽으로 옮긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (Categories = "State"))
	FGameplayTag DriftState;

	/** 옮길 거리(cm, 최소 · 최대) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	FVector2D DriftDistance = FVector2D(200.f, 500.f);

	/** 이 상태가 새로 생기면(클라이언트 연출) 몸이 튀어 올랐다 돌아온다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (Categories = "State"))
	FGameplayTag KickState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	float KickHeight = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.05"))
	float KickSeconds = 1.2f;

	// ── 줍기(수집품) ──
	/** F 로 주우면 도감에 이 줄이 쌓인다(Journal.csv 행). 사람마다 한 번 — 주운 사람에게만 사라진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collect")
	FName JournalId;
};

/**
 * 채집점 한 종 (HarvestNodes.csv 한 행 — 행 이름 = Kind, 예: D10_Branch). 채집 셀 관리자(AGrimHarvestCell)가 쓴다.
 * 나무 · 풀 채집(Q125 — 나중)도 이 표에 행을 더해 받는다: 메시 · 아이템 · HitsToHarvest(여러 번 쳐야 하는 것) · 재생.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimHarvestNodeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString Note;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TSoftObjectPtr<UMaterialInterface> MeshMaterial;

	/** 그리기 끝 거리(cm) — 줍는 것 60 m(분포 규칙 「성능 예산 안의 상한」) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look", meta = (ClampMin = "0"))
	float CullDistance = 6000.f;

	/** 얻는 아이템(Items.csv) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	FName Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield", meta = (ClampMin = "1"))
	int32 CountMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield", meta = (ClampMin = "1"))
	int32 CountMax = 1;

	/** 몇 번 F 해야 거두나(나무 베기 같은 것). 1 = 한 번에 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield", meta = (ClampMin = "1"))
	int32 HitsToHarvest = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	EGrimHarvestOwnership Ownership = EGrimHarvestOwnership::Shared;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	EGrimPropReset ResetRule = EGrimPropReset::Dawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "0"))
	float ResetSeconds = 0.f;

	/** F 가 닿는 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "50"))
	float InteractRadius = 200.f;
};

/** 도감 한 줄 (Journal.csv — 행 이름 = JournalId). 도감 탭이 생기기 전 임시 목록(콘솔 Grim.Journal). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimJournalRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 묶음(예: Page) — 쪽 수 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal")
	FName Group;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal")
	FString Text;
};

/** 보상 한 줄(Rewards 문자열을 푼 것). */
struct GRIMOIRE_API FGrimRewardEntry
{
	FName Item;
	int32 Min = 1;
	int32 Max = 1;
	float Weight = 1.f;
};

/** 표 셋을 읽어 둔 것(CSV — 설정 UGrimDressingSettings). 콘솔 Grim.Meadow.Reload 로 다시 읽는다. */
struct GRIMOIRE_API FGrimDressingData
{
	static const FGrimReactivePropRow* FindProp(FName PropId);
	static const FGrimHarvestNodeRow* FindHarvestNode(FName Kind);
	static const FGrimJournalRow* FindJournal(FName JournalId);
	static const UDataTable* GetPropTable();
	static const UDataTable* GetHarvestTable();
	static const UDataTable* GetJournalTable();
	static void Reset();

	/** 테스트: 표를 바꿔 끼운다(nullptr = 그대로). 끝나면 Reset */
	static void SetTablesForTest(UDataTable* Props, UDataTable* Harvest, UDataTable* Journal);

	/** CSV 파일을 RowStruct 표로 읽는다(테스트 · 다시 읽기). 문제는 OutProblems */
	static UDataTable* LoadCSV(const FString& ContentRelativePath, UScriptStruct* RowStruct, TArray<FString>& OutProblems);

	/** 「아이템:최소-최대:무게|…」 풀기. 틀린 조각은 OutProblems */
	static TArray<FGrimRewardEntry> ParseRewards(const FString& Text, TArray<FString>* OutProblems = nullptr);

	/** 하나 고르기(빈손 무게 포함). 빈손이면 Item = NAME_None. Roll = 0~1 난수 */
	static FGrimRewardEntry PickReward(const TArray<FGrimRewardEntry>& Entries, float NothingWeight, float Roll);
};
