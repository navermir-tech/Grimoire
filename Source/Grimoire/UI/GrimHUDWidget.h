#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "Magic/GrimSpellbookComponent.h"
#include "GrimHUDWidget.generated.h"

class AGrimPlayerState;
class UAbilitySystemComponent;
class UBorder;
class UCanvasPanel;
class UGrimProgressionComponent;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UWidget;

/**
 * 게임 HUD (M2) — 위젯 트리를 C++ 로 짠다(WBP 없음).
 *  - 왼쪽 아래: 서클 · 막대(HP · 마나 · 배고픔 · 체온 · 피곤함 — UGrimUISettings::VitalBars, 어트리뷰트가 없으면 숨김)
 *  - 가운데 아래: 기본 공격 자리(R26 — 연타 번째 「2/4」) + 지금 세트의 단축 슬롯 4칸(호그와트 레거시, 세트 · 칸 수는 UGrimSpellbookSettings)
 *    — 칸마다 키 글자(키보드 · 패드, UGrimSpellInputSettings) · 쿨다운 · 숙련도 등급 글자. 패드 세트 활성(R2/RT 누름) 중엔 테두리가 밝다
 *  - 위: 알림(숙련도 등급 오름 · 배움 · 승급), 가운데 아래: 스킬북 안내(곁에 있을 때)
 * 값은 매 틱 읽는다(PlayerState → ASC · 마법서 · 승급). 서버 권위 값을 읽기만 한다.
 */
UCLASS()
class GRIMOIRE_API UGrimHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 위쪽 알림 한 줄 */
	void ShowToast(const FText& Text, const FLinearColor& Color = FLinearColor::White);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	void BuildTree();
	void BindPlayerState(AGrimPlayerState* PlayerState);
	void RefreshVitals(UAbilitySystemComponent* ASC);
	void RefreshSlots(UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook);
	void RefreshPrompt(const UGrimSpellbookComponent* Spellbook);

	UFUNCTION()
	void HandleGradeUp(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, int32 GradeIndex);

	UFUNCTION()
	void HandleLearnResult(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, EGrimLearnResult Result);

	UFUNCTION()
	void HandleCircleUpResult(UGrimProgressionComponent* Progression, int32 Circle, bool bSuccess);

private:
	struct FBarWidgets
	{
		UWidget* Row = nullptr;
		UProgressBar* Bar = nullptr;
		UTextBlock* Value = nullptr;
		FGameplayAttribute Attribute;
		FGameplayAttribute MaxAttribute;
		bool bResolved = false;
	};

	struct FSlotWidgets
	{
		UWidget* Root = nullptr;
		UBorder* Frame = nullptr;
		UBorder* Inner = nullptr;
		UTextBlock* Key = nullptr;
		UTextBlock* Name = nullptr;
		UTextBlock* Grade = nullptr;
		UProgressBar* Cooldown = nullptr;
		UTextBlock* CooldownText = nullptr;
	};

	FSlotWidgets MakeSlot(const FText& KeyLabel);
	void FillSlot(FSlotWidgets& SlotW, UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, bool bSelected);

	TArray<FBarWidgets> Bars;
	TArray<FSlotWidgets> Slots;
	FSlotWidgets BasicSlot;
	UTextBlock* CircleText = nullptr;
	UTextBlock* SetText = nullptr;
	UTextBlock* ToastText = nullptr;
	UTextBlock* PromptText = nullptr;

	float ToastRemaining = 0.f;
	float PromptTimer = 0.f;
	float ResolveTimer = 0.f;

	TWeakObjectPtr<AGrimPlayerState> BoundPlayerState;
	TWeakObjectPtr<UAbilitySystemComponent> ResolvedASC;
};
