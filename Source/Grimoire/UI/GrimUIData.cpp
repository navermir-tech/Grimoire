#include "UI/GrimUIData.h"
#include "UI/GrimUIMath.h"
#include "UI/GrimUISettings.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameplayEffect.h"
#include "Styling/CoreStyle.h"

void UGrimIdButton::Bind()
{
	OnClicked.AddUniqueDynamic(this, &UGrimIdButton::Relay);
}

void UGrimIdButton::Relay()
{
	OnIdClicked.ExecuteIfBound(Id);
}

namespace GrimUIData
{
	bool FindAttribute(const UAbilitySystemComponent* ASC, const FString& Name, FGameplayAttribute& OutAttribute)
	{
		if (!ASC || Name.IsEmpty())
		{
			return false;
		}
		FString SetName, PropertyName;
		FGrimUIMath::SplitAttributeName(Name, SetName, PropertyName);
		for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
		{
			if (!Set || !FGrimUIMath::AttributeSetNameMatches(Set->GetClass()->GetName(), SetName))
			{
				continue;
			}
			const FStructProperty* Property = FindFProperty<FStructProperty>(Set->GetClass(), FName(*PropertyName));
			if (Property && Property->Struct && Property->Struct->IsChildOf(FGameplayAttributeData::StaticStruct()))
			{
				OutAttribute = FGameplayAttribute(const_cast<FStructProperty*>(Property));
				return true;
			}
		}
		return false;
	}

	bool GetSpellCooldown(const UAbilitySystemComponent* ASC, const UGrimSpellDefinition* Spell, float& OutRemaining, float& OutDuration)
	{
		OutRemaining = 0.f;
		OutDuration = 0.f;
		if (!ASC || !Spell || !Spell->CooldownTag.IsValid())
		{
			return false;
		}
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Spell->CooldownTag));
		const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		for (const TPair<float, float>& Time : Times)
		{
			if (Time.Key > OutRemaining)
			{
				OutRemaining = Time.Key;
				OutDuration = Time.Value;
			}
		}
		return OutRemaining > 0.f;
	}

	FLinearColor GetElementColor(const UGrimSpellDefinition* Spell)
	{
		if (!Spell)
		{
			return FLinearColor::Gray;
		}
		if (const FGrimElementDisplay* Display = UGrimUISettings::Get()->FindElementDisplay(Spell->Element))
		{
			return Display->Color;
		}
		return Spell->Color;
	}

	FSlateFontInfo Font(int32 Size, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color, bool bBold)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(Text);
		Block->SetFont(Font(Size, bBold));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetShadowOffset(FVector2D(1.f, 1.f));
		Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return Block;
	}

	UBorder* MakeBorder(UWidgetTree* Tree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Border->SetBrushColor(Color);
		Border->SetPadding(Padding);
		return Border;
	}

	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Fill)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(0.f);
		return Bar;
	}

	USizeBox* MakeSizeBox(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (Width > 0.f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Box->SetHeightOverride(Height);
		}
		if (Content)
		{
			Box->SetContent(Content);
		}
		return Box;
	}
}
