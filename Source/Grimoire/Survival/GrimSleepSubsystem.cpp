#include "Survival/GrimSleepSubsystem.h"
#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

bool UGrimSleepSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimSleepSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

void UGrimSleepSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (IsServer())
	{
		// 밤이 시작될 때 이미 누워 있는 경우도 잡도록 주기적으로도 본다.
		InWorld.GetTimerManager().SetTimer(EvaluateTimer, this, &ThisClass::Evaluate, 1.f, true);
	}
}

void UGrimSleepSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluateTimer);
		World->GetTimerManager().ClearTimer(FadeTimer);
		World->GetTimerManager().ClearTimer(MidnightTimer);
	}
	Super::Deinitialize();
}

void UGrimSleepSubsystem::NotifySleepStateChanged()
{
	Evaluate();
}

TArray<UGrimSurvivalComponent*> UGrimSleepSubsystem::GatherPlayers() const
{
	TArray<UGrimSurvivalComponent*> Result;
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return Result;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AGrimPlayerState* GrimPlayerState = Cast<AGrimPlayerState>(PlayerState);
		if (!GrimPlayerState || GrimPlayerState->IsOnlyASpectator() || GrimPlayerState->IsInactive())
		{
			continue;
		}
		if (UGrimSurvivalComponent* Survival = GrimPlayerState->GetSurvival())
		{
			Result.Add(Survival);
		}
	}
	return Result;
}

void UGrimSleepSubsystem::CountSleepers(int32& OutSleeping, int32& OutPlayers) const
{
	OutSleeping = 0;
	OutPlayers = 0;
	for (const UGrimSurvivalComponent* Survival : GatherPlayers())
	{
		++OutPlayers;
		OutSleeping += Survival->IsSleeping() ? 1 : 0;
	}
}

bool UGrimSleepSubsystem::CanSkipNow() const
{
	const UGrimTimeOfDaySubsystem* TimeOfDay = GetWorld() ? GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr;
	if (!TimeOfDay)
	{
		return false;
	}
	int32 Sleeping = 0;
	int32 Players = 0;
	CountSleepers(Sleeping, Players);
	return FGrimSurvivalModel::ShouldSkipNight(Sleeping, Players, GetDefault<UGrimSurvivalSettings>()->Sleep.SleepSkipRatio, TimeOfDay->IsNight());
}

void UGrimSleepSubsystem::Evaluate()
{
	if (!IsServer() || GetWorld()->GetTimerManager().IsTimerActive(MidnightTimer))
	{
		return;
	}

	const bool bCanSkip = CanSkipNow();
	if (bSkipPending)
	{
		// 페이드 중에 누가 일어나면 취소.
		if (!bCanSkip)
		{
			bSkipPending = false;
			GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
			UE_LOG(LogGrimSurvival, Log, TEXT("밤 건너뛰기 취소 — 잠든 비율이 모자람"));
			OnNightSkipCancelled.Broadcast();
		}
		return;
	}
	if (!bCanSkip)
	{
		return;
	}

	bSkipPending = true;
	const float Fade = GetDefault<UGrimSurvivalSettings>()->Sleep.SkipFadeSeconds;
	for (UGrimSurvivalComponent* Survival : GatherPlayers())
	{
		Survival->NotifyNightSkipFade(Fade);
	}
	OnNightSkipBegin.Broadcast(Fade);
	UE_LOG(LogGrimSurvival, Log, TEXT("밤 건너뛰기 시작 — 페이드 %.1f초"), Fade);

	if (Fade > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(FadeTimer, this, &ThisClass::ExecuteSkip, Fade, false);
	}
	else
	{
		ExecuteSkip();
	}
}

void UGrimSleepSubsystem::ExecuteSkip()
{
	bSkipPending = false;
	UGrimTimeOfDaySubsystem* TimeOfDay = GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>();
	if (!TimeOfDay || !CanSkipNow())
	{
		OnNightSkipCancelled.Broadcast();
		return;
	}

	const FGrimSleepTuning& Sleep = GetDefault<UGrimSurvivalSettings>()->Sleep;
	const float Now = TimeOfDay->GetTimeOfDay();
	const float Target = FMath::Clamp(FGrimTimeOfDayMath::GetDawn(TimeOfDay->GetDayRatio()) + Sleep.WakeAfterDawn, 0.f, 0.999f);
	const float SkippedDays = FGrimSurvivalModel::ComputeDaysUntil(Now, Target);
	const float DayLengthSeconds = TimeOfDay->GetDayLengthSeconds();

	// 잠든 사람 먼저 처리(깨우면 잠든 수가 바뀐다).
	TArray<UGrimSurvivalComponent*> Sleepers;
	for (UGrimSurvivalComponent* Survival : GatherPlayers())
	{
		if (Survival->IsSleeping())
		{
			Sleepers.Add(Survival);
		}
	}

	AdvanceTimeTo(Target);
	for (UGrimSurvivalComponent* Survival : Sleepers)
	{
		Survival->ApplyNightSkip(SkippedDays, DayLengthSeconds);
	}
	UE_LOG(LogGrimSurvival, Log, TEXT("밤 건너뜀 — %.3f일, %d명"), SkippedDays, Sleepers.Num());
	OnNightSkipped.Broadcast(SkippedDays);
}

void UGrimSleepSubsystem::AdvanceTimeTo(float TargetTimeOfDay)
{
	UGrimTimeOfDaySubsystem* TimeOfDay = GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>();
	if (!TimeOfDay)
	{
		return;
	}
	if (TimeOfDay->GetTimeOfDay() < TargetTimeOfDay)
	{
		TimeOfDay->SetTimeOfDay(TargetTimeOfDay);
		return;
	}

	// TODO(날씨 갈래): 앞으로만 가는 SkipTo 가 생기면 이 두 단계를 그것으로 바꾼다.
	PendingTargetTimeOfDay = TargetTimeOfDay;
	PendingStartDay = TimeOfDay->GetDayIndex();
	TimeOfDay->SetTimeOfDay(0.9999f);
	GetWorld()->GetTimerManager().SetTimer(MidnightTimer, this, &ThisClass::FinishAdvanceAcrossMidnight, 0.1f, true);
}

void UGrimSleepSubsystem::FinishAdvanceAcrossMidnight()
{
	UGrimTimeOfDaySubsystem* TimeOfDay = GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>();
	if (!TimeOfDay)
	{
		GetWorld()->GetTimerManager().ClearTimer(MidnightTimer);
		return;
	}
	if (TimeOfDay->GetDayIndex() == PendingStartDay)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(MidnightTimer);
	TimeOfDay->SetTimeOfDay(PendingTargetTimeOfDay);
}
