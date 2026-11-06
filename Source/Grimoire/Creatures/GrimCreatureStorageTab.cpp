#include "Creatures/GrimCreatureStorageTab.h"
#include "Creatures/GrimCreatureStorageWidget.h"
#include "Items/GrimInventoryWidget.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimCreatureStorageTab"

const FName UGrimCreatureStorageTab::TabId(TEXT("Creatures"));

FText UGrimCreatureStorageTab::GetLabel() const
{
	return LOCTEXT("TabCreatures", "생물");
}

UWidget* UGrimCreatureStorageTab::BuildContent(UWidgetTree* Tree)
{
	// 자기 위젯 트리 · 틱을 가진 UUserWidget 이라 소지품 화면의 트리(Tree)는 쓰지 않고 그대로 끼운다.
	if (APlayerController* PC = GetOwningPlayer())
	{
		Page = CreateWidget<UGrimCreatureStorageWidget>(PC, UGrimCreatureStorageWidget::StaticClass());
	}
	return Page;
}

void UGrimCreatureStorageTab::OnShown()
{
	if (Page)
	{
		Page->SetTabActive(true);
	}
}

void UGrimCreatureStorageTab::OnHidden()
{
	if (Page)
	{
		Page->SetTabActive(false);
	}
}

#undef LOCTEXT_NAMESPACE
