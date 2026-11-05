#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GrimGrimoireWidget.generated.h"

class AGrimPlayerState;
class UBorder;
class UGrimIdButton;
class UHorizontalBox;
class UProgressBar;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UGrimSpellDefinition;
class UGrimSpellbookComponent;

/**
 * 그리모어 화면 (플레이어의 마법서, R21 · Q26) — 위젯 트리를 C++ 로 짠다. 탭 셋:
 *  - 「마법」: 배운 마법 목록(숙련도 없는 항목 → 서클별 → 배운 순서) · 속성 필터 · 등급 글자 · 다음 등급 막대 · 실제 마나 · 쿨타임,
 *    단축 슬롯 배치(세트 고르기 → 마법 고르기 → 칸 누르기), 완드 보정. 줄을 누르면 그 마법을 고른다(서버 RPC). S 는 금테.
 *  - 「성장 트리」: 빈 자리(속성 성장 트리 노말 · 4원소 · 빛 · 어둠 — 다음 묶음).
 *  - 「도감」: 빈 자리(발견한 상호작용 규칙 기록 — 다음 묶음).
 * 머리에는 서클 · 호칭 · 최대 마나 · 다음 서클 조건. 열고 닫기는 UGrimUISubsystem(IA_ToggleGrimoire).
 */
UCLASS()
class GRIMOIRE_API UGrimGrimoireWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 다음 틱에 목록을 다시 짠다(열 때). */
	void MarkDirty() { bListDirty = true; }

	/** 0 = 마법, 1 = 성장 트리, 2 = 도감 */
	void SetTab(int32 TabIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildTree();
	UWidget* BuildMagicPage();
	UWidget* BuildPlaceholderPage(const FText& Title, const FText& Body);
	void RebuildList(const AGrimPlayerState* PlayerState);
	void RefreshRows(const AGrimPlayerState* PlayerState);
	void RefreshHeader(const AGrimPlayerState* PlayerState);
	void RefreshSlotEditor(const AGrimPlayerState* PlayerState);
	void RefreshFilterButtons();
	void RefreshTabButtons();

	void HandleTabClicked(int32 TabIndex);
	void HandleFilterClicked(int32 FilterIndex);
	void HandleRowClicked(int32 KnownIndex);
	void HandleSetClicked(int32 SetIndex);
	void HandleSlotClicked(int32 SlotInSet);

	UGrimSpellbookComponent* GetSpellbook() const;

private:
	struct FRowWidgets
	{
		int32 KnownIndex = INDEX_NONE;
		UGrimIdButton* Button = nullptr;
		UBorder* Frame = nullptr;
		UTextBlock* Grade = nullptr;
		UTextBlock* GradeName = nullptr;
		UProgressBar* Bar = nullptr;
		UTextBlock* XP = nullptr;
		UTextBlock* Cost = nullptr;
		UTextBlock* Selected = nullptr;
	};

	UTextBlock* CircleText = nullptr;
	UTextBlock* RequirementText = nullptr;
	UTextBlock* WandText = nullptr;
	UHorizontalBox* FilterRow = nullptr;
	UVerticalBox* List = nullptr;
	UWidgetSwitcher* Pages = nullptr;

	TArray<UGrimIdButton*> TabButtons;
	int32 TabIndex = 0;

	TArray<UGrimIdButton*> SetButtons;
	TArray<UGrimIdButton*> SlotButtons;
	TArray<UTextBlock*> SlotLabels;
	/** 슬롯에 놓으려고 고른 마법(배운 번호) */
	int32 PickedKnownIndex = INDEX_NONE;

	/** [0] = 전체(빈 태그) */
	TArray<FGameplayTag> FilterTags;
	TArray<UGrimIdButton*> FilterButtons;
	int32 FilterIndex = 0;

	TArray<FRowWidgets> Rows;
	int32 LastKnownCount = INDEX_NONE;
	bool bListDirty = true;
	float RefreshTimer = 0.f;
};
