#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "GameplayTagsManager.h"
#include "NativeGameplayTags.h"

// 부속성 표지(R35 · Q35). Element.* 와 같은 이름 아래에 둔다 — 10속성 확정 태그까지 미리 등록해 둔다(Element 쪽 태그가 아직 없으면 그냥 쓰이지 않는다).
namespace GrimDamageTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Water, "SubElement.Water", "부속성 표지 — 물");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Water_Liquid, "SubElement.Water.Liquid", "부속성 표지 — 물(액체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Water_Ice, "SubElement.Water.Ice", "부속성 표지 — 얼음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Fire, "SubElement.Fire", "부속성 표지 — 불");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Wind, "SubElement.Wind", "부속성 표지 — 바람");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Earth, "SubElement.Earth", "부속성 표지 — 땅");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Earth_Stone, "SubElement.Earth.Stone", "부속성 표지 — 땅(돌)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Earth_Plant, "SubElement.Earth.Plant", "부속성 표지 — 풀");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Light, "SubElement.Light", "부속성 표지 — 빛 계열");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Light_Holy, "SubElement.Light.Holy", "부속성 표지 — 신성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Light_Radiance, "SubElement.Light.Radiance", "부속성 표지 — 빛");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Dark, "SubElement.Dark", "부속성 표지 — 어둠 계열");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Dark_Hex, "SubElement.Dark.Hex", "부속성 표지 — 흑마법");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SubElement_Dark_Shadow, "SubElement.Dark.Shadow", "부속성 표지 — 그림자술");
}

namespace GrimDamageExecutionLocal
{
	const FGameplayTag& ElementRoot()
	{
		static const FGameplayTag Root = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Element"), false);
		return Root;
	}

	const FGameplayTag& ElementNone()
	{
		static const FGameplayTag None = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Element.None"), false);
		return None;
	}

	bool IsDefenderElement(const FGameplayTag& Tag)
	{
		return Tag.IsValid() && Tag != ElementRoot() && Tag.MatchesTag(ElementRoot()) && !(ElementNone().IsValid() && Tag == ElementNone());
	}
}

UGrimDamageExecution::UGrimDamageExecution()
{
}

FGameplayTag UGrimDamageExecution::FindElementTag(const FGameplayTagContainer& Tags)
{
	const FGameplayTag& Root = GrimDamageExecutionLocal::ElementRoot();
	for (const FGameplayTag& Tag : Tags)
	{
		// Element 자체가 아니라 그 아래 태그(Element.Fire …)만 속성으로 본다.
		if (Tag != Root && Tag.MatchesTag(Root))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UGrimDamageExecution::GetSecondaryMarkerTag(const FGameplayTag& ElementTag)
{
	if (!GrimDamageExecutionLocal::IsDefenderElement(ElementTag))
	{
		return FGameplayTag();
	}
	const FString Name = TEXT("Sub") + ElementTag.ToString();
	return UGameplayTagsManager::Get().RequestGameplayTag(FName(*Name), false);
}

void UGrimDamageExecution::ResolveDefenderElements(const FGameplayTagContainer& Tags, FGameplayTag& OutPrimary, FGameplayTag& OutSecondary)
{
	OutPrimary = FGameplayTag();
	OutSecondary = FGameplayTag();

	TArray<FGameplayTag, TInlineAllocator<4>> Elements;
	for (const FGameplayTag& Tag : Tags)
	{
		if (GrimDamageExecutionLocal::IsDefenderElement(Tag))
		{
			Elements.Add(Tag);
		}
	}
	if (Elements.IsEmpty())
	{
		return;
	}

	// 표지가 붙은 것이 부속성. 나머지 중 첫째가 주속성.
	for (const FGameplayTag& Element : Elements)
	{
		const FGameplayTag Marker = GetSecondaryMarkerTag(Element);
		if (!OutSecondary.IsValid() && Marker.IsValid() && Tags.HasTagExact(Marker))
		{
			OutSecondary = Element;
		}
		else if (!OutPrimary.IsValid())
		{
			OutPrimary = Element;
		}
	}

	if (!OutPrimary.IsValid())
	{
		// 부속성 표지만 있다 — 그것을 주속성으로(속성 하나).
		OutPrimary = OutSecondary;
		OutSecondary = FGameplayTag();
	}
	else if (!OutSecondary.IsValid() && Elements.Num() >= 2)
	{
		// 표지 없이 둘 — 묶음 순서대로.
		OutSecondary = Elements[1] == OutPrimary ? Elements[0] : Elements[1];
	}
}

float UGrimDamageExecution::CombineDualAffinity(float PrimaryAffinity, float SecondaryAffinity, bool bHasSecondary, float SecondaryWeight)
{
	if (!bHasSecondary)
	{
		return PrimaryAffinity;
	}
	return FMath::Max(0.f, PrimaryAffinity * (1.f + SecondaryWeight * (SecondaryAffinity - 1.f)));
}

float UGrimDamageExecution::CalcAffinityAgainst(const FGameplayTag& AttackElement, const FGameplayTag& DefendPrimary, const FGameplayTag& DefendSecondary)
{
	const UGrimMagicSettings* Magic = UGrimMagicSettings::Get();
	const float Primary = Magic->GetAffinity(AttackElement, DefendPrimary);
	const bool bHasSecondary = DefendSecondary.IsValid() && AttackElement.IsValid();
	const float Secondary = bHasSecondary ? Magic->GetAffinity(AttackElement, DefendSecondary) : 1.f;
	return CombineDualAffinity(Primary, Secondary, bHasSecondary, UGrimDamageSettings::Get()->SecondaryElementWeight);
}

void UGrimDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const float BaseDamage = Spec.GetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, false, 0.f);
	if (BaseDamage <= 0.f)
	{
		return;
	}

	FGameplayTagContainer SpecTags;
	Spec.GetAllAssetTags(SpecTags);
	const FGameplayTag AttackElement = FindElementTag(SpecTags);

	FGameplayTag DefendPrimary;
	FGameplayTag DefendSecondary;
	if (const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags())
	{
		ResolveDefenderElements(*TargetTags, DefendPrimary, DefendSecondary);
	}

	const float Affinity = CalcAffinityAgainst(AttackElement, DefendPrimary, DefendSecondary);
	const float FinalDamage = GrimMagicMath::CalcDamage(BaseDamage, 1.f, Affinity);
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGrimVitalAttributeSet::GetHealthAttribute(), EGameplayModOp::AddBase, -FinalDamage));
	}
}
