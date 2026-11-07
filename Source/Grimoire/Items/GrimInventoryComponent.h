#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Core/GrimSaveTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimInventoryComponent.generated.h"

class AGrimCookingPot;
class AGrimItemPickup;
class APawn;
class APlayerController;
class UGrimInventoryComponent;
class UGrimInventoryWidget;

/** 소유 클라이언트에 보내는 가방 알림(화면 한 줄) */
UENUM()
enum class EGrimItemNotice : uint8
{
	Obtained,
	BagFull,
	Eaten,
	GotSick,
	Learned,
	CannotUse,
	Dropped,
	IngredientsAdded,
	NothingToAdd,
	TookBack,
	/** 장비를 입었다(M4) */
	Equipped,
	/** 서클이 모자라 스킬북을 못 읽는다(Q64) — Count = 필요한 서클 */
	CircleTooLow,
};

UENUM(BlueprintType)
enum class EGrimItemUseResult : uint8
{
	Eaten,
	Learned,
	ProficiencyGained,
	/** 장비를 입었다(M4 — UGrimEquipmentComponent) */
	Equipped,
	CannotUse,
	Invalid,
};

/** 가방 한 칸(FastArray 항목). 칸 번호를 들고 있다 — 클라이언트에서 배열 순서가 바뀌어도 칸이 맞게. */
USTRUCT()
struct GRIMOIRE_API FGrimInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	FGrimItemInstance Item;

	void PostReplicatedAdd(const struct FGrimInventoryList& InArraySerializer);
	void PostReplicatedChange(const struct FGrimInventoryList& InArraySerializer);
	void PreReplicatedRemove(const struct FGrimInventoryList& InArraySerializer);
};

USTRUCT()
struct GRIMOIRE_API FGrimInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGrimInventoryEntry> Entries;

	/** 복제 콜백을 받을 컴포넌트 */
	UPROPERTY(NotReplicated)
	TObjectPtr<UGrimInventoryComponent> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGrimInventoryEntry, FGrimInventoryList>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGrimInventoryList> : public TStructOpsTypeTraitsBase2<FGrimInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnInventoryChanged, UGrimInventoryComponent* /*Inventory*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FGrimOnItemNotice, UGrimInventoryComponent* /*Inventory*/, EGrimItemNotice /*Notice*/, FName /*ItemId*/, int32 /*Count*/);

/**
 * 플레이어 가방 (docs/systems/building 「인벤토리 · 아이템」). PlayerState 에 붙는다 — Core 를 고치지 않으려고
 * UGrimItemSubsystem(서버)이 런타임에 붙인다(복제되는 동적 컴포넌트, 이름 고정 "GrimInventory" — 퀘스트 컴포넌트와 같은 방식).
 *  - 칸 수 고정(UGrimItemSettings::PlayerSlotCount). 칸 계산은 FGrimInventoryModel.
 *  - 칸은 FastArray 로 소유 클라이언트에게만 복제. 바꾸는 일은 서버 함수만, 클라이언트는 Server_ RPC 로 청한다.
 *  - 줍기 · 버리기 · 먹기 · 스킬북 읽기 · 냄비에 재료 넣기. 퀘스트 이벤트 Item.Obtained.<ID · 묶음> · Food.Eaten · Cooking.IngredientsAdded.
 *  - 세이브: IGrimSaveParticipant(Player, "Items.Inventory" v1).
 *  - 로컬 플레이어 쪽에서는 가방 화면 · 줍기 안내 위젯(UGrimInventoryWidget)을 띄운다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimInventoryComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimInventoryComponent();

	/** 런타임에 붙는 이름(서버 · 클라이언트 같은 이름이어야 복제가 잇는다) */
	static const FName ComponentName;

	/** 폰 · 컨트롤러 · PlayerState 어느 것이든 */
	static UGrimInventoryComponent* FindForActor(const AActor* Actor);

	// ───── 읽기 (서버 · 소유 클라이언트) ─────

	int32 GetSlotCount() const { return Slots.Num(); }

	/** 칸 순서대로(빈 칸 = IsEmpty) */
	const TArray<FGrimItemInstance>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Items")
	int32 CountOf(FName ItemId) const;

	APawn* GetPawn() const;

	// ───── 서버 ─────

	/** 넣는다. 넣은 개수를 돌려준다(가방이 차면 나머지는 못 넣음). bNotify 면 Item.Obtained 이벤트 · 알림. */
	int32 AddItem(const FGrimItemInstance& Item, bool bNotify = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Items")
	int32 AddItemById(FName ItemId, int32 Count) { return AddItem(FGrimItemInstance(ItemId, Count)); }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Items")
	int32 RemoveItem(FName ItemId, int32 Count);

	FGrimItemInstance RemoveFromSlot(int32 SlotIndex, int32 Count);

	bool MoveItem(int32 From, int32 To);

	/** 쓴다: 음식 = 먹기, 스킬북 = 읽기. 재료 · 도구는 쓸 수 없다. */
	EGrimItemUseResult UseItem(int32 SlotIndex);

	/** 폰 앞 바닥에 떨군다. */
	AGrimItemPickup* DropItem(int32 SlotIndex, int32 Count);

	/** 줍는다(가방에 들어간 만큼). bCheckReach 면 거리 검사. 주운 개수. */
	int32 PickUp(AGrimItemPickup* Pickup, bool bCheckReach = true);

	/** 냄비에 재료를 넣는다(넣을 게 없고 냄비가 끓지 않으면 내용물을 꺼낸다). */
	bool InteractWithPot(AGrimCookingPot* Pot, bool bCheckReach = true);

	/** 서버 · 소유 클라이언트에 알림을 보낸다 */
	void SendNotice(EGrimItemNotice Notice, FName ItemId, int32 Count);

	/** 서버: 퀘스트 이벤트(이 플레이어) */
	void NotifyQuest(FName EventId, int32 Amount = 1) const;

	// ───── RPC ─────

	UFUNCTION(Server, Reliable)
	void Server_MoveItem(int32 From, int32 To);

	UFUNCTION(Server, Reliable)
	void Server_UseItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_DropItem(int32 SlotIndex, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_PickUp(AGrimItemPickup* Pickup);

	UFUNCTION(Server, Reliable)
	void Server_InteractPot(AGrimCookingPot* Pot);

	/** 소유 클라이언트가 가방 화면을 열었다 — 튜토리얼 「가방 열기」(퀘스트 이벤트 UI.Inventory.Opened) */
	UFUNCTION(Server, Reliable)
	void Server_NotifyBagOpened();

	/** 콘솔 명령을 서버에서(개발 빌드만 동작). docs/systems/building 「콘솔」 */
	UFUNCTION(Server, Reliable)
	void ServerDebugCommand(const FString& Command, const FString& Arg, int32 A, int32 B);

	// ───── 이벤트 (서버 · 소유 클라이언트) ─────

	FGrimOnInventoryChanged OnInventoryChanged;
	FGrimOnItemNotice OnNotice;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Items.Inventory"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: int32 칸 수, FGrimItemInstance::Serialize × 칸 수. 읽을 때 표에 없는 아이템은 버리고, 칸이 줄었으면 넘치는 것은 빈 칸에 넣어 본다. */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 복제 콜백(FGrimInventoryEntry)에서 부른다 */
	void HandleReplicatedChange();

	/** 로컬 소유 컨트롤러(리슨 호스트 · 소유 클라이언트). 없으면 nullptr */
	APlayerController* GetLocalOwningController() const;

	/** 로컬 쪽 가방 화면(없으면 nullptr) — 거점 시설이 탭을 연다 */
	UGrimInventoryWidget* GetWidget() const { return Widget; }

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Client, Reliable)
	void Client_Notice(EGrimItemNotice Notice, FName ItemId, int32 Count);

private:
	/** 서버: 칸 수만큼 항목을 만든다(처음 한 번) */
	void EnsureEntries();
	/** 서버: 새 칸 배열을 FastArray 에 쓴다(바뀐 칸만 dirty) */
	void CommitSlots(const TArray<FGrimItemInstance>& NewSlots);
	/** FastArray → 칸 배열(클라이언트) */
	void RebuildSlotsFromList();

	EGrimItemUseResult EatFood(int32 SlotIndex, const FGrimItemDefinition& Def);
	EGrimItemUseResult ReadSkillbook(int32 SlotIndex, const FGrimItemDefinition& Def);

	bool HasAuthority() const;
	bool EnsureWidget();

	UPROPERTY(Replicated)
	FGrimInventoryList List;

	/** 칸 순서대로 본 것(서버 · 클라이언트) */
	TArray<FGrimItemInstance> Slots;

	UPROPERTY(Transient)
	TObjectPtr<UGrimInventoryWidget> Widget;

	FTimerHandle WidgetTimer;
};
