#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/Button.h"
#include "Fonts/SlateFontInfo.h"
#include "GrimUIData.generated.h"

class UAbilitySystemComponent;
class UGrimSpellDefinition;
class UWidgetTree;
class UTextBlock;
class UBorder;
class UProgressBar;
class USizeBox;
class UWidget;

/** 눌리면 자기 번호를 알리는 버튼 — 그리모어 목록 · 필터. */
UCLASS()
class GRIMOIRE_API UGrimIdButton : public UButton
{
	GENERATED_BODY()

public:
	int32 Id = INDEX_NONE;

	TDelegate<void(int32)> OnIdClicked;

	/** OnClicked 를 OnIdClicked 로 잇는다(만든 뒤 한 번). */
	void Bind();

private:
	UFUNCTION()
	void Relay();
};

/** HUD · 그리모어가 같이 쓰는 데이터 읽기 · 위젯 만들기 도우미 */
namespace GrimUIData
{
	/** 이름("Health" · "GrimVitalAttributeSet.Health")으로 ASC 의 어트리뷰트를 찾는다. 없으면 false. */
	GRIMOIRE_API bool FindAttribute(const UAbilitySystemComponent* ASC, const FString& Name, FGameplayAttribute& OutAttribute);

	/** 마법 쿨다운의 남은 · 전체 초(쿨다운 태그를 가진 활성 GE 중 가장 긴 것). 없으면 false. */
	GRIMOIRE_API bool GetSpellCooldown(const UAbilitySystemComponent* ASC, const UGrimSpellDefinition* Spell, float& OutRemaining, float& OutDuration);

	/** 속성색(설정, 없으면 정의의 Color) */
	GRIMOIRE_API FLinearColor GetElementColor(const UGrimSpellDefinition* Spell);

	GRIMOIRE_API FSlateFontInfo Font(int32 Size, bool bBold = false);

	GRIMOIRE_API UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color = FLinearColor::White, bool bBold = false);
	GRIMOIRE_API UBorder* MakeBorder(UWidgetTree* Tree, const FLinearColor& Color, const FMargin& Padding);
	GRIMOIRE_API UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Fill);
	GRIMOIRE_API USizeBox* MakeSizeBox(UWidgetTree* Tree, UWidget* Content, float Width, float Height);
}
