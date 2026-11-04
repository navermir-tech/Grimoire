#include "Interaction/GrimElementalPropagationSubsystem.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionSettings.h"
#include "Engine/World.h"

bool UGrimElementalPropagationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UGrimElementalPropagationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGrimElementalPropagationSubsystem, STATGROUP_Tickables);
}

double UGrimElementalPropagationSubsystem::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UGrimElementalPropagationSubsystem::RegisterSource(UGrimElementalStateComponent* Component)
{
	if (!Component || Sources.ContainsByPredicate([Component](const FSource& Source) { return Source.Component == Component; }))
	{
		return;
	}
	// 불이 붙은 순간이 아니라 한 간격 뒤에 처음 번진다.
	FSource& Added = Sources.AddDefaulted_GetRef();
	Added.Component = Component;
	Added.NextTime = GetNow() + GetDefault<UGrimInteractionSettings>()->PropagationInterval;
}

void UGrimElementalPropagationSubsystem::UnregisterSource(UGrimElementalStateComponent* Component)
{
	if (bProcessing)
	{
		// 처리 중에는 인덱스가 밀리지 않게 비워만 두고, 다음 처리 때 지운다.
		for (FSource& Source : Sources)
		{
			if (Source.Component == Component)
			{
				Source.Component.Reset();
			}
		}
		return;
	}
	Sources.RemoveAll([Component](const FSource& Source) { return Source.Component == Component; });
}

void UGrimElementalPropagationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ProcessDueSources(GetNow());
}

int32 UGrimElementalPropagationSubsystem::ProcessDueSources(double CurrentTime)
{
	Sources.RemoveAll([](const FSource& Source) { return !Source.Component.IsValid(); });
	if (Sources.Num() == 0)
	{
		Cursor = 0;
		return 0;
	}

	const UGrimInteractionSettings* Settings = GetDefault<UGrimInteractionSettings>();
	const int32 StartBudget = Settings->MaxPropagationQueriesPerFrame;
	int32 Budget = StartBudget;

	// 처리 중에 새로 불붙은 이웃이 등록돼도 이번 프레임에는 시작 시점의 출처만 본다.
	const int32 Count = Sources.Num();
	Cursor = Cursor % Count;
	int32 Visited = 0;
	TGuardValue<bool> ProcessingGuard(bProcessing, true);
	for (; Visited < Count && Budget > 0; ++Visited)
	{
		const int32 Index = (Cursor + Visited) % Count;
		if (Sources[Index].NextTime > CurrentTime)
		{
			continue;
		}

		Sources[Index].NextTime = CurrentTime + Settings->PropagationInterval;
		if (UGrimElementalStateComponent* Component = Sources[Index].Component.Get())
		{
			Component->PropagateToNeighbors(Budget);
		}
	}
	Cursor = Sources.Num() > 0 ? (Cursor + Visited) % Sources.Num() : 0;
	return StartBudget - Budget;
}
