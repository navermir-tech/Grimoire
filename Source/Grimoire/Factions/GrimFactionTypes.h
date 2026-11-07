#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GrimFactionTypes.generated.h"

struct FGrimQuestDatabase;
struct FGrimQuestLog;

/** 마법탑 NPC 서비스 ID (TowerNpcs.csv 의 Services 에 쓴다) */
namespace GrimTowerServices
{
	GRIMOIRE_API extern const FName Trial;
	GRIMOIRE_API extern const FName Purify;
	GRIMOIRE_API extern const FName Repair;
	GRIMOIRE_API extern const FName Erase;

	GRIMOIRE_API bool IsKnown(FName Service);

	/** "Purify:<트리 ID>" 면 트리 ID 를 꺼낸다(정화 의식은 트리마다 메뉴 하나 — 한 번에 트리 하나, Q31) */
	GRIMOIRE_API bool ParsePurify(FName Service, FName& OutTreeId);

	/** "Buy:<아이템 ID>" 면 아이템 ID 를 꺼낸다(상점 — 값은 UGrimShopSettings, 화폐 솔리두스 Q38) */
	GRIMOIRE_API bool ParseBuy(FName Service, FName& OutItemId);
}

/** 대사 맥락 (TowerDialogue.csv 의 Context). 서비스 결과는 서버가 고른 맥락을 클라이언트가 이 표로 보여 준다. */
namespace GrimDialogueContexts
{
	/** 말을 걸었을 때 */
	GRIMOIRE_API extern const FName Talk;
	/** 서비스 메뉴 위의 한 줄 */
	GRIMOIRE_API extern const FName Menu;
	GRIMOIRE_API extern const FName TrialStarted;
	/** 승급 조건(마법 수 · 등급 합)이 모자람 */
	GRIMOIRE_API extern const FName TrialNotReady;
	/** 시작 조건(퀘스트 진행)이 모자람 */
	GRIMOIRE_API extern const FName TrialLocked;
	/** 이 서클의 시험이 없음(최고 서클 · 표에 없음) */
	GRIMOIRE_API extern const FName TrialNone;
	/** 이미 시험 중 */
	GRIMOIRE_API extern const FName TrialBusy;
	GRIMOIRE_API extern const FName Purify;
	/** 정화할 것(찍은 노드)이 없다 */
	GRIMOIRE_API extern const FName PurifyEmpty;
	/** 대리 수리(솔, Q121): 고쳤다 · 솔이 모자람 · 고칠 것이 없다 */
	GRIMOIRE_API extern const FName Repair;
	GRIMOIRE_API extern const FName RepairNoMoney;
	GRIMOIRE_API extern const FName RepairNothing;
	GRIMOIRE_API extern const FName Erase;
	/** 너무 멀다 · 잘못된 요청 */
	GRIMOIRE_API extern const FName Refused;
	/** 샀다 · 솔이 모자람 · 가방이 가득(Buy:<아이템>) */
	GRIMOIRE_API extern const FName Buy;
	GRIMOIRE_API extern const FName BuyNoMoney;
	GRIMOIRE_API extern const FName BuyBagFull;

	/** 표가 꼭 가져야 하는 맥락(서비스가 있는 NPC) */
	GRIMOIRE_API TArray<FName> RequiredForServices();

	/** 상점 서비스(Buy:<아이템>)가 있는 NPC 가 더 가져야 하는 맥락 */
	GRIMOIRE_API TArray<FName> RequiredForShop();
}

/** TowerNpcs.csv 의 한 행. 행 이름 = NPC ID(맵의 AGrimTowerNPC::NpcId). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimNpcRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString DisplayName;

	/** 이름 아래 작은 글씨(직함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString Title;

	/** 서비스 ID 를 | 로(Trial|Purify:<트리>|Repair|Erase). 비우면 대화만. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString Services;

	/** 메뉴 글씨를 Services 와 같은 순서로 | 로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString ServiceLabels;
};

/** TowerDialogue.csv 의 한 행. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	/** NPC ID. None = 모든 NPC */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Npc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Context;

	/** 조건 퀘스트. None = 언제나(기본 대사) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Quest;

	/** 0 이상 = 그 퀘스트가 그 단계일 때, -1 = 진행 중이면, -2 = 끝냈으면 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int32 Step = -1;

	/** 같은 조건끼리 고를 때 큰 쪽 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int32 Priority = 0;

	/** 대사 줄을 | 로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Lines;
};

struct GRIMOIRE_API FGrimNpcDef
{
	FName Id;
	FString DisplayName;
	FString Title;
	TArray<FName> Services;
	TArray<FString> ServiceLabels;
};

struct GRIMOIRE_API FGrimDialogueEntry
{
	FName RowName;
	FName Npc;
	FName Context;
	FName Quest;
	int32 Step = -1;
	int32 Priority = 0;
	TArray<FString> Lines;
};

/** NPC · 대사 표. 고르기는 순수 계산(Grimoire.Quest.Dialogue). */
struct GRIMOIRE_API FGrimDialogueDatabase
{
	TArray<FGrimNpcDef> Npcs;
	TArray<FGrimDialogueEntry> Entries;

	const FGrimNpcDef* FindNpc(FName NpcId) const;

	/** NPC 의 맥락 대사 중 지금 퀘스트 진행에 가장 잘 맞는 것. 없으면 nullptr. */
	const FGrimDialogueEntry* Select(FName NpcId, FName Context, const FGrimQuestLog& Log) const;

	/** 표 둘을 읽는다. Quests 를 주면 대사의 퀘스트 ID 도 확인한다. 문제가 있으면 false. */
	static bool Build(const UDataTable* NpcTable, const UDataTable* DialogueTable, const FGrimQuestDatabase* Quests, FGrimDialogueDatabase& Out, TArray<FString>& OutProblems);
	static bool BuildFromCSV(const FString& NpcsCSV, const FString& DialogueCSV, const FGrimQuestDatabase* Quests, FGrimDialogueDatabase& Out, TArray<FString>& OutProblems);

	/** 설정의 CSV 를 한 번 읽어 둔다. */
	static const FGrimDialogueDatabase& Get();
	static void ResetCache();
};
