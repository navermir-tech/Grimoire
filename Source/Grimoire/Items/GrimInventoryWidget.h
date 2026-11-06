#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/GrimInventoryComponent.h"
#include "UI/GrimUIData.h"
#include "GrimInventoryWidget.generated.h"

class AGrimCookingPot;
class AGrimItemPickup;
class UBorder;
class UGrimInventoryTab;
class UHorizontalBox;
class UVerticalBox;
class UGrimIdButton;
class UTextBlock;
class UUniformGridPanel;
class UWidget;

/** 가방 칸 · 행동 버튼 — 포커스를 받지 않는다(눌러도 키 입력이 게임으로 계속 가서 가방 키로 닫을 수 있게) */
UCLASS()
class GRIMOIRE_API UGrimItemButton : public UGrimIdButton
{
	GENERATED_BODY()

public:
	UGrimItemButton() { InitIsFocusable(false); }
};

/**
 * 가방 화면 · 줍기 안내(C++ 위젯 트리) — 로컬 플레이어마다 하나(UGrimInventoryComponent 가 띄운다).
 *  - 아래 가운데: 줍기 · 냄비 안내 「[F] 줍기 — 재칼로프 고기(날)」, 그 위에 알림 한 줄(얻음 · 먹음 · 가방 가득 …).
 *  - 가방 키(UGrimItemSettings::BagKey, 기본 L): 가운데에 칸 격자 + 고른 칸 설명 + 사용(먹기 · 읽기) · 버리기 · 옮기기.
 *    열린 동안 마우스 커서 · 시점 고정(그리모어 화면과 같은 방식 — 그리모어가 열려 있으면 닫는다).
 *  - 키는 침대 · NPC 처럼 PlayerController 키 상태로 본다(정식 입력 액션은 조작 갈래 몫).
 * 값은 소유 클라이언트에 복제된 칸 · 픽업 · 냄비를 읽기만 하고, 바꾸는 것은 서버 RPC.
 *
 * 탭(M4): 맨 앞 「가방」 탭은 이 화면 자신이고, 나머지는 UGrimInventoryTab — ini InventoryTabClasses 또는 RegisterTabClass 로 등록한다.
 * 장비 · 저장고 · 작업대 · 거점 탭은 거점 갈래(Base/GrimBaseTabs)가, 다른 갈래(생물 보관 등)도 같은 방법으로 붙인다.
 */
UCLASS()
class GRIMOIRE_API UGrimInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetInventory(UGrimInventoryComponent* InInventory);

	bool IsBagOpen() const { return bBagOpen; }
	void SetBagOpen(bool bOpen);

	// ───── 탭 (M4) ─────

	/** 가방 탭(이 화면 자신) */
	static const FName BagTabId;
	/** 장비 탭(거점 갈래 — I 키가 바로 연다) */
	static const FName EquipmentTabId;

	/** 코드로 탭 클래스를 더한다(화면을 만들기 전에 — 모듈 시작 · 정적 초기화). ini 의 InventoryTabClasses 와 합친다. */
	static void RegisterTabClass(TSubclassOf<UGrimInventoryTab> TabClass);

	/** 화면을 열고 그 탭을 고른다 */
	void OpenTab(FName TabId);

	/** 탭을 고른다(None · BagTabId = 가방) */
	void SelectTab(FName TabId);

	FName GetActiveTab() const { return ActiveTab; }
	UGrimInventoryTab* FindTab(FName TabId) const;
	UGrimInventoryComponent* GetInventory() const { return Inventory.Get(); }

	/** 안내 글(테스트도 쓴다) */
	static FString DescribePickup(const AGrimItemPickup* Pickup);
	static FString DescribePot(const AGrimCookingPot* Pot);
	static FString DescribeItem(const FGrimItemInstance& Item);
	static FText DescribeNotice(EGrimItemNotice Notice, FName ItemId, int32 Count);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void BuildTree();
	void RefreshBag();
	void RefreshPrompt();
	void HandleKeys();
	void Interact();

	void HandleInventoryChanged(UGrimInventoryComponent* InInventory);
	void HandleNotice(UGrimInventoryComponent* InInventory, EGrimItemNotice Notice, FName ItemId, int32 Count);
	void HandleSlotClicked(int32 SlotIndex);
	void HandleActionClicked(int32 Action);

	void CreateTabs(UVerticalBox* Host);
	void RefreshTabs(bool bForceContent);
	void HandleTabClicked(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGrimInventoryTab>> Tabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> TabContents;

	/** 0 = 가방, 1.. = Tabs */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGrimIdButton>> TabButtons;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TabBar;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BagContent;

	FName ActiveTab;
	float TabAccumulator = 0.f;

	TWeakObjectPtr<UGrimInventoryComponent> Inventory;
	FDelegateHandle ChangedHandle;
	FDelegateHandle NoticeHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> PromptRoot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BagRoot;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> SlotGrid;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGrimIdButton>> SlotButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotTexts;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MoveButtonText;

	TWeakObjectPtr<AGrimItemPickup> TargetPickup;
	TWeakObjectPtr<AGrimCookingPot> TargetPot;

	bool bBagOpen = false;
	bool bBagDirty = true;
	bool bMoveArmed = false;
	int32 SelectedSlot = INDEX_NONE;
	float NoticeRemaining = 0.f;
	float PromptAccumulator = 0.f;
};
