// 지형 디버그 콘솔 명령 — 헤드리스 -game 으로 L_OpenBeta 를 열어 지형 높이 · 걷기 · 시간 · 날씨를 로그로 본다
// (Scripts/Terrain/check_openbeta.ps1). 지연(초)을 받는 건 -ExecCmds 가 맵 로드보다 먼저 실행되기 때문이다.

#include "World/GrimTerrainMarker.h"
#include "World/GrimTerrainTypes.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "LandscapeStreamingProxy.h"

#if !UE_BUILD_SHIPPING

namespace GrimTerrainDebugLocal
{
	float ParseFloatArg(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	void RunInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		auto RunNow = [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					Fn(World);
				}
			}
		};
		if (Delay <= 0.f)
		{
			RunNow();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RunNow](float)
		{
			RunNow();
			return false;
		}), Delay);
	}

	FString MarkerName(const AActor* Actor)
	{
#if WITH_EDITOR
		return Actor->GetActorLabel(false);
#else
		return Actor->GetName();
#endif
	}

	APawn* GetLocalPawn(UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController() && PC->GetPawn())
			{
				return PC->GetPawn();
			}
		}
		return nullptr;
	}

	/** 그 XY 의 StartZ(기본: 하늘)에서 아래로 쏴 맞은 땅 높이(cm). 못 맞으면 false */
	bool TraceGround(UWorld* World, const FVector2D& XY, float& OutZ, FString& OutHitName, const AActor* Ignore = nullptr, double StartZ = 100000.0)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimTerrainProbe), true, Ignore);
		const FVector Start(XY.X, XY.Y, StartZ);
		const FVector End(XY.X, XY.Y, -100000.0);
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			OutZ = Hit.ImpactPoint.Z;
			OutHitName = Hit.GetActor() ? Hit.GetActor()->GetClass()->GetName() : TEXT("?");
			return true;
		}
		return false;
	}

	/** Grim.Terrain.Probe [지연초] — 플레이어 · 표식 자리의 땅 높이, 랜드스케이프 프록시 수, 시간, 날씨 */
	void ProbeCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ParseFloatArg(Args, 0, 0.f), [](UWorld* World)
		{
			int32 Landscapes = 0, Proxies = 0;
			FBox RenderBounds(ForceInit);
			for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
			{
				Cast<ALandscapeStreamingProxy>(*It) ? ++Proxies : ++Landscapes;
				for (const ULandscapeComponent* Component : It->LandscapeComponents)
				{
					if (Component)
					{
						RenderBounds += Component->Bounds.GetBox();
					}
				}
			}
			// 그림(렌더) 높이 범위 — 높이 지도 텍스처가 비면(-nullrhi 로 만든 랜드스케이프) 해수면 아래 -256 m 근처로 나온다
			UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 맵 %s: 랜드스케이프 %d · 스트리밍 프록시 %d, 그림 높이 %.0f ~ %.0f cm"), *World->GetMapName(), Landscapes, Proxies,
				RenderBounds.IsValid ? RenderBounds.Min.Z : 0.0, RenderBounds.IsValid ? RenderBounds.Max.Z : 0.0);

			if (APawn* Pawn = GetLocalPawn(World))
			{
				const FVector Location = Pawn->GetActorLocation();
				float GroundZ = 0.f;
				FString HitName;
				const bool bHit = TraceGround(World, FVector2D(Location), GroundZ, HitName, Pawn, Location.Z);
				const ACharacter* Character = Cast<ACharacter>(Pawn);
				const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
				const FString Mode = Movement ? StaticEnum<EMovementMode>()->GetNameStringByValue(Movement->MovementMode) : TEXT("?");
				const APhysicsVolume* Volume = Pawn->GetPhysicsVolume();
				UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 플레이어 (%.0f, %.0f, %.0f) 아래 땅 %s %.0f (%s), 이동 %s, 물 볼륨 %s"),
					Location.X, Location.Y, Location.Z, bHit ? TEXT("") : TEXT("없음"), GroundZ, *HitName, *Mode,
					(Volume && Volume->bWaterVolume) ? *MarkerName(Volume) : TEXT("없음"));
			}
			else
			{
				UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 플레이어 폰 없음"));
			}

			for (AGrimTerrainMarker* Marker : AGrimTerrainMarker::FindMarkers(World, FGameplayTag()))
			{
				const FVector Location = Marker->GetActorLocation();
				float GroundZ = 0.f;
				FString HitName;
				if (TraceGround(World, FVector2D(Location), GroundZ, HitName, Marker, Location.Z + 100.0))
				{
					UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 표식 %s %s (%.0f, %.0f): 땅 %.0f cm (%s), 표식 %.0f cm"),
						*MarkerName(Marker), *Marker->MarkerTag.ToString(), Location.X, Location.Y, GroundZ, *HitName, Location.Z);
				}
				else
				{
					UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 표식 %s %s (%.0f, %.0f): 땅 없음(아직 로드 안 됨?)"),
						*MarkerName(Marker), *Marker->MarkerTag.ToString(), Location.X, Location.Y);
				}
			}

			if (const UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
			{
				UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 시간: %d일 %.2f시"), Time->GetDayIndex(), Time->GetHours());
			}
			if (const UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>())
			{
				const APawn* Pawn = GetLocalPawn(World);
				const FVector At = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
				UE_LOG(LogGrimTerrain, Display, TEXT("[Probe] 날씨: 지역 %s, %s, 기온 %.1f"), *Weather->GetRegionAt(At).ToString(),
					*StaticEnum<EGrimWeatherType>()->GetNameStringByValue(static_cast<int64>(Weather->GetWeatherAt(At))), Weather->GetAmbientTemperature(At));
			}
		});
	}

	/** Grim.Terrain.Walk <지연초> <걷는초> [방향 Yaw] — 로컬 플레이어를 그 방향으로 걷게 하고 처음 · 끝 위치를 남긴다 */
	void WalkCommand(const TArray<FString>& Args)
	{
		const float Duration = ParseFloatArg(Args, 1, 5.f);
		const float Yaw = ParseFloatArg(Args, 2, 0.f);
		RunInGameWorlds(ParseFloatArg(Args, 0, 0.f), [Duration, Yaw](UWorld* World)
		{
			APawn* Pawn = GetLocalPawn(World);
			if (!Pawn)
			{
				UE_LOG(LogGrimTerrain, Display, TEXT("[Walk] 플레이어 폰 없음"));
				return;
			}
			const FVector Start = Pawn->GetActorLocation();
			const FVector Direction = FRotator(0.f, Yaw, 0.f).Vector();
			UE_LOG(LogGrimTerrain, Display, TEXT("[Walk] 시작 (%.0f, %.0f, %.0f) 방향 %.0f도 %.1f초"), Start.X, Start.Y, Start.Z, Yaw, Duration);
			TWeakObjectPtr<APawn> WeakPawn(Pawn);
			TSharedRef<double> Elapsed = MakeShared<double>(0.0);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakPawn, Direction, Duration, Start, Elapsed](float DeltaTime)
			{
				APawn* Walker = WeakPawn.Get();
				if (!Walker)
				{
					return false;
				}
				*Elapsed += DeltaTime;
				if (*Elapsed < Duration)
				{
					Walker->AddMovementInput(Direction, 1.f);
					return true;
				}
				const FVector End = Walker->GetActorLocation();
				const ACharacter* Character = Cast<ACharacter>(Walker);
				const bool bFalling = Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
				UE_LOG(LogGrimTerrain, Display, TEXT("[Walk] 끝 (%.0f, %.0f, %.0f): 수평 %.0f cm, 높이 변화 %.0f cm, 떨어지는 중 %s"),
					End.X, End.Y, End.Z, FVector::Dist2D(Start, End), End.Z - Start.Z, bFalling ? TEXT("예") : TEXT("아니오"));
				return false;
			}));
		});
	}

	/** Grim.Terrain.Teleport <X> <Y> [지연초] — 그 XY 의 땅 위(2 m)로 옮긴다 */
	void TeleportCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimTerrain, Warning, TEXT("사용: Grim.Terrain.Teleport <X> <Y> [지연초]"));
			return;
		}
		const FVector2D XY(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]));
		RunInGameWorlds(ParseFloatArg(Args, 2, 0.f), [XY](UWorld* World)
		{
			APawn* Pawn = GetLocalPawn(World);
			float GroundZ = 0.f;
			FString HitName;
			if (Pawn && TraceGround(World, XY, GroundZ, HitName, Pawn))
			{
				Pawn->TeleportTo(FVector(XY.X, XY.Y, GroundZ + 200.f), Pawn->GetActorRotation());
				UE_LOG(LogGrimTerrain, Display, TEXT("[Teleport] (%.0f, %.0f) 땅 %.0f (%s)"), XY.X, XY.Y, GroundZ, *HitName);
			}
			else
			{
				UE_LOG(LogGrimTerrain, Display, TEXT("[Teleport] (%.0f, %.0f): 폰 또는 땅 없음"), XY.X, XY.Y);
			}
		});
	}

	/** Grim.Terrain.ViewFrom <X> <Y> <Z> <Yaw> <Pitch> [지연초] — 그 자리의 카메라로 화면을 돌린다(사람이 볼 스크린숏용) */
	void ViewFromCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 5)
		{
			UE_LOG(LogGrimTerrain, Warning, TEXT("사용: Grim.Terrain.ViewFrom <X> <Y> <Z> <Yaw> <Pitch> [지연초] [스크린숏 이름]"));
			return;
		}
		const FVector Location(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		const FRotator Rotation(FCString::Atof(*Args[4]), FCString::Atof(*Args[3]), 0.f);
		const FString ShotName = Args.IsValidIndex(6) ? Args[6] : FString();
		RunInGameWorlds(ParseFloatArg(Args, 5, 0.f), [Location, Rotation, ShotName](UWorld* World)
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (PC && PC->IsLocalController())
				{
					ACameraActor* Camera = World->SpawnActor<ACameraActor>(Location, Rotation);
					if (Camera)
					{
						Camera->GetCameraComponent()->SetFieldOfView(80.f);
						PC->SetViewTarget(Camera);
						UE_LOG(LogGrimTerrain, Display, TEXT("[ViewFrom] (%.0f, %.0f, %.0f) yaw %.0f pitch %.0f"), Location.X, Location.Y, Location.Z, Rotation.Yaw, Rotation.Pitch);
						if (!ShotName.IsEmpty())
						{
							// 스트리밍 · 풀이 따라오게 잠깐 기다렸다 찍는다
							const FString File = FPaths::ProjectSavedDir() / TEXT("TerrainCheck") / (ShotName + TEXT(".png"));
							FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([File](float)
							{
								FScreenshotRequest::RequestScreenshot(File, false, false);
								UE_LOG(LogGrimTerrain, Display, TEXT("[ViewFrom] 스크린숏 %s"), *File);
								return false;
							}), 4.f);
						}
					}
					return;
				}
			}
		});
	}

	FAutoConsoleCommand GViewFromCommand(
		TEXT("Grim.Terrain.ViewFrom"),
		TEXT("그 자리 · 방향의 카메라로 화면을 돌리고, 이름을 주면 4초 뒤 Saved/TerrainCheck/<이름>.png 로 찍는다. <X> <Y> <Z> <Yaw> <Pitch> [지연초] [스크린숏 이름]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ViewFromCommand));

	FAutoConsoleCommand GProbeCommand(
		TEXT("Grim.Terrain.Probe"),
		TEXT("플레이어 · 지형 표식 자리의 땅 높이, 랜드스케이프 프록시 수, 시간, 날씨를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ProbeCommand));

	FAutoConsoleCommand GWalkCommand(
		TEXT("Grim.Terrain.Walk"),
		TEXT("로컬 플레이어를 걷게 하고 처음 · 끝 위치를 로그로. <지연초> <걷는초> [방향 Yaw(기본 0 = +X 북쪽, 90 = +Y 동쪽)]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&WalkCommand));

	FAutoConsoleCommand GTeleportCommand(
		TEXT("Grim.Terrain.Teleport"),
		TEXT("로컬 플레이어를 그 XY 의 땅 위로 옮긴다. <X> <Y> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&TeleportCommand));
}

#endif
