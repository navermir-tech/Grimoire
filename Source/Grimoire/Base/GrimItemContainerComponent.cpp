#include "Base/GrimItemContainerComponent.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UGrimItemContainerComponent::UGrimItemContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimItemContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimItemContainerComponent, Slots);
}

void UGrimItemContainerComponent::OnRep_Slots()
{
	OnContainerChanged.Broadcast(this);
}

bool UGrimItemContainerComponent::IsEmpty() const
{
	for (const FGrimItemInstance& Slot : Slots)
	{
		if (!Slot.IsEmpty())
		{
			return false;
		}
	}
	return true;
}

void UGrimItemContainerComponent::InitSlots(int32 Count)
{
	if (Slots.Num() != Count)
	{
		TArray<FGrimItemInstance> NewSlots = Slots;
		NewSlots.SetNum(FMath::Max(1, Count));
		SetSlots(NewSlots);
	}
}

void UGrimItemContainerComponent::SetSlots(const TArray<FGrimItemInstance>& NewSlots)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->FlushNetDormancy();
	}
	Slots = NewSlots;
	OnContainerChanged.Broadcast(this);
}

int32 UGrimItemContainerComponent::AddItem(const FGrimItemInstance& Item)
{
	TArray<FGrimItemInstance> NewSlots = Slots;
	const int32 Added = FGrimInventoryModel::Add(NewSlots, UGrimItemSettings::GetDatabase(), Item);
	if (Added > 0)
	{
		SetSlots(NewSlots);
	}
	return Added;
}

int32 UGrimItemContainerComponent::RemoveItem(FName ItemId, int32 Count)
{
	TArray<FGrimItemInstance> NewSlots = Slots;
	const int32 Removed = FGrimInventoryModel::Remove(NewSlots, ItemId, Count);
	if (Removed > 0)
	{
		SetSlots(NewSlots);
	}
	return Removed;
}

FGrimItemInstance UGrimItemContainerComponent::RemoveFromSlot(int32 SlotIndex, int32 Count)
{
	TArray<FGrimItemInstance> NewSlots = Slots;
	FGrimItemInstance Removed = FGrimInventoryModel::RemoveAt(NewSlots, SlotIndex, Count);
	if (!Removed.IsEmpty())
	{
		SetSlots(NewSlots);
	}
	return Removed;
}

void UGrimItemContainerComponent::DropAll(const FVector& Location)
{
	for (const FGrimItemInstance& Slot : Slots)
	{
		if (!Slot.IsEmpty())
		{
			UGrimItemStatics::SpawnPickup(GetOwner(), Slot.ItemId, Slot.Count, Location + FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 0.f));
		}
	}
	TArray<FGrimItemInstance> Empty;
	Empty.SetNum(Slots.Num());
	SetSlots(Empty);
}

// ───── 세이브 (Container.<이름> v1) ─────

FName UGrimItemContainerComponent::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Container.%s"), GetOwner() ? *GetOwner()->GetName() : TEXT("None")));
}

void UGrimItemContainerComponent::WriteSaveData(FArchive& Ar)
{
	int32 Count = Slots.Num();
	Ar << Count;
	for (FGrimItemInstance& Slot : Slots)
	{
		Slot.Serialize(Ar);
	}
}

void UGrimItemContainerComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	int32 Count = 0;
	Ar << Count;
	Count = FMath::Clamp(Count, 0, 500);
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	TArray<FGrimItemInstance> Loaded;
	for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index)
	{
		FGrimItemInstance Item;
		Item.Serialize(Ar);
		// 표에 없는 아이템은 버린다(가방과 같은 규칙).
		Loaded.Add(Database.Find(Item.ItemId) ? Item : FGrimItemInstance());
	}
	if (Ar.IsError())
	{
		return;
	}
	Loaded.SetNum(FMath::Max(Slots.Num(), Loaded.Num()));
	SetSlots(Loaded);
}
