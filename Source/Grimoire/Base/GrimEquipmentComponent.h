#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Core/GrimSaveTypes.h"
#include "Base/GrimBaseTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGrimEquipmentComponent;
class UGrimInventoryComponent;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnEquipmentChanged, UGrimEquipmentComponent* /*Equipment*/);

/**
 * 장비 (R18 · Q21 · Q22, docs/systems/m4-plan). PlayerState 에 붙는다 — 거점 서브시스템(서버)이 런타임에 붙인다(이름 고정 "GrimEquipment").
 *  - 칸 8(EGrimEquipSlot). 칸은 소유 클라이언트에만 복제. 바꾸는 일은 서버 함수, 클라이언트는 Server_ RPC.
 *  - 입기: 가방의 Kind=Equipment 아이템을 「사용」 하면(가방 → 칸, 칸에 있던 것은 가방으로). 벗기: 장비 탭.
 *  - 내구도: 인스턴스의 Durability(음수 = 가득) · RepairCount. 최대 = 표의 MaxDurability × (1 − 0.1 × 수리 횟수, 하한 50%).
 *    닳기(서버만): 완드 = 마법 시전 확정(ASC AbilityCommitted), 방어구 = 체력이 깎일 때(생존 틱 제외) 피해 비례, 장신구 = 착용 시간.
 *    0 → 완드는 「부러진 완드」(Q22 — Status.WandBroken: 마법 불가 · 주먹만, 수리로 되살림), 방어구 · 장신구는 사라지고 수리 재료를 조금 돌려준다.
 *  - 완드 칸이 비면(Q71) Status.WandMissing — 부러짐과 같이 마법 불가 · 주먹만. 처음 들어오면 StartingEquipment(기본 완드)를 입힌다.
 *  - 체온 보정: 장비의 ColdResist · HeatResist 합을 무한 GE(SetByCaller) 하나로 건다.
 *  - 수리: RepairAll — 「수리」 마법을 작업대에 걸 때(가방의 재료로).
 *  - 세이브: Player 「Base.Equipment」 v2(칸 수 + FGrimItemInstance — v1 과 형식이 같고, v1 을 읽으면 빈 칸에 시작 장비를 넣는다).
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimEquipmentComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimEquipmentComponent();

	static const FName ComponentName;

	static UGrimEquipmentComponent* FindForActor(const AActor* Actor);

	const TArray<FGrimItemInstance>& GetSlots() const { return Slots; }
	const FGrimItemInstance& GetSlot(EGrimEquipSlot Slot) const;

	/** 인스턴스의 지금 · 최대 내구도(표에 없으면 -1) */
	static float GetDurability(const FGrimItemInstance& Item);
	static float GetMaxDurability(const FGrimItemInstance& Item);

	bool IsWandBroken() const;

	/** 완드 칸이 비었다(Q71 — 주먹질) */
	bool IsWandMissing() const;

	// ───── 서버 ─────

	/** 가방 칸의 장비를 입는다(그 칸에 있던 것은 가방으로) */
	bool EquipFromBag(int32 BagSlot);

	/** 벗어 가방에 넣는다(가방이 차면 못 벗는다) */
	bool Unequip(EGrimEquipSlot Slot);

	/** 그 칸을 닳게 한다 */
	void Wear(EGrimEquipSlot Slot, float Amount);

	/** 닳은 장비를 모두 가방의 재료로 고친다. 쓴 재료 수. */
	int32 RepairAll(UGrimInventoryComponent* Bag);

	/** 빈 칸에 시작 장비(UGrimBaseSettings::StartingEquipment)를 넣는다. 넣은 수 */
	int32 GiveStartingEquipment();

	/** 칸에 바로 넣는다(콘솔 · 테스트 · 세이브) */
	void SetSlotItem(EGrimEquipSlot Slot, const FGrimItemInstance& Item);

	UFUNCTION(Server, Reliable)
	void Server_Unequip(uint8 Slot);

	FGrimOnEquipmentChanged OnEquipmentChanged;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Base.Equipment"); }
	virtual int32 GetSaveVersion() const override { return 2; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Slots();

private:
	UAbilitySystemComponent* GetASC() const;
	void BindASC();
	void HandleAbilityCommitted(UGameplayAbility* Ability);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void TickAccessories();
	void CommitChange();
	void RefreshEffects();
	void BreakSlot(EGrimEquipSlot Slot);

	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FGrimItemInstance> Slots;

	FActiveGameplayEffectHandle ResistHandle;
	FDelegateHandle CommittedHandle;
	FDelegateHandle HealthHandle;
	FTimerHandle AccessoryTimer;
	FTimerHandle BindTimer;
};
