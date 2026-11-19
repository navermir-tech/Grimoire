// 크레딧 탭(Q137) 테스트 — docs/systems/world-zones.md 「크레딧 확인 탭」, docs/qa/world-zones.md
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/GrimCredits.h"
#include "UI/GrimMenuWidgets.h"
#include "UI/GrimUISettings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimCreditsTestLocal
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreditsDataTest, "Grimoire.UI.Credits.Data", GrimCreditsTestLocal::Flags)
bool FGrimCreditsDataTest::RunTest(const FString& Parameters)
{
	// 게임이 읽는 정본 파일: 오류 없이 읽히고, 표기마다 필수 칸이 있다
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), UGrimUISettings::Get()->CreditsFile);
	FString Text, Error;
	if (!TestTrue(TEXT("Credits.json 을 연다"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	FGrimCredits Credits;
	TestTrue(FString::Printf(TEXT("오류 없이 읽는다(%s)"), *Error), FGrimCreditsData::Parse(Text, Credits, Error));
	TestTrue(TEXT("CC BY 표기가 하나 이상"), Credits.Attributions.Num() >= 1);
	TestFalse(TEXT("게임 이름"), Credits.Game.IsEmpty());
	TestTrue(TEXT("엔진 표기가 있다"), Credits.Notices.ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Epic Games")); }));

	// 문서(docs/art/credits.md)의 표 · 표기 문구가 같은 데이터에서 나왔다 — 줄 모양이 credits.mjs 와 같아야 한다
	FString Doc;
	if (TestTrue(TEXT("docs/art/credits.md 를 연다"), FFileHelper::LoadFileToString(Doc, *FPaths::Combine(FPaths::ProjectDir(), TEXT("docs/art/credits.md")))))
	{
		for (const FGrimCreditEntry& Entry : Credits.Attributions)
		{
			TestTrue(Entry.Asset + TEXT(" 링크가 문서에"), Doc.Contains(Entry.AssetUrl));
			TestTrue(Entry.Asset + TEXT(" 영어 표기 한 줄이 문서에(다르면 node Scripts/Docs/credits.mjs --write)"), Doc.Contains(FGrimCreditsData::MakeAttributionLine(Entry)));
			TestTrue(Entry.Asset + TEXT(" 한국어 표기 한 줄이 문서에"), Doc.Contains(FGrimCreditsData::MakeAttributionLineKo(Entry)));
		}
		for (const FString& Notice : Credits.Notices)
		{
			TestTrue(TEXT("엔진 표기가 문서에"), Doc.Contains(Notice));
		}
	}

	// 줄 모양
	FGrimCreditEntry Sample;
	Sample.Author = TEXT("Someone");
	Sample.Asset = TEXT("Thing");
	Sample.AssetUrl = TEXT("https://example.com/thing");
	Sample.License = TEXT("CC BY 4.0");
	Sample.LicenseUrl = TEXT("https://creativecommons.org/licenses/by/4.0/");
	Sample.ModifiedEn = TEXT("Colors were modified.");
	Sample.ModifiedKo = TEXT("색을 고쳤다.");
	TestEqual(TEXT("영어 한 줄"), FGrimCreditsData::MakeAttributionLine(Sample),
		FString(TEXT("Thing by Someone (https://example.com/thing), licensed under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Colors were modified.")));
	TestEqual(TEXT("한국어 한 줄"), FGrimCreditsData::MakeAttributionLineKo(Sample), FString(TEXT("Someone 의 Thing — CC BY 4.0. 색을 고쳤다.")));

	// 필수 칸이 빈 표기는 알린다
	FGrimCredits Bad;
	TestFalse(TEXT("고친 점이 빈 표기는 실패"), FGrimCreditsData::Parse(TEXT(R"({ "Attributions": [ { "Author": "A", "Asset": "B", "AssetUrl": "u", "License": "CC BY 4.0", "LicenseUrl": "l", "ModifiedKo": "", "ModifiedEn": "x" } ] })"), Bad, Error));
	TestTrue(TEXT("어느 칸인지 알린다"), Error.Contains(TEXT("ModifiedKo")));
	TestEqual(TEXT("읽은 만큼은 남는다"), Bad.Attributions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreditsPanelTest, "Grimoire.UI.Credits.Panel", GrimCreditsTestLocal::Flags)
bool FGrimCreditsPanelTest::RunTest(const FString& Parameters)
{
	// 설정 칸(메인 메뉴 · Esc 메뉴 공용)에 「크레딧 확인」 탭이 있고 크레딧 줄이 쌓인다
	UGrimMenuSettingsPanel* Panel = NewObject<UGrimMenuSettingsPanel>();
	Panel->Initialize();
	Panel->ShowTab(UGrimMenuSettingsPanel::TabCredits);
	TestEqual(TEXT("크레딧 탭으로"), Panel->GetTab(), static_cast<int32>(UGrimMenuSettingsPanel::TabCredits));
	const FGrimCredits& Credits = FGrimCreditsData::Get();
	// 제목 · 소개 2 + 표기마다 5 줄 + 엔진 표기
	const int32 Expected = 1 + (Credits.IntroKo.IsEmpty() ? 0 : 1) + (Credits.IntroEn.IsEmpty() ? 0 : 1) + Credits.Attributions.Num() * 5 + Credits.Notices.Num();
	TestEqual(TEXT("크레딧 줄 수"), Panel->GetCreditLineCount(), Expected);
	Panel->ShowTab(UGrimMenuSettingsPanel::TabOptions);
	TestEqual(TEXT("설정 탭으로 돌아감"), Panel->GetTab(), static_cast<int32>(UGrimMenuSettingsPanel::TabOptions));
	Panel->ShowTab(42);
	TestEqual(TEXT("모르는 탭 = 설정"), Panel->GetTab(), static_cast<int32>(UGrimMenuSettingsPanel::TabOptions));
	return true;
}

#endif
