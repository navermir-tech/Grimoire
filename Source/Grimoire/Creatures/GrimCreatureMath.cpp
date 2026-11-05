#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"

namespace GrimCreatureMathLocal
{
	const TArray<FLinearColor>& EmptyPalette()
	{
		static const TArray<FLinearColor> Empty;
		return Empty;
	}

	EGrimCreaturePersonality RollPersonality(FRandomStream& Rng)
	{
		const UEnum* Enum = StaticEnum<EGrimCreaturePersonality>();
		// None(0) 과 숨은 _MAX 를 뺀 값 중 하나.
		const int32 Count = Enum ? Enum->NumEnums() - 2 : 0;
		if (Count <= 0)
		{
			return EGrimCreaturePersonality::None;
		}
		return static_cast<EGrimCreaturePersonality>(1 + Rng.RandRange(0, Count - 1));
	}
}

FLinearColor GrimCreatureMath::JitterColor(const FLinearColor& Base, FRandomStream& Rng, float HueDeg, float Sat, float Val)
{
	// 감마 공간(sRGB)에서 HSV 로 흔든다 — 사람 눈에 고른 흔들림.
	const FLinearColor Hsv = Base.LinearRGBToHSV();
	FLinearColor Out = Hsv;
	Out.R = FMath::Fmod(Hsv.R + Rng.FRandRange(-HueDeg, HueDeg) + 360.f, 360.f);
	Out.G = FMath::Clamp(Hsv.G * (1.f + Rng.FRandRange(-Sat, Sat)), 0.f, 1.f);
	Out.B = FMath::Max(0.f, Hsv.B * (1.f + Rng.FRandRange(-Val, Val)));
	FLinearColor Rgb = Out.HSVToLinearRGB();
	Rgb.A = Base.A;
	return Rgb;
}

float GrimCreatureMath::CalcStatScale(float SizeScale, bool bGiant, const FGrimCreatureRollTuning& Tuning)
{
	if (bGiant)
	{
		// 거대 개체: 보통 크기의 위쪽(1.1) 몫에 거대 배율을 곱한다.
		return FMath::Max(0.1f, (1.f + (SizeScale - 1.f) * Tuning.StatPerSize) * Tuning.GiantStatMultiplier);
	}
	return FMath::Max(0.1f, 1.f + (SizeScale - 1.f) * Tuning.StatPerSize);
}

bool GrimCreatureMath::IsPartForSex(EGrimCreatureSexFilter Filter, EGrimCreatureSex Sex)
{
	switch (Filter)
	{
	case EGrimCreatureSexFilter::FemaleOnly: return Sex == EGrimCreatureSex::Female;
	case EGrimCreatureSexFilter::MaleOnly: return Sex == EGrimCreatureSex::Male;
	default: return true;
	}
}

const TArray<FLinearColor>& GrimCreatureMath::GetPaletteForSex(const FGrimCreatureColorRegion& Region, EGrimCreatureSex Sex)
{
	if (Sex == EGrimCreatureSex::Male && !Region.MalePalette.IsEmpty())
	{
		return Region.MalePalette;
	}
	if (Sex == EGrimCreatureSex::Female && !Region.FemalePalette.IsEmpty())
	{
		return Region.FemalePalette;
	}
	return Region.BasePalette;
}

FGrimCreatureInstance GrimCreatureMath::RollInstance(const UGrimCreatureSpecies& Species, int32 Seed, const FGrimCreatureRollTuning& Tuning)
{
	FRandomStream Rng(Seed);
	FGrimCreatureInstance Instance;
	Instance.SpeciesId = Species.SpeciesId.IsNone() ? Species.GetFName() : Species.SpeciesId;
	Instance.Seed = Seed;

	// 굴리는 순서를 바꾸면 같은 씨앗의 개체가 바뀐다 — 순서를 지킨다: 성별 → 거대 → 크기 → 돌연변이 → 색 → 성격.
	Instance.Sex = Rng.FRand() < Species.MaleRatio ? EGrimCreatureSex::Male : EGrimCreatureSex::Female;
	Instance.bGiant = Rng.FRand() < Species.GiantChance;
	Instance.SizeScale = Instance.bGiant
		? Rng.FRandRange(Tuning.GiantSizeMin, Tuning.GiantSizeMax)
		: Rng.FRandRange(Tuning.SizeMin, Tuning.SizeMax);
	Instance.StatScale = CalcStatScale(Instance.SizeScale, Instance.bGiant, Tuning);

	const bool bMutantRoll = Rng.FRand() < Tuning.WildMutantChance;
	const int32 RegionCount = Species.ColorRegions.Num();
	const int32 MutantRegion = (bMutantRoll && RegionCount > 0 && !Species.MutantPalette.IsEmpty()) ? Rng.RandRange(0, RegionCount - 1) : INDEX_NONE;
	Instance.bMutant = MutantRegion != INDEX_NONE;

	Instance.Colors.SetNum(RegionCount);
	for (int32 i = 0; i < RegionCount; ++i)
	{
		const FGrimCreatureColorRegion& Region = Species.ColorRegions[i];
		FGrimCreatureRegionColor& Out = Instance.Colors[i];
		const TArray<FLinearColor>& Palette = (i == MutantRegion) ? Species.MutantPalette : GetPaletteForSex(Region, Instance.Sex);
		if (Palette.IsEmpty())
		{
			Out.Color = FLinearColor(0.5f, 0.5f, 0.5f);
			continue;
		}
		const int32 Index = Rng.RandRange(0, Palette.Num() - 1);
		Out.PaletteIndex = static_cast<uint8>(Index);
		Out.bMutant = (i == MutantRegion);
		// 흔들림은 늘 굴린다(영역 설정과 상관없이 같은 개수) — 씨앗 재현성.
		const FLinearColor Jittered = JitterColor(Palette[Index], Rng, Tuning.HueJitterDegrees, Tuning.SaturationJitter, Tuning.ValueJitter);
		Out.Color = Region.bJitter ? Jittered : Palette[Index];
	}

	Instance.Personality = GrimCreatureMathLocal::RollPersonality(Rng);
	return Instance;
}

TArray<FGrimCreatureLootDrop> GrimCreatureMath::RollLoot(const TArray<FGrimCreatureLootEntry>& Table, bool bGiant, float GiantMultiplier, FRandomStream& Rng)
{
	TArray<FGrimCreatureLootDrop> Drops;
	for (const FGrimCreatureLootEntry& Entry : Table)
	{
		if (Entry.ItemId.IsNone() || Rng.FRand() >= Entry.Chance)
		{
			continue;
		}
		int32 Count = Rng.RandRange(FMath::Min(Entry.MinCount, Entry.MaxCount), FMath::Max(Entry.MinCount, Entry.MaxCount));
		if (bGiant)
		{
			Count = FMath::RoundToInt(Count * GiantMultiplier);
		}
		if (Count <= 0)
		{
			continue;
		}
		if (FGrimCreatureLootDrop* Existing = Drops.FindByPredicate([&Entry](const FGrimCreatureLootDrop& Drop) { return Drop.ItemId == Entry.ItemId; }))
		{
			Existing->Count += Count;
		}
		else
		{
			Drops.Add({ Entry.ItemId, Count });
		}
	}
	return Drops;
}

bool GrimCreatureMath::IsActiveTime(EGrimCreatureActivity Activity, bool bNight, float Hours, float DuskStartHour, float DuskEndHour)
{
	switch (Activity)
	{
	case EGrimCreatureActivity::Diurnal: return !bNight;
	case EGrimCreatureActivity::Nocturnal: return bNight;
	case EGrimCreatureActivity::DayAndDusk: return !bNight || (Hours >= DuskStartHour && Hours < DuskEndHour);
	default: return true;
	}
}

EGrimCreatureAIState GrimCreatureMath::Decide(const FGrimCreatureSenses& Senses, const FGrimCreatureTemperamentInfo& Info)
{
	// 1. 불붙으면 공황.
	if (Senses.bBurning)
	{
		return EGrimCreatureAIState::Flee;
	}
	// 1-1. 계약(파크툼)에 저항해 화나면 덤빈다(공격이 없으면 도망).
	if (Senses.bEnraged)
	{
		return Info.bCanAttack ? EGrimCreatureAIState::Fight : EGrimCreatureAIState::Flee;
	}
	// 2. 빛을 피하는 종(검은 개)은 불빛에서 물러난다.
	if (Info.bAvoidsLight && Senses.bLightNear)
	{
		return EGrimCreatureAIState::Flee;
	}
	const bool bThreatened = Senses.bThreatNear || Senses.bProvoked;
	// 3. 체력이 낮으면 누구든 물러난다.
	if (bThreatened && Senses.HealthRatio <= Info.FleeHealthRatio)
	{
		return EGrimCreatureAIState::Flee;
	}
	// 4. 겁 많은 종은 위협만 보여도.
	if (Info.Temperament == EGrimCreatureTemperament::Timid && bThreatened)
	{
		return EGrimCreatureAIState::Flee;
	}
	// 5. 맞으면 맞선다(공격이 없으면 도망).
	if (Senses.bProvoked)
	{
		return Info.bCanAttack ? EGrimCreatureAIState::Fight : EGrimCreatureAIState::Flee;
	}
	// 6. 공격적 종은 먼저 덤빈다(활동 시간에만 — 잠든 개는 덤비지 않는다).
	if (Info.Temperament == EGrimCreatureTemperament::Aggressive && Info.bCanAttack && Senses.bActiveTime && Senses.bHostileInAggroRange)
	{
		return EGrimCreatureAIState::Fight;
	}
	// 7. 활동 시간 밖이면 잔다.
	if (!Senses.bActiveTime)
	{
		return EGrimCreatureAIState::Sleep;
	}
	// 8. 육식은 먹이를 쫓는다.
	if (Info.Diet == EGrimCreatureDiet::Carnivore && Info.bCanAttack && Senses.bPreyNear)
	{
		return EGrimCreatureAIState::Hunt;
	}
	return EGrimCreatureAIState::Idle;
}

EGrimCreatureAIState GrimCreatureMath::PickCalmActivity(float Roll, EGrimCreatureDiet Diet, const FVector& Weights)
{
	const float IdleW = FMath::Max(0.f, Weights.X);
	const float WanderW = FMath::Max(0.f, Weights.Y);
	const float GrazeW = Diet == EGrimCreatureDiet::Herbivore ? FMath::Max(0.f, Weights.Z) : 0.f;
	const float Total = IdleW + WanderW + GrazeW;
	if (Total <= 0.f)
	{
		return EGrimCreatureAIState::Idle;
	}
	const float Pick = FMath::Clamp(Roll, 0.f, 1.f) * Total;
	if (Pick < IdleW)
	{
		return EGrimCreatureAIState::Idle;
	}
	if (Pick < IdleW + WanderW || GrazeW <= 0.f)
	{
		return EGrimCreatureAIState::Wander;
	}
	return EGrimCreatureAIState::Graze;
}

int32 GrimCreatureMath::StepRegrow(int32 Current, int32 Cap, float& InOutTimer, float Dt, float RegrowSeconds)
{
	if (Current >= Cap)
	{
		InOutTimer = 0.f;
		return 0;
	}
	if (RegrowSeconds <= 0.f)
	{
		InOutTimer = 0.f;
		return Cap - Current;
	}
	InOutTimer += FMath::Max(0.f, Dt);
	int32 Added = 0;
	while (InOutTimer >= RegrowSeconds && Current + Added < Cap)
	{
		InOutTimer -= RegrowSeconds;
		++Added;
	}
	if (Current + Added >= Cap)
	{
		InOutTimer = 0.f;
	}
	return Added;
}

bool GrimCreatureMath::ShouldUnfold(float NearestPlayerDistance, float SpawnRadius, bool bActiveTime, bool bHideOutsideActiveTime, int32 TotalActive, int32 MaxActive)
{
	if (TotalActive >= MaxActive)
	{
		return false;
	}
	if (bHideOutsideActiveTime && !bActiveTime)
	{
		return false;
	}
	return NearestPlayerDistance <= SpawnRadius;
}

bool GrimCreatureMath::ShouldFold(float NearestPlayerDistance, float DespawnRadius, bool bActiveTime, bool bHideOutsideActiveTime)
{
	if (bHideOutsideActiveTime && !bActiveTime)
	{
		return true;
	}
	return NearestPlayerDistance > DespawnRadius;
}

int32 GrimCreatureMath::RollHerdSize(int32 Min, int32 Max, int32 Remaining, FRandomStream& Rng)
{
	const int32 Lo = FMath::Max(1, FMath::Min(Min, Max));
	const int32 Hi = FMath::Max(Lo, FMath::Max(Min, Max));
	return FMath::Clamp(Rng.RandRange(Lo, Hi), 1, FMath::Max(1, Remaining));
}
