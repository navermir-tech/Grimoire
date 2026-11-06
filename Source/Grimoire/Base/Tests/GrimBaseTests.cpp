#include "Misc/AutomationTest.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseTypes.h"
#include "Base/GrimBuildPiece.h"
#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimItemContainerComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일의 도우미와 부딪히지 않게 이름을 붙인다.
namespace GrimBaseTestsLocal
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGrimBuildSlot MakeSlot(EGrimBuildCategory Category, int32 X, int32 Y, int32 Level = 0, int32 Dir = 0)
	{
		FGrimBuildSlot Slot;
		Slot.Category = Category;
		Slot.X = X;
		Slot.Y = Y;
		Slot.Level = Level;
		Slot.Dir = Dir;
		return Slot;
	}

	TMap<uint64, bool> StableOf(const TArray<FGrimBuildSlot>& Slots)
	{
		return FGrimBuildSupport::ComputeStable(Slots);
	}

	struct FBaseTestWorld
	{
		UWorld* World = nullptr;

		FBaseTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimBaseTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FBaseTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		AGrimBuildPiece* SpawnPiece(const AGrimBaseCore* Core, FName PieceId, FGrimBuildSlot Slot) const
		{
			const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
			if (!Def)
			{
				return nullptr;
			}
			Slot.Category = Def->Row.Category;
			const FTransform Transform = Core->GetGrid().GetSlotTransform(Slot);
			AGrimBuildPiece* Piece = World->SpawnActorDeferred<AGrimBuildPiece>(AGrimBuildPiece::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			Piece->InitializePiece(PieceId, Slot);
			Piece->FinishSpawning(Transform);
			return Piece;
		}
	};
}

// ───────────────────────── 표 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseTableTest, "Grimoire.Base.Table", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseTableTest::RunTest(const FString& Parameters)
{
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	FString Pieces, Crops, Equipment;
	TestTrue(TEXT("BuildPieces.csv 를 읽는다"), UGrimItemSettings::LoadContentFile(Settings->BuildPiecesCSV, Pieces));
	TestTrue(TEXT("Crops.csv 를 읽는다"), UGrimItemSettings::LoadContentFile(Settings->CropsCSV, Crops));
	TestTrue(TEXT("Equipment.csv 를 읽는다"), UGrimItemSettings::LoadContentFile(Settings->EquipmentCSV, Equipment));

	FGrimBaseDatabase Database;
	TArray<FString> Problems;
	const bool bClean = FGrimBaseDatabase::BuildFromCSV(Pieces, Crops, Equipment, &UGrimItemSettings::GetDatabase(), Database, Problems);
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}
	TestTrue(TEXT("표에 문제가 없다"), bClean);

	// M4 범위: 구조 조각 6종 · 시설 4종 + 밭 · 작물 2종.
	TSet<EGrimBuildCategory> Categories;
	TSet<FString> StationClasses;
	for (const TPair<FName, FGrimBuildPieceDefinition>& Pair : Database.Pieces)
	{
		Categories.Add(Pair.Value.Row.Category);
		if (Pair.Value.Row.Category == EGrimBuildCategory::Station)
		{
			StationClasses.Add(Pair.Value.Row.SpawnClass);
			TestNotNull(*FString::Printf(TEXT("시설 %s 의 클래스"), *Pair.Key.ToString()), LoadClass<AActor>(nullptr, *Pair.Value.Row.SpawnClass));
		}
	}
	for (const EGrimBuildCategory Category : { EGrimBuildCategory::Core, EGrimBuildCategory::Foundation, EGrimBuildCategory::Wall, EGrimBuildCategory::Door,
		EGrimBuildCategory::Roof, EGrimBuildCategory::Stair, EGrimBuildCategory::Pillar })
	{
		TestTrue(*FString::Printf(TEXT("범주 %d 가 표에 있다"), static_cast<int32>(Category)), Categories.Contains(Category));
	}
	TestTrue(TEXT("시설 5종(침대 · 모닥불 · 저장고 · 작업대 · 밭)"), StationClasses.Num() >= 5);
	TestEqual(TEXT("작물 2종"), Database.Crops.Num(), 2);
	TestTrue(TEXT("장비가 있다"), Database.Equipment.Num() >= 3);
	TestEqual(TEXT("건축 패널 순서 = 조각 수"), Database.PieceOrder.Num(), Database.Pieces.Num());

	// 검사가 잡는지: 모르는 아이템 · 시설 클래스 없음 · 거점 핵 없음.
	const FString Header = TEXT("Name,DisplayName,Description,Category,Material,Cost,MaxHealth,RepairItem,Extent,Color,SpawnClass,SortOrder\n");
	FGrimBaseDatabase Bad;
	TArray<FString> BadProblems;
	FGrimBaseDatabase::BuildFromCSV(Header + TEXT("W,벽,,Wall,Wood,\"NoSuchItem:2\",100,Wood,\"(X=1,Y=1,Z=1)\",\"(R=1,G=1,B=1,A=1)\",,0\nS,상자,,Station,Wood,\"Wood:1\",100,Wood,\"(X=1,Y=1,Z=1)\",\"(R=1,G=1,B=1,A=1)\",,1\n"),
		Crops, Equipment, &UGrimItemSettings::GetDatabase(), Bad, BadProblems);
	const FString Joined = FString::Join(BadProblems, TEXT("\n"));
	TestTrue(TEXT("모르는 비용 아이템"), Joined.Contains(TEXT("NoSuchItem")));
	TestTrue(TEXT("시설 SpawnClass 없음"), Joined.Contains(TEXT("SpawnClass")));
	TestTrue(TEXT("거점 핵 없음"), Joined.Contains(TEXT("거점 핵")));
	return true;
}

// ───────────────────────── 격자 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseGridSnapTest, "Grimoire.Base.Grid.Snap", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseGridSnapTest::RunTest(const FString& Parameters)
{
	FGrimBuildGrid Grid;
	Grid.Origin = FVector(1000.f, -500.f, 200.f);
	Grid.Yaw = 90.f;
	auto Aim = [&Grid](float X, float Y, float Z) { return Grid.LocalToWorld(FVector(X, Y, Z)); };

	TestTrue(TEXT("로컬 ↔ 월드 되돌림"), Grid.WorldToLocal(Aim(123.f, -45.f, 67.f)).Equals(FVector(123.f, -45.f, 67.f), 0.01f));
	TestEqual(TEXT("층 높이 = 벽 + 판"), Grid.GetLevelHeight(), 320.f);

	FGrimBuildSlot Slot = Grid.Snap(EGrimBuildCategory::Foundation, Aim(310.f, 20.f, 30.f), 0);
	TestTrue(TEXT("바닥 → 가까운 칸 (1,0,0)"), Slot.X == 1 && Slot.Y == 0 && Slot.Level == 0);

	Slot = Grid.Snap(EGrimBuildCategory::Wall, Aim(140.f, 10.f, 30.f), 0);
	TestTrue(TEXT("벽: 칸 0 의 +X 모서리 → (0,0) 방향 0"), Slot.X == 0 && Slot.Y == 0 && Slot.Dir == 0);
	Slot = Grid.Snap(EGrimBuildCategory::Wall, Aim(-140.f, 10.f, 30.f), 0);
	TestTrue(TEXT("벽: 칸 0 의 −X 모서리 → (−1,0) 방향 0"), Slot.X == -1 && Slot.Y == 0 && Slot.Dir == 0);
	Slot = Grid.Snap(EGrimBuildCategory::Door, Aim(10.f, -140.f, 30.f), 0);
	TestTrue(TEXT("문: 칸 0 의 −Y 모서리 → (0,−1) 방향 1"), Slot.X == 0 && Slot.Y == -1 && Slot.Dir == 1);
	Slot = Grid.Snap(EGrimBuildCategory::Wall, Aim(140.f, 10.f, 350.f + 100.f), 0);
	TestEqual(TEXT("위층 바닥 위를 겨누면 1층 벽"), Slot.Level, 1);

	Slot = Grid.Snap(EGrimBuildCategory::Pillar, Aim(160.f, 170.f, 30.f), 0);
	TestTrue(TEXT("기둥: 꼭짓점 (0,0)"), Slot.X == 0 && Slot.Y == 0);
	Slot = Grid.Snap(EGrimBuildCategory::Pillar, Aim(-160.f, -20.f, 30.f), 0);
	TestTrue(TEXT("기둥: 꼭짓점 (−1,−1)"), Slot.X == -1 && Slot.Y == -1);

	Slot = Grid.Snap(EGrimBuildCategory::Roof, Aim(0.f, 0.f, 330.f), 0);
	TestEqual(TEXT("벽 꼭대기를 겨누면 0층 지붕"), Slot.Level, 0);
	Slot = Grid.Snap(EGrimBuildCategory::Roof, Aim(0.f, 0.f, 0.f), 0);
	TestEqual(TEXT("땅을 겨누면 0층 지붕"), Slot.Level, 0);
	Slot = Grid.Snap(EGrimBuildCategory::Stair, Aim(0.f, 0.f, 30.f), 5);
	TestEqual(TEXT("계단 방향은 4로 돈다"), Slot.Dir, 1);

	// 기준점 · 방향
	const FGrimBuildSlot WallY = GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Wall, 0, 0, 0, 1);
	TestTrue(TEXT("방향 1 벽의 기준점 = (0, 150, 바닥 윗면)"), Grid.GetSlotAnchorLocal(WallY).Equals(FVector(0.f, 150.f, 30.f), 0.01f));
	TestEqual(TEXT("방향 1 벽은 격자 + 90°"), static_cast<float>(Grid.GetSlotTransform(WallY).Rotator().Yaw), 180.f, 0.01f);
	const FGrimBuildSlot Roof = GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Roof, 1, 1, 0);
	TestEqual(TEXT("0층 지붕 높이 = 바닥 윗면 + 벽"), static_cast<float>(Grid.GetSlotAnchorLocal(Roof).Z), 330.f, 0.01f);

	TArray<FGrimShapeBox> Boxes;
	Grid.GetShape(EGrimBuildCategory::Door, Boxes);
	TestEqual(TEXT("문 = 기둥 둘 + 상인방 + 문짝"), Boxes.Num(), 4);
	TestTrue(TEXT("문짝 표시"), Boxes.Num() == 4 && Boxes[3].bDoorLeaf);
	Boxes.Reset();
	Grid.GetShape(EGrimBuildCategory::Stair, Boxes);
	TestTrue(TEXT("계단 한 칸 높이는 오를 수 있는 45 cm 아래"), Boxes.Num() >= 2 && (Boxes[1].Extent.Z - Boxes[0].Extent.Z) * 2.f < 45.f);

	// 겹침 열쇠: 벽과 문은 같은 모서리를 나눠 쓴다.
	TestEqual(TEXT("벽 · 문 같은 모서리"), GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Wall, 2, 3, 0, 1).GetOccupancyKey(),
		GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Door, 2, 3, 0, 1).GetOccupancyKey());
	TestNotEqual(TEXT("바닥 · 지붕은 다른 층"), GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Foundation, 2, 3).GetOccupancyKey(),
		GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Roof, 2, 3).GetOccupancyKey());
	TestNotEqual(TEXT("음수 칸도 따로"), GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Foundation, -1, 0).GetOccupancyKey(),
		GrimBaseTestsLocal::MakeSlot(EGrimBuildCategory::Foundation, 1, 0).GetOccupancyKey());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseGridSupportTest, "Grimoire.Base.Grid.Support", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseGridSupportTest::RunTest(const FString& Parameters)
{
	using namespace GrimBaseTestsLocal;
	TArray<FGrimBuildSlot> Built = { MakeSlot(EGrimBuildCategory::Foundation, 0, 0) };
	TMap<uint64, bool> Stable = StableOf(Built);

	TestTrue(TEXT("0층 바닥은 늘 받쳐진다"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Foundation, 9, 9), Stable));
	TestFalse(TEXT("1층 바닥은 없다"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Foundation, 0, 0, 1), Stable));
	TestTrue(TEXT("바닥 +X 모서리의 벽"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Wall, 0, 0, 0, 0), Stable));
	TestTrue(TEXT("바닥 −X 모서리의 벽(앞 칸 −1)"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Wall, -1, 0, 0, 0), Stable));
	TestFalse(TEXT("바닥 없는 곳의 벽"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Wall, 4, 4, 0, 1), Stable));
	TestTrue(TEXT("바닥 모퉁이의 기둥"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Pillar, -1, -1), Stable));
	TestTrue(TEXT("바닥 위 계단"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Stair, 0, 0), Stable));
	TestFalse(TEXT("벽 없는 지붕"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Roof, 0, 0), Stable));

	Built.Add(MakeSlot(EGrimBuildCategory::Wall, 0, 0, 0, 0));
	Stable = StableOf(Built);
	bool bDirect = false;
	TestTrue(TEXT("벽 위 지붕"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Roof, 0, 0), Stable, &bDirect));
	TestTrue(TEXT("벽 위 지붕은 직접"), bDirect);
	TestTrue(TEXT("모서리를 나눈 이웃 칸 지붕도 직접(같은 벽)"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Roof, 1, 0), Stable, &bDirect) && bDirect);

	Built.Add(MakeSlot(EGrimBuildCategory::Roof, 0, 0));
	Stable = StableOf(Built);
	TestTrue(TEXT("한 칸 내민 지붕(−X)"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Roof, -1, 0), Stable, &bDirect));
	TestFalse(TEXT("내민 지붕은 간접"), bDirect);
	Built.Add(MakeSlot(EGrimBuildCategory::Roof, -1, 0));
	Stable = StableOf(Built);
	TestFalse(TEXT("두 칸 내밀기는 안 된다"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Roof, -2, 0), Stable));
	TestTrue(TEXT("지붕 위(1층) 벽"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Wall, 0, 0, 1, 1), Stable));
	TestFalse(TEXT("지붕 없는 곳 1층 벽"), FGrimBuildSupport::HasSupport(MakeSlot(EGrimBuildCategory::Wall, 5, 0, 1, 1), Stable));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseGridCollapseTest, "Grimoire.Base.Grid.Collapse", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseGridCollapseTest::RunTest(const FString& Parameters)
{
	using namespace GrimBaseTestsLocal;
	// 집 한 칸: 바닥 · 벽 · 지붕 · 내민 지붕 · 1층 벽. 순서를 섞어도 결과는 같아야 한다(지붕이 벽보다 앞에).
	TArray<FGrimBuildSlot> House = {
		MakeSlot(EGrimBuildCategory::Wall, 0, 0, 1, 1),
		MakeSlot(EGrimBuildCategory::Roof, -1, 0),
		MakeSlot(EGrimBuildCategory::Roof, 0, 0),
		MakeSlot(EGrimBuildCategory::Wall, 0, 0, 0, 0),
		MakeSlot(EGrimBuildCategory::Foundation, 0, 0),
	};
	TestEqual(TEXT("다 받쳐지면 무너지는 것 없음"), FGrimBuildSupport::FindUnsupported(House).Num(), 0);

	TArray<FGrimBuildSlot> NoFloor = House;
	NoFloor.RemoveAt(4);
	TestEqual(TEXT("바닥이 없어지면 넷 다 무너진다"), FGrimBuildSupport::FindUnsupported(NoFloor).Num(), 4);

	TArray<FGrimBuildSlot> NoWall = House;
	NoWall.RemoveAt(3);
	const TArray<int32> Fallen = FGrimBuildSupport::FindUnsupported(NoWall);
	TestEqual(TEXT("벽이 없어지면 지붕 둘 · 1층 벽이 무너진다"), Fallen.Num(), 3);
	TestFalse(TEXT("바닥은 남는다"), Fallen.Contains(3));

	// 서로만 받치는 지붕 둘은 떠 있으면 안 된다.
	TArray<FGrimBuildSlot> Floating = { MakeSlot(EGrimBuildCategory::Roof, 3, 3), MakeSlot(EGrimBuildCategory::Roof, 4, 3) };
	TestEqual(TEXT("지붕끼리는 서로 받치지 못한다"), FGrimBuildSupport::FindUnsupported(Floating).Num(), 2);
	return true;
}

// ───────────────────────── 내구도 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseDurabilityRepairTest, "Grimoire.Base.Durability.Repair", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseDurabilityRepairTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("가득이면 재료 0"), FGrimDurabilityMath::GetMaterialsNeeded(200.f, 200.f, 0.25f), 0);
	TestEqual(TEXT("150/200 → 재료 1"), FGrimDurabilityMath::GetMaterialsNeeded(150.f, 200.f, 0.25f), 1);
	TestEqual(TEXT("149/200 → 재료 2"), FGrimDurabilityMath::GetMaterialsNeeded(149.f, 200.f, 0.25f), 2);
	TestEqual(TEXT("0/200 → 재료 4"), FGrimDurabilityMath::GetMaterialsNeeded(0.f, 200.f, 0.25f), 4);
	TestEqual(TEXT("재료 1개 = 25%"), FGrimDurabilityMath::Repair(50.f, 200.f, 1, 0.25f), 100.f, 0.001f);
	TestEqual(TEXT("최대를 넘지 않는다"), FGrimDurabilityMath::Repair(190.f, 200.f, 3, 0.25f), 200.f, 0.001f);
	TestEqual(TEXT("재료 0 이면 그대로"), FGrimDurabilityMath::Repair(10.f, 200.f, 0, 0.25f), 10.f, 0.001f);
	TestEqual(TEXT("잔해 = 비용 절반 내림(3 → 1)"), FGrimDurabilityMath::GetRefundCount(3, 0.5f), 1);
	TestEqual(TEXT("잔해 = 비용 절반(4 → 2)"), FGrimDurabilityMath::GetRefundCount(4, 0.5f), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseDurabilityEquipmentTest, "Grimoire.Base.Durability.Equipment", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseDurabilityEquipmentTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("수리 0번 = 처음 최대"), FGrimDurabilityMath::GetEquipmentMax(200.f, 0, 0.1f, 0.5f), 200.f, 0.001f);
	TestEqual(TEXT("수리 3번 = 70%"), FGrimDurabilityMath::GetEquipmentMax(200.f, 3, 0.1f, 0.5f), 140.f, 0.001f);
	TestEqual(TEXT("수리 9번도 하한 50%"), FGrimDurabilityMath::GetEquipmentMax(200.f, 9, 0.1f, 0.5f), 100.f, 0.001f);
	TestEqual(TEXT("방어구: 피해 25 · 단위 10 → 2.5 닳음"), FGrimDurabilityMath::GetArmorWear(25.f, 1.f, 10.f), 2.5f, 0.001f);

	// 표의 장비로: 음수 내구도 = 새것(가득), 수리 횟수가 최대치를 줄인다.
	const FGrimEquipmentDefinition* Robe = UGrimBaseSettings::GetDatabase().FindEquipment(TEXT("HideRobe"));
	if (TestNotNull(TEXT("HideRobe 장비 표"), Robe))
	{
		FGrimItemInstance Item(TEXT("HideRobe"), 1);
		TestEqual(TEXT("새것 = 최대"), UGrimEquipmentComponent::GetDurability(Item), Robe->Row.MaxDurability, 0.001f);
		Item.RepairCount = 2;
		Item.Durability = 500.f;
		TestEqual(TEXT("수리 2번 최대 = 80%"), UGrimEquipmentComponent::GetMaxDurability(Item), Robe->Row.MaxDurability * 0.8f, 0.001f);
		TestEqual(TEXT("지금 값은 최대를 넘지 않는다"), UGrimEquipmentComponent::GetDurability(Item), Robe->Row.MaxDurability * 0.8f, 0.001f);
	}
	TestTrue(TEXT("장비가 아니면 -1"), UGrimEquipmentComponent::GetMaxDurability(FGrimItemInstance(TEXT("Herb"), 1)) < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseDurabilityDecayTest, "Grimoire.Base.Durability.Decay", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseDurabilityDecayTest::RunTest(const FString& Parameters)
{
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	const float Inside = FGrimDurabilityMath::GetDecayPerSecond(200.f, true, Settings->InsideDecayFractionPerHour, Settings->OutsideDecayFractionPerHour);
	const float Outside = FGrimDurabilityMath::GetDecayPerSecond(200.f, false, Settings->InsideDecayFractionPerHour, Settings->OutsideDecayFractionPerHour);
	TestTrue(TEXT("밖이 안보다 훨씬 빨리 닳는다(R30)"), Outside > Inside * 50.f);
	TestTrue(TEXT("안: 실제 하루에 최대치의 20% 아래"), Inside * 86400.f < 200.f * 0.2f);
	TestTrue(TEXT("밖: 실제 하루 안에 부서진다"), Outside * 86400.f >= 200.f);
	TestEqual(TEXT("끄면(0) 안 닳는다"), FGrimDurabilityMath::GetDecayPerSecond(200.f, false, 0.f, 0.f), 0.f);
	return true;
}

// ───────────────────────── 작물 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseCropGrowthTest, "Grimoire.Base.Crop.Growth", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseCropGrowthTest::RunTest(const FString& Parameters)
{
	FGrimCropRow Crop;
	Crop.GrowSeconds = 100.f;
	Crop.Stages = 3;
	Crop.DryGrowthScale = 0.f;
	Crop.NightGrowthScale = 0.5f;
	Crop.MaxColdGrade = 1;
	Crop.MaxHeatGrade = 2;
	Crop.AshBonus = 0.2f;

	FGrimCropConditions Good;
	Good.bWet = true;
	Good.bDaylight = true;
	TestEqual(TEXT("젖은 밭 · 낮 = 1/GrowSeconds"), FGrimCropMath::GetGrowthRate(Crop, Good), 0.01f, 1e-6f);

	FGrimCropConditions Dry = Good;
	Dry.bWet = false;
	TestEqual(TEXT("마르면 멈춘다"), FGrimCropMath::GetGrowthRate(Crop, Dry), 0.f);

	FGrimCropConditions Night = Good;
	Night.bDaylight = false;
	TestEqual(TEXT("밤은 절반"), FGrimCropMath::GetGrowthRate(Crop, Night), 0.005f, 1e-6f);
	Night.bIlluminated = true;
	TestEqual(TEXT("빛 마법이면 밤에도 낮처럼"), FGrimCropMath::GetGrowthRate(Crop, Night), 0.01f, 1e-6f);

	FGrimCropConditions Cold = Good;
	Cold.ColdGrade = 2;
	TestEqual(TEXT("견디는 추위를 넘으면 멈춘다(설산 = 온실)"), FGrimCropMath::GetGrowthRate(Crop, Cold), 0.f);

	FGrimCropConditions Ash = Good;
	Ash.bAsh = true;
	TestEqual(TEXT("재 비료 +20%"), FGrimCropMath::GetGrowthRate(Crop, Ash), 0.012f, 1e-6f);

	TestEqual(TEXT("단계 0"), FGrimCropMath::GetStage(0.f, 3), 0);
	TestEqual(TEXT("단계 1"), FGrimCropMath::GetStage(0.34f, 3), 1);
	TestEqual(TEXT("단계 2"), FGrimCropMath::GetStage(0.99f, 3), 2);
	TestEqual(TEXT("다 자람 = Stages"), FGrimCropMath::GetStage(1.f, 3), 3);
	TestTrue(TEXT("1 이면 거둘 수 있다"), FGrimCropMath::IsMature(1.f));
	return true;
}

// ───────────────────────── 저장고 · 제작 · 인스턴스 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseContainerTransferTest, "Grimoire.Base.Container.Transfer", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseContainerTransferTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	const FGrimItemDefinition* Wood = Items.Find(TEXT("Wood"));
	if (!TestNotNull(TEXT("Wood 아이템"), Wood))
	{
		return false;
	}
	const int32 Stack = Wood->GetMaxStack();

	TArray<FGrimItemInstance> Bag;
	Bag.SetNum(4);
	Bag[0] = FGrimItemInstance(TEXT("Wood"), 30);
	TArray<FGrimItemInstance> Chest;
	Chest.SetNum(2);
	TestEqual(TEXT("통째로 옮긴다"), FGrimSlotTransferMath::Transfer(Bag, 0, -1, Chest, Items), 30);
	TestTrue(TEXT("가방 칸이 비었다"), Bag[0].IsEmpty());
	TestEqual(TEXT("저장고에 30"), FGrimInventoryModel::CountOf(Chest, TEXT("Wood")), 30);

	// 저장고 칸 하나에 Stack-5 가 있고 빈 칸이 없으면 5개만 들어간다.
	TArray<FGrimItemInstance> Full;
	Full.Add(FGrimItemInstance(TEXT("Wood"), Stack - 5));
	Bag[1] = FGrimItemInstance(TEXT("Wood"), 30);
	TestEqual(TEXT("들어간 만큼만 빠진다"), FGrimSlotTransferMath::Transfer(Bag, 1, -1, Full, Items), 5);
	TestEqual(TEXT("가방에 남은 25"), Bag[1].Count, 25);

	TArray<FGrimItemInstance> Other;
	Other.Add(FGrimItemInstance(TEXT("Stone"), 1));
	TestEqual(TEXT("빈 칸 없으면 0"), FGrimSlotTransferMath::Transfer(Bag, 1, 3, Other, Items), 0);
	TestEqual(TEXT("가방 그대로"), Bag[1].Count, 25);
	TestEqual(TEXT("빈 칸에서는 0"), FGrimSlotTransferMath::Transfer(Bag, 3, -1, Chest, Items), 0);

	// 장비(내구도 있음)는 쌓이지 않는다.
	TArray<FGrimItemInstance> Gear;
	Gear.SetNum(2);
	FGrimItemInstance Hood(TEXT("FurHood"), 1);
	Hood.Durability = 50.f;
	Hood.RepairCount = 1;
	TArray<FGrimItemInstance> GearSource = { Hood };
	TestEqual(TEXT("장비 옮기기"), FGrimSlotTransferMath::Transfer(GearSource, 0, -1, Gear, Items), 1);
	TestTrue(TEXT("내구도 · 수리 횟수를 지닌 채"), Gear[0].Durability == 50.f && Gear[0].RepairCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseCraftTest, "Grimoire.Base.Craft", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseCraftTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	const FGrimRecipeDefinition* Hood = Items.Recipes.FindByPredicate([](const FGrimRecipeDefinition& Recipe)
	{
		return Recipe.Id == TEXT("FurHood") && Recipe.Row.Station == TEXT("Workbench");
	});
	if (!TestNotNull(TEXT("작업대 레시피 FurHood"), Hood))
	{
		return false;
	}
	TArray<FGrimItemInstance> Slots;
	Slots.SetNum(3);
	Slots[0] = FGrimItemInstance(TEXT("JackalopeFur"), 2);
	TestFalse(TEXT("털 2개로는 못 만든다"), FGrimSlotTransferMath::HasInputs(Slots, Items, Hood->Inputs));
	Slots[2] = FGrimItemInstance(TEXT("JackalopeFur"), 2);
	TestTrue(TEXT("두 칸을 합쳐 4개면 된다"), FGrimSlotTransferMath::HasInputs(Slots, Items, Hood->Inputs));
	TestTrue(TEXT("재료를 뺀다"), FGrimSlotTransferMath::ConsumeInputs(Slots, Items, Hood->Inputs));
	TestEqual(TEXT("털 1개 남음"), FGrimInventoryModel::CountOf(Slots, TEXT("JackalopeFur")), 1);
	TestFalse(TEXT("모자라면 칸이 그대로"), FGrimSlotTransferMath::ConsumeInputs(Slots, Items, Hood->Inputs));
	TestEqual(TEXT("털 여전히 1개"), FGrimInventoryModel::CountOf(Slots, TEXT("JackalopeFur")), 1);

	// 묶음 재료(#Meat)는 ID 재료 다음에 채운다.
	TArray<FGrimRecipeInput> Inputs;
	TArray<FString> Problems;
	FGrimItemDatabase::ParseInputs(TEXT("#Meat:1|JackalopeMeat:1"), Inputs, Problems);
	TArray<FGrimItemInstance> Meat = { FGrimItemInstance(TEXT("JackalopeMeat"), 1), FGrimItemInstance(TEXT("RoastedJackalopeMeat"), 1) };
	TestTrue(TEXT("ID 재료가 날고기를 먼저 가져가고 묶음이 구운 고기를"), FGrimSlotTransferMath::ConsumeInputs(Meat, Items, Inputs));
	TestTrue(TEXT("둘 다 쓰였다"), Meat[0].IsEmpty() && Meat[1].IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseItemSerializeTest, "Grimoire.Base.Item.Serialize", GrimBaseTestsLocal::PureFlags)
bool FGrimBaseItemSerializeTest::RunTest(const FString& Parameters)
{
	auto RoundTrip = [](FGrimItemInstance Item, TArray<uint8>& OutBytes)
	{
		FMemoryWriter Writer(OutBytes);
		Item.Serialize(Writer);
		FGrimItemInstance Loaded;
		FMemoryReader Reader(OutBytes);
		Loaded.Serialize(Reader);
		return Loaded;
	};

	FGrimItemInstance Plain(TEXT("Wood"), 7);
	TArray<uint8> PlainBytes;
	TestTrue(TEXT("보통 아이템 되돌림"), RoundTrip(Plain, PlainBytes) == Plain);

	// 옛 형식과 같은 바이트(수리 횟수가 없으면 표시도 없다)
	TArray<uint8> Legacy;
	{
		FMemoryWriter Writer(Legacy);
		FString Id = TEXT("Wood");
		int32 Count = 7;
		float Durability = -1.f;
		int32 NumEnchantments = 0;
		Writer << Id << Count << Durability << NumEnchantments;
	}
	TestTrue(TEXT("수리 안 한 것은 옛 형식 그대로"), Legacy == PlainBytes);

	FGrimItemInstance Repaired(TEXT("HideRobe"), 1);
	Repaired.Durability = 33.f;
	Repaired.RepairCount = 3;
	Repaired.Enchantments.Add(TEXT("Sturdy"));
	TArray<uint8> RepairedBytes;
	const FGrimItemInstance Loaded = RoundTrip(Repaired, RepairedBytes);
	TestTrue(TEXT("수리 횟수 · 인챈트 · 내구도 되돌림"), Loaded == Repaired);
	TestFalse(TEXT("수리한 장비는 쌓이지 않는다"), Loaded.CanStackWith(Loaded));
	return true;
}

// ───────────────────────── 월드: 지붕 · 무너짐 · 저장고 ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseShelterTest, "Grimoire.Base.Shelter", GrimBaseTestsLocal::WorldFlags)
bool FGrimBaseShelterTest::RunTest(const FString& Parameters)
{
	using namespace GrimBaseTestsLocal;
	FBaseTestWorld Test;
	UWorld* World = Test.World;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimBaseCore* Core = World->SpawnActor<AGrimBaseCore>(FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(World);
	const UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
	if (!TestNotNull(TEXT("거점 핵"), Core) || !TestNotNull(TEXT("거점 서브시스템"), Subsystem) || !TestNotNull(TEXT("날씨 서브시스템"), Weather))
	{
		return false;
	}
	TestEqual(TEXT("거점 핵 등록"), Subsystem->FindBaseAt(FVector(500.f, 0.f, 0.f)), Core);
	TestNull(TEXT("반경 밖은 거점 아님"), Subsystem->FindBaseAt(FVector(UGrimBaseSettings::Get()->BaseRadius + 100.f, 0.f, 0.f)));

	// 칸 (2,0) 에 집 한 칸: 바닥 · 벽 · 지붕.
	AGrimBuildPiece* Floor = Test.SpawnPiece(Core, TEXT("WoodFoundation"), MakeSlot(EGrimBuildCategory::Foundation, 2, 0));
	AGrimBuildPiece* Wall = Test.SpawnPiece(Core, TEXT("WoodWall"), MakeSlot(EGrimBuildCategory::Wall, 2, 0, 0, 0));
	AGrimBuildPiece* Roof = Test.SpawnPiece(Core, TEXT("WoodRoof"), MakeSlot(EGrimBuildCategory::Roof, 2, 0, 0));
	if (!TestNotNull(TEXT("바닥"), Floor) || !TestNotNull(TEXT("벽"), Wall) || !TestNotNull(TEXT("지붕"), Roof))
	{
		return false;
	}
	TestEqual(TEXT("조각 셋이 이 거점 것"), Subsystem->GetPieces(Core).Num(), 3);
	TestEqual(TEXT("조각 ID 복제 값"), Roof->GetPieceId(), FName(TEXT("WoodRoof")));
	TestEqual(TEXT("체력 = 표의 최대"), Roof->GetStructure()->GetHealth(), Roof->GetStructure()->GetMaxHealth(), 0.01f);

	const FGrimBuildGrid Grid = Core->GetGrid();
	const FVector Inside = Grid.LocalToWorld(FVector(600.f, 0.f, Grid.GetFloorTop(0) + 190.f));
	const FVector Outside = Grid.LocalToWorld(FVector(1200.f, 0.f, Grid.GetFloorTop(0) + 190.f));
	TestTrue(TEXT("지붕 아래는 비를 막는다"), Weather->IsSheltered(Inside));
	TestFalse(TEXT("지붕 없는 곳은 막지 못한다"), Weather->IsSheltered(Outside));

	// 지붕 아래 모닥불도 지붕 아래다(비에 안 꺼짐).
	AGrimBaseCampfire* Fire = World->SpawnActor<AGrimBaseCampfire>(Grid.LocalToWorld(FVector(600.f, 60.f, Grid.GetFloorTop(0))), FRotator::ZeroRotator, Params);
	TestTrue(TEXT("집 안 모닥불은 지붕 아래"), Fire && Weather->IsActorSheltered(Fire));

	// 바닥을 부수면 받침 없는 벽 · 지붕이 무너진다.
	Floor->GetStructure()->ApplyStructureDamage(100000.f, TEXT("시험"));
	TestFalse(TEXT("바닥이 부서졌다"), IsValid(Floor));
	TestEqual(TEXT("받침 없는 둘이 무너진다"), UGrimBaseSubsystem::Get(World)->CollapseUnsupported(), 2);
	TestEqual(TEXT("조각이 남지 않는다"), Subsystem->GetPieces(Core).Num(), 0);
	TestFalse(TEXT("지붕이 없으니 비를 맞는다"), Weather->IsSheltered(Inside, Fire));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseStructureTest, "Grimoire.Base.Structure", GrimBaseTestsLocal::WorldFlags)
bool FGrimBaseStructureTest::RunTest(const FString& Parameters)
{
	using namespace GrimBaseTestsLocal;
	FBaseTestWorld Test;
	UWorld* World = Test.World;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimBaseCore* Core = World->SpawnActor<AGrimBaseCore>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	AGrimBuildPiece* Wall = Test.SpawnPiece(Core, TEXT("StoneFoundation"), MakeSlot(EGrimBuildCategory::Foundation, 0, 0));
	UGrimStructureComponent* Structure = Wall ? Wall->GetStructure() : nullptr;
	if (!TestNotNull(TEXT("돌 바닥"), Structure))
	{
		return false;
	}
	const float Max = Structure->GetMaxHealth();
	Structure->ApplyStructureDamage(Max * 0.6f, TEXT("시험"));
	TestEqual(TEXT("피해 60%"), Structure->GetHealth(), Max * 0.4f, 0.01f);
	TestEqual(TEXT("재료 3개면 가득(25% × 3 ≥ 60%)"), Structure->GetMaterialsNeeded(), 3);
	Structure->ApplyRepairMaterials(2);
	TestEqual(TEXT("재료 2개 = 50% 회복"), Structure->GetHealth(), Max * 0.9f, 0.01f);
	TestEqual(TEXT("잔해 = 비용 절반"), Structure->GetRefundItems(0.5f).Num(), 1);
	TestNotNull(TEXT("거점 안"), Structure->FindBaseCore());

	// 세이브 되돌림(구조 · 조각 자리)
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes);
		Structure->WriteSaveData(Writer);
	}
	Structure->SetHealth(1.f);
	{
		FMemoryReader Reader(Bytes);
		Structure->ReadSaveData(Reader, 1);
	}
	TestEqual(TEXT("세이브에서 체력 되돌림"), Structure->GetHealth(), Max * 0.9f, 0.01f);

	// 저장고: 칸 · 세이브
	AGrimStorageChest* Chest = World->SpawnActor<AGrimStorageChest>(FVector(100.f, 0.f, 50.f), FRotator::ZeroRotator, Params);
	UGrimItemContainerComponent* Container = Chest ? Chest->GetContainer() : nullptr;
	if (TestNotNull(TEXT("저장고"), Container))
	{
		TestEqual(TEXT("저장고 칸 수"), Container->GetSlotCount(), UGrimBaseSettings::Get()->StorageSlotCount);
		TestEqual(TEXT("넣기"), Container->AddItem(FGrimItemInstance(TEXT("Stone"), 12)), 12);
		TArray<uint8> ChestBytes;
		{
			FMemoryWriter Writer(ChestBytes);
			Container->WriteSaveData(Writer);
		}
		Container->RemoveItem(TEXT("Stone"), 12);
		{
			FMemoryReader Reader(ChestBytes);
			Container->ReadSaveData(Reader, 1);
		}
		TestEqual(TEXT("저장고 세이브 되돌림"), Container->CountOf(TEXT("Stone")), 12);
	}

	// 밭: 심기 · 자라기 · 거두기
	AGrimFarmPlot* Plot = World->SpawnActor<AGrimFarmPlot>(FVector(400.f, 0.f, 50.f), FRotator::ZeroRotator, Params);
	if (TestNotNull(TEXT("밭"), Plot))
	{
		TestTrue(TEXT("곡물 심기"), Plot->Plant(TEXT("Grain")));
		TestFalse(TEXT("이미 심은 밭에는 못 심는다"), Plot->Plant(TEXT("Managrass")));
		Plot->AdvanceGrowth(100000.f);
		TestEqual(TEXT("마른 밭은 자라지 않는다"), Plot->GetGrowth(), 0.f);
		TestEqual(TEXT("다 자라기 전엔 못 거둔다"), Plot->Harvest().Num(), 0);
	}
	return true;
}

#endif
