#include "Items/GrimInventoryComponent.h"
#include "Items/GrimCookingPot.h"
#include "Items/GrimInventoryWidget.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalObject.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FName UGrimInventoryComponent::ComponentName(TEXT("GrimInventory"));

// ───── FastArray 콜백 ─────

void FGrimInventoryEntry::PostReplicatedAdd(const FGrimInventoryList& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleReplicatedChange();
	}
}

void FGrimInventoryEntry::PostReplicatedChange(const FGrimInventoryList& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleReplicatedChange();
	}
}

void FGrimInventoryEntry::PreReplicatedRemove(const FGrimInventoryList& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleReplicatedChange();
	}
}

// ───── 기본 ─────

UGrimInventoryComponent::UGrimInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
	List.Owner = this;
}

void UGrimInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	List.Owner = this;
}

void UGrimInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimInventoryComponent, List, COND_OwnerOnly);
}

UGrimInventoryComponent* UGrimInventoryComponent::FindForActor(const AActor* Actor)
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
	return PlayerState ? PlayerState->FindComponentByClass<UGrimInventoryComponent>() : nullptr;
}

bool UGrimInventoryComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

APawn* UGrimInventoryComponent::GetPawn() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

int32 UGrimInventoryComponent::CountOf(FName ItemId) const
{
	return FGrimInventoryModel::CountOf(Slots, ItemId);
}

void UGrimInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	List.Owner = this;
	if (HasAuthority())
	{
		EnsureEntries();
	}
	if (GetNetMode() != NM_DedicatedServer && !EnsureWidget())
	{
		// 클라이언트에서는 컨트롤러의 PlayerState 가 늦게 잇기도 한다 — 만들 수 있을 때까지 가끔 본다.
		GetWorld()->GetTimerManager().SetTimer(WidgetTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (EnsureWidget())
			{
				GetWorld()->GetTimerManager().ClearTimer(WidgetTimer);
			}
		}), 0.5f, true);
	}
}

void UGrimInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WidgetTimer);
	}
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* UGrimInventoryComponent::GetLocalOwningController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController() && PlayerController->PlayerState == GetOwner())
		{
			return PlayerController;
		}
	}
	return nullptr;
}

bool UGrimInventoryComponent::EnsureWidget()
{
	if (Widget)
	{
		return true;
	}
	APlayerController* PlayerController = GetLocalOwningController();
	if (!PlayerController)
	{
		return false;
	}
	Widget = CreateWidget<UGrimInventoryWidget>(PlayerController, UGrimInventoryWidget::StaticClass());
	if (!Widget)
	{
		return false;
	}
	Widget->SetInventory(this);
	Widget->AddToViewport(8);
	return true;
}

// ───── 칸 ─────

void UGrimInventoryComponent::EnsureEntries()
{
	if (!HasAuthority())
	{
		return;
	}
	const int32 Desired = FMath::Max(UGrimItemSettings::Get()->PlayerSlotCount, List.Entries.Num());
	while (List.Entries.Num() < Desired)
	{
		FGrimInventoryEntry& Entry = List.Entries.AddDefaulted_GetRef();
		Entry.SlotIndex = List.Entries.Num() - 1;
		List.MarkItemDirty(Entry);
	}
	if (Slots.Num() != List.Entries.Num())
	{
		Slots.SetNum(List.Entries.Num());
		for (const FGrimInventoryEntry& Entry : List.Entries)
		{
			Slots[Entry.SlotIndex] = Entry.Item;
		}
	}
}

void UGrimInventoryComponent::CommitSlots(const TArray<FGrimItemInstance>& NewSlots)
{
	EnsureEntries();
	bool bChanged = false;
	for (int32 Index = 0; Index < List.Entries.Num() && Index < NewSlots.Num(); ++Index)
	{
		FGrimInventoryEntry& Entry = List.Entries[Index];
		if (Entry.Item != NewSlots[Index])
		{
			Entry.Item = NewSlots[Index];
			List.MarkItemDirty(Entry);
			bChanged = true;
		}
	}
	for (const FGrimInventoryEntry& Entry : List.Entries)
	{
		Slots[Entry.SlotIndex] = Entry.Item;
	}
	if (bChanged)
	{
		// 서버(리슨 호스트 화면)는 복제 콜백이 없으니 직접 알린다.
		OnInventoryChanged.Broadcast(this);
		GetOwner()->ForceNetUpdate();
	}
}

void UGrimInventoryComponent::RebuildSlotsFromList()
{
	int32 Count = 0;
	for (const FGrimInventoryEntry& Entry : List.Entries)
	{
		Count = FMath::Max(Count, Entry.SlotIndex + 1);
	}
	Slots.Init(FGrimItemInstance(), Count);
	for (const FGrimInventoryEntry& Entry : List.Entries)
	{
		if (Slots.IsValidIndex(Entry.SlotIndex))
		{
			Slots[Entry.SlotIndex] = Entry.Item;
		}
	}
}

void UGrimInventoryComponent::HandleReplicatedChange()
{
	RebuildSlotsFromList();
	OnInventoryChanged.Broadcast(this);
}

// ───── 서버: 넣기 · 빼기 · 옮기기 ─────

int32 UGrimInventoryComponent::AddItem(const FGrimItemInstance& Item, bool bNotify)
{
	if (!HasAuthority() || Item.IsEmpty())
	{
		return 0;
	}
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	const FGrimItemDefinition* Def = Database.Find(Item.ItemId);
	if (!Def)
	{
		UE_LOG(LogGrimItems, Warning, TEXT("[Server] 가방 %s: 모르는 아이템 %s"), *GetNameSafe(GetOwner()), *Item.ItemId.ToString());
		return 0;
	}
	EnsureEntries();
	TArray<FGrimItemInstance> NewSlots = Slots;
	const int32 Added = FGrimInventoryModel::Add(NewSlots, Database, Item);
	if (Added > 0)
	{
		CommitSlots(NewSlots);
		UE_LOG(LogGrimItems, Display, TEXT("[Server] 가방 %s: + %s × %d (지금 %d)"), *GetNameSafe(GetOwner()), *Item.ItemId.ToString(), Added, CountOf(Item.ItemId));
	}
	if (bNotify)
	{
		if (Added > 0)
		{
			// 퀘스트: Item.Obtained.<ID> 와 묶음마다 Item.Obtained.<묶음>(튜토리얼 Item.Obtained.Meat).
			NotifyQuest(GrimItemEvents::Obtained(Item.ItemId), Added);
			for (const FName& Group : Def->Groups)
			{
				NotifyQuest(GrimItemEvents::Obtained(Group), Added);
			}
			SendNotice(EGrimItemNotice::Obtained, Item.ItemId, Added);
		}
		if (Added < Item.Count)
		{
			SendNotice(EGrimItemNotice::BagFull, Item.ItemId, Item.Count - Added);
		}
	}
	return Added;
}

int32 UGrimInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (!HasAuthority() || Count <= 0)
	{
		return 0;
	}
	EnsureEntries();
	TArray<FGrimItemInstance> NewSlots = Slots;
	const int32 Removed = FGrimInventoryModel::Remove(NewSlots, ItemId, Count);
	if (Removed > 0)
	{
		CommitSlots(NewSlots);
	}
	return Removed;
}

FGrimItemInstance UGrimInventoryComponent::RemoveFromSlot(int32 SlotIndex, int32 Count)
{
	if (!HasAuthority())
	{
		return FGrimItemInstance();
	}
	EnsureEntries();
	TArray<FGrimItemInstance> NewSlots = Slots;
	const FGrimItemInstance Taken = FGrimInventoryModel::RemoveAt(NewSlots, SlotIndex, Count);
	if (!Taken.IsEmpty())
	{
		CommitSlots(NewSlots);
	}
	return Taken;
}

bool UGrimInventoryComponent::MoveItem(int32 From, int32 To)
{
	if (!HasAuthority())
	{
		return false;
	}
	EnsureEntries();
	TArray<FGrimItemInstance> NewSlots = Slots;
	if (!FGrimInventoryModel::Move(NewSlots, UGrimItemSettings::GetDatabase(), From, To))
	{
		return false;
	}
	CommitSlots(NewSlots);
	return true;
}

// ───── 서버: 쓰기 ─────

EGrimItemUseResult UGrimInventoryComponent::UseItem(int32 SlotIndex)
{
	if (!HasAuthority() || !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return EGrimItemUseResult::Invalid;
	}
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(Slots[SlotIndex].ItemId);
	if (!Def)
	{
		return EGrimItemUseResult::Invalid;
	}
	switch (Def->Row.Kind)
	{
	case EGrimItemKind::Food: return EatFood(SlotIndex, *Def);
	case EGrimItemKind::Skillbook: return ReadSkillbook(SlotIndex, *Def);
	default:
		SendNotice(EGrimItemNotice::CannotUse, Def->Id, 1);
		return EGrimItemUseResult::CannotUse;
	}
}

EGrimItemUseResult UGrimInventoryComponent::EatFood(int32 SlotIndex, const FGrimItemDefinition& Def)
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return EGrimItemUseResult::Invalid;
	}
	const float HungerBefore = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute());
	UGrimItemStatics::ApplyFoodEffects(ASC, Def);

	// 날것이면 배탈 확률(저항 반영) — 생존 쪽 ApplyDisease(이미 걸렸거나 슈퍼치트면 그대로).
	bool bSick = false;
	const float BaseChance = FGrimFoodModel::GetBaseDiseaseChance(Def, GetDefault<UGrimSurvivalSettings>()->Disease.RawFoodStomachChance);
	if (BaseChance > 0.f)
	{
		const float Chance = FGrimSurvivalModel::ComputeDiseaseChance(BaseChance, ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetDiseaseResistAttribute()));
		if (FMath::FRand() < Chance)
		{
			if (UGrimSurvivalComponent* Survival = PlayerState->GetSurvival())
			{
				Survival->ApplyDisease(EGrimDisease::Stomach);
				bSick = true;
			}
		}
	}

	RemoveFromSlot(SlotIndex, 1);
	UE_LOG(LogGrimItems, Display, TEXT("[Server] %s 이(가) %s 을(를) 먹었다: 배고픔 %.1f → %.1f%s"), *GetNameSafe(GetOwner()), *Def.Id.ToString(),
		HungerBefore, ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute()), bSick ? TEXT(" · 배탈") : TEXT(""));
	NotifyQuest(GrimItemEvents::FoodEaten);
	SendNotice(bSick ? EGrimItemNotice::GotSick : EGrimItemNotice::Eaten, Def.Id, 1);
	return EGrimItemUseResult::Eaten;
}

EGrimItemUseResult UGrimInventoryComponent::ReadSkillbook(int32 SlotIndex, const FGrimItemDefinition& Def)
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	UGrimSpellDefinition* Spell = UGrimQuestSettings::LoadSpell(Def.Row.TeachesSpell);
	if (!Spellbook || !Spell)
	{
		UE_LOG(LogGrimItems, Warning, TEXT("[Server] 스킬북 %s: 마법 %s 정의가 없다"), *Def.Id.ToString(), *Def.Row.TeachesSpell.ToString());
		SendNotice(EGrimItemNotice::CannotUse, Def.Id, 1);
		return EGrimItemUseResult::Invalid;
	}
	const EGrimLearnResult Result = Spellbook->UseSkillbook(Spell);
	if (Result == EGrimLearnResult::Learned || Result == EGrimLearnResult::ProficiencyGained)
	{
		RemoveFromSlot(SlotIndex, 1);
		SendNotice(EGrimItemNotice::Learned, Def.Id, 1);
		return Result == EGrimLearnResult::Learned ? EGrimItemUseResult::Learned : EGrimItemUseResult::ProficiencyGained;
	}
	SendNotice(EGrimItemNotice::CannotUse, Def.Id, 1);
	return EGrimItemUseResult::CannotUse;
}

// ───── 서버: 버리기 · 줍기 · 냄비 ─────

AGrimItemPickup* UGrimInventoryComponent::DropItem(int32 SlotIndex, int32 Count)
{
	APawn* Pawn = GetPawn();
	if (!HasAuthority() || !Pawn)
	{
		return nullptr;
	}
	const FGrimItemInstance Taken = RemoveFromSlot(SlotIndex, Count);
	if (Taken.IsEmpty())
	{
		return nullptr;
	}
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	const FRotator Facing(0.f, Pawn->GetActorRotation().Yaw, 0.f);
	const FVector Location = Pawn->GetActorLocation() + Facing.Vector() * Settings->DropDistance;
	AGrimItemPickup* Pickup = UGrimItemStatics::SpawnPickup(this, Taken.ItemId, Taken.Count, Location);
	if (!Pickup)
	{
		AddItem(Taken, false); // 못 놓았으면 돌려놓는다
		return nullptr;
	}
	Pickup->SetItem(Taken); // 내구도 · 인챈트(M4)까지
	Pickup->BlockAutoPickupFor(Pawn, Settings->DropAutoPickupDelay);
	SendNotice(EGrimItemNotice::Dropped, Taken.ItemId, Taken.Count);
	return Pickup;
}

int32 UGrimInventoryComponent::PickUp(AGrimItemPickup* Pickup, bool bCheckReach)
{
	if (!HasAuthority() || !IsValid(Pickup) || Pickup->IsActorBeingDestroyed() || Pickup->GetItem().IsEmpty())
	{
		return 0;
	}
	if (bCheckReach && !Pickup->IsWithinReach(GetPawn(), UGrimItemSettings::Get()->ServerReachSlack))
	{
		UE_LOG(LogGrimItems, Warning, TEXT("[Server] %s: 픽업 %s 이(가) 너무 멀다"), *GetNameSafe(GetOwner()), *Pickup->GetName());
		return 0;
	}
	const FGrimItemInstance Item = Pickup->GetItem();
	const int32 Added = AddItem(Item, true);
	if (Added > 0)
	{
		FGrimItemInstance Remaining = Item;
		Remaining.Count -= Added;
		Pickup->SetItem(Remaining); // 다 주웠으면 사라진다
	}
	return Added;
}

bool UGrimInventoryComponent::InteractWithPot(AGrimCookingPot* Pot, bool bCheckReach)
{
	if (!HasAuthority() || !IsValid(Pot))
	{
		return false;
	}
	if (bCheckReach && !Pot->IsWithinReach(GetPawn(), UGrimItemSettings::Get()->ServerReachSlack))
	{
		return false;
	}
	if (const int32 Added = Pot->AddIngredientsFrom(this); Added > 0)
	{
		NotifyQuest(GrimItemEvents::CookingIngredientsAdded);
		SendNotice(EGrimItemNotice::IngredientsAdded, NAME_None, Added);
		return true;
	}
	if (const int32 Taken = Pot->TakeBackInto(this); Taken > 0)
	{
		SendNotice(EGrimItemNotice::TookBack, NAME_None, Taken);
		return true;
	}
	SendNotice(EGrimItemNotice::NothingToAdd, NAME_None, 0);
	return false;
}

// ───── 알림 · 퀘스트 ─────

void UGrimInventoryComponent::SendNotice(EGrimItemNotice Notice, FName ItemId, int32 Count)
{
	if (!HasAuthority())
	{
		return;
	}
	if (GetLocalOwningController())
	{
		OnNotice.Broadcast(this, Notice, ItemId, Count); // 리슨 호스트 자신
	}
	else
	{
		Client_Notice(Notice, ItemId, Count);
	}
}

void UGrimInventoryComponent::Client_Notice_Implementation(EGrimItemNotice Notice, FName ItemId, int32 Count)
{
	OnNotice.Broadcast(this, Notice, ItemId, Count);
}

void UGrimInventoryComponent::NotifyQuest(FName EventId, int32 Amount) const
{
	if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
	{
		UGrimQuestComponent::NotifyQuestEvent(Owner, EventId, Amount);
	}
}

// ───── RPC ─────

void UGrimInventoryComponent::Server_MoveItem_Implementation(int32 From, int32 To)
{
	MoveItem(From, To);
}

void UGrimInventoryComponent::Server_UseItem_Implementation(int32 SlotIndex)
{
	UseItem(SlotIndex);
}

void UGrimInventoryComponent::Server_NotifyBagOpened_Implementation()
{
	NotifyQuest(TEXT("UI.Inventory.Opened"));
}

void UGrimInventoryComponent::Server_DropItem_Implementation(int32 SlotIndex, int32 Count)
{
	DropItem(SlotIndex, Count);
}

void UGrimInventoryComponent::Server_PickUp_Implementation(AGrimItemPickup* Pickup)
{
	PickUp(Pickup, true);
}

void UGrimInventoryComponent::Server_InteractPot_Implementation(AGrimCookingPot* Pot)
{
	InteractWithPot(Pot, true);
}

void UGrimInventoryComponent::ServerDebugCommand_Implementation(const FString& Command, const FString& Arg, int32 A, int32 B)
{
#if !UE_BUILD_SHIPPING
	APawn* Pawn = GetPawn();
	UWorld* World = GetWorld();
	const FName ItemId(*Arg);
	const FRotator Facing(0.f, Pawn ? Pawn->GetActorRotation().Yaw : 0.f, 0.f);
	const FVector Ahead = Pawn ? Pawn->GetActorLocation() + Facing.Vector() * 150.f : FVector::ZeroVector;

	if (Command == TEXT("Give"))
	{
		AddItem(FGrimItemInstance(ItemId, FMath::Max(1, A)));
	}
	else if (Command == TEXT("Spawn"))
	{
		UGrimItemStatics::SpawnPickup(this, ItemId, FMath::Max(1, A), Ahead);
	}
	else if (Command == TEXT("PickUp"))
	{
		if (AGrimItemPickup* Pickup = AGrimItemPickup::FindNearestReachable(Pawn))
		{
			PickUp(Pickup, true);
		}
		else
		{
			UE_LOG(LogGrimItems, Display, TEXT("[Server] %s: 주울 것이 곁에 없다"), *GetNameSafe(GetOwner()));
		}
	}
	else if (Command == TEXT("Use"))
	{
		UseItem(A);
	}
	else if (Command == TEXT("Eat"))
	{
		const int32 SlotIndex = Slots.IndexOfByPredicate([ItemId](const FGrimItemInstance& Slot) { return !Slot.IsEmpty() && Slot.ItemId == ItemId; });
		if (SlotIndex != INDEX_NONE)
		{
			UseItem(SlotIndex);
		}
	}
	else if (Command == TEXT("Drop"))
	{
		DropItem(A, FMath::Max(1, B));
	}
	else if (Command == TEXT("DropId"))
	{
		const int32 SlotIndex = Slots.IndexOfByPredicate([ItemId](const FGrimItemInstance& Slot) { return !Slot.IsEmpty() && Slot.ItemId == ItemId; });
		if (SlotIndex != INDEX_NONE)
		{
			DropItem(SlotIndex, FMath::Max(1, A));
		}
	}
	else if (Command == TEXT("Move"))
	{
		MoveItem(A, B);
	}
	else if (Command == TEXT("Fire"))
	{
		// 불 마법을 직접 쏜 셈 — 가장 가까운 픽업(6m 안)에 불 자극
		AGrimItemPickup* Best = nullptr;
		float BestDistSq = FMath::Square(600.f);
		for (TActorIterator<AGrimItemPickup> It(World); It && Pawn; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = GrimTags::Element_Fire;
		Stimulus.Intensity = A > 0 ? static_cast<float>(A) : 1.f;
		Stimulus.Source = EGrimStimulusSource::Spell;
		if (Best)
		{
			UGrimElementalStatics::ApplyStimulusToActor(Best, Stimulus, Pawn);
		}
	}
	else if (Command == TEXT("SpawnFire") && Pawn)
	{
		// 불붙은 모닥불(폰 앞 2m 바닥)
		const FVector Ground = UGrimItemStatics::FindDropLocation(World, Pawn->GetActorLocation() + Facing.Vector() * 200.f, 0.f, Pawn);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AGrimCampfire* Campfire = World->SpawnActor<AGrimCampfire>(AGrimCampfire::StaticClass(), Ground, FRotator::ZeroRotator, Params))
		{
			FGrimElementalStimulus Stimulus;
			Stimulus.Element = GrimTags::Element_Fire;
			Stimulus.Intensity = 1.f;
			Stimulus.Source = EGrimStimulusSource::Spell;
			UGrimElementalStatics::ApplyStimulusToActor(Campfire, Stimulus, Pawn);
			UE_LOG(LogGrimItems, Display, TEXT("[Server] 모닥불 %s at %s Burning=%.1f"), *Campfire->GetName(), *Ground.ToCompactString(),
				UGrimElementalStatics::GetStateIntensity(Campfire, GrimTags::State_Burning));
		}
	}
	else if (Command == TEXT("SpawnPot") && Pawn)
	{
		// 가장 가까운 모닥불 위(없으면 폰 앞 바닥)
		FVector Location = UGrimItemStatics::FindDropLocation(World, Ahead, 0.f, Pawn);
		float BestDistSq = FMath::Square(800.f);
		for (TActorIterator<AGrimCampfire> It(World); It; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Location = It->GetActorLocation() + FVector(0.f, 0.f, 55.f);
			}
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FRotator PotFacing(0.f, (Pawn->GetActorLocation() - Location).Rotation().Yaw, 0.f);
		if (AGrimCookingPot* Pot = World->SpawnActor<AGrimCookingPot>(AGrimCookingPot::StaticClass(), Location, PotFacing, Params))
		{
			UE_LOG(LogGrimItems, Display, TEXT("[Server] 냄비 %s at %s"), *Pot->GetName(), *Location.ToCompactString());
		}
	}
	else if (Command == TEXT("Pot"))
	{
		if (AGrimCookingPot* Pot = AGrimCookingPot::FindNearestReachable(Pawn))
		{
			InteractWithPot(Pot, true);
		}
		else
		{
			UE_LOG(LogGrimItems, Display, TEXT("[Server] %s: 곁에 냄비가 없다"), *GetNameSafe(GetOwner()));
		}
	}
	else if (Command == TEXT("Hunger"))
	{
		const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
		if (UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr)
		{
			ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), static_cast<float>(A));
		}
	}
	else if (Command == TEXT("Clear"))
	{
		TArray<FGrimItemInstance> Empty;
		Empty.SetNum(Slots.Num());
		CommitSlots(Empty);
	}
	else
	{
		UE_LOG(LogGrimItems, Warning, TEXT("[Server] 모르는 아이템 콘솔 명령 %s"), *Command);
	}
#endif
}

// ───── 세이브 (Items.Inventory v1) ─────

void UGrimInventoryComponent::WriteSaveData(FArchive& Ar)
{
	EnsureEntries();
	int32 Num = Slots.Num();
	Ar << Num;
	for (FGrimItemInstance Slot : Slots)
	{
		Slot.Serialize(Ar);
	}
}

void UGrimInventoryComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	int32 Num = 0;
	Ar << Num;
	TArray<FGrimItemInstance> Loaded;
	for (int32 Index = 0; Index < FMath::Clamp(Num, 0, 1000) && !Ar.IsError(); ++Index)
	{
		FGrimItemInstance Slot;
		Slot.Serialize(Ar);
		Loaded.Add(Slot);
	}
	if (Ar.IsError() || !HasAuthority())
	{
		return;
	}

	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	EnsureEntries();
	TArray<FGrimItemInstance> NewSlots;
	NewSlots.SetNum(Slots.Num());
	TArray<FGrimItemInstance> Overflow;
	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		const FGrimItemInstance& Slot = Loaded[Index];
		if (Slot.IsEmpty())
		{
			continue;
		}
		if (!Database.Find(Slot.ItemId))
		{
			UE_LOG(LogGrimItems, Warning, TEXT("[Server] 가방 세이브: 표에 없는 아이템 %s × %d 를 버린다"), *Slot.ItemId.ToString(), Slot.Count);
			continue;
		}
		if (NewSlots.IsValidIndex(Index))
		{
			NewSlots[Index] = Slot;
		}
		else
		{
			Overflow.Add(Slot);
		}
	}
	for (const FGrimItemInstance& Slot : Overflow)
	{
		const int32 Added = FGrimInventoryModel::Add(NewSlots, Database, Slot);
		if (Added < Slot.Count)
		{
			UE_LOG(LogGrimItems, Warning, TEXT("[Server] 가방 세이브: 칸이 모자라 %s × %d 를 잃었다"), *Slot.ItemId.ToString(), Slot.Count - Added);
		}
	}
	CommitSlots(NewSlots);
}
