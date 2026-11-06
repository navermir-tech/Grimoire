#include "Base/GrimBuilderComponent.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseInteractable.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseTags.h"
#include "Base/GrimBaseWidget.h"
#include "Base/GrimBuildGhost.h"
#include "Base/GrimBuildPiece.h"
#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimItemContainerComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Core/Cheats/GrimCheatTags.h"
#include "Core/GrimPlayerState.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimInventoryWidget.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellbookComponent.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "Quest/GrimQuestComponent.h"
#include "Save/GrimSaveSubsystem.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GrimBuilder"

const FName UGrimBuilderComponent::ComponentName(TEXT("GrimBuilder"));

namespace GrimBuilderLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FCollisionObjectQueryParams SolidObjects(bool bWithPawns)
	{
		FCollisionObjectQueryParams Params;
		Params.AddObjectTypesToQuery(ECC_WorldStatic);
		Params.AddObjectTypesToQuery(ECC_WorldDynamic);
		if (bWithPawns)
		{
			Params.AddObjectTypesToQuery(ECC_Pawn);
		}
		return Params;
	}

	float DistanceToActor(const AActor* Target, const FVector& From)
	{
		const FBox Box = Target ? Target->GetComponentsBoundingBox(false, false) : FBox(ForceInit);
		if (!Box.IsValid)
		{
			return Target ? FVector::Dist(Target->GetActorLocation(), From) : TNumericLimits<float>::Max();
		}
		return FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(From));
	}

	template <typename T>
	T* FindNearest(UWorld* World, const FVector& From, float MaxDistance)
	{
		T* Best = nullptr;
		float BestDistance = MaxDistance;
		for (TActorIterator<T> It(World); It; ++It)
		{
			const float Distance = FVector::Dist(It->GetActorLocation(), From);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}
}

UGrimBuilderComponent::UGrimBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

UGrimBuilderComponent* UGrimBuilderComponent::FindForActor(const AActor* Actor)
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
	return PlayerState ? PlayerState->FindComponentByClass<UGrimBuilderComponent>() : nullptr;
}

bool UGrimBuilderComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

APawn* UGrimBuilderComponent::GetPawn() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

UGrimInventoryComponent* UGrimBuilderComponent::GetBag() const
{
	return UGrimInventoryComponent::FindForActor(GetOwner());
}

APlayerController* UGrimBuilderComponent::GetLocalOwningController() const
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

FString UGrimBuilderComponent::GetOwnerKey() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	if (!PlayerState)
	{
		return FString();
	}
	if (const UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(GetWorld()))
	{
		const FString Id = Save->GetPlayerId(PlayerState);
		if (!Id.IsEmpty())
		{
			return Id;
		}
	}
	return PlayerState->GetPlayerName();
}

bool UGrimBuilderComponent::IsWithinReach(const AActor* Target, float Radius) const
{
	const APawn* Pawn = GetPawn();
	if (!Pawn || !Target)
	{
		return false;
	}
	return GrimBuilderLocal::DistanceToActor(Target, Pawn->GetActorLocation()) <= Radius + UGrimBaseSettings::Get()->ServerReachSlack;
}

// ───── 로컬: 준비 · 틱 ─────

void UGrimBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!TryLocalSetup())
	{
		// 클라이언트에서는 컨트롤러의 PlayerState 가 늦게 잇기도 한다(가방과 같은 방식).
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			TryLocalSetup();
		}), 0.5f, true);
	}
}

bool UGrimBuilderComponent::TryLocalSetup()
{
	if (bLocalSetupDone)
	{
		return true;
	}
	APlayerController* PlayerController = GetLocalOwningController();
	if (!PlayerController)
	{
		return false;
	}
	bLocalSetupDone = true;
	Widget = CreateWidget<UGrimBaseWidget>(PlayerController, UGrimBaseWidget::StaticClass());
	if (Widget)
	{
		Widget->SetBuilder(this);
		Widget->AddToViewport(9);
	}
	SetComponentTickEnabled(true);
	// 남은 재시도 타이머는 이 함수가 참을 돌려주면 더 할 일이 없다 — 타이머 모두를 지운다.
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	return true;
}

void UGrimBuilderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (Ghost)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimBuilderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetLocalOwningController())
	{
		return;
	}
	HandleLocalInput();
	if (bBuildMode)
	{
		UpdatePreview();
	}
	InteractAccumulator += DeltaTime;
	if (InteractAccumulator >= 0.1f)
	{
		InteractAccumulator = 0.f;
		UpdateInteractTarget();
		// 연 시설에서 멀어지면 탭을 닫는다.
		if (AActor* Station = OpenStationActor.Get())
		{
			const APawn* Pawn = GetPawn();
			if (!Pawn || GrimBuilderLocal::DistanceToActor(Station, Pawn->GetActorLocation()) > UGrimBaseSettings::Get()->StationOpenRadius)
			{
				CloseStation();
			}
		}
		else if (OpenStationActor.IsStale())
		{
			CloseStation();
		}
	}
}

void UGrimBuilderComponent::HandleLocalInput()
{
	APlayerController* PC = GetLocalOwningController();
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	UGrimInventoryComponent* Bag = GetBag();
	UGrimInventoryWidget* InventoryWidget = Bag ? Bag->GetWidget() : nullptr;
	const bool bBagOpen = InventoryWidget && InventoryWidget->IsBagOpen();

	if (PC->WasInputKeyJustPressed(Settings->GearKey) && InventoryWidget)
	{
		if (bBagOpen && InventoryWidget->GetActiveTab() == UGrimInventoryWidget::EquipmentTabId)
		{
			InventoryWidget->SetBagOpen(false);
		}
		else
		{
			InventoryWidget->OpenTab(UGrimInventoryWidget::EquipmentTabId);
		}
		return;
	}
	if (bBagOpen)
	{
		return; // 화면이 열린 동안에는 클릭 · 키가 화면 몫
	}
	if (PC->WasInputKeyJustPressed(Settings->BuildKey))
	{
		SetBuildMode(!bBuildMode);
		return;
	}
	if (bBuildMode)
	{
		if (PC->WasInputKeyJustPressed(Settings->PrevPieceKey))
		{
			CyclePiece(-1);
		}
		if (PC->WasInputKeyJustPressed(Settings->NextPieceKey))
		{
			CyclePiece(1);
		}
		if (PC->WasInputKeyJustPressed(Settings->RotateKey))
		{
			Rotate();
		}
		if (PC->WasInputKeyJustPressed(Settings->PlaceKey))
		{
			PlaceFromPreview();
		}
		if (PC->WasInputKeyJustPressed(Settings->DemolishKey))
		{
			DemolishAimed();
		}
	}
	if (PC->WasInputKeyJustPressed(Settings->InteractKey))
	{
		if (IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(InteractTarget.Get()))
		{
			Interactable->InteractLocal(this);
		}
	}
}

// ───── 건축 모드 ─────

FName UGrimBuilderComponent::GetSelectedPiece() const
{
	const TArray<FName>& Order = UGrimBaseSettings::GetDatabase().PieceOrder;
	return Order.IsValidIndex(SelectedIndex) ? Order[SelectedIndex] : NAME_None;
}

void UGrimBuilderComponent::CyclePiece(int32 Step)
{
	const int32 Num = UGrimBaseSettings::GetDatabase().PieceOrder.Num();
	if (Num > 0)
	{
		SelectedIndex = ((SelectedIndex + Step) % Num + Num) % Num;
	}
}

void UGrimBuilderComponent::SetBuildMode(bool bOn)
{
	if (bOn == bBuildMode)
	{
		return;
	}
	bBuildMode = bOn;
	Server_SetBuildMode(bOn);
	if (!bOn && Ghost)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
}

void UGrimBuilderComponent::Server_SetBuildMode_Implementation(bool bOn)
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	if (UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr)
	{
		// 복제 루스 태그 — 그리모어를 거두고(Q25) 시전을 막는다(CastBlockingTags).
		ASC->SetLooseGameplayTagCount(GrimMagicFXTags::State_BuildMode, bOn ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	}
}

bool UGrimBuilderComponent::TraceAim(FHitResult& OutHit) const
{
	APlayerController* PC = GetLocalOwningController();
	APawn* Pawn = GetPawn();
	if (!PC || !Pawn)
	{
		return false;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimBuildAim), false, Pawn);
	if (Ghost)
	{
		Params.AddIgnoredActor(Ghost);
	}
	TArray<AActor*> Attached;
	Pawn->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);
	const FVector End = ViewLocation + ViewRotation.Vector() * UGrimBaseSettings::Get()->BuildTraceDistance;
	return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, End, ECC_Visibility, Params);
}

AActor* UGrimBuilderComponent::TraceAimedActor() const
{
	FHitResult Hit;
	return TraceAim(Hit) ? Hit.GetActor() : nullptr;
}

void UGrimBuilderComponent::UpdatePreview()
{
	const FName PieceId = GetSelectedPiece();
	const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
	FHitResult AimHit;
	const bool bHasAim = TraceAim(AimHit);
	const FVector Aim = AimHit.ImpactPoint;

	Preview = FGrimBuildPreview();
	Preview.PieceId = PieceId;
	Preview.bHasAim = bHasAim;
	if (!Def || !bHasAim)
	{
		Preview.Result = bHasAim ? EGrimBuildResult::UnknownPiece : EGrimBuildResult::NoGround;
		if (Ghost)
		{
			Ghost->SetActorHiddenInGame(true);
		}
		return;
	}

	const APawn* Pawn = GetPawn();
	if (Def->IsStructural())
	{
		AGrimBaseCore* Core = UGrimBaseSubsystem::Get(this) ? UGrimBaseSubsystem::Get(this)->FindBaseAt(Aim) : nullptr;
		Preview.Core = Core;
		if (Core)
		{
			Preview.Slot = Core->GetGrid().Snap(Def->Row.Category, Aim, RotationQuarter);
			Preview.Result = CheckPlacement(PieceId, Core, Preview.Slot, FTransform::Identity, true, Preview.Transform);
		}
		else
		{
			Preview.Result = EGrimBuildResult::OutsideBase;
			Preview.Transform = FTransform(FRotator(0.f, Pawn ? Pawn->GetActorRotation().Yaw : 0.f, 0.f), Aim);
		}
	}
	else
	{
		const float Yaw = (Pawn ? Pawn->GetActorRotation().Yaw : 0.f) + 90.f * RotationQuarter;
		const FTransform Free(FRotator(0.f, Yaw, 0.f), Aim);
		Preview.Result = CheckPlacement(PieceId, nullptr, FGrimBuildSlot(), Free, true, Preview.Transform);
	}

	if (!Ghost)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Ghost = GetWorld()->SpawnActor<AGrimBuildGhost>(AGrimBuildGhost::StaticClass(), Preview.Transform, Params);
	}
	if (Ghost)
	{
		Ghost->ShowPiece(PieceId);
		Ghost->SetActorTransform(Preview.Transform);
		Ghost->SetValid(Preview.Result == EGrimBuildResult::Ok);
		Ghost->SetActorHiddenInGame(false);
	}
}

void UGrimBuilderComponent::PlaceFromPreview()
{
	if (Preview.Result != EGrimBuildResult::Ok)
	{
		OnNotice.Broadcast(this, EGrimBaseNotice::PlaceFailed, Preview.PieceId, static_cast<int32>(Preview.Result));
		return;
	}
	Server_PlacePiece(Preview.PieceId, Preview.Core.Get(), Preview.Slot, Preview.Transform);
}

void UGrimBuilderComponent::DemolishAimed()
{
	AActor* Target = TraceAimedActor();
	if (Target && UGrimStructureComponent::FindOn(Target))
	{
		Server_Demolish(Target);
	}
}

void UGrimBuilderComponent::UpdateInteractTarget()
{
	const APawn* Pawn = GetPawn();
	InteractTarget = nullptr;
	if (!Pawn)
	{
		return;
	}
	const float Radius = UGrimBaseSettings::Get()->InteractRadius;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimBaseInteract), false, Pawn);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), FCollisionShape::MakeSphere(Radius), Params);
	float BestDistance = Radius + 1.f;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		const IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(Actor);
		if (!Interactable || !Interactable->CanInteract(Pawn))
		{
			continue;
		}
		const float Distance = GrimBuilderLocal::DistanceToActor(Actor, Pawn->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			InteractTarget = Actor;
		}
	}
}

void UGrimBuilderComponent::OpenStation(AActor* Station)
{
	const IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(Station);
	UGrimInventoryComponent* Bag = GetBag();
	UGrimInventoryWidget* InventoryWidget = Bag ? Bag->GetWidget() : nullptr;
	if (!Interactable || !InventoryWidget || Interactable->GetStationTabId().IsNone())
	{
		return;
	}
	OpenStationActor = Station;
	InventoryWidget->OpenTab(Interactable->GetStationTabId());
}

void UGrimBuilderComponent::CloseStation()
{
	const IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(OpenStationActor.Get());
	const FName TabId = Interactable ? Interactable->GetStationTabId() : NAME_None;
	OpenStationActor = nullptr;
	UGrimInventoryComponent* Bag = GetBag();
	UGrimInventoryWidget* InventoryWidget = Bag ? Bag->GetWidget() : nullptr;
	if (InventoryWidget && !TabId.IsNone() && InventoryWidget->GetActiveTab() == TabId)
	{
		InventoryWidget->SelectTab(NAME_None);
	}
}

// ───── 검사 ─────

EGrimBuildResult UGrimBuilderComponent::CheckPlacement(FName PieceId, const AGrimBaseCore* Core, const FGrimBuildSlot& InSlot, const FTransform& FreeTransform,
	bool bCheckReach, FTransform& OutTransform) const
{
	using namespace GrimBuilderLocal;
	const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
	if (!Def)
	{
		return EGrimBuildResult::UnknownPiece;
	}
	const APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		return EGrimBuildResult::NoPawn;
	}
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this);
	UWorld* World = GetWorld();
	if (!Subsystem || !World)
	{
		return EGrimBuildResult::NoPawn;
	}

	if (Def->IsStructural())
	{
		if (!Core)
		{
			return EGrimBuildResult::OutsideBase;
		}
		FGrimBuildSlot Slot = InSlot;
		Slot.Category = Def->Row.Category;
		const FGrimBuildGrid Grid = Core->GetGrid();
		OutTransform = Grid.GetSlotTransform(Slot);
		if (!Core->IsInside(OutTransform.GetLocation()))
		{
			return EGrimBuildResult::OutsideBase;
		}
		if (bCheckReach && FVector::Dist(Pawn->GetActorLocation(), OutTransform.GetLocation()) > Settings->BuildReach)
		{
			return EGrimBuildResult::OutOfReach;
		}
		if (Slot.Level < 0 || Slot.Level > Grid.MaxLevel)
		{
			return EGrimBuildResult::NoSupport;
		}
		TArray<FGrimBuildSlot> Existing;
		for (const AGrimBuildPiece* Piece : Subsystem->GetPieces(Core))
		{
			if (Piece->GetSlot().GetOccupancyKey() == Slot.GetOccupancyKey())
			{
				return EGrimBuildResult::Occupied;
			}
			Existing.Add(Piece->GetSlot());
		}
		if (!FGrimBuildSupport::HasSupport(Slot, FGrimBuildSupport::ComputeStable(Existing)))
		{
			return EGrimBuildResult::NoSupport;
		}
		// 사람을 가두지 않게 — 막는 상자가 폰과 겹치면 안 된다.
		TArray<FGrimShapeBox> Boxes;
		Grid.GetShape(Slot.Category, Boxes);
		FCollisionObjectQueryParams PawnObjects;
		PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
		for (const FGrimShapeBox& Box : Boxes)
		{
			if (Box.bDoorLeaf)
			{
				continue;
			}
			const FVector Center = OutTransform.TransformPosition(Box.Center);
			if (World->OverlapAnyTestByObjectType(Center, OutTransform.GetRotation(), PawnObjects, FCollisionShape::MakeBox(Box.Extent * 0.95f)))
			{
				return EGrimBuildResult::Blocked;
			}
		}
	}
	else
	{
		OutTransform = FTransform(FRotator(0.f, FreeTransform.Rotator().Yaw, 0.f), FreeTransform.GetLocation());
		const FVector Location = OutTransform.GetLocation();
		if (bCheckReach && FVector::Dist(Pawn->GetActorLocation(), Location) > Settings->BuildReach)
		{
			return EGrimBuildResult::OutOfReach;
		}
		if (Def->Row.Category == EGrimBuildCategory::Core)
		{
			if (Subsystem->CountCoresOwnedBy(GetOwnerKey()) >= Settings->MaxBasesPerPlayer)
			{
				return EGrimBuildResult::TooManyBases;
			}
			for (const AGrimBaseCore* Other : Subsystem->GetCores())
			{
				if (FVector::Dist2D(Other->GetActorLocation(), Location) < Settings->CoreSpacing)
				{
					return EGrimBuildResult::TooCloseToBase;
				}
			}
		}
		// 바닥이 있어야 한다(땅 · 바닥 조각).
		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimBuildGround), false, Pawn);
		if (!World->LineTraceSingleByObjectType(Ground, Location + FVector(0.f, 0.f, 50.f), Location - FVector(0.f, 0.f, 60.f), SolidObjects(false), Params))
		{
			return EGrimBuildResult::NoGround;
		}
		// 다른 것과 겹치지 않게(바닥에서 조금 띄운 상자).
		const FVector Extent = Def->Row.Extent * 0.9f;
		const FVector Center = Location + FVector(0.f, 0.f, Def->Row.Extent.Z + 10.f);
		if (World->OverlapAnyTestByObjectType(Center, OutTransform.GetRotation(), SolidObjects(true), FCollisionShape::MakeBox(Extent)))
		{
			return EGrimBuildResult::Blocked;
		}
	}

	// 재료(마지막 — 다른 까닭을 먼저 보여 준다).
	const UGrimInventoryComponent* Bag = GetBag();
	for (const FGrimRecipeInput& Input : Def->Cost)
	{
		if (!Bag || Bag->CountOf(Input.Id) < Input.Count)
		{
			return EGrimBuildResult::NotEnoughMaterials;
		}
	}
	return EGrimBuildResult::Ok;
}

// ───── 서버 ─────

void UGrimBuilderComponent::GiveOrDrop(const FGrimItemInstance& Item, const FVector& DropLocation)
{
	UGrimInventoryComponent* Bag = GetBag();
	const int32 Added = Bag ? Bag->AddItem(Item, true) : 0;
	if (Added < Item.Count)
	{
		UGrimItemStatics::SpawnPickup(GetOwner(), Item.ItemId, Item.Count - Added, DropLocation);
	}
}

bool UGrimBuilderComponent::SpendMana(float Amount)
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}
	if (Amount <= 0.f || ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super))
	{
		return true;
	}
	const float Mana = ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute());
	if (Mana + KINDA_SMALL_NUMBER < Amount)
	{
		return false;
	}
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), Mana - Amount);
	return true;
}

void UGrimBuilderComponent::SendNotice(EGrimBaseNotice Notice, FName Item, int32 CountOrResult)
{
	if (GetLocalOwningController())
	{
		OnNotice.Broadcast(this, Notice, Item, CountOrResult); // 리슨 호스트 자신
	}
	else if (HasAuthority())
	{
		Client_Notice(Notice, Item, CountOrResult);
	}
}

void UGrimBuilderComponent::Client_Notice_Implementation(EGrimBaseNotice Notice, FName Item, int32 CountOrResult)
{
	OnNotice.Broadcast(this, Notice, Item, CountOrResult);
}

EGrimBuildResult UGrimBuilderComponent::PlacePiece(FName PieceId, AGrimBaseCore* Core, const FGrimBuildSlot& Slot, const FTransform& FreeTransform, bool bCheckReach)
{
	if (!HasAuthority())
	{
		return EGrimBuildResult::NoPawn;
	}
	FTransform Transform;
	const EGrimBuildResult Result = CheckPlacement(PieceId, Core, Slot, FreeTransform, bCheckReach, Transform);
	if (Result != EGrimBuildResult::Ok)
	{
		FString Blocker;
		if (Result == EGrimBuildResult::Blocked)
		{
			// 무엇과 겹쳤는지(헤드리스 확인용) — 시설 상자 자리에서 다시 찾는다.
			const FGrimBuildPieceDefinition* BlockedDef = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
			if (BlockedDef && BlockedDef->IsFreePlaced())
			{
				TArray<FOverlapResult> Overlaps;
				const FVector Center = FreeTransform.GetLocation() + FVector(0.f, 0.f, BlockedDef->Row.Extent.Z + 10.f);
				GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FRotator(0.f, FreeTransform.Rotator().Yaw, 0.f).Quaternion(), GrimBuilderLocal::SolidObjects(true),
					FCollisionShape::MakeBox(BlockedDef->Row.Extent * 0.9f));
				for (const FOverlapResult& Overlap : Overlaps)
				{
					Blocker += FString::Printf(TEXT(" %s.%s"), *GetNameSafe(Overlap.GetActor()), *GetNameSafe(Overlap.GetComponent()));
				}
			}
		}
		UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 놓기 거절 %s: %s%s"), *GetNameSafe(GetOwner()), *PieceId.ToString(), *DescribeResult(Result).ToString(), *Blocker);
		SendNotice(EGrimBaseNotice::PlaceFailed, PieceId, static_cast<int32>(Result));
		return Result;
	}
	const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
	UGrimInventoryComponent* Bag = GetBag();
	for (const FGrimRecipeInput& Input : Def->Cost)
	{
		Bag->RemoveItem(Input.Id, Input.Count);
	}

	UWorld* World = GetWorld();
	AActor* Spawned = nullptr;
	if (Def->IsStructural())
	{
		FGrimBuildSlot FinalSlot = Slot;
		FinalSlot.Category = Def->Row.Category;
		AGrimBuildPiece* Piece = World->SpawnActorDeferred<AGrimBuildPiece>(AGrimBuildPiece::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Piece)
		{
			Piece->InitializePiece(PieceId, FinalSlot);
			Piece->FinishSpawning(Transform);
			Spawned = Piece;
		}
	}
	else
	{
		UClass* Class = LoadClass<AActor>(nullptr, *Def->Row.SpawnClass);
		AActor* Actor = Class ? World->SpawnActorDeferred<AActor>(Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn) : nullptr;
		if (Actor)
		{
			if (UGrimStructureComponent* Structure = UGrimStructureComponent::FindOn(Actor))
			{
				Structure->InitializePiece(PieceId);
			}
			if (AGrimBaseCore* NewCore = Cast<AGrimBaseCore>(Actor))
			{
				const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
				NewCore->SetOwnerInfo(GetOwnerKey(), PlayerState ? PlayerState->GetPlayerName() : FString());
			}
			Actor->FinishSpawning(Transform);
			Spawned = Actor;
		}
		else
		{
			UE_LOG(LogGrimBase, Error, TEXT("[Server] %s: 스폰할 클래스를 못 찾음 %s"), *PieceId.ToString(), *Def->Row.SpawnClass);
		}
	}
	if (!Spawned)
	{
		return EGrimBuildResult::UnknownPiece;
	}
	FGrimBuildSlot LoggedSlot = Slot;
	LoggedSlot.Category = Def->Row.Category;
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 놓음 %s → %s %s"), *GetNameSafe(GetOwner()), *PieceId.ToString(), *Spawned->GetName(),
		Def->IsStructural() ? *LoggedSlot.ToString() : *Transform.GetLocation().ToCompactString());
	UGrimQuestComponent::NotifyQuestEvent(GetOwner(), Def->Row.Category == EGrimBuildCategory::Core ? GrimBaseEvents::CorePlaced : GrimBaseEvents::PiecePlaced(Def->Row.Category));
	SendNotice(EGrimBaseNotice::Placed, PieceId, 1);
	return EGrimBuildResult::Ok;
}

bool UGrimBuilderComponent::Demolish(AActor* Target, bool bCheckReach)
{
	UGrimStructureComponent* Structure = UGrimStructureComponent::FindOn(Target);
	if (!HasAuthority() || !Structure || (bCheckReach && !IsWithinReach(Target, UGrimBaseSettings::Get()->BuildReach)))
	{
		return false;
	}
	if (const AGrimBaseCore* Core = Cast<AGrimBaseCore>(Target); Core && !Core->GetOwnerId().IsEmpty() && Core->GetOwnerId() != GetOwnerKey())
	{
		SendNotice(EGrimBaseNotice::NotOwner, Structure->GetPieceId());
		return false;
	}
	const FName PieceId = Structure->GetPieceId();
	for (const FGrimItemInstance& Item : Structure->GetRefundItems(UGrimBaseSettings::Get()->DemolishRefundFraction))
	{
		GiveOrDrop(Item, Target->GetActorLocation());
	}
	Structure->Break(false, TEXT("철거"));
	SendNotice(EGrimBaseNotice::Demolished, PieceId, 1);
	return true;
}

bool UGrimBuilderComponent::InteractWith(AActor* Target, bool bCheckReach)
{
	IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(Target);
	APawn* Pawn = GetPawn();
	if (!HasAuthority() || !Interactable || !Pawn || !Interactable->CanInteract(Pawn)
		|| (bCheckReach && !IsWithinReach(Target, UGrimBaseSettings::Get()->InteractRadius)))
	{
		return false;
	}
	Interactable->InteractServer(this, Pawn);
	return true;
}

bool UGrimBuilderComponent::TransferStorage(AGrimStorageChest* Chest, bool bToChest, int32 SlotIndex, int32 Count, bool bCheckReach)
{
	UGrimInventoryComponent* Bag = GetBag();
	UGrimItemContainerComponent* Container = Chest ? Chest->GetContainer() : nullptr;
	if (!HasAuthority() || !Bag || !Container || (bCheckReach && !IsWithinReach(Chest, UGrimBaseSettings::Get()->StationOpenRadius)))
	{
		return false;
	}
	if (bToChest)
	{
		if (!Bag->GetSlots().IsValidIndex(SlotIndex) || Bag->GetSlots()[SlotIndex].IsEmpty())
		{
			return false;
		}
		FGrimItemInstance Moving = Bag->GetSlots()[SlotIndex];
		Moving.Count = Count < 0 ? Moving.Count : FMath::Min(Count, Moving.Count);
		const int32 Added = Container->AddItem(Moving);
		if (Added <= 0)
		{
			SendNotice(EGrimBaseNotice::StorageFull, Moving.ItemId, Moving.Count);
			return false;
		}
		Bag->RemoveFromSlot(SlotIndex, Added);
		SendNotice(EGrimBaseNotice::Stored, Moving.ItemId, Added);
		return true;
	}
	if (!Container->GetSlots().IsValidIndex(SlotIndex) || Container->GetSlots()[SlotIndex].IsEmpty())
	{
		return false;
	}
	FGrimItemInstance Moving = Container->GetSlots()[SlotIndex];
	Moving.Count = Count < 0 ? Moving.Count : FMath::Min(Count, Moving.Count);
	const int32 Added = Bag->AddItem(Moving, false);
	if (Added <= 0)
	{
		SendNotice(EGrimBaseNotice::BagFull, Moving.ItemId, Moving.Count);
		return false;
	}
	Container->RemoveFromSlot(SlotIndex, Added);
	SendNotice(EGrimBaseNotice::TookOut, Moving.ItemId, Added);
	return true;
}

bool UGrimBuilderComponent::Craft(AGrimWorkbench* Workbench, FName RecipeId, bool bCheckReach)
{
	UGrimInventoryComponent* Bag = GetBag();
	if (!HasAuthority() || !Bag || !Workbench || (bCheckReach && !IsWithinReach(Workbench, UGrimBaseSettings::Get()->StationOpenRadius)))
	{
		return false;
	}
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	const FGrimRecipeDefinition* Recipe = Items.Recipes.FindByPredicate([RecipeId](const FGrimRecipeDefinition& Candidate)
	{
		return Candidate.Id == RecipeId && Candidate.Row.Station == AGrimWorkbench::StationId;
	});
	if (!Recipe)
	{
		return false;
	}
	const TArray<FGrimItemInstance> Before = Bag->GetSlots();
	TArray<FGrimItemInstance> After = Before;
	if (!FGrimSlotTransferMath::ConsumeInputs(After, Items, Recipe->Inputs))
	{
		SendNotice(EGrimBaseNotice::CraftFailed, Recipe->Row.Output, 0);
		return false;
	}
	for (int32 Index = 0; Index < Before.Num(); ++Index)
	{
		const int32 Removed = Before[Index].Count - (After.IsValidIndex(Index) && After[Index].ItemId == Before[Index].ItemId ? After[Index].Count : 0);
		if (!Before[Index].IsEmpty() && Removed > 0)
		{
			Bag->RemoveFromSlot(Index, Removed);
		}
	}
	GiveOrDrop(FGrimItemInstance(Recipe->Row.Output, FMath::Max(1, Recipe->Row.OutputCount)), Workbench->GetActorLocation() + FVector(0.f, 0.f, 100.f));
	UGrimQuestComponent::NotifyQuestEvent(GetOwner(), GrimBaseEvents::Crafted(Recipe->Row.Output));
	SendNotice(EGrimBaseNotice::Crafted, Recipe->Row.Output, FMath::Max(1, Recipe->Row.OutputCount));
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 작업대 제작 %s → %s ×%d"), *GetNameSafe(GetOwner()), *RecipeId.ToString(), *Recipe->Row.Output.ToString(), Recipe->Row.OutputCount);
	return true;
}

bool UGrimBuilderComponent::PlantOrHarvest(AGrimFarmPlot* Plot)
{
	UGrimInventoryComponent* Bag = GetBag();
	if (!HasAuthority() || !Plot || !Bag)
	{
		return false;
	}
	if (Plot->IsMature())
	{
		const FName CropId = Plot->GetCropId();
		const TArray<FGrimItemInstance> Harvested = Plot->Harvest();
		for (const FGrimItemInstance& Item : Harvested)
		{
			GiveOrDrop(Item, Plot->GetActorLocation() + FVector(0.f, 0.f, 60.f));
		}
		UGrimQuestComponent::NotifyQuestEvent(GetOwner(), GrimBaseEvents::Harvested(CropId));
		SendNotice(EGrimBaseNotice::Harvested, Harvested.Num() > 0 ? Harvested[0].ItemId : NAME_None, Harvested.Num() > 0 ? Harvested[0].Count : 0);
		return true;
	}
	if (!Plot->GetCropId().IsNone())
	{
		SendNotice(EGrimBaseNotice::StillGrowing, Plot->GetCropId(), FMath::RoundToInt(Plot->GetGrowth() * 100.f));
		return false;
	}
	const FGrimBaseDatabase& Database = UGrimBaseSettings::GetDatabase();
	for (int32 Index = 0; Index < Bag->GetSlots().Num(); ++Index)
	{
		const FGrimItemInstance& Item = Bag->GetSlots()[Index];
		const FGrimCropDefinition* Crop = Item.IsEmpty() ? nullptr : Database.FindCropBySeed(Item.ItemId);
		if (Crop && Plot->Plant(Crop->Id))
		{
			Bag->RemoveFromSlot(Index, 1);
			SendNotice(EGrimBaseNotice::Planted, Crop->Row.SeedItem, 1);
			UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 밭 %s 에 %s 심음"), *GetNameSafe(GetOwner()), *Plot->GetName(), *Crop->Id.ToString());
			return true;
		}
	}
	SendNotice(EGrimBaseNotice::NoSeed);
	return false;
}

bool UGrimBuilderComponent::KnowsRepairSpell() const
{
	if (!UGrimBaseSettings::Get()->bRequireRepairSpell)
	{
		return true;
	}
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	if (!Spellbook)
	{
		return false;
	}
	for (const UGrimSpellDefinition* Spell : Spellbook->GetKnownSpells())
	{
		if (Spell && Spell->SpellTag.MatchesTagExact(GrimBaseTags::Spell_Repair))
		{
			return true;
		}
	}
	return false;
}

int32 UGrimBuilderComponent::PerformRepair(AActor* Target, bool bFromSpell, bool bCheckReach)
{
	using namespace GrimBuilderLocal;
	UGrimInventoryComponent* Bag = GetBag();
	if (!HasAuthority() || !Target || !Bag)
	{
		return 0;
	}
	if (!KnowsRepairSpell())
	{
		SendNotice(EGrimBaseNotice::NeedRepairSpell);
		return 0;
	}
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	if (bCheckReach && !IsWithinReach(Target, FMath::Max(Settings->BuildReach, Settings->StationOpenRadius)))
	{
		return 0;
	}
	const UGrimSpellDefinition* Spell = Settings->RepairSpell.LoadSynchronous();
	const float SpellMana = bFromSpell ? 0.f : (Spell ? Spell->ManaCost : 15.f);

	// 작업대 = 내 장비
	if (Cast<AGrimWorkbench>(Target))
	{
		UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(GetOwner());
		if (!Equipment)
		{
			return 0;
		}
		if (!SpendMana(SpellMana))
		{
			SendNotice(EGrimBaseNotice::NoMana);
			return 0;
		}
		const int32 Used = Equipment->RepairAll(Bag);
		SendNotice(Used > 0 ? EGrimBaseNotice::Repaired : EGrimBaseNotice::NothingToRepair, NAME_None, Used);
		return Used;
	}

	// 거점 핵 = 거점의 닳은 것을 저장고의 재료로(마나 많이 듦)
	if (AGrimBaseCore* Core = Cast<AGrimBaseCore>(Target))
	{
		UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this);
		TArray<UGrimItemContainerComponent*> Storages;
		for (TActorIterator<AGrimStorageChest> It(GetWorld()); It; ++It)
		{
			if (Core->IsInside(It->GetActorLocation()) && It->GetContainer())
			{
				Storages.Add(It->GetContainer());
			}
		}
		TArray<UGrimStructureComponent*> Damaged;
		for (UGrimStructureComponent* Structure : Subsystem ? Subsystem->GetStructures() : TArray<UGrimStructureComponent*>())
		{
			if (Structure->IsDamaged() && Structure->FindBaseCore() == Core && !Structure->GetRepairItem().IsNone())
			{
				Damaged.Add(Structure);
			}
		}
		if (Damaged.Num() == 0)
		{
			SendNotice(EGrimBaseNotice::NothingToRepair);
			return 0;
		}
		if (!SpendMana(SpellMana + Settings->BaseRepairManaCost))
		{
			SendNotice(EGrimBaseNotice::NoMana);
			return 0;
		}
		int32 Used = 0;
		for (UGrimStructureComponent* Structure : Damaged)
		{
			int32 Needed = Structure->GetMaterialsNeeded();
			int32 Taken = 0;
			for (UGrimItemContainerComponent* Storage : Storages)
			{
				if (Needed - Taken <= 0)
				{
					break;
				}
				Taken += Storage->RemoveItem(Structure->GetRepairItem(), Needed - Taken);
			}
			Structure->ApplyRepairMaterials(Taken);
			Used += Taken;
		}
		UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 거점 수리: 닳은 것 %d, 저장고 재료 %d"), *GetNameSafe(GetOwner()), Damaged.Num(), Used);
		SendNotice(Used > 0 ? EGrimBaseNotice::BaseRepaired : EGrimBaseNotice::NoRepairMaterials, NAME_None, Used);
		return Used;
	}

	// 조각 · 시설 = 가방의 재료로 그것을
	UGrimStructureComponent* Structure = UGrimStructureComponent::FindOn(Target);
	if (!Structure)
	{
		return 0;
	}
	const int32 Needed = Structure->GetMaterialsNeeded();
	if (Needed <= 0)
	{
		SendNotice(EGrimBaseNotice::NothingToRepair);
		return 0;
	}
	const int32 Use = FMath::Min(Needed, Bag->CountOf(Structure->GetRepairItem()));
	if (Use <= 0)
	{
		SendNotice(EGrimBaseNotice::NoRepairMaterials, Structure->GetRepairItem());
		return 0;
	}
	if (!SpendMana(SpellMana))
	{
		SendNotice(EGrimBaseNotice::NoMana);
		return 0;
	}
	Bag->RemoveItem(Structure->GetRepairItem(), Use);
	Structure->ApplyRepairMaterials(Use);
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 수리 %s: %s ×%d → %.0f/%.0f"), *GetNameSafe(GetOwner()), *Target->GetName(), *Structure->GetRepairItem().ToString(), Use,
		Structure->GetHealth(), Structure->GetMaxHealth());
	SendNotice(EGrimBaseNotice::Repaired, Structure->GetRepairItem(), Use);
	return Use;
}

// ───── RPC ─────

void UGrimBuilderComponent::Server_PlacePiece_Implementation(FName PieceId, AGrimBaseCore* Core, FGrimBuildSlot Slot, FTransform FreeTransform)
{
	PlacePiece(PieceId, Core, Slot, FreeTransform, true);
}

void UGrimBuilderComponent::Server_Demolish_Implementation(AActor* Target)
{
	Demolish(Target, true);
}

void UGrimBuilderComponent::Server_Interact_Implementation(AActor* Target)
{
	InteractWith(Target, true);
}

void UGrimBuilderComponent::Server_TransferStorage_Implementation(AGrimStorageChest* Chest, bool bToChest, int32 SlotIndex, int32 Count)
{
	TransferStorage(Chest, bToChest, SlotIndex, Count, true);
}

void UGrimBuilderComponent::Server_Craft_Implementation(AGrimWorkbench* Workbench, FName RecipeId)
{
	Craft(Workbench, RecipeId, true);
}

void UGrimBuilderComponent::Server_Repair_Implementation(AActor* Target)
{
	PerformRepair(Target, false, true);
}

// ───── 알림 글 ─────

FText UGrimBuilderComponent::DescribeResult(EGrimBuildResult Result)
{
	switch (Result)
	{
	case EGrimBuildResult::Ok: return LOCTEXT("ResultOk", "놓을 수 있다");
	case EGrimBuildResult::UnknownPiece: return LOCTEXT("ResultUnknown", "모르는 조각");
	case EGrimBuildResult::NoPawn: return LOCTEXT("ResultNoPawn", "몸이 없다");
	case EGrimBuildResult::OutOfReach: return LOCTEXT("ResultReach", "너무 멀다");
	case EGrimBuildResult::OutsideBase: return LOCTEXT("ResultOutside", "거점 밖 — 구조 조각은 거점 핵 둘레에만 짓는다");
	case EGrimBuildResult::Occupied: return LOCTEXT("ResultOccupied", "이미 조각이 있다");
	case EGrimBuildResult::NoSupport: return LOCTEXT("ResultSupport", "받침이 없다(바닥 · 벽 · 기둥 위에)");
	case EGrimBuildResult::NotEnoughMaterials: return LOCTEXT("ResultMaterials", "재료가 모자라다");
	case EGrimBuildResult::TooManyBases: return LOCTEXT("ResultTooMany", "거점은 더 놓을 수 없다");
	case EGrimBuildResult::TooCloseToBase: return LOCTEXT("ResultTooClose", "다른 거점과 너무 가깝다");
	case EGrimBuildResult::Blocked: return LOCTEXT("ResultBlocked", "무언가와 겹친다");
	case EGrimBuildResult::NoGround: return LOCTEXT("ResultGround", "놓을 바닥이 없다");
	default: return FText::GetEmpty();
	}
}

FText UGrimBuilderComponent::DescribeNotice(EGrimBaseNotice Notice, FName Item, int32 CountOrResult)
{
	const FGrimBuildPieceDefinition* Piece = UGrimBaseSettings::GetDatabase().FindPiece(Item);
	const FText ItemName = Piece ? Piece->GetDisplayName() : UGrimItemStatics::GetItemDisplayName(Item);
	switch (Notice)
	{
	case EGrimBaseNotice::Placed: return FText::Format(LOCTEXT("Placed", "{0} 을(를) 지었다"), ItemName);
	case EGrimBaseNotice::PlaceFailed: return FText::Format(LOCTEXT("PlaceFailed", "{0}: {1}"), ItemName, DescribeResult(static_cast<EGrimBuildResult>(CountOrResult)));
	case EGrimBaseNotice::Demolished: return FText::Format(LOCTEXT("Demolished", "{0} 을(를) 헐었다 — 재료 절반을 돌려받았다"), ItemName);
	case EGrimBaseNotice::NotOwner: return LOCTEXT("NotOwner", "남의 거점 핵은 헐 수 없다");
	case EGrimBaseNotice::Crafted: return FText::Format(LOCTEXT("Crafted", "{0} ×{1} 을(를) 만들었다"), ItemName, CountOrResult);
	case EGrimBaseNotice::CraftFailed: return FText::Format(LOCTEXT("CraftFailed", "{0}: 재료가 모자라다"), ItemName);
	case EGrimBaseNotice::Repaired: return FText::Format(LOCTEXT("Repaired", "고쳤다 — 재료 {0}개"), CountOrResult);
	case EGrimBaseNotice::NothingToRepair: return LOCTEXT("NothingToRepair", "고칠 것이 없다");
	case EGrimBaseNotice::NeedRepairSpell: return LOCTEXT("NeedRepairSpell", "「수리」 마법을 배워야 고칠 수 있다(스킬북 · 마법탑)");
	case EGrimBaseNotice::NoRepairMaterials: return Item.IsNone() ? LOCTEXT("NoRepairMaterialsAny", "수리 재료가 없다") : FText::Format(LOCTEXT("NoRepairMaterials", "수리 재료({0})가 없다"), ItemName);
	case EGrimBaseNotice::NoMana: return LOCTEXT("NoMana", "마나가 모자라다");
	case EGrimBaseNotice::BaseRepaired: return FText::Format(LOCTEXT("BaseRepaired", "거점을 고쳤다 — 저장고 재료 {0}개"), CountOrResult);
	case EGrimBaseNotice::Planted: return FText::Format(LOCTEXT("Planted", "{0} 을(를) 심었다"), ItemName);
	case EGrimBaseNotice::NoSeed: return LOCTEXT("NoSeed", "심을 씨앗이 없다");
	case EGrimBaseNotice::Harvested: return FText::Format(LOCTEXT("Harvested", "{0} ×{1} 을(를) 거뒀다"), ItemName, CountOrResult);
	case EGrimBaseNotice::StillGrowing: return FText::Format(LOCTEXT("StillGrowing", "아직 자라는 중 — {0}%"), CountOrResult);
	case EGrimBaseNotice::StorageFull: return LOCTEXT("StorageFull", "저장고가 가득 찼다");
	case EGrimBaseNotice::BagFull: return LOCTEXT("BagFull", "가방이 가득 찼다");
	case EGrimBaseNotice::Stored: return FText::Format(LOCTEXT("Stored", "{0} ×{1} 을(를) 저장고에 넣었다"), ItemName, CountOrResult);
	case EGrimBaseNotice::TookOut: return FText::Format(LOCTEXT("TookOut", "{0} ×{1} 을(를) 꺼냈다"), ItemName, CountOrResult);
	default: return FText::GetEmpty();
	}
}

// ───── 콘솔(개발 빌드) ─────

void UGrimBuilderComponent::ServerDebugCommand_Implementation(const FString& Command, const FString& Arg, const TArray<int32>& Numbers)
{
#if !UE_BUILD_SHIPPING
	using namespace GrimBuilderLocal;
	APawn* Pawn = GetPawn();
	UWorld* World = GetWorld();
	UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this);
	if (!Pawn || !World || !Subsystem)
	{
		UE_LOG(LogGrimBase, Warning, TEXT("[Server] Grim.Base.%s: 폰이 없다"), *Command);
		return;
	}
	auto Num = [&Numbers](int32 Index, int32 Default = 0) { return Numbers.IsValidIndex(Index) ? Numbers[Index] : Default; };
	auto NearestCore = [&]() -> AGrimBaseCore*
	{
		AGrimBaseCore* Best = Subsystem->FindBaseAt(Pawn->GetActorLocation());
		if (!Best)
		{
			float BestDistance = TNumericLimits<float>::Max();
			for (AGrimBaseCore* Core : Subsystem->GetCores())
			{
				const float Distance = FVector::Dist(Core->GetActorLocation(), Pawn->GetActorLocation());
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					Best = Core;
				}
			}
		}
		return Best;
	};
	auto FindNamed = [World](const FString& Name) -> AActor*
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName() == Name)
			{
				return *It;
			}
		}
		return nullptr;
	};
	auto SurfaceAt = [&](const FVector& Location) -> FVector
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimBaseDebugSurface), false, Pawn);
		if (World->LineTraceSingleByObjectType(Hit, Location + FVector(0.f, 0.f, 150.f), Location - FVector(0.f, 0.f, 600.f), SolidObjects(false), Params))
		{
			return Hit.ImpactPoint;
		}
		return Location;
	};

	if (Command == TEXT("Core"))
	{
		// 폰 앞 Numbers[0] cm(기본 400) 땅에, 폰 방향으로.
		const float Yaw = Pawn->GetActorRotation().Yaw;
		const FVector Location = SurfaceAt(Pawn->GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * Num(0, 400));
		PlacePiece(Arg.IsEmpty() ? FName(TEXT("BaseCore")) : FName(*Arg), nullptr, FGrimBuildSlot(), FTransform(FRotator(0.f, Yaw, 0.f), Location), false);
	}
	else if (Command == TEXT("Place"))
	{
		// Place <조각> <X> <Y> <층> <방향> — 가장 가까운 거점의 격자에.
		FGrimBuildSlot Slot;
		Slot.X = Num(0);
		Slot.Y = Num(1);
		Slot.Level = Num(2);
		Slot.Dir = Num(3);
		PlacePiece(FName(*Arg), NearestCore(), Slot, FTransform::Identity, false);
	}
	else if (Command == TEXT("PlaceAt"))
	{
		// PlaceAt <시설> <X> <Y> <로컬 X cm> <로컬 Y cm> <90° 단위 방향> — 그 칸 바닥 위(바닥 조각 · 땅)에.
		if (AGrimBaseCore* Core = NearestCore())
		{
			const FGrimBuildGrid Grid = Core->GetGrid();
			const FVector Local(Num(0) * Grid.CellSize + Num(2), Num(1) * Grid.CellSize + Num(3), Grid.GetFloorTop(0) + 20.f);
			const FVector Location = SurfaceAt(Grid.LocalToWorld(Local));
			PlacePiece(FName(*Arg), Core, FGrimBuildSlot(), FTransform(FRotator(0.f, Grid.Yaw + 90.f * Num(4), 0.f), Location), false);
		}
	}
	else if (Command == TEXT("Teleport"))
	{
		// Teleport <X> <Y> <층> — 그 칸 바닥 위로.
		if (AGrimBaseCore* Core = NearestCore())
		{
			const FGrimBuildGrid Grid = Core->GetGrid();
			const FVector Target = Grid.LocalToWorld(FVector(Num(0) * Grid.CellSize, Num(1) * Grid.CellSize, Grid.GetFloorTop(Num(2)) + 100.f));
			Pawn->TeleportTo(Target, Pawn->GetActorRotation(), false, true);
		}
	}
	else if (Command == TEXT("Demolish") || Command == TEXT("Door") || Command == TEXT("Repair") || Command == TEXT("Damage") || Command == TEXT("SetHealth"))
	{
		AActor* Target = FindNamed(Arg);
		if (!Target)
		{
			UE_LOG(LogGrimBase, Warning, TEXT("[Server] Grim.Base.%s: 액터 %s 없음"), *Command, *Arg);
			return;
		}
		if (Command == TEXT("Demolish"))
		{
			Demolish(Target, false);
		}
		else if (Command == TEXT("Door"))
		{
			InteractWith(Target, false);
		}
		else if (Command == TEXT("Repair"))
		{
			PerformRepair(Target, false, false);
		}
		else if (UGrimStructureComponent* Structure = UGrimStructureComponent::FindOn(Target))
		{
			if (Command == TEXT("Damage"))
			{
				Structure->ApplyStructureDamage(static_cast<float>(Num(0, 10)), TEXT("콘솔"));
			}
			else
			{
				Structure->SetHealth(static_cast<float>(Num(0, 1)));
			}
		}
	}
	else if (Command == TEXT("RepairNearest"))
	{
		// 가장 가까운 작업대(장비) · 거점 핵(거점 수리) — Arg = Workbench | Core
		AActor* Target = Arg == TEXT("Core") ? static_cast<AActor*>(NearestCore()) : static_cast<AActor*>(FindNearest<AGrimWorkbench>(World, Pawn->GetActorLocation(), 100000.f));
		PerformRepair(Target, false, false);
	}
	else if (Command == TEXT("LearnRepair"))
	{
		const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
		UGrimSpellDefinition* Spell = UGrimBaseSettings::Get()->RepairSpell.LoadSynchronous();
		UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
		const bool bLearned = Spell && Spellbook && Spellbook->LearnSpell(Spell);
		UE_LOG(LogGrimBase, Display, TEXT("[Server] 「수리」 배우기: %s(%s)"), bLearned ? TEXT("배움") : TEXT("못 배움 · 이미 앎"), *GetNameSafe(Spell));
	}
	else if (Command == TEXT("Store") || Command == TEXT("Take"))
	{
		// Store <아이템> [개수] · Take <아이템> [개수] — 가장 가까운 저장고
		AGrimStorageChest* Chest = FindNearest<AGrimStorageChest>(World, Pawn->GetActorLocation(), 100000.f);
		UGrimInventoryComponent* Bag = GetBag();
		if (!Chest || !Bag)
		{
			return;
		}
		const bool bToChest = Command == TEXT("Store");
		const TArray<FGrimItemInstance>& Source = bToChest ? Bag->GetSlots() : Chest->GetContainer()->GetSlots();
		const int32 SlotIndex = Source.IndexOfByPredicate([&Arg](const FGrimItemInstance& Item) { return Item.ItemId == FName(*Arg); });
		TransferStorage(Chest, bToChest, SlotIndex, Num(0, -1), false);
	}
	else if (Command == TEXT("Craft"))
	{
		Craft(FindNearest<AGrimWorkbench>(World, Pawn->GetActorLocation(), 100000.f), FName(*Arg), false);
	}
	else if (Command == TEXT("Plot"))
	{
		PlantOrHarvest(FindNearest<AGrimFarmPlot>(World, Pawn->GetActorLocation(), 100000.f));
	}
	else if (Command == TEXT("Grow"))
	{
		for (TActorIterator<AGrimFarmPlot> It(World); It; ++It)
		{
			It->AdvanceGrowth(static_cast<float>(Num(0, 60)));
		}
	}
	else if (Command == TEXT("Equip"))
	{
		UGrimInventoryComponent* Bag = GetBag();
		UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(GetOwner());
		const int32 SlotIndex = Bag ? Bag->GetSlots().IndexOfByPredicate([&Arg](const FGrimItemInstance& Item) { return Item.ItemId == FName(*Arg); }) : INDEX_NONE;
		if (Equipment && SlotIndex != INDEX_NONE)
		{
			Equipment->EquipFromBag(SlotIndex);
		}
	}
	else if (Command == TEXT("Unequip") || Command == TEXT("Wear"))
	{
		if (UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(GetOwner()))
		{
			const EGrimEquipSlot Slot = static_cast<EGrimEquipSlot>(FMath::Clamp(Num(0), 0, static_cast<int32>(EGrimEquipSlot::Count) - 1));
			if (Command == TEXT("Unequip"))
			{
				Equipment->Unequip(Slot);
			}
			else
			{
				Equipment->Wear(Slot, static_cast<float>(Num(1, 10)));
			}
		}
	}
	else if (Command == TEXT("Collapse"))
	{
		Subsystem->CollapseUnsupported();
	}
	else
	{
		UE_LOG(LogGrimBase, Warning, TEXT("[Server] Grim.Base: 모르는 명령 %s"), *Command);
	}
#endif
}

#undef LOCTEXT_NAMESPACE
