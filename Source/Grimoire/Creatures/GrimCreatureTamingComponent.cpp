#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Creatures/GrimTamingTags.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace GrimCreatureTamingLocal // 유니티 빌드 이름 충돌 방지
{
	bool IsActiveState(EGrimCreatureAIState State)
	{
		return State == EGrimCreatureAIState::Flee || State == EGrimCreatureAIState::Fight || State == EGrimCreatureAIState::Hunt || State == EGrimCreatureAIState::Ridden;
	}

	bool IsRestState(EGrimCreatureAIState State)
	{
		return State == EGrimCreatureAIState::Stay || State == EGrimCreatureAIState::Sleep;
	}
}

UGrimCreatureTamingComponent::UGrimCreatureTamingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

AGrimCreature* UGrimCreatureTamingComponent::GetCreature() const
{
	return Cast<AGrimCreature>(GetOwner());
}

bool UGrimCreatureTamingComponent::IsTamed() const
{
	const AGrimCreature* Creature = GetCreature();
	return Creature && Creature->GetInstance().IsTamed();
}

float UGrimCreatureTamingComponent::GetLoyalty() const
{
	const AGrimCreature* Creature = GetCreature();
	return Creature ? Creature->GetInstance().Loyalty : 0.f;
}

EGrimLoyaltyStage UGrimCreatureTamingComponent::GetStage() const
{
	return GrimTamingMath::GetStage(GetLoyalty(), UGrimTamingSettings::Get()->Loyalty);
}

APawn* UGrimCreatureTamingComponent::GetOwnerPawn() const
{
	return OwnerPlayerState ? OwnerPlayerState->GetPawn() : nullptr;
}

FString UGrimCreatureTamingComponent::GetDisplayName() const
{
	const AGrimCreature* Creature = GetCreature();
	if (!Creature)
	{
		return FString();
	}
	if (!Creature->GetInstance().Nickname.IsEmpty())
	{
		return Creature->GetInstance().Nickname;
	}
	const UGrimCreatureSpecies* Species = Creature->GetSpecies();
	return Species && !Species->DisplayName.IsEmpty() ? Species->DisplayName.ToString() : Creature->GetInstance().SpeciesId.ToString();
}

void UGrimCreatureTamingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimCreatureTamingComponent, Command);
	DOREPLIFETIME(UGrimCreatureTamingComponent, RecentReasons);
	DOREPLIFETIME(UGrimCreatureTamingComponent, OwnerPlayerState);
	DOREPLIFETIME(UGrimCreatureTamingComponent, bKnockedOut);
}

void UGrimCreatureTamingComponent::BeginPlay()
{
	Super::BeginPlay();
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !GetWorld())
	{
		return;
	}
	if (Creature->HasAuthority())
	{
		const float Interval = FMath::Max(0.5f, UGrimTamingSettings::Get()->Loyalty.TickSeconds);
		LastTickTime = GetWorld()->GetTimeSeconds();
		GetWorld()->GetTimerManager().SetTimer(TickTimer, this, &UGrimCreatureTamingComponent::ServerTick, Interval, true, FMath::FRandRange(0.f, Interval));
		if (UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent())
		{
			HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).AddUObject(this, &UGrimCreatureTamingComponent::HandleHealthChanged);
		}
	}
	RefreshLocalTags();
}

void UGrimCreatureTamingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (AGrimCreature* Creature = GetCreature(); Creature && HealthHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// ───────────────────────── 복제 · 태그 ─────────────────────────

void UGrimCreatureTamingComponent::OnRep_TamingState()
{
	RefreshLocalTags();
	OnTamingChanged.Broadcast(this);
}

void UGrimCreatureTamingComponent::HandleInstanceReplicated()
{
	RefreshLocalTags();
	OnTamingChanged.Broadcast(this);
}

void UGrimCreatureTamingComponent::MarkChanged()
{
	RefreshLocalTags();
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
	OnTamingChanged.Broadcast(this);
}

void UGrimCreatureTamingComponent::RefreshLocalTags()
{
	AGrimCreature* Creature = GetCreature();
	UAbilitySystemComponent* ASC = Creature ? Creature->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}
	const bool bTamed = IsTamed();
	if (bTamed != bLocalTamedTag)
	{
		ASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Tamed, bTamed ? 1 : 0);
		bLocalTamedTag = bTamed;
	}
	const FGameplayTag Wanted = bTamed ? GrimTamingTags::GetStageTag(GetStage()) : FGameplayTag();
	if (Wanted != LocalStageTag)
	{
		if (LocalStageTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(LocalStageTag, 0);
		}
		if (Wanted.IsValid())
		{
			ASC->SetLooseGameplayTagCount(Wanted, 1);
		}
		LocalStageTag = Wanted;
	}
	ASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_KnockedOut, bKnockedOut ? 1 : 0);
}

// ───────────────────────── 서버: 길들이기 ─────────────────────────

void UGrimCreatureTamingComponent::Claim(APlayerState* NewOwner, const FString& OwnerPlayerId)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || OwnerPlayerId.IsEmpty())
	{
		return;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	FGrimCreatureInstance Instance = Creature->GetInstance();
	Instance.OwnerId = OwnerPlayerId;
	Instance.Loyalty = FMath::Clamp(Settings->Loyalty.InitialLoyalty, 1.f, 100.f);
	Creature->SetInstanceServer(Instance);

	OwnerPlayerState = NewOwner;
	Command = EGrimTamedCommand::Follow;
	RecentReasons.Reset();
	FGrimLoyaltyReason Reason;
	Reason.Event = EGrimLoyaltyEvent::Tamed;
	Reason.Delta = Instance.Loyalty;
	GrimTamingMath::PushReason(RecentReasons, Reason, Settings->Loyalty.RecentReasonCount);

	EndEnrage();
	Creature->ClearProvoker();
	if (Settings->Capture.TamedHealToRatio > 0.f && Creature->GetHealthRatio() < Settings->Capture.TamedHealToRatio)
	{
		Creature->SetHealthRatioServer(Settings->Capture.TamedHealToRatio);
	}
	const double Now = GetWorld()->GetTimeSeconds();
	LastFedTime = FMath::Max(LastFedTime, Now);
	LastStarveEventTime = Now;
	bHasExplorePoint = false;
	Creature->SetHomeLocation(Creature->GetActorLocation());
	MarkChanged();
}

void UGrimCreatureTamingComponent::RestoreOwned(APlayerState* NewOwner, const FGrimOwnedCreatureRecord& Record, double OfflineSeconds)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority())
	{
		return;
	}
	OwnerPlayerState = NewOwner;
	Command = Record.Command;
	RecentReasons = Record.Reasons;
	LastFedTime = GetWorld()->GetTimeSeconds();
	LastStarveEventTime = LastFedTime;
	MarkChanged();

	// 접속하지 않은 동안 — 한 시간에 주인 부재 하나(상한).
	const int32 Absent = GrimTamingMath::GetOfflineAbsentEvents(OfflineSeconds, UGrimTamingSettings::Get()->Loyalty.OfflineAbsentMaxEvents);
	for (int32 i = 0; i < Absent && IsTamed(); ++i)
	{
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::OwnerAbsent);
	}
}

FGrimOwnedCreatureRecord UGrimCreatureTamingComponent::MakeRecord() const
{
	FGrimOwnedCreatureRecord Record;
	if (const AGrimCreature* Creature = GetCreature())
	{
		Record.Instance = Creature->GetInstance();
		Record.HealthRatio = FMath::Max(0.01f, Creature->GetHealthRatio());
	}
	Record.Command = Command;
	Record.Reasons = RecentReasons;
	Record.SavedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	return Record;
}

void UGrimCreatureTamingComponent::SetLoyaltyServer(float NewLoyalty)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	FGrimCreatureInstance Instance = Creature->GetInstance();
	Instance.Loyalty = FMath::Clamp(NewLoyalty, 0.f, 100.f);
	Creature->SetInstanceServer(Instance);
}

float UGrimCreatureTamingComponent::ApplyLoyaltyEvent(EGrimLoyaltyEvent Event, float Scale)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !IsTamed())
	{
		return 0.f;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	float Loyalty = Creature->GetInstance().Loyalty;
	const float Applied = GrimTamingMath::ApplyLoyaltyEvent(Loyalty, RecentReasons, Event, Settings->Loyalty, Settings->FindPersonality(Creature->GetInstance().Personality), Scale);
	if (FMath::IsNearlyZero(Applied, 1e-4f))
	{
		return 0.f;
	}
	SetLoyaltyServer(Loyalty);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 충성도 %s seed=%d %s %+.1f → %.1f"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed,
		GrimTamingMath::EventName(Event), Applied, Loyalty);
	MarkChanged();
	if (Loyalty <= 0.f)
	{
		ReleaseToWild(true);
	}
	return Applied;
}

void UGrimCreatureTamingComponent::DebugSetLoyalty(float NewLoyalty)
{
	if (IsTamed())
	{
		SetLoyaltyServer(NewLoyalty);
		MarkChanged();
		if (NewLoyalty <= 0.f)
		{
			ReleaseToWild(true);
		}
	}
}

void UGrimCreatureTamingComponent::SetCommand(EGrimTamedCommand NewCommand)
{
	if (Command != NewCommand)
	{
		Command = NewCommand;
		RestAccum = 0.f;
		MarkChanged();
	}
}

void UGrimCreatureTamingComponent::SetNickname(const FString& NewName)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !IsTamed())
	{
		return;
	}
	FGrimCreatureInstance Instance = Creature->GetInstance();
	Instance.Nickname = GrimTamingMath::SanitizeNickname(NewName, UGrimTamingSettings::Get()->Behavior.NicknameMaxLength);
	Creature->SetInstanceServer(Instance);
	MarkChanged();
}

void UGrimCreatureTamingComponent::ReleaseToWild(bool bLeftByLoyalty)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !IsTamed())
	{
		return;
	}
	const FString FormerOwner = Creature->GetInstance().OwnerId;
	if (UGrimMountComponent* Mount = Creature->GetMount(); Mount && Mount->IsRidden())
	{
		Mount->Dismount(true);
	}
	FGrimCreatureInstance Instance = Creature->GetInstance();
	Instance.OwnerId.Reset();
	Instance.Loyalty = 0.f;
	Instance.Nickname.Reset();
	Creature->SetInstanceServer(Instance);
	OwnerPlayerState = nullptr;
	Command = EGrimTamedCommand::Follow;
	RecentReasons.Reset();
	bKnockedOut = false;
	Creature->SetHomeLocation(Creature->GetActorLocation());
	MarkChanged();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] %s %s seed=%d (주인 %s)"), bLeftByLoyalty ? TEXT("떠남(충성도 0)") : TEXT("놓아줌"),
		*Instance.SpeciesId.ToString(), Instance.Seed, *FormerOwner);
	if (UGrimTamingSubsystem* Subsystem = GetWorld()->GetSubsystem<UGrimTamingSubsystem>())
	{
		Subsystem->NotifyReleased(Creature, FormerOwner, bLeftByLoyalty);
	}
}

void UGrimCreatureTamingComponent::RecordFed(APlayerState* Feeder, bool bFavorite)
{
	if (!GetWorld())
	{
		return;
	}
	LastFeeder = Feeder;
	LastFedTime = GetWorld()->GetTimeSeconds();
	bLastFedFavorite = bFavorite;
	LastStarveEventTime = LastFedTime;
	if (IsTamed())
	{
		ApplyLoyaltyEvent(bFavorite ? EGrimLoyaltyEvent::FedFavorite : EGrimLoyaltyEvent::Fed);
	}
}

bool UGrimCreatureTamingComponent::WasFedBy(const APlayerState* Feeder, bool& bOutFavorite) const
{
	bOutFavorite = false;
	if (!GetWorld() || !Feeder || LastFeeder.Get() != Feeder)
	{
		return false;
	}
	if (GetWorld()->GetTimeSeconds() - LastFedTime > UGrimTamingSettings::Get()->Capture.FedMemorySeconds)
	{
		return false;
	}
	bOutFavorite = bLastFedFavorite;
	return true;
}

// ───────────────────────── 서버: 기절 · 화남 ─────────────────────────

bool UGrimCreatureTamingComponent::HandleKnockedOut()
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !IsTamed())
	{
		return false;
	}
	if (bKnockedOut)
	{
		return true;
	}
	bKnockedOut = true;
	if (UGrimMountComponent* Mount = Creature->GetMount(); Mount && Mount->IsRidden())
	{
		Mount->Dismount(true);
	}
	Creature->ClearProvoker();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 쓰러짐 %s seed=%d — %.0f초 뒤 일어난다"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed,
		UGrimTamingSettings::Get()->Loyalty.KnockoutSeconds);
	ApplyLoyaltyEvent(EGrimLoyaltyEvent::KnockedOut);
	MarkChanged();
	if (IsTamed())
	{
		GetWorld()->GetTimerManager().SetTimer(ReviveTimer, this, &UGrimCreatureTamingComponent::Revive, FMath::Max(0.1f, UGrimTamingSettings::Get()->Loyalty.KnockoutSeconds), false);
	}
	else
	{
		// 쓰러지며 충성도 0 → 떠났다. 야생은 쓰러진 채 둘 수 없으니 바로 일어난다.
		Revive();
	}
	return true;
}

void UGrimCreatureTamingComponent::Revive()
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !bKnockedOut)
	{
		return;
	}
	bKnockedOut = false;
	Creature->SetHealthRatioServer(UGrimTamingSettings::Get()->Loyalty.KnockoutReviveRatio);
	MarkChanged();
}

void UGrimCreatureTamingComponent::Enrage(AActor* Caster)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority())
	{
		return;
	}
	bEnraged = true;
	if (UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Enraged, 1);
	}
	Creature->Provoke(Caster);
	GetWorld()->GetTimerManager().SetTimer(EnrageTimer, this, &UGrimCreatureTamingComponent::EndEnrage, FMath::Max(0.1f, UGrimTamingSettings::Get()->Capture.EnrageSeconds), false);
}

void UGrimCreatureTamingComponent::EndEnrage()
{
	bEnraged = false;
	if (AGrimCreature* Creature = GetCreature())
	{
		if (UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent())
		{
			ASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Enraged, 0);
		}
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnrageTimer);
	}
}

bool UGrimCreatureTamingComponent::IsEnraged() const
{
	return bEnraged;
}

bool UGrimCreatureTamingComponent::IsExhausted() const
{
	return Exertion >= UGrimTamingSettings::Get()->Loyalty.ExertionMax;
}

void UGrimCreatureTamingComponent::NotifyWonTogether()
{
	ApplyLoyaltyEvent(EGrimLoyaltyEvent::WonTogether);
}

void UGrimCreatureTamingComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !IsTamed() || Data.NewValue <= Data.OldValue || !Data.GEModData)
	{
		return;
	}
	// 플레이어가 준 회복(신성 치유 마법 자리)이 최대 체력의 HealMinRatio 이상이면 치료.
	const AActor* Instigator = Data.GEModData->EffectSpec.GetEffectContext().GetOriginalInstigator();
	const bool bFromPlayer = Cast<APlayerState>(Instigator) || (Cast<APawn>(Instigator) && Cast<APawn>(Instigator)->GetPlayerState());
	const float Max = Creature->GetMaxHealth();
	if (bFromPlayer && Max > 0.f && (Data.NewValue - Data.OldValue) / Max >= UGrimTamingSettings::Get()->Loyalty.HealMinRatio)
	{
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::Healed);
	}
}

// ───────────────────────── 서버: 주기 ─────────────────────────

void UGrimCreatureTamingComponent::ServerTick()
{
	using namespace GrimCreatureTamingLocal;
	AGrimCreature* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World || Creature->IsDead())
	{
		return;
	}
	const FGrimLoyaltyTuning& Tuning = UGrimTamingSettings::Get()->Loyalty;
	const double Now = World->GetTimeSeconds();
	const float Dt = static_cast<float>(FMath::Max(0.0, Now - LastTickTime));
	LastTickTime = Now;
	const EGrimCreatureAIState State = Creature->GetAIState();

	// 고단함(야생도 — 지치면 포획 확률 ↑)
	const bool bWasExhausted = IsExhausted();
	if (IsActiveState(State))
	{
		Exertion = FMath::Min(Tuning.ExertionMax, Exertion + Tuning.ExertionPerSecond * Dt);
	}
	else
	{
		Exertion = FMath::Max(0.f, Exertion - Tuning.ExertionRecoveryPerSecond * Dt);
	}
	if (bWasExhausted != IsExhausted())
	{
		if (UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent())
		{
			ASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Exhausted, IsExhausted() ? 1 : 0);
		}
	}

	if (!IsTamed() || bKnockedOut)
	{
		return;
	}

	// 굶김
	if (Now - LastFedTime > Tuning.StarveSeconds && Now - LastStarveEventTime > Tuning.StarveRepeatSeconds)
	{
		LastStarveEventTime = Now;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::Starved);
	}
	// 쉼(기다리기 · 잠)
	RestAccum = IsRestState(State) ? RestAccum + Dt : 0.f;
	if (RestAccum >= Tuning.RestSeconds)
	{
		RestAccum = 0.f;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::Rested);
	}
	// 다친 채 내버려 둠
	InjuredAccum = Creature->GetHealthRatio() < Tuning.InjuredHealthRatio ? InjuredAccum + Dt : 0.f;
	if (InjuredAccum >= Tuning.InjuredNeglectSeconds)
	{
		InjuredAccum = 0.f;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::LeftInjured);
	}
	// 혹사(지쳤는데 계속)
	OverworkAccum = IsExhausted() && IsActiveState(State) ? OverworkAccum + Dt : 0.f;
	if (OverworkAccum >= Tuning.OverworkRepeatSeconds)
	{
		OverworkAccum = 0.f;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::Overworked);
	}
	// 환경: 맞지 않음 = 불 생물이 젖음 · 물 생물이 불붙음, 맞음 = 서식지 태그가 맞는 구역 안
	if (const UGrimCreatureSpecies* Species = Creature->GetSpecies())
	{
		const UGrimElementalStateComponent* Elemental = Creature->GetElementalState();
		const bool bBad = Elemental && ((Species->PrimaryElement.MatchesTag(GrimTags::Element_Fire) && Elemental->HasState(GrimTags::State_Wet))
			|| (Species->PrimaryElement.MatchesTag(GrimTags::Element_Water) && Elemental->HasState(GrimTags::State_Burning)));
		bool bGood = false;
		if (!bBad && !Species->HabitatTags.IsEmpty())
		{
			const FVector Location = Creature->GetActorLocation();
			for (TActorIterator<AGrimHabitatVolume> It(World); It && !bGood; ++It)
			{
				const UBoxComponent* Box = It->GetBounds();
				bGood = Box && It->HabitatTags.HasAny(Species->HabitatTags) && Box->Bounds.GetBox().IsInside(Location);
			}
		}
		BadEnvAccum = bBad ? BadEnvAccum + Dt : 0.f;
		GoodEnvAccum = bGood ? GoodEnvAccum + Dt : 0.f;
		if (BadEnvAccum >= Tuning.BadEnvironmentSeconds)
		{
			BadEnvAccum = 0.f;
			ApplyLoyaltyEvent(EGrimLoyaltyEvent::BadEnvironment);
		}
		if (GoodEnvAccum >= Tuning.GoodEnvironmentSeconds)
		{
			GoodEnvAccum = 0.f;
			ApplyLoyaltyEvent(EGrimLoyaltyEvent::GoodEnvironment);
		}
	}
	// 함께 지낸 날
	DayAccum += Dt;
	if (DayAccum >= Tuning.DaySeconds)
	{
		DayAccum = 0.f;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::DaysTogether);
	}
	// 주인 부재 · 함께 탐험
	const APawn* OwnerPawn = GetOwnerPawn();
	const float OwnerDistance = OwnerPawn ? FVector::Dist(OwnerPawn->GetActorLocation(), Creature->GetActorLocation()) : TNumericLimits<float>::Max();
	if (OwnerDistance > Tuning.AbsentDistance)
	{
		AbsentAccum += Dt;
		if (AbsentAccum >= (bAbsentFired ? Tuning.AbsentRepeatSeconds : Tuning.AbsentSeconds))
		{
			AbsentAccum = 0.f;
			bAbsentFired = true;
			ApplyLoyaltyEvent(EGrimLoyaltyEvent::OwnerAbsent);
		}
	}
	else
	{
		AbsentAccum = 0.f;
		bAbsentFired = false;
		const FVector Here = Creature->GetActorLocation();
		if (!bHasExplorePoint)
		{
			LastExplorePoint = Here;
			bHasExplorePoint = true;
		}
		else if (OwnerDistance < Tuning.AbsentDistance * 0.5f && FVector::Dist(Here, LastExplorePoint) >= Tuning.ExploreDistance)
		{
			LastExplorePoint = Here;
			ApplyLoyaltyEvent(EGrimLoyaltyEvent::Explored);
		}
	}
	// 다툼: 다른 생물과 싸우고 있다
	if (const AGrimCreature* Rival = Cast<AGrimCreature>(Creature->GetProvoker()); Rival && Now - LastQuarrelTime > Tuning.QuarrelRepeatSeconds)
	{
		LastQuarrelTime = Now;
		ApplyLoyaltyEvent(EGrimLoyaltyEvent::Quarrel);
	}
}
