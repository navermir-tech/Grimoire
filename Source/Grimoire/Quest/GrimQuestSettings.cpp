#include "Quest/GrimQuestSettings.h"
#include "Factions/GrimFactionTypes.h"
#include "Magic/GrimSpellDefinition.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GrimQuestSettingsLocal
{
	FGrimQuestDatabase GQuestDatabase;
	bool GQuestDatabaseLoaded = false;

	FAutoConsoleCommand GReloadCommand(
		TEXT("Grim.Quest.Reload"),
		TEXT("퀘스트 · NPC · 대사 표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimQuestSettings::ReloadData));
}

bool UGrimQuestSettings::LoadContentFile(const FString& RelativePath, FString& OutText)
{
	return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));
}

const FGrimQuestDatabase& UGrimQuestSettings::GetQuestDatabase()
{
	using namespace GrimQuestSettingsLocal;
	if (GQuestDatabaseLoaded)
	{
		return GQuestDatabase;
	}
	GQuestDatabaseLoaded = true;

	const UGrimQuestSettings* Settings = Get();
	FString Quests, Steps, Objectives;
	TArray<FString> Problems;
	if (!LoadContentFile(Settings->QuestsCSV, Quests) || !LoadContentFile(Settings->QuestStepsCSV, Steps) || !LoadContentFile(Settings->QuestObjectivesCSV, Objectives))
	{
		UE_LOG(LogGrimQuest, Error, TEXT("퀘스트 표를 열 수 없다: %s · %s · %s"), *Settings->QuestsCSV, *Settings->QuestStepsCSV, *Settings->QuestObjectivesCSV);
		GQuestDatabase = FGrimQuestDatabase();
		return GQuestDatabase;
	}
	FGrimQuestDatabase::BuildFromCSV(Quests, Steps, Objectives, GQuestDatabase, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimQuest, Error, TEXT("퀘스트 표: %s"), *Problem);
	}
	UE_LOG(LogGrimQuest, Log, TEXT("퀘스트 %d 개를 읽었다(문제 %d)"), GQuestDatabase.Quests.Num(), Problems.Num());
	return GQuestDatabase;
}

void UGrimQuestSettings::ReloadData()
{
	GrimQuestSettingsLocal::GQuestDatabaseLoaded = false;
	FGrimDialogueDatabase::ResetCache();
	GetQuestDatabase();
	FGrimDialogueDatabase::Get();
}

UGrimSpellDefinition* UGrimQuestSettings::LoadSpell(FName SpellId)
{
	if (SpellId.IsNone())
	{
		return nullptr;
	}
	return LoadObject<UGrimSpellDefinition>(nullptr, *GetSpellAssetPath(SpellId), nullptr, LOAD_NoWarn);
}

FString UGrimQuestSettings::GetSpellAssetPath(FName SpellId)
{
	const FString Name = Get()->SpellAssetPrefix + SpellId.ToString();
	return FString::Printf(TEXT("%s/%s.%s"), *Get()->SpellAssetDirectory, *Name, *Name);
}

bool UGrimQuestSettings::SpellMatchesId(const UGrimSpellDefinition* Spell, FName SpellId)
{
	if (!Spell || SpellId.IsNone())
	{
		return false;
	}
	const FString Name = Spell->GetName();
	const FString Id = SpellId.ToString();
	return Name.Equals(Get()->SpellAssetPrefix + Id, ESearchCase::IgnoreCase) || Name.Equals(Id, ESearchCase::IgnoreCase);
}
