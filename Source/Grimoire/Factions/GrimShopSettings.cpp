#include "Factions/GrimShopSettings.h"
#include "Base/GrimEquipmentComponent.h"
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

FName GrimTowerShop::TryRepair(UGrimInventoryComponent* Inventory, int32* OutFee)
{
	const UGrimShopSettings* Shop = UGrimShopSettings::Get();
	UGrimEquipmentComponent* Equipment = Inventory ? UGrimEquipmentComponent::FindForActor(Inventory->GetOwner()) : nullptr;
	if (!Equipment || !Inventory->GetOwner()->HasAuthority())
	{
		return GrimDialogueContexts::Refused;
	}
	const int32 Units = Equipment->GetRepairUnitsNeeded();
	const int32 Fee = Units * Shop->RepairPricePerMaterial;
	if (OutFee)
	{
		*OutFee = Fee;
	}
	if (Units <= 0)
	{
		return GrimDialogueContexts::RepairNothing;
	}
	if (Inventory->CountOf(Shop->CurrencyItem) < Fee || (Fee > 0 && Inventory->RemoveItem(Shop->CurrencyItem, Fee) < Fee))
	{
		return GrimDialogueContexts::RepairNoMoney;
	}
	const int32 Repaired = Equipment->RepairAllWithoutMaterials();
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 대리 수리: %s 재료 몫 %d → %d %s"), *GetNameSafe(Inventory->GetOwner()), Repaired, Fee, *Shop->CurrencyItem.ToString());
	return GrimDialogueContexts::Repair;
}
