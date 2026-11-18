#include "Dressing/GrimReactiveProp.h"
#include "Dressing/GrimDressingSettings.h"
#include "Dressing/GrimDressingTypes.h"
#include "Dressing/GrimJournalComponent.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Items/GrimItemStatics.h"
#include "Save/GrimPlayerIdentity.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GrimReactiveProp"

namespace GrimReactivePropLocal
{
	/** 그 폰의 PlayerId — 클라이언트는 자기 것(로그인 옵션과 같은 값), 서버는 ResolveServerSide */
	FString KeyForPawn(const APawn* Pawn)
	{
		if (!Pawn)
		{
			return FString();
		}
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()); PC && PC->IsLocalController())
		{
			return FGrimPlayerIdentity::GetLocalPlayerId(PC->GetLocalPlayer());
		}
		if (Pawn->HasAuthority())
		{
			return FGrimPlayerIdentity::ResolveServerSide(Pawn->GetPlayerState());
		}
		return FString();
	}
}

AGrimReactiveProp::AGrimReactiveProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// 책장이 바람에 날려 옮겨 가면 자리도 복제한다(휴면 — 옮길 때만 깨운다)
	SetReplicatingMovement(true);

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(Root);
	// 마법(투사체는 WorldDynamic 을 막는다) · 비(WorldDynamic 오버랩) · 불 번짐이 이 상자로 닿는다. 폰은 겹치기만(행 bBlocksPawn 이면 막는다)
	HitBox->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	HitBox->SetCanEverAffectNavigation(false);
	HitBox->SetGenerateOverlapEvents(false);

	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(false);
	BodyMesh->SetCanEverAffectNavigation(false);

	AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentMesh"));
	AccentMesh->SetupAttachment(Root);
	AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AccentMesh->SetCastShadow(false);
	AccentMesh->SetCanEverAffectNavigation(false);

	LoopFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LoopFX"));
	LoopFX->SetupAttachment(Root);
	LoopFX->SetAutoActivate(false);

	FlameLight->SetCastShadows(false);
	ElementalState->Material.Reset();
}

void AGrimReactiveProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGrimReactiveProp, PropId, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGrimReactiveProp, JournalOverride, COND_InitialOnly);
	DOREPLIFETIME(AGrimReactiveProp, CollectedBy);
}

const FGrimReactivePropRow* AGrimReactiveProp::GetRow() const
{
	return FGrimDressingData::FindProp(PropId);
}

FName AGrimReactiveProp::GetJournalId() const
{
	if (!JournalOverride.IsNone())
	{
		return JournalOverride;
	}
	const FGrimReactivePropRow* Row = GetRow();
	return Row ? Row->JournalId : NAME_None;
}

TArray<FString> AGrimReactiveProp::GetPropIdOptions() const
{
	TArray<FString> Options;
	if (const UDataTable* Table = FGrimDressingData::GetPropTable())
	{
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			Options.Add(Pair.Key.ToString());
		}
	}
	return Options;
}

void AGrimReactiveProp::SetPropId(FName NewPropId)
{
	PropId = NewPropId;
	ApplyDefinition();
	if (HasActorBegunPlay())
	{
		RefreshPropVisuals();
	}
}

void AGrimReactiveProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDefinition();
	RefreshPropVisuals();
}

void AGrimReactiveProp::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// 놓인 액터는 게임에서 OnConstruction 이 다시 돌지 않는다 — 표 값(재질 · 처음 상태)을 컴포넌트 BeginPlay 전에 입힌다
	ApplyDefinition();
}

void AGrimReactiveProp::ApplyDefinition()
{
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row)
	{
		if (!PropId.IsNone())
		{
			UE_LOG(LogGrimDressing, Warning, TEXT("%s: ReactiveProps 표에 %s 가 없다"), *GetName(), *PropId.ToString());
		}
		return;
	}

	bTargetable = Row->bTargetable;

	ElementalState->Material.Reset();
	for (const FGameplayTag& Tag : Row->Materials)
	{
		if (Tag.IsValid())
		{
			ElementalState->Material.AddTag(Tag);
		}
	}
	ElementalState->InitialStates.Reset();
	if (Row->InitialState.IsValid() && Row->InitialIntensity > 0.f)
	{
		ElementalState->InitialStates.Add(Row->InitialState, Row->InitialIntensity);
	}

	HitBox->SetBoxExtent(Row->HitExtent, false);
	HitBox->SetRelativeLocation(Row->HitOffset);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, Row->bBlocksPawn ? ECR_Block : ECR_Overlap);

	FlameMesh->SetRelativeLocation(Row->FlameOffset);
	FlameScaleAtOne = Row->FlameScale;
	FlameLight->SetRelativeLocation(Row->FlameOffset + FVector(0.f, 0.f, 15.f));
	FlameLight->SetAttenuationRadius(Row->LightRadius);
	FlameLight->SetCastShadows(false);
	FlameLight->MaxDrawDistance = Row->LightMaxDrawDistance;
	LightCandelaPerIntensity = Row->LightCandela;

	BodyBaseTransform = FTransform(Row->MeshRotation, Row->MeshOffset, Row->MeshScale);
	BodyMesh->SetCachedMaxDrawDistance(Row->CullDistance);
	BodyMesh->LDMaxDrawDistance = Row->CullDistance;

	UStaticMesh* Accent = Row->AccentMesh.LoadSynchronous();
	AccentMesh->SetStaticMesh(Accent);
	AccentMesh->EmptyOverrideMaterials();
	if (UMaterialInterface* AccentMat = Row->AccentMaterial.LoadSynchronous())
	{
		AccentMesh->SetMaterial(0, AccentMat);
	}
	AccentMesh->SetRelativeLocation(Row->AccentOffset);
	AccentMesh->SetRelativeScale3D(Row->AccentScale);
	AccentMesh->LDMaxDrawDistance = Row->CullDistance;
	AccentMesh->SetCachedMaxDrawDistance(Row->CullDistance);

	LoopFX->SetAsset(Row->LoopFX.LoadSynchronous());
	LoopFX->SetRelativeLocation(Row->LoopFXOffset);
}

void AGrimReactiveProp::BeginPlay()
{
	// 컴포넌트 BeginPlay(처음 상태) 전에 묶어야 처음 상태로도 모습이 맞는다
	ElementalState->OnStateChangedNative.AddUObject(this, &AGrimReactiveProp::HandlePropStateChanged);
	RewardStream.Initialize(static_cast<int32>(GetTypeHash(GetFName())));

	Super::BeginPlay();

	const FGrimReactivePropRow* Row = GetRow();
	UWorld* World = GetWorld();
	if (Row && World)
	{
		if (UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
		{
			if (HasAuthority() && Row->ResetRule == EGrimPropReset::Dawn)
			{
				Time->OnNightEnd.AddDynamic(this, &AGrimReactiveProp::HandleDawn);
				bBoundDawn = true;
			}
			if (GetNetMode() != NM_DedicatedServer && !Row->LoopFX.IsNull()
				&& (Row->LoopFXWhen == EGrimPropFXWhen::Day || Row->LoopFXWhen == EGrimPropFXWhen::Night))
			{
				Time->OnNightBegin.AddDynamic(this, &AGrimReactiveProp::HandleNightChanged);
				Time->OnNightEnd.AddDynamic(this, &AGrimReactiveProp::HandleNightChanged);
				bBoundDayNight = true;
			}
		}
		if (!GetJournalId().IsNone())
		{
			if (UGrimInteractableSubsystem* Interactables = World->GetSubsystem<UGrimInteractableSubsystem>())
			{
				Interactables->Register(this);
			}
		}
	}
	// 공통 액터가 몸 재질을 색 파라미터용 동적 인스턴스로 바꿨다 — 팩 메시는 원래 재질로 되돌린다(RefreshPropVisuals 가 표의 재질을 다시 입힌다)
	if (Row)
	{
		BodyMesh->EmptyOverrideMaterials();
	}
	RefreshPropVisuals();
	ScheduleReset();
}

void AGrimReactiveProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResetTimer);
		if (UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
		{
			if (bBoundDawn)
			{
				Time->OnNightEnd.RemoveDynamic(this, &AGrimReactiveProp::HandleDawn);
			}
			if (bBoundDayNight)
			{
				Time->OnNightBegin.RemoveDynamic(this, &AGrimReactiveProp::HandleNightChanged);
				Time->OnNightEnd.RemoveDynamic(this, &AGrimReactiveProp::HandleNightChanged);
			}
		}
		if (UGrimInteractableSubsystem* Interactables = World->GetSubsystem<UGrimInteractableSubsystem>())
		{
			Interactables->Unregister(this);
		}
	}
	ElementalState->OnStateChangedNative.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}

FVector AGrimReactiveProp::GetTargetPoint() const
{
	return HitBox->GetComponentLocation();
}

bool AGrimReactiveProp::CanBeTargetedBy(const AActor* Requester) const
{
	return Super::CanBeTargetedBy(Requester) && !IsCollectedByLocalPlayer();
}

// ───── 상태 → 모습 · 보상 ─────

void AGrimReactiveProp::HandlePropStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity)
{
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row)
	{
		return;
	}
	const bool bAppeared = OldIntensity <= 0.f && NewIntensity > 0.f;
	// 늦게 들어온 클라이언트가 이미 바뀐 상태를 받을 때는 한 번짜리 연출을 틀지 않는다
	const bool bLive = HasActorBegunPlay() && GetGameTimeSinceCreation() > 0.5f;

	if (HasAuthority() && bAppeared)
	{
		if (Row->RewardState.IsValid() && StateTag == Row->RewardState)
		{
			SpawnReward();
		}
		if (Row->DriftState.IsValid() && StateTag == Row->DriftState)
		{
			DriftWithWind();
		}
	}
	if (GetNetMode() != NM_DedicatedServer && bAppeared && bLive)
	{
		if (Row->BurstState.IsValid() && StateTag == Row->BurstState)
		{
			if (UNiagaraSystem* Burst = Row->BurstFX.LoadSynchronous())
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Burst, GetActorTransform().TransformPosition(Row->LoopFXOffset), GetActorRotation());
			}
		}
		if (Row->KickState.IsValid() && StateTag == Row->KickState)
		{
			StartKick();
		}
	}
	if (HasAuthority())
	{
		ScheduleReset();
	}
	RefreshPropVisuals();
}

void AGrimReactiveProp::RefreshPropVisuals()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row)
	{
		return;
	}

	bShowingAlt = Row->AltState.IsValid() && ElementalState->HasState(Row->AltState);
	const bool bCollected = IsCollectedByLocalPlayer();

	// 몸
	UStaticMesh* WantMesh = (bShowingAlt && !Row->AltMesh.IsNull()) ? Row->AltMesh.LoadSynchronous() : Row->Mesh.LoadSynchronous();
	UMaterialInterface* WantMaterial = (bShowingAlt && !Row->AltMaterial.IsNull()) ? Row->AltMaterial.LoadSynchronous() : Row->MeshMaterial.LoadSynchronous();
	if (BodyMesh->GetStaticMesh() != WantMesh)
	{
		BodyMesh->SetStaticMesh(WantMesh);
		BodyMesh->EmptyOverrideMaterials();
	}
	if (WantMaterial && BodyMesh->GetMaterial(0) != WantMaterial)
	{
		BodyMesh->SetMaterial(0, WantMaterial);
	}
	else if (!WantMaterial && BodyMesh->OverrideMaterials.Num() > 0 && WantMesh)
	{
		BodyMesh->EmptyOverrideMaterials();
	}
	const FVector Scale = (bShowingAlt && !Row->AltScale.IsNearlyZero()) ? Row->AltScale : Row->MeshScale;
	const FRotator Rotation = (bShowingAlt && Row->bAltUsesRotation) ? Row->AltRotation : Row->MeshRotation;
	BodyBaseTransform = FTransform(Rotation, Row->MeshOffset, Scale);
	if (KickElapsed < 0.f)
	{
		BodyMesh->SetRelativeTransform(BodyBaseTransform);
	}
	BodyMesh->SetHiddenInGame(!WantMesh || bCollected || (bShowingAlt && Row->bAltHidesBody));
	BodyMesh->SetVisibility(WantMesh != nullptr);

	// 덧붙은 메시
	bool bAccent = AccentMesh->GetStaticMesh() != nullptr && !bCollected;
	if (Row->AccentShow == EGrimPropAccentShow::Default)
	{
		bAccent &= !bShowingAlt;
	}
	else if (Row->AccentShow == EGrimPropAccentShow::Alt)
	{
		bAccent &= bShowingAlt;
	}
	AccentMesh->SetHiddenInGame(!bAccent);

	// 고리 이펙트
	bool bFX = LoopFX->GetAsset() != nullptr && !bCollected;
	switch (Row->LoopFXWhen)
	{
	case EGrimPropFXWhen::Day: bFX &= !IsNightNow(); break;
	case EGrimPropFXWhen::Night: bFX &= IsNightNow(); break;
	case EGrimPropFXWhen::Alt: bFX &= bShowingAlt; break;
	default: break;
	}
	if (Row->LoopFXHideState.IsValid() && ElementalState->HasState(Row->LoopFXHideState))
	{
		bFX = false;
	}
	if (HasActorBegunPlay())
	{
		if (bFX && !LoopFX->IsActive())
		{
			LoopFX->Activate(true);
		}
		else if (!bFX && LoopFX->IsActive())
		{
			LoopFX->Deactivate();
		}
	}
	SetActorEnableCollision(!bCollected || HasAuthority());
}

void AGrimReactiveProp::HandleNightChanged()
{
	RefreshPropVisuals();
}

bool AGrimReactiveProp::IsNightNow() const
{
	const UWorld* World = GetWorld();
	const UGrimTimeOfDaySubsystem* Time = World ? World->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr;
	return Time && Time->IsNight();
}

void AGrimReactiveProp::SpawnReward()
{
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row || Row->Rewards.IsEmpty())
	{
		return;
	}
	const TArray<FGrimRewardEntry> Entries = FGrimDressingData::ParseRewards(Row->Rewards);
	const FGrimRewardEntry Picked = FGrimDressingData::PickReward(Entries, Row->NothingWeight, RewardStream.FRand());
	if (Picked.Item.IsNone())
	{
		UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s(%s): 빈손"), *GetName(), *PropId.ToString());
		return;
	}
	const int32 Count = RewardStream.RandRange(Picked.Min, Picked.Max);
	const FVector Location = GetActorTransform().TransformPosition(Row->RewardOffset);
	if (UGrimItemStatics::SpawnPickup(this, Picked.Item, Count, Location))
	{
		++RewardsSpawned;
		UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s(%s): 보상 %s ×%d"), *GetName(), *PropId.ToString(), *Picked.Item.ToString(), Count);
	}
}

void AGrimReactiveProp::DriftWithWind()
{
	const FGrimReactivePropRow* Row = GetRow();
	UWorld* World = GetWorld();
	if (!Row || !World)
	{
		return;
	}
	// 바람을 쏜 쪽을 모른다 — 가장 가까운 플레이어에게서 멀어지는 쪽(대개 시전자), 없으면 아무 쪽
	FVector Dir = FVector::ZeroVector;
	float Best = 3000.f * 3000.f;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		if (It->IsPlayerControlled())
		{
			const FVector Delta = GetActorLocation() - It->GetActorLocation();
			if (Delta.SizeSquared2D() < Best)
			{
				Best = Delta.SizeSquared2D();
				Dir = Delta.GetSafeNormal2D();
			}
		}
	}
	if (Dir.IsNearlyZero())
	{
		Dir = FRotator(0.f, RewardStream.FRandRange(0.f, 360.f), 0.f).Vector();
	}
	Dir = Dir.RotateAngleAxis(RewardStream.FRandRange(-25.f, 25.f), FVector::UpVector);
	const float Distance = RewardStream.FRandRange(Row->DriftDistance.X, FMath::Max(Row->DriftDistance.X, Row->DriftDistance.Y));
	FVector Target = GetActorLocation() + Dir * Distance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimPropDrift), false, this);
	const float Depth = GetDefault<UGrimDressingSettings>()->DriftGroundTraceDepth;
	if (World->LineTraceSingleByChannel(Hit, Target + FVector(0.f, 0.f, Depth * 0.5f), Target - FVector(0.f, 0.f, Depth), ECC_Visibility, Params))
	{
		Target = Hit.ImpactPoint;
	}
	FlushNetDormancy();
	SetActorLocationAndRotation(Target, FRotator(0.f, RewardStream.FRandRange(0.f, 360.f), 0.f));
	bDrifted = true;
	UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s(%s): 바람에 %.0f cm 날려 감"), *GetName(), *PropId.ToString(), Distance);
}

void AGrimReactiveProp::StartKick()
{
	KickElapsed = 0.f;
	SetActorTickEnabled(true);
}

void AGrimReactiveProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row || KickElapsed < 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	KickElapsed += DeltaSeconds;
	const float T = FMath::Clamp(KickElapsed / Row->KickSeconds, 0.f, 1.f);
	// 튀어 올랐다(포물선) 출렁이며 제자리로
	const float Up = Row->KickHeight * 4.f * T * (1.f - T);
	const float Wobble = FMath::Sin(T * PI * 6.f) * (1.f - T) * 8.f;
	FTransform Kicked = BodyBaseTransform;
	Kicked.AddToTranslation(FVector(Wobble, 0.f, Up));
	Kicked.ConcatenateRotation(FRotator(0.f, T * 540.f, 0.f).Quaternion());
	BodyMesh->SetRelativeTransform(Kicked);
	if (T >= 1.f)
	{
		KickElapsed = -1.f;
		BodyMesh->SetRelativeTransform(BodyBaseTransform);
		SetActorTickEnabled(false);
	}
}

// ───── 되돌리기 ─────

bool AGrimReactiveProp::IsAtInitialState() const
{
	const FGrimReactivePropRow* Row = GetRow();
	const bool bWantInitial = Row && Row->InitialState.IsValid() && Row->InitialIntensity > 0.f;
	int32 Live = 0;
	for (const FGrimElementalState& Item : ElementalState->GetStates().Items)
	{
		if (Item.Intensity <= 0.f)
		{
			continue;
		}
		++Live;
		if (!bWantInitial || Item.StateTag != Row->InitialState)
		{
			return false;
		}
	}
	return bWantInitial ? Live == 1 : Live == 0;
}

void AGrimReactiveProp::ResetToInitial()
{
	if (!HasAuthority())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(ResetTimer);
	ElementalState->ResetStates(ElementalState->InitialStates);
	UE_LOG(LogGrimDressing, Verbose, TEXT("[Server] %s(%s): 처음 상태로"), *GetName(), *PropId.ToString());
}

void AGrimReactiveProp::HandleDawn()
{
	const FGrimReactivePropRow* Row = GetRow();
	if (HasAuthority() && Row && Row->ResetRule == EGrimPropReset::Dawn && !IsAtInitialState())
	{
		ResetToInitial();
	}
}

void AGrimReactiveProp::ScheduleReset()
{
	const FGrimReactivePropRow* Row = GetRow();
	UWorld* World = GetWorld();
	if (!Row || !World || !HasAuthority() || Row->ResetRule != EGrimPropReset::Seconds || Row->ResetSeconds <= 0.f)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (IsAtInitialState())
	{
		Timers.ClearTimer(ResetTimer);
	}
	else if (!Timers.IsTimerActive(ResetTimer))
	{
		Timers.SetTimer(ResetTimer, this, &AGrimReactiveProp::ResetToInitial, Row->ResetSeconds, false);
	}
}

// ───── 줍기(수집품) ─────

bool AGrimReactiveProp::IsCollectedByLocalPlayer() const
{
	if (CollectedBy.Num() == 0)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController() && CollectedBy.Contains(FGrimPlayerIdentity::GetLocalPlayerId(PC->GetLocalPlayer())))
		{
			return true;
		}
	}
	return false;
}

bool AGrimReactiveProp::GetInteractionOffer(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const
{
	const FGrimReactivePropRow* Row = GetRow();
	if (!Row || GetJournalId().IsNone() || !Pawn)
	{
		return false;
	}
	const float Distance = FVector::Dist(Pawn->GetActorLocation(), GetActorLocation());
	if (Distance > GetDefault<UGrimDressingSettings>()->CollectRadius)
	{
		return false;
	}
	const FString Key = GrimReactivePropLocal::KeyForPawn(Pawn);
	if (Key.IsEmpty() || CollectedBy.Contains(Key))
	{
		return false;
	}
	OutOffer.Distance = Distance;
	OutOffer.Prompt = FText::Format(LOCTEXT("Collect", "줍기 — {0}"), FText::FromString(Row->DisplayName.IsEmpty() ? PropId.ToString() : Row->DisplayName));
	OutOffer.Token = 0;
	return true;
}

bool AGrimReactiveProp::Interact(APawn* Pawn, int32 Token)
{
	const FGrimReactivePropRow* Row = GetRow();
	if (!HasAuthority() || !Row || GetJournalId().IsNone() || !Pawn)
	{
		return false;
	}
	const UGrimDressingSettings* Settings = GetDefault<UGrimDressingSettings>();
	if (FVector::Dist(Pawn->GetActorLocation(), GetActorLocation()) > Settings->CollectRadius + Settings->ServerReachSlack)
	{
		UE_LOG(LogGrimDressing, Warning, TEXT("[Server] %s: %s 이(가) 너무 멀다"), *GetName(), *GetNameSafe(Pawn));
		return false;
	}
	const FString Key = FGrimPlayerIdentity::ResolveServerSide(Pawn->GetPlayerState());
	if (Key.IsEmpty() || CollectedBy.Contains(Key))
	{
		return false;
	}
	FlushNetDormancy();
	CollectedBy.Add(Key);
	if (UGrimJournalComponent* Journal = UGrimJournalComponent::Find(Pawn))
	{
		Journal->AddEntry(GetJournalId());
	}
	UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s(%s): %s 가 주움(%d명)"), *GetName(), *PropId.ToString(), *Key, CollectedBy.Num());
	RefreshPropVisuals();
	return true;
}

void AGrimReactiveProp::OnRep_PropId()
{
	ApplyDefinition();
	RefreshPropVisuals();
}

void AGrimReactiveProp::OnRep_CollectedBy()
{
	RefreshPropVisuals();
}

// ───── 세이브 ─────

FName AGrimReactiveProp::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Dressing.Prop.%s"), *GetName()));
}

void AGrimReactiveProp::WriteSaveData(FArchive& Ar)
{
	FTransform Where = GetActorTransform();
	Ar << CollectedBy;
	Ar << bDrifted;
	Ar << Where;
}

void AGrimReactiveProp::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FString> Collected;
	bool bWasDrifted = false;
	FTransform Where;
	Ar << Collected;
	Ar << bWasDrifted;
	Ar << Where;
	FlushNetDormancy();
	CollectedBy = MoveTemp(Collected);
	bDrifted = bWasDrifted;
	if (bDrifted)
	{
		SetActorTransform(Where);
	}
	RefreshPropVisuals();
}

#undef LOCTEXT_NAMESPACE
