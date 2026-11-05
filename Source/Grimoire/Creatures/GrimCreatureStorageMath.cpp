#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimCreatureStorageSettings.h"

namespace GrimCreatureStorageMathLocal // 유니티 빌드 이름 충돌 방지
{
	constexpr int32 MaxBoxEntries = 4096;
}

int32 GrimCreatureStorageMath::GetCircleValue(const TArray<FGrimCircleLimitStep>& Steps, int32 Circle, int32 Fallback)
{
	int32 BestCircle = TNumericLimits<int32>::Lowest();
	int32 Value = Fallback;
	for (const FGrimCircleLimitStep& Step : Steps)
	{
		if (Circle >= Step.MinCircle && Step.MinCircle > BestCircle)
		{
			BestCircle = Step.MinCircle;
			Value = Step.Value;
		}
	}
	return Value;
}

int32 GrimCreatureStorageMath::ComputeLimit(const TArray<FGrimCircleLimitStep>& Steps, int32 Circle, const TArray<FGrimTagLimitBonus>& TagBonuses,
	const FGameplayTagContainer& OwnedTags, int32 Max, int32 Fallback)
{
	int32 Limit = GetCircleValue(Steps, Circle, Fallback);
	for (const FGrimTagLimitBonus& Bonus : TagBonuses)
	{
		if (Bonus.Tag.IsValid() && OwnedTags.HasTagExact(Bonus.Tag))
		{
			Limit += Bonus.Bonus;
		}
	}
	return FMath::Clamp(Limit, 0, FMath::Max(0, Max));
}

EGrimNewTameDestination GrimCreatureStorageMath::DecideNewTame(const FGrimCreatureStorageCounts& Counts, bool bAllowBox)
{
	if (Counts.PartyCount < Counts.PartyLimit)
	{
		return EGrimNewTameDestination::Party;
	}
	if (Counts.bStorageLearned && Counts.StorageCount < Counts.StorageCapacity)
	{
		return EGrimNewTameDestination::Storage;
	}
	if (bAllowBox && Counts.bHasBox && Counts.BoxCount < Counts.BoxCapacity)
	{
		return EGrimNewTameDestination::Box;
	}
	return EGrimNewTameDestination::Refused;
}

EGrimCreatureStorageResult GrimCreatureStorageMath::CheckTransfer(EGrimCreaturePlace From, EGrimCreaturePlace To, const FGrimCreatureStorageCounts& Counts)
{
	if (From == To)
	{
		return EGrimCreatureStorageResult::Invalid;
	}
	const auto AtBox = [](EGrimCreaturePlace Place) { return Place == EGrimCreaturePlace::Box || Place == EGrimCreaturePlace::Released; };
	if ((AtBox(From) || AtBox(To)) && !Counts.bHasBox)
	{
		return EGrimCreatureStorageResult::NoBox;
	}
	switch (To)
	{
	case EGrimCreaturePlace::Party:
		return Counts.PartyCount < Counts.PartyLimit ? EGrimCreatureStorageResult::Ok : EGrimCreatureStorageResult::PartyFull;
	case EGrimCreaturePlace::Storage:
		if (!Counts.bStorageLearned)
		{
			return EGrimCreatureStorageResult::NotLearned;
		}
		return Counts.StorageCount < Counts.StorageCapacity ? EGrimCreatureStorageResult::Ok : EGrimCreatureStorageResult::StorageFull;
	case EGrimCreaturePlace::Box:
		// 풀어놓은 것을 거두면 칸 수는 그대로다.
		if (From != EGrimCreaturePlace::Released && Counts.BoxCount >= Counts.BoxCapacity)
		{
			return EGrimCreatureStorageResult::BoxFull;
		}
		return EGrimCreatureStorageResult::Ok;
	case EGrimCreaturePlace::Released:
		if (From != EGrimCreaturePlace::Box && Counts.BoxCount >= Counts.BoxCapacity)
		{
			return EGrimCreatureStorageResult::BoxFull;
		}
		return Counts.ReleasedCount < Counts.ReleasedMax ? EGrimCreatureStorageResult::Ok : EGrimCreatureStorageResult::ReleasedFull;
	default:
		return EGrimCreatureStorageResult::Invalid;
	}
}

float GrimCreatureStorageMath::GetHealedRatio(float HealthRatio, double StoredUtcSeconds, double NowUtcSeconds, float PerMinute)
{
	const float Base = FMath::Clamp(HealthRatio, 0.01f, 1.f);
	if (StoredUtcSeconds <= 0.0 || PerMinute <= 0.f || NowUtcSeconds <= StoredUtcSeconds)
	{
		return Base;
	}
	const double Minutes = (NowUtcSeconds - StoredUtcSeconds) / 60.0;
	return FMath::Clamp(static_cast<float>(Base + Minutes * PerMinute), 0.01f, 1.f);
}

int32 GrimCreatureStorageMath::FindRecord(const TArray<FGrimOwnedCreatureRecord>& Records, FName SpeciesId, int32 Seed)
{
	return Records.IndexOfByPredicate([SpeciesId, Seed](const FGrimOwnedCreatureRecord& R) { return R.Instance.SpeciesId == SpeciesId && R.Instance.Seed == Seed; });
}

int32 GrimCreatureStorageMath::FindBoxEntry(const TArray<FGrimCreatureBoxEntry>& Entries, FName SpeciesId, int32 Seed)
{
	return Entries.IndexOfByPredicate([SpeciesId, Seed](const FGrimCreatureBoxEntry& E) { return E.Record.Instance.SpeciesId == SpeciesId && E.Record.Instance.Seed == Seed; });
}

bool GrimCreatureStorageMath::SerializeBoxEntries(FArchive& Ar, TArray<FGrimCreatureBoxEntry>& Entries)
{
	int32 Count = Entries.Num();
	Ar << Count;
	if (Ar.IsLoading())
	{
		if (Count < 0 || Count > GrimCreatureStorageMathLocal::MaxBoxEntries)
		{
			Ar.SetError();
			return false;
		}
		Entries.SetNum(Count);
	}
	for (FGrimCreatureBoxEntry& Entry : Entries)
	{
		Ar << Entry.bReleased;
		if (!GrimTamingMath::SerializeOwnedRecord(Ar, Entry.Record))
		{
			return false;
		}
	}
	return !Ar.IsError();
}

bool GrimCreatureStorageMath::ParseNormalSkill(FName TeachesSpell, FName& OutSkillId)
{
	const FString Text = TeachesSpell.ToString();
	const FString Prefix = UGrimCreatureStorageSettings::SkillPrefix();
	if (TeachesSpell.IsNone() || !Text.StartsWith(Prefix) || Text.Len() <= Prefix.Len())
	{
		return false;
	}
	OutSkillId = FName(*Text.Mid(Prefix.Len()));
	return true;
}

FString GrimCreatureStorageMath::ResultText(EGrimCreatureStorageResult Result)
{
	switch (Result)
	{
	case EGrimCreatureStorageResult::Ok: return TEXT("옮겼다");
	case EGrimCreatureStorageResult::NotLearned: return TEXT("생물 보관함을 아직 배우지 않았다 — 스킬북: 쿠스토디아");
	case EGrimCreatureStorageResult::PartyFull: return TEXT("더 데리고 다닐 수 없다");
	case EGrimCreatureStorageResult::StorageFull: return TEXT("보관함이 가득 찼다 — 거점 생물 상자로 옮기자");
	case EGrimCreatureStorageResult::BoxFull: return TEXT("거점 생물 상자가 가득 찼다");
	case EGrimCreatureStorageResult::ReleasedFull: return TEXT("이 상자에는 더 풀어놓을 수 없다");
	case EGrimCreatureStorageResult::NotFound: return TEXT("그 생물이 거기 없다");
	case EGrimCreatureStorageResult::NoBox: return TEXT("거점 생물 상자 곁이 아니다");
	default: return TEXT("옮길 수 없다");
	}
}

const TCHAR* GrimCreatureStorageMath::PlaceName(EGrimCreaturePlace Place)
{
	switch (Place)
	{
	case EGrimCreaturePlace::Party: return TEXT("Party");
	case EGrimCreaturePlace::Storage: return TEXT("Storage");
	case EGrimCreaturePlace::Box: return TEXT("Box");
	case EGrimCreaturePlace::Released: return TEXT("Released");
	default: return TEXT("?");
	}
}

FString GrimCreatureStorageMath::PlaceDisplayName(EGrimCreaturePlace Place)
{
	switch (Place)
	{
	case EGrimCreaturePlace::Party: return TEXT("데리고 다님");
	case EGrimCreaturePlace::Storage: return TEXT("보관함");
	case EGrimCreaturePlace::Box: return TEXT("거점 상자");
	case EGrimCreaturePlace::Released: return TEXT("거점에 풀어놓음");
	default: return TEXT("?");
	}
}

bool GrimCreatureStorageMath::ParsePlace(const FString& Text, EGrimCreaturePlace& OutPlace)
{
	for (EGrimCreaturePlace Place : { EGrimCreaturePlace::Party, EGrimCreaturePlace::Storage, EGrimCreaturePlace::Box, EGrimCreaturePlace::Released })
	{
		if (Text.Equals(PlaceName(Place), ESearchCase::IgnoreCase))
		{
			OutPlace = Place;
			return true;
		}
	}
	return false;
}
