#include "Quest/GrimTutorialProps.h"
#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTypes.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AGrimCropPlot::AGrimCropPlot()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
	}
	// 흙 두둑 60 × 60 × 20cm
	BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.2f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	BodyColor = FLinearColor(0.32f, 0.2f, 0.1f);
	WetColor = FLinearColor(0.12f, 0.08f, 0.05f);
	WetIntensityForFullColor = 1.f;

	Sprout = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sprout"));
	Sprout->SetupAttachment(Root);
	if (ConeMesh.Succeeded())
	{
		Sprout->SetStaticMesh(ConeMesh.Object);
	}
	Sprout->SetRelativeLocation(FVector(0.f, 0.f, 32.f));
	Sprout->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.25f));
	Sprout->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

AGrimQuestTriggerVolume::AGrimQuestTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(800.f, 800.f, 400.f));
	Bounds->SetCollisionProfileName(TEXT("Trigger"));
	Bounds->SetGenerateOverlapEvents(true);
	Bounds->SetCanEverAffectNavigation(false);
	SetRootComponent(Bounds);
	SetActorHiddenInGame(true);
}

void AGrimQuestTriggerVolume::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Bounds->OnComponentBeginOverlap.AddDynamic(this, &AGrimQuestTriggerVolume::HandleBeginOverlap);
	}
}

void AGrimQuestTriggerVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	UGrimQuestComponent* Quest = Pawn ? UGrimQuestComponent::FindForActor(Pawn) : nullptr;
	if (!Quest)
	{
		return;
	}
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 가 발동 볼륨 %s 에 들어왔다"), *Pawn->GetName(), *GetName());
	if (!QuestToStart.IsNone() && FGrimQuestModel::CanStart(UGrimQuestSettings::GetQuestDatabase(), Quest->GetLog(), QuestToStart) == EGrimQuestStartResult::Started)
	{
		Quest->StartQuest(QuestToStart);
	}
	if (!EventOnEnter.IsNone())
	{
		Quest->NotifyEvent(EventOnEnter);
	}
}
