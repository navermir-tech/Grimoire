// Grim.Cam <X> <Y> <Z> <피치> <요> [FOV] — 그 자리에 고정 카메라를 세워 화면을 그쪽으로 돌린다(개발 빌드).
// 인자 없이 치면 캐릭터 카메라로 돌아온다. 캐릭터는 계속 조종 · 시전할 수 있어서, 이펙트를 옆에서 찍을 때 쓴다
// (Scripts/MagicFX/capture_bolt.ps1). 디버그 카메라(ToggleDebugCamera)는 BugItGo 로 위치가 옮겨지지 않는다.
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommandWithWorldAndArgs GGrimCamCommand(
	TEXT("Grim.Cam"),
	TEXT("<X> <Y> <Z> <피치> <요> [FOV] — 고정 카메라로 본다. 인자 없으면 캐릭터 카메라로."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PlayerController = World ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		if (!PlayerController)
		{
			return;
		}
		static TWeakObjectPtr<ACameraActor> Camera;
		if (Args.Num() < 5)
		{
			PlayerController->SetViewTarget(PlayerController->GetPawn());
			if (Camera.IsValid())
			{
				Camera->Destroy();
			}
			return;
		}
		const FVector Location(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]), FCString::Atod(*Args[2]));
		const FRotator Rotation(FCString::Atod(*Args[3]), FCString::Atod(*Args[4]), 0.0);
		if (!Camera.IsValid() || Camera->GetWorld() != World)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Camera = World->SpawnActor<ACameraActor>(Location, Rotation, Params);
		}
		if (!Camera.IsValid())
		{
			return;
		}
		Camera->SetActorLocationAndRotation(Location, Rotation);
		UCameraComponent* Component = Camera->GetCameraComponent();
		Component->SetConstraintAspectRatio(false);
		if (Args.Num() >= 6)
		{
			Component->SetFieldOfView(FCString::Atof(*Args[5]));
		}
		PlayerController->SetViewTarget(Camera.Get());
		UE_LOG(LogTemp, Display, TEXT("[Grim.Cam] %s %s"), *Location.ToString(), *Rotation.ToString());
	}));

#endif
