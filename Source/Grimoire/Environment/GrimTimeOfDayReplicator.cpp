#include "Environment/GrimTimeOfDayReplicator.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AGrimTimeOfDayReplicator::AGrimTimeOfDayReplicator()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	// 스냅숏이 바뀔 때만 ForceNetUpdate 로 보낸다. 평소엔 거의 갱신할 일이 없다.
	SetNetUpdateFrequency(1.f);
}

void AGrimTimeOfDayReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimTimeOfDayReplicator, Snapshot);
}

void AGrimTimeOfDayReplicator::BeginPlay()
{
	Super::BeginPlay();

	// 클라이언트: 첫 복제가 BeginPlay 전에 왔을 수 있으니 여기서도 넘긴다.
	if (!HasAuthority())
	{
		OnRep_Snapshot();
	}
}

void AGrimTimeOfDayReplicator::SetSnapshot(const FGrimTimeSnapshot& NewSnapshot)
{
	check(HasAuthority());
	Snapshot = NewSnapshot;
	ForceNetUpdate();
}

void AGrimTimeOfDayReplicator::OnRep_Snapshot()
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
		{
			TimeOfDay->ApplyReplicatedSnapshot(Snapshot);
		}
	}
}
