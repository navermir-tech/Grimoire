#include "Appearance/GrimAppearanceSettings.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GrimAppearanceSettingsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FGrimAppearanceCatalog GCatalog;
	bool GCatalogLoaded = false;

	bool LoadContentFile(const FString& RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));
	}
}

UGrimAppearanceSettings::UGrimAppearanceSettings()
{
	BodyMeshes = {
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")))
	};
	BodyMaterialChannels = { EGrimAppearanceChannel::Skin, EGrimAppearanceChannel::Under };
	NpcIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")));
}

const FGrimAppearanceCatalog& UGrimAppearanceSettings::GetCatalog()
{
	using namespace GrimAppearanceSettingsLocal;
	if (GCatalogLoaded)
	{
		return GCatalog;
	}
	GCatalogLoaded = true;

	const UGrimAppearanceSettings* Settings = Get();
	TArray<FString> Problems;
	const UGrimAppearanceCatalogAsset* Asset = Settings->CatalogAsset.IsNull() ? nullptr : Settings->CatalogAsset.LoadSynchronous();
	if (Asset)
	{
		FGrimAppearanceCatalog::BuildFromTables(Asset->Parts, Asset->Pieces, Asset->Palettes, Asset->Morphs, Asset->Rules, Asset->Npcs, GCatalog, Problems);
	}
	else
	{
		FString Parts, Pieces, Palettes, Morphs, Rules, Npcs;
		const TPair<const FString*, FString*> Files[] = {
			{ &Settings->PartsCSV, &Parts }, { &Settings->PiecesCSV, &Pieces }, { &Settings->PalettesCSV, &Palettes },
			{ &Settings->MorphsCSV, &Morphs }, { &Settings->RulesCSV, &Rules }, { &Settings->NpcsCSV, &Npcs } };
		for (const TPair<const FString*, FString*>& File : Files)
		{
			if (!LoadContentFile(*File.Key, *File.Value))
			{
				Problems.Add(FString::Printf(TEXT("표를 열 수 없다: %s"), **File.Key));
			}
		}
		FGrimAppearanceCatalog::BuildFromCSV(Parts, Pieces, Palettes, Morphs, Rules, Npcs, GCatalog, Problems);
	}
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimAppearance, Error, TEXT("외형 표: %s"), *Problem);
	}
	UE_LOG(LogGrimAppearance, Log, TEXT("외형 표(%s): 조각 %d · 소품 %d · 슬라이더 %d · 규칙 %d · 이름 있는 NPC %d"),
		Asset ? TEXT("DataAsset") : TEXT("CSV"), GCatalog.Parts.Num(), GCatalog.Pieces.Num(), GCatalog.Morphs.Num(), GCatalog.Rules.Num(), GCatalog.Npcs.Num());
	return GCatalog;
}

void UGrimAppearanceSettings::ReloadCatalog()
{
	GrimAppearanceSettingsLocal::GCatalogLoaded = false;
	GetCatalog();
}
