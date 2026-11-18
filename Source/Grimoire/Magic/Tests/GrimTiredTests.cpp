#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMissText.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

namespace GrimTiredTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FTiredTestWorld
	{
		UWorld* World = nullptr;

		FTiredTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimTiredTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FTiredTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	int32 CountMissTexts(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AGrimMissText> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}
}

// 지침(Q132 · Q165): 생물 · 플레이어에게 맞은 판정 뒤 확률로 빗나가고(서버), 시전자 화면에 「MISS」(클라이언트 RPC).
// 시전 시간이 있는 마법만 시전 시간이 는다 — 즉발 · 쿨다운은 그대로.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTiredMissTest, "Grimoire.Magic.Tired.Miss", GrimTiredTestsLocal::WorldFlags)
bool FGrimTiredMissTest::RunTest(const FString& Parameters)
{
	using namespace GrimTiredTestsLocal;
	FTiredTestWorld TestWorld;
	UWorld* World = TestWorld.World;

	AGrimPlayerState* Player = World->SpawnActor<AGrimPlayerState>();
	if (!TestNotNull(TEXT("플레이어"), Player))
	{
		return false;
	}
	UAbilitySystemComponent* Source = Player->GetAbilitySystemComponent();
	Source->InitAbilityActorInfo(Player, Player);
	if (!Source->GetSet<UGrimVitalAttributeSet>())
	{
		Source->AddSpawnedAttribute(Player->GetVitalSet());
	}
	if (!TestTrue(TEXT("피곤함 어트리뷰트"), Source->HasAttributeSetForAttribute(UGrimVitalAttributeSet::GetFatigueAttribute())))
	{
		return false;
	}

	// 맞을 대상(생물 · 플레이어 자리 — ASC 가 있는 것)
	AActor* TargetActor = World->SpawnActor<AActor>();
	UAbilitySystemComponent* Target = NewObject<UAbilitySystemComponent>(TargetActor, TEXT("TargetASC"));
	Target->RegisterComponent();
	Target->InitAbilityActorInfo(TargetActor, TargetActor);

	auto SetFatigue = [Source](float Value) { Source->SetNumericAttributeBase(UGrimVitalAttributeSet::GetFatigueAttribute(), Value); };

	SetFatigue(30.f);
	TestFalse(TEXT("보통(30): 굴림 0 이어도 맞음"), UGrimSpellAbility::RollTiredMiss(Source, Target, 0.f));
	SetFatigue(60.f);
	TestTrue(TEXT("지침(60): 낮은 굴림 → 빗나감"), UGrimSpellAbility::RollTiredMiss(Source, Target, 0.f));
	TestFalse(TEXT("지침(60): 높은 굴림 → 맞음"), UGrimSpellAbility::RollTiredMiss(Source, Target, 0.99f));
	TestFalse(TEXT("사물(ASC 없음)에는 빗나가지 않는다 — 속성 자극은 늘 된다"), UGrimSpellAbility::RollTiredMiss(Source, nullptr, 0.f));
	SetFatigue(85.f);
	TestFalse(TEXT("탈진(85): 시전이 막히므로 빗나감 굴림 없음"), UGrimSpellAbility::RollTiredMiss(Source, Target, 0.f));

	// 확률을 실제로 굴려 본다(지침 65 → 20 %): 2,000번 중 대략 그만큼
	SetFatigue(65.f);
	int32 Misses = 0;
	for (int32 i = 0; i < 2000; ++i)
	{
		Misses += UGrimSpellAbility::RollTiredMiss(Source, Target) ? 1 : 0;
	}
	TestTrue(*FString::Printf(TEXT("65 에서 빗나감 비율 ≈ 20%% (%d/2000)"), Misses), Misses > 300 && Misses < 500);

	// 시전 시간
	UGrimSpellDefinition* Instant = NewObject<UGrimSpellDefinition>();
	Instant->CastWindup = 0.1f;
	UGrimSpellDefinition* Charged = NewObject<UGrimSpellDefinition>();
	Charged->CastWindup = 1.f;
	SetFatigue(60.f);
	TestEqual(TEXT("지침: 즉발(0.1초 시전 대기)은 그대로"), UGrimSpellAbility::GetCastTime(Instant, Source), 0.1f, 1e-4f);
	TestEqual(TEXT("지침: 시전 시간 1초 × 1.3"), UGrimSpellAbility::GetCastTime(Charged, Source), 1.3f, 1e-4f);
	SetFatigue(10.f);
	TestEqual(TEXT("보통: 1초 그대로"), UGrimSpellAbility::GetCastTime(Charged, Source), 1.f, 1e-4f);

	// 「MISS」 피해 숫자: 시전자 화면에 하나, 같은 자리 잦은 빗나감은 한 번만
	UGrimSpellbookComponent* Spellbook = Player->GetSpellbook();
	if (TestNotNull(TEXT("마법서"), Spellbook))
	{
		const FVector At(100.f, 0.f, 150.f);
		Spellbook->ClientShowMiss(At);
		TestEqual(TEXT("알림 1"), Spellbook->GetMissNoticeCount(), 1);
		TestEqual(TEXT("MISS 글 하나"), CountMissTexts(World), 1);
		for (TActorIterator<AGrimMissText> It(World); It; ++It)
		{
			TestEqual(TEXT("글 = MISS"), It->GetText()->Text.ToString(), UGrimMagicSettings::Get()->MissText.Text.ToString());
			TestTrue(TEXT("맞은 자리 근처"), FVector::Dist(It->GetActorLocation(), At) < 1.f);
			TestEqual(TEXT("빛을 받지 않는 글(터짐 빛에 뭉개지지 않게)"), GetNameSafe(It->GetText()->TextMaterial), FString(TEXT("UnlitText")));
		}
		Spellbook->ClientShowMiss(At + FVector(30.f, 0.f, 0.f));
		TestEqual(TEXT("알림 2"), Spellbook->GetMissNoticeCount(), 2);
		TestEqual(TEXT("같은 자리 잦은 빗나감 — 글은 그대로 하나"), CountMissTexts(World), 1);
		Spellbook->ClientShowMiss(At + FVector(1000.f, 0.f, 0.f));
		TestEqual(TEXT("다른 대상 — 글 하나 더"), CountMissTexts(World), 2);
	}

	TargetActor->Destroy();
	Player->Destroy();
	return true;
}

#endif
