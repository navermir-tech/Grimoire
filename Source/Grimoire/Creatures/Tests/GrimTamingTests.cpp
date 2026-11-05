#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTags.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingTags.h"
#include "Items/GrimItemSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimPactumProjectile.h"
#include "Magic/GrimSpellDefinition.h"
#include "Quest/GrimQuestSettings.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimTamingTest // 유니티 빌드 이름 충돌 방지
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 시험용 충성도 수치(ini 에 기대지 않는다) */
	FGrimLoyaltyTuning MakeLoyaltyTuning()
	{
		FGrimLoyaltyTuning Tuning;
		Tuning.InitialLoyalty = 50.f;
		Tuning.RecentReasonCount = 3;
		auto Delta = [&Tuning](EGrimLoyaltyEvent Event, float Value)
		{
			FGrimLoyaltyEventDelta Row;
			Row.Event = Event;
			Row.Delta = Value;
			Tuning.EventDeltas.Add(Row);
		};
		Delta(EGrimLoyaltyEvent::Fed, 3.f);
		Delta(EGrimLoyaltyEvent::FedFavorite, 6.f);
		Delta(EGrimLoyaltyEvent::Rested, 2.f);
		Delta(EGrimLoyaltyEvent::Starved, -5.f);
		Delta(EGrimLoyaltyEvent::KnockedOut, -8.f);
		Delta(EGrimLoyaltyEvent::Overworked, -4.f);
		auto Stage = [&Tuning](EGrimLoyaltyStage InStage, float Min, float Obedience, float Speed, float Refuse, float Buck)
		{
			FGrimLoyaltyStageTuning Row;
			Row.Stage = InStage;
			Row.MinLoyalty = Min;
			Row.Obedience = Obedience;
			Row.MountSpeedScale = Speed;
			Row.MountRefuseChance = Refuse;
			Row.BuckChance = Buck;
			Tuning.Stages.Add(Row);
		};
		Stage(EGrimLoyaltyStage::Devoted, 80.f, 1.f, 1.15f, 0.f, 0.f);
		Stage(EGrimLoyaltyStage::Normal, 50.f, 1.f, 1.f, 0.f, 0.f);
		Stage(EGrimLoyaltyStage::Discontent, 20.f, 0.75f, 0.9f, 0.1f, 0.f);
		Stage(EGrimLoyaltyStage::Rebellious, 1.f, 0.4f, 0.8f, 0.6f, 0.25f);
		return Tuning;
	}

	FGrimCaptureTuning MakeCaptureTuning()
	{
		FGrimCaptureTuning Tuning; // 기본값 = ini 와 같은 제안값
		return Tuning;
	}

	FGrimOwnedCreatureRecord MakeRecord(int32 Seed)
	{
		FGrimOwnedCreatureRecord Record;
		Record.Instance.SpeciesId = TEXT("Kelpie");
		Record.Instance.Seed = Seed;
		Record.Instance.Sex = EGrimCreatureSex::Male;
		FGrimCreatureRegionColor A;
		A.PaletteIndex = 2;
		A.Color = FLinearColor(0.1f, 0.2f, 0.3f, 1.f);
		FGrimCreatureRegionColor B;
		B.PaletteIndex = 1;
		B.bMutant = true;
		B.Color = FLinearColor(0.9f, 0.1f, 0.5f, 1.f);
		Record.Instance.Colors = { A, B };
		Record.Instance.SizeScale = 1.37f;
		Record.Instance.bGiant = true;
		Record.Instance.bMutant = true;
		Record.Instance.Personality = EGrimCreaturePersonality::Brawler;
		Record.Instance.StatScale = 1.6f;
		Record.Instance.OwnerId = TEXT("host-abc-PIE1");
		Record.Instance.Loyalty = 72.5f;
		Record.Instance.Nickname = TEXT("물결이");
		Record.HealthRatio = 0.4f;
		Record.Command = EGrimTamedCommand::Wait;
		FGrimLoyaltyReason R1;
		R1.Event = EGrimLoyaltyEvent::FedFavorite;
		R1.Delta = 6.f;
		FGrimLoyaltyReason R2;
		R2.Event = EGrimLoyaltyEvent::Starved;
		R2.Delta = -5.f;
		Record.Reasons = { R1, R2 };
		Record.SavedUtcSeconds = 1.7e9;
		return Record;
	}

	struct FScopedWorld
	{
		UWorld* World = nullptr;

		FScopedWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimTamingTestWorld"));
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

using namespace GrimTamingTest;

// 포획 확률: 몫마다 더하고 등급을 빼고 자른다(스펙 「길들이기 — 확률에 더해지는 것」).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingChanceTest, "Grimoire.Taming.Chance", PureFlags)
bool FGrimTamingChanceTest::RunTest(const FString& Parameters)
{
	const FGrimCaptureTuning Tuning = MakeCaptureTuning();
	FGrimCaptureInput Input;
	Input.SizeClass = EGrimCreatureSizeClass::Small;

	FGrimCaptureChance C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("체력 가득 · 소형 · 1서클 = 기본만"), C.Total, 0.3f, 1e-4f);
	TestEqual(TEXT("약해짐 몫 없음"), C.Weakened, 0.f, 1e-5f);

	Input.HealthRatio = 0.3f;
	C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("문턱(30%) = 약해짐 몫만"), C.Weakened, 0.2f, 1e-4f);
	Input.HealthRatio = 0.1f;
	C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("10% = 약해짐 + 깊이 2/3"), C.Weakened, 0.2f + 0.15f * (2.f / 3.f), 1e-4f);
	const float Weak = C.Total;

	Input.bElementMatch = true;
	C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("속성 일치(파크툼 + 속성)"), C.Total, Weak + 0.25f, 1e-4f);

	Input.bFed = true;
	C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("먹이"), C.Food, 0.1f, 1e-5f);
	Input.bFedFavorite = true;
	C = GrimTamingMath::CalcCaptureChance(Input, Tuning, 0.3f);
	TestEqual(TEXT("좋아하는 먹이는 더 큰 몫 하나"), C.Food, 0.2f, 1e-5f);
	TestEqual(TEXT("상한 0.95"), C.Total, 0.95f, 1e-4f);

	// 완드 · 서클 · 숙련도 · 잠 · 지침
	FGrimCaptureInput B;
	B.WandElementPower = 1.2f;
	B.CasterCircle = 9;
	B.RequiredCircle = 1;
	B.ProficiencyBonus = 0.07f;
	B.bAsleep = true;
	B.bExhausted = true;
	C = GrimTamingMath::CalcCaptureChance(B, Tuning, 0.3f);
	TestEqual(TEXT("완드 친화 (1.2 − 1) × 0.5"), C.Wand, 0.1f, 1e-4f);
	TestEqual(TEXT("서클 차 상한 0.15"), C.Circle, 0.15f, 1e-4f);
	TestEqual(TEXT("숙련도 추가 효과"), C.Proficiency, 0.07f, 1e-5f);
	TestEqual(TEXT("잠듦"), C.Asleep, 0.15f, 1e-5f);
	TestEqual(TEXT("지침"), C.Exhausted, 0.1f, 1e-5f);
	B.WandElementPower = 3.f;
	TestEqual(TEXT("완드 친화 상한"), GrimTamingMath::CalcCaptureChance(B, Tuning, 0.3f).Wand, 0.15f, 1e-4f);

	// 등급
	FGrimCaptureInput D;
	D.SizeClass = EGrimCreatureSizeClass::Medium;
	D.CasterCircle = 2;
	D.RequiredCircle = 2;
	TestEqual(TEXT("중형 −0.10"), GrimTamingMath::CalcCaptureChance(D, Tuning, 0.3f).Total, 0.2f, 1e-4f);
	D.bGiant = true;
	TestEqual(TEXT("거대 −0.10 더"), GrimTamingMath::CalcCaptureChance(D, Tuning, 0.3f).Total, 0.1f, 1e-4f);
	D.SizeClass = EGrimCreatureSizeClass::Legendary;
	TestEqual(TEXT("하한 0.02"), GrimTamingMath::CalcCaptureChance(D, Tuning, 0.3f).Total, 0.02f, 1e-4f);

	// 서클 모자람
	FGrimCaptureInput E;
	E.CasterCircle = 1;
	E.RequiredCircle = 3;
	C = GrimTamingMath::CalcCaptureChance(E, Tuning, 0.3f);
	TestTrue(TEXT("요구 서클 미만 → 시도 불가"), C.bCircleTooLow);
	TestEqual(TEXT("확률 0"), C.Total, 0.f, 1e-5f);
	FGrimCaptureTuning Lenient = Tuning;
	Lenient.MaxCircleDeficit = 2;
	C = GrimTamingMath::CalcCaptureChance(E, Lenient, 0.3f);
	TestFalse(TEXT("여유 2단계면 시도할 수 있다"), C.bCircleTooLow);
	TestEqual(TEXT("모자란 단계마다 −0.2"), C.Circle, -0.4f, 1e-4f);

	TestTrue(TEXT("굴림 < 확률 = 성공"), GrimTamingMath::RollCapture(0.55f, 0.54f));
	TestFalse(TEXT("굴림 ≥ 확률 = 실패"), GrimTamingMath::RollCapture(0.55f, 0.55f));
	TestFalse(TEXT("확률 0 은 굴림 0 이어도 실패"), GrimTamingMath::RollCapture(0.f, 0.f));
	return true;
}

// 충성도: 사건 → 0~100, 단계, 최근 까닭 세 줄.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingLoyaltyTest, "Grimoire.Taming.Loyalty", PureFlags)
bool FGrimTamingLoyaltyTest::RunTest(const FString& Parameters)
{
	const FGrimLoyaltyTuning Tuning = MakeLoyaltyTuning();
	float Loyalty = Tuning.InitialLoyalty;
	TArray<FGrimLoyaltyReason> Reasons;
	TestEqual(TEXT("길들인 직후 보통"), GrimTamingMath::GetStage(Loyalty, Tuning), EGrimLoyaltyStage::Normal);

	TestEqual(TEXT("먹이 +3"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Fed, Tuning, nullptr), 3.f, 1e-4f);
	TestEqual(TEXT("53"), Loyalty, 53.f, 1e-4f);
	GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::FedFavorite, Tuning, nullptr);
	TestEqual(TEXT("좋아하는 먹이는 두 배 +6"), Loyalty, 59.f, 1e-4f);
	GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Starved, Tuning, nullptr);
	GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Rested, Tuning, nullptr);
	TestEqual(TEXT("최근 까닭은 세 줄"), Reasons.Num(), 3);
	TestEqual(TEXT("맨 앞 = 마지막 사건"), Reasons[0].Event, EGrimLoyaltyEvent::Rested);
	TestEqual(TEXT("두째 = 굶김 −5"), Reasons[1].Delta, -5.f, 1e-4f);
	TestEqual(TEXT("셋째 = 좋아하는 먹이"), Reasons[2].Event, EGrimLoyaltyEvent::FedFavorite);
	TestEqual(TEXT("모르는(변화량 0) 사건은 아무 일 없음"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Quarrel, Tuning, nullptr), 0.f, 1e-5f);
	TestEqual(TEXT("까닭도 그대로"), Reasons[0].Event, EGrimLoyaltyEvent::Rested);

	Loyalty = 98.f;
	TestEqual(TEXT("100 에서 자른다 — 실제 +2"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Fed, Tuning, nullptr), 2.f, 1e-4f);
	TestEqual(TEXT("100"), Loyalty, 100.f, 1e-4f);
	TestEqual(TEXT("100 이면 더 오르지 않고 까닭도 없음"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Fed, Tuning, nullptr), 0.f, 1e-5f);

	TestEqual(TEXT("80 = 충직"), GrimTamingMath::GetStage(80.f, Tuning), EGrimLoyaltyStage::Devoted);
	TestEqual(TEXT("79.9 = 보통"), GrimTamingMath::GetStage(79.9f, Tuning), EGrimLoyaltyStage::Normal);
	TestEqual(TEXT("49 = 불만"), GrimTamingMath::GetStage(49.f, Tuning), EGrimLoyaltyStage::Discontent);
	TestEqual(TEXT("19 = 반항"), GrimTamingMath::GetStage(19.f, Tuning), EGrimLoyaltyStage::Rebellious);
	TestEqual(TEXT("0.5 = 반항(1 미만이어도 0 초과)"), GrimTamingMath::GetStage(0.5f, Tuning), EGrimLoyaltyStage::Rebellious);
	TestEqual(TEXT("0 = 떠남"), GrimTamingMath::GetStage(0.f, Tuning), EGrimLoyaltyStage::Gone);

	Loyalty = 5.f;
	GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::KnockedOut, Tuning, nullptr);
	TestEqual(TEXT("0 아래로 가지 않는다"), Loyalty, 0.f, 1e-5f);
	TestEqual(TEXT("실제 변화 −5 가 까닭"), Reasons[0].Delta, -5.f, 1e-4f);

	TestEqual(TEXT("접속 안 함 2.5시간 = 주인 부재 2"), GrimTamingMath::GetOfflineAbsentEvents(2.5 * 3600.0, 5), 2);
	TestEqual(TEXT("상한"), GrimTamingMath::GetOfflineAbsentEvents(48.0 * 3600.0, 5), 5);
	TestEqual(TEXT("음수 시간"), GrimTamingMath::GetOfflineAbsentEvents(-10.0, 5), 0);

	// 저장소의 설정(ini)도 스펙 표와 맞는다.
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	TestEqual(TEXT("ini: 길들인 직후 50"), Settings->Loyalty.InitialLoyalty, 50.f, 1e-4f);
	TestEqual(TEXT("ini: 좋아하는 먹이 = 먹이 × 2"), Settings->GetEventDelta(EGrimLoyaltyEvent::FedFavorite), Settings->GetEventDelta(EGrimLoyaltyEvent::Fed) * 2.f, 1e-4f);
	for (EGrimLoyaltyEvent Event : { EGrimLoyaltyEvent::Fed, EGrimLoyaltyEvent::Rested, EGrimLoyaltyEvent::Healed, EGrimLoyaltyEvent::WonTogether, EGrimLoyaltyEvent::GoodEnvironment, EGrimLoyaltyEvent::DaysTogether, EGrimLoyaltyEvent::Explored })
	{
		TestTrue(FString::Printf(TEXT("ini: 오르는 사건 %s"), GrimTamingMath::EventName(Event)), Settings->GetEventDelta(Event) > 0.f);
	}
	for (EGrimLoyaltyEvent Event : { EGrimLoyaltyEvent::Starved, EGrimLoyaltyEvent::Overworked, EGrimLoyaltyEvent::LeftInjured, EGrimLoyaltyEvent::KnockedOut, EGrimLoyaltyEvent::BadEnvironment, EGrimLoyaltyEvent::Quarrel, EGrimLoyaltyEvent::OwnerAbsent })
	{
		TestTrue(FString::Printf(TEXT("ini: 내리는 사건 %s"), GrimTamingMath::EventName(Event)), Settings->GetEventDelta(Event) < 0.f);
	}
	TestEqual(TEXT("ini: 80 = 충직"), GrimTamingMath::GetStage(80.f, Settings->Loyalty), EGrimLoyaltyStage::Devoted);
	TestEqual(TEXT("ini: 50 = 보통"), GrimTamingMath::GetStage(50.f, Settings->Loyalty), EGrimLoyaltyStage::Normal);
	TestEqual(TEXT("ini: 20 = 불만"), GrimTamingMath::GetStage(20.f, Settings->Loyalty), EGrimLoyaltyStage::Discontent);
	TestEqual(TEXT("ini: 1 = 반항"), GrimTamingMath::GetStage(1.f, Settings->Loyalty), EGrimLoyaltyStage::Rebellious);
	TestEqual(TEXT("ini: 단계 태그"), GrimTamingTags::GetStageTag(EGrimLoyaltyStage::Devoted).GetTagName(), FName(TEXT("Creature.Loyalty.Devoted")));
	return true;
}

// 성격(Q27): 사건별 배율 → 없으면 오름 · 내림 배율.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingPersonalityTest, "Grimoire.Taming.Personality", PureFlags)
bool FGrimTamingPersonalityTest::RunTest(const FString& Parameters)
{
	const FGrimLoyaltyTuning Tuning = MakeLoyaltyTuning();
	FGrimPersonalityTuning Glutton;
	Glutton.Personality = EGrimCreaturePersonality::Glutton;
	FGrimLoyaltyEventScale FedScale;
	FedScale.Event = EGrimLoyaltyEvent::Fed;
	FedScale.Scale = 2.f;
	FGrimLoyaltyEventScale StarveScale;
	StarveScale.Event = EGrimLoyaltyEvent::Starved;
	StarveScale.Scale = 2.f;
	Glutton.EventScales = { FedScale, StarveScale };
	Glutton.GainScale = 1.f;
	Glutton.LossScale = 0.5f;

	float Loyalty = 50.f;
	TArray<FGrimLoyaltyReason> Reasons;
	TestEqual(TEXT("먹보: 먹이 × 2"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Fed, Tuning, &Glutton), 6.f, 1e-4f);
	TestEqual(TEXT("먹보: 굶기면 × 2"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Starved, Tuning, &Glutton), -10.f, 1e-4f);
	TestEqual(TEXT("사건별 배율이 없으면 내림 배율"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Overworked, Tuning, &Glutton), -2.f, 1e-4f);
	TestEqual(TEXT("성격 없음 = 1"), GrimTamingMath::GetPersonalityScale(nullptr, EGrimLoyaltyEvent::Fed, true), 1.f, 1e-5f);
	TestEqual(TEXT("Scale 배율도 곱한다"), GrimTamingMath::ApplyLoyaltyEvent(Loyalty, Reasons, EGrimLoyaltyEvent::Rested, Tuning, nullptr, 2.f), 4.f, 1e-4f);

	// ini: 굴리는 성격(None 밖) 모두 행이 있고, 먹보 · 일벌레 · 겁쟁이 · 싸움꾼 · 잠꾸러기가 스펙대로 다르다.
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	const UEnum* Enum = StaticEnum<EGrimCreaturePersonality>();
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
	{
		const EGrimCreaturePersonality Value = static_cast<EGrimCreaturePersonality>(Enum->GetValueByIndex(Index));
		const FGrimPersonalityTuning* Row = Settings->FindPersonality(Value);
		TestNotNull(*FString::Printf(TEXT("ini: 성격 %s"), *Enum->GetNameStringByIndex(Index)), Row);
		if (Row)
		{
			TestFalse(*FString::Printf(TEXT("ini: 성격 이름 %s"), *Enum->GetNameStringByIndex(Index)), Row->DisplayName.IsEmpty());
		}
	}
	const FGrimPersonalityTuning* IniGlutton = Settings->FindPersonality(EGrimCreaturePersonality::Glutton);
	const FGrimPersonalityTuning* Workaholic = Settings->FindPersonality(EGrimCreaturePersonality::Workaholic);
	const FGrimPersonalityTuning* Coward = Settings->FindPersonality(EGrimCreaturePersonality::Coward);
	const FGrimPersonalityTuning* Brawler = Settings->FindPersonality(EGrimCreaturePersonality::Brawler);
	TestTrue(TEXT("ini 먹보: 먹이로 잘 오른다"), GrimTamingMath::GetPersonalityScale(IniGlutton, EGrimLoyaltyEvent::Fed, true) > 1.f);
	TestTrue(TEXT("ini 먹보: 굶기면 크게 떨어진다"), GrimTamingMath::GetPersonalityScale(IniGlutton, EGrimLoyaltyEvent::Starved, false) > 1.f);
	TestTrue(TEXT("ini 일벌레: 혹사에 덜 떨어진다"), GrimTamingMath::GetPersonalityScale(Workaholic, EGrimLoyaltyEvent::Overworked, false) < 1.f);
	TestTrue(TEXT("ini 일벌레: 작업 속도 ↑(M5 자리)"), GrimTamingMath::GetWorkSpeedScale(EGrimLoyaltyStage::Normal, Settings->Loyalty, Workaholic) > 1.f);
	TestTrue(TEXT("ini 겁쟁이: 기절에 크게 떨어진다"), GrimTamingMath::GetPersonalityScale(Coward, EGrimLoyaltyEvent::KnockedOut, false) > 1.f);
	TestTrue(TEXT("ini 싸움꾼: 함께 이기면 크게 오른다"), GrimTamingMath::GetPersonalityScale(Brawler, EGrimLoyaltyEvent::WonTogether, true) > 1.f);
	return true;
}

// 명령 · 탑승: 길들인 AI 판단 · 순종 · 거부 · 떨어뜨림 · 속도 · 이름.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingCommandsTest, "Grimoire.Taming.Commands", PureFlags)
bool FGrimTamingCommandsTest::RunTest(const FString& Parameters)
{
	FGrimTamedSenses S;
	S.FollowDistance = 350.f;
	S.OwnerDistance = 1000.f;
	TestEqual(TEXT("따라오기 · 멀다 → Follow"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Follow);
	S.OwnerDistance = 200.f;
	TestEqual(TEXT("따라오기 · 가깝다 → Idle"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Idle);
	S.OwnerDistance = -1.f;
	TestEqual(TEXT("주인 없음 → Idle"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Idle);
	S.OwnerDistance = 1000.f;
	S.Command = EGrimTamedCommand::Wait;
	TestEqual(TEXT("기다리기 → Stay(멀어도)"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Stay);
	S.bProvoked = true;
	TestEqual(TEXT("다른 생물에게 맞음 → Fight"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Fight);
	S.bTimid = true;
	TestEqual(TEXT("겁 많은 종은 물러난다"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Flee);
	S.bTimid = false;
	S.HealthRatio = 0.1f;
	TestEqual(TEXT("체력 낮음 → Flee"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Flee);
	S.bBurning = true;
	TestEqual(TEXT("불붙음 → Flee"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Flee);
	S.bKnockedOut = true;
	TestEqual(TEXT("기절 → Sleep"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Sleep);
	S.bRidden = true;
	TestEqual(TEXT("탔음 → Ridden(무엇보다 먼저)"), GrimTamingMath::DecideTamed(S), EGrimCreatureAIState::Ridden);

	// 야생 판단: 계약에 저항해 화나면 겁 많은 종도 덤빈다.
	FGrimCreatureSenses Wild;
	Wild.bEnraged = true;
	Wild.bThreatNear = true;
	FGrimCreatureTemperamentInfo Timid;
	Timid.Temperament = EGrimCreatureTemperament::Timid;
	Timid.bCanAttack = true;
	TestEqual(TEXT("화난 겁쟁이 → Fight"), GrimCreatureMath::Decide(Wild, Timid), EGrimCreatureAIState::Fight);
	Wild.bEnraged = false;
	TestEqual(TEXT("화가 풀리면 → Flee"), GrimCreatureMath::Decide(Wild, Timid), EGrimCreatureAIState::Flee);

	const FGrimLoyaltyTuning Tuning = MakeLoyaltyTuning();
	TestTrue(TEXT("보통은 늘 따른다"), GrimTamingMath::RollObedience(EGrimLoyaltyStage::Normal, Tuning, 0.999f));
	TestTrue(TEXT("불만 0.74 → 따름"), GrimTamingMath::RollObedience(EGrimLoyaltyStage::Discontent, Tuning, 0.74f));
	TestFalse(TEXT("불만 0.76 → 무시"), GrimTamingMath::RollObedience(EGrimLoyaltyStage::Discontent, Tuning, 0.76f));
	TestFalse(TEXT("반항 0.5 → 무시"), GrimTamingMath::RollObedience(EGrimLoyaltyStage::Rebellious, Tuning, 0.5f));
	TestFalse(TEXT("충직은 타기를 거부하지 않는다"), GrimTamingMath::RollMountRefuse(EGrimLoyaltyStage::Devoted, Tuning, 0.f));
	TestTrue(TEXT("반항 0.5 → 거부"), GrimTamingMath::RollMountRefuse(EGrimLoyaltyStage::Rebellious, Tuning, 0.5f));
	TestTrue(TEXT("반항 0.2 → 떨어뜨림"), GrimTamingMath::RollBuck(EGrimLoyaltyStage::Rebellious, Tuning, 0.2f));
	TestFalse(TEXT("보통은 떨어뜨리지 않는다"), GrimTamingMath::RollBuck(EGrimLoyaltyStage::Normal, Tuning, 0.f));
	TestEqual(TEXT("충직 탑승 속도 ↑"), GrimTamingMath::GetMountSpeedScale(EGrimLoyaltyStage::Devoted, Tuning), 1.15f, 1e-4f);
	TestEqual(TEXT("표에 없는 단계 = 1"), GrimTamingMath::GetMountSpeedScale(EGrimLoyaltyStage::Gone, Tuning), 1.f, 1e-4f);

	TestEqual(TEXT("이름: 앞뒤 공백 · 줄바꿈"), GrimTamingMath::SanitizeNickname(TEXT("  물\n결이 "), 16), FString(TEXT("물 결이")));
	TestEqual(TEXT("이름: 길이"), GrimTamingMath::SanitizeNickname(TEXT("가나다라마바사"), 4), FString(TEXT("가나다라")));
	TestEqual(TEXT("이름: 빈 것"), GrimTamingMath::SanitizeNickname(TEXT("   "), 16), FString());
	return true;
}

// 세이브 왕복: Creatures.Owned 기록을 쓰고 읽으면 같다. 깨진 수는 읽지 않는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingSaveRoundTripTest, "Grimoire.Taming.SaveRoundTrip", PureFlags)
bool FGrimTamingSaveRoundTripTest::RunTest(const FString& Parameters)
{
	TArray<FGrimOwnedCreatureRecord> Records = { MakeRecord(11), MakeRecord(12) };
	Records[1].Instance.SpeciesId = TEXT("Jackalope");
	Records[1].Instance.Nickname.Reset();
	Records[1].Reasons.Reset();
	Records[1].Command = EGrimTamedCommand::Follow;

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	TestTrue(TEXT("쓰기"), GrimTamingMath::SerializeOwnedRecords(Writer, Records));

	TArray<FGrimOwnedCreatureRecord> Loaded;
	FMemoryReader Reader(Bytes);
	if (!TestTrue(TEXT("읽기"), GrimTamingMath::SerializeOwnedRecords(Reader, Loaded)) || !TestEqual(TEXT("수"), Loaded.Num(), 2))
	{
		return false;
	}
	for (int32 i = 0; i < 2; ++i)
	{
		const FGrimOwnedCreatureRecord& A = Records[i];
		const FGrimOwnedCreatureRecord& B = Loaded[i];
		TestEqual(TEXT("종"), B.Instance.SpeciesId, A.Instance.SpeciesId);
		TestEqual(TEXT("씨앗"), B.Instance.Seed, A.Instance.Seed);
		TestEqual(TEXT("성별"), B.Instance.Sex, A.Instance.Sex);
		TestEqual(TEXT("색 수"), B.Instance.Colors.Num(), A.Instance.Colors.Num());
		for (int32 c = 0; c < FMath::Min(A.Instance.Colors.Num(), B.Instance.Colors.Num()); ++c)
		{
			TestEqual(TEXT("팔레트 번호"), B.Instance.Colors[c].PaletteIndex, A.Instance.Colors[c].PaletteIndex);
			TestEqual(TEXT("돌연변이 영역"), B.Instance.Colors[c].bMutant, A.Instance.Colors[c].bMutant);
			TestTrue(TEXT("색(비트까지)"), B.Instance.Colors[c].Color == A.Instance.Colors[c].Color);
		}
		TestEqual(TEXT("크기"), B.Instance.SizeScale, A.Instance.SizeScale);
		TestEqual(TEXT("거대"), B.Instance.bGiant, A.Instance.bGiant);
		TestEqual(TEXT("돌연변이"), B.Instance.bMutant, A.Instance.bMutant);
		TestEqual(TEXT("성격"), B.Instance.Personality, A.Instance.Personality);
		TestEqual(TEXT("능력치 배율"), B.Instance.StatScale, A.Instance.StatScale);
		TestEqual(TEXT("주인 PlayerId"), B.Instance.OwnerId, A.Instance.OwnerId);
		TestEqual(TEXT("충성도"), B.Instance.Loyalty, A.Instance.Loyalty);
		TestEqual(TEXT("이름(한글)"), B.Instance.Nickname, A.Instance.Nickname);
		TestEqual(TEXT("체력 비율"), B.HealthRatio, A.HealthRatio);
		TestEqual(TEXT("명령"), B.Command, A.Command);
		TestTrue(TEXT("까닭"), B.Reasons == A.Reasons);
		TestEqual(TEXT("저장 시각"), B.SavedUtcSeconds, A.SavedUtcSeconds);
		TestTrue(TEXT("같은 개체"), B.IsSameCreature(A.Instance));
	}

	// 깨진 데이터: 수가 터무니없다 → false
	TArray<uint8> Bad;
	FMemoryWriter BadWriter(Bad);
	int32 Huge = 100000;
	BadWriter << Huge;
	TArray<FGrimOwnedCreatureRecord> BadLoaded;
	FMemoryReader BadReader(Bad);
	TestFalse(TEXT("깨진 수는 읽지 않는다"), GrimTamingMath::SerializeOwnedRecords(BadReader, BadLoaded));

	// 잘린 데이터
	TArray<uint8> Cut = Bytes;
	Cut.SetNum(Bytes.Num() / 2);
	TArray<FGrimOwnedCreatureRecord> CutLoaded;
	FMemoryReader CutReader(Cut);
	TestFalse(TEXT("잘린 데이터는 실패"), GrimTamingMath::SerializeOwnedRecords(CutReader, CutLoaded));
	return true;
}

// 데이터: 파크툼 정의 · 스킬북 아이템 · 탈것 · 먹이 · 종의 길들이기 자리.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingAssetsTest, "Grimoire.Taming.Assets", WorldFlags)
bool FGrimTamingAssetsTest::RunTest(const FString& Parameters)
{
	struct FExpected
	{
		const TCHAR* Id;
		const TCHAR* Element;
		int32 Circle;
	};
	const FExpected Expected[] = {
		{ TEXT("Pactum"), TEXT("Element.None"), 1 },
		{ TEXT("Pactum_Ignis"), TEXT("Element.Fire"), 2 },
		{ TEXT("Pactum_Aqua"), TEXT("Element.Water"), 2 },
		{ TEXT("Pactum_Ventus"), TEXT("Element.Wind"), 2 },
		{ TEXT("Pactum_Terra"), TEXT("Element.Earth"), 2 },
		{ TEXT("Pactum_Umbra"), TEXT("Element.Dark.Shadow"), 3 },
	};
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	for (const FExpected& Row : Expected)
	{
		const UGrimSpellDefinition* Spell = UGrimQuestSettings::LoadSpell(Row.Id);
		if (!TestNotNull(*FString::Printf(TEXT("정의 DA_Spell_%s"), Row.Id), Spell))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: 볼트"), Row.Id), Spell->Delivery == EGrimSpellDelivery::Bolt);
		TestTrue(*FString::Printf(TEXT("%s: 파크툼 투사체"), Row.Id), Spell->ProjectileClass && Spell->ProjectileClass->IsChildOf(AGrimPactumProjectile::StaticClass()));
		TestEqual(*FString::Printf(TEXT("%s: 피해 없음"), Row.Id), Spell->Damage, 0.f);
		TestEqual(*FString::Printf(TEXT("%s: 속성"), Row.Id), Spell->Element.GetTagName(), FName(Row.Element));
		TestEqual(*FString::Printf(TEXT("%s: 서클"), Row.Id), Spell->Circle, Row.Circle);
		TestTrue(*FString::Printf(TEXT("%s: 숙련도 추가 효과 = 포획 확률"), Row.Id), Spell->ProficiencyExtraEffect.EffectTag == GrimTamingTags::Taming_CaptureBonus && Spell->ProficiencyExtraEffect.MaxValue > 0.f);
		TestTrue(*FString::Printf(TEXT("%s: 마법 태그 Spell.Pactum*"), Row.Id), Spell->SpellTag.MatchesTag(GrimTamingTags::Spell_Pactum));
		const FGrimItemDefinition* Book = Items.Find(FName(*FString::Printf(TEXT("Skillbook_%s"), Row.Id)));
		TestTrue(*FString::Printf(TEXT("스킬북 아이템 Skillbook_%s"), Row.Id), Book && Book->Row.TeachesSpell == FName(Row.Id));
	}

	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	UGrimCreatureSpecies* Kelpie = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Kelpie"));
	if (TestNotNull(TEXT("켈피"), Kelpie))
	{
		TestTrue(TEXT("켈피는 탈것"), Kelpie->bMountable);
		TestTrue(TEXT("켈피 물 위 달리기"), Kelpie->WaterMovement == EGrimCreatureWaterMovement::RunOnSurface);
		TestNotNull(TEXT("켈피 안장 설정"), Settings->FindMountProfile(TEXT("Kelpie")));
	}
	for (const TCHAR* Id : { TEXT("Jackalope"), TEXT("Kelpie"), TEXT("BlackDog") })
	{
		const UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Id);
		if (!TestNotNull(*FString::Printf(TEXT("종 %s"), Id), Species))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s 길들일 수 있다"), Id), Species->bTameable);
		TestTrue(*FString::Printf(TEXT("%s 먹는 묶음"), Id), Settings->GetAcceptedGroups(Species->SpeciesId, Species->Diet).Num() > 0);
		if (const FGrimCreatureFoodRule* Rule = Settings->FindFoodRule(Species->SpeciesId))
		{
			for (const FName& Favorite : Rule->FavoriteItems)
			{
				TestNotNull(*FString::Printf(TEXT("%s 좋아하는 먹이 %s 가 아이템 표에"), Id, *Favorite.ToString()), Items.Find(Favorite));
			}
		}
	}
	TestEqual(TEXT("튜토리얼 이벤트 이름"), UGrimCreatureSettings::GetQuestEventName(Settings->TamedEventPrefix, EGrimCreatureSizeClass::Small), FName(TEXT("Creature.Tamed.Small")));
	TestTrue(TEXT("숙련도 추가 효과 태그 설정"), Settings->CaptureBonusEffectTag == GrimTamingTags::Taming_CaptureBonus);
	return true;
}

// 생물 액터: 길들임 → 태그 · 충성도 · 기절(죽지 않음) → 떠남(충성도 0).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTamingActorTest, "Grimoire.Taming.Actor", WorldFlags)
bool FGrimTamingActorTest::RunTest(const FString& Parameters)
{
	UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"));
	if (!TestNotNull(TEXT("재칼로프 에셋"), Species))
	{
		return false;
	}
	FScopedWorld TestWorld;
	const FGrimCreatureInstance Instance = GrimCreatureMath::RollInstance(*Species, 4242, UGrimCreatureSettings::Get()->Roll);
	AGrimCreature* Creature = AGrimCreature::SpawnCreature(TestWorld.World, Species, Instance, FVector::ZeroVector, 0.f, 0.2f);
	if (!TestNotNull(TEXT("스폰"), Creature) || !TestNotNull(TEXT("길들이기 컴포넌트"), Creature->GetTaming()) || !TestNotNull(TEXT("탈것 컴포넌트"), Creature->GetMount()))
	{
		return false;
	}
	UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent();
	TestFalse(TEXT("야생"), Taming->IsTamed());
	TestFalse(TEXT("재칼로프는 탈것이 아니다"), Creature->GetMount()->IsMountable());
	TestTrue(TEXT("약해진 채 스폰"), ASC->HasMatchingGameplayTag(GrimCreatureTags::Creature_State_Weakened));

	Taming->Claim(nullptr, TEXT("TestPlayer"));
	TestTrue(TEXT("길들임"), Taming->IsTamed());
	TestEqual(TEXT("주인 PlayerId"), Creature->GetInstance().OwnerId, FString(TEXT("TestPlayer")));
	TestEqual(TEXT("충성도 50"), Taming->GetLoyalty(), UGrimTamingSettings::Get()->Loyalty.InitialLoyalty, 1e-3f);
	TestTrue(TEXT("Creature.State.Tamed"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_State_Tamed));
	TestTrue(TEXT("Creature.Loyalty.Normal"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_Loyalty_Normal));
	TestEqual(TEXT("까닭 「계약」"), Taming->GetRecentReasons().Num() > 0 ? Taming->GetRecentReasons()[0].Event : EGrimLoyaltyEvent::None, EGrimLoyaltyEvent::Tamed);
	TestTrue(TEXT("체력을 반까지 채운다"), Creature->GetHealthRatio() >= UGrimTamingSettings::Get()->Capture.TamedHealToRatio - 0.01f);
	TestFalse(TEXT("약해짐 표지를 뗐다"), ASC->HasMatchingGameplayTag(GrimCreatureTags::Creature_State_Weakened));
	TestFalse(TEXT("플레이어 없는 요청자(생물)는 겨눌 수 있다"), !Creature->CanBeTargetedBy(nullptr));

	Taming->DebugSetLoyalty(85.f);
	TestTrue(TEXT("85 → Creature.Loyalty.Devoted"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_Loyalty_Devoted));
	TestFalse(TEXT("앞 단계 태그는 뗐다"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_Loyalty_Normal));

	const float Before = Taming->GetLoyalty();
	Taming->RecordFed(nullptr, true);
	TestTrue(TEXT("좋아하는 먹이로 오른다"), Taming->GetLoyalty() > Before);

	// 기절: 체력이 0 이 돼도 죽지 않는다.
	Creature->ApplyDebugDamage(Creature->GetMaxHealth() * 2.f, FGameplayTag(), nullptr);
	TestFalse(TEXT("길들인 생물은 죽지 않는다"), Creature->IsDead());
	TestTrue(TEXT("쓰러졌다"), Taming->IsKnockedOut());
	TestEqual(TEXT("기절 까닭"), Taming->GetRecentReasons()[0].Event, EGrimLoyaltyEvent::KnockedOut);

	const FGrimOwnedCreatureRecord Record = Taming->MakeRecord();
	TestTrue(TEXT("기록 = 같은 개체"), Record.IsSameCreature(Creature->GetInstance()));
	TestEqual(TEXT("기록의 충성도"), Record.Instance.Loyalty, Taming->GetLoyalty(), 1e-3f);

	// 충성도 0 → 떠남(야생으로, 다시 잡을 수 있다).
	Taming->DebugSetLoyalty(0.f);
	TestFalse(TEXT("떠남 → 야생"), Taming->IsTamed());
	TestTrue(TEXT("주인 없음"), Creature->GetInstance().OwnerId.IsEmpty());
	TestFalse(TEXT("Creature.State.Tamed 뗌"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_State_Tamed));
	TestFalse(TEXT("단계 태그 뗌"), ASC->HasMatchingGameplayTag(GrimTamingTags::Creature_Loyalty_Devoted));
	return true;
}

#endif
