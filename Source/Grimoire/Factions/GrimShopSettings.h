#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimShopSettings.generated.h"

/** 상점 물건 하나의 값(솔) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimShopPrice
{
	GENERATED_BODY()

	/** 아이템 표(Items.csv)의 ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 Price = 0;
};

/**
 * 마법탑 NPC 상점(서비스 「Buy:<아이템 ID>」 — TowerNpcs.csv) — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimShopSettings].
 * 화폐는 아이템 솔리두스(Q38, 아이템 표 Solidus). 첫 물건: 「스킬북: 쿠스토디아」(생물 보관 노말 스킬, R38 · Q42).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Shop"))
class GRIMOIRE_API UGrimShopSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimShopSettings* Get() { return GetDefault<UGrimShopSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 값을 치르는 아이템(화폐) */
	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	FName CurrencyItem = TEXT("Solidus");

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TArray<FGrimShopPrice> Prices;

	/** 마법탑 대리 수리(Q121): 수리 재료 1개 몫(최대 내구도 RepairFractionPerMaterial)마다 이만큼 솔. 재료는 받지 않는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0"))
	int32 RepairPricePerMaterial = 5;

	/** 값(없으면 INDEX_NONE — 팔지 않는다) */
	int32 FindPrice(FName ItemId) const
	{
		const FGrimShopPrice* Row = Prices.FindByPredicate([ItemId](const FGrimShopPrice& P) { return P.ItemId == ItemId; });
		return Row ? Row->Price : INDEX_NONE;
	}
};

class UGrimInventoryComponent;

namespace GrimTowerShop
{
	/**
	 * 서버: 가방의 솔로 아이템 하나를 산다. 대사 맥락을 돌려준다 — Buy(샀다) · Buy.NoMoney · Buy.BagFull · Refused(팔지 않음).
	 * NPC 서비스 「Buy:<아이템 ID>」(UGrimQuestComponent::ExecuteService)가 부른다.
	 */
	GRIMOIRE_API FName TryBuy(UGrimInventoryComponent* Inventory, FName ItemId);

	/**
	 * 서버: 대리 수리(Q121 — 레스티투오를 배우지 않은 사람). 입은 장비의 닳은 만큼(재료 몫 × RepairPricePerMaterial 솔)을 받고 고친다
	 * — 수리 횟수 · 최대치 −10% 는 작업대 수리와 같다. 대사 맥락 Repair · Repair.NoMoney · Repair.Nothing. OutFee = 받은(받을) 솔.
	 */
	GRIMOIRE_API FName TryRepair(UGrimInventoryComponent* Inventory, int32* OutFee = nullptr);
}
