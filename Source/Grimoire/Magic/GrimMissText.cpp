#include "Magic/GrimMissText.h"
#include "Magic/GrimMagicSettings.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AGrimMissText::AGrimMissText()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	RootComponent = Text;
}

AGrimMissText* AGrimMissText::Spawn(UWorld* World, const FVector& Location)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	return World->SpawnActor<AGrimMissText>(AGrimMissText::StaticClass(), Location, FRotator::ZeroRotator, Params);
}

void AGrimMissText::BeginPlay()
{
	Super::BeginPlay();
	const FGrimMissTextTuning& Tuning = UGrimMagicSettings::Get()->MissText;
	if (UMaterialInterface* Material = Tuning.Material.LoadSynchronous())
	{
		Text->SetTextMaterial(Material);
	}
	Text->SetText(Tuning.Text);
	Text->SetTextRenderColor(Tuning.Color.ToFColor(true));
	Text->SetWorldSize(Tuning.WorldSize);
	Lifetime = FMath::Max(Tuning.Lifetime, 0.1f);
	RiseSpeed = Tuning.RiseSpeed;
	SetLifeSpan(Lifetime);
}

void AGrimMissText::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaSeconds));

	// 카메라를 본다(글 앞면 = +X). 끝 30 % 동안 줄어들며 사라진다(글 머티리얼은 투명도를 받지 않는다).
	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0); PC && PC->PlayerCameraManager)
	{
		const FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
	}
	const float Fade = FMath::Clamp((Lifetime - Age) / (Lifetime * 0.3f), 0.f, 1.f);
	SetActorScale3D(FVector(FMath::Max(Fade, 0.05f)));
}
