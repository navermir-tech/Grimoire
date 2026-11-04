#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalObject.h"
#include "Interaction/GrimElementalPropagationSubsystem.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionSettings.h"
#include "Interaction/GrimInteractionTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/Paths.h"

namespace GrimInteractionTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGrimElementalStimulus Stim(const FGameplayTag& Element, float Intensity)
	{
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = Element;
		Stimulus.Intensity = Intensity;
		return Stimulus;
	}

	/** 저장소의 규칙표 CSV 를 그대로 읽는다(게임과 같은 원본) */
	const UDataTable* LoadRepoTable(FAutomationTestBase& Test)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), GetDefault<UGrimInteractionSettings>()->ReactionTableCSV);
		TArray<FString> Problems;
		static TStrongObjectPtr<UDataTable> Cached;
		if (!Cached)
		{
			Cached.Reset(FGrimElementalReactionLibrary::LoadTableFromCSVFile(Path, Problems));
			for (const FString& Problem : Problems)
			{
				Test.AddError(FString::Printf(TEXT("CSV 문제: %s"), *Problem));
			}
		}
		return Cached.Get();
	}

	/** 순수 로직용 사물: 재질 + 상태 배열 */
	struct FTestObject
	{
		const UDataTable* Table = nullptr;
		FGameplayTagContainer Material;
		FGrimElementalStateArray States;

		void Hit(const FGameplayTag& Element, float Intensity)
		{
			TArray<FGrimElementalStateChange> Changes;
			FGrimElementalReactionLibrary::ApplyStimulus(Table, States, Material, Stim(Element, Intensity), 1000.f, Changes);
		}

		void Wait(float Seconds)
		{
			TArray<FGrimElementalStateChange> Changes;
			FGrimElementalReactionLibrary::AdvanceTime(Table, States, Material, Seconds, 1000.f, Changes);
		}

		float Burning() const { return States.GetIntensity(GrimTags::State_Burning); }
		float Wet() const { return States.GetIntensity(GrimTags::State_Wet); }
	};

	/** 자동화 테스트용 게임 월드. 소멸자가 정리한다 */
	struct FScopedGameWorld
	{
		UWorld* World = nullptr;

		FScopedGameWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimInteractionTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			// 게임 모드가 없으니 액터 BeginPlay 를 직접 연다.
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedGameWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		template <typename T>
		T* Spawn(const FVector& Location)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<T>(Location, FRotator::ZeroRotator, Params);
		}
	};
}

using namespace GrimInteractionTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInteractionReactionTableTest, "Grimoire.Interaction.ReactionTable", Flags)
bool FGrimInteractionReactionTableTest::RunTest(const FString& Parameters)
{
	const UDataTable* Table = LoadRepoTable(*this);
	if (!TestNotNull(TEXT("규칙표 CSV 를 읽는다"), Table))
	{
		return false;
	}
	TestTrue(TEXT("규칙 10행 이상"), Table->GetRowMap().Num() >= 10);

	// 모든 Stimulus 행은 속성이 있고, 결과가 하나는 있다.
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FGrimElementalReactionRow& Row = *reinterpret_cast<const FGrimElementalReactionRow*>(Pair.Value);
		if (Row.Trigger == EGrimReactionTrigger::Stimulus)
		{
			TestTrue(FString::Printf(TEXT("%s: 속성이 있다"), *Pair.Key.ToString()), Row.Element.IsValid());
		}
		TestTrue(FString::Printf(TEXT("%s: 결과가 있다"), *Pair.Key.ToString()), Row.ResultState.IsValid() || Row.ClearStates.Num() > 0);
	}

	const FGrimElementalReactionRow* Grass = Table->FindRow<FGrimElementalReactionRow>(TEXT("Grass_Ignite"), TEXT("test"));
	if (TestNotNull(TEXT("Grass_Ignite 행"), Grass))
	{
		TestEqual(TEXT("배열 칸(BlockedByStates) 파싱"), Grass->BlockedByStates.Num(), 3);
		TestTrue(TEXT("구조체 칸(Propagate) 파싱"), Grass->Propagate.IsActive());
		TestEqual(TEXT("전파 반경"), Grass->Propagate.Radius, 250.f);
		TestEqual(TEXT("전파 대상 재질"), Grass->Propagate.TargetMaterial, GrimInteractionTags::Material_Flammable.GetTag());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInteractionCandleExampleTest, "Grimoire.Interaction.CandleExample", Flags)
bool FGrimInteractionCandleExampleTest::RunTest(const FString& Parameters)
{
	FTestObject Candle;
	Candle.Table = LoadRepoTable(*this);
	Candle.Material.AddTag(GrimInteractionTags::Material_Flammable);
	if (!Candle.Table)
	{
		return false;
	}

	// 꺼짐 + 약한 불 → 그대로
	Candle.Hit(GrimTags::Element_Fire, 0.3f);
	TestEqual(TEXT("약한 불(0.3)로는 안 켜진다"), Candle.Burning(), 0.f);

	// 꺼짐 + 불 → 켜짐(불꽃 1)
	Candle.Hit(GrimTags::Element_Fire, 1.f);
	TestEqual(TEXT("불 → 켜짐(불꽃 1)"), Candle.Burning(), 1.f);

	// 켜짐 + 바람 → 커짐(+1, 최대 3)
	Candle.Hit(GrimTags::Element_Wind, 1.f);
	TestEqual(TEXT("바람 → 불꽃 2"), Candle.Burning(), 2.f);
	Candle.Hit(GrimTags::Element_Wind, 1.f);
	Candle.Hit(GrimTags::Element_Wind, 1.f);
	TestEqual(TEXT("바람 여러 번 → 최대 3"), Candle.Burning(), 3.f);

	// 켜짐 + 어둠 → 작아짐
	Candle.Hit(GrimTags::Element_Dark, 1.f);
	TestEqual(TEXT("어둠 → 불꽃 -1"), Candle.Burning(), 2.f);

	// 켜짐 + 돌풍 → 꺼짐
	Candle.Hit(GrimTags::Element_Wind, 3.f);
	TestEqual(TEXT("돌풍(3.0) → 꺼짐"), Candle.Burning(), 0.f);
	TestFalse(TEXT("돌풍으로는 젖지 않는다"), Candle.States.Has(GrimTags::State_Wet));

	// 켜짐 + 물 → 꺼짐 + 젖음
	Candle.Hit(GrimTags::Element_Fire, 1.f);
	TestEqual(TEXT("다시 켜짐"), Candle.Burning(), 1.f);
	Candle.Hit(GrimTags::Element_Water, 1.f);
	TestEqual(TEXT("물 → 꺼짐"), Candle.Burning(), 0.f);
	TestEqual(TEXT("물 → 젖음 1"), Candle.Wet(), 1.f);

	// 젖음 + 불 → 불이 안 붙는다
	Candle.Hit(GrimTags::Element_Fire, 0.6f);
	TestEqual(TEXT("젖으면 불(0.6)이 안 붙는다"), Candle.Burning(), 0.f);
	TestEqual(TEXT("약한 불은 말리지도 못한다"), Candle.Wet(), 1.f);

	// 젖음 + 센 불 → 마름(같은 자극으로 켜지지는 않음) → 다음 불에 켜짐
	Candle.Hit(GrimTags::Element_Fire, 1.f);
	TestFalse(TEXT("센 불 → 마름"), Candle.States.Has(GrimTags::State_Wet));
	TestEqual(TEXT("말리는 불로는 아직 안 켜진다"), Candle.Burning(), 0.f);
	Candle.Hit(GrimTags::Element_Fire, 1.f);
	TestEqual(TEXT("마른 뒤 불 → 켜짐"), Candle.Burning(), 1.f);

	// 젖음 + 바람 → 마름
	Candle.Hit(GrimTags::Element_Water, 1.f);
	Candle.Hit(GrimTags::Element_Wind, 1.f);
	TestFalse(TEXT("바람 → 마름"), Candle.States.Has(GrimTags::State_Wet));

	// 젖음 + 시간 → 마름
	Candle.Hit(GrimTags::Element_Water, 1.f);
	Candle.Hit(GrimTags::Element_Water, 1.f);
	TestEqual(TEXT("물 두 번 → 젖음 2"), Candle.Wet(), 2.f);
	Candle.Wait(10.f);
	TestEqual(TEXT("10초 뒤에는 아직 젖음"), Candle.Wet(), 2.f);
	Candle.Wait(10.5f);
	TestFalse(TEXT("20초 뒤 → 마름"), Candle.States.Has(GrimTags::State_Wet));

	// 불이 붙지 않는 재질(돌)은 불에 반응하지 않는다
	FTestObject Stone;
	Stone.Table = Candle.Table;
	Stone.Hit(GrimTags::Element_Fire, 2.f);
	TestEqual(TEXT("돌에는 불이 안 붙는다"), Stone.Burning(), 0.f);
	Stone.Hit(GrimTags::Element_Water, 1.f);
	TestEqual(TEXT("돌도 젖는다"), Stone.Wet(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInteractionDryGrassTest, "Grimoire.Interaction.DryGrassBurnsOut", Flags)
bool FGrimInteractionDryGrassTest::RunTest(const FString& Parameters)
{
	FTestObject Grass;
	Grass.Table = LoadRepoTable(*this);
	Grass.Material.AddTag(GrimInteractionTags::Material_Flammable);
	Grass.Material.AddTag(GrimInteractionTags::Material_Consumable);
	if (!Grass.Table)
	{
		return false;
	}

	Grass.Hit(GrimTags::Element_Fire, 1.f);
	TestEqual(TEXT("마른 풀에 불"), Grass.Burning(), 1.f);
	const FGrimElementalState* Burning = Grass.States.Find(GrimTags::State_Burning);
	if (TestNotNull(TEXT("불붙은 상태"), Burning))
	{
		TestTrue(TEXT("풀 불은 시간 제한"), Burning->IsTimed());
		TestTrue(TEXT("풀 불은 번진다"), Burning->Propagation.IsActive());
	}

	Grass.Hit(GrimTags::Element_Fire, 1.f);
	Grass.Wait(9.f);
	TestEqual(TEXT("다 타면 불이 꺼진다"), Grass.Burning(), 0.f);
	TestTrue(TEXT("다 타면 탄 상태"), Grass.States.Has(GrimInteractionTags::State_Charred));

	Grass.Hit(GrimTags::Element_Fire, 2.f);
	TestEqual(TEXT("탄 풀은 다시 안 탄다"), Grass.Burning(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInteractionComponentTest, "Grimoire.Interaction.Component", Flags)
bool FGrimInteractionComponentTest::RunTest(const FString& Parameters)
{
	FScopedGameWorld TestWorld;
	AGrimCandle* Candle = TestWorld.Spawn<AGrimCandle>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("양초 스폰"), Candle))
	{
		return false;
	}
	UGrimElementalStateComponent* State = Candle->GetElementalState();

	TestTrue(TEXT("양초는 휴면으로 시작"), Candle->NetDormancy == DORM_DormantAll);
	TestFalse(TEXT("컴포넌트는 틱하지 않는다"), State->IsComponentTickEnabled());
	TestFalse(TEXT("상태가 없으면 타이머도 없다"), State->IsStateTimerActive());
	TestTrue(TEXT("기반 인터페이스로 찾힌다"), UGrimElementalStatics::FindReceiver(Candle) == State);

	int32 NumChanges = 0;
	State->OnStateChangedNative.AddLambda([&NumChanges](UGrimElementalStateComponent*, FGameplayTag, float, float) { ++NumChanges; });

	UGrimElementalStatics::ApplyStimulusToActor(Candle, Stim(GrimTags::Element_Fire, 1.f), nullptr);
	TestEqual(TEXT("기반 함수로 불 → 켜짐"), UGrimElementalStatics::GetStateIntensity(Candle, GrimTags::State_Burning), 1.f);
	TestEqual(TEXT("변화 델리게이트 1번"), NumChanges, 1);
	TestFalse(TEXT("무한 상태만 있으면 타이머 없음"), State->IsStateTimerActive());
	TestTrue(TEXT("불붙은 양초는 전파 출처"), State->HasActivePropagation());

	UGrimElementalStatics::ApplyStimulusToActor(Candle, Stim(GrimTags::Element_Water, 1.f), nullptr);
	TestEqual(TEXT("물 → 꺼짐 + 젖음: 변화 2번 더"), NumChanges, 3);
	TestTrue(TEXT("젖음(시간 제한)이 있으면 타이머"), State->IsStateTimerActive());
	TestFalse(TEXT("꺼지면 전파 출처가 아니다"), State->HasActivePropagation());

	State->AdvanceTime(30.f);
	TestFalse(TEXT("시간 → 마름"), State->HasState(GrimTags::State_Wet));
	TestFalse(TEXT("상태가 없어지면 타이머도 멈춘다"), State->IsStateTimerActive());
	TestEqual(TEXT("마름도 델리게이트"), NumChanges, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInteractionPropagationTest, "Grimoire.Interaction.Propagation", Flags)
bool FGrimInteractionPropagationTest::RunTest(const FString& Parameters)
{
	FScopedGameWorld TestWorld;
	UGrimElementalPropagationSubsystem* Propagation = TestWorld.World->GetSubsystem<UGrimElementalPropagationSubsystem>();
	if (!TestNotNull(TEXT("전파 서브시스템"), Propagation))
	{
		return false;
	}

	AGrimDryGrass* Source = TestWorld.Spawn<AGrimDryGrass>(FVector(0.f, 0.f, 0.f));
	AGrimDryGrass* Near = TestWorld.Spawn<AGrimDryGrass>(FVector(150.f, 0.f, 0.f));
	AGrimDryGrass* WetNear = TestWorld.Spawn<AGrimDryGrass>(FVector(-150.f, 0.f, 0.f));
	AGrimCandle* FarCandle = TestWorld.Spawn<AGrimCandle>(FVector(2000.f, 0.f, 0.f));
	if (!Source || !Near || !WetNear || !FarCandle)
	{
		AddError(TEXT("스폰 실패"));
		return false;
	}

	UGrimElementalStatics::ApplyStimulusToActor(WetNear, Stim(GrimTags::Element_Water, 1.f), nullptr);
	UGrimElementalStatics::ApplyStimulusToActor(Source, Stim(GrimTags::Element_Fire, 1.f), nullptr);
	TestEqual(TEXT("불붙은 풀 하나가 출처로 등록"), Propagation->GetNumSources(), 1);

	TestEqual(TEXT("간격 전에는 번지지 않는다"), Propagation->ProcessDueSources(TestWorld.World->GetTimeSeconds()), 0);
	TestEqual(TEXT("아직 이웃은 안 탄다"), UGrimElementalStatics::GetStateIntensity(Near, GrimTags::State_Burning), 0.f);

	const double Later = TestWorld.World->GetTimeSeconds() + 100.0;
	TestEqual(TEXT("출처 하나 = 쿼리 하나"), Propagation->ProcessDueSources(Later), 1);
	TestEqual(TEXT("가까운 마른 풀에 번진다"), UGrimElementalStatics::GetStateIntensity(Near, GrimTags::State_Burning), 1.f);
	TestEqual(TEXT("젖은 풀에는 안 번진다"), UGrimElementalStatics::GetStateIntensity(WetNear, GrimTags::State_Burning), 0.f);
	TestEqual(TEXT("먼 양초에는 안 번진다"), UGrimElementalStatics::GetStateIntensity(FarCandle, GrimTags::State_Burning), 0.f);
	TestEqual(TEXT("번진 풀도 출처가 된다"), Propagation->GetNumSources(), 2);

	// 다 타면 출처에서 빠진다
	Source->GetElementalState()->AdvanceTime(10.f);
	Near->GetElementalState()->AdvanceTime(10.f);
	TestTrue(TEXT("다 탄 풀"), Source->GetElementalState()->HasState(GrimInteractionTags::State_Charred));
	Propagation->ProcessDueSources(Later + 100.0);
	TestEqual(TEXT("다 타면 출처가 없다"), Propagation->GetNumSources(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
