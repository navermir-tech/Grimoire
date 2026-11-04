#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"

void UGrimTimeOfDaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 서버 스냅숏이 오기 전(또는 독립 실행)의 기본값. 클라이언트는 복제가 오면 덮어쓴다.
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();
	Snapshot.TotalDays = Settings->StartTimeOfDay;
	Snapshot.ServerSeconds = 0.0;
	Snapshot.DayLengthSeconds = FMath::Max(1.f, Settings->DayLengthMinutes * 60.f);
	Snapshot.DayRatio = Settings->DayRatio;
}

bool UGrimTimeOfDaySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGrimTimeOfDaySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!IsServer())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Replicator = InWorld.SpawnActor<AGrimTimeOfDayReplicator>(Params);

	PublishSnapshot(GetDefault<UGrimEnvironmentSettings>()->StartTimeOfDay);
	UE_LOG(LogGrimEnvironment, Log, TEXT("시간 시작: %.3f, 하루 %.0f초, 낮 비율 %.3f"), Snapshot.TotalDays, Snapshot.DayLengthSeconds, Snapshot.DayRatio);
}

void UGrimTimeOfDaySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsServer())
	{
		// 서버가 시간을 진행한다: 일정 간격마다 지금 값으로 스냅숏을 다시 찍어 오차가 쌓이지 않게 한다.
		const float Resync = GetDefault<UGrimEnvironmentSettings>()->ResyncIntervalSeconds;
		if (Replicator && GetServerSeconds() - Snapshot.ServerSeconds >= Resync)
		{
			PublishSnapshot(ExtrapolateSnapshot());
		}
	}
	else if (CorrectionDays != 0.0)
	{
		const float CorrectionSeconds = GetDefault<UGrimEnvironmentSettings>()->ClientCorrectionSeconds;
		const double Decay = CorrectionSeconds > 0.f ? FMath::Exp(-DeltaTime / CorrectionSeconds) : 0.0;
		CorrectionDays *= Decay;
		if (FMath::Abs(CorrectionDays) * Snapshot.DayLengthSeconds < 0.01)
		{
			CorrectionDays = 0.0;
		}
	}

	UpdateEvents();
}

TStatId UGrimTimeOfDaySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGrimTimeOfDaySubsystem, STATGROUP_Tickables);
}

double UGrimTimeOfDaySubsystem::GetTotalDays() const
{
	return ExtrapolateSnapshot() + CorrectionDays;
}

float UGrimTimeOfDaySubsystem::GetTimeOfDay() const
{
	return FGrimTimeOfDayMath::GetTimeOfDay(GetTotalDays());
}

int32 UGrimTimeOfDaySubsystem::GetDayIndex() const
{
	return FGrimTimeOfDayMath::GetDayIndex(GetTotalDays());
}

bool UGrimTimeOfDaySubsystem::IsNight() const
{
	return FGrimTimeOfDayMath::IsNight(GetTimeOfDay(), Snapshot.DayRatio);
}

float UGrimTimeOfDaySubsystem::GetSunAngleDegrees() const
{
	return FGrimTimeOfDayMath::GetSunAngleDegrees(GetTimeOfDay(), Snapshot.DayRatio);
}

void UGrimTimeOfDaySubsystem::SetTimeOfDay(float NewTimeOfDay)
{
	if (!IsServer())
	{
		return;
	}
	const double Day = FMath::FloorToDouble(ExtrapolateSnapshot());
	PublishSnapshot(Day + FMath::Clamp<double>(NewTimeOfDay, 0.0, 0.9999));
}

void UGrimTimeOfDaySubsystem::SetDayLengthMinutes(float NewDayLengthMinutes)
{
	if (!IsServer())
	{
		return;
	}
	const double Now = ExtrapolateSnapshot();
	Snapshot.DayLengthSeconds = FMath::Max(1.f, NewDayLengthMinutes * 60.f);
	PublishSnapshot(Now);
}

void UGrimTimeOfDaySubsystem::ApplyReplicatedSnapshot(const FGrimTimeSnapshot& NewSnapshot)
{
	if (IsServer())
	{
		return;
	}

	const double Before = GetTotalDays();
	Snapshot = NewSnapshot;
	const double After = ExtrapolateSnapshot();

	// 첫 스냅숏이나 큰 점프(시각 설정)는 바로 맞추고, 작은 차이만 부드럽게 메운다.
	const double DiffSeconds = (Before - After) * Snapshot.DayLengthSeconds;
	CorrectionDays = (bReceivedServerSnapshot && FMath::Abs(DiffSeconds) < 5.0) ? Before - After : 0.0;
	if (!bReceivedServerSnapshot || CorrectionDays == 0.0)
	{
		bEventsPrimed = false;
	}
	bReceivedServerSnapshot = true;
}

bool UGrimTimeOfDaySubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

double UGrimTimeOfDaySubsystem::GetServerSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0;
	}
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}
	return World->GetTimeSeconds();
}

double UGrimTimeOfDaySubsystem::ExtrapolateSnapshot() const
{
	return FGrimTimeOfDayMath::ExtrapolateTotalDays(Snapshot.TotalDays, Snapshot.ServerSeconds, GetServerSeconds(), Snapshot.DayLengthSeconds);
}

void UGrimTimeOfDaySubsystem::PublishSnapshot(double TotalDays)
{
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();
	Snapshot.TotalDays = TotalDays;
	Snapshot.ServerSeconds = GetServerSeconds();
	Snapshot.DayRatio = Settings->DayRatio;
	if (Replicator)
	{
		Replicator->SetSnapshot(Snapshot);
	}
}

void UGrimTimeOfDaySubsystem::UpdateEvents()
{
	const float TimeOfDay = GetTimeOfDay();
	const bool bNight = FGrimTimeOfDayMath::IsNight(TimeOfDay, Snapshot.DayRatio);
	const int32 Hour = FMath::FloorToInt32(TimeOfDay * 24.f);

	// 시작 직후 · 시각 점프 직후에는 현재 상태만 기억하고 이벤트를 내지 않는다.
	if (!bEventsPrimed)
	{
		bEventsPrimed = true;
		bWasNight = bNight;
		LastHour = Hour;
		return;
	}

	if (Hour != LastHour)
	{
		LastHour = Hour;
		OnHourChanged.Broadcast(Hour);
	}
	if (bNight != bWasNight)
	{
		bWasNight = bNight;
		UE_LOG(LogGrimEnvironment, Verbose, TEXT("%s (%.3f)"), bNight ? TEXT("밤 시작") : TEXT("밤 끝"), TimeOfDay);
		if (bNight)
		{
			OnNightBegin.Broadcast();
		}
		else
		{
			OnNightEnd.Broadcast();
		}
	}
}

// 디버그 콘솔 명령(서버 월드에서 실행).
static FAutoConsoleCommandWithWorldAndArgs GGrimTimeSetCommand(
	TEXT("grim.Time.Set"),
	TEXT("시각을 바꾼다. 인자: 시(0~24). 예: grim.Time.Set 20"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (World && Args.Num() > 0)
		{
			if (UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
			{
				TimeOfDay->SetTimeOfDay(FCString::Atof(*Args[0]) / 24.f);
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GGrimTimeDayLengthCommand(
	TEXT("grim.Time.DayLength"),
	TEXT("하루 길이(분)를 바꾼다. 예: grim.Time.DayLength 2"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (World && Args.Num() > 0)
		{
			if (UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
			{
				TimeOfDay->SetDayLengthMinutes(FCString::Atof(*Args[0]));
			}
		}
	}));
