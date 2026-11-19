// 구역 이름 표시(Q130) 테스트 — docs/systems/world-zones.md, docs/qa/world-zones.md
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UI/GrimZoneBannerWidget.h"
#include "World/GrimZoneDiscoveryComponent.h"
#include "World/GrimZoneSettings.h"
#include "World/GrimZoneTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimZoneTestLocal
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr const TCHAR* LayoutPath = TEXT("Scripts/Terrain/openbeta_layout.json");

	/**
	 * 시험 표: 100 × 100 m 큰 구역 둘(서 · 동) — 서쪽에 소구역 둘(가운데 (20, 50) · (80, 50)이 아니라 (20, 50) · (40, 80)),
	 * 동쪽에 반지름 10 m 소구역 하나. 원점 (50, 0).
	 */
	const TCHAR* SmallJson = TEXT(R"({
		"WorldOrigin": { "X": 50, "Y": 0 },
		"Sections": [
			{ "Name": "West", "Tier": "Region", "Label": "서쪽", "Note": "개발 설명", "Subtitle": "서쪽 들", "Center": { "X": 50, "Y": 50 },
			  "Polygon": [ { "X": 0, "Y": 0 }, { "X": 100, "Y": 0 }, { "X": 100, "Y": 100 }, { "X": 0, "Y": 100 } ] },
			{ "Name": "East", "Tier": "Region", "Label": "동쪽", "Note": "동쪽 설명", "Center": { "X": 150, "Y": 50 },
			  "Polygon": [ { "X": 100, "Y": 0 }, { "X": 200, "Y": 0 }, { "X": 200, "Y": 100 }, { "X": 100, "Y": 100 } ] },
			{ "Name": "WestA", "Tier": "Sector", "Label": "서 A", "Center": { "X": 20, "Y": 50 } },
			{ "Name": "WestB", "Tier": "Sector", "Label": "서 B", "Center": { "X": 40, "Y": 80 } },
			{ "Name": "EastSpot", "Tier": "Sector", "Label": "동 점", "Center": { "X": 150, "Y": 50 }, "Radius": 10 }
		]
	})");

	bool LoadSmall(FAutomationTestBase& Test, FGrimZoneDatabase& Out)
	{
		FString Error;
		const bool bOk = FGrimZoneMath::ParseJson(SmallJson, Out, Error);
		Test.TestTrue(FString::Printf(TEXT("시험 표를 읽는다(%s)"), *Error), bOk);
		return bOk;
	}

	bool LoadFile(FAutomationTestBase& Test, const FString& FullPath, FGrimZoneDatabase& Out)
	{
		FString Text, Error;
		if (!Test.TestTrue(FString::Printf(TEXT("%s 를 연다"), *FullPath), FFileHelper::LoadFileToString(Text, *FullPath)))
		{
			return false;
		}
		const bool bOk = FGrimZoneMath::ParseJson(Text, Out, Error);
		Test.TestTrue(FString::Printf(TEXT("%s 를 오류 없이 읽는다(%s)"), *FPaths::GetCleanFilename(FullPath), *Error), bOk);
		return bOk;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneParseTest, "Grimoire.World.Zones.Parse", GrimZoneTestLocal::Flags)
bool FGrimZoneParseTest::RunTest(const FString& Parameters)
{
	FGrimZoneDatabase Db;
	if (!GrimZoneTestLocal::LoadSmall(*this, Db))
	{
		return false;
	}
	TestEqual(TEXT("구역 5"), Db.Zones.Num(), 5);
	TestEqual(TEXT("원점"), Db.WorldOrigin, FVector2D(50.0, 0.0));
	const FGrimZoneDef* West = Db.Find(TEXT("West"));
	const FGrimZoneDef* East = Db.Find(TEXT("East"));
	const FGrimZoneDef* WestB = Db.Find(TEXT("WestB"));
	const FGrimZoneDef* EastSpot = Db.Find(TEXT("EastSpot"));
	if (!TestNotNull(TEXT("West"), West) || !TestNotNull(TEXT("East"), East) || !TestNotNull(TEXT("WestB"), WestB) || !TestNotNull(TEXT("EastSpot"), EastSpot))
	{
		return false;
	}
	TestEqual(TEXT("이름"), West->Label.ToString(), FString(TEXT("서쪽")));
	TestEqual(TEXT("Subtitle 이 있으면 그것"), West->GetDisplaySubtitle().ToString(), FString(TEXT("서쪽 들")));
	TestEqual(TEXT("Subtitle 이 없으면 Note"), East->GetDisplaySubtitle().ToString(), FString(TEXT("동쪽 설명")));
	TestEqual(TEXT("큰 구역 경계 꼭짓점"), West->Polygon.Num(), 4);
	TestTrue(TEXT("큰 구역은 Parent 가 없다"), West->Parent.IsNone());
	TestEqual(TEXT("소구역의 큰 구역은 Center 로"), WestB->Parent, FName(TEXT("West")));
	TestEqual(TEXT("반지름"), EastSpot->Radius, 10.f);

	// 잘못된 표: 이름 겹침 · Tier 모름 · 경계 없는 큰 구역 · 어디에도 들지 않는 소구역
	const TCHAR* Bad = TEXT(R"({ "WorldOrigin": { "X": 0, "Y": 0 }, "Sections": [
		{ "Name": "A", "Tier": "Region", "Center": { "X": 0, "Y": 0 }, "Polygon": [ { "X": 0, "Y": 0 }, { "X": 10, "Y": 0 }, { "X": 10, "Y": 10 } ] },
		{ "Name": "A", "Tier": "Region", "Center": { "X": 0, "Y": 0 }, "Polygon": [] },
		{ "Name": "B", "Tier": "Town", "Center": { "X": 0, "Y": 0 } },
		{ "Name": "C", "Tier": "Region", "Center": { "X": 0, "Y": 0 } },
		{ "Name": "D", "Tier": "Sector", "Center": { "X": 500, "Y": 500 } } ] })");
	FGrimZoneDatabase BadDb;
	FString Error;
	TestFalse(TEXT("잘못된 표는 실패"), FGrimZoneMath::ParseJson(Bad, BadDb, Error));
	TestTrue(TEXT("이름 겹침을 알린다"), Error.Contains(TEXT("겹친다")));
	TestTrue(TEXT("Tier 를 알린다"), Error.Contains(TEXT("Town")));
	TestTrue(TEXT("경계 없는 큰 구역을 알린다"), Error.Contains(TEXT("C:")));
	TestTrue(TEXT("떠도는 소구역을 알린다"), Error.Contains(TEXT("D:")));
	TestTrue(TEXT("읽은 만큼은 남는다(A · C · D)"), BadDb.Find(TEXT("A")) && BadDb.Find(TEXT("D")));
	TestFalse(TEXT("JSON 이 아니면 실패"), FGrimZoneMath::ParseJson(TEXT("{ not json"), BadDb, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneFindTest, "Grimoire.World.Zones.FindAt", GrimZoneTestLocal::Flags)
bool FGrimZoneFindTest::RunTest(const FString& Parameters)
{
	FGrimZoneDatabase Db;
	if (!GrimZoneTestLocal::LoadSmall(*this, Db))
	{
		return false;
	}
	auto At = [&Db](double U, double V) { return FGrimZoneMath::FindAt(Db, FVector2D(U, V)); };

	TestEqual(TEXT("서쪽 · A 가까이"), At(10, 40).Region, FName(TEXT("West")));
	TestEqual(TEXT("A 가까이 = A"), At(10, 40).Sector, FName(TEXT("WestA")));
	TestEqual(TEXT("B 가까이 = B"), At(45, 90).Sector, FName(TEXT("WestB")));
	TestEqual(TEXT("서쪽 큰 구역 끝(90, 10)도 가장 가까운 A"), At(90, 10).Sector, FName(TEXT("WestA")));
	TestEqual(TEXT("동쪽"), At(150, 55).Region, FName(TEXT("East")));
	TestEqual(TEXT("반지름 안 = 동 점"), At(155, 55).Sector, FName(TEXT("EastSpot")));
	TestTrue(TEXT("반지름 밖 = 소구역 없음"), At(180, 90).Sector.IsNone());
	TestEqual(TEXT("반지름 밖이어도 큰 구역은 있다"), At(180, 90).Region, FName(TEXT("East")));
	TestTrue(TEXT("표 밖 = 없음"), At(-5, 50).Region.IsNone() && At(-5, 50).Sector.IsNone());
	TestTrue(TEXT("표 밖 북쪽"), At(50, 150).Region.IsNone());

	// 오목한 다각형(ㄷ 모양): 홈 안은 밖이다
	const TArray<FVector2D> U = { { 0, 0 }, { 30, 0 }, { 30, 30 }, { 20, 30 }, { 20, 10 }, { 10, 10 }, { 10, 30 }, { 0, 30 } };
	TestTrue(TEXT("ㄷ 왼쪽 기둥 안"), FGrimZoneMath::IsPointInPolygon({ 5, 20 }, U));
	TestFalse(TEXT("ㄷ 홈 안은 밖"), FGrimZoneMath::IsPointInPolygon({ 15, 20 }, U));
	TestTrue(TEXT("ㄷ 바닥 안"), FGrimZoneMath::IsPointInPolygon({ 15, 5 }, U));
	TestFalse(TEXT("꼭짓점 2 개는 늘 밖"), FGrimZoneMath::IsPointInPolygon({ 1, 1 }, { { 0, 0 }, { 10, 10 } }));

	// 월드 ↔ 지도: 월드 X = 북쪽(V), Y = 동쪽(U), cm. 원점 (50, 0)
	const FVector World = FGrimZoneMath::MapToWorld(Db, FVector2D(60.0, 25.0), 123.0);
	TestEqual(TEXT("지도 → 월드 X(북)"), World.X, 2500.0);
	TestEqual(TEXT("지도 → 월드 Y(동)"), World.Y, 1000.0);
	TestEqual(TEXT("지도 → 월드 Z 그대로"), World.Z, 123.0);
	TestEqual(TEXT("월드 → 지도 되돌림"), FGrimZoneMath::WorldToMap(Db, World), FVector2D(60.0, 25.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneVisitTest, "Grimoire.World.Zones.Visit", GrimZoneTestLocal::Flags)
bool FGrimZoneVisitTest::RunTest(const FString& Parameters)
{
	const FName West(TEXT("West")), WestA(TEXT("WestA")), WestB(TEXT("WestB")), East(TEXT("East"));
	auto Hit = [](FName Region, FName Sector) { FGrimZoneHit H; H.Region = Region; H.Sector = Sector; return H; };
	constexpr double Stable = 1.0, Cooldown = 60.0;
	FGrimZoneVisitTracker Tracker;
	TArray<FGrimZoneEntered> Out;

	// 첫 진입: 1 초 머문 뒤에야, 큰 구역 → 소구역 순서로 「처음」
	TestFalse(TEXT("막 들어선 순간은 아직"), Tracker.Step(Hit(West, WestA), 0.0, Stable, Cooldown, Out));
	TestEqual(TEXT("머물기 전엔 알림 없음"), Out.Num(), 0);
	Tracker.Step(Hit(West, WestA), 0.5, Stable, Cooldown, Out);
	TestEqual(TEXT("0.5 초도 아직"), Out.Num(), 0);
	TestTrue(TEXT("1 초 머물면 본 구역이 바뀐다"), Tracker.Step(Hit(West, WestA), 1.0, Stable, Cooldown, Out));
	if (TestEqual(TEXT("알림 둘"), Out.Num(), 2))
	{
		TestEqual(TEXT("먼저 큰 구역"), Out[0].Zone, West);
		TestTrue(TEXT("큰 구역 처음"), Out[0].bFirstVisit);
		TestEqual(TEXT("다음 소구역"), Out[1].Zone, WestA);
		TestTrue(TEXT("소구역 처음"), Out[1].bFirstVisit);
	}
	TestEqual(TEXT("본 구역 둘"), Tracker.Discovered.Num(), 2);

	// 같은 자리에 머물면 더 알리지 않는다
	Out.Reset();
	Tracker.Step(Hit(West, WestA), 5.0, Stable, Cooldown, Out);
	TestEqual(TEXT("머무는 동안 조용"), Out.Num(), 0);

	// 경계에서 깜빡임: B 로 0.4 초 갔다가 돌아오면 아무 일 없다
	Tracker.Step(Hit(West, WestB), 6.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestB), 6.4, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestA), 6.5, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestA), 8.0, Stable, Cooldown, Out);
	TestEqual(TEXT("깜빡임은 알리지 않는다"), Out.Num(), 0);

	// 같은 큰 구역 안 다른 소구역 = 소구역만 「처음」
	Tracker.Step(Hit(West, WestB), 10.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestB), 11.0, Stable, Cooldown, Out);
	if (TestEqual(TEXT("소구역만 알림"), Out.Num(), 1))
	{
		TestEqual(TEXT("B"), Out[0].Zone, WestB);
		TestTrue(TEXT("B 처음"), Out[0].bFirstVisit);
	}

	// 동쪽(소구역 없음) 처음 → 서쪽으로 돌아오면 「다시」(쿨다운이 지났으면)
	Out.Reset();
	Tracker.Step(Hit(East, NAME_None), 20.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(East, NAME_None), 21.0, Stable, Cooldown, Out);
	TestTrue(TEXT("동쪽 처음"), Out.Num() == 1 && Out[0].Zone == East && Out[0].bFirstVisit);
	Out.Reset();
	Tracker.Step(Hit(West, WestA), 30.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestA), 31.0, Stable, Cooldown, Out);
	TestEqual(TEXT("쿨다운 안의 다시 들어섬은 알리지 않는다(서쪽은 1 초, A 는 1 초에 알림)"), Out.Num(), 0);
	Tracker.Step(Hit(East, NAME_None), 40.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(East, NAME_None), 41.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestA), 100.0, Stable, Cooldown, Out);
	Tracker.Step(Hit(West, WestA), 101.0, Stable, Cooldown, Out);
	const FGrimZoneEntered* WestAgain = Out.FindByPredicate([West](const FGrimZoneEntered& E) { return E.Zone == West; });
	TestTrue(TEXT("쿨다운이 지나면 서쪽 「다시」"), WestAgain && !WestAgain->bFirstVisit);
	TestFalse(TEXT("동쪽 다시는 쿨다운 안(21 → 41 초)"), Out.ContainsByPredicate([East](const FGrimZoneEntered& E) { return E.Zone == East; }));

	// 구역 밖으로 나가는 것은 알리지 않는다
	Out.Reset();
	Tracker.Step(FGrimZoneHit(), 200.0, Stable, Cooldown, Out);
	Tracker.Step(FGrimZoneHit(), 201.0, Stable, Cooldown, Out);
	TestEqual(TEXT("밖으로 = 조용"), Out.Num(), 0);
	TestTrue(TEXT("지금 자리 = 없음"), Tracker.Current.Region.IsNone());

	// 머물기 0 초 = 바로
	FGrimZoneVisitTracker Instant;
	Out.Reset();
	Instant.Step(Hit(East, NAME_None), 0.0, 0.0, Cooldown, Out);
	TestEqual(TEXT("StableSeconds 0 이면 바로 알린다"), Out.Num(), 1);

	// 세이브에서 읽은 목록 합치기: 지금 것을 지우지 않고, 이미 본 구역은 「다시」
	FGrimZoneVisitTracker Loaded;
	Loaded.Discovered.Add(East);
	TestTrue(TEXT("합치면 바뀐다"), Loaded.MergeDiscovered({ West, East, NAME_None }));
	TestEqual(TEXT("합친 목록(None 은 뺀다)"), Loaded.Discovered.Num(), 2);
	TestFalse(TEXT("같은 것을 또 합치면 그대로"), Loaded.MergeDiscovered({ West }));
	Out.Reset();
	Loaded.Step(Hit(West, NAME_None), 0.0, 0.0, Cooldown, Out);
	TestTrue(TEXT("세이브에 있던 구역은 「다시」"), Out.Num() == 1 && !Out[0].bFirstVisit);

	Loaded.Reset();
	TestEqual(TEXT("Reset = 모두 잊음"), Loaded.Discovered.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneSaveTest, "Grimoire.World.Zones.Save", GrimZoneTestLocal::Flags)
bool FGrimZoneSaveTest::RunTest(const FString& Parameters)
{
	// 세이브 형식 v1: TArray<FString>. 읽으면 지금 목록과 합친다.
	TArray<uint8> Bytes;
	{
		TArray<FString> Names = { TEXT("Sec_TowerCape"), TEXT("Sub_Yard") };
		FMemoryWriter Writer(Bytes);
		Writer << Names;
	}
	UGrimZoneDiscoveryComponent* First = NewObject<UGrimZoneDiscoveryComponent>();
	int32 Changes = 0;
	First->OnDiscoveredChanged.AddLambda([&Changes](UGrimZoneDiscoveryComponent*) { ++Changes; });
	{
		FMemoryReader Reader(Bytes);
		First->ReadSaveData(Reader, First->GetSaveVersion());
	}
	TestEqual(TEXT("읽은 본 구역 둘"), First->GetDiscovered().Num(), 2);
	TestTrue(TEXT("마탑 곶을 봤다"), First->IsDiscovered(TEXT("Sec_TowerCape")));
	TestEqual(TEXT("바뀜 알림 한 번"), Changes, 1);
	TestEqual(TEXT("세이브 id"), First->GetSaveId(), FName(TEXT("World.Zones")));
	TestEqual(TEXT("플레이어 몫"), First->GetSaveScope(), EGrimSaveScope::Player);

	// 다시 쓰고 → 다른 컴포넌트가 읽으면 같다
	TArray<uint8> Again;
	{
		FMemoryWriter Writer(Again);
		First->WriteSaveData(Writer);
	}
	UGrimZoneDiscoveryComponent* Second = NewObject<UGrimZoneDiscoveryComponent>();
	{
		FMemoryReader Reader(Again);
		Second->ReadSaveData(Reader, 1);
	}
	TestEqual(TEXT("왕복한 목록"), Second->GetDiscovered(), First->GetDiscovered());

	// 같은 것을 또 읽어도 겹치지 않는다
	{
		FMemoryReader Reader(Again);
		Second->ReadSaveData(Reader, 1);
	}
	TestEqual(TEXT("두 번 읽어도 둘"), Second->GetDiscovered().Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneDataTest, "Grimoire.World.Zones.DataMatchesLayout", GrimZoneTestLocal::Flags)
bool FGrimZoneDataTest::RunTest(const FString& Parameters)
{
	using namespace GrimZoneTestLocal;
	// 게임이 읽는 생성물(설정 ZonesFile)과 정본 레이아웃이 같아야 한다 — 다르면 python Scripts/Terrain/export_zones.py
	FGrimZoneDatabase Data, Layout;
	const FString DataPath = FPaths::Combine(FPaths::ProjectContentDir(), UGrimZoneSettings::Get()->ZonesFile);
	if (!LoadFile(*this, DataPath, Data) || !LoadFile(*this, FPaths::Combine(FPaths::ProjectDir(), LayoutPath), Layout))
	{
		return false;
	}
	TestEqual(TEXT("원점이 같다"), Data.WorldOrigin, Layout.WorldOrigin);
	if (TestEqual(TEXT("구역 수가 같다(다르면 export_zones.py)"), Data.Zones.Num(), Layout.Zones.Num()))
	{
		for (int32 i = 0; i < Data.Zones.Num(); ++i)
		{
			const FGrimZoneDef& A = Data.Zones[i];
			const FGrimZoneDef& B = Layout.Zones[i];
			const FString Name = B.Name.ToString();
			TestEqual(Name + TEXT(" 이름"), A.Name, B.Name);
			TestEqual(Name + TEXT(" 층"), A.Tier, B.Tier);
			TestEqual(Name + TEXT(" 표시 이름"), A.Label.ToString(), B.Label.ToString());
			TestEqual(Name + TEXT(" 한 줄"), A.GetDisplaySubtitle().ToString(), B.GetDisplaySubtitle().ToString());
			TestEqual(Name + TEXT(" 가운데"), A.Center, B.Center);
			TestEqual(Name + TEXT(" 반지름"), A.Radius, B.Radius);
			TestEqual(Name + TEXT(" 경계"), A.Polygon, B.Polygon);
		}
	}

	// 지역 1 의 땅(2032 × 4064 m)을 큰 구역이 빈틈 · 겹침 없이 덮는다(50 m 격자, 경계선 위를 피해 12.5 m 어긋나게)
	int32 Gaps = 0, Overlaps = 0;
	FString FirstGap, FirstOverlap;
	for (double V = 12.5; V < 4064.0; V += 50.0)
	{
		for (double U = 12.5; U < 2032.0; U += 50.0)
		{
			int32 Count = 0;
			for (const FGrimZoneDef& Zone : Data.Zones)
			{
				Count += Zone.Tier == EGrimZoneTier::Region && FGrimZoneMath::IsPointInPolygon({ U, V }, Zone.Polygon) ? 1 : 0;
			}
			if (Count == 0 && Gaps++ == 0)
			{
				FirstGap = FString::Printf(TEXT("(%.1f, %.1f)"), U, V);
			}
			if (Count > 1 && Overlaps++ == 0)
			{
				FirstOverlap = FString::Printf(TEXT("(%.1f, %.1f)"), U, V);
			}
		}
	}
	TestEqual(FString::Printf(TEXT("빈틈 없음(첫 곳 %s)"), *FirstGap), Gaps, 0);
	TestEqual(FString::Printf(TEXT("겹침 없음(첫 곳 %s)"), *FirstOverlap), Overlaps, 0);

	// 모든 구역은 자기 가운데에서 자기 이름이 나온다(소구역은 큰 구역 안, 큰 구역은 경계 안)
	for (const FGrimZoneDef& Zone : Data.Zones)
	{
		const FGrimZoneHit Hit = FGrimZoneMath::FindAt(Data, Zone.Center);
		const FName Got = Zone.Tier == EGrimZoneTier::Region ? Hit.Region : Hit.Sector;
		TestEqual(FString::Printf(TEXT("%s 가운데"), *Zone.Name.ToString()), Got, Zone.Name);
		TestFalse(FString::Printf(TEXT("%s 이름이 있다"), *Zone.Name.ToString()), Zone.Label.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s 한 줄이 있다"), *Zone.Name.ToString()), Zone.GetDisplaySubtitle().IsEmpty());
	}

	// 랜드마크 자리(레이아웃 Landmarks)가 이름과 맞는 구역에
	const struct { double U, V; const TCHAR* Region; } Landmarks[] = {
		{ 1000, 380, TEXT("Sec_TowerCape") },     // 마법탑
		{ 1100, 1980, TEXT("Sec_CentralHills") }, // 거대 고목
		{ 1000, 3890, TEXT("Sec_NorthPass") },    // 바위 문
		{ 560, 3700, TEXT("Sec_Highland") },      // 선돌 고리
		{ 1340, 2880, TEXT("Sec_Valley") },       // 폭포
		{ 1620, 2120, TEXT("Sec_Riverside") },    // 폐허 감시탑
		{ 1400, 870, TEXT("Sec_TowerCape") },     // 풍차(마탑 구간 안 — 소구역 풍차 언덕)
	};
	for (const auto& Landmark : Landmarks)
	{
		TestEqual(FString::Printf(TEXT("(%.0f, %.0f) 는 %s"), Landmark.U, Landmark.V, Landmark.Region),
			FGrimZoneMath::FindAt(Data, { Landmark.U, Landmark.V }).Region, FName(Landmark.Region));
	}
	TestEqual(TEXT("풍차 = 풍차 언덕 소구역"), FGrimZoneMath::FindAt(Data, { 1400, 870 }).Sector, FName(TEXT("Sub_Windmill")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoneBannerTest, "Grimoire.World.Zones.Banner", GrimZoneTestLocal::Flags)
bool FGrimZoneBannerTest::RunTest(const FString& Parameters)
{
	FGrimZoneBannerStyle Style;
	Style.FadeInSeconds = 1.f;
	Style.HoldSeconds = 2.f;
	Style.FadeOutSeconds = 2.f;
	TestEqual(TEXT("시작 = 0"), UGrimZoneBannerWidget::ComputeAlpha(Style, 0.f), 0.f);
	TestEqual(TEXT("나타나는 중"), UGrimZoneBannerWidget::ComputeAlpha(Style, 0.5f), 0.5f);
	TestEqual(TEXT("머묾 = 1"), UGrimZoneBannerWidget::ComputeAlpha(Style, 2.f), 1.f);
	TestEqual(TEXT("사라지는 중"), UGrimZoneBannerWidget::ComputeAlpha(Style, 4.f), 0.5f);
	TestEqual(TEXT("끝 = 0"), UGrimZoneBannerWidget::ComputeAlpha(Style, 6.f), 0.f);
	TestEqual(TEXT("전체 시간"), Style.GetTotalSeconds(), 5.f);
	Style.FadeInSeconds = 0.f;
	TestEqual(TEXT("나타남 0 초 = 바로 1"), UGrimZoneBannerWidget::ComputeAlpha(Style, 0.f), 1.f);

	// 설정: 처음 큰 구역은 이름 + 한 줄, 다시 들어선 소구역은 띄우지 않는다(DefaultGame.ini)
	const UGrimZoneSettings* Settings = UGrimZoneSettings::Get();
	TestTrue(TEXT("큰 구역 처음 = 보임 · 한 줄"), Settings->RegionFirst.bEnabled && Settings->RegionFirst.bShowSubtitle);
	TestTrue(TEXT("큰 구역 다시 = 작게"), Settings->RegionRevisit.bEnabled && Settings->RegionRevisit.TitleSize < Settings->RegionFirst.TitleSize);
	TestFalse(TEXT("소구역 다시 = 안 띄움"), Settings->SectorRevisit.bEnabled);
	TestTrue(TEXT("구역 맵 = L_OpenBeta"), Settings->Maps.Contains(TEXT("L_OpenBeta")));

	// 줄: 많이 쌓이면 「다시」부터 버린다
	FGrimZoneDef Zone;
	Zone.Name = TEXT("Test");
	Zone.Label = FText::FromString(TEXT("시험"));
	UGrimZoneBannerWidget* Banner = NewObject<UGrimZoneBannerWidget>();
	const int32 Max = FMath::Max(1, Settings->MaxQueuedBanners);
	for (int32 i = 0; i < Max; ++i)
	{
		Zone.Name = FName(TEXT("Revisit"), i + 1);
		Banner->Enqueue(Zone, false);
	}
	Zone.Name = TEXT("NewPlace");
	Banner->Enqueue(Zone, true);
	TestEqual(TEXT("줄 길이는 MaxQueuedBanners"), Banner->GetQueuedCount(), Max);
	TestTrue(TEXT("아무것도 안 떠 있다(틱 전)"), Banner->GetShowingZone().IsNone());
	return true;
}

#endif
