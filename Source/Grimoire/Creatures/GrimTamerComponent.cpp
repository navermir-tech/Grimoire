#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureMenuWidget.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Items/GrimCookingPot.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

const FName UGrimTamerComponent::ComponentName(TEXT("GrimTamer"));

UGrimTamerComponent::UGrimTamerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

UGrimTamerComponent* UGrimTamerComponent::FindForActor(const AActor* Actor)
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
	return PlayerState ? PlayerState->FindComponentByClass<UGrimTamerComponent>() : nullptr;
}

APlayerState* UGrimTamerComponent::GetPlayerState() const
{
	return Cast<APlayerState>(GetOwner());
}

APawn* UGrimTamerComponent::GetPawn() const
{
	const APlayerState* PlayerState = GetPlayerState();
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

APlayerController* UGrimTamerComponent::GetLocalOwningController() const
{
	const APlayerState* PlayerState = GetPlayerState();
	APlayerController* PC = PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
	return PC && PC->IsLocalController() ? PC : nullptr;
}

bool UGrimTamerComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

FString UGrimTamerComponent::GetPlayerId() const
{
	if (CachedPlayerId.IsEmpty() && HasAuthority())
	{
		CachedPlayerId = UGrimTamingSubsystem::ResolvePlayerId(GetPlayerState());
	}
	return CachedPlayerId;
}

bool UGrimTamerComponent::OwnsCreature(const AGrimCreature* Creature) const
{
	const UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	return Taming && Taming->IsTamed() && Taming->GetOwnerPlayerState() == GetPlayerState();
}

void UGrimTamerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGrimTamerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	// 퇴장(PlayerState 가 지워짐): 세이브는 퇴장 때 먼저 쓰였다 — 세계에 남은 내 생물을 거둔다.
	if (HasAuthority() && EndPlayReason == EEndPlayReason::Destroyed)
	{
		DespawnOwned();
	}
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

// ───────────────────────── 서버: 기록 ─────────────────────────

void UGrimTamerComponent::AddOrUpdateRecord(const FGrimOwnedCreatureRecord& Record)
{
	if (FGrimOwnedCreatureRecord* Existing = Records.FindByPredicate([&Record](const FGrimOwnedCreatureRecord& R) { return R.IsSameCreature(Record.Instance); }))
	{
		*Existing = Record;
	}
	else
	{
		Records.Add(Record);
	}
}

void UGrimTamerComponent::RemoveRecord(const FGrimCreatureInstance& Instance)
{
	Records.RemoveAll([&Instance](const FGrimOwnedCreatureRecord& R) { return R.IsSameCreature(Instance); });
}

void UGrimTamerComponent::RefreshRecordsFromWorld()
{
	const UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}
	for (AGrimCreature* Creature : Subsystem->GetOwnedCreatures(GetPlayerId()))
	{
		if (const UGrimCreatureTamingComponent* Taming = Creature->GetTaming())
		{
			AddOrUpdateRecord(Taming->MakeRecord());
		}
	}
}

int32 UGrimTamerComponent::SummonOwned()
{
	UWorld* World = GetWorld();
	APawn* Pawn = GetPawn();
	UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(this);
	if (!World || !Pawn || !Subsystem || !HasAuthority())
	{
		return 0;
	}
	const FString PlayerId = GetPlayerId();
	const TArray<AGrimCreature*> Alive = Subsystem->GetOwnedCreatures(PlayerId);
	const FGrimTamedBehaviorTuning& Behavior = UGrimTamingSettings::Get()->Behavior;
	const double NowUtc = FDateTime::UtcNow().ToUnixTimestamp();
	int32 Summoned = 0;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		FGrimOwnedCreatureRecord& Record = Records[Index];
		Record.Instance.OwnerId = PlayerId;
		AGrimCreature* const* Found = Alive.FindByPredicate([&Record](const AGrimCreature* C) { return Record.IsSameCreature(C->GetInstance()); });
		if (Found)
		{
			// 이미 세계에 있다(다시 불러오기) — 상태만 맞춘다.
			if (UGrimCreatureTamingComponent* Taming = (*Found)->GetTaming())
			{
				Taming->RestoreOwned(GetPlayerState(), Record, 0.0);
			}
			continue;
		}
		UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Record.Instance.SpeciesId);
		if (!Species)
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("[Server] 가진 개체 %s seed=%d: 종이 없어 부르지 못한다"), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed);
			continue;
		}
		// 주인 둘레에 고르게(뒤쪽 반원).
		const float Angle = 180.f + (Index - (Records.Num() - 1) * 0.5f) * 40.f;
		const FVector Dir = Pawn->GetActorForwardVector().GetSafeNormal2D().RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Point = Pawn->GetActorLocation() + Dir * Behavior.SummonDistance;
		FVector Ground = Point - FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight());
		UGrimEcosystemSubsystem::FindGround(World, Point, Ground);
		AGrimCreature* Creature = AGrimCreature::SpawnCreature(World, Species, Record.Instance, Ground, Pawn->GetActorRotation().Yaw, Record.HealthRatio);
		if (!Creature)
		{
			continue;
		}
		const double Offline = Record.SavedUtcSeconds > 0.0 ? FMath::Max(0.0, NowUtc - Record.SavedUtcSeconds) : 0.0;
		if (UGrimCreatureTamingComponent* Taming = Creature->GetTaming())
		{
			Taming->RestoreOwned(GetPlayerState(), Record, Offline);
		}
		++Summoned;
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 가진 개체 부름 %s seed=%d 주인=%s 충성도=%.0f"), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed, *PlayerId, Record.Instance.Loyalty);
	}
	return Summoned;
}

void UGrimTamerComponent::DespawnOwned()
{
	UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(this);
	if (!Subsystem)
	{
		return;
	}
	for (AGrimCreature* Creature : Subsystem->GetOwnedCreatures(GetPlayerId()))
	{
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 주인 퇴장 — 거둠 %s seed=%d"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed);
		Creature->Destroy();
	}
}

void UGrimTamerComponent::TrySummonLater()
{
	if (!bSummonPending)
	{
		return;
	}
	if (GetPawn() || ++SummonAttempts > 40)
	{
		bSummonPending = false;
		GetWorld()->GetTimerManager().ClearTimer(SummonTimer);
		SummonOwned();
	}
}

// ───────────────────────── 세이브 ─────────────────────────

void UGrimTamerComponent::WriteSaveData(FArchive& Ar)
{
	RefreshRecordsFromWorld();
	GrimTamingMath::SerializeOwnedRecords(Ar, Records);
}

void UGrimTamerComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FGrimOwnedCreatureRecord> Loaded;
	if (!GrimTamingMath::SerializeOwnedRecords(Ar, Loaded))
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] Creatures.Owned 를 읽지 못했다(v%d)"), Version);
		return;
	}
	Records = MoveTemp(Loaded);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 가진 개체 %d 를 읽었다 — 곁에 부른다"), Records.Num());
	if (GetPawn())
	{
		SummonOwned();
	}
	else if (UWorld* World = GetWorld())
	{
		bSummonPending = true;
		SummonAttempts = 0;
		World->GetTimerManager().SetTimer(SummonTimer, this, &UGrimTamerComponent::TrySummonLater, 0.5f, true);
	}
}

// ───────────────────────── 서버: 행동 ─────────────────────────

bool UGrimTamerComponent::IsWithinReach(const AGrimCreature* Creature) const
{
	const APawn* Pawn = GetPawn();
	const FGrimTamedBehaviorTuning& Behavior = UGrimTamingSettings::Get()->Behavior;
	return Pawn && Creature && FVector::Dist(Pawn->GetActorLocation(), Creature->GetActorLocation()) <= Behavior.InteractRadius + Behavior.ServerReachSlack;
}

FName UGrimTamerComponent::FindFoodFor(const AGrimCreature* Creature, bool& bOutFavorite) const
{
	bOutFavorite = false;
	const UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(GetOwner());
	const UGrimCreatureSpecies* Species = Creature ? Creature->GetSpecies() : nullptr;
	if (!Inventory || !Species)
	{
		return NAME_None;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	if (const FGrimCreatureFoodRule* Rule = Settings->FindFoodRule(Species->SpeciesId))
	{
		for (const FName& Favorite : Rule->FavoriteItems)
		{
			if (Inventory->CountOf(Favorite) > 0)
			{
				bOutFavorite = true;
				return Favorite;
			}
		}
	}
	const TArray<FName> Groups = Settings->GetAcceptedGroups(Species->SpeciesId, Species->Diet);
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	for (const FGrimItemInstance& Slot : Inventory->GetSlots())
	{
		const FGrimItemDefinition* Def = Slot.IsEmpty() ? nullptr : Database.Find(Slot.ItemId);
		if (Def && Groups.ContainsByPredicate([Def](const FName& Group) { return Def->IsInGroup(Group); }))
		{
			return Slot.ItemId;
		}
	}
	return NAME_None;
}

bool UGrimTamerComponent::FeedCreature(AGrimCreature* Creature)
{
	UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(GetOwner());
	UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	if (!Inventory || !Taming)
	{
		return false;
	}
	bool bFavorite = false;
	const FName Food = FindFoodFor(Creature, bFavorite);
	if (Food.IsNone() || Inventory->RemoveItem(Food, 1) <= 0)
	{
		SendNotice(FString::Printf(TEXT("%s 이(가) 먹을 것이 가방에 없다"), *Taming->GetDisplayName()));
		return false;
	}
	Taming->RecordFed(GetPlayerState(), bFavorite);
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(Food);
	const FString FoodName = Def ? Def->GetDisplayName().ToString() : Food.ToString();
	SendNotice(FString::Printf(TEXT("%s 에게 %s 을(를) 줬다%s"), *Taming->GetDisplayName(), *FoodName, bFavorite ? TEXT(" — 아주 좋아한다!") : TEXT("")));
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 먹이 %s → %s seed=%d%s"), *Food.ToString(), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, bFavorite ? TEXT(" (좋아함)") : TEXT(""));
	return true;
}

bool UGrimTamerComponent::DoAction(AGrimCreature* Creature, EGrimTamerAction Action, bool bCheckReach)
{
	if (!HasAuthority() || !Creature || Creature->IsDead())
	{
		return false;
	}
	if (bCheckReach && !IsWithinReach(Creature))
	{
		return false;
	}
	UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	if (!Taming)
	{
		return false;
	}
	// 먹이는 야생에게도(포획 확률) — 그 밖은 내 생물에게만.
	if (Action == EGrimTamerAction::Feed)
	{
		return FeedCreature(Creature);
	}
	if (!OwnsCreature(Creature))
	{
		SendNotice(TEXT("내 생물이 아니다"));
		return false;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	const FString Name = Taming->GetDisplayName();
	switch (Action)
	{
	case EGrimTamerAction::Follow:
	case EGrimTamerAction::Wait:
	{
		if (!GrimTamingMath::RollObedience(Taming->GetStage(), Settings->Loyalty, FMath::FRand()))
		{
			SendNotice(FString::Printf(TEXT("%s 이(가) 명령을 무시했다"), *Name));
			UE_LOG(LogGrimTaming, Display, TEXT("[Server] 명령 무시 %s seed=%d (충성도 %.0f)"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, Taming->GetLoyalty());
			return false;
		}
		const EGrimTamedCommand Command = Action == EGrimTamerAction::Follow ? EGrimTamedCommand::Follow : EGrimTamedCommand::Wait;
		Taming->SetCommand(Command);
		SendNotice(FString::Printf(TEXT("%s: %s"), *Name, Command == EGrimTamedCommand::Follow ? TEXT("따라온다") : TEXT("여기서 기다린다")));
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 명령 %s → %s seed=%d"), Command == EGrimTamedCommand::Follow ? TEXT("Follow") : TEXT("Wait"),
			*Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed);
		return true;
	}
	case EGrimTamerAction::Mount:
	{
		UGrimMountComponent* Mount = Creature->GetMount();
		ACharacter* Rider = Cast<ACharacter>(GetPawn());
		FString Reason;
		if (!Mount || !Mount->TryMount(Rider, Reason))
		{
			SendNotice(Reason.IsEmpty() ? FString(TEXT("탈 수 없다")) : Reason);
			return false;
		}
		SendNotice(FString::Printf(TEXT("%s 에 올라탔다 — [F] 내리기"), *Name));
		return true;
	}
	case EGrimTamerAction::Release:
		Taming->ReleaseToWild(false);
		return true;
	default:
		return false;
	}
}

bool UGrimTamerComponent::DoRename(AGrimCreature* Creature, const FString& NewName, bool bCheckReach)
{
	if (!HasAuthority() || !OwnsCreature(Creature) || (bCheckReach && !IsWithinReach(Creature)))
	{
		return false;
	}
	const FString Clean = GrimTamingMath::SanitizeNickname(NewName, UGrimTamingSettings::Get()->Behavior.NicknameMaxLength);
	Creature->GetTaming()->SetNickname(Clean);
	AddOrUpdateRecord(Creature->GetTaming()->MakeRecord());
	SendNotice(Clean.IsEmpty() ? FString(TEXT("이름을 지웠다")) : FString::Printf(TEXT("이름: %s"), *Clean));
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 이름 %s seed=%d → \"%s\""), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, *Clean);
	return true;
}

bool UGrimTamerComponent::DoDismount()
{
	UGrimMountComponent* Mount = UGrimMountComponent::FindMountOf(GetPawn());
	if (!HasAuthority() || !Mount)
	{
		return false;
	}
	Mount->Dismount(false);
	return true;
}

void UGrimTamerComponent::SendNotice(const FString& Text)
{
	if (!HasAuthority() || Text.IsEmpty())
	{
		return;
	}
	if (GetLocalOwningController())
	{
		Client_Notice_Implementation(Text);
	}
	else
	{
		Client_Notice(Text);
	}
}

void UGrimTamerComponent::Client_Notice_Implementation(const FString& Text)
{
	UE_LOG(LogGrimTaming, Display, TEXT("[%s] 알림: %s"), GetWorld() && GetWorld()->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"), *Text);
	OnNotice.Broadcast(this, Text);
	if (Widget)
	{
		Widget->ShowNotice(Text);
	}
}

// ───────────────────────── RPC ─────────────────────────

void UGrimTamerComponent::Server_Action_Implementation(AGrimCreature* Creature, EGrimTamerAction Action)
{
	DoAction(Creature, Action, true);
}

void UGrimTamerComponent::Server_Rename_Implementation(AGrimCreature* Creature, const FString& NewName)
{
	DoRename(Creature, NewName, true);
}

void UGrimTamerComponent::Server_Dismount_Implementation()
{
	DoDismount();
}

void UGrimTamerComponent::Server_MountInput_Implementation(FVector_NetQuantize100 Input, FRotator ControlRotation)
{
	if (UGrimMountComponent* Mount = UGrimMountComponent::FindMountOf(GetPawn()))
	{
		Mount->SetRiderInput(Input);
		if (APlayerController* PC = GetPlayerState() ? Cast<APlayerController>(GetPlayerState()->GetOwner()) : nullptr)
		{
			PC->SetControlRotation(ControlRotation);
		}
	}
}

void UGrimTamerComponent::RequestAction(AGrimCreature* Creature, EGrimTamerAction Action)
{
	Server_Action(Creature, Action);
}

void UGrimTamerComponent::RequestRename(AGrimCreature* Creature, const FString& NewName)
{
	Server_Rename(Creature, NewName);
}

void UGrimTamerComponent::RequestDismount()
{
	Server_Dismount();
}

// ───────────────────────── 로컬: 안내 · 메뉴 · 탑승 입력 ─────────────────────────

void UGrimTamerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (APlayerController* PC = GetLocalOwningController())
	{
		TickLocal(PC);
	}
}

bool UGrimTamerComponent::EnsureWidget(APlayerController* PC)
{
	if (Widget)
	{
		return true;
	}
	if (!PC || !PC->GetLocalPlayer())
	{
		return false;
	}
	Widget = CreateWidget<UGrimCreatureMenuWidget>(PC, UGrimCreatureMenuWidget::StaticClass());
	if (Widget)
	{
		Widget->SetTamer(this);
		Widget->AddToViewport(18);
	}
	return Widget != nullptr;
}

AGrimCreature* UGrimTamerComponent::FindNearestCreature(const APawn* Pawn, bool bOwned) const
{
	if (!Pawn || !GetWorld())
	{
		return nullptr;
	}
	const float Radius = UGrimTamingSettings::Get()->Behavior.InteractRadius;
	AGrimCreature* Best = nullptr;
	float BestDistance = Radius;
	for (TActorIterator<AGrimCreature> It(GetWorld()); It; ++It)
	{
		AGrimCreature* Creature = *It;
		if (Creature->IsDead() || !Creature->GetSpecies())
		{
			continue;
		}
		const bool bMine = OwnsCreature(Creature);
		if (bOwned ? !bMine : (Creature->GetInstance().IsTamed() || !Creature->GetSpecies()->bTameable))
		{
			continue;
		}
		const float Distance = FVector::Dist(Pawn->GetActorLocation(), Creature->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Creature;
		}
	}
	return Best;
}

void UGrimTamerComponent::TickLocal(APlayerController* PC)
{
	ACharacter* Me = Cast<ACharacter>(PC->GetPawn());
	if (!Me)
	{
		return;
	}
	const FKey& Key = UGrimTamingSettings::Get()->Behavior.MenuKey;
	const bool bKeyPressed = PC->WasInputKeyJustPressed(Key);

	// 탄 동안: 캐릭터 이동이 꺼져 있다 — 쌓인 이동 입력을 탈것으로.
	if (UGrimMountComponent* Mount = UGrimMountComponent::FindMountOf(Me))
	{
		const FVector Input = Me->ConsumeMovementInputVector().GetClampedToMaxSize(1.f);
		if (HasAuthority())
		{
			Mount->SetRiderInput(Input);
		}
		else
		{
			const double Now = GetWorld()->GetTimeSeconds();
			// 캐릭터 이동이 꺼져 카메라 갱신(ServerUpdateCamera)도 멈춘다 — 서버의 시선(조준 검증 · 서버 조준)이 낡지 않게 계속 보낸다.
			if (PC->PlayerCameraManager && PC->PlayerCameraManager->bUseClientSideCameraUpdates)
			{
				PC->PlayerCameraManager->bShouldSendClientSideCameraUpdate = true;
			}
			const FRotator Rotation = PC->GetControlRotation();
			if (Now >= NextInputSendTime || !Input.Equals(LastSentInput, 0.05f) || !Rotation.Equals(LastSentRotation, 1.f))
			{
				NextInputSendTime = Now + UGrimTamingSettings::Get()->Mount.InputSendInterval;
				LastSentInput = Input;
				LastSentRotation = Rotation;
				Server_MountInput(Input, Rotation);
			}
		}
		if (EnsureWidget(PC))
		{
			Widget->ShowPrompt(FString::Printf(TEXT("[%s] 내리기"), *Key.GetDisplayName().ToString()));
		}
		if (bKeyPressed && (!Widget || !Widget->IsMenuOpen()))
		{
			RequestDismount();
		}
		return;
	}

	if (Widget && Widget->IsMenuOpen())
	{
		return;
	}

	PromptAccumulator += GetWorld()->GetDeltaSeconds();
	if (PromptAccumulator >= 0.1f || bKeyPressed)
	{
		PromptAccumulator = 0.f;
		PromptCreature = nullptr;
		bPromptIsFeed = false;
		// 줍기 · 냄비가 먼저(같은 키) — 그 앞에서는 생물 안내를 내지 않는다.
		const bool bItemFirst = AGrimItemPickup::FindNearestReachable(Me) || AGrimCookingPot::FindNearestReachable(Me);
		if (!bItemFirst)
		{
			if (AGrimCreature* Owned = FindNearestCreature(Me, true))
			{
				PromptCreature = Owned;
			}
			else if (AGrimCreature* Wild = FindNearestCreature(Me, false))
			{
				bool bFavorite = false;
				// 가방은 소유 클라이언트에도 복제된다 — 먹일 것이 있을 때만 안내.
				if (!FindFoodFor(Wild, bFavorite).IsNone())
				{
					PromptCreature = Wild;
					bPromptIsFeed = true;
				}
			}
		}
		if (AGrimCreature* Target = PromptCreature.Get(); Target && EnsureWidget(PC))
		{
			const FString Name = Target->GetTaming() ? Target->GetTaming()->GetDisplayName() : Target->GetInstance().SpeciesId.ToString();
			Widget->ShowPrompt(bPromptIsFeed ? FString::Printf(TEXT("[%s] %s 에게 먹이 주기"), *Key.GetDisplayName().ToString(), *Name)
				: FString::Printf(TEXT("[%s] %s 돌보기"), *Key.GetDisplayName().ToString(), *Name));
		}
		else if (Widget)
		{
			Widget->HidePrompt();
		}
	}

	if (bKeyPressed)
	{
		if (AGrimCreature* Target = PromptCreature.Get())
		{
			if (bPromptIsFeed)
			{
				RequestAction(Target, EGrimTamerAction::Feed);
			}
			else if (EnsureWidget(PC))
			{
				Widget->OpenMenu(Target);
			}
		}
	}
}
