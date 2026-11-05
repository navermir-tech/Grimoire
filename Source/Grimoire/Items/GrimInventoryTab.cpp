#include "Items/GrimInventoryTab.h"
#include "Items/GrimInventoryWidget.h"

UGrimInventoryComponent* UGrimInventoryTab::GetInventory() const
{
	const UGrimInventoryWidget* Owner = OwnerWidget.Get();
	return Owner ? Owner->GetInventory() : nullptr;
}

APlayerController* UGrimInventoryTab::GetOwningPlayer() const
{
	const UGrimInventoryWidget* Owner = OwnerWidget.Get();
	return Owner ? Owner->GetOwningPlayer() : nullptr;
}
