#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

bool UGrimEcosystemSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGrimEcosystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadEcology();
}

void UGrimEcosystemSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (IsServer())
	{
		LastUpdateTime = InWorld.GetTimeSeconds();
		const float Interval = FMath::Max(0.1f, UGrimCreatureSettings::Get()->Ecosystem.UpdateInterval);
		InWorld.GetTimerManager().SetTimer(UpdateTimer, this, &UGrimEcosystemSubsystem::HandleTimer, Interval, true);
	}
}

void UGrimEcosystemSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	Habitats.Reset();
	Creatures.Reset();
	Super::Deinitialize();
}

bool UGrimEcosystemSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

// ───────────────────────── 먹이 관계 표 ─────────────────────────

void UGrimEcosystemSubsystem::LoadEcology()
{
	EcologyRows.Reset();
	const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), UGrimCreatureSettings::Get()->EcologyCSV);
	TArray<FString> Problems;
	LoadEcologyFromCSV(FilePath, EcologyRows, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimCreatures, Warning, TEXT("먹이 관계 표 %s: %s"), *FilePath, *Problem);
	}
}

bool UGrimEcosystemSubsystem::LoadEcologyFromCSV(const FString& FilePath, TArray<FGrimCreatureEcologyRow>& OutRows, TArray<FString>& OutProblems)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutProblems.Add(FString::Printf(TEXT("파일을 열 수 없다: %s"), *FilePath));
		return false;
	}
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), TEXT("DT_CreatureEcology")));
	Table->RowStruct = FGrimCreatureEcologyRow::StaticStruct();
	OutProblems.Append(Table->CreateTableFromCSVString(Text));
	const int32 Before = OutRows.Num();
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		if (const FGrimCreatureEcologyRow* Row = reinterpret_cast<const FGrimCreatureEcologyRow*>(Pair.Value))
		{
			OutRows.Add(*Row);
		}
	}
	Table->MarkAsGarbage();
	return OutRows.Num() > Before;
}

bool UGrimEcosystemSubsystem::IsPreyOf(FName Predator, FName Prey) const
{
	return GetHuntPriority(Predator, Prey) != INDEX_NONE;
}

bool UGrimEcosystemSubsystem::HasPredator(FName Prey) const
{
	return EcologyRows.ContainsByPredicate([Prey](const FGrimCreatureEcologyRow& Row) { return Row.Prey == Prey; });
}

int32 UGrimEcosystemSubsystem::GetHuntPriority(FName Predator, FName Prey) const
{
	for (const FGrimCreatureEcologyRow& Row : EcologyRows)
	{
		if (Row.Predator == Predator && Row.Prey == Prey)
		{
			return FMath::Max(0, Row.Priority);
		}
	}
	return INDEX_NONE;
}

// ───────────────────────── 시간 ─────────────────────────

void UGrimEcosystemSubsystem::GetTimeOfDay(bool& bOutNight, float& OutHours) const
{
	bOutNight = false;
	OutHours = 12.f;
	if (const UWorld* World = GetWorld())
	{
		if (const UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
		{
			bOutNight = Time->IsNight();
			OutHours = Time->GetHours();
		}
	}
}

bool UGrimEcosystemSubsystem::IsActiveNow(EGrimCreatureActivity Activity) const
{
	bool bNight = false;
	float Hours = 12.f;
	GetTimeOfDay(bNight, Hours);
	const FGrimCreatureAITuning& AI = UGrimCreatureSettings::Get()->AI;
	return GrimCreatureMath::IsActiveTime(Activity, bNight, Hours, AI.DuskStartHour, AI.DuskEndHour);
}

// ───────────────────────── 등록 ─────────────────────────

void UGrimEcosystemSubsystem::RegisterCreature(AGrimCreature* Creature)
{
	if (Creature)
	{
		Creatures.AddUnique(Creature);
	}
}

void UGrimEcosystemSubsystem::UnregisterCreature(AGrimCreature* Creature)
{
	Creatures.Remove(Creature);
	Creatures.RemoveAll([](const TWeakObjectPtr<AGrimCreature>& Weak) { return !Weak.IsValid(); });
	for (FGrimHabitatRuntime& Runtime : Habitats)
	{
		for (FGrimHabitatPopulation& Population : Runtime.Populations)
		{
			Population.Active.Remove(Creature);
		}
	}
}

int32 UGrimEcosystemSubsystem::GetActiveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AGrimCreature>& Weak : Creatures)
	{
		if (Weak.IsValid() && !Weak->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

void UGrimEcosystemSubsystem::RegisterHabitat(AGrimHabitatVolume* Volume)
{
	if (!Volume || !IsServer() || Habitats.ContainsByPredicate([Volume](const FGrimHabitatRuntime& R) { return R.Volume.Get() == Volume; }))
	{
		return;
	}
	FGrimHabitatRuntime& Runtime = Habitats.AddDefaulted_GetRef();
	Runtime.Volume = Volume;
	Runtime.Rng.Initialize(Volume->RandomSeed != 0 ? Volume->RandomSeed : FMath::Rand());

	for (const FGrimHabitatSpeciesEntry& Entry : Volume->Species)
	{
		if (!Entry.Species || Entry.MaxCount <= 0)
		{
			continue;
		}
		if (!Volume->HabitatTags.IsEmpty() && !Entry.Species->HabitatTags.IsEmpty() && !Volume->HabitatTags.HasAny(Entry.Species->HabitatTags))
		{
			UE_LOG(LogGrimCreatures, Warning, TEXT("서식 구역 %s 이 종 %s 의 서식지(%s)와 맞지 않는다 — 그래도 놓는다"),
				*Volume->GetName(), *Entry.Species->SpeciesId.ToString(), *Entry.Species->HabitatTags.ToStringSimple());
		}
		FGrimHabitatPopulation& Population = Runtime.Populations.AddDefaulted_GetRef();
		Population.Species = Entry.Species;
		Population.Cap = Entry.MaxCount;
		Population.Activity = Entry.bOverrideActivity ? Entry.Activity : Entry.Species->Activity;
		Population.RegrowSeconds = Entry.RegrowSeconds > 0.f ? Entry.RegrowSeconds : UGrimCreatureSettings::Get()->Ecosystem.RegrowSeconds;
		if (Volume->bStartFull)
		{
			FillPopulation(Runtime, Population, Population.Cap);
		}
	}
	UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 서식 구역 %s 등록 — 종 %d"), *Volume->GetName(), Runtime.Populations.Num());
}

void UGrimEcosystemSubsystem::UnregisterHabitat(AGrimHabitatVolume* Volume)
{
	Habitats.RemoveAll([Volume](const FGrimHabitatRuntime& Runtime) { return Runtime.Volume.Get() == Volume || !Runtime.Volume.IsValid(); });
}

void UGrimEcosystemSubsystem::HandleCreatureDied(AGrimCreature* Creature)
{
	Creatures.Remove(Creature);
	for (FGrimHabitatRuntime& Runtime : Habitats)
	{
		for (FGrimHabitatPopulation& Population : Runtime.Populations)
		{
			if (Population.Active.Remove(Creature) > 0)
			{
				++Population.Killed;
				UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 개체군 %s 현재 %d/%d (죽음)"), *Population.Species->SpeciesId.ToString(), Population.GetCurrent(), Population.Cap);
			}
		}
	}
}

// ───────────────────────── 기록 · 접기 · 펼치기 ─────────────────────────

FGrimFoldedCreature UGrimEcosystemSubsystem::MakeRecord(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, const FVector& Around, float Radius, int32 HerdId)
{
	FGrimFoldedCreature Record;
	Record.Instance = GrimCreatureMath::RollInstance(*Population.Species, Runtime.Rng.RandHelper(MAX_int32 - 1) + 1, UGrimCreatureSettings::Get()->Roll);
	const FVector2D Offset = FVector2D(Runtime.Rng.FRandRange(-Radius, Radius), Runtime.Rng.FRandRange(-Radius, Radius));
	Record.Location = Around + FVector(Offset.X, Offset.Y, 0.f);
	Record.HealthRatio = 1.f;
	Record.HerdId = HerdId;
	return Record;
}

void UGrimEcosystemSubsystem::FillPopulation(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, int32 Count)
{
	AGrimHabitatVolume* Volume = Runtime.Volume.Get();
	if (!Volume || !Population.Species)
	{
		return;
	}
	const float HerdRadius = UGrimCreatureSettings::Get()->Ecosystem.HerdRadius;
	int32 Remaining = Count;
	while (Remaining > 0)
	{
		// 무리 하나: 가운데 한 점 + 둘레. 무리에 거대 개체가 있으면 그것이 우두머리(첫째 자리로).
		const int32 HerdSize = GrimCreatureMath::RollHerdSize(Population.Species->HerdSizeMin, Population.Species->HerdSizeMax, Remaining, Runtime.Rng);
		const int32 HerdId = Population.NextHerdId++;
		const FVector Center = Volume->GetRandomPoint(Runtime.Rng);
		const int32 First = Population.Folded.Num();
		for (int32 i = 0; i < HerdSize; ++i)
		{
			Population.Folded.Add(MakeRecord(Runtime, Population, Center, HerdSize > 1 ? HerdRadius : 0.f, HerdId));
		}
		for (int32 i = First + 1; i < Population.Folded.Num(); ++i)
		{
			if (Population.Folded[i].Instance.bGiant && !Population.Folded[First].Instance.bGiant)
			{
				Population.Folded.Swap(First, i);
				break;
			}
		}
		Remaining -= HerdSize;
	}
}

void UGrimEcosystemSubsystem::FoldCreature(FGrimHabitatPopulation& Population, AGrimCreature* Creature)
{
	if (!Creature)
	{
		return;
	}
	Population.Folded.Add(Creature->MakeFoldedRecord());
	Population.Active.Remove(Creature);
	Creatures.Remove(Creature);
	UE_LOG(LogGrimCreatures, Verbose, TEXT("[Server] 접음 %s seed=%d"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed);
	Creature->Destroy();
}

AGrimCreature* UGrimEcosystemSubsystem::UnfoldRecord(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, const FGrimFoldedCreature& Record)
{
	UWorld* World = GetWorld();
	FVector Ground = Record.Location;
	if (!FindGround(World, Record.Location, Ground))
	{
		// 바닥이 없다(지형 밖) — 기록 그대로 둔다.
		return nullptr;
	}
	AGrimCreature* Creature = AGrimCreature::SpawnCreature(World, Population.Species, Record.Instance, Ground, Runtime.Rng.FRandRange(0.f, 360.f), Record.HealthRatio, Runtime.Volume.Get(), Record.HerdId);
	if (Creature)
	{
		Population.Active.Add(Creature);
	}
	return Creature;
}

bool UGrimEcosystemSubsystem::FindGround(UWorld* World, const FVector& Location, FVector& OutGround)
{
	if (!World)
	{
		return false;
	}
	const FGrimEcosystemTuning& Tuning = UGrimCreatureSettings::Get()->Ecosystem;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimCreatureGround), false);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	const FVector Start = Location + FVector(0.f, 0.f, Tuning.GroundTraceUp);
	const FVector End = Location - FVector(0.f, 0.f, Tuning.GroundTraceDown);
	if (World->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

float UGrimEcosystemSubsystem::GetNearestPlayerDistance(const FVector& Location) const
{
	float Best = TNumericLimits<float>::Max();
	for (const FVector& Player : PlayerLocations)
	{
		Best = FMath::Min(Best, static_cast<float>(FVector::Dist(Player, Location)));
	}
	return Best;
}

// ───────────────────────── 갱신 ─────────────────────────

void UGrimEcosystemSubsystem::HandleTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const float Dt = static_cast<float>(Now - LastUpdateTime);
	LastUpdateTime = Now;
	UpdateNow(Dt);
}

void UGrimEcosystemSubsystem::UpdateNow(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !IsServer())
	{
		return;
	}
	const FGrimEcosystemTuning& Tuning = UGrimCreatureSettings::Get()->Ecosystem;

	PlayerLocations.Reset();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
		{
			PlayerLocations.Add(Pawn->GetActorLocation());
		}
	}
	Creatures.RemoveAll([](const TWeakObjectPtr<AGrimCreature>& Weak) { return !Weak.IsValid(); });

	int32 TotalActive = GetActiveCount();
	int32 SpawnBudget = Tuning.MaxSpawnsPerUpdate;

	for (FGrimHabitatRuntime& Runtime : Habitats)
	{
		AGrimHabitatVolume* Volume = Runtime.Volume.Get();
		if (!Volume)
		{
			continue;
		}
		for (FGrimHabitatPopulation& Population : Runtime.Populations)
		{
			if (!Population.Species)
			{
				continue;
			}
			Population.Active.RemoveAll([](const TWeakObjectPtr<AGrimCreature>& Weak) { return !Weak.IsValid() || Weak->IsDead(); });

			// 1. 회복 — 줄어든 만큼 RegrowSeconds 마다 하나(멸종 없음). 새 개체는 남은 무리 곁(없으면 구역 아무 데).
			const int32 Added = GrimCreatureMath::StepRegrow(Population.GetCurrent(), Population.Cap, Population.RegrowTimer, DeltaSeconds, Population.RegrowSeconds);
			for (int32 i = 0; i < Added; ++i)
			{
				const FVector Around = !Population.Folded.IsEmpty() ? Population.Folded[Runtime.Rng.RandRange(0, Population.Folded.Num() - 1)].Location : Volume->GetRandomPoint(Runtime.Rng);
				Population.Folded.Add(MakeRecord(Runtime, Population, Around, Tuning.HerdRadius, Population.NextHerdId++));
				++Population.Regrown;
				UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 개체군 %s 회복 → %d/%d"), *Population.Species->SpeciesId.ToString(), Population.GetCurrent(), Population.Cap);
			}

			const bool bActiveTime = IsActiveNow(Population.Activity);
			const bool bHide = Population.Species->bHideOutsideActiveTime;

			// 2. 접기 — 멀어졌거나 숨는 시간
			for (int32 i = Population.Active.Num() - 1; i >= 0; --i)
			{
				AGrimCreature* Creature = Population.Active[i].Get();
				if (Creature && GrimCreatureMath::ShouldFold(GetNearestPlayerDistance(Creature->GetActorLocation()), Tuning.DespawnRadius, bActiveTime, bHide))
				{
					FoldCreature(Population, Creature);
					--TotalActive;
				}
			}

			// 3. 펼치기 — 가까운 기록(전역 상한 · 갱신당 상한)
			for (int32 i = Population.Folded.Num() - 1; i >= 0 && SpawnBudget > 0; --i)
			{
				const FGrimFoldedCreature Record = Population.Folded[i];
				if (!GrimCreatureMath::ShouldUnfold(GetNearestPlayerDistance(Record.Location), Tuning.SpawnRadius, bActiveTime, bHide, TotalActive, Tuning.MaxActiveCreatures))
				{
					continue;
				}
				if (UnfoldRecord(Runtime, Population, Record))
				{
					Population.Folded.RemoveAt(i);
					++TotalActive;
					--SpawnBudget;
				}
			}
		}
	}
}
