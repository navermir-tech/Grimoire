#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionEffects.h"
#include "Progression/GrimProgressionSettings.h"
#include "Progression/GrimProgressionTags.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGrimLevelComponent::UGrimLevelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimLevelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 레벨은 모두에게(이름표 · 파티 창), 경험 · 보너스 포인트는 소유자에게만.
	DOREPLIFETIME(UGrimLevelComponent, Level);
	DOREPLIFETIME_CONDITION(UGrimLevelComponent, TotalXP, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UGrimLevelComponent, BonusSkillPoints, COND_OwnerOnly);
}

UGrimLevelComponent* UGrimLevelComponent::FindFor(const AActor* Owner)
{
	return Owner ? Owner->FindComponentByClass<UGrimLevelComponent>() : nullptr;
}

UAbilitySystemComponent* UGrimLevelComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

// ───────────────────────── 시작 · 끝 ─────────────────────────

void UGrimLevelComponent::BeginPlay()
{
	Super::BeginPlay();

	AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	if (!PlayerState)
	{
		return;
	}
	ActorInfoReadyHandle = PlayerState->OnAbilityActorInfoReady.AddUObject(this, &ThisClass::HandleAbilityActorInfoReady);

	if (PlayerState->HasAuthority())
	{
		// 경험 출처: 스킬북으로 새 마법 · 서클 승급
		if (UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook())
		{
			Spellbook->OnLearnResult.AddDynamic(this, &ThisClass::HandleLearnResult);
		}
		if (UGrimProgressionComponent* Progression = PlayerState->GetProgression())
		{
			Progression->OnCircleUpResult.AddDynamic(this, &ThisClass::HandleCircleUpResult);
		}
		bBoundSources = true;
	}

	// 폰이 먼저 초기화를 마친 경우(BeginPlay 보다 빠른 빙의)도 놓치지 않는다.
	if (const UAbilitySystemComponent* ASC = GetASC(); ASC && ASC->AbilityActorInfo.IsValid())
	{
		if (APawn* Pawn = Cast<APawn>(ASC->GetAvatarActor()))
		{
			HandleAbilityActorInfoReady(PlayerState, Pawn);
		}
	}
}

void UGrimLevelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		PlayerState->OnAbilityActorInfoReady.Remove(ActorInfoReadyHandle);
		if (bBoundSources)
		{
			if (UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook())
			{
				Spellbook->OnLearnResult.RemoveDynamic(this, &ThisClass::HandleLearnResult);
			}
			if (UGrimProgressionComponent* Progression = PlayerState->GetProgression())
			{
				Progression->OnCircleUpResult.RemoveDynamic(this, &ThisClass::HandleCircleUpResult);
			}
		}
	}
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RegionTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimLevelComponent::HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	ApplyLevelBonus();

	if (UWorld* World = GetWorld(); World && !World->GetTimerManager().IsTimerActive(RegionTimer))
	{
		World->GetTimerManager().SetTimer(RegionTimer, this, &ThisClass::CheckRegion, UGrimLevelSettings::Get()->RegionCheckInterval, true);
	}
}

// ───────────────────────── 읽기 ─────────────────────────

int32 UGrimLevelComponent::GetMaxLevel() const
{
	return FGrimLevelMath::GetMaxLevel(UGrimLevelSettings::GetCurve(), UGrimLevelSettings::Get()->LevelCap);
}

FGrimLevelProgress UGrimLevelComponent::GetProgress() const
{
	FGrimLevelProgress Progress = FGrimLevelMath::GetProgress(UGrimLevelSettings::GetCurve(), TotalXP, UGrimLevelSettings::Get()->LevelCap);
	// 다른 플레이어(경험이 복제되지 않음)는 복제된 레벨을 믿는다.
	Progress.Level = FMath::Max(Progress.Level, Level);
	return Progress;
}

int32 UGrimLevelComponent::GetTotalSkillPoints() const
{
	return FGrimLevelMath::GetSkillPointsAtLevel(UGrimLevelSettings::GetCurve(), Level) + BonusSkillPoints;
}

float UGrimLevelComponent::GetMaxHealthBonus() const
{
	return FGrimLevelMath::GetMaxHealthBonusAtLevel(UGrimLevelSettings::GetCurve(), Level);
}

float UGrimLevelComponent::GetMaxManaBonus() const
{
	return FGrimLevelMath::GetMaxManaBonusAtLevel(UGrimLevelSettings::GetCurve(), Level);
}

// ───────────────────────── 경험 ─────────────────────────

float UGrimLevelComponent::AwardExperience(float Amount, FGameplayTag Source)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.f)
	{
		return 0.f;
	}
	const UGrimLevelSettings* Settings = UGrimLevelSettings::Get();
	const FGrimXPSourceRule Rule = Settings->GetSourceRule(Source);
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const int32 Repeat = RepeatTracker.Register(Source, Now, Rule.RepeatWindowSeconds);
	const float Gain = FGrimLevelMath::ComputeXPGain(Amount, Repeat, Rule.RepeatDecay, Rule.MinMultiplier, Settings->GlobalXPRate);

	const float Before = TotalXP;
	TotalXP = FGrimLevelMath::ClampXP(UGrimLevelSettings::GetCurve(), TotalXP + Gain, Settings->LevelCap);
	const float Given = TotalXP - Before;
	UE_LOG(LogGrimProgression, Log, TEXT("[Server] %s 경험 +%.1f (요청 %.1f, %s 반복 %d) → 누적 %.1f"), *GetNameSafe(GetOwner()), Given, Amount,
		*Source.ToString(), Repeat, TotalXP);
	RefreshLevel();
	return Given;
}

float UGrimLevelComponent::AwardDefaultExperience(FGameplayTag Source)
{
	return AwardExperience(UGrimLevelSettings::Get()->GetSourceRule(Source).DefaultXP, Source);
}

FString UGrimLevelComponent::MakeDiscoveryKey(FGameplayTag Source, FName Key)
{
	return FString::Printf(TEXT("%s|%s"), *Source.GetTagName().ToString(), *Key.ToString());
}

bool UGrimLevelComponent::AwardDiscovery(FGameplayTag Source, FName Key)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Key.IsNone())
	{
		return false;
	}
	bool bAlready = false;
	Discoveries.Add(MakeDiscoveryKey(Source, Key), &bAlready);
	if (bAlready)
	{
		return false;
	}
	UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 첫 발견: %s %s"), *GetNameSafe(GetOwner()), *Source.ToString(), *Key.ToString());
	AwardDefaultExperience(Source);
	return true;
}

bool UGrimLevelComponent::HasDiscovered(FGameplayTag Source, FName Key) const
{
	return Discoveries.Contains(MakeDiscoveryKey(Source, Key));
}

bool UGrimLevelComponent::NotifyRuleDiscovered(FName RuleId)
{
	return AwardDiscovery(GrimProgressionTags::XPSource_RuleDiscovery, RuleId);
}

void UGrimLevelComponent::AddBonusSkillPoints(int32 Points)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BonusSkillPoints = FMath::Max(BonusSkillPoints + Points, 0);
	}
}

void UGrimLevelComponent::SetTotalXP(float NewTotalXP)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TotalXP = FGrimLevelMath::ClampXP(UGrimLevelSettings::GetCurve(), NewTotalXP, UGrimLevelSettings::Get()->LevelCap);
		RefreshLevel();
	}
}

void UGrimLevelComponent::SetLevel(int32 NewLevel)
{
	const FGrimLevelCurve& Curve = UGrimLevelSettings::GetCurve();
	SetTotalXP(FGrimLevelMath::GetXPForLevel(Curve, FMath::Clamp(NewLevel, 1, GetMaxLevel())));
}

void UGrimLevelComponent::Server_DebugAddXP_Implementation(float Amount, FGameplayTag Source)
{
#if !UE_BUILD_SHIPPING
	AwardExperience(Amount, Source.IsValid() ? Source : FGameplayTag(GrimProgressionTags::XPSource_Debug));
#endif
}

void UGrimLevelComponent::Server_DebugSetLevel_Implementation(int32 NewLevel)
{
#if !UE_BUILD_SHIPPING
	SetLevel(NewLevel);
#endif
}

void UGrimLevelComponent::RefreshLevel()
{
	const int32 OldLevel = Level;
	Level = FGrimLevelMath::GetLevelForXP(UGrimLevelSettings::GetCurve(), TotalXP, UGrimLevelSettings::Get()->LevelCap);
	ApplyLevelBonus();
	if (Level != OldLevel)
	{
		UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 레벨 %d → %d (포인트 %d, 최대 HP +%.0f, 최대 마나 +%.0f)"), *GetNameSafe(GetOwner()), OldLevel, Level,
			GetTotalSkillPoints(), GetMaxHealthBonus(), GetMaxManaBonus());
		OnLevelChanged.Broadcast(this, Level, OldLevel);
	}
}

void UGrimLevelComponent::ApplyLevelBonus()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (AppliedBonusLevel == Level && (LevelBonusHandle.IsValid() || Level <= 1))
	{
		return;
	}
	FGrimProgressionBonus Bonus;
	Bonus.MaxHealth = GetMaxHealthBonus();
	Bonus.MaxMana = GetMaxManaBonus();
	if (FGrimProgressionEffects::ApplyBonus(GetASC(), Bonus, LevelBonusHandle))
	{
		AppliedBonusLevel = Level;
	}
}

void UGrimLevelComponent::OnRep_Level(int32 OldLevel)
{
	if (OldLevel != Level)
	{
		OnLevelChanged.Broadcast(this, Level, OldLevel);
	}
}

// ───────────────────────── 경험 출처 ─────────────────────────

void UGrimLevelComponent::HandleLearnResult(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, EGrimLearnResult Result)
{
	// 서버에서만(클라이언트의 같은 알림은 Client RPC 에서 온다). 새로 배운 것만 — 숙련도 경험으로 쓴 책은 숙련도가 보상이다.
	if (Result == EGrimLearnResult::Learned && GetOwner()->HasAuthority())
	{
		AwardDefaultExperience(GrimProgressionTags::XPSource_Skillbook);
	}
}

void UGrimLevelComponent::HandleCircleUpResult(UGrimProgressionComponent* Progression, int32 Circle, bool bSuccess)
{
	if (bSuccess && GetOwner()->HasAuthority())
	{
		// 서클마다 한 번(강등 뒤 다시 올라도 또 주지 않는다)
		AwardDiscovery(GrimProgressionTags::XPSource_CircleUp, FName(*FString::Printf(TEXT("Circle%d"), Circle)));
	}
}

void UGrimLevelComponent::CheckRegion()
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	const UWorld* World = GetWorld();
	const UGrimWeatherSubsystem* Weather = World ? World->GetSubsystem<UGrimWeatherSubsystem>() : nullptr;
	if (!Pawn || !Weather)
	{
		return;
	}
	const FName Region = Weather->GetRegionAt(Pawn->GetActorLocation());
	if (Region.IsNone())
	{
		return;
	}
	if (!bRegionSeeded)
	{
		bRegionSeeded = true;
		if (!UGrimLevelSettings::Get()->bAwardStartingRegion)
		{
			// 처음 서 있던 곳은 조용히 「발견」만.
			Discoveries.Add(MakeDiscoveryKey(GrimProgressionTags::XPSource_RegionDiscovery, Region));
			return;
		}
	}
	AwardDiscovery(GrimProgressionTags::XPSource_RegionDiscovery, Region);
}

// ───────────────────────── 세이브 ─────────────────────────

void UGrimLevelComponent::WriteSaveData(FArchive& Ar)
{
	float SavedXP = TotalXP;
	int32 SavedBonus = BonusSkillPoints;
	TArray<FString> SavedDiscoveries = Discoveries.Array();
	SavedDiscoveries.Sort();
	Ar << SavedXP;
	Ar << SavedBonus;
	Ar << SavedDiscoveries;
}

void UGrimLevelComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	float SavedXP = 0.f;
	int32 SavedBonus = 0;
	TArray<FString> SavedDiscoveries;
	Ar << SavedXP;
	Ar << SavedBonus;
	Ar << SavedDiscoveries;
	if (Ar.IsError())
	{
		UE_LOG(LogGrimProgression, Error, TEXT("레벨 세이브(v%d)를 읽지 못했다"), Version);
		return;
	}
	BonusSkillPoints = FMath::Max(SavedBonus, 0);
	// 지금 세션에서 이미 발견한 것(시작 지역 등)과 합친다.
	Discoveries.Append(SavedDiscoveries);
	RepeatTracker.Reset();
	SetTotalXP(SavedXP);
}
