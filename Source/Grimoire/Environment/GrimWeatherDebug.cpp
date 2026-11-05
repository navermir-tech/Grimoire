// 날씨 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 복제를 확인할 때 쓴다(Scripts/Environment/net_check_weather.ps1).
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다. 지연 뒤에 그 프로세스의 모든 게임 월드에 적용한다.
// 유니티 빌드에서 다른 디버그 파일(Interaction · Magic)과 이름이 부딪히지 않게 이름 붙은 네임스페이스를 쓴다.

#include "Environment/GrimWeatherSubsystem.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimWeatherMath.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimWeatherDebugLocal
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	FString WeatherName(EGrimWeatherType Type)
	{
		return StaticEnum<EGrimWeatherType>()->GetNameStringByValue(static_cast<int64>(Type));
	}

	bool ParseWeather(const FString& Text, EGrimWeatherType& OutType)
	{
		const int64 Value = StaticEnum<EGrimWeatherType>()->GetValueByNameString(Text);
		if (Value == INDEX_NONE || Value >= static_cast<int64>(EGrimWeatherType::Count))
		{
			return false;
		}
		OutType = static_cast<EGrimWeatherType>(Value);
		return true;
	}

	float ParseFloatArg(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	/** Delay 초 뒤(0 이면 바로) 이 프로세스의 게임 월드마다 Fn 을 부른다 */
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

	/** 그 월드에서 볼 위치: 로컬 플레이어 폰(없으면 첫 폰, 그것도 없으면 원점) */
	FVector GetProbeLocation(UWorld* World)
	{
		const APawn* Fallback = nullptr;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				continue;
			}
			if (PC->IsLocalController())
			{
				return Pawn->GetActorLocation();
			}
			Fallback = Fallback ? Fallback : Pawn;
		}
		return Fallback ? Fallback->GetActorLocation() : FVector::ZeroVector;
	}

	/** Grim.Weather.Set <지역|All> <날씨> [블렌드초] [지연초] — 서버 */
	void SetCommand(const TArray<FString>& Args)
	{
		EGrimWeatherType Type;
		if (Args.Num() < 2 || !ParseWeather(Args[1], Type))
		{
			UE_LOG(LogGrimEnvironment, Warning, TEXT("사용: Grim.Weather.Set <지역|All> <Clear|Cloudy|Fog|Rain|Thunderstorm|Snow|Sandstorm> [블렌드초] [지연초]"));
			return;
		}
		const FString Region = Args[0];
		const float Blend = ParseFloatArg(Args, 2, -1.f);
		RunInGameWorlds(ParseFloatArg(Args, 3, 0.f), [Region, Type, Blend](UWorld* World)
		{
			UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
			if (!Weather || World->GetNetMode() == NM_Client)
			{
				return;
			}
			int32 Count = 0;
			for (const FGrimRegionWeather& State : TArray<FGrimRegionWeather>(Weather->GetAllRegionWeather()))
			{
				if (Region.Equals(TEXT("All"), ESearchCase::IgnoreCase) || State.RegionId.ToString().Equals(Region, ESearchCase::IgnoreCase))
				{
					Count += Weather->SetRegionWeather(State.RegionId, Type, Blend) ? 1 : 0;
				}
			}
			UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] Grim.Weather.Set %s %s -> %d개 지역"), NetModeName(World), *Region, *WeatherName(Type), Count);
		});
	}

	/** Grim.Weather.Dump [지연초] — 이 프로세스에서 본 지역 날씨 · 로컬 플레이어 위치의 값 */
	void DumpCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ParseFloatArg(Args, 0, 0.f), [](UWorld* World)
		{
			const UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
			if (!Weather)
			{
				return;
			}
			for (const FGrimRegionWeather& State : Weather->GetAllRegionWeather())
			{
				UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] 지역 %s: %s (이전 %s, 블렌드 %.0f초)"), NetModeName(World),
					*State.RegionId.ToString(), *WeatherName(State.Type), *WeatherName(State.Previous), State.TransitionSeconds);
			}

			const FVector Probe = GetProbeLocation(World);
			const FGrimClimateExposureBreakdown Exposure = Weather->GetClimateExposureBreakdown(Probe);
			const FGrimWeatherProfile Profile = Weather->GetBlendedProfileAt(Probe);
			UE_LOG(LogGrimEnvironment, Display,
				TEXT("[%s] 위치 (%.0f, %.0f): 지역 %s, 날씨 %s, 기온 %.1f, 등급 더위 %d 추위 %d (지역 %d/%d 날씨 %d/%d), 비 %.2f, 시야 %.0f, 락온 x%.2f, 지붕 %s"),
				NetModeName(World), Probe.X, Probe.Y, *Weather->GetRegionAt(Probe).ToString(), *WeatherName(Weather->GetWeatherAt(Probe)),
				Weather->GetAmbientTemperature(Probe), Exposure.Total.Heat, Exposure.Total.Cold,
				Exposure.Region.Heat, Exposure.Region.Cold, Exposure.Weather.Heat, Exposure.Weather.Cold,
				Profile.RainAmount, Profile.VisibilityDistance, Profile.SoftLockRangeScale,
				Weather->IsSheltered(Probe + FVector(0, 0, 100)) ? TEXT("있음") : TEXT("없음"));
		});
	}

	/** Grim.Weather.Probe <X> <Y> <Z> [지연초] — 그 위치의 지역 · 날씨 · 기온 · 등급 · 지붕 */
	void ProbeCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogGrimEnvironment, Warning, TEXT("사용: Grim.Weather.Probe <X> <Y> <Z> [지연초]"));
			return;
		}
		const FVector Probe(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		RunInGameWorlds(ParseFloatArg(Args, 3, 0.f), [Probe](UWorld* World)
		{
			const UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
			if (!Weather)
			{
				return;
			}
			const FGrimClimateExposureBreakdown Exposure = Weather->GetClimateExposureBreakdown(Probe);
			UE_LOG(LogGrimEnvironment, Display,
				TEXT("[%s] 탐침 (%.0f, %.0f, %.0f): 지역 %s, 날씨 %s, 기온 %.1f, 등급 더위 %d 추위 %d (지역 %d/%d 날씨 %d/%d, 실내 %d/%d), 지붕 %s"),
				NetModeName(World), Probe.X, Probe.Y, Probe.Z, *Weather->GetRegionAt(Probe).ToString(), *WeatherName(Weather->GetWeatherAt(Probe)),
				Weather->GetAmbientTemperature(Probe), Exposure.Total.Heat, Exposure.Total.Cold,
				Exposure.Region.Heat, Exposure.Region.Cold, Exposure.Weather.Heat, Exposure.Weather.Cold,
				Exposure.Sheltered.Heat, Exposure.Sheltered.Cold, Weather->IsSheltered(Probe) ? TEXT("있음") : TEXT("없음"));
		});
	}

	/** Grim.Weather.Strike <지역> [지연초] — 서버: 바로 낙뢰 하나 */
	void StrikeCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimEnvironment, Warning, TEXT("사용: Grim.Weather.Strike <지역> [지연초]"));
			return;
		}
		const FName Region(*Args[0]);
		RunInGameWorlds(ParseFloatArg(Args, 1, 0.f), [Region](UWorld* World)
		{
			UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
			if (Weather && World->GetNetMode() != NM_Client && !Weather->ForceLightningStrike(Region))
			{
				UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] Grim.Weather.Strike %s: 칠 곳 없음(그 지역에 플레이어가 없다)"), NetModeName(World), *Region.ToString());
			}
		});
	}

	FAutoConsoleCommand GSetCommand(
		TEXT("Grim.Weather.Set"),
		TEXT("서버: 지역 날씨를 바꾼다. <지역|All> <날씨> [블렌드초(기본 설정값)] [지연초]. 예: Grim.Weather.Set Meadow Rain 0"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SetCommand));

	FAutoConsoleCommand GDumpCommand(
		TEXT("Grim.Weather.Dump"),
		TEXT("이 프로세스에서 본 지역 날씨와 로컬 플레이어 위치의 기온 · 등급 · 비 · 시야 · 지붕을 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));

	FAutoConsoleCommand GProbeCommand(
		TEXT("Grim.Weather.Probe"),
		TEXT("그 위치의 지역 · 날씨 · 기온 · 등급(몫별) · 지붕을 로그로. <X> <Y> <Z> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ProbeCommand));

	FAutoConsoleCommand GStrikeCommand(
		TEXT("Grim.Weather.Strike"),
		TEXT("서버: 그 지역의 플레이어 둘레에 바로 낙뢰 하나(시험용). <지역> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&StrikeCommand));
}

#endif
