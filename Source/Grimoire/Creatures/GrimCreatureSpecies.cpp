#include "Creatures/GrimCreatureSpecies.h"
#include "Misc/DataValidation.h"

DEFINE_LOG_CATEGORY(LogGrimCreatures);

const FGrimCreatureAttack* UGrimCreatureSpecies::FindAttack(const FGameplayTag& Element) const
{
	return Attacks.FindByPredicate([&Element](const FGrimCreatureAttack& Attack) { return Attack.Element == Element; });
}

FGameplayTag UGrimCreatureSpecies::RequestTag(FName TagName)
{
	return FGameplayTag::RequestGameplayTag(TagName, false);
}

FGameplayTagContainer UGrimCreatureSpecies::MakeTagContainer(const TArray<FName>& TagNames)
{
	FGameplayTagContainer Container;
	for (const FName& Name : TagNames)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Name, false);
		if (Tag.IsValid())
		{
			Container.AddTag(Tag);
		}
	}
	return Container;
}

#if WITH_EDITOR
EDataValidationResult UGrimCreatureSpecies::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Fail = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	if (SpeciesId.IsNone())
	{
		Fail(TEXT("SpeciesId 가 비었다"));
	}
	// R35: 속성은 하나 또는 둘, 무속성 생물은 없다.
	if (!PrimaryElement.IsValid())
	{
		Fail(TEXT("주속성이 없다(무속성 생물은 없다 — R35)"));
	}
	if (SecondaryElement.IsValid() && SecondaryElement == PrimaryElement)
	{
		Fail(TEXT("부속성이 주속성과 같다"));
	}
	if (ColorRegions.Num() < 1 || ColorRegions.Num() > 6)
	{
		Fail(TEXT("색 영역은 1~6개"));
	}
	for (const FGrimCreatureBodyPart& Part : BodyParts)
	{
		if (!ColorRegions.IsValidIndex(Part.ColorRegion))
		{
			Fail(FString::Printf(TEXT("부위 %s 의 색 영역 %d 이 없다"), *Part.PartName.ToString(), Part.ColorRegion));
		}
	}
	if (HerdSizeMax < HerdSizeMin)
	{
		Fail(TEXT("무리 크기 최대 < 최소"));
	}
	for (const FGrimCreatureAttack& Attack : Attacks)
	{
		if (!Attack.Element.IsValid())
		{
			Fail(FString::Printf(TEXT("공격 %s 에 속성이 없다"), *Attack.AttackName.ToString()));
		}
	}
	return Result;
}
#endif
