#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "World/GrimTerrainAnalysis.h"
#include "World/GrimTerrainEditorLibrary.h"
#include "World/GrimTerrainGenerator.h"
#include "World/GrimTerrainMarker.h"
#include "World/GrimTerrainTypes.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimTerrainTestLocal
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr const TCHAR* LayoutPath = TEXT("Scripts/Terrain/openbeta_layout.json");

	/** 작은 시험 레이아웃(254 × 254 m): 언덕 · 계곡 · 강 · 개울 · 연못 · 길 · 평지 하나씩 */
	FGrimTerrainLayout MakeSmallLayout(int32 Seed)
	{
		FGrimTerrainLayout L;
		L.Seed = Seed;
		L.SectionQuads = 63;
		L.SectionsPerComponent = 2;
		L.ComponentsX = 2;
		L.ComponentsY = 2;
		L.WorldOrigin = FVector2D(127.0, 60.0);
		L.NorthRiseStart = 0.f;
		L.NorthRiseEnd = 254.f;
		L.WestRidgeWidth = 30.f;
		L.NorthRidgeWidth = 30.f;
		L.EastRidgeWidth = 20.f;
		L.CoastLine = 20.f;
		L.CoastWobble = 3.f;
		L.CoastWidth = 15.f;
		L.CliffWidth = 10.f;
		L.HeadlandBulge = 5.f;
		L.HeadlandWidth = 30.f;
		L.TowerHill.Center = FVector2D(127.0, 60.0);
		L.TowerHill.PlateauRadius = 12.f;
		L.TowerHill.Radius = 40.f;
		L.TowerHill.Height = 40.f;
		L.Valley.Points = { FVector2D(40.0, 220.0), FVector2D(120.0, 190.0), FVector2D(200.0, 180.0) };
		L.Valley.FloorWidth = 20.f;
		L.Valley.Depth = 8.f;
		L.Valley.GentleWidth = 15.f;
		L.Valley.CliffWidth = 5.f;
		L.Valley.FloorSmoothing = 20.f;
		FGrimTerrainZone& Zone = L.Zones.AddDefaulted_GetRef();
		Zone.Name = TEXT("Upland");
		Zone.Polygon = { FVector2D(0.0, 200.0), FVector2D(254.0, 200.0), FVector2D(254.0, 254.0), FVector2D(0.0, 254.0) };
		Zone.BaseHeight = 35.f;
		Zone.BlendWidth = 30.f;
		FGrimTerrainHillSpec& HillSpec = L.Hills.AddDefaulted_GetRef();
		HillSpec.Name = TEXT("Hill_Test");
		HillSpec.Center = FVector2D(60.0, 170.0);
		HillSpec.Height = 10.f;
		HillSpec.Diameter = 50.f;
		FGrimTerrainHillField& Field = L.HillFields.AddDefaulted_GetRef();
		Field.Name = TEXT("Field_Test");
		Field.Polygon = { FVector2D(140.0, 80.0), FVector2D(220.0, 80.0), FVector2D(220.0, 160.0), FVector2D(140.0, 160.0) };
		Field.Count = 3;
		Field.MinDiameter = 20.f;
		Field.MaxDiameter = 40.f;
		Field.MinHeight = 3.f;
		Field.MaxHeight = 8.f;
		L.River.Points = { FVector2D(225.0, 260.0), FVector2D(222.0, 120.0), FVector2D(228.0, -10.0) };
		L.River.Width = 14.f;
		L.River.Depth = 3.f;
		L.River.BankWidth = 12.f;
		FGrimTerrainChannel& Creek = L.Creeks.AddDefaulted_GetRef();
		Creek.Name = TEXT("Creek");
		Creek.Points = { FVector2D(45.0, 218.0), FVector2D(120.0, 190.0), FVector2D(215.0, 178.0) };
		Creek.Width = 3.f;
		Creek.Depth = 0.6f;
		Creek.BankWidth = 3.f;
		FGrimTerrainPond& Pond = L.Ponds.AddDefaulted_GetRef();
		Pond.Name = TEXT("Pond");
		Pond.Center = FVector2D(80.0, 120.0);
		Pond.Radius = 12.f;
		Pond.ShoreWidth = 6.f;
		FGrimTerrainChannel& Path = L.Paths.AddDefaulted_GetRef();
		Path.Points = { FVector2D(127.0, 70.0), FVector2D(130.0, 150.0), FVector2D(110.0, 190.0) };
		Path.Width = 3.f;
		FGrimTerrainSite& Site = L.BaseSites.AddDefaulted_GetRef();
		Site.Center = FVector2D(110.0, 100.0);
		Site.Radius = 8.f;
		Site.BlendWidth = 8.f;
		FGrimTerrainFoliageRule& Tree = L.Foliage.AddDefaulted_GetRef();
		Tree.Name = TEXT("Tree");
		Tree.DensityPerHa = 60.f;
		Tree.ValleyDensityPerHa = 200.f;
		return L;
	}

	bool LoadOpenBeta(FAutomationTestBase& Test, FGrimTerrainLayout& Out)
	{
		const bool bLoaded = UGrimTerrainEditorLibrary::LoadLayoutFromJson(LayoutPath, Out);
		Test.TestTrue(TEXT("openbeta_layout.json 읽음"), bLoaded);
		return bLoaded;
	}

	/** 오픈 베타 지형은 크니까(약 830만 정점) 한 번만 만들어 테스트끼리 나눠 쓴다 */
	const FGrimTerrainMaps* GetOpenBetaMaps(FAutomationTestBase& Test, FGrimTerrainLayout& OutLayout)
	{
		static FGrimTerrainMaps CachedMaps;
		static FString CachedText;
		if (!LoadOpenBeta(Test, OutLayout))
		{
			return nullptr;
		}
		FString Now;
		FFileHelper::LoadFileToString(Now, *FPaths::Combine(FPaths::ProjectDir(), LayoutPath));
		if (!CachedMaps.IsValid() || CachedText != Now)
		{
			FGrimTerrainGenerator::Generate(OutLayout, CachedMaps);
			CachedText = Now;
		}
		return &CachedMaps;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainLayoutSizeTest, "Grimoire.Terrain.Layout.Size", GrimTerrainTestLocal::Flags)
bool FGrimTerrainLayoutSizeTest::RunTest(const FString& Parameters)
{
	FGrimTerrainLayout Layout;
	FString Error;
	TestTrue(TEXT("기본 레이아웃은 유효"), FGrimTerrainGenerator::Validate(Layout, Error));
	// 127칸 × 2² 섹션 = 구성 요소 254칸. 8 × 16 구성 요소 → 2032 × 4064 m(1 m 칸) = 2033 × 4065 정점
	TestEqual(TEXT("정점 X"), FGrimTerrainGenerator::GetVertexCount(Layout).X, 2033);
	TestEqual(TEXT("정점 Y"), FGrimTerrainGenerator::GetVertexCount(Layout).Y, 4065);

	FGrimTerrainLayout Bad = Layout;
	Bad.SectionQuads = 100;
	TestFalse(TEXT("섹션 100칸은 거부"), FGrimTerrainGenerator::Validate(Bad, Error));
	Bad = Layout;
	Bad.SectionsPerComponent = 3;
	TestFalse(TEXT("섹션 3 × 3 은 거부"), FGrimTerrainGenerator::Validate(Bad, Error));

	TestEqual(TEXT("해수면 = 32768"), static_cast<int32>(FGrimTerrainGenerator::HeightToLandscape(0.f)), 32768);
	TestEqual(TEXT("1 m = 128"), static_cast<int32>(FGrimTerrainGenerator::HeightToLandscape(1.f)), 32768 + 128);
	TestEqual(TEXT("왕복 42 m"), FGrimTerrainGenerator::LandscapeToHeight(FGrimTerrainGenerator::HeightToLandscape(42.f)), 42.f, 0.01f);
	TestEqual(TEXT("왕복 -25 m"), FGrimTerrainGenerator::LandscapeToHeight(FGrimTerrainGenerator::HeightToLandscape(-25.f)), -25.f, 0.01f);
	TestEqual(TEXT("범위 밖은 잘림"), static_cast<int32>(FGrimTerrainGenerator::HeightToLandscape(1000.f)), 65535);

	TestEqual(TEXT("월드 원점 = 탑"), FGrimTerrainGenerator::WorldToMap(Layout, FVector2D::ZeroVector), Layout.WorldOrigin);
	// 월드 +X = 북쪽, +Y = 동쪽: 탑에서 동쪽 10 m = 월드 Y 1000, 북쪽 5 m = 월드 X 500
	TestEqual(TEXT("지도 동쪽 → 월드 +Y"), FGrimTerrainGenerator::MapToWorld(Layout, Layout.WorldOrigin.X + 10.0, Layout.WorldOrigin.Y, 2.0), FVector(0.0, 1000.0, 200.0));
	TestEqual(TEXT("지도 북쪽 → 월드 +X"), FGrimTerrainGenerator::MapToWorld(Layout, Layout.WorldOrigin.X, Layout.WorldOrigin.Y + 5.0, 0.0), FVector(500.0, 0.0, 0.0));
	TestEqual(TEXT("월드 → 지도 왕복"), FGrimTerrainGenerator::WorldToMap(Layout, FVector2D(500.0, 1000.0)), Layout.WorldOrigin + FVector2D(10.0, 5.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainDeterministicTest, "Grimoire.Terrain.Generator.Deterministic", GrimTerrainTestLocal::Flags)
bool FGrimTerrainDeterministicTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainMaps A, B, C;
	FGrimTerrainGenerator::Generate(MakeSmallLayout(7), A);
	FGrimTerrainGenerator::Generate(MakeSmallLayout(7), B);
	FGrimTerrainGenerator::Generate(MakeSmallLayout(8), C);
	TestEqual(TEXT("크기(2 × 2 구성 요소 × 126칸 + 1)"), A.Heights.Num(), 253 * 253);
	TestTrue(TEXT("같은 시드 = 같은 높이"), A.Heights == B.Heights);
	TestTrue(TEXT("같은 시드 = 같은 물가 무게"), A.Shore == B.Shore);
	TestTrue(TEXT("같은 시드 = 같은 길 무게"), A.Path == B.Path);
	TestFalse(TEXT("다른 시드 = 다른 높이"), A.Heights == C.Heights);
	TestTrue(TEXT("손 언덕 + 시드 언덕이 놓임"), A.PlacedHills.Num() >= 2);
	TestEqual(TEXT("같은 시드 = 같은 언덕 수"), A.PlacedHills.Num(), B.PlacedHills.Num());

	const FGrimTerrainLayout Layout = MakeSmallLayout(7);
	TArray<FTransform> T1, T2;
	FGrimTerrainGenerator::ScatterFoliage(Layout, A, Layout.Foliage[0], T1);
	FGrimTerrainGenerator::ScatterFoliage(Layout, B, Layout.Foliage[0], T2);
	TestTrue(TEXT("식생이 뿌려짐"), T1.Num() > 0);
	TestEqual(TEXT("같은 식생 수"), T1.Num(), T2.Num());
	if (T1.Num() > 0 && T1.Num() == T2.Num())
	{
		TestTrue(TEXT("같은 식생 자리"), T1[0].Equals(T2[0]) && T1.Last().Equals(T2.Last()));
	}

	// 작은 지형도 규칙을 지킨다: 탑 평지 · 연못 · 개울 · 평지
	const float Plateau = FGrimTerrainGenerator::SampleHeight(A, Layout.TowerHill.Center.X + 5.0, Layout.TowerHill.Center.Y - 5.0);
	TestEqual(TEXT("탑 평지 높이"), Plateau, Layout.TowerHill.Height, 0.01f);
	const FGrimTerrainWaterBody* Pond = A.Water.FindByPredicate([](const FGrimTerrainWaterBody& W) { return W.Kind == TEXT("Pond"); });
	if (TestNotNull(TEXT("연못 물"), Pond))
	{
		const double Level = Pond->Points[0].Z / 100.0;
		TestTrue(TEXT("연못 가운데는 물 아래"), FGrimTerrainGenerator::SampleHeight(A, 80.0, 120.0) < Level - 1.0);
		TestTrue(TEXT("연못 둑은 물 위"), FGrimTerrainGenerator::SampleHeight(A, 80.0 + 12.0 + 3.0, 120.0) > Level);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainOpenBetaShapeTest, "Grimoire.Terrain.Generator.OpenBeta", GrimTerrainTestLocal::Flags)
bool FGrimTerrainOpenBetaShapeTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = GetOpenBetaMaps(*this, L);
	if (!MapsPtr)
	{
		return false;
	}
	const FGrimTerrainMaps& M = *MapsPtr;
	const FIntPoint Count = FGrimTerrainGenerator::GetVertexCount(L);
	TestEqual(TEXT("2 × 4 km 근처(X)"), (Count.X - 1) * L.QuadSize, 2032.f, 0.5f);
	TestEqual(TEXT("2 × 4 km 근처(Y)"), (Count.Y - 1) * L.QuadSize, 4064.f, 0.5f);
	TestEqual(TEXT("월드 원점 = 탑 언덕 가운데"), L.WorldOrigin, L.TowerHill.Center);

	const float MinH = FMath::Min(M.Heights);
	const float MaxH = FMath::Max(M.Heights);
	TestTrue(FString::Printf(TEXT("높이가 랜드스케이프 범위 안(%.1f ~ %.1f)"), MinH, MaxH), MinH > -250.f && MaxH < 250.f);

	// 탑 평지: 반지름 안 어디나 같은 높이
	int32 Bumps = 0;
	for (int32 Ring = 0; Ring < 4; ++Ring)
	{
		for (int32 Index = 0; Index < 24; ++Index)
		{
			const double R = (L.TowerHill.PlateauRadius - 1.0) * (Ring + 1) / 4.0;
			const double A = 2.0 * UE_DOUBLE_PI * Index / 24.0;
			const float H = FGrimTerrainGenerator::SampleHeight(M, L.TowerHill.Center.X + R * FMath::Cos(A), L.TowerHill.Center.Y + R * FMath::Sin(A));
			Bumps += FMath::Abs(H - L.TowerHill.Height) > 0.01f ? 1 : 0;
		}
	}
	TestEqual(TEXT("탑 평지는 평평"), Bumps, 0);

	// 가장자리: 서 · 북 · 동은 산등성이(남부 목초지보다 높음), 남은 바다
	const float Meadow = FGrimTerrainGenerator::SampleHeight(M, 1000.0, 800.0);
	double West = 0.0, North = 0.0, South = 0.0;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		West += FGrimTerrainGenerator::SampleHeight(M, 5.0, 800.0 + Index * 140.0) / 20.0;
		North += FGrimTerrainGenerator::SampleHeight(M, 200.0 + Index * 70.0, 4060.0) / 20.0;
		South += FGrimTerrainGenerator::SampleHeight(M, 100.0 + Index * 90.0, 2.0) / 20.0;
	}
	TestTrue(FString::Printf(TEXT("서쪽 산등성이 %.0f > 초원 %.0f + 40"), West, Meadow), West > Meadow + 40.0);
	TestTrue(FString::Printf(TEXT("북쪽 산등성이 %.0f > 초원 %.0f + 60"), North, Meadow), North > Meadow + 60.0);
	TestTrue(FString::Printf(TEXT("남쪽 끝은 바닷속 %.1f"), South), South < L.SeaLevel - 5.0);

	// 큰 강: 가운데선은 해수면 아래(바다로 이어진다)
	int32 DryRiver = 0;
	for (int32 Index = 1; Index + 1 < L.River.Points.Num(); ++Index)
	{
		const FVector2D& P = L.River.Points[Index];
		DryRiver += FGrimTerrainGenerator::SampleHeight(M, P.X, P.Y) >= L.SeaLevel ? 1 : 0;
	}
	TestEqual(TEXT("강바닥은 해수면 아래"), DryRiver, 0);

	// 물: 연못은 둘레보다 낮고 둑이 물보다 높다, 개울은 아래로만 흐른다
	int32 Ponds = 0;
	for (const FGrimTerrainWaterBody& Water : M.Water)
	{
		if (Water.Kind == TEXT("Pond"))
		{
			++Ponds;
			const FGrimTerrainPond* Spec = L.Ponds.FindByPredicate([&](const FGrimTerrainPond& P) { return P.Name == Water.Name; });
			if (!TestNotNull(TEXT("연못 레이아웃"), Spec))
			{
				continue;
			}
			const double Level = Water.Points[0].Z / 100.0;
			TestTrue(FString::Printf(TEXT("%s 물 높이 %.1f 는 해수면 위"), *Water.Name.ToString(), Level), Level > L.SeaLevel + 1.0);
			TestTrue(FString::Printf(TEXT("%s 가운데는 물 아래"), *Water.Name.ToString()), FGrimTerrainGenerator::SampleHeight(M, Spec->Center.X, Spec->Center.Y) < Level - Spec->Depth * 0.8);
			for (int32 Index = 0; Index < 16; ++Index)
			{
				const double A = 2.0 * UE_DOUBLE_PI * Index / 16.0;
				const double R = Water.Size / 100.0 + 0.5;
				const float Rim = FGrimTerrainGenerator::SampleHeight(M, Spec->Center.X + R * FMath::Cos(A), Spec->Center.Y + R * FMath::Sin(A));
				if (Rim <= Level)
				{
					AddError(FString::Printf(TEXT("%s 물 평면 가장자리(%d)가 땅 위로 드러난다: 땅 %.2f ≤ 물 %.2f"), *Water.Name.ToString(), Index, Rim, Level));
					break;
				}
			}
		}
		else if (Water.Kind == TEXT("Creek"))
		{
			int32 UpHill = 0;
			for (int32 Index = 1; Index < Water.Points.Num(); ++Index)
			{
				UpHill += Water.Points[Index].Z > Water.Points[Index - 1].Z + 0.01 ? 1 : 0;
			}
			TestEqual(FString::Printf(TEXT("%s 은 아래로만"), *Water.Name.ToString()), UpHill, 0);
			TestTrue(FString::Printf(TEXT("%s 는 해수면 위"), *Water.Name.ToString()), Water.Points.Last().Z > L.SeaLevel * 100.0);
		}
	}
	TestTrue(FString::Printf(TEXT("연못 4~6곳(%d)"), Ponds), Ponds >= 4 && Ponds <= 6);

	// 거점 평지는 평평하고 물 밖
	for (const FGrimTerrainSite& Site : L.BaseSites)
	{
		const float Center = FGrimTerrainGenerator::SampleHeight(M, Site.Center.X, Site.Center.Y);
		const float Edge = FGrimTerrainGenerator::SampleHeight(M, Site.Center.X + Site.Radius * 0.9, Site.Center.Y);
		TestEqual(FString::Printf(TEXT("거점 %s 평평"), *Site.Name.ToString()), Edge, Center, 0.05f);
		TestTrue(FString::Printf(TEXT("거점 %s 는 물 밖(%.1f m)"), *Site.Name.ToString(), Center), Center > L.SeaLevel + 1.0);
	}

	// 표식은 지도 안, 서식 구역은 땅 위(켈피는 물가)
	for (const FGrimTerrainMarkerSpec& Marker : L.Markers)
	{
		TestTrue(FString::Printf(TEXT("표식 %s 지도 안"), *Marker.Name.ToString()),
			Marker.Center.X > 0.0 && Marker.Center.Y > 0.0 && Marker.Center.X < (Count.X - 1) * L.QuadSize && Marker.Center.Y < (Count.Y - 1) * L.QuadSize);
		TestTrue(FString::Printf(TEXT("표식 %s 는 땅 위"), *Marker.Name.ToString()), FGrimTerrainGenerator::SampleHeight(M, Marker.Center.X, Marker.Center.Y) > L.SeaLevel);
	}
	const TArray<FName> Habitats = { TEXT("Habitat.Jackalope"), TEXT("Habitat.Kelpie"), TEXT("Habitat.BlackDog") };
	for (const FName& Habitat : Habitats)
	{
		TestTrue(FString::Printf(TEXT("서식 구역 %s"), *Habitat.ToString()), L.Markers.ContainsByPredicate([&](const FGrimTerrainMarkerSpec& Each) { return Each.Tag == Habitat; }));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainFoliageRulesTest, "Grimoire.Terrain.Foliage.Rules", GrimTerrainTestLocal::Flags)
bool FGrimTerrainFoliageRulesTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = GetOpenBetaMaps(*this, L);
	if (!MapsPtr)
	{
		return false;
	}
	const FGrimTerrainMaps& M = *MapsPtr;
	TestTrue(TEXT("식생 규칙이 있다"), L.Foliage.Num() >= 3);
	for (const FGrimTerrainFoliageRule& Rule : L.Foliage)
	{
		TArray<FTransform> Transforms;
		FGrimTerrainGenerator::ScatterFoliage(L, M, Rule, Transforms);
		TestTrue(FString::Printf(TEXT("%s 가 뿌려짐(%d)"), *Rule.Name.ToString(), Transforms.Num()), Transforms.Num() > 0);
		int32 Broken = 0;
		for (int32 Index = 0; Index < Transforms.Num(); Index += FMath::Max(1, Transforms.Num() / 3000))
		{
			const FVector World = Transforms[Index].GetLocation();
			const FVector2D Map = FGrimTerrainGenerator::WorldToMap(L, FVector2D(World));
			const int32 X = FMath::Clamp(FMath::RoundToInt32(Map.X / M.QuadSize), 0, M.SizeX - 1);
			const int32 Y = FMath::Clamp(FMath::RoundToInt32(Map.Y / M.QuadSize), 0, M.SizeY - 1);
			const int32 I = M.Index(X, Y);
			const float Height = FGrimTerrainGenerator::SampleHeight(M, Map.X, Map.Y);
			const bool bOk = M.WaterDistance[I] >= Rule.MinWaterDistance - 0.01f
				&& M.PathDistance[I] >= Rule.MinPathDistance - 0.01f
				&& Height >= Rule.MinHeight - 0.01f && Height <= Rule.MaxHeight + 0.01f
				&& FVector2D::Distance(Map, L.TowerHill.Center) >= L.TowerHill.PlateauRadius + L.TowerClearance - 0.01;
			Broken += bOk ? 0 : 1;
		}
		TestEqual(FString::Printf(TEXT("%s 가 물 · 길 · 높이 · 탑 규칙을 지킴"), *Rule.Name.ToString()), Broken, 0);
	}
	return true;
}

// ───── 설계 검사(docs/dev/level-design.md 「6. 다 만든 뒤 확인할 것」) — 기준값은 레이아웃 Analysis ─────

namespace GrimTerrainTestLocal
{
	/** 오픈 베타 검사 결과도 한 번만(시선 검사가 몇 초 걸린다) */
	const FGrimTerrainAnalysisResult* GetOpenBetaAnalysis(FAutomationTestBase& Test, FGrimTerrainLayout& OutLayout, const FGrimTerrainMaps*& OutMaps)
	{
		static FGrimTerrainAnalysisResult Cached;
		static const FGrimTerrainMaps* CachedFor = nullptr;
		static FString CachedText;
		OutMaps = GetOpenBetaMaps(Test, OutLayout);
		if (!OutMaps)
		{
			return nullptr;
		}
		FString Now;
		FFileHelper::LoadFileToString(Now, *FPaths::Combine(FPaths::ProjectDir(), LayoutPath));
		if (CachedFor != OutMaps || CachedText != Now)
		{
			Cached = FGrimTerrainAnalysis::Run(OutLayout, *OutMaps);
			CachedFor = OutMaps;
			CachedText = Now;
		}
		Test.AddInfo(Cached.ToString());
		return &Cached;
	}

	/** 계곡 가운데선 둘레 깊이: 양쪽 둑(바닥 반폭 + 비탈 + 15 m) 높이 − 바닥 높이 */
	void ValleyDepths(const FGrimTerrainLayout& L, const FGrimTerrainMaps& M, double& OutGentle, double& OutCliff)
	{
		OutGentle = OutCliff = 0.0;
		int32 Samples = 0;
		const TArray<FVector2D>& P = L.Valley.Points;
		for (int32 Index = 1; Index + 1 < P.Num(); ++Index)
		{
			const FVector2D Dir = (P[Index + 1] - P[Index - 1]).GetSafeNormal();
			const FVector2D Left(-Dir.Y, Dir.X);
			const FVector2D CliffDir = L.Valley.CliffSide >= 0 ? Left : -Left;
			const double Floor = FGrimTerrainGenerator::SampleHeight(M, P[Index].X, P[Index].Y);
			const FVector2D Gentle = P[Index] - CliffDir * (L.Valley.FloorWidth * 0.5 + L.Valley.GentleWidth + 15.0);
			const FVector2D Cliff = P[Index] + CliffDir * (L.Valley.FloorWidth * 0.5 + L.Valley.CliffWidth + 15.0);
			OutGentle += FGrimTerrainGenerator::SampleHeight(M, Gentle.X, Gentle.Y) - Floor;
			OutCliff += FGrimTerrainGenerator::SampleHeight(M, Cliff.X, Cliff.Y) - Floor;
			++Samples;
		}
		if (Samples > 0)
		{
			OutGentle /= Samples;
			OutCliff /= Samples;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainDesignShapeTest, "Grimoire.Terrain.Design.Shape", GrimTerrainTestLocal::Flags)
bool FGrimTerrainDesignShapeTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = nullptr;
	const FGrimTerrainAnalysisResult* Result = GetOpenBetaAnalysis(*this, L, MapsPtr);
	if (!Result)
	{
		return false;
	}
	const FGrimTerrainMaps& M = *MapsPtr;
	const FGrimTerrainAnalysisSpec& A = L.Analysis;

	// 1. 경사 분포
	TestTrue(FString::Printf(TEXT("땅의 %.0f%% 이상이 %.0f~%.0f° (지금 %.1f%%)"), A.RollingMinFraction * 100.f, A.RollingMinDeg, A.RollingMaxDeg, Result->RollingFraction * 100.0),
		Result->RollingFraction >= A.RollingMinFraction);
	TestTrue(FString::Printf(TEXT("평지(0~%.0f°)는 %.0f%% 이하 (지금 %.1f%%)"), A.FlatMaxDeg, A.FlatMaxFraction * 100.f, Result->FlatFraction * 100.0),
		Result->FlatFraction <= A.FlatMaxFraction);

	// 2. 구역 높이 차
	TestEqual(TEXT("높이 차를 볼 구역이 모두 있다"), Result->ZoneMeans.Num(), A.ZoneSteps.Num());
	for (int32 Index = 0; Index < Result->ZoneSteps.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("%s → %s 평균 높이 차 %.1f m ≥ %.0f m"), *Result->ZoneNames[Index].ToString(), *Result->ZoneNames[Index + 1].ToString(), Result->ZoneSteps[Index], A.ZoneStepMin),
			Result->ZoneSteps[Index] >= A.ZoneStepMin);
	}

	// 언덕 군: 크기가 섞이고(작은 것이 많고 큰 것이 적게), 10~60 m · 지름 150~500 m, 가장 가파른 곳 ≤ 약 27°
	int32 HillCount = 0, Small = 0, Big = 0, Outcrops = 0, OutOfRange = 0, TooSteep = 0;
	for (const FGrimTerrainPlacedHill& Hill : M.PlacedHills)
	{
		if (Hill.bSmallShape)
		{
			++Outcrops;
			continue;
		}
		++HillCount;
		const float Diameter = Hill.Radius * 2.f;
		Small += Diameter < 250.f ? 1 : 0;
		Big += Diameter >= 350.f ? 1 : 0;
		OutOfRange += (Hill.Height < 9.99f || Hill.Height > 60.01f || Diameter < 149.9f || Diameter > 500.1f) ? 1 : 0;
		const float MaxSlope = FMath::RadiansToDegrees(FMath::Atan(Hill.Height / Hill.Radius * Hill.Aspect * FGrimTerrainGenerator::HillMaxSlopeFactor(Hill.Sharpness)));
		TooSteep += MaxSlope > 35.f ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("언덕 %d(지름 250 m 아래 %d · 350 m 이상 %d), 바위 노두 %d"), HillCount, Small, Big, Outcrops));
	TestTrue(FString::Printf(TEXT("언덕이 40 개 넘게(%d)"), HillCount), HillCount >= 40);
	TestTrue(FString::Printf(TEXT("작은 언덕(%d)이 큰 언덕(%d)보다 많다"), Small, Big), Small > Big * 2 && Big >= 3);
	TestEqual(TEXT("언덕 높이 10~60 m · 지름 150~500 m"), OutOfRange, 0);
	TestEqual(TEXT("언덕 가장 가파른 곳 ≤ 35°(타원 짧은 쪽 포함)"), TooSteep, 0);
	TestTrue(FString::Printf(TEXT("바위 노두(%d)"), Outcrops), Outcrops >= 20);
	for (const TCHAR* Name : { TEXT("Hill_GiantTree"), TEXT("Hill_StoneCircle"), TEXT("Hill_Windmill"), TEXT("Hill_Lookout") })
	{
		TestTrue(FString::Printf(TEXT("손 언덕 %s"), Name), M.PlacedHills.ContainsByPredicate([Name](const FGrimTerrainPlacedHill& Hill) { return Hill.Name == Name; }));
	}

	// 계곡: 바닥이 둘레보다 30~45 m 낮고(평균), 한쪽은 절벽
	double Gentle = 0.0, Cliff = 0.0;
	ValleyDepths(L, M, Gentle, Cliff);
	AddInfo(FString::Printf(TEXT("계곡 깊이(평균): 완만한 쪽 %.1f m, 절벽 쪽 %.1f m"), Gentle, Cliff));
	TestTrue(FString::Printf(TEXT("계곡 완만한 쪽 깊이 %.1f m ≥ 30"), Gentle), Gentle >= 30.0);
	TestTrue(FString::Printf(TEXT("계곡 절벽 쪽 깊이 %.1f m ≥ 30"), Cliff), Cliff >= 30.0);
	int32 CliffSamples = 0;
	for (int32 Index = 1; Index + 1 < L.Valley.Points.Num(); ++Index)
	{
		const FVector2D Dir = (L.Valley.Points[Index + 1] - L.Valley.Points[Index - 1]).GetSafeNormal();
		const FVector2D CliffDir = (L.Valley.CliffSide >= 0 ? 1.0 : -1.0) * FVector2D(-Dir.Y, Dir.X);
		// 바닥 · 절벽 폭이 들쭉날쭉하므로 절벽 쪽 단면에서 가장 가파른 곳을 본다
		float Steepest = 0.f;
		const double Far = L.Valley.FloorWidth * 0.5 * (1.0 + L.Valley.WidthVariation) + L.Valley.CliffWidth * (1.0 + L.Valley.CliffVariation);
		for (double Along = L.Valley.FloorWidth * 0.25; Along <= Far; Along += 2.0)
		{
			const FVector2D At = L.Valley.Points[Index] + CliffDir * Along;
			Steepest = FMath::Max(Steepest, FGrimTerrainGenerator::SampleSlopeDeg(M, At.X, At.Y));
		}
		CliffSamples += Steepest >= 45.f ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("계곡 절벽 쪽 비탈이 45° 넘는 곳(%d / %d)"), CliffSamples, L.Valley.Points.Num() - 2), CliffSamples * 2 >= L.Valley.Points.Num() - 2);

	// 폭포: 고지 → 계곡 단차, 랜드마크 자리 가까이
	const FGrimTerrainLandmark* FallsMark = L.Landmarks.FindByPredicate([](const FGrimTerrainLandmark& Each) { return Each.Kind == TEXT("Waterfall"); });
	const FGrimTerrainWaterBody* Falls = nullptr;
	double FallsDistance = TNumericLimits<double>::Max();
	for (const FGrimTerrainWaterBody& Water : M.Water)
	{
		if (Water.Kind == TEXT("Falls") && FallsMark)
		{
			const double Distance = FVector2D::Distance(FGrimTerrainGenerator::WorldToMap(L, FVector2D(Water.Points[0])), FallsMark->Center);
			if (Distance < FallsDistance)
			{
				FallsDistance = Distance;
				Falls = &Water;
			}
		}
	}
	if (TestNotNull(TEXT("폭포 랜드마크"), FallsMark) && TestNotNull(TEXT("폭포가 생겼다"), Falls))
	{
		TestTrue(FString::Printf(TEXT("폭포 높이 %.0f m ≥ 20"), Falls->Depth / 100.f), Falls->Depth >= 2000.f);
		const FVector2D Top = FGrimTerrainGenerator::WorldToMap(L, FVector2D(Falls->Points[0]));
		TestTrue(FString::Printf(TEXT("폭포 위 끝(%.0f, %.0f)이 랜드마크 자리에서 60 m 안"), Top.X, Top.Y), FVector2D::Distance(Top, FallsMark->Center) <= 60.0);
	}

	// 고개: 북쪽 산등성이의 틈이 둘레 산등성이보다 낮다
	for (const FGrimTerrainGap& Gap : L.PassGaps)
	{
		const double MapH = (M.SizeY - 1) * M.QuadSize;
		const float GapHeight = FGrimTerrainGenerator::SampleHeight(M, Gap.CenterU, MapH - 60.0);
		const float Side = 0.5f * (FGrimTerrainGenerator::SampleHeight(M, Gap.CenterU - Gap.Width - Gap.BlendWidth, MapH - 60.0) + FGrimTerrainGenerator::SampleHeight(M, Gap.CenterU + Gap.Width + Gap.BlendWidth, MapH - 60.0));
		TestTrue(FString::Printf(TEXT("고개 %s 바닥 %.0f m < 양옆 산등성이 %.0f m − 30"), *Gap.Name.ToString(), GapHeight, Side), GapHeight < Side - 30.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainDesignPathsTest, "Grimoire.Terrain.Design.Paths", GrimTerrainTestLocal::Flags)
bool FGrimTerrainDesignPathsTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = nullptr;
	const FGrimTerrainAnalysisResult* Result = GetOpenBetaAnalysis(*this, L, MapsPtr);
	if (!Result)
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("길 경사 %.1f° ≤ %.0f° (%.0f, %.0f)"), Result->MaxPathGradeDeg, L.Analysis.PathMaxGradeDeg, Result->MaxPathGradeAt.X, Result->MaxPathGradeAt.Y),
		Result->MaxPathGradeDeg <= L.Analysis.PathMaxGradeDeg);
	TestTrue(FString::Printf(TEXT("큰길 · 갈림길 3 개 이상(%d)"), MapsPtr->PathLines.Num()), MapsPtr->PathLines.Num() >= 3);
	// 4. 밀도
	TestEqual(FString::Printf(TEXT("길 위 어디서나 %.0f m 안에 POI (가장 먼 곳 %.0f m, %.0f, %.0f)"), L.Analysis.PoiMaxDistance, Result->MaxPoiDistance, Result->MaxPoiDistanceAt.X, Result->MaxPoiDistanceAt.Y),
		Result->DensityFailures, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainDesignSightTest, "Grimoire.Terrain.Design.Sightlines", GrimTerrainTestLocal::Flags)
bool FGrimTerrainDesignSightTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = nullptr;
	const FGrimTerrainAnalysisResult* Result = GetOpenBetaAnalysis(*this, L, MapsPtr);
	if (!Result)
	{
		return false;
	}
	// 3. 길 위 샘플 점마다 수평선에 POI · 랜드마크 MinVisible 개 이상
	FString Fails;
	for (int32 Index = 0; Index < FMath::Min(Result->SightFailPoints.Num(), 12); ++Index)
	{
		Fails += FString::Printf(TEXT(" (%.0f, %.0f)"), Result->SightFailPoints[Index].X, Result->SightFailPoints[Index].Y);
	}
	TestEqual(FString::Printf(TEXT("길 샘플 %d 곳 모두 %d 개 이상 보인다(최소 %d, 평균 %.1f) 모자란 곳:%s"), Result->PathSamples, L.Analysis.MinVisible, Result->MinVisible, Result->MeanVisible, *Fails),
		Result->SightFailures, 0);

	// 이름 붙은 시선(탑 문 앞 · 고목 언덕 꼭대기 …)
	for (const FGrimTerrainViewCheck& Check : L.Analysis.ViewChecks)
	{
		const TArray<FName> Seen = FGrimTerrainAnalysis::VisibleFrom(L, *MapsPtr, Check.From.X, Check.From.Y, Check.EyeHeight);
		FString SeenText;
		for (const FName& Name : Seen)
		{
			SeenText += TEXT(" ") + Name.ToString();
		}
		AddInfo(FString::Printf(TEXT("%s 에서 보임:%s"), *Check.Name.ToString(), *SeenText));
		for (const FName& Must : Check.MustSee)
		{
			TestTrue(FString::Printf(TEXT("%s 에서 %s 가 보인다"), *Check.Name.ToString(), *Must.ToString()), Seen.Contains(Must));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainDesignPoiTest, "Grimoire.Terrain.Design.Pois", GrimTerrainTestLocal::Flags)
bool FGrimTerrainDesignPoiTest::RunTest(const FString& Parameters)
{
	using namespace GrimTerrainTestLocal;
	FGrimTerrainLayout L;
	const FGrimTerrainMaps* MapsPtr = GetOpenBetaMaps(*this, L);
	if (!MapsPtr)
	{
		return false;
	}
	const FGrimTerrainMaps& M = *MapsPtr;

	// 랜드마크 7 — 종류가 모두 다르다(실루엣이 겹치지 않게)
	TSet<FName> LandmarkKinds;
	for (const FGrimTerrainLandmark& Landmark : L.Landmarks)
	{
		LandmarkKinds.Add(Landmark.Kind);
		TestTrue(FString::Printf(TEXT("랜드마크 %s 는 땅 위"), *Landmark.Name.ToString()), FGrimTerrainGenerator::SampleHeight(M, Landmark.Center.X, Landmark.Center.Y) > L.SeaLevel + 1.f);
	}
	TestEqual(TEXT("랜드마크 7"), L.Landmarks.Num(), 7);
	TestEqual(TEXT("랜드마크 종류가 모두 다르다"), LandmarkKinds.Num(), L.Landmarks.Num());
	for (const TCHAR* Kind : { TEXT("Tower"), TEXT("GiantTree"), TEXT("RockGate"), TEXT("StoneCircle"), TEXT("Waterfall"), TEXT("Watchtower"), TEXT("Windmill") })
	{
		TestTrue(FString::Printf(TEXT("랜드마크 %s"), Kind), LandmarkKinds.Contains(Kind));
	}

	// POI 30~45 · 묶음 8~10(묶음마다 2~4) · 종류가 섞인다
	TestTrue(FString::Printf(TEXT("POI 30~45 (%d)"), L.Pois.Num()), L.Pois.Num() >= 30 && L.Pois.Num() <= 45);
	TMap<FName, int32> Clusters, Kinds;
	TSet<FName> Names;
	for (const FGrimTerrainPoi& Poi : L.Pois)
	{
		++Clusters.FindOrAdd(Poi.Cluster);
		++Kinds.FindOrAdd(Poi.Kind);
		TestFalse(FString::Printf(TEXT("POI 이름 겹침 %s"), *Poi.Name.ToString()), Names.Contains(Poi.Name));
		Names.Add(Poi.Name);
		const int32 I = M.NearestIndex(Poi.Center.X, Poi.Center.Y);
		TestTrue(FString::Printf(TEXT("POI %s 는 물 밖 땅 위"), *Poi.Name.ToString()), M.Heights[I] > L.SeaLevel + 0.5f && M.WaterDistance[I] > 1.f);
	}
	TestTrue(FString::Printf(TEXT("묶음 8~10 (%d)"), Clusters.Num()), Clusters.Num() >= 8 && Clusters.Num() <= 10);
	for (const TPair<FName, int32>& Cluster : Clusters)
	{
		TestTrue(FString::Printf(TEXT("묶음 %s 는 2~4 개(%d)"), *Cluster.Key.ToString(), Cluster.Value), Cluster.Value >= 2 && Cluster.Value <= 4);
	}
	// 계획 비율(속성 퍼즐 20 · 자원 25 · 생물 15 · 폐허 15 · 전망대 10 · 이야기 10 · 거점 터 5 %)에서 ±1.5 개 안
	const TArray<TPair<FName, float>> Plan = { { TEXT("Puzzle"), 0.20f }, { TEXT("Resource"), 0.25f }, { TEXT("Creature"), 0.15f }, { TEXT("Ruin"), 0.15f },
		{ TEXT("Viewpoint"), 0.10f }, { TEXT("Story"), 0.10f }, { TEXT("BaseSite"), 0.05f } };
	FString KindText;
	for (const TPair<FName, float>& Each : Plan)
	{
		const int32* Have = Kinds.Find(Each.Key);
		const float Want = Each.Value * L.Pois.Num();
		KindText += FString::Printf(TEXT(" %s %d(계획 %.1f)"), *Each.Key.ToString(), Have ? *Have : 0, Want);
		TestTrue(FString::Printf(TEXT("POI 종류 %s 수 %d ≈ 계획 %.1f"), *Each.Key.ToString(), Have ? *Have : 0, Want), Have && FMath::Abs(*Have - Want) <= 1.5f);
	}
	AddInfo(TEXT("POI 종류:") + KindText);
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTerrainOpenBetaMapTest, "Grimoire.Terrain.Map.OpenBeta", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FGrimTerrainOpenBetaMapTest::RunTest(const FString& Parameters)
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	const FString MapPackage = TEXT("/Game/Grimoire/Maps/L_OpenBeta");
	const FString ActorsPath = TEXT("/Game/__ExternalActors__/Grimoire/Maps/L_OpenBeta");
	Registry.ScanPathsSynchronous({ TEXT("/Game/Grimoire/Maps"), ActorsPath }, true);

	TArray<FAssetData> MapAssets;
	Registry.GetAssetsByPackageName(*MapPackage, MapAssets, true);
	if (!TestEqual(TEXT("L_OpenBeta 맵이 있다"), MapAssets.Num(), 1))
	{
		return false;
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(*ActorsPath);
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = true;
	TArray<FAssetData> Actors;
	Registry.GetAssets(Filter, Actors);

	TMap<FString, int32> ByClass;
	TSet<FString> MarkerTags;
	for (const FAssetData& Actor : Actors)
	{
		++ByClass.FindOrAdd(Actor.AssetClassPath.GetAssetName().ToString());
		FString Tag;
		if (Actor.GetTagValue(AGrimTerrainMarker::AssetRegistryMarkerTagName, Tag))
		{
			MarkerTags.Add(Tag);
		}
	}
	auto CountOf = [&ByClass](const TCHAR* Name) { const int32* Found = ByClass.Find(Name); return Found ? *Found : 0; };
	AddInfo(FString::Printf(TEXT("외부 액터 %d개: 랜드스케이프 프록시 %d, 표식 %d, 폴리지 %d, 물리 볼륨 %d"), Actors.Num(),
		CountOf(TEXT("LandscapeStreamingProxy")), CountOf(TEXT("GrimTerrainMarker")), CountOf(TEXT("InstancedFoliageActor")), CountOf(TEXT("PhysicsVolume"))));

	TestEqual(TEXT("랜드스케이프 하나"), CountOf(TEXT("Landscape")), 1);
	TestTrue(TEXT("스트리밍 프록시 여러 개"), CountOf(TEXT("LandscapeStreamingProxy")) >= 8);
	TestTrue(TEXT("폴리지(식생)"), CountOf(TEXT("InstancedFoliageActor")) >= 1);
	TestTrue(TEXT("물 볼륨(바다 · 연못)"), CountOf(TEXT("PhysicsVolume")) >= 4);
	TestEqual(TEXT("시작 자리 하나"), CountOf(TEXT("PlayerStart")), 1);
	TestEqual(TEXT("기후 볼륨 하나(Meadow)"), CountOf(TEXT("GrimClimateRegionVolume")), 1);
	TestEqual(TEXT("하늘 하나"), CountOf(TEXT("GrimSkyController")), 1);
	TestEqual(TEXT("탑 스승 하나"), CountOf(TEXT("GrimTowerNPC")), 1);
	TestTrue(TEXT("표식 여러 개"), CountOf(TEXT("GrimTerrainMarker")) >= 8);
	for (const TCHAR* Tag : { TEXT("Terrain.Site.Tower"), TEXT("Terrain.Site.TowerYard"), TEXT("Terrain.Site.Base"), TEXT("Terrain.Path"),
		TEXT("Habitat.Jackalope"), TEXT("Habitat.Kelpie"), TEXT("Habitat.BlackDog") })
	{
		TestTrue(FString::Printf(TEXT("표식 %s"), Tag), MarkerTags.Contains(Tag));
	}
	// 랜드마크 7 · POI 종류(레이아웃에 있는 것) 표식
	FGrimTerrainLayout Layout;
	if (GrimTerrainTestLocal::LoadOpenBeta(*this, Layout))
	{
		for (const FGrimTerrainLandmark& Landmark : Layout.Landmarks)
		{
			const FString Tag = TEXT("Landmark.") + Landmark.Kind.ToString();
			TestTrue(FString::Printf(TEXT("표식 %s"), *Tag), MarkerTags.Contains(Tag));
		}
		for (const FGrimTerrainPoi& Poi : Layout.Pois)
		{
			const FString Tag = TEXT("Poi.") + Poi.Kind.ToString();
			TestTrue(FString::Printf(TEXT("표식 %s"), *Tag), MarkerTags.Contains(Tag));
		}
	}
	return true;
}
#endif

#endif
