// 성능 측정 콘솔 명령(개발 빌드) — docs/systems/performance.md 「측정」, Scripts/Perf/measure.ps1 이 -ExecCmds 로 부른다.
//   Grim.Perf.Goto <X> <Y> <Yaw> [Pitch] [지연초]  로컬 폰을 그 XY 땅 위로 옮기고 그 방향을 보게 한다(뷰는 폰 카메라 — 실제 플레이 화면)
//   Grim.Perf.Sample <이름> <초> [지연초]          그동안의 프레임 · 게임 · 렌더 · RHI · GPU 시간, 드로 콜, 메모리를 한 줄(GrimPerf|)로 로그
//   Grim.Perf.Net <이름> <초> [지연초]             그동안의 연결별 주고받은 바이트 · 패킷(초당), 복제 객체 수를 한 줄(GrimPerfNet|)로 로그
// 줄 형식은 「키=값」 을 | 로 이은 것 — 요약 스크립트(Scripts/Perf/summarize.ps1)가 그대로 표로 옮긴다.
#include "Creatures/GrimCreature.h"
#include "World/GrimDevLabel.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "Landscape.h"
#include "Misc/App.h"
#include "Engine/NetworkObjectList.h"
#include "RenderTimer.h"
#include "RHIStats.h"
#include "DynamicRHI.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGrimPerf, Log, All);

namespace GrimPerfLocal
{
	float ParseFloatArg(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	UWorld* FindGameWorld()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && World->IsGameWorld())
			{
				return World;
			}
		}
		return nullptr;
	}

	void RunLater(float Delay, TFunction<void()> Fn)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Fn](float)
		{
			Fn();
			return false;
		}), FMath::Max(0.f, Delay));
	}

	double Average(const TArray<float>& Values)
	{
		double Sum = 0.0;
		for (float Value : Values)
		{
			Sum += Value;
		}
		return Values.Num() > 0 ? Sum / Values.Num() : 0.0;
	}

	float Percentile(TArray<float> Values, float Fraction)
	{
		if (Values.IsEmpty())
		{
			return 0.f;
		}
		Values.Sort();
		return Values[FMath::Clamp(FMath::FloorToInt32(Fraction * (Values.Num() - 1)), 0, Values.Num() - 1)];
	}

	/** 월드 틱(액터 틱까지) 시간 — 헤드리스(-nullrhi) 서버에서도 잡히는 게임 스레드 값 */
	double GWorldTickStart = 0.0;
	float GLastWorldTickMs = 0.f;
	FDelegateHandle GTickStartHandle;
	FDelegateHandle GTickEndHandle;

	void EnsureWorldTickHooks()
	{
		if (GTickStartHandle.IsValid())
		{
			return;
		}
		GTickStartHandle = FWorldDelegates::OnWorldTickStart.AddLambda([](UWorld* World, ELevelTick, float)
		{
			if (World && World->IsGameWorld())
			{
				GWorldTickStart = FPlatformTime::Seconds();
			}
		});
		GTickEndHandle = FWorldDelegates::OnWorldPostActorTick.AddLambda([](UWorld* World, ELevelTick, float)
		{
			if (World && World->IsGameWorld() && GWorldTickStart > 0.0)
			{
				GLastWorldTickMs = static_cast<float>((FPlatformTime::Seconds() - GWorldTickStart) * 1000.0);
			}
		});
	}

	struct FFrameCapture
	{
		FString Name;
		double EndTime = 0.0;
		TArray<float> Frame, Game, Render, Rhi, Gpu, WorldTick;
		int64 Draws = 0;
		int64 Prims = 0;
	};

	void FinishFrameCapture(const FFrameCapture& Capture)
	{
		UWorld* World = FindGameWorld();
		int32 Actors = 0;
		int32 Creatures = 0;
		int32 Labels = 0;
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				++Actors;
				Creatures += It->IsA<AGrimCreature>() ? 1 : 0;
				Labels += (It->IsA<AGrimDevLabel>() && !It->IsHidden()) ? 1 : 0;
			}
		}
		const int32 N = FMath::Max(1, Capture.Frame.Num());
		const float Avg = static_cast<float>(Average(Capture.Frame));
		int32 Hitches = 0;
		for (float Ms : Capture.Frame)
		{
			Hitches += Ms > 33.4f ? 1 : 0;
		}
		const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
		const FString NetMode = !World ? TEXT("None") : World->GetNetMode() == NM_ListenServer ? TEXT("ListenServer") : World->GetNetMode() == NM_Client ? TEXT("Client") : World->GetNetMode() == NM_DedicatedServer ? TEXT("Dedicated") : TEXT("Standalone");
		UE_LOG(LogGrimPerf, Display,
			TEXT("GrimPerf|name=%s|mode=%s|frames=%d|fps=%.1f|frame_ms=%.2f|p95_ms=%.2f|max_ms=%.2f|hitches=%d|game_ms=%.2f|render_ms=%.2f|rhi_ms=%.2f|gpu_ms=%.2f|world_ms=%.2f|draws=%lld|prims=%lld|mem_mb=%.0f|actors=%d|creatures=%d|labels=%d"),
			*Capture.Name, *NetMode, Capture.Frame.Num(), Avg > 0.f ? 1000.f / Avg : 0.f, Avg, Percentile(Capture.Frame, 0.95f), Percentile(Capture.Frame, 1.f), Hitches,
			Average(Capture.Game), Average(Capture.Render), Average(Capture.Rhi), Average(Capture.Gpu), Average(Capture.WorldTick),
			Capture.Draws / N, Capture.Prims / N, Memory.UsedPhysical / (1024.0 * 1024.0), Actors, Creatures, Labels);
	}

	void SampleCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimPerf, Warning, TEXT("사용: Grim.Perf.Sample <이름> <초> [지연초]"));
			return;
		}
		const FString Name = Args[0];
		const float Seconds = FMath::Max(0.5f, FCString::Atof(*Args[1]));
		EnsureWorldTickHooks();
		RunLater(ParseFloatArg(Args, 2, 0.f), [Name, Seconds]()
		{
			TSharedRef<FFrameCapture> Capture = MakeShared<FFrameCapture>();
			Capture->Name = Name;
			Capture->EndTime = FPlatformTime::Seconds() + Seconds;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Capture](float)
			{
				Capture->Frame.Add(static_cast<float>(FApp::GetDeltaTime() * 1000.0));
				Capture->Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
				Capture->Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
				Capture->Rhi.Add(FPlatformTime::ToMilliseconds(GRHIThreadTime));
				Capture->Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
				Capture->WorldTick.Add(GLastWorldTickMs);
				Capture->Draws += GNumDrawCallsRHI[0];
				Capture->Prims += GNumPrimitivesDrawnRHI[0];
				if (FPlatformTime::Seconds() < Capture->EndTime)
				{
					return true;
				}
				FinishFrameCapture(*Capture);
				return false;
			}));
		});
	}

	struct FNetCapture
	{
		FString Name;
		double EndTime = 0.0;
		int32 Frames = 0;
		int64 InBytes = 0, OutBytes = 0, InPackets = 0, OutPackets = 0;
		int32 Connections = 0;
		int32 MaxObjects = 0;
		int32 MaxActive = 0;
	};

	void SampleNet(FNetCapture& Capture)
	{
		UWorld* World = FindGameWorld();
		UNetDriver* Driver = World ? World->GetNetDriver() : nullptr;
		if (!Driver)
		{
			return;
		}
		TArray<UNetConnection*> Connections(Driver->ClientConnections);
		if (Driver->ServerConnection)
		{
			Connections.Add(Driver->ServerConnection);
		}
		++Capture.Frames;
		Capture.Connections = FMath::Max(Capture.Connections, Connections.Num());
		for (const UNetConnection* Connection : Connections)
		{
			Capture.InBytes += Connection->InBytesPerSecond;
			Capture.OutBytes += Connection->OutBytesPerSecond;
			Capture.InPackets += Connection->InPacketsPerSecond;
			Capture.OutPackets += Connection->OutPacketsPerSecond;
		}
		Capture.MaxObjects = FMath::Max(Capture.MaxObjects, Driver->GetNetworkObjectList().GetAllObjects().Num());
		Capture.MaxActive = FMath::Max(Capture.MaxActive, Driver->GetNetworkObjectList().GetActiveObjects().Num());
	}

	void NetCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimPerf, Warning, TEXT("사용: Grim.Perf.Net <이름> <초> [지연초]"));
			return;
		}
		const FString Name = Args[0];
		const float Seconds = FMath::Max(1.f, FCString::Atof(*Args[1]));
		RunLater(ParseFloatArg(Args, 2, 0.f), [Name, Seconds]()
		{
			TSharedRef<FNetCapture> Capture = MakeShared<FNetCapture>();
			Capture->Name = Name;
			Capture->EndTime = FPlatformTime::Seconds() + Seconds;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Capture](float)
			{
				SampleNet(*Capture);
				if (FPlatformTime::Seconds() < Capture->EndTime)
				{
					return true;
				}
				const UWorld* World = FindGameWorld();
				const TCHAR* Role = World && World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
				const int32 Samples = FMath::Max(1, Capture->Frames);
				// 연결 수로 나눠 「한 사람당」 값을 낸다
				const int32 PerConnection = FMath::Max(1, Capture->Connections);
				UE_LOG(LogGrimPerf, Display,
					TEXT("GrimPerfNet|name=%s|role=%s|connections=%d|in_kBps=%.2f|out_kBps=%.2f|in_pps=%.1f|out_pps=%.1f|net_objects=%d|net_active=%d"),
					*Capture->Name, Role, Capture->Connections,
					Capture->InBytes / double(Samples) / PerConnection / 1024.0, Capture->OutBytes / double(Samples) / PerConnection / 1024.0,
					Capture->InPackets / double(Samples) / PerConnection, Capture->OutPackets / double(Samples) / PerConnection,
					Capture->MaxObjects, Capture->MaxActive);
				return false;
			}));
		});
	}

	void GotoCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogGrimPerf, Warning, TEXT("사용: Grim.Perf.Goto <X> <Y> <Yaw> [Pitch] [지연초]"));
			return;
		}
		const FVector2D XY(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]));
		const FRotator Rotation(ParseFloatArg(Args, 3, -5.f), FCString::Atof(*Args[2]), 0.f);
		RunLater(ParseFloatArg(Args, 4, 0.f), [XY, Rotation]()
		{
			UWorld* World = FindGameWorld();
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				UE_LOG(LogGrimPerf, Warning, TEXT("[Goto] 로컬 폰 없음"));
				return;
			}
			// 랜드스케이프 높이 — 나무 · 그레이박스에 맞으면 빼고 다시 쏜다
			double GroundZ = Pawn->GetActorLocation().Z;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimPerfGoto), true, Pawn);
			for (int32 Attempt = 0; Attempt < 16; ++Attempt)
			{
				FHitResult Hit;
				if (!World->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, 100000.0), FVector(XY.X, XY.Y, -100000.0), ECC_Visibility, Params))
				{
					break;
				}
				if (Cast<ALandscapeProxy>(Hit.GetActor()))
				{
					GroundZ = Hit.ImpactPoint.Z;
					break;
				}
				Params.AddIgnoredActor(Hit.GetActor());
			}
			Pawn->TeleportTo(FVector(XY.X, XY.Y, GroundZ + 120.0), FRotator(0.f, Rotation.Yaw, 0.f));
			PC->SetControlRotation(Rotation);
			PC->SetViewTarget(Pawn);
			UE_LOG(LogGrimPerf, Display, TEXT("[Goto] (%.0f, %.0f, %.0f) yaw %.0f"), XY.X, XY.Y, GroundZ, Rotation.Yaw);
		});
	}

	FAutoConsoleCommand GSampleCommand(
		TEXT("Grim.Perf.Sample"),
		TEXT("<이름> <초> [지연초] — 그동안의 프레임 · 스레드 · GPU 시간과 드로 콜을 GrimPerf| 한 줄로 로그"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SampleCommand));

	FAutoConsoleCommand GNetCommand(
		TEXT("Grim.Perf.Net"),
		TEXT("<이름> <초> [지연초] — 그동안의 연결당 주고받은 kB/s · 패킷/s · 복제 객체 수를 GrimPerfNet| 한 줄로 로그"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&NetCommand));

	FAutoConsoleCommand GGotoCommand(
		TEXT("Grim.Perf.Goto"),
		TEXT("<X> <Y> <Yaw> [Pitch] [지연초] — 로컬 폰을 그 XY 땅 위로 옮기고 그 방향을 보게 한다(측정 자리)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&GotoCommand));
}

#endif
