#include "Environment/GrimWeatherSubsystem.h"
#include "Environment/GrimClimateRegionVolume.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherMath.h"
#include "Environment/GrimWeatherReplicator.h"
#include "Environment/GrimWeatherSettings.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Engine/DataTable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GrimWeatherLocal
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		if (!World)
		{
			return TEXT("None");
		}
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

	/** 표가 없을 때의 기본 지역: 환경 설정의 전역 기온 곡선(M1)과 스펙 「탑 주변 낮 0 · 밤 추위 +1」, 늘 맑음. */
	FGrimClimateRow MakeFallbackRow()
	{
		const UGrimEnvironmentSettings* Env = GetDefault<UGrimEnvironmentSettings>();
		FGrimClimateRow Row;
		Row.Note = TEXT("기후 표 없음 — 환경 설정의 전역 곡선");
		Row.MinTemperature = Env->MinAmbientTemperature;
		Row.MaxTemperature = Env->MaxAmbientTemperature;
		Row.ColdestTimeOfDay = Env->ColdestTimeOfDay;
		Row.WarmestTimeOfDay = Env->WarmestTimeOfDay;
		Row.NightCold = 1;
		Row.FromClear.Clear = 1.f;
		return Row;
	}

	/** 위로 트레이스할 때 막는 것: 월드 스태틱 · 다이내믹(폰 · 물리 몸체는 지붕이 아니다). */
	FCollisionObjectQueryParams ShelterObjectParams()
	{
		FCollisionObjectQueryParams Params;
		Params.AddObjectTypesToQuery(ECC_WorldStatic);
		Params.AddObjectTypesToQuery(ECC_WorldDynamic);
		return Params;
	}

	/** 비 · 낙뢰 대상 오버랩: 속성 사물(월드 다이내믹) · 폰 · 물리 몸체 */
	FCollisionObjectQueryParams TargetObjectParams()
	{
		FCollisionObjectQueryParams Params;
		Params.AddObjectTypesToQuery(ECC_WorldDynamic);
		Params.AddObjectTypesToQuery(ECC_Pawn);
		Params.AddObjectTypesToQuery(ECC_PhysicsBody);
		return Params;
	}
}

// ---------------------------------------------------------------------------
// 수명

bool UGrimWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGrimWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 서버 · 클라이언트 모두 표를 읽는다(조회 함수가 클라이언트에서도 지역 값을 쓴다).
	LoadClimateTable();

	const int32 Seed = GetDefault<UGrimWeatherSettings>()->RandomSeed;
	Random.Initialize(Seed != 0 ? Seed : static_cast<int32>(FPlatformTime::Cycles()));

	// 복제가 오기 전(또는 독립 실행)의 값: 각 지역의 초기 날씨.
	InitializeRegionWeather(0.0);
}

void UGrimWeatherSubsystem::Deinitialize()
{
	Volumes.Reset();
	Replicator = nullptr;
	Super::Deinitialize();
}

void UGrimWeatherSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!IsServer())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Replicator = InWorld.SpawnActor<AGrimWeatherReplicator>(Params);

	const double Now = GetServerSeconds();
	InitializeRegionWeather(Now);
	NextRainPassSeconds = Now + GetDefault<UGrimWeatherSettings>()->RainStimulusIntervalSeconds;
	Publish();

	UE_LOG(LogGrimEnvironment, Log, TEXT("날씨 시작: 지역 %d개, 기본 지역 %s"), Rows.Num(), *GetDefaultRegionId().ToString());
}

void UGrimWeatherSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsServer() || !Replicator || Rows.IsEmpty())
	{
		return;
	}

	const double Now = GetServerSeconds();
	UpdateStateMachine(Now);

	if (Now >= NextRainPassSeconds)
	{
		NextRainPassSeconds = Now + GetDefault<UGrimWeatherSettings>()->RainStimulusIntervalSeconds;
		RunRainPass();
	}

	UpdateLightning(Now);
}

TStatId UGrimWeatherSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGrimWeatherSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// 기후 표

void UGrimWeatherSubsystem::LoadClimateTable()
{
	using namespace GrimWeatherLocal;
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();

	RegionIds.Reset();
	Rows.Reset();

	const UDataTable* Table = nullptr;
	if (!Settings->ClimateTableAsset.IsNull())
	{
		Table = Settings->ClimateTableAsset.LoadSynchronous();
		if (!Table || Table->GetRowStruct() != FGrimClimateRow::StaticStruct())
		{
			UE_LOG(LogGrimEnvironment, Error, TEXT("기후 표 에셋 %s 을 쓸 수 없다(없거나 행 구조체가 다름). CSV 로 대신한다."), *Settings->ClimateTableAsset.ToString());
			Table = nullptr;
		}
	}
	if (Table)
	{
		CopyClimateRows(Table, RegionIds, Rows);
	}
	else
	{
		const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), Settings->ClimateTableCSV);
		TArray<FString> Problems;
		LoadClimateRowsFromCSV(FilePath, RegionIds, Rows, Problems);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogGrimEnvironment, Warning, TEXT("기후 표 %s: %s"), *FilePath, *Problem);
		}
	}

	DefaultRegionIndex = RegionIds.IndexOfByKey(Settings->DefaultRegionId);
	if (DefaultRegionIndex == INDEX_NONE)
	{
		if (!Rows.IsEmpty())
		{
			UE_LOG(LogGrimEnvironment, Error, TEXT("기본 지역 %s 이 기후 표에 없다. 표 없이 전역 곡선을 쓴다."), *Settings->DefaultRegionId.ToString());
		}
		RegionIds.Add(Settings->DefaultRegionId);
		Rows.Add(MakeFallbackRow());
		DefaultRegionIndex = Rows.Num() - 1;
	}

	UE_LOG(LogGrimEnvironment, Log, TEXT("기후 지역 %d개를 읽었다(기본 %s)"), Rows.Num(), *Settings->DefaultRegionId.ToString());
}

void UGrimWeatherSubsystem::CopyClimateRows(const UDataTable* Table, TArray<FName>& OutIds, TArray<FGrimClimateRow>& OutRows)
{
	if (!Table || Table->GetRowStruct() != FGrimClimateRow::StaticStruct())
	{
		return;
	}
	// RowMap 은 CSV 행 순서를 지킨다.
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		if (const FGrimClimateRow* Row = reinterpret_cast<const FGrimClimateRow*>(Pair.Value))
		{
			OutIds.Add(Pair.Key);
			OutRows.Add(*Row);
		}
	}
}

bool UGrimWeatherSubsystem::LoadClimateRowsFromCSV(const FString& FilePath, TArray<FName>& OutIds, TArray<FGrimClimateRow>& OutRows, TArray<FString>& OutProblems)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutProblems.Add(FString::Printf(TEXT("파일을 열 수 없다: %s"), *FilePath));
		return false;
	}

	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), TEXT("DT_ClimateRegions")));
	Table->RowStruct = FGrimClimateRow::StaticStruct();
	OutProblems.Append(Table->CreateTableFromCSVString(Text));
	const int32 Before = OutRows.Num();
	CopyClimateRows(Table, OutIds, OutRows);
	Table->MarkAsGarbage();

	if (OutRows.Num() == Before)
	{
		OutProblems.Add(TEXT("행이 없다"));
		return false;
	}
	return true;
}

void UGrimWeatherSubsystem::InitializeRegionWeather(double NowSeconds)
{
	RegionWeather.SetNum(Rows.Num());
	Runtime.SetNum(Rows.Num());
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const EGrimWeatherType Initial = FGrimWeatherMath::ApplyPrecipitation(Rows[i], FGrimWeatherMath::ClampToAllowed(Rows[i], Rows[i].InitialWeather));
		FGrimRegionWeather& State = RegionWeather[i];
		State.RegionId = RegionIds[i];
		State.Type = Initial;
		State.Previous = Initial;
		State.TransitionStartServerSeconds = NowSeconds;
		State.TransitionSeconds = 0.f;

		Runtime[i] = FRegionRuntime();
		Runtime[i].StateStartSeconds = NowSeconds;
		Runtime[i].NextCheckSeconds = NowSeconds;
	}
}

int32 UGrimWeatherSubsystem::FindRegionIndex(FName RegionId) const
{
	return RegionIds.IndexOfByKey(RegionId);
}

const FGrimClimateRow* UGrimWeatherSubsystem::FindRegionRow(FName RegionId) const
{
	const int32 Index = FindRegionIndex(RegionId);
	return Rows.IsValidIndex(Index) ? &Rows[Index] : nullptr;
}

FName UGrimWeatherSubsystem::GetDefaultRegionId() const
{
	return RegionIds.IsValidIndex(DefaultRegionIndex) ? RegionIds[DefaultRegionIndex] : NAME_None;
}

// ---------------------------------------------------------------------------
// 지역 블렌드

void UGrimWeatherSubsystem::RegisterRegionVolume(AGrimClimateRegionVolume* Volume)
{
	if (!Volume)
	{
		return;
	}
	if (FindRegionIndex(Volume->RegionId) == INDEX_NONE)
	{
		UE_LOG(LogGrimEnvironment, Warning, TEXT("기후 볼륨 %s 의 지역 %s 이 기후 표에 없다 — 무시한다."), *Volume->GetName(), *Volume->RegionId.ToString());
		return;
	}
	Volumes.AddUnique(Volume);
}

void UGrimWeatherSubsystem::UnregisterRegionVolume(AGrimClimateRegionVolume* Volume)
{
	Volumes.Remove(Volume);
}

void UGrimWeatherSubsystem::GatherRegionWeights(const FVector& Location, FRegionSamples& OutSamples) const
{
	OutSamples.Reset();

	TArray<float, TInlineAllocator<8>> Weights;
	TArray<int32, TInlineAllocator<8>> Indices;
	for (const TWeakObjectPtr<AGrimClimateRegionVolume>& Weak : Volumes)
	{
		const AGrimClimateRegionVolume* Volume = Weak.Get();
		if (!Volume)
		{
			continue;
		}
		const float W = Volume->GetBlendWeight(Location);
		const int32 Index = W > 0.f ? FindRegionIndex(Volume->RegionId) : INDEX_NONE;
		if (Index != INDEX_NONE)
		{
			Weights.Add(W);
			Indices.Add(Index);
		}
	}

	const float DefaultWeight = FGrimWeatherMath::NormalizeRegionWeights(MakeArrayView(Weights.GetData(), Weights.Num()));

	auto AddSample = [&OutSamples](int32 Index, float W)
	{
		for (FRegionSample& Sample : OutSamples)
		{
			if (Sample.RegionIndex == Index)
			{
				Sample.Weight += W;
				return;
			}
		}
		OutSamples.Add({ Index, W });
	};

	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		AddSample(Indices[i], Weights[i]);
	}
	if (DefaultWeight > 0.f || OutSamples.IsEmpty())
	{
		AddSample(DefaultRegionIndex, OutSamples.IsEmpty() ? 1.f : DefaultWeight);
	}
}

float UGrimWeatherSubsystem::GetRegionWeightAt(int32 RegionIndex, const FVector& Location) const
{
	FRegionSamples Samples;
	GatherRegionWeights(Location, Samples);
	for (const FRegionSample& Sample : Samples)
	{
		if (Sample.RegionIndex == RegionIndex)
		{
			return Sample.Weight;
		}
	}
	return 0.f;
}

FName UGrimWeatherSubsystem::GetRegionAt(FVector Location) const
{
	FRegionSamples Samples;
	GatherRegionWeights(Location, Samples);
	const FRegionSample* Best = nullptr;
	for (const FRegionSample& Sample : Samples)
	{
		if (!Best || Sample.Weight > Best->Weight)
		{
			Best = &Sample;
		}
	}
	return Best && RegionIds.IsValidIndex(Best->RegionIndex) ? RegionIds[Best->RegionIndex] : NAME_None;
}

// ---------------------------------------------------------------------------
// 기온 · 등급

float UGrimWeatherSubsystem::GetAmbientTemperature(FVector Location) const
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	const float TimeOfDay = GetTimeOfDay();
	const double Now = GetServerSeconds();

	FRegionSamples Samples;
	GatherRegionWeights(Location, Samples);

	float Temperature = 0.f;
	for (const FRegionSample& Sample : Samples)
	{
		const FGrimClimateRow& Row = Rows[Sample.RegionIndex];
		float RegionTemperature = FGrimWeatherMath::GetRegionTemperature(Row, TimeOfDay);
		const FGrimWeatherMix Mix = FGrimWeatherMath::GetRegionMix(RegionWeather[Sample.RegionIndex], Now);
		for (EGrimWeatherType Type : TEnumRange<EGrimWeatherType>())
		{
			RegionTemperature += Mix.Get(Type) * Settings->GetProfile(Type).TemperatureOffset;
		}
		Temperature += Sample.Weight * RegionTemperature;
	}
	return Temperature;
}

FGrimClimateExposureBreakdown UGrimWeatherSubsystem::GetClimateExposureBreakdown(FVector Location) const
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	const float TimeOfDay = GetTimeOfDay();
	const bool bNight = FGrimTimeOfDayMath::IsNight(TimeOfDay, GetDayRatio());
	const bool bNoon = FGrimWeatherMath::IsNoon(TimeOfDay, Settings->NoonStartHour, Settings->NoonEndHour);
	const double Now = GetServerSeconds();

	FRegionSamples Samples;
	GatherRegionWeights(Location, Samples);

	FGrimExposureValue RegionValue;
	FGrimExposureValue WeatherValue;
	for (const FRegionSample& Sample : Samples)
	{
		const FGrimClimateRow& Row = Rows[Sample.RegionIndex];
		const FGrimExposureValue R = FGrimWeatherMath::GetRegionExposure(Row, bNight);
		const FGrimWeatherMix Mix = FGrimWeatherMath::GetRegionMix(RegionWeather[Sample.RegionIndex], Now);
		const FGrimExposureValue W = FGrimWeatherMath::GetWeatherExposure(Row, Mix, bNoon);
		RegionValue.Heat += Sample.Weight * R.Heat;
		RegionValue.Cold += Sample.Weight * R.Cold;
		WeatherValue.Heat += Sample.Weight * W.Heat;
		WeatherValue.Cold += Sample.Weight * W.Cold;
	}

	FGrimClimateExposureBreakdown Out;
	Out.Region.Heat = FGrimWeatherMath::RoundGrade(RegionValue.Heat);
	Out.Region.Cold = FGrimWeatherMath::RoundGrade(RegionValue.Cold);
	Out.Weather.Heat = FGrimWeatherMath::RoundGrade(WeatherValue.Heat);
	Out.Weather.Cold = FGrimWeatherMath::RoundGrade(WeatherValue.Cold);
	FGrimWeatherMath::NetGrades(Out.Region.Heat + Out.Weather.Heat, Out.Region.Cold + Out.Weather.Cold, Out.Total.Heat, Out.Total.Cold);
	FGrimWeatherMath::NetGrades(Out.Region.Heat, Out.Region.Cold, Out.Sheltered.Heat, Out.Sheltered.Cold);
	return Out;
}

FGrimClimateExposure UGrimWeatherSubsystem::GetClimateExposure(FVector Location) const
{
	return GetClimateExposureBreakdown(Location).Total;
}

// ---------------------------------------------------------------------------
// 지붕

bool UGrimWeatherSubsystem::IsSheltered(FVector Location, const AActor* IgnoreActor) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimWeatherShelter), false);
	if (IgnoreActor)
	{
		Params.AddIgnoredActor(IgnoreActor);
		TArray<AActor*> Attached;
		IgnoreActor->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}

	const FVector End = Location + FVector::UpVector * GetDefault<UGrimWeatherSettings>()->ShelterTraceDistance;
	FHitResult Hit;
	return World->LineTraceSingleByObjectType(Hit, Location, End, GrimWeatherLocal::ShelterObjectParams(), Params);
}

bool UGrimWeatherSubsystem::IsActorSheltered(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}
	FVector Start = Actor->GetActorLocation();
	const FBox Box = Actor->GetComponentsBoundingBox(false, false);
	if (Box.IsValid)
	{
		const FVector Center = Box.GetCenter();
		Start = FVector(Center.X, Center.Y, Box.Max.Z + 5.0);
	}
	return IsSheltered(Start, Actor);
}

// ---------------------------------------------------------------------------
// 날씨 조회

FGrimWeatherMix UGrimWeatherSubsystem::GetWeatherMixAt(const FVector& Location) const
{
	const double Now = GetServerSeconds();
	FRegionSamples Samples;
	GatherRegionWeights(Location, Samples);

	FGrimWeatherMix Mix;
	for (const FRegionSample& Sample : Samples)
	{
		Mix.AddScaled(FGrimWeatherMath::GetRegionMix(RegionWeather[Sample.RegionIndex], Now), Sample.Weight);
	}
	return Mix;
}

EGrimWeatherType UGrimWeatherSubsystem::GetWeatherAt(FVector Location) const
{
	return GetWeatherMixAt(Location).GetDominant();
}

float UGrimWeatherSubsystem::GetWeatherWeightAt(FVector Location, EGrimWeatherType Type) const
{
	return Type == EGrimWeatherType::Count ? 0.f : GetWeatherMixAt(Location).Get(Type);
}

FGrimWeatherProfile UGrimWeatherSubsystem::GetBlendedProfileAt(FVector Location) const
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	const FGrimWeatherMix Mix = GetWeatherMixAt(Location);

	FGrimWeatherProfile Out;
	Out.TemperatureOffset = Out.RainAmount = Out.SnowAmount = Out.DustAmount = 0.f;
	Out.VisibilityDistance = Out.SoftLockRangeScale = Out.SunIntensityScale = Out.SkyLightScale = Out.FogDensityAdd = 0.f;
	for (EGrimWeatherType Type : TEnumRange<EGrimWeatherType>())
	{
		const float W = Mix.Get(Type);
		if (W <= 0.f)
		{
			continue;
		}
		const FGrimWeatherProfile& P = Settings->GetProfile(Type);
		Out.TemperatureOffset += W * P.TemperatureOffset;
		Out.RainAmount += W * P.RainAmount;
		Out.SnowAmount += W * P.SnowAmount;
		Out.DustAmount += W * P.DustAmount;
		Out.VisibilityDistance += W * P.VisibilityDistance;
		Out.SoftLockRangeScale += W * P.SoftLockRangeScale;
		Out.SunIntensityScale += W * P.SunIntensityScale;
		Out.SkyLightScale += W * P.SkyLightScale;
		Out.FogDensityAdd += W * P.FogDensityAdd;
	}
	return Out;
}

float UGrimWeatherSubsystem::GetRainAmountAt(FVector Location) const
{
	return GetBlendedProfileAt(Location).RainAmount;
}

float UGrimWeatherSubsystem::GetVisibilityDistance(FVector Location) const
{
	return GetBlendedProfileAt(Location).VisibilityDistance;
}

float UGrimWeatherSubsystem::GetSoftLockRangeScale(FVector Location) const
{
	return GetBlendedProfileAt(Location).SoftLockRangeScale;
}

bool UGrimWeatherSubsystem::GetRegionWeather(FName RegionId, FGrimRegionWeather& OutWeather) const
{
	const int32 Index = FindRegionIndex(RegionId);
	if (!RegionWeather.IsValidIndex(Index))
	{
		return false;
	}
	OutWeather = RegionWeather[Index];
	return true;
}

// ---------------------------------------------------------------------------
// 상태 머신 (서버)

void UGrimWeatherSubsystem::UpdateStateMachine(double NowSeconds)
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	bool bChanged = false;

	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		FRegionRuntime& State = Runtime[i];
		if (NowSeconds - State.StateStartSeconds < Rows[i].MinWeatherSeconds || NowSeconds < State.NextCheckSeconds)
		{
			continue;
		}
		State.NextCheckSeconds = NowSeconds + Settings->TransitionCheckIntervalSeconds;

		const EGrimWeatherType Current = RegionWeather[i].Type;
		const EGrimWeatherType Next = FGrimWeatherMath::PickNextWeather(Rows[i], Current, Random.FRand());
		if (Next != Current)
		{
			StartTransition(i, Next, Settings->BlendSeconds, NowSeconds);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		Publish();
	}
}

void UGrimWeatherSubsystem::StartTransition(int32 RegionIndex, EGrimWeatherType NewWeather, float BlendSeconds, double NowSeconds)
{
	FGrimRegionWeather& State = RegionWeather[RegionIndex];
	const EGrimWeatherType Old = State.Type;

	// 블렌드 도중이면 지금 더 큰 쪽에서 새로 시작한다.
	const float Alpha = FGrimWeatherMath::GetTransitionAlpha(NowSeconds, State.TransitionStartServerSeconds, State.TransitionSeconds);
	State.Previous = Alpha >= 0.5f ? State.Type : State.Previous;
	State.Type = NewWeather;
	State.TransitionStartServerSeconds = NowSeconds;
	State.TransitionSeconds = FMath::Max(0.f, BlendSeconds);

	Runtime[RegionIndex].StateStartSeconds = NowSeconds;
	Runtime[RegionIndex].NextCheckSeconds = NowSeconds;

	UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] 날씨 %s: %s -> %s (블렌드 %.0f초)"), GrimWeatherLocal::NetModeName(GetWorld()),
		*State.RegionId.ToString(), *GrimWeatherLocal::WeatherName(Old), *GrimWeatherLocal::WeatherName(NewWeather), State.TransitionSeconds);
	OnRegionWeatherChanged.Broadcast(State.RegionId, NewWeather, Old);
}

bool UGrimWeatherSubsystem::SetRegionWeather(FName RegionId, EGrimWeatherType NewWeather, float BlendSeconds)
{
	const int32 Index = FindRegionIndex(RegionId);
	if (!IsServer() || !Rows.IsValidIndex(Index) || NewWeather == EGrimWeatherType::Count)
	{
		return false;
	}
	const EGrimWeatherType Allowed = FGrimWeatherMath::ApplyPrecipitation(Rows[Index], FGrimWeatherMath::ClampToAllowed(Rows[Index], NewWeather));
	if (Allowed == RegionWeather[Index].Type)
	{
		return true;
	}
	const float Blend = BlendSeconds < 0.f ? GetDefault<UGrimWeatherSettings>()->BlendSeconds : BlendSeconds;
	StartTransition(Index, Allowed, Blend, GetServerSeconds());
	Publish();
	return true;
}

void UGrimWeatherSubsystem::Publish()
{
	if (Replicator)
	{
		Replicator->SetRegionWeather(RegionWeather);
	}
}

void UGrimWeatherSubsystem::ApplyReplicatedRegionWeather(const TArray<FGrimRegionWeather>& NewRegionWeather)
{
	if (IsServer())
	{
		return;
	}

	for (const FGrimRegionWeather& Incoming : NewRegionWeather)
	{
		const int32 Index = FindRegionIndex(Incoming.RegionId);
		if (!RegionWeather.IsValidIndex(Index))
		{
			continue;
		}
		const EGrimWeatherType Old = RegionWeather[Index].Type;
		RegionWeather[Index] = Incoming;
		if (bReceivedReplication && Old != Incoming.Type)
		{
			UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] 날씨 %s: %s -> %s (블렌드 %.0f초)"), GrimWeatherLocal::NetModeName(GetWorld()),
				*Incoming.RegionId.ToString(), *GrimWeatherLocal::WeatherName(Old), *GrimWeatherLocal::WeatherName(Incoming.Type), Incoming.TransitionSeconds);
			OnRegionWeatherChanged.Broadcast(Incoming.RegionId, Incoming.Type, Old);
		}
	}
	bReceivedReplication = true;
}

// ---------------------------------------------------------------------------
// 비 (서버)

void UGrimWeatherSubsystem::CollectPlayerPawns(TArray<APawn*>& OutPawns) const
{
	OutPawns.Reset();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PC = It->Get())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				OutPawns.Add(Pawn);
			}
		}
	}
}

void UGrimWeatherSubsystem::RunRainPass()
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	const double Now = GetServerSeconds();

	// 어느 지역에도 비가 없으면 오버랩하지 않는다.
	bool bAnyRain = false;
	for (const FGrimRegionWeather& State : RegionWeather)
	{
		const FGrimWeatherMix Mix = FGrimWeatherMath::GetRegionMix(State, Now);
		float Rain = 0.f;
		for (EGrimWeatherType Type : TEnumRange<EGrimWeatherType>())
		{
			Rain += Mix.Get(Type) * Settings->GetProfile(Type).RainAmount;
		}
		bAnyRain |= Rain >= Settings->RainStimulusMinAmount;
	}
	if (!bAnyRain)
	{
		return;
	}

	UWorld* World = GetWorld();
	TArray<APawn*> Pawns;
	CollectPlayerPawns(Pawns);

	FGrimElementalStimulus Stimulus;
	Stimulus.Element = GrimTags::Element_Water;
	Stimulus.Intensity = Settings->RainStimulusIntensity;
	Stimulus.Source = EGrimStimulusSource::Weather;

	const FCollisionObjectQueryParams ObjectParams = GrimWeatherLocal::TargetObjectParams();
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Settings->RainStimulusRadius);

	// 플레이어 둘레만 본다(멀리 있는 사물은 아무도 보지 않는다). 대상마다 그 자리의 지역 · 날씨로 판정한다.
	TSet<const AActor*> Visited;
	int32 Budget = Settings->MaxRainTargetsPerPass;
	int32 Wetted = 0;
	for (const APawn* Center : Pawns)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimWeatherRain), false);
		World->OverlapMultiByObjectType(Overlaps, Center->GetActorLocation(), FQuat::Identity, ObjectParams, Sphere, Params);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (!Target || Visited.Contains(Target))
			{
				continue;
			}
			Visited.Add(Target);
			if (!UGrimElementalStatics::FindReceiver(Target))
			{
				continue;
			}
			if (GetRainAmountAt(Target->GetActorLocation()) < Settings->RainStimulusMinAmount)
			{
				continue;
			}
			if (Budget-- <= 0)
			{
				break;
			}
			if (IsActorSheltered(Target))
			{
				continue;
			}
			if (UGrimElementalStatics::ApplyStimulusToActor(Target, Stimulus, nullptr))
			{
				++Wetted;
			}
		}
		if (Budget <= 0)
		{
			break;
		}
	}

	UE_LOG(LogGrimEnvironment, Verbose, TEXT("비 자극: %d개(대상 %d, 상한 %d)"), Wetted, Visited.Num(), Settings->MaxRainTargetsPerPass);
}

// ---------------------------------------------------------------------------
// 낙뢰 (서버)

void UGrimWeatherSubsystem::UpdateLightning(double NowSeconds)
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	for (int32 i = 0; i < RegionWeather.Num(); ++i)
	{
		FRegionRuntime& State = Runtime[i];
		const float Storm = FGrimWeatherMath::GetRegionMix(RegionWeather[i], NowSeconds).Get(EGrimWeatherType::Thunderstorm);
		if (Storm < Settings->LightningMinAmount)
		{
			State.NextLightningSeconds = 0.0;
			continue;
		}

		const float MinInterval = Settings->LightningMinIntervalSeconds;
		const float MaxInterval = FMath::Max(MinInterval, Settings->LightningMaxIntervalSeconds);
		if (State.NextLightningSeconds <= 0.0)
		{
			State.NextLightningSeconds = NowSeconds + Random.FRandRange(MinInterval, MaxInterval);
			continue;
		}
		if (NowSeconds >= State.NextLightningSeconds)
		{
			State.NextLightningSeconds = NowSeconds + Random.FRandRange(MinInterval, MaxInterval);
			StrikeLightning(i);
		}
	}
}

bool UGrimWeatherSubsystem::ForceLightningStrike(FName RegionId)
{
	const int32 Index = FindRegionIndex(RegionId);
	return IsServer() && Rows.IsValidIndex(Index) && StrikeLightning(Index);
}

bool UGrimWeatherSubsystem::StrikeLightning(int32 RegionIndex)
{
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	UWorld* World = GetWorld();

	// 그 지역에 서 있는 플레이어 하나를 고른다(아무도 없으면 치지 않는다).
	TArray<APawn*> Pawns;
	CollectPlayerPawns(Pawns);
	Pawns.RemoveAll([this, RegionIndex](const APawn* Pawn) { return GetRegionWeightAt(RegionIndex, Pawn->GetActorLocation()) < 0.5f; });
	if (Pawns.IsEmpty())
	{
		return false;
	}
	const APawn* Center = Pawns[Random.RandRange(0, Pawns.Num() - 1)];
	const FVector CenterLocation = Center->GetActorLocation();

	// 후보: 야외의 속성 대상. 점수 = 높이 · 젖음 · 전도체.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimWeatherLightning), false);
	World->OverlapMultiByObjectType(Overlaps, CenterLocation, FQuat::Identity, GrimWeatherLocal::TargetObjectParams(),
		FCollisionShape::MakeSphere(Settings->LightningSearchRadius), Params);

	TArray<AActor*> Candidates;
	TArray<float> Scores;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Candidates.Contains(Target) || !UGrimElementalStatics::FindReceiver(Target))
		{
			continue;
		}
		if (Candidates.Num() >= Settings->MaxLightningCandidates)
		{
			break;
		}
		if (IsActorSheltered(Target))
		{
			continue;
		}
		const FBox Box = Target->GetComponentsBoundingBox(false, false);
		const float Top = Box.IsValid ? static_cast<float>(Box.Max.Z) : static_cast<float>(Target->GetActorLocation().Z);
		const UGrimElementalStateComponent* State = Target->FindComponentByClass<UGrimElementalStateComponent>();
		const bool bConductive = State && State->Material.HasTag(GrimInteractionTags::Material_Conductive);
		const float Wet = UGrimElementalStatics::GetStateIntensity(Target, GrimTags::State_Wet);

		Candidates.Add(Target);
		Scores.Add(FGrimWeatherMath::GetLightningScore(Top - static_cast<float>(CenterLocation.Z), Wet, bConductive,
			Settings->LightningBaseScore, Settings->LightningHeightWeightPerMeter, Settings->LightningWetWeight, Settings->LightningConductiveWeight));
	}

	FVector StrikeLocation;
	AActor* Struck = nullptr;
	const int32 Picked = Random.FRand() < Settings->LightningGroundChance ? INDEX_NONE : FGrimWeatherMath::PickWeightedIndex(Scores, Random.FRand());
	if (Candidates.IsValidIndex(Picked))
	{
		Struck = Candidates[Picked];
		StrikeLocation = Struck->GetActorLocation();

		FGrimElementalStimulus Stimulus;
		Stimulus.Element = GrimTags::Element_Light;
		Stimulus.Intensity = Settings->LightningStimulusIntensity;
		Stimulus.Source = EGrimStimulusSource::Weather;
		UGrimElementalStatics::ApplyStimulusToActor(Struck, Stimulus, nullptr);
	}
	else
	{
		// 빈 땅: 둘레 아무 곳, 아래로 트레이스해 땅을 찾는다(연출만).
		const FVector2D Offset = FVector2D(Random.VRand()).GetSafeNormal() * Random.FRandRange(0.2f, 1.f) * Settings->LightningSearchRadius;
		const FVector Top = CenterLocation + FVector(Offset.X, Offset.Y, 3000.0);
		FHitResult Hit;
		StrikeLocation = World->LineTraceSingleByChannel(Hit, Top, Top - FVector(0, 0, 10000.0), ECC_Visibility)
			? Hit.ImpactPoint : FVector(Top.X, Top.Y, CenterLocation.Z);
	}

	UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] 낙뢰 %s: %s (%.0f, %.0f, %.0f), 후보 %d"), GrimWeatherLocal::NetModeName(World),
		*RegionIds[RegionIndex].ToString(), Struck ? *Struck->GetName() : TEXT("땅"),
		StrikeLocation.X, StrikeLocation.Y, StrikeLocation.Z, Candidates.Num());

	if (Replicator)
	{
		// 서버(리슨 호스트 포함)에서도 _Implementation 이 돈다.
		Replicator->MulticastLightningStrike(StrikeLocation);
	}
	return true;
}

void UGrimWeatherSubsystem::HandleLightningStrike(const FVector& Location)
{
	if (!IsServer())
	{
		UE_LOG(LogGrimEnvironment, Display, TEXT("[%s] 낙뢰 연출 (%.0f, %.0f, %.0f)"), GrimWeatherLocal::NetModeName(GetWorld()), Location.X, Location.Y, Location.Z);
	}
	OnLightningStrike.Broadcast(Location);
}

// ---------------------------------------------------------------------------
// 시간

bool UGrimWeatherSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

double UGrimWeatherSubsystem::GetServerSeconds() const
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

float UGrimWeatherSubsystem::GetTimeOfDay() const
{
	if (const UGrimTimeOfDaySubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr)
	{
		return Time->GetTimeOfDay();
	}
	return GetDefault<UGrimEnvironmentSettings>()->StartTimeOfDay;
}

float UGrimWeatherSubsystem::GetDayRatio() const
{
	if (const UGrimTimeOfDaySubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr)
	{
		return Time->GetDayRatio();
	}
	return GetDefault<UGrimEnvironmentSettings>()->DayRatio;
}
