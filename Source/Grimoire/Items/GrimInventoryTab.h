#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GrimInventoryTab.generated.h"

class APlayerController;
class UGrimInventoryComponent;
class UGrimInventoryWidget;
class UWidget;
class UWidgetTree;

/**
 * 소지품 화면의 탭 하나(M4 — docs/systems/m4-plan 「인벤토리 완성」). 「가방」 탭은 화면 자신이고, 나머지는 이 클래스를 상속해 붙인다.
 * 등록: ini [/Script/Grimoire.GrimItemSettings] +InventoryTabClasses=/Script/Grimoire.<클래스>(데이터) 또는 UGrimInventoryWidget::RegisterTabClass(코드).
 * 화면이 만들 때 한 번 BuildContent 로 위젯을 만들고, 열린 동안 0.2초마다(또는 MarkDirty 뒤) Refresh 를 부른다.
 * IsAvailable 이 거짓이면 탭 단추가 숨는다(예: 저장고 탭은 상자를 열었을 때만).
 */
UCLASS(Abstract)
class GRIMOIRE_API UGrimInventoryTab : public UObject
{
	GENERATED_BODY()

public:
	/** 본문은 .cpp(약한 포인터 대입에 완전한 형이 필요 — 이 헤더만 넣은 곳에서도 빌드되게) */
	void InitializeTab(UGrimInventoryWidget* InOwner);

	virtual FName GetTabId() const PURE_VIRTUAL(UGrimInventoryTab::GetTabId, return NAME_None;);
	virtual FText GetLabel() const PURE_VIRTUAL(UGrimInventoryTab::GetLabel, return FText::GetEmpty(););
	/** 탭 단추 순서(가방 = 0) */
	virtual int32 GetOrder() const { return 100; }
	virtual bool IsAvailable() const { return true; }

	/** 탭 내용 위젯을 만든다(한 번) */
	virtual UWidget* BuildContent(UWidgetTree* Tree) PURE_VIRTUAL(UGrimInventoryTab::BuildContent, return nullptr;);

	/** 값을 다시 읽어 그린다 */
	virtual void Refresh() {}

	virtual void OnShown() {}
	virtual void OnHidden() {}

	UGrimInventoryWidget* GetOwnerWidget() const { return OwnerWidget.Get(); }
	UGrimInventoryComponent* GetInventory() const;
	APlayerController* GetOwningPlayer() const;

	void MarkDirty() { bDirty = true; }
	bool ConsumeDirty() { const bool bWas = bDirty; bDirty = false; return bWas; }

private:
	TWeakObjectPtr<UGrimInventoryWidget> OwnerWidget;
	bool bDirty = true;
};
