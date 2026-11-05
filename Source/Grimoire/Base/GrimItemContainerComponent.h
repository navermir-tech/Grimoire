#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimItemContainerComponent.generated.h"

class UGrimItemContainerComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnContainerChanged, UGrimItemContainerComponent* /*Container*/);

/**
 * 시설 인벤토리(저장고) — 칸 수 고정, 칸 계산은 가방과 같은 FGrimInventoryModel.
 * 칸은 관련성 거리 안 모두에게 복제한다(열람자만 복제는 측정 뒤 — docs/systems/m4-plan 「복제」). 바꾸는 일은 서버만(건축 컴포넌트 RPC).
 * 세이브: World 「Container.<액터 이름>」 v1(칸 수 + 칸마다 FGrimItemInstance).
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimItemContainerComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimItemContainerComponent();

	const TArray<FGrimItemInstance>& GetSlots() const { return Slots; }
	int32 GetSlotCount() const { return Slots.Num(); }
	int32 CountOf(FName ItemId) const { return FGrimInventoryModel::CountOf(Slots, ItemId); }
	bool IsEmpty() const;

	// ───── 서버 ─────

	void InitSlots(int32 Count);
	void SetSlots(const TArray<FGrimItemInstance>& NewSlots);
	int32 AddItem(const FGrimItemInstance& Item);
	int32 RemoveItem(FName ItemId, int32 Count);
	FGrimItemInstance RemoveFromSlot(int32 SlotIndex, int32 Count);
	/** 내용물을 모두 바닥에 떨군다(철거 · 부서짐) */
	void DropAll(const FVector& Location);

	FGrimOnContainerChanged OnContainerChanged;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Slots();

	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FGrimItemInstance> Slots;
};
