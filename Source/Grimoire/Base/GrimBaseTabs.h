#pragma once

#include "CoreMinimal.h"
#include "Items/GrimInventoryTab.h"
#include "GrimBaseTabs.generated.h"

class UGrimBuilderComponent;
class UGrimIdButton;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;

/** 거점 갈래 탭 공통: 건축 컴포넌트 · 연 시설 찾기 */
UCLASS(Abstract)
class GRIMOIRE_API UGrimBaseTabBase : public UGrimInventoryTab
{
	GENERATED_BODY()

protected:
	UGrimBuilderComponent* GetBuilder() const;

	template <typename T>
	T* GetOpenStation() const;

	/** 버튼 하나(포커스 없음 — 가방 키로 닫히게) */
	UGrimIdButton* MakeButton(UWidgetTree* Tree, int32 Id, const FText& Label, float Width, float Height, UTextBlock** OutLabel = nullptr);
};

/** 장비 탭(I) — 칸 8(Q21) · 내구도 · 벗기 · 체온 보정 합 */
UCLASS()
class GRIMOIRE_API UGrimEquipmentTab : public UGrimBaseTabBase
{
	GENERATED_BODY()

public:
	virtual FName GetTabId() const override;
	virtual FText GetLabel() const override;
	virtual int32 GetOrder() const override { return 10; }
	virtual UWidget* BuildContent(UWidgetTree* Tree) override;
	virtual void Refresh() override;

	/** 한 칸 글(테스트 · 덤프) */
	static FString DescribeSlot(int32 SlotIndex, const struct FGrimItemInstance& Item);

private:
	void HandleUnequip(int32 SlotIndex);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotTexts;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SummaryText;
};

/** 저장고 탭 — 상자를 F 로 열었을 때만. 왼쪽 가방 칸을 누르면 넣고, 오른쪽 저장고 칸을 누르면 꺼낸다(통째로) */
UCLASS()
class GRIMOIRE_API UGrimStorageTab : public UGrimBaseTabBase
{
	GENERATED_BODY()

public:
	virtual FName GetTabId() const override;
	virtual FText GetLabel() const override;
	virtual int32 GetOrder() const override { return 20; }
	virtual bool IsAvailable() const override;
	virtual UWidget* BuildContent(UWidgetTree* Tree) override;
	virtual void Refresh() override;

private:
	void HandleBagClicked(int32 SlotIndex);
	void HandleChestClicked(int32 SlotIndex);
	void EnsureButtons(UUniformGridPanel* Grid, TArray<TObjectPtr<UTextBlock>>& Texts, int32 Count, bool bChest);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetTree> Tree;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> BagGrid;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> ChestGrid;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> BagTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ChestTexts;
};

/** 작업대 탭 — 레시피(Station=Workbench)를 누르면 만든다, 「수리」 걸기(장비) */
UCLASS()
class GRIMOIRE_API UGrimWorkbenchTab : public UGrimBaseTabBase
{
	GENERATED_BODY()

public:
	virtual FName GetTabId() const override;
	virtual FText GetLabel() const override;
	virtual int32 GetOrder() const override { return 30; }
	virtual bool IsAvailable() const override;
	virtual UWidget* BuildContent(UWidgetTree* Tree) override;
	virtual void Refresh() override;

private:
	void HandleRecipeClicked(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RecipeTexts;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InfoText;

	TArray<FName> RecipeIds;
};

/** 거점 탭 — 거점 핵을 F 로 열었을 때만. 조각 수 · 닳은 것 · 저장고 재료, 「거점 수리」 */
UCLASS()
class GRIMOIRE_API UGrimBaseCoreTab : public UGrimBaseTabBase
{
	GENERATED_BODY()

public:
	virtual FName GetTabId() const override;
	virtual FText GetLabel() const override;
	virtual int32 GetOrder() const override { return 40; }
	virtual bool IsAvailable() const override;
	virtual UWidget* BuildContent(UWidgetTree* Tree) override;
	virtual void Refresh() override;

private:
	void HandleRepairClicked(int32 Unused);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InfoText;
};
