#pragma once

#include "CoreMinimal.h"
#include "Items/GrimInventoryTab.h"
#include "GrimCreatureStorageTab.generated.h"

class UGrimCreatureStorageWidget;

/**
 * 소지품 화면 「생물」 탭(R38) — 데리고 다님 · 보관함 목록. 내용은 UGrimCreatureStorageWidget(상자 관리 화면과 같은 위젯)이 맡고 스스로 갱신한다.
 * 등록: ini [/Script/Grimoire.GrimItemSettings] +InventoryTabClasses=/Script/Grimoire.GrimCreatureStorageTab.
 * 순서 15: 늘 보이는 탭(가방 · 장비) 바로 뒤 — 시설을 열 때만 생기는 탭(저장고 · 작업대 · 거점)이 끼어도 자리가 바뀌지 않게.
 */
UCLASS()
class GRIMOIRE_API UGrimCreatureStorageTab : public UGrimInventoryTab
{
	GENERATED_BODY()

public:
	static const FName TabId;

	virtual FName GetTabId() const override { return TabId; }
	virtual FText GetLabel() const override;
	virtual int32 GetOrder() const override { return 15; }
	virtual UWidget* BuildContent(UWidgetTree* Tree) override;
	virtual void OnShown() override;
	virtual void OnHidden() override;

	UGrimCreatureStorageWidget* GetPage() const { return Page; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UGrimCreatureStorageWidget> Page;
};
