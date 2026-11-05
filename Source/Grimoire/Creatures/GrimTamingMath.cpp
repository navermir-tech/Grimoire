#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"

DEFINE_LOG_CATEGORY(LogGrimTaming);

FString FGrimCaptureChance::ToString() const
{
	return FString::Printf(TEXT("%.0f%% (기본 %.2f 약함 %.2f 잠 %.2f 지침 %.2f 먹이 %.2f 속성 %.2f 완드 %.2f 서클 %+.2f 숙련 %.2f 장신구 %.2f 등급 -%.2f%s)"),
		Total * 100.f, Base, Weakened, Asleep, Exhausted, Food, Element, Wand, Circle, Proficiency, Accessory, GradePenalty, bCircleTooLow ? TEXT(" 서클 모자람") : TEXT(""));
}

namespace GrimTamingMathLocal // 유니티 빌드에서 다른 파일과 부딪히지 않게
{
	const FGrimLoyaltyStageTuning* FindStageTuning(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning)
	{
		return Tuning.Stages.FindByPredicate([Stage](const FGrimLoyaltyStageTuning& Row) { return Row.Stage == Stage; });
	}

	float FindDelta(EGrimLoyaltyEvent Event, const FGrimLoyaltyTuning& Tuning)
	{
		const FGrimLoyaltyEventDelta* Row = Tuning.EventDeltas.FindByPredicate([Event](const FGrimLoyaltyEventDelta& R) { return R.Event == Event; });
		return Row ? Row->Delta : 0.f;
	}

	constexpr int32 MaxRecords = 512;
	constexpr int32 MaxColors = 16;
	constexpr int32 MaxReasons = 16;
}

FGrimCaptureChance GrimTamingMath::CalcCaptureChance(const FGrimCaptureInput& Input, const FGrimCaptureTuning& Tuning, float WeakenedRatio)
{
	FGrimCaptureChance Out;
	Out.Base = Tuning.BaseChance;

	// 약해짐: 문턱 아래면 몫 하나 + 더 약할수록 깊이 몫(체력 0 에 가까울수록 커진다).
	const float Health = FMath::Clamp(Input.HealthRatio, 0.f, 1.f);
	if (WeakenedRatio > 0.f && Health <= WeakenedRatio)
	{
		Out.Weakened = Tuning.WeakenedBonus + Tuning.WeakenedDepthBonus * (1.f - Health / WeakenedRatio);
	}
	Out.Asleep = Input.bAsleep ? Tuning.AsleepBonus : 0.f;
	Out.Exhausted = Input.bExhausted ? Tuning.ExhaustedBonus : 0.f;
	Out.Food = Input.bFedFavorite ? FMath::Max(Tuning.FavoriteFoodBonus, Tuning.FedBonus) : (Input.bFed ? Tuning.FedBonus : 0.f);
	Out.Element = Input.bElementMatch ? Tuning.ElementMatchBonus : 0.f;
	Out.Wand = FMath::Clamp((Input.WandElementPower - 1.f) * Tuning.WandAffinityScale, 0.f, Tuning.WandAffinityMaxBonus);

	const int32 CircleDiff = Input.CasterCircle - Input.RequiredCircle;
	if (CircleDiff >= 0)
	{
		Out.Circle = FMath::Min(CircleDiff * Tuning.CircleSurplusBonusPerCircle, Tuning.CircleSurplusMaxBonus);
	}
	else
	{
		Out.bCircleTooLow = -CircleDiff > Tuning.MaxCircleDeficit;
		Out.Circle = CircleDiff * Tuning.CircleDeficitPenaltyPerCircle;
	}
	Out.Proficiency = FMath::Max(0.f, Input.ProficiencyBonus);
	Out.Accessory = FMath::Max(0.f, Input.AccessoryBonus);

	const int32 SizeIndex = static_cast<int32>(Input.SizeClass);
	Out.GradePenalty = (Tuning.SizeClassPenalty.IsValidIndex(SizeIndex) ? Tuning.SizeClassPenalty[SizeIndex] : 0.f) + (Input.bGiant ? Tuning.GiantPenalty : 0.f);

	Out.Raw = Out.Base + Out.Weakened + Out.Asleep + Out.Exhausted + Out.Food + Out.Element + Out.Wand + Out.Circle + Out.Proficiency + Out.Accessory - Out.GradePenalty;
	Out.Total = Out.bCircleTooLow ? 0.f : FMath::Clamp(Out.Raw, Tuning.MinChance, FMath::Max(Tuning.MinChance, Tuning.MaxChance));
	return Out;
}

bool GrimTamingMath::RollCapture(float Chance, float Roll)
{
	return Chance > 0.f && Roll < Chance;
}

EGrimLoyaltyStage GrimTamingMath::GetStage(float Loyalty, const FGrimLoyaltyTuning& Tuning)
{
	if (Loyalty <= 0.f)
	{
		return EGrimLoyaltyStage::Gone;
	}
	EGrimLoyaltyStage Best = EGrimLoyaltyStage::Rebellious;
	float BestMin = -1.f;
	for (const FGrimLoyaltyStageTuning& Row : Tuning.Stages)
	{
		if (Row.Stage != EGrimLoyaltyStage::Gone && Loyalty >= Row.MinLoyalty && Row.MinLoyalty > BestMin)
		{
			Best = Row.Stage;
			BestMin = Row.MinLoyalty;
		}
	}
	return Best;
}

bool GrimTamingMath::IsGainEvent(EGrimLoyaltyEvent Event, const FGrimLoyaltyTuning& Tuning)
{
	return GrimTamingMathLocal::FindDelta(Event, Tuning) > 0.f;
}

float GrimTamingMath::GetPersonalityScale(const FGrimPersonalityTuning* Personality, EGrimLoyaltyEvent Event, bool bGain)
{
	if (!Personality)
	{
		return 1.f;
	}
	if (const FGrimLoyaltyEventScale* Row = Personality->EventScales.FindByPredicate([Event](const FGrimLoyaltyEventScale& R) { return R.Event == Event; }))
	{
		return FMath::Max(0.f, Row->Scale);
	}
	return FMath::Max(0.f, bGain ? Personality->GainScale : Personality->LossScale);
}

void GrimTamingMath::PushReason(TArray<FGrimLoyaltyReason>& InOutReasons, const FGrimLoyaltyReason& Reason, int32 Count)
{
	InOutReasons.Insert(Reason, 0);
	const int32 Keep = FMath::Max(1, Count);
	if (InOutReasons.Num() > Keep)
	{
		InOutReasons.SetNum(Keep);
	}
}

float GrimTamingMath::ApplyLoyaltyEvent(float& InOutLoyalty, TArray<FGrimLoyaltyReason>& InOutReasons, EGrimLoyaltyEvent Event,
	const FGrimLoyaltyTuning& Tuning, const FGrimPersonalityTuning* Personality, float Scale)
{
	const float Base = GrimTamingMathLocal::FindDelta(Event, Tuning);
	if (Base == 0.f || Scale <= 0.f)
	{
		return 0.f;
	}
	const float Wanted = Base * GetPersonalityScale(Personality, Event, Base > 0.f) * Scale;
	const float Before = InOutLoyalty;
	InOutLoyalty = FMath::Clamp(InOutLoyalty + Wanted, 0.f, 100.f);
	const float Applied = InOutLoyalty - Before;
	if (!FMath::IsNearlyZero(Applied, 1e-4f))
	{
		FGrimLoyaltyReason Reason;
		Reason.Event = Event;
		Reason.Delta = Applied;
		PushReason(InOutReasons, Reason, Tuning.RecentReasonCount);
	}
	return Applied;
}

bool GrimTamingMath::RollObedience(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll)
{
	const FGrimLoyaltyStageTuning* Row = GrimTamingMathLocal::FindStageTuning(Stage, Tuning);
	return Roll < (Row ? Row->Obedience : 1.f);
}

bool GrimTamingMath::RollMountRefuse(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll)
{
	const FGrimLoyaltyStageTuning* Row = GrimTamingMathLocal::FindStageTuning(Stage, Tuning);
	return Row && Roll < Row->MountRefuseChance;
}

bool GrimTamingMath::RollBuck(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll)
{
	const FGrimLoyaltyStageTuning* Row = GrimTamingMathLocal::FindStageTuning(Stage, Tuning);
	return Row && Roll < Row->BuckChance;
}

float GrimTamingMath::GetMountSpeedScale(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning)
{
	const FGrimLoyaltyStageTuning* Row = GrimTamingMathLocal::FindStageTuning(Stage, Tuning);
	return Row ? FMath::Max(0.f, Row->MountSpeedScale) : 1.f;
}

float GrimTamingMath::GetWorkSpeedScale(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, const FGrimPersonalityTuning* Personality)
{
	const FGrimLoyaltyStageTuning* Row = GrimTamingMathLocal::FindStageTuning(Stage, Tuning);
	return (Row ? FMath::Max(0.f, Row->WorkSpeedScale) : 1.f) * (Personality ? FMath::Max(0.f, Personality->WorkSpeedScale) : 1.f);
}

int32 GrimTamingMath::GetOfflineAbsentEvents(double OfflineSeconds, int32 MaxEvents)
{
	if (OfflineSeconds <= 0.0)
	{
		return 0;
	}
	return FMath::Clamp(static_cast<int32>(OfflineSeconds / 3600.0), 0, FMath::Max(0, MaxEvents));
}

EGrimCreatureAIState GrimTamingMath::DecideTamed(const FGrimTamedSenses& Senses)
{
	if (Senses.bRidden)
	{
		return EGrimCreatureAIState::Ridden;
	}
	if (Senses.bKnockedOut)
	{
		return EGrimCreatureAIState::Sleep;
	}
	if (Senses.bBurning)
	{
		return EGrimCreatureAIState::Flee;
	}
	if (Senses.bProvoked)
	{
		if (!Senses.bCanAttack || Senses.bTimid || Senses.HealthRatio <= Senses.FleeHealthRatio)
		{
			return EGrimCreatureAIState::Flee;
		}
		return EGrimCreatureAIState::Fight;
	}
	if (Senses.Command == EGrimTamedCommand::Wait)
	{
		return EGrimCreatureAIState::Stay;
	}
	if (Senses.OwnerDistance > Senses.FollowDistance)
	{
		return EGrimCreatureAIState::Follow;
	}
	return EGrimCreatureAIState::Idle;
}

bool GrimTamingMath::SerializeOwnedRecord(FArchive& Ar, FGrimOwnedCreatureRecord& Record)
{
	using namespace GrimTamingMathLocal;
	FGrimCreatureInstance& I = Record.Instance;

	FString SpeciesId = I.SpeciesId.ToString();
	Ar << SpeciesId;
	Ar << I.Seed;
	uint8 Sex = static_cast<uint8>(I.Sex);
	Ar << Sex;

	int32 ColorCount = I.Colors.Num();
	Ar << ColorCount;
	if (Ar.IsLoading())
	{
		if (ColorCount < 0 || ColorCount > MaxColors)
		{
			Ar.SetError();
			return false;
		}
		I.Colors.SetNum(ColorCount);
	}
	for (FGrimCreatureRegionColor& Color : I.Colors)
	{
		Ar << Color.PaletteIndex;
		Ar << Color.bMutant;
		Ar << Color.Color;
	}
	Ar << I.SizeScale;
	Ar << I.bGiant;
	Ar << I.bMutant;
	uint8 Personality = static_cast<uint8>(I.Personality);
	Ar << Personality;
	Ar << I.StatScale;
	Ar << I.OwnerId;
	Ar << I.Loyalty;
	Ar << I.Nickname;

	Ar << Record.HealthRatio;
	uint8 Command = static_cast<uint8>(Record.Command);
	Ar << Command;

	int32 ReasonCount = Record.Reasons.Num();
	Ar << ReasonCount;
	if (Ar.IsLoading())
	{
		if (ReasonCount < 0 || ReasonCount > MaxReasons)
		{
			Ar.SetError();
			return false;
		}
		Record.Reasons.SetNum(ReasonCount);
	}
	for (FGrimLoyaltyReason& Reason : Record.Reasons)
	{
		uint8 Event = static_cast<uint8>(Reason.Event);
		Ar << Event;
		Ar << Reason.Delta;
		if (Ar.IsLoading())
		{
			Reason.Event = Event <= static_cast<uint8>(EGrimLoyaltyEvent::OwnerAbsent) ? static_cast<EGrimLoyaltyEvent>(Event) : EGrimLoyaltyEvent::None;
		}
	}
	Ar << Record.SavedUtcSeconds;

	if (Ar.IsLoading())
	{
		I.SpeciesId = FName(*SpeciesId);
		I.Sex = Sex == static_cast<uint8>(EGrimCreatureSex::Male) ? EGrimCreatureSex::Male : EGrimCreatureSex::Female;
		I.Personality = Personality <= static_cast<uint8>(EGrimCreaturePersonality::Sleepyhead) ? static_cast<EGrimCreaturePersonality>(Personality) : EGrimCreaturePersonality::None;
		Record.Command = Command == static_cast<uint8>(EGrimTamedCommand::Wait) ? EGrimTamedCommand::Wait : EGrimTamedCommand::Follow;
		I.Loyalty = FMath::Clamp(I.Loyalty, 0.f, 100.f);
		Record.HealthRatio = FMath::Clamp(Record.HealthRatio, 0.01f, 1.f);
	}
	return !Ar.IsError();
}

bool GrimTamingMath::SerializeOwnedRecords(FArchive& Ar, TArray<FGrimOwnedCreatureRecord>& Records)
{
	int32 Count = Records.Num();
	Ar << Count;
	if (Ar.IsLoading())
	{
		if (Count < 0 || Count > GrimTamingMathLocal::MaxRecords)
		{
			Ar.SetError();
			return false;
		}
		Records.SetNum(Count);
	}
	for (FGrimOwnedCreatureRecord& Record : Records)
	{
		if (!SerializeOwnedRecord(Ar, Record))
		{
			return false;
		}
	}
	return !Ar.IsError();
}

FString GrimTamingMath::SanitizeNickname(const FString& Raw, int32 MaxLength)
{
	FString Out = Raw.Replace(TEXT("\n"), TEXT(" ")).Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\t"), TEXT(" "));
	Out.TrimStartAndEndInline();
	if (Out.Len() > FMath::Max(1, MaxLength))
	{
		Out.LeftInline(FMath::Max(1, MaxLength));
		Out.TrimEndInline();
	}
	return Out;
}

const TCHAR* GrimTamingMath::EventName(EGrimLoyaltyEvent Event)
{
	switch (Event)
	{
	case EGrimLoyaltyEvent::Tamed: return TEXT("Tamed");
	case EGrimLoyaltyEvent::Fed: return TEXT("Fed");
	case EGrimLoyaltyEvent::FedFavorite: return TEXT("FedFavorite");
	case EGrimLoyaltyEvent::Rested: return TEXT("Rested");
	case EGrimLoyaltyEvent::Healed: return TEXT("Healed");
	case EGrimLoyaltyEvent::WonTogether: return TEXT("WonTogether");
	case EGrimLoyaltyEvent::Explored: return TEXT("Explored");
	case EGrimLoyaltyEvent::GoodEnvironment: return TEXT("GoodEnvironment");
	case EGrimLoyaltyEvent::DaysTogether: return TEXT("DaysTogether");
	case EGrimLoyaltyEvent::Starved: return TEXT("Starved");
	case EGrimLoyaltyEvent::Overworked: return TEXT("Overworked");
	case EGrimLoyaltyEvent::LeftInjured: return TEXT("LeftInjured");
	case EGrimLoyaltyEvent::KnockedOut: return TEXT("KnockedOut");
	case EGrimLoyaltyEvent::BadEnvironment: return TEXT("BadEnvironment");
	case EGrimLoyaltyEvent::Quarrel: return TEXT("Quarrel");
	case EGrimLoyaltyEvent::OwnerAbsent: return TEXT("OwnerAbsent");
	default: return TEXT("None");
	}
}

FString GrimTamingMath::EventDisplayText(EGrimLoyaltyEvent Event)
{
	switch (Event)
	{
	case EGrimLoyaltyEvent::Tamed: return TEXT("계약을 맺었다");
	case EGrimLoyaltyEvent::Fed: return TEXT("먹이를 받았다");
	case EGrimLoyaltyEvent::FedFavorite: return TEXT("좋아하는 먹이를 받았다");
	case EGrimLoyaltyEvent::Rested: return TEXT("푹 쉬었다");
	case EGrimLoyaltyEvent::Healed: return TEXT("치료를 받았다");
	case EGrimLoyaltyEvent::WonTogether: return TEXT("함께 싸워 이겼다");
	case EGrimLoyaltyEvent::Explored: return TEXT("함께 먼 길을 갔다");
	case EGrimLoyaltyEvent::GoodEnvironment: return TEXT("살기 좋은 곳에 있다");
	case EGrimLoyaltyEvent::DaysTogether: return TEXT("오래 함께 지냈다");
	case EGrimLoyaltyEvent::Starved: return TEXT("굶었다");
	case EGrimLoyaltyEvent::Overworked: return TEXT("지쳤는데 혹사당했다");
	case EGrimLoyaltyEvent::LeftInjured: return TEXT("다친 채 내버려졌다");
	case EGrimLoyaltyEvent::KnockedOut: return TEXT("싸우다 쓰러졌다");
	case EGrimLoyaltyEvent::BadEnvironment: return TEXT("맞지 않는 곳에 있다");
	case EGrimLoyaltyEvent::Quarrel: return TEXT("다른 생물과 다퉜다");
	case EGrimLoyaltyEvent::OwnerAbsent: return TEXT("주인이 오래 없었다");
	default: return TEXT("");
	}
}
