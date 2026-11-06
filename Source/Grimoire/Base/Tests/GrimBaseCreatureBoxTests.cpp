#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimStructureComponent.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureStorageTab.h"
#include "Items/GrimItemSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

// M4 거점 × R38 생물 보관이 만나는 자리: 소지품 「생물」 탭 등록, 건축 시설 「생물 상자」, 지은 상자가 없어질 때 생물 옮기기.

namespace GrimBaseCreatureBoxTest // 유니티 빌드 이름 충돌 방지
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FScopedWorld
	{
		UWorld* World = nullptr;

		FScopedWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimBaseCreatureBoxTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

using namespace GrimBaseCreatureBoxTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseCreatureBoxDataTest, "Grimoire.Base.CreatureBox.Data", PureFlags)
bool FGrimBaseCreatureBoxDataTest::RunTest(const FString& Parameters)
{
	// 생물 탭: ini 한 줄로 등록, 장비(10) 다음 · 시설 탭(20~) 앞
	const FSoftClassPath TabPath(UGrimCreatureStorageTab::StaticClass());
	TestTrue(TEXT("ini InventoryTabClasses 에 생물 탭"), UGrimItemSettings::Get()->InventoryTabClasses.Contains(TabPath));
	const UGrimCreatureStorageTab* Tab = GetDefault<UGrimCreatureStorageTab>();
	TestEqual(TEXT("생물 탭 ID"), Tab->GetTabId(), FName(TEXT("Creatures")));
	TestTrue(TEXT("생물 탭 순서: 장비 다음 · 저장고 앞"), Tab->GetOrder() > 10 && Tab->GetOrder() < 20);

	// 건축 시설 「생물 상자」
	const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(TEXT("CreatureBox"));
	if (!TestNotNull(TEXT("조각 표에 CreatureBox"), Def))
	{
		return false;
	}
	TestTrue(TEXT("시설"), Def->Row.Category == EGrimBuildCategory::Station);
	UClass* Class = LoadClass<AActor>(nullptr, *Def->Row.SpawnClass);
	TestTrue(TEXT("스폰 클래스 = 거점 생물 상자의 지은 판"), Class && Class->IsChildOf(AGrimBaseCreatureBox::StaticClass()));
	TestNotNull(TEXT("지은 상자에 구조 컴포넌트(내구도)"), Class ? UGrimStructureComponent::FindOn(Class->GetDefaultObject<AActor>()) : nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBaseCreatureBoxRescueTest, "Grimoire.Base.CreatureBox.Rescue", WorldFlags)
bool FGrimBaseCreatureBoxRescueTest::RunTest(const FString& Parameters)
{
	FScopedWorld TestWorld;
	UWorld* World = TestWorld.World;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimBaseCreatureBox* Built = World->SpawnActor<AGrimBaseCreatureBox>(AGrimBaseCreatureBox::StaticClass(), FTransform(FVector(0.f, 0.f, 0.f)), Params);
	AGrimCreatureBox* Other = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), FTransform(FVector(2000.f, 0.f, 0.f)), Params);
	if (!TestNotNull(TEXT("지은 상자"), Built) || !TestNotNull(TEXT("다른 상자"), Other))
	{
		return false;
	}

	FGrimOwnedCreatureRecord Record;
	Record.Instance.SpeciesId = TEXT("Jackalope");
	Record.Instance.Seed = 9301;
	Record.Instance.OwnerId = TEXT("PS-9301");
	Record.Instance.Nickname = TEXT("구름");
	TestTrue(TEXT("지은 상자에 넣음"), Built->AddEntry(Record, false));
	Record.Instance.Seed = 9302;
	TestTrue(TEXT("하나 더"), Built->AddEntry(Record, false));

	Built->Destroy();
	TestEqual(TEXT("부서진 상자의 생물 → 다른 상자로"), Other->GetEntries().Num(), 2);
	const bool bKept = Other->GetEntries().ContainsByPredicate([](const FGrimCreatureBoxEntry& Entry)
	{
		return Entry.Record.Instance.Seed == 9301 && Entry.Record.Instance.Nickname == TEXT("구름") && !Entry.bReleased;
	});
	TestTrue(TEXT("기록 그대로(이름 · 씨앗), 상자 안"), bKept);
	return true;
}

#endif
