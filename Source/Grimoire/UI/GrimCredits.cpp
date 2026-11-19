#include "UI/GrimCredits.h"

#include "UI/GrimUISettings.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimCredits, Log, All);

namespace GrimCreditsLocal
{
	FGrimCredits GCredits;
	bool GLoaded = false;
}

bool FGrimCreditsData::Parse(const FString& Text, FGrimCredits& Out, FString& OutError)
{
	Out = FGrimCredits();
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Out, 0, 0))
	{
		OutError = TEXT("JSON 을 읽지 못함");
		return false;
	}
	TArray<FString> Problems;
	for (int32 Index = 0; Index < Out.Attributions.Num(); ++Index)
	{
		const FGrimCreditEntry& Entry = Out.Attributions[Index];
		const TPair<const TCHAR*, const FString*> Required[] = {
			{ TEXT("Author"), &Entry.Author }, { TEXT("Asset"), &Entry.Asset }, { TEXT("AssetUrl"), &Entry.AssetUrl },
			{ TEXT("License"), &Entry.License }, { TEXT("LicenseUrl"), &Entry.LicenseUrl },
			{ TEXT("ModifiedKo"), &Entry.ModifiedKo }, { TEXT("ModifiedEn"), &Entry.ModifiedEn } };
		for (const TPair<const TCHAR*, const FString*>& Field : Required)
		{
			if (Field.Value->TrimStartAndEnd().IsEmpty())
			{
				Problems.Add(FString::Printf(TEXT("Attributions[%d].%s 가 비었다"), Index, Field.Key));
			}
		}
	}
	OutError = FString::Join(Problems, TEXT(" / "));
	return Problems.Num() == 0;
}

const FGrimCredits& FGrimCreditsData::Get()
{
	using namespace GrimCreditsLocal;
	if (!GLoaded)
	{
		GLoaded = true;
		const FString& RelativePath = UGrimUISettings::Get()->CreditsFile;
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectContentDir(), RelativePath)))
		{
			UE_LOG(LogGrimCredits, Error, TEXT("크레딧 %s 를 열지 못함 — 크레딧 탭이 빈다"), *RelativePath);
			GCredits = FGrimCredits();
			return GCredits;
		}
		FString Error;
		if (!Parse(Text, GCredits, Error))
		{
			UE_LOG(LogGrimCredits, Error, TEXT("크레딧 %s: %s"), *RelativePath, *Error);
		}
	}
	return GCredits;
}

void FGrimCreditsData::Reload()
{
	GrimCreditsLocal::GLoaded = false;
	Get();
}

FString FGrimCreditsData::MakeAttributionLine(const FGrimCreditEntry& Entry)
{
	return FString::Printf(TEXT("%s by %s (%s), licensed under %s (%s). %s"),
		*Entry.Asset, *Entry.Author, *Entry.AssetUrl, *Entry.License, *Entry.LicenseUrl, *Entry.ModifiedEn).TrimEnd();
}

FString FGrimCreditsData::MakeAttributionLineKo(const FGrimCreditEntry& Entry)
{
	return FString::Printf(TEXT("%s 의 %s — %s. %s"), *Entry.Author, *Entry.Asset, *Entry.License, *Entry.ModifiedKo).TrimEnd();
}
