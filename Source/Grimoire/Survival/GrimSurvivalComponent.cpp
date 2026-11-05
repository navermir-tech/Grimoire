#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimBed.h"
#include "Survival/GrimSleepSubsystem.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Core/Cheats/GrimCheatTags.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Core/GrimTargetingTypes.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "AbilitySystemComponent.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarGrimSurvivalShowDebug(
	TEXT("grim.Survival.ShowDebug"),
	1,
	TEXT("생존 · 시간 디버그 표시(1 = 켬). 정식 HUD 가 생기면 기본값을 0 으로 바꾼다."));

namespace GrimSurvivalComponentPrivate
{
	/** MaxWalkSpeed 에 곱하는 배율의 바닥 — 0 이면 비율로 되돌릴 수 없다. */
	constexpr float MinAppliedMoveSpeedScale = 0.01f;

	/** 서버가 받아 주는 침대 거리 여유(cm) — 클라이언트 위치가 조금 앞서 있어도 거절하지 않게. */
	constexpr float BedReachSlack = 100.f;

	bool HasExplicitTag(const UAbilitySystemComponent* ASC, const FGameplayTag& Tag)
	{
		return ASC && ASC->GetOwnedGameplayTags().HasTagExact(Tag);
	}
}

UGrimSurvivalComponent::UGrimSurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimSurvivalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimSurvivalComponent, ClimateExposure, COND_OwnerOnly);
	DOREPLIFETIME(UGrimSurvivalComponent, DownedEndServerTime);
}

void UGrimSurvivalComponent::OnRegister()
{
	Super::OnRegister();

	// BeginPlay 보다 먼저 묶는다 — 폰 빙의가 PlayerState BeginPlay 보다 빠를 수도 있다.
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		if (!ActorInfoReadyHandle.IsValid())
		{
			ActorInfoReadyHandle = PlayerState->OnAbilityActorInfoReady.AddUObject(this, &ThisClass::HandleAbilityActorInfoReady);
		}
	}
}

void UGrimSurvivalComponent::OnUnregister()
{
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		PlayerState->OnAbilityActorInfoReady.Remove(ActorInfoReadyHandle);
	}
	ActorInfoReadyHandle.Reset();
	Super::OnUnregister();
}

void UGrimSurvivalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_DedicatedServer)
	{
		DebugDrawHandle = UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateUObject(this, &ThisClass::DrawDebug));
	}

	// 이미 폰과 연결된 뒤에 시작했다면 바로 처리한다.
	if (APawn* Pawn = GetAvatarPawn())
	{
		HandleAbilityActorInfoReady(Cast<AGrimPlayerState>(GetOwner()), Pawn);
	}
}

void UGrimSurvivalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DebugDrawHandle.IsValid())
	{
		UDebugDrawService::Unregister(DebugDrawHandle);
		DebugDrawHandle.Reset();
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SurvivalTickTimer);
		World->GetTimerManager().ClearTimer(DownedTimer);
		World->GetTimerManager().ClearTimer(CollapseTimer);
	}
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()).Remove(MoveSpeedChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UGrimSurvivalComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

APawn* UGrimSurvivalComponent::GetAvatarPawn() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr;
}

// ── 읽기 ──

float UGrimSurvivalComponent::GetAmbientTemperature() const
{
	const UWorld* World = GetWorld();
	const UGrimWeatherSubsystem* Weather = World ? World->GetSubsystem<UGrimWeatherSubsystem>() : nullptr;
	if (!Weather)
	{
		return 0.f;
	}
	const APawn* Pawn = GetAvatarPawn();
	return Weather->GetAmbientTemperature(Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector);
}

FGrimClimateExposure UGrimSurvivalComponent::GetClimateDeficit() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return FGrimClimateExposure();
	}
	return FGrimSurvivalModel::ComputeDeficit(ClimateExposure,
		ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHeatResistAttribute()),
		ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()));
}

bool UGrimSurvivalComponent::IsSleeping() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sleeping);
}

bool UGrimSurvivalComponent::IsDowned() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Downed);
}

float UGrimSurvivalComponent::GetDownedSecondsRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World || DownedEndServerTime <= 0.f || !IsDowned())
	{
		return 0.f;
	}
	const AGameStateBase* GameState = World->GetGameState();
	const double Now = GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return FMath::Max(0.f, DownedEndServerTime - static_cast<float>(Now));
}

bool UGrimSurvivalComponent::GetRespawnPoint(FTransform& OutTransform) const
{
	if (bHasRespawnPoint)
	{
		OutTransform = RespawnTransform;
	}
	return bHasRespawnPoint;
}

// ── 시작 ──

void UGrimSurvivalComponent::HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn)
{
	if (!GetOwner() || !HasBegunPlay())
	{
		return;
	}
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !Pawn)
	{
		return;
	}

	// 서버 · 클라이언트 모두: 이동 배율을 폰에 반영(소유 클라이언트도 같은 속도로 예측해야 보정이 튀지 않는다).
	if (!MoveSpeedChangedHandle.IsValid())
	{
		MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute())
			.AddUObject(this, &ThisClass::HandleMoveSpeedScaleChanged);
	}
	ApplyMoveSpeedScale();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// PlayerState 는 폰이 바뀌어도 유지된다 — 초기값은 처음 한 번만.
	if (!bAttributesInitialized)
	{
		InitializeAttributes(ASC);
	}
	if (!HealthChangedHandle.IsValid())
	{
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::HandleHealthChanged);
	}

	UWorld* World = GetWorld();
	if (!World->GetTimerManager().IsTimerActive(SurvivalTickTimer))
	{
		LastTickWorldSeconds = World->GetTimeSeconds();
		const float Interval = FMath::Max(0.1f, GetDefault<UGrimSurvivalSettings>()->Tuning.TickIntervalSeconds);
		World->GetTimerManager().SetTimer(SurvivalTickTimer, this, &ThisClass::SurvivalTick, Interval, true);
	}
}

void UGrimSurvivalComponent::InitializeAttributes(UAbilitySystemComponent* ASC)
{
	const UGrimSurvivalSettings* Settings = GetDefault<UGrimSurvivalSettings>();
	const FGrimSurvivalTuning& Tuning = Settings->Tuning;

	// MaxHealth 를 먼저 — Health 클램프가 MaxHealth 를 본다.
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetMaxHealthAttribute(), Tuning.InitialMaxHealth);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), Tuning.InitialMaxHealth);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), Tuning.InitialHunger);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), Tuning.InitialBodyTemperature);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetFatigueAttribute(), 0.f);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetDiseaseResistAttribute(), Settings->Disease.InitialDiseaseResist);
	bAttributesInitialized = true;
}

// ── 주기 효과 ──

void UGrimSurvivalComponent::SurvivalTick()
{
	using namespace GrimSurvivalComponentPrivate;

	UAbilitySystemComponent* ASC = GetASC();
	APawn* Pawn = GetAvatarPawn();
	UWorld* World = GetWorld();
	if (!ASC || !Pawn || !World)
	{
		return;
	}

	const FGrimSurvivalRules Rules = UGrimSurvivalSettings::GetRules();
	const double Now = World->GetTimeSeconds();
	const float DeltaSeconds = static_cast<float>(Now - LastTickWorldSeconds);
	LastTickWorldSeconds = Now;

	// 슈퍼치트: 체온 등급 · 피곤함 · 질병 · 쓰러짐도 막는다(HP · 마나 · 배고픔 · 체온 값은 치트 GE 가 고정).
	const bool bCheat = ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super);
	if (bCheat && IsDowned())
	{
		Revive();
	}

	ClimateExposure = GatherClimateExposure(Pawn, bCheat);

	const UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>();

	FGrimSurvivalInputs Inputs;
	Inputs.Health = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute());
	Inputs.MaxHealth = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute());
	Inputs.Hunger = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute());
	Inputs.BodyTemperature = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetBodyTemperatureAttribute());
	Inputs.Fatigue = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetFatigueAttribute());
	Inputs.Mana = ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute());
	Inputs.MaxMana = ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute());
	Inputs.Exposure = ClimateExposure;
	Inputs.HeatResist = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHeatResistAttribute());
	Inputs.ColdResist = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute());
	Inputs.bNight = TimeOfDay && TimeOfDay->IsNight();
	Inputs.bSleeping = IsSleeping();
	Inputs.bDowned = IsDowned();
	Inputs.bStomachSick = !bCheat && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick_Stomach);
	Inputs.bColdSick = !bCheat && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick_Cold);

	FGrimSurvivalDeltas Deltas = FGrimSurvivalModel::ComputeTick(Inputs, DeltaSeconds, Rules);
	if (bCheat)
	{
		Deltas.Fatigue = -Inputs.Fatigue;
		Deltas.Mana = 0.f;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimSurvivalTickEffect::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HealthMagnitudeName, Deltas.Health);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HungerMagnitudeName, Deltas.Hunger);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::BodyTemperatureMagnitudeName, Deltas.BodyTemperature);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	if (Deltas.Fatigue != 0.f || Deltas.Mana != 0.f)
	{
		const FGameplayEffectSpecHandle Extra = ASC->MakeOutgoingSpec(UGrimSurvivalExtraTickEffect::StaticClass(), 1.f, Context);
		if (Extra.IsValid())
		{
			Extra.Data->SetSetByCallerMagnitude(UGrimSurvivalExtraTickEffect::FatigueMagnitudeName, Deltas.Fatigue);
			Extra.Data->SetSetByCallerMagnitude(UGrimSurvivalExtraTickEffect::ManaMagnitudeName, Deltas.Mana);
			ASC->ApplyGameplayEffectSpecToSelf(*Extra.Data.Get());
		}
	}

	// 상태는 적용 뒤 값으로.
	const FGrimClimateExposure Deficit = FGrimSurvivalModel::ComputeDeficit(ClimateExposure, Inputs.HeatResist, Inputs.ColdResist);
	const float Fatigue = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetFatigueAttribute());
	LastStatus = FGrimSurvivalModel::EvaluateStatus(
		ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute()), Deficit, Fatigue, Rules);
	UpdateStatusTags(ASC, LastStatus);

	// 질병: 젖은 채 추우면(추위 모자람 1 이상) 주기마다 감기 굴림.
	if (bCheat)
	{
		CureAllDiseases();
		WetColdSeconds = 0.f;
	}
	else
	{
		const bool bWetAndCold = UGrimElementalStatics::GetStateIntensity(Pawn, GrimTags::State_Wet) > 0.f && Deficit.Cold >= 1;
		const int32 Rolls = FGrimSurvivalModel::AccumulateWetCold(WetColdSeconds, DeltaSeconds, bWetAndCold, Rules.Disease);
		for (int32 i = 0; i < Rolls; ++i)
		{
			RollDisease(EGrimDisease::Cold, Rules.Disease.WetColdChance);
		}
	}

	// 피곤함 바닥 → 기절(강제 수면).
	const bool bCollapsed = ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Collapsed);
	if (LastStatus.bCollapse && !bCollapsed && !bCheat && !IsDowned())
	{
		StopSleeping();
		SetLooseTag(ASC, GrimSurvivalTags::Status_Collapsed, true);
		StartSleeping(nullptr, true);
		World->GetTimerManager().SetTimer(CollapseTimer, this, &ThisClass::HandleCollapseFinished, FMath::Max(0.1f, Rules.Fatigue.CollapseSleepSeconds), false);
		UE_LOG(LogGrimSurvival, Log, TEXT("%s 기절 — %.0f초 강제 수면"), *GetNameSafe(GetOwner()), Rules.Fatigue.CollapseSleepSeconds);
	}
	else if (IsSleeping() && !bCollapsed)
	{
		if (HasHostileNearby(Pawn))
		{
			// 전투 중 회피 금지 — 적이 다가오면 깬다.
			StopSleeping();
			SendSleepResult(EGrimSleepResult::HostileNearby);
		}
		else if (!Inputs.bNight && Fatigue <= 0.f)
		{
			// 낮잠은 피곤함이 다 풀리면 일어난다.
			StopSleeping();
			SendSleepResult(EGrimSleepResult::Woke);
		}
	}

	if (!IsDowned() && !bCheat && ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()) <= 0.f)
	{
		EnterDowned();
	}

	RefreshPenalty();
}

FGrimClimateExposure UGrimSurvivalComponent::GatherClimateExposure(const APawn* Pawn, bool bCheat) const
{
	if (bCheat || !Pawn)
	{
		return FGrimClimateExposure();
	}

	FGrimClimateExposure World;
	if (const UGrimWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UGrimWeatherSubsystem>())
	{
		// 지붕 있는 실내면 날씨 몫을 빼고 지역 몫만(Sheltered) 쓴다.
		const FGrimClimateExposureBreakdown Breakdown = Weather->GetClimateExposureBreakdown(Pawn->GetActorLocation());
		World = Weather->IsActorSheltered(Pawn) ? Breakdown.Sheltered : Breakdown.Total;
	}

	const UAbilitySystemComponent* ASC = GetASC();
	const ACharacter* Character = Cast<ACharacter>(Pawn);

	FGrimPersonalClimate Personal;
	Personal.bWet = UGrimElementalStatics::GetStateIntensity(Pawn, GrimTags::State_Wet) > 0.f;
	Personal.bBurning = UGrimElementalStatics::GetStateIntensity(Pawn, GrimTags::State_Burning) > 0.f;
	Personal.bNearHeatSource = HasHeatSourceNearby(Pawn);
	Personal.bSwimming = (ASC && ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Swimming))
		|| (Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsSwimming());

	const FGrimClimateTuning& Climate = GetDefault<UGrimSurvivalSettings>()->Climate;
	return FGrimSurvivalModel::CombineExposure(World, FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate));
}

bool UGrimSurvivalComponent::HasHeatSourceNearby(const APawn* Pawn) const
{
	const float Radius = GetDefault<UGrimSurvivalSettings>()->Tuning.HeatSourceRadius;
	if (Radius <= 0.f)
	{
		return false;
	}

	// 열원은 질의 콜리전이 있는 액터 중 State.Burning 세기가 있는 것(양초 · 모닥불 · 불붙은 풀).
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSurvivalHeatSources), false, Pawn);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(Radius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		if (Actor && Actor != Pawn && UGrimElementalStatics::GetStateIntensity(Actor, GrimTags::State_Burning) > 0.f)
		{
			return true;
		}
	}
	return false;
}

bool UGrimSurvivalComponent::HasHostileNearby(const APawn* Pawn) const
{
	const float Radius = GetDefault<UGrimSurvivalSettings>()->Sleep.HostileBlockRadius;
	if (!Pawn || Radius <= 0.f)
	{
		return false;
	}

	// 지금 적대 = IGrimTargetable 종류 Enemy(생물 · 사냥꾼 AI 가 생기면 그쪽 판정으로 넓힌다).
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSurvivalHostiles), false, Pawn);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(Radius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == Pawn)
		{
			continue;
		}
		const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Actor);
		if (Targetable && Targetable->GetTargetKind() == EGrimTargetKind::Enemy && Targetable->CanBeTargetedBy(Pawn))
		{
			return true;
		}
	}
	return false;
}

AGrimBed* UGrimSurvivalComponent::FindNearestBed(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return nullptr;
	}
	AGrimBed* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGrimBed> It(GetWorld()); It; ++It)
	{
		if (It->IsWithinReach(Pawn, GrimSurvivalComponentPrivate::BedReachSlack))
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
	}
	return Best;
}

void UGrimSurvivalComponent::SetLooseTag(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, bool bOn)
{
	// 복제 루스 태그 — 모든 클라이언트가 본다(마나 회복 정지 · 연출 등은 태그로 판단). 하위 태그가 따로 있어 정확히 비교한다.
	if (ASC && GrimSurvivalComponentPrivate::HasExplicitTag(ASC, Tag) != bOn)
	{
		ASC->SetLooseGameplayTagCount(Tag, bOn ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	}
}

void UGrimSurvivalComponent::UpdateStatusTags(UAbilitySystemComponent* ASC, const FGrimSurvivalStatus& Status)
{
	SetLooseTag(ASC, GrimSurvivalTags::Status_Hungry, Status.bHungry);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Starving, Status.bStarving);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Chilly, Status.bChilly);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Hypothermia, Status.bHypothermia);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Hypothermia_Severe, Status.bHypothermiaSevere);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Warm, Status.bWarm);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Hyperthermia, Status.bHyperthermia);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Hyperthermia_Severe, Status.bHyperthermiaSevere);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Exhausted, Status.bExhausted);
}

void UGrimSurvivalComponent::RefreshPenalty()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}
	const FGrimSurvivalRules Rules = UGrimSurvivalSettings::GetRules();
	UpdatePenaltyEffect(ASC, FGrimSurvivalModel::ComputeModifiers(LastStatus, IsSleeping(), IsDowned(), Rules));
}

void UGrimSurvivalComponent::UpdatePenaltyEffect(UAbilitySystemComponent* ASC, const FGrimSurvivalModifiers& Modifiers)
{
	if (Modifiers.Equals(AppliedModifiers) && (PenaltyHandle.IsValid() || Modifiers.IsIdentity()))
	{
		return;
	}
	if (PenaltyHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(PenaltyHandle);
		PenaltyHandle.Invalidate();
	}
	AppliedModifiers = Modifiers;
	if (Modifiers.IsIdentity())
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimSurvivalPenaltyEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::MoveSpeedMagnitudeName, Modifiers.MoveSpeed);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::CastSpeedMagnitudeName, Modifiers.CastSpeed);
		Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::ManaCostMagnitudeName, Modifiers.ManaCost);
		PenaltyHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

// ── 이동 배율 ──

void UGrimSurvivalComponent::HandleMoveSpeedScaleChanged(const FOnAttributeChangeData& Data)
{
	ApplyMoveSpeedScale();
}

void UGrimSurvivalComponent::ApplyMoveSpeedScale()
{
	using namespace GrimSurvivalComponentPrivate;

	const UAbilitySystemComponent* ASC = GetASC();
	ACharacter* Character = Cast<ACharacter>(GetAvatarPawn());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!ASC || !Movement)
	{
		return;
	}
	// 서버와 소유 클라이언트만(시뮬레이션 프록시는 위치를 받는다).
	if (!Character->HasAuthority() && !Character->IsLocallyControlled())
	{
		return;
	}

	if (SpeedScaledPawn.Get() != Character)
	{
		SpeedScaledPawn = Character;
		AppliedMoveSpeedScale = 1.f;
	}
	const float Scale = FMath::Max(MinAppliedMoveSpeedScale, ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()));
	if (!FMath::IsNearlyEqual(Scale, AppliedMoveSpeedScale))
	{
		// 절대값이 아니라 비율로 — 줌 조준 등 다른 감속도 같은 방식이면 서로 덮어쓰지 않는다.
		Movement->MaxWalkSpeed *= Scale / AppliedMoveSpeedScale;
		AppliedMoveSpeedScale = Scale;
	}
}

// ── 잠 ──

void UGrimSurvivalComponent::NotifySleepSubsystem()
{
	if (UGrimSleepSubsystem* Sleep = GetWorld() ? GetWorld()->GetSubsystem<UGrimSleepSubsystem>() : nullptr)
	{
		Sleep->NotifySleepStateChanged();
	}
}

EGrimSleepResult UGrimSurvivalComponent::StartSleeping(AGrimBed* Bed, bool bForced)
{
	UAbilitySystemComponent* ASC = GetASC();
	APawn* Pawn = GetAvatarPawn();
	if (!ASC || !Pawn || !GetOwner()->HasAuthority())
	{
		return EGrimSleepResult::NotAllowed;
	}
	if (IsDowned())
	{
		return EGrimSleepResult::Downed;
	}
	if (IsSleeping())
	{
		return EGrimSleepResult::AlreadySleeping;
	}
	if (!bForced)
	{
		if (Bed && !Bed->IsWithinReach(Pawn, GrimSurvivalComponentPrivate::BedReachSlack))
		{
			return EGrimSleepResult::TooFar;
		}
		if (HasHostileNearby(Pawn))
		{
			return EGrimSleepResult::HostileNearby;
		}
	}

	SetLooseTag(ASC, GrimSurvivalTags::Status_Sleeping, true);
	ASC->CancelAllAbilities();
	CurrentBed = Bed;
	if (Bed)
	{
		bHasRespawnPoint = true;
		RespawnTransform = Bed->GetRespawnTransform();
	}
	UE_LOG(LogGrimSurvival, Log, TEXT("%s 잠듦(%s)"), *GetNameSafe(GetOwner()), Bed ? *Bed->GetName() : TEXT("그 자리"));

	RefreshPenalty();
	NotifySleepSubsystem();
	return EGrimSleepResult::Started;
}

void UGrimSurvivalComponent::StopSleeping()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || !IsSleeping())
	{
		return;
	}
	SetLooseTag(ASC, GrimSurvivalTags::Status_Sleeping, false);
	SetLooseTag(ASC, GrimSurvivalTags::Status_Collapsed, false);
	GetWorld()->GetTimerManager().ClearTimer(CollapseTimer);
	CurrentBed.Reset();
	UE_LOG(LogGrimSurvival, Log, TEXT("%s 일어남"), *GetNameSafe(GetOwner()));

	RefreshPenalty();
	NotifySleepSubsystem();
}

void UGrimSurvivalComponent::HandleCollapseFinished()
{
	StopSleeping();
	SetLooseTag(GetASC(), GrimSurvivalTags::Status_Collapsed, false);
}

void UGrimSurvivalComponent::ApplyNightSkip(float SkippedDays, float DayLengthSeconds)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}
	const FGrimSurvivalRules Rules = UGrimSurvivalSettings::GetRules();
	const float HungerLoss = FGrimSurvivalModel::ComputeSkippedHungerLoss(SkippedDays, DayLengthSeconds, Rules);
	const float Hunger = ASC->GetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute());
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), Hunger - HungerLoss);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetFatigueAttribute(), 0.f);
	StopSleeping();
	UE_LOG(LogGrimSurvival, Log, TEXT("%s 밤을 넘김 — 배고픔 −%.1f, 피곤함 0"), *GetNameSafe(GetOwner()), HungerLoss);
}

void UGrimSurvivalComponent::NotifyNightSkipFade(float FadeSeconds)
{
	ClientNightSkipFade(FadeSeconds);
}

void UGrimSurvivalComponent::ClientNightSkipFade_Implementation(float FadeSeconds)
{
	OnNightSkipFade.Broadcast(FadeSeconds);
}

void UGrimSurvivalComponent::SendSleepResult(EGrimSleepResult Result)
{
	ClientSleepResult(Result);
}

void UGrimSurvivalComponent::ClientSleepResult_Implementation(EGrimSleepResult Result)
{
	UE_LOG(LogGrimSurvival, Log, TEXT("잠 요청 결과: %s"), *UEnum::GetValueAsString(Result));
	OnSleepResult.Broadcast(Result);
}

void UGrimSurvivalComponent::ServerRequestSleep_Implementation(AGrimBed* Bed)
{
	if (!Bed)
	{
		Bed = FindNearestBed(GetAvatarPawn());
	}
#if UE_BUILD_SHIPPING
	if (!Bed)
	{
		SendSleepResult(EGrimSleepResult::TooFar);
		return;
	}
#endif
	// 개발 빌드는 침대 없이도 그 자리에서 잘 수 있다(콘솔 grim.Survival.Sleep). 부활 지점은 바뀌지 않는다.
	SendSleepResult(StartSleeping(Bed));
}

void UGrimSurvivalComponent::ServerRequestWake_Implementation()
{
	if (const UAbilitySystemComponent* ASC = GetASC(); ASC && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Collapsed))
	{
		SendSleepResult(EGrimSleepResult::NotAllowed);
		return;
	}
	if (IsSleeping())
	{
		StopSleeping();
		SendSleepResult(EGrimSleepResult::Woke);
	}
}

// ── 질병 · 저항 ──

void UGrimSurvivalComponent::NotifyFoodEaten(bool bRaw)
{
	if (bRaw)
	{
		RollDisease(EGrimDisease::Stomach, GetDefault<UGrimSurvivalSettings>()->Disease.RawFoodStomachChance);
	}
}

void UGrimSurvivalComponent::RollDisease(EGrimDisease Disease, float BaseChance)
{
	const UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}
	const float Chance = FGrimSurvivalModel::ComputeDiseaseChance(BaseChance, ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetDiseaseResistAttribute()));
	if (FMath::FRand() < Chance)
	{
		ApplyDisease(Disease);
	}
}

void UGrimSurvivalComponent::ApplyDisease(EGrimDisease Disease)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super))
	{
		return;
	}
	const FGrimDiseaseTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Disease;
	const bool bStomach = Disease == EGrimDisease::Stomach;
	const FGameplayTag Tag = bStomach ? GrimSurvivalTags::Status_Sick_Stomach : GrimSurvivalTags::Status_Sick_Cold;
	if (ASC->HasMatchingGameplayTag(Tag))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> EffectClass = bStomach
		? TSubclassOf<UGameplayEffect>(UGrimDiseaseStomachEffect::StaticClass())
		: TSubclassOf<UGameplayEffect>(UGrimDiseaseColdEffect::StaticClass());
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		const float Duration = bStomach ? Tuning.StomachDurationSeconds : Tuning.ColdDurationSeconds;
		Spec.Data->SetSetByCallerMagnitude(UGrimDiseaseEffect::DurationMagnitudeName, Duration);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		UE_LOG(LogGrimSurvival, Log, TEXT("%s 질병 %s — %.0f초"), *GetNameSafe(GetOwner()), *Tag.ToString(), Duration);
	}
}

void UGrimSurvivalComponent::CureAllDiseases()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (ASC && GetOwner()->HasAuthority() && ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick))
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GrimSurvivalTags::Status_Sick));
	}
}

void UGrimSurvivalComponent::SetTestClimateResist(float Heat, float Cold)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (TestResistHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(TestResistHandle);
		TestResistHandle.Invalidate();
	}
	if (Heat == 0.f && Cold == 0.f)
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimClimateResistEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(UGrimClimateResistEffect::HeatMagnitudeName, Heat);
		Spec.Data->SetSetByCallerMagnitude(UGrimClimateResistEffect::ColdMagnitudeName, Cold);
		TestResistHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UGrimSurvivalComponent::SetFatigue(float Value)
{
	if (UAbilitySystemComponent* ASC = GetASC(); ASC && GetOwner()->HasAuthority())
	{
		ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetFatigueAttribute(), Value);
	}
}

// ── 쓰러짐 · 죽음 · 부활 ──

void UGrimSurvivalComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// GE 적용 중에 태그 · GE 를 더 붙이지 않도록 다음 틱으로 미룬다.
	if (Data.NewValue <= 0.f && !IsDowned() && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::EnterDowned);
	}
}

void UGrimSurvivalComponent::ForceDown()
{
	if (UAbilitySystemComponent* ASC = GetASC(); ASC && GetOwner()->HasAuthority())
	{
		ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), 0.f);
	}
}

void UGrimSurvivalComponent::EnterDowned()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || IsDowned() || ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super)
		|| ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()) > 0.f)
	{
		return;
	}

	StopSleeping();
	SetLooseTag(ASC, GrimSurvivalTags::Status_Downed, true);
	ASC->CancelAllAbilities();

	UWorld* World = GetWorld();
	const float Duration = GetDefault<UGrimSurvivalSettings>()->Downed.DownedSeconds;
	const AGameStateBase* GameState = World->GetGameState();
	DownedEndServerTime = static_cast<float>(GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds()) + Duration;
	World->GetTimerManager().SetTimer(DownedTimer, this, &ThisClass::HandleDownedExpired, FMath::Max(0.1f, Duration), false);
	UE_LOG(LogGrimSurvival, Log, TEXT("%s 쓰러짐 — %.0f초 안에 일으키지 않으면 사망"), *GetNameSafe(GetOwner()), Duration);

	RefreshPenalty();
	OnDowned.Broadcast();
}

void UGrimSurvivalComponent::Revive()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || !IsDowned())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(DownedTimer);
	DownedEndServerTime = 0.f;
	SetLooseTag(ASC, GrimSurvivalTags::Status_Downed, false);

	const float MaxHealth = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), MaxHealth * GetDefault<UGrimSurvivalSettings>()->Downed.ReviveHealthFraction);
	UE_LOG(LogGrimSurvival, Log, TEXT("%s 일으켜짐"), *GetNameSafe(GetOwner()));
	RefreshPenalty();
}

void UGrimSurvivalComponent::HandleDownedExpired()
{
	DieAndRespawn();
}

void UGrimSurvivalComponent::DieAndRespawn()
{
	UAbilitySystemComponent* ASC = GetASC();
	APawn* Pawn = GetAvatarPawn();
	AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	UWorld* World = GetWorld();
	if (!ASC || !Pawn || !PlayerState || !World || !PlayerState->HasAuthority())
	{
		return;
	}

	UE_LOG(LogGrimSurvival, Log, TEXT("%s 사망"), *GetNameSafe(GetOwner()));
	// 가방 일부 떨굼은 인벤토리(M4) 뒤 — OnDied 에 붙인다.
	OnDied.Broadcast();

	// 부활 지점: 마지막 침대 → 없으면 PlayerStart → 그것도 없으면 제자리.
	FTransform Target = Pawn->GetActorTransform();
	if (!GetRespawnPoint(Target))
	{
		AController* Controller = Pawn->GetController();
		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (AActor* Start = (GameMode && Controller) ? GameMode->FindPlayerStart(Controller) : nullptr)
		{
			Target = Start->GetActorTransform();
		}
	}

	// 폰을 새로 만들지 않고 옮긴다(PlayerState · 마법서 · 장비 유지).
	const FRotator Rotation(0.f, Target.Rotator().Yaw, 0.f);
	Pawn->TeleportTo(Target.GetLocation(), Rotation, false, false);
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
	{
		PlayerController->SetControlRotation(Rotation);
		PlayerController->ClientSetRotation(Rotation);
	}

	GetWorld()->GetTimerManager().ClearTimer(DownedTimer);
	DownedEndServerTime = 0.f;
	SetLooseTag(ASC, GrimSurvivalTags::Status_Downed, false);

	const UGrimSurvivalSettings* Settings = GetDefault<UGrimSurvivalSettings>();
	const float MaxHealth = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), MaxHealth * Settings->Downed.RespawnHealthFraction);
	const float Hunger = ASC->GetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute());
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), FMath::Max(Hunger, Settings->Downed.RespawnMinHunger));
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), Settings->Tuning.NormalBodyTemperature);

	UE_LOG(LogGrimSurvival, Log, TEXT("%s 부활 — %s"), *GetNameSafe(GetOwner()), bHasRespawnPoint ? TEXT("마지막 침대") : TEXT("시작 지점"));
	RefreshPenalty();
	OnRespawned.Broadcast();
}

// ── 콘솔(개발 빌드) ──

void UGrimSurvivalComponent::ServerDebugCommand_Implementation(const FString& Command, float A, float B)
{
#if !UE_BUILD_SHIPPING
	if (Command == TEXT("Sleep"))
	{
		ServerRequestSleep_Implementation(nullptr);
	}
	else if (Command == TEXT("Wake"))
	{
		ServerRequestWake_Implementation();
	}
	else if (Command == TEXT("Revive"))
	{
		Revive();
	}
	else if (Command == TEXT("Down"))
	{
		ForceDown();
	}
	else if (Command == TEXT("Die"))
	{
		if (!IsDowned())
		{
			ForceDown();
			EnterDowned();
		}
		DieAndRespawn();
	}
	else if (Command == TEXT("Fatigue"))
	{
		SetFatigue(A);
	}
	else if (Command == TEXT("EatRaw"))
	{
		NotifyFoodEaten(true);
	}
	else if (Command == TEXT("Sick"))
	{
		ApplyDisease(A >= 1.f ? EGrimDisease::Cold : EGrimDisease::Stomach);
	}
	else if (Command == TEXT("Cure"))
	{
		CureAllDiseases();
	}
	else if (Command == TEXT("Resist"))
	{
		SetTestClimateResist(A, B);
	}
	else if (Command == TEXT("SpawnBed"))
	{
		// 시험용: 폰 앞 1.5m 에 침대(A ≥ 1 이면 침낭). 맵 배치는 통합 때.
		if (const APawn* Pawn = GetAvatarPawn())
		{
			const FRotator Yaw(0.f, Pawn->GetActorRotation().Yaw, 0.f);
			const FVector Location = Pawn->GetActorLocation() + Yaw.Vector() * 150.f - FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight());
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.bDeferConstruction = true;
			if (AGrimBed* Bed = GetWorld()->SpawnActor<AGrimBed>(AGrimBed::StaticClass(), Location, Yaw, Params))
			{
				Bed->SetBedroll(A >= 1.f);
				Bed->FinishSpawning(FTransform(Yaw, Location));
				UE_LOG(LogGrimSurvival, Log, TEXT("침대 놓음: %s at %s"), *Bed->GetName(), *Location.ToCompactString());
			}
		}
	}
	else
	{
		UE_LOG(LogGrimSurvival, Warning, TEXT("알 수 없는 생존 콘솔 명령: %s"), *Command);
	}
#endif
}

// ── 디버그 표시 ──

void UGrimSurvivalComponent::DrawDebug(UCanvas* Canvas, APlayerController* PlayerController)
{
	if (!Canvas || !PlayerController || CVarGrimSurvivalShowDebug.GetValueOnGameThread() == 0)
	{
		return;
	}
	// 화면의 주인 플레이어 것만 그린다.
	if (!PlayerController->IsLocalController() || PlayerController->PlayerState != GetOwner())
	{
		return;
	}
	const UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	FString Time = TEXT("-");
	if (const UGrimTimeOfDaySubsystem* TimeOfDay = GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>())
	{
		const float Hours = TimeOfDay->GetHours();
		const int32 Hour = FMath::FloorToInt32(Hours);
		const int32 Minute = FMath::FloorToInt32((Hours - Hour) * 60.f);
		Time = FString::Printf(TEXT("Day %d  %02d:%02d  %s"), TimeOfDay->GetDayIndex() + 1, Hour, Minute, TimeOfDay->IsNight() ? TEXT("Night") : TEXT("Day"));
	}

	FString Statuses;
	const TPair<FGameplayTag, const TCHAR*> StatusLabels[] = {
		{ GrimSurvivalTags::Status_Hungry, TEXT("Hungry") },
		{ GrimSurvivalTags::Status_Starving, TEXT("Starving") },
		{ GrimSurvivalTags::Status_Chilly, TEXT("Chilly") },
		{ GrimSurvivalTags::Status_Hypothermia, TEXT("Hypothermia") },
		{ GrimSurvivalTags::Status_Hypothermia_Severe, TEXT("Severe") },
		{ GrimSurvivalTags::Status_Warm, TEXT("Warm") },
		{ GrimSurvivalTags::Status_Hyperthermia, TEXT("Hyperthermia") },
		{ GrimSurvivalTags::Status_Hyperthermia_Severe, TEXT("Heatstroke") },
		{ GrimSurvivalTags::Status_Exhausted, TEXT("Exhausted") },
		{ GrimSurvivalTags::Status_Collapsed, TEXT("Collapsed") },
		{ GrimSurvivalTags::Status_Sleeping, TEXT("Sleeping") },
		{ GrimSurvivalTags::Status_Downed, TEXT("Downed") },
		{ GrimSurvivalTags::Status_Sick_Stomach, TEXT("Stomachache") },
		{ GrimSurvivalTags::Status_Sick_Cold, TEXT("Cold") },
		{ GrimTags::State_Wet, TEXT("Wet") },
		{ GrimTags::State_Burning, TEXT("Burning") },
	};
	for (const TPair<FGameplayTag, const TCHAR*>& Label : StatusLabels)
	{
		if (GrimSurvivalComponentPrivate::HasExplicitTag(ASC, Label.Key))
		{
			Statuses += Statuses.IsEmpty() ? Label.Value : FString(TEXT(", ")) + Label.Value;
		}
	}

	auto Get = [ASC](const FGameplayAttribute& Attribute) { return ASC->GetNumericAttribute(Attribute); };

	// 디버그 글꼴(엔진 기본)에 한글 글리프가 없어 라벨은 영어로 둔다. 정식 HUD 는 UI 시스템 몫.
	TArray<FString> Lines = {
		Time,
		FString::Printf(TEXT("HP %.0f / %.0f"), Get(UGrimVitalAttributeSet::GetHealthAttribute()), Get(UGrimVitalAttributeSet::GetMaxHealthAttribute())),
		FString::Printf(TEXT("Mana %.0f / %.0f"), Get(UGrimMagicAttributeSet::GetManaAttribute()), Get(UGrimMagicAttributeSet::GetMaxManaAttribute())),
		FString::Printf(TEXT("Hunger %.1f   Fatigue %.1f"), Get(UGrimVitalAttributeSet::GetHungerAttribute()), Get(UGrimVitalAttributeSet::GetFatigueAttribute())),
		FString::Printf(TEXT("Body %.2f C   Air %.1f C"), Get(UGrimVitalAttributeSet::GetBodyTemperatureAttribute()), GetAmbientTemperature()),
		FString::Printf(TEXT("Cold +%d / resist %+.0f   Heat +%d / resist %+.0f"),
			ClimateExposure.Cold, Get(UGrimVitalAttributeSet::GetColdResistAttribute()),
			ClimateExposure.Heat, Get(UGrimVitalAttributeSet::GetHeatResistAttribute())),
		FString::Printf(TEXT("Move x%.2f  Cast x%.2f  ManaCost x%.2f"),
			Get(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()), Get(UGrimVitalAttributeSet::GetCastSpeedScaleAttribute()),
			Get(UGrimVitalAttributeSet::GetManaCostScaleAttribute())),
		Statuses.IsEmpty() ? FString(TEXT("Status -")) : TEXT("Status ") + Statuses,
	};
	if (IsDowned())
	{
		Lines.Add(FString::Printf(TEXT("DOWNED  %.0fs"), GetDownedSecondsRemaining()));
	}

	UFont* Font = GEngine->GetMediumFont();
	float Y = 40.f;
	for (const FString& Line : Lines)
	{
		Canvas->SetDrawColor(FColor::White);
		Canvas->DrawText(Font, Line, 40.f, Y);
		Y += 20.f;
	}
	if (ASC->HasMatchingGameplayTag(GrimCheatTags::Cheat_Super))
	{
		Canvas->SetDrawColor(FColor::Yellow);
		Canvas->DrawText(Font, TEXT("CHEAT Super ON"), 40.f, Y);
	}
}
