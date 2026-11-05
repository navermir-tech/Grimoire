#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Quest/GrimQuestTypes.h"
#include "GrimQuestSettings.generated.h"

class UGrimSpellDefinition;

/**
 * 퀘스트 · 마법탑 NPC 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimQuestSettings].
 * 퀘스트 · 단계 · 목표 · NPC · 대사는 Content/Grimoire/Data 의 CSV(런타임에 읽는다 — 숙련도 등급표와 같은 방식).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Quest"))
class GRIMOIRE_API UGrimQuestSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimQuestSettings* Get() { return GetDefault<UGrimQuestSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 퀘스트 표 셋을 한 번 읽어 둔다. 문제가 있으면 로그(Error)에 남기고 읽은 만큼 쓴다. */
	static const FGrimQuestDatabase& GetQuestDatabase();

	/** 표를 다시 읽게 한다(콘솔 Grim.Quest.Reload). 대사 표도 같이. */
	static void ReloadData();

	/** Content 기준 경로의 파일을 읽는다(없으면 false). */
	static bool LoadContentFile(const FString& RelativePath, FString& OutText);

	/** 마법 ID("Fireball") → 정의 에셋(SpellAssetPathFormat 로). 없으면 nullptr. */
	static UGrimSpellDefinition* LoadSpell(FName SpellId);

	/** 정의가 마법 ID 의 것인가(에셋 이름 = SpellAssetPrefix + ID, 또는 ID 그대로) */
	static bool SpellMatchesId(const UGrimSpellDefinition* Spell, FName SpellId);

	// ───── 표 (Content 폴더 기준) ─────

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString QuestsCSV = TEXT("Grimoire/Data/Quests.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString QuestStepsCSV = TEXT("Grimoire/Data/QuestSteps.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString QuestObjectivesCSV = TEXT("Grimoire/Data/QuestObjectives.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString NpcsCSV = TEXT("Grimoire/Data/TowerNpcs.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString DialogueCSV = TEXT("Grimoire/Data/TowerDialogue.csv");

	// ───── 진행 ─────

	/** 서버가 조회형 목표를 다시 보는 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Progress", meta = (ClampMin = "0.05", Units = "s"))
	float PollIntervalSeconds = 0.25f;

	/** AutoStart 퀘스트를 시작하는 맵(짧은 이름). 비우면 모든 맵. */
	UPROPERTY(Config, EditAnywhere, Category = "Progress")
	TArray<FString> AutoStartMaps;

	/** ReachLocation 의 Value 가 0 일 때 반경 */
	UPROPERTY(Config, EditAnywhere, Category = "Progress", meta = (ClampMin = "0", Units = "cm"))
	float DefaultLocationRadius = 400.f;

	/** 마법 ID → 에셋 경로. {0} 이 ID */
	UPROPERTY(Config, EditAnywhere, Category = "Progress")
	FString SpellAssetPathFormat = TEXT("/Game/Grimoire/Magic/Spells/DA_Spell_{0}.DA_Spell_{0}");

	UPROPERTY(Config, EditAnywhere, Category = "Progress")
	FString SpellAssetPrefix = TEXT("DA_Spell_");

	/** 경험치 출처 태그 이름(레벨 갈래의 AwardExperience 에 넘긴다 — 통합 때) */
	UPROPERTY(Config, EditAnywhere, Category = "Progress")
	FName QuestXPSourceTag = TEXT("XP.Source.Quest");

	// ───── 마법탑 NPC ─────

	/** 대화 키 — 침대와 같이 「다가가 키」(마법 상호작용의 예외). 정식 상호작용 키는 조작 갈래 몫. */
	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FKey TalkKey = EKeys::F;

	UPROPERTY(Config, EditAnywhere, Category = "NPC", meta = (ClampMin = "0", Units = "cm"))
	float TalkRadius = 300.f;

	/** 서버가 대화 · 서비스 요청을 받을 때 거리 여유(클라이언트 위치 오차) */
	UPROPERTY(Config, EditAnywhere, Category = "NPC", meta = (ClampMin = "0", Units = "cm"))
	float ServerTalkSlack = 200.f;

	// ───── 화면 ─────

	/** 오른쪽 위 퀘스트 추적 창 */
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	bool bShowTracker = true;

	/** 퀘스트 알림 한 줄이 떠 있는 초 */
	UPROPERTY(Config, EditAnywhere, Category = "UI", meta = (ClampMin = "0", Units = "s"))
	float NoticeSeconds = 4.f;
};
