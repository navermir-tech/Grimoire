#include "Environment/GrimWeatherReplicator.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AGrimWeatherReplicator::AGrimWeatherReplicator()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	// 바뀔 때만 ForceNetUpdate 로 보낸다.
	SetNetUpdateFrequency(1.f);
}

void AGrimWeatherReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimWeatherReplicator, RegionWeather);
}

void AGrimWeatherReplicator::BeginPlay()
{
	Super::BeginPlay();

	// 클라이언트: 첫 복제가 BeginPlay 전에 왔을 수 있으니 여기서도 넘긴다.
	if (!HasAuthority())
	{
		OnRep_RegionWeather();
	}
}

void AGrimWeatherReplicator::SetRegionWeather(const TArray<FGrimRegionWeather>& NewRegionWeather)
{
	check(HasAuthority());
	RegionWeather = NewRegionWeather;
	ForceNetUpdate();
}

void AGrimWeatherReplicator::MulticastLightningStrike_Implementation(FVector_NetQuantize Location)
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>())
		{
			Weather->HandleLightningStrike(Location);
		}
	}
}

void AGrimWeatherReplicator::OnRep_RegionWeather()
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>())
		{
			Weather->ApplyReplicatedRegionWeather(RegionWeather);
		}
	}
}
