#include "Items/GrimItemSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GrimItemSettingsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FGrimItemDatabase GItemDatabase;
	bool GItemDatabaseLoaded = false;

	FAutoConsoleCommand GReloadItemsCommand(
		TEXT("Grim.Items.Reload"),
		TEXT("아이템 · 요리 표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimItemSettings::ReloadData));
}

bool UGrimItemSettings::LoadContentFile(const FString& RelativePath, FString& OutText)
{
	return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));
}

const FGrimItemDatabase& UGrimItemSettings::GetDatabase()
{
	using namespace GrimItemSettingsLocal;
	if (GItemDatabaseLoaded)
	{
		return GItemDatabase;
	}
	GItemDatabaseLoaded = true;

	const UGrimItemSettings* Settings = Get();
	FString Items, Recipes;
	if (!LoadContentFile(Settings->ItemsCSV, Items))
	{
		UE_LOG(LogGrimItems, Error, TEXT("아이템 표를 열 수 없다: %s"), *Settings->ItemsCSV);
		GItemDatabase = FGrimItemDatabase();
		return GItemDatabase;
	}
	if (!LoadContentFile(Settings->RecipesCSV, Recipes))
	{
		UE_LOG(LogGrimItems, Error, TEXT("요리 표를 열 수 없다: %s"), *Settings->RecipesCSV);
	}
	TArray<FString> Problems;
	FGrimItemDatabase::BuildFromCSV(Items, Recipes, GItemDatabase, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimItems, Error, TEXT("아이템 표: %s"), *Problem);
	}
	UE_LOG(LogGrimItems, Log, TEXT("아이템 표: 아이템 %d · 레시피 %d"), GItemDatabase.Items.Num(), GItemDatabase.Recipes.Num());
	return GItemDatabase;
}

FString UGrimItemSettings::GetGroupDisplayName(FName Group)
{
	const FString* Found = Get()->GroupDisplayNames.Find(Group);
	return Found ? *Found : Group.ToString();
}

void UGrimItemSettings::ReloadData()
{
	GrimItemSettingsLocal::GItemDatabaseLoaded = false;
	GetDatabase();
}
