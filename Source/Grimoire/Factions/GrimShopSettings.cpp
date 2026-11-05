#include "Factions/GrimShopSettings.h"
#include "Factions/GrimFactionTypes.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Quest/GrimQuestTypes.h"

FName GrimTowerShop::TryBuy(UGrimInventoryComponent* Inventory, FName ItemId)
{
	const UGrimShopSettings* Shop = UGrimShopSettings::Get();
	const int32 Price = Shop->FindPrice(ItemId);
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	if (!Inventory || !Inventory->GetOwner() || !Inventory->GetOwner()->HasAuthority() || Price == INDEX_NONE || !Database.Find(ItemId))
	{
		UE_LOG(LogGrimQuest, Warning, TEXT("[Server] 상점: %s 은(는) 팔지 않는다(값 · 아이템 표 · 가방)"), *ItemId.ToString());
		return GrimDialogueContexts::Refused;
	}
	if (Inventory->CountOf(Shop->CurrencyItem) < Price)
	{
		return GrimDialogueContexts::BuyNoMoney;
	}
	if (FGrimInventoryModel::GetAddableCount(Inventory->GetSlots(), Database, ItemId) < 1)
	{
		return GrimDialogueContexts::BuyBagFull;
	}
	if (Price > 0 && Inventory->RemoveItem(Shop->CurrencyItem, Price) < Price)
	{
		return GrimDialogueContexts::BuyNoMoney;
	}
	Inventory->AddItem(FGrimItemInstance(ItemId, 1));
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 상점: %s 이(가) %s 을(를) %d %s 에 샀다"), *GetNameSafe(Inventory->GetOwner()), *ItemId.ToString(), Price, *Shop->CurrencyItem.ToString());
	return GrimDialogueContexts::Buy;
}
