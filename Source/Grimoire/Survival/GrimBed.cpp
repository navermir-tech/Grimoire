#include "Survival/GrimBed.h"
#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Core/GrimPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGrimBed::AGrimBed()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// 서버가 RPC 인자로 받으려면 네트워크로 가리킬 수 있어야 한다(배치 액터는 이름으로, 스폰한 것은 복제로).
	bReplicates = true;
	SetNetUpdateFrequency(1.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mattress = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mattress"));
	Mattress->SetupAttachment(Root);
	Pillow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillow"));
	Pillow->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		Mattress->SetStaticMesh(CubeMesh.Object);
		Pillow->SetStaticMesh(CubeMesh.Object);
	}
	Pillow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyShape();
}

void AGrimBed::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimBed, bBedroll);
}

void AGrimBed::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShape();
}

void AGrimBed::ApplyShape()
{
	// 큐브는 100cm. 침대 200×100×40, 침낭 190×80×8.
	if (bBedroll)
	{
		Mattress->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
		Mattress->SetRelativeScale3D(FVector(1.9f, 0.8f, 0.08f));
		Pillow->SetVisibility(false);
	}
	else
	{
		Mattress->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
		Mattress->SetRelativeScale3D(FVector(2.f, 1.f, 0.4f));
		Pillow->SetRelativeLocation(FVector(75.f, 0.f, 47.f));
		Pillow->SetRelativeScale3D(FVector(0.35f, 0.7f, 0.14f));
		Pillow->SetVisibility(true);
	}
}

void AGrimBed::BeginPlay()
{
	Super::BeginPlay();
	// 키 입력 · 안내는 화면이 있는 쪽에서만.
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
}

FTransform AGrimBed::GetRespawnTransform() const
{
	const FTransform& Transform = GetActorTransform();
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), Transform.TransformPosition(RespawnOffset));
}

bool AGrimBed::IsWithinReach(const APawn* Pawn, float Slack) const
{
	if (!Pawn)
	{
		return false;
	}
	const float Radius = GetDefault<UGrimSurvivalSettings>()->Sleep.BedInteractRadius + FMath::Max(0.f, Slack);
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

void AGrimBed::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController())
		{
			HandleLocalPlayer(PlayerController);
		}
	}
}

void AGrimBed::HandleLocalPlayer(APlayerController* PlayerController)
{
	APawn* Pawn = PlayerController->GetPawn();
	if (!IsWithinReach(Pawn))
	{
		return;
	}
	const AGrimPlayerState* PlayerState = PlayerController->GetPlayerState<AGrimPlayerState>();
	UGrimSurvivalComponent* Survival = PlayerState ? PlayerState->GetSurvival() : nullptr;
	if (!Survival)
	{
		return;
	}

	const FKey& Key = GetDefault<UGrimSurvivalSettings>()->Sleep.BedInteractKey;
	const bool bSleeping = Survival->IsSleeping();

#if ENABLE_DRAW_DEBUG
	// 디버그 글꼴에 한글 글리프가 없어 영어. 정식 안내는 UI 몫.
	const FString Prompt = FString::Printf(TEXT("[%s] %s"), *Key.GetDisplayName().ToString(), bSleeping ? TEXT("Get up") : TEXT("Sleep"));
	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 90.f), Prompt, nullptr, FColor::White, 0.f, true);
#endif

	if (PlayerController->WasInputKeyJustPressed(Key))
	{
		if (bSleeping)
		{
			Survival->ServerRequestWake();
		}
		else
		{
			Survival->ServerRequestSleep(this);
		}
	}
}

// ── 세이브 (Bed.<이름> v1) ──

FName AGrimBed::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Bed.%s"), *GetName()));
}

void AGrimBed::WriteSaveData(FArchive& Ar)
{
	bool bSavedBedroll = bBedroll;
	Ar << bSavedBedroll;
}

void AGrimBed::ReadSaveData(FArchive& Ar, int32 Version)
{
	bool bSavedBedroll = false;
	Ar << bSavedBedroll;
	if (HasAuthority() && !Ar.IsError() && bSavedBedroll != bBedroll)
	{
		// 다시 스폰할 때는 FinishSpawning 전에 불린다 — 모양은 OnConstruction 이 다시 맞춘다.
		bBedroll = bSavedBedroll;
		ApplyShape();
	}
}
