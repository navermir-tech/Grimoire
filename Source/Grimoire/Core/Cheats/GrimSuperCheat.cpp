#include "Core/Cheats/GrimSuperCheat.h"
#include "Core/Cheats/GrimCheatTags.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

const FName UGrimSuperCheatEffect::HungerMagnitudeName(TEXT("Cheat.Hunger"));
const FName UGrimSuperCheatEffect::BodyTemperatureMagnitudeName(TEXT("Cheat.BodyTemperature"));

namespace
{
	/** 대상의 다른 어트리뷰트(최대치)로 덮어쓴다. 스냅샷이 없어 최대치가 바뀌면 다시 계산된다. */
	FGameplayModifierInfo MakeOverrideFromAttribute(const FGameplayAttribute& Attribute, const FGameplayAttribute& Source)
	{
		FAttributeBasedFloat AttributeBased;
		AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(Source, EGameplayEffectAttributeCaptureSource::Target, false);

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
		return Modifier;
	}

	FGameplayModifierInfo MakeOverrideFromSetByCaller(const FGameplayAttribute& Attribute, FName DataName)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = DataName;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

UGrimSuperCheatEffect::UGrimSuperCheatEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Modifiers.Add(MakeOverrideFromAttribute(UGrimVitalAttributeSet::GetHealthAttribute(), UGrimVitalAttributeSet::GetMaxHealthAttribute()));
	Modifiers.Add(MakeOverrideFromAttribute(UGrimMagicAttributeSet::GetManaAttribute(), UGrimMagicAttributeSet::GetMaxManaAttribute()));
	Modifiers.Add(MakeOverrideFromSetByCaller(UGrimVitalAttributeSet::GetHungerAttribute(), HungerMagnitudeName));
	Modifiers.Add(MakeOverrideFromSetByCaller(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), BodyTemperatureMagnitudeName));

	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("SuperCheatTag"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(GrimCheatTags::Cheat_Super);
	TargetTags->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTags);
}

namespace GrimSuperCheat
{
	bool IsEnabled(const UAbilitySystemComponent* ASC)
	{
		return ASC && ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super);
	}

	void SetEnabled(UAbilitySystemComponent* ASC, bool bEnable)
	{
		if (!ASC || !ASC->IsOwnerActorAuthoritative() || IsEnabled(ASC) == bEnable)
		{
			return;
		}

		const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;

		if (bEnable)
		{
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimSuperCheatEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
			if (!Spec.IsValid())
			{
				return;
			}
			Spec.Data->SetSetByCallerMagnitude(UGrimSuperCheatEffect::HungerMagnitudeName, Tuning.MaxHunger);
			Spec.Data->SetSetByCallerMagnitude(UGrimSuperCheatEffect::BodyTemperatureMagnitudeName, Tuning.NormalBodyTemperature);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

			// 생존 틱이 다음 틱에 다시 판정하지만, 켜는 즉시 상태 태그를 뗀다.
			for (const FGameplayTag& Tag : { GrimSurvivalTags::Status_Hungry.GetTag(), GrimSurvivalTags::Status_Starving.GetTag(),
				GrimSurvivalTags::Status_Hypothermia.GetTag(), GrimSurvivalTags::Status_Hyperthermia.GetTag() })
			{
				ASC->SetLooseGameplayTagCount(Tag, 0, EGameplayTagReplicationState::TagOnly);
			}
		}
		else
		{
			// 켜 있는 동안 베이스 값은 비용 · 감소로 줄어 있다 — 끈 순간 그 값으로 떨어지지 않게 채운 뒤 뗀다.
			ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute()));
			ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute()));
			ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), Tuning.MaxHunger);
			ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), Tuning.NormalBodyTemperature);
			ASC->RemoveActiveGameplayEffectBySourceEffect(UGrimSuperCheatEffect::StaticClass(), nullptr);
		}
	}
}
