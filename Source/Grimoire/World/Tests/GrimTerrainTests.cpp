#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
		L.Valley.Width = 50.f;
		L.Valley.Depth = 8.f;
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
		Site.Center = FVector2D(170.0, 120.0);
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

	// 가장자리: 서 · 북 · 동은 산등성이(초원보다 높음), 남은 바다
	const float Meadow = FGrimTerrainGenerator::SampleHeight(M, 1000.0, 1500.0);
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
	TestTrue(TEXT("연못 3~4곳"), Ponds >= 3 && Ponds <= 4);

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
	return true;
}
#endif

#endif
