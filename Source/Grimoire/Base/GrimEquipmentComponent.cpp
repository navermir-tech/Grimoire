#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimBaseEffects.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseTags.h"
#include "Core/GrimPlayerState.h"
#include "Core/Cheats/GrimCheatTags.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Magic/GrimSpellAbility.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FName UGrimEquipmentComponent::ComponentName(TEXT("GrimEquipment"));

namespace GrimEquipmentLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FGrimItemInstance EmptyEquipmentSlot;

	bool IsArmorSlot(EGrimEquipSlot Slot)
	{
		return Slot >= EGrimEquipSlot::Head && Slot <= EGrimEquipSlot::Feet;
	}

	bool IsAccessorySlot(EGrimEquipSlot Slot)
	{
		return Slot == EGrimEquipSlot::Ring || Slot == EGrimEquipSlot::Necklace;
	}
}

UGrimEquipmentComponent::UGrimEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Slots.SetNum(static_cast<int32>(EGrimEquipSlot::Count));
}

UGrimEquipmentComponent* UGrimEquipmentComponent::FindForActor(const AActor* Actor)
{
	const APlayerState* PlayerState = Cast<APlayerState>(Actor);
	if (!PlayerState)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			PlayerState = Pawn->GetPlayerState();
		}
		else if (const AController* Controller = Cast<AController>(Actor))
		{
			PlayerState = Controller->PlayerState;
		}
	}
	return PlayerState ? PlayerState->FindComponentByClass<UGrimEquipmentComponent>() : nullptr;
}

void UGrimEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimEquipmentComponent, Slots, COND_OwnerOnly);
}

void UGrimEquipmentComponent::OnRep_Slots()
{
	Slots.SetNum(static_cast<int32>(EGrimEquipSlot::Count));
	OnEquipmentChanged.Broadcast(this);
}

const FGrimItemInstance& UGrimEquipmentComponent::GetSlot(EGrimEquipSlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Slots.IsValidIndex(Index) ? Slots[Index] : GrimEquipmentLocal::EmptyEquipmentSlot;
}

float UGrimEquipmentComponent::GetMaxDurability(const FGrimItemInstance& Item)
{
	const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Item.ItemId);
	if (!Def)
	{
		return -1.f;
	}
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	return FGrimDurabilityMath::GetEquipmentMax(Def->Row.MaxDurability, Item.RepairCount, Settings->EquipmentMaxLossPerRepair, Settings->EquipmentMinMaxFraction);
}

float UGrimEquipmentComponent::GetDurability(const FGrimItemInstance& Item)
{
	const float Max = GetMaxDurability(Item);
	if (Max < 0.f)
	{
		return -1.f;
	}
	// 음수 = 새것(가득) — 만들 때 · 콘솔로 받을 때 따로 정하지 않아도 된다.
	return Item.Durability < 0.f ? Max : FMath::Min(Item.Durability, Max);
}

bool UGrimEquipmentComponent::IsWandBroken() const
{
	const FGrimItemInstance& Wand = GetSlot(EGrimEquipSlot::Wand);
	return !Wand.IsEmpty() && GetDurability(Wand) <= 0.f;
}

bool UGrimEquipmentComponent::IsWandMissing() const
{
	return GetSlot(EGrimEquipSlot::Wand).IsEmpty();
}

int32 UGrimEquipmentComponent::GiveStartingEquipment()
{
	int32 Given = 0;
	for (const FName& ItemId : UGrimBaseSettings::Get()->StartingEquipment)
	{
		const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(ItemId);
		const int32 Index = Def ? static_cast<int32>(Def->Row.Slot) : INDEX_NONE;
		if (!Slots.IsValidIndex(Index))
		{
			UE_LOG(LogGrimBase, Warning, TEXT("[Server] 시작 장비 %s 가 장비 표에 없다"), *ItemId.ToString());
			continue;
		}
		if (Slots[Index].IsEmpty())
		{
			Slots[Index] = FGrimItemInstance(ItemId, 1);
			++Given;
		}
	}
	if (Given > 0)
	{
		UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 시작 장비 %d개를 입혔다"), *GetNameSafe(GetOwner()), Given);
	}
	return Given;
}

UAbilitySystemComponent* UGrimEquipmentComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

void UGrimEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	Slots.SetNum(static_cast<int32>(EGrimEquipSlot::Count));
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// 처음 들어온 플레이어의 시작 장비(Q71 기본 완드). 세이브가 있으면 곧 ReadSaveData 가 칸을 통째로 바꾼다.
	GiveStartingEquipment();
	BindASC();
	if (!CommittedHandle.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(BindTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			BindASC();
			if (CommittedHandle.IsValid())
			{
				GetWorld()->GetTimerManager().ClearTimer(BindTimer);
			}
		}), 0.5f, true);
	}
	GetWorld()->GetTimerManager().SetTimer(AccessoryTimer, this, &ThisClass::TickAccessories, UGrimBaseSettings::Get()->AccessoryWearIntervalSeconds, true);
}

void UGrimEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AccessoryTimer);
		World->GetTimerManager().ClearTimer(BindTimer);
	}
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AbilityCommittedCallbacks.Remove(CommittedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimEquipmentComponent::BindASC()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || CommittedHandle.IsValid())
	{
		return;
	}
	CommittedHandle = ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleAbilityCommitted);
	HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	RefreshEffects();
}

void UGrimEquipmentComponent::HandleAbilityCommitted(UGameplayAbility* Ability)
{
	// 완드는 시전할 때 닳는다(기본 공격 포함 — 마법 어빌리티면).
	if (!GetOwner()->HasAuthority() || !Cast<UGrimSpellAbility>(Ability))
	{
		return;
	}
	const FGrimItemInstance& Wand = GetSlot(EGrimEquipSlot::Wand);
	if (const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Wand.ItemId))
	{
		Wear(EGrimEquipSlot::Wand, Def->Row.WearPerUse);
	}
}

void UGrimEquipmentComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!GetOwner()->HasAuthority() || Data.NewValue >= Data.OldValue)
	{
		return;
	}
	// 배고픔 · 체온 같은 생존 틱으로 줄어든 것은 「맞은」 것이 아니다.
	if (Data.GEModData)
	{
		const UGameplayEffect* Def = Data.GEModData->EffectSpec.Def;
		if (Def && (Def->IsA<UGrimSurvivalTickEffect>() || Def->IsA<UGrimSurvivalExtraTickEffect>() || Def->IsA<UGrimSurvivalPenaltyEffect>()))
		{
			return;
		}
	}
	const float Damage = Data.OldValue - Data.NewValue;
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	for (int32 Index = static_cast<int32>(EGrimEquipSlot::Head); Index <= static_cast<int32>(EGrimEquipSlot::Feet); ++Index)
	{
		if (const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Slots[Index].ItemId))
		{
			Wear(static_cast<EGrimEquipSlot>(Index), FGrimDurabilityMath::GetArmorWear(Damage, Def->Row.WearPerUse, Settings->ArmorWearDamageUnit));
		}
	}
}

void UGrimEquipmentComponent::TickAccessories()
{
	for (const EGrimEquipSlot Slot : { EGrimEquipSlot::Ring, EGrimEquipSlot::Necklace })
	{
		if (const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(GetSlot(Slot).ItemId))
		{
			Wear(Slot, Def->Row.WearPerUse);
		}
	}
}

void UGrimEquipmentComponent::Wear(EGrimEquipSlot Slot, float Amount)
{
	const int32 Index = static_cast<int32>(Slot);
	if (!GetOwner()->HasAuthority() || !Slots.IsValidIndex(Index) || Slots[Index].IsEmpty() || Amount <= 0.f)
	{
		return;
	}
	if (const UAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super))
	{
		return;
	}
	const float Current = GetDurability(Slots[Index]);
	if (Current <= 0.f)
	{
		return;
	}
	const float Next = FMath::Max(0.f, Current - Amount);
	Slots[Index].Durability = Next;
	if (Next <= 0.f)
	{
		BreakSlot(Slot);
	}
	CommitChange();
}

void UGrimEquipmentComponent::BreakSlot(EGrimEquipSlot Slot)
{
	const int32 Index = static_cast<int32>(Slot);
	const FGrimItemInstance Broken = Slots[Index];
	if (Slot == EGrimEquipSlot::Wand)
	{
		// Q22: 사라지지 않고 「부러진 완드」로 남는다 — 수리로 되살린다.
		UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 의 완드 %s 가 부러졌다"), *GetNameSafe(GetOwner()), *Broken.ItemId.ToString());
		return;
	}
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 의 장비 %s 가 부서졌다"), *GetNameSafe(GetOwner()), *Broken.ItemId.ToString());
	Slots[Index].Clear();
	const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Broken.ItemId);
	const int32 Salvage = UGrimBaseSettings::Get()->BrokenEquipmentSalvage;
	if (Def && !Def->Row.RepairItem.IsNone() && Salvage > 0)
	{
		UGrimInventoryComponent* Bag = UGrimInventoryComponent::FindForActor(GetOwner());
		const int32 Added = Bag ? Bag->AddItem(FGrimItemInstance(Def->Row.RepairItem, Salvage), true) : 0;
		const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
		if (Added < Salvage && PlayerState && PlayerState->GetPawn())
		{
			UGrimItemStatics::SpawnPickup(GetOwner(), Def->Row.RepairItem, Salvage - Added, PlayerState->GetPawn()->GetActorLocation());
		}
	}
}

void UGrimEquipmentComponent::CommitChange()
{
	RefreshEffects();
	OnEquipmentChanged.Broadcast(this);
}

void UGrimEquipmentComponent::RefreshEffects()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}
	float Cold = 0.f;
	float Heat = 0.f;
	for (const FGrimItemInstance& Item : Slots)
	{
		const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Item.ItemId);
		if (Def && GetDurability(Item) > 0.f)
		{
			Cold += Def->Row.ColdResist;
			Heat += Def->Row.HeatResist;
		}
	}
	if (ResistHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ResistHandle);
		ResistHandle.Invalidate();
	}
	if (!FMath::IsNearlyZero(Cold) || !FMath::IsNearlyZero(Heat))
	{
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimEquipmentResistEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(UGrimEquipmentResistEffect::ColdResistMagnitudeName, Cold);
			Spec.Data->SetSetByCallerMagnitude(UGrimEquipmentResistEffect::HeatResistMagnitudeName, Heat);
			ResistHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	// 부러진 완드(Q22) · 빈 완드 칸(Q71) — 복제 루스 태그(클라이언트의 시전 예측도 막힌다).
	ASC->SetLooseGameplayTagCount(GrimBaseTags::Status_WandBroken, IsWandBroken() ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	ASC->SetLooseGameplayTagCount(GrimBaseTags::Status_WandMissing, IsWandMissing() ? 1 : 0, EGameplayTagReplicationState::TagOnly);
}

bool UGrimEquipmentComponent::EquipFromBag(int32 BagSlot)
{
	UGrimInventoryComponent* Bag = UGrimInventoryComponent::FindForActor(GetOwner());
	if (!GetOwner()->HasAuthority() || !Bag || !Bag->GetSlots().IsValidIndex(BagSlot))
	{
		return false;
	}
	const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Bag->GetSlots()[BagSlot].ItemId);
	if (!Def)
	{
		return false;
	}
	const int32 Index = static_cast<int32>(Def->Row.Slot);
	const FGrimItemInstance Taken = Bag->RemoveFromSlot(BagSlot, 1);
	if (Taken.IsEmpty())
	{
		return false;
	}
	const FGrimItemInstance Previous = Slots[Index];
	Slots[Index] = Taken;
	Slots[Index].Count = 1;
	if (!Previous.IsEmpty() && Bag->AddItem(Previous, false) < 1)
	{
		const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
		if (PlayerState && PlayerState->GetPawn())
		{
			UGrimItemStatics::SpawnPickup(GetOwner(), Previous.ItemId, 1, PlayerState->GetPawn()->GetActorLocation());
			UE_LOG(LogGrimBase, Warning, TEXT("[Server] 가방이 차서 벗은 %s 를 바닥에 놓았다"), *Previous.ItemId.ToString());
		}
	}
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 입음: %s(%s)"), *GetNameSafe(GetOwner()), *Taken.ItemId.ToString(), *StaticEnum<EGrimEquipSlot>()->GetNameStringByIndex(Index));
	CommitChange();
	return true;
}

bool UGrimEquipmentComponent::Unequip(EGrimEquipSlot Slot)
{
	const int32 Index = static_cast<int32>(Slot);
	UGrimInventoryComponent* Bag = UGrimInventoryComponent::FindForActor(GetOwner());
	if (!GetOwner()->HasAuthority() || !Bag || !Slots.IsValidIndex(Index) || Slots[Index].IsEmpty())
	{
		return false;
	}
	if (Bag->AddItem(Slots[Index], false) < 1)
	{
		Bag->SendNotice(EGrimItemNotice::BagFull, Slots[Index].ItemId, 1);
		return false;
	}
	Slots[Index].Clear();
	CommitChange();
	return true;
}

void UGrimEquipmentComponent::Server_Unequip_Implementation(uint8 Slot)
{
	if (Slot < static_cast<uint8>(EGrimEquipSlot::Count))
	{
		Unequip(static_cast<EGrimEquipSlot>(Slot));
	}
}

void UGrimEquipmentComponent::SetSlotItem(EGrimEquipSlot Slot, const FGrimItemInstance& Item)
{
	const int32 Index = static_cast<int32>(Slot);
	if (Slots.IsValidIndex(Index))
	{
		Slots[Index] = Item;
		CommitChange();
	}
}

int32 UGrimEquipmentComponent::RepairAll(UGrimInventoryComponent* Bag)
{
	if (!GetOwner()->HasAuthority() || !Bag)
	{
		return 0;
	}
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	int32 Used = 0;
	for (FGrimItemInstance& Item : Slots)
	{
		const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Item.ItemId);
		if (!Def || Def->Row.RepairItem.IsNone())
		{
			continue;
		}
		const float Current = GetDurability(Item);
		const int32 Needed = FGrimDurabilityMath::GetMaterialsNeeded(Current, GetMaxDurability(Item), Settings->RepairFractionPerMaterial);
		const int32 Use = FMath::Min(Needed, Bag->CountOf(Def->Row.RepairItem));
		if (Use <= 0)
		{
			continue;
		}
		Bag->RemoveItem(Def->Row.RepairItem, Use);
		// 고칠 때마다 최대치가 준다(인슈라오디드처럼 결국 새로 만들게).
		++Item.RepairCount;
		const float NewMax = GetMaxDurability(Item);
		Item.Durability = FGrimDurabilityMath::Repair(FMath::Min(Current, NewMax), NewMax, Use, Settings->RepairFractionPerMaterial);
		Used += Use;
		UE_LOG(LogGrimBase, Log, TEXT("[Server] 장비 수리 %s: 재료 %s ×%d → %.0f/%.0f(수리 %d번)"), *Item.ItemId.ToString(), *Def->Row.RepairItem.ToString(), Use, Item.Durability, NewMax, Item.RepairCount);
	}
	if (Used > 0)
	{
		CommitChange();
	}
	return Used;
}

// ───── 세이브 (Base.Equipment v2 — 형식은 v1 그대로) ─────

void UGrimEquipmentComponent::WriteSaveData(FArchive& Ar)
{
	int32 Count = Slots.Num();
	Ar << Count;
	for (FGrimItemInstance& Item : Slots)
	{
		Item.Serialize(Ar);
	}
}

void UGrimEquipmentComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	int32 Count = 0;
	Ar << Count;
	Count = FMath::Clamp(Count, 0, 64);
	TArray<FGrimItemInstance> Loaded;
	Loaded.SetNum(static_cast<int32>(EGrimEquipSlot::Count));
	for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index)
	{
		FGrimItemInstance Item;
		Item.Serialize(Ar);
		const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(Item.ItemId);
		// 표에서 칸이 바뀌었으면 그 칸으로, 없어진 장비는 버린다.
		if (Def && Loaded.IsValidIndex(static_cast<int32>(Def->Row.Slot)))
		{
			Loaded[static_cast<int32>(Def->Row.Slot)] = Item;
		}
	}
	if (!Ar.IsError())
	{
		Slots = Loaded;
		// v1 세이브는 완드 칸이 비어도 보이지 않는 기본 완드로 시전했다(Q71 전) — 주먹만 남지 않게 시작 장비를 넣는다.
		if (Version < 2)
		{
			GiveStartingEquipment();
		}
		CommitChange();
	}
}
