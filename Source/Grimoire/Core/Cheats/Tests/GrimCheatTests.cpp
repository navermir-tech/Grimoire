#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/Cheats/GrimCheatTags.h"
#include "Core/Cheats/GrimSuperCheat.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicTags.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	constexpr EAutomationTestFlags GrimCheatTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 테스트용 빈 월드 하나. 소멸자에서 정리한다. */
	struct FGrimCheatTestWorld
	{
		UWorld* World = nullptr;

		FGrimCheatTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FGrimCheatTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** ASC + 마법 · 생존 어트리뷰트를 가진 액터(서버 권위). */
		UAbilitySystemComponent* SpawnPlayer()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
			ASC->RegisterComponent();
			ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
			ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
			ASC->InitAbilityActorInfo(Actor, Actor);
			return ASC;
		}
	};

	/** 생존 틱 하나(베이스 가산)를 흉내 낸다. */
	void ApplySurvivalTick(UAbilitySystemComponent* ASC, float Health, float Hunger, float BodyTemperature)
	{
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimSurvivalTickEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HealthMagnitudeName, Health);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HungerMagnitudeName, Hunger);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::BodyTemperatureMagnitudeName, BodyTemperature);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	void ApplyManaCost(UAbilitySystemComponent* ASC, float Cost)
	{
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimGE_SpellCost::StaticClass(), 1.f, ASC->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_ManaCost, -Cost);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

// 켜면 최대에 고정되고(비용 · 감소가 와도), 끄면 최대에서 다시 감소한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCheatSuperTest, "Grimoire.Cheat.Super", GrimCheatTestFlags)
bool FGrimCheatSuperTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;
	FGrimCheatTestWorld TestWorld;
	UAbilitySystemComponent* ASC = TestWorld.SpawnPlayer();

	auto Get = [ASC](const FGameplayAttribute& Attribute) { return ASC->GetNumericAttribute(Attribute); };
	const FGameplayAttribute Health = UGrimVitalAttributeSet::GetHealthAttribute();
	const FGameplayAttribute MaxHealth = UGrimVitalAttributeSet::GetMaxHealthAttribute();
	const FGameplayAttribute Hunger = UGrimVitalAttributeSet::GetHungerAttribute();
	const FGameplayAttribute Body = UGrimVitalAttributeSet::GetBodyTemperatureAttribute();
	const FGameplayAttribute Mana = UGrimMagicAttributeSet::GetManaAttribute();
	const FGameplayAttribute MaxMana = UGrimMagicAttributeSet::GetMaxManaAttribute();

	// 지친 상태에서 시작.
	ASC->SetNumericAttributeBase(MaxHealth, 100.f);
	ASC->SetNumericAttributeBase(Health, 30.f);
	ASC->SetNumericAttributeBase(MaxMana, 80.f);
	ASC->SetNumericAttributeBase(Mana, 5.f);
	ASC->SetNumericAttributeBase(Hunger, 3.f);
	ASC->SetNumericAttributeBase(Body, Tuning.HypothermiaThreshold - 2.f);
	ASC->SetLooseGameplayTagCount(GrimSurvivalTags::Status_Hungry, 1, EGameplayTagReplicationState::TagOnly);
	ASC->SetLooseGameplayTagCount(GrimSurvivalTags::Status_Hypothermia, 1, EGameplayTagReplicationState::TagOnly);

	// 켜기
	GrimSuperCheat::SetEnabled(ASC, true);
	TestTrue(TEXT("켜짐 태그"), GrimSuperCheat::IsEnabled(ASC) && ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super));
	TestEqual(TEXT("HP = MaxHealth"), Get(Health), 100.f);
	TestEqual(TEXT("마나 = MaxMana"), Get(Mana), 80.f);
	TestEqual(TEXT("배고픔 = MaxHunger"), Get(Hunger), Tuning.MaxHunger);
	TestEqual(TEXT("체온 = 정상"), Get(Body), Tuning.NormalBodyTemperature);
	TestFalse(TEXT("Status.Hungry 풀림"), ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Hungry));
	TestFalse(TEXT("Status.Hypothermia 풀림"), ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Hypothermia));

	// 두 번 켜도 GE 는 하나.
	GrimSuperCheat::SetEnabled(ASC, true);
	TestEqual(TEXT("GE 하나"), ASC->GetGameplayTagCount(GrimCheatTags::Cheat_Super), 1);

	// 비용 · 생존 감소 · 피해가 와도 그대로.
	ApplyManaCost(ASC, 30.f);
	for (int32 i = 0; i < 10; ++i)
	{
		ApplySurvivalTick(ASC, -20.f, -5.f, -1.f);
	}
	TestEqual(TEXT("비용 뒤에도 마나 최대"), Get(Mana), 80.f);
	TestEqual(TEXT("감소 뒤에도 HP 최대"), Get(Health), 100.f);
	TestEqual(TEXT("감소 뒤에도 배고픔 최대"), Get(Hunger), Tuning.MaxHunger);
	TestEqual(TEXT("감소 뒤에도 체온 정상"), Get(Body), Tuning.NormalBodyTemperature);

	// 최대치가 바뀌면 따라간다.
	ASC->SetNumericAttributeBase(MaxHealth, 150.f);
	TestEqual(TEXT("MaxHealth 를 따라감"), Get(Health), 150.f);

	// 끄기: 최대에서 원래 흐름으로.
	GrimSuperCheat::SetEnabled(ASC, false);
	TestFalse(TEXT("꺼짐"), GrimSuperCheat::IsEnabled(ASC));
	TestEqual(TEXT("끈 직후 HP 최대"), Get(Health), 150.f);
	TestEqual(TEXT("끈 직후 마나 최대"), Get(Mana), 80.f);
	TestEqual(TEXT("끈 직후 배고픔 최대"), Get(Hunger), Tuning.MaxHunger);

	ApplyManaCost(ASC, 10.f);
	ApplySurvivalTick(ASC, -5.f, -1.f, -0.5f);
	TestEqual(TEXT("감소 재개: 마나"), Get(Mana), 70.f);
	TestEqual(TEXT("감소 재개: HP"), Get(Health), 145.f);
	TestEqual(TEXT("감소 재개: 배고픔"), Get(Hunger), Tuning.MaxHunger - 1.f);
	TestEqual(TEXT("감소 재개: 체온"), Get(Body), Tuning.NormalBodyTemperature - 0.5f);
	return true;
}

#endif
