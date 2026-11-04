#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Core/Cheats/GrimCheatTags.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarGrimSurvivalShowDebug(
	TEXT("grim.Survival.ShowDebug"),
	1,
	TEXT("생존 · 시간 디버그 표시(1 = 켬). 정식 HUD 가 생기면 기본값을 0 으로 바꾼다."));

UGrimSurvivalComponent::UGrimSurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

float UGrimSurvivalComponent::GetAmbientTemperature() const
{
	const UWorld* World = GetWorld();
	const UGrimWeatherSubsystem* Weather = World ? World->GetSubsystem<UGrimWeatherSubsystem>() : nullptr;
	if (!Weather)
	{
		return GetDefault<UGrimSurvivalSettings>()->Tuning.ComfortTemperature;
	}
	const APawn* Pawn = GetAvatarPawn();
	return Weather->GetAmbientTemperature(Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector);
}

void UGrimSurvivalComponent::HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HasBegunPlay())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !Pawn)
	{
		return;
	}

	// PlayerState 는 폰이 바뀌어도 유지된다 — 초기값은 처음 한 번만.
	if (!bAttributesInitialized)
	{
		InitializeAttributes(ASC);
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
	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;

	// MaxHealth 를 먼저 — Health 클램프가 MaxHealth 를 본다.
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetMaxHealthAttribute(), Tuning.InitialMaxHealth);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), Tuning.InitialMaxHealth);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHungerAttribute(), Tuning.InitialHunger);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), Tuning.InitialBodyTemperature);
	bAttributesInitialized = true;
}

void UGrimSurvivalComponent::SurvivalTick()
{
	UAbilitySystemComponent* ASC = GetASC();
	APawn* Pawn = GetAvatarPawn();
	UWorld* World = GetWorld();
	if (!ASC || !Pawn || !World)
	{
		return;
	}

	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;
	const double Now = World->GetTimeSeconds();
	const float DeltaSeconds = static_cast<float>(Now - LastTickWorldSeconds);
	LastTickWorldSeconds = Now;

	LastHeatWarmth = GatherHeatWarmth(Pawn);

	FGrimSurvivalInputs Inputs;
	Inputs.Health = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute());
	Inputs.MaxHealth = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute());
	Inputs.Hunger = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute());
	Inputs.BodyTemperature = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetBodyTemperatureAttribute());
	Inputs.AmbientTemperature = GetAmbientTemperature();
	Inputs.HeatWarmth = LastHeatWarmth;
	Inputs.WetIntensity = UGrimElementalStatics::GetStateIntensity(Pawn, GrimTags::State_Wet);

	const FGrimSurvivalDeltas Deltas = FGrimSurvivalModel::ComputeTick(Inputs, DeltaSeconds, Tuning);

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

	UpdateStatusTags(ASC, FGrimSurvivalModel::EvaluateStatus(
		ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute()),
		ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetBodyTemperatureAttribute()),
		Tuning));
}

float UGrimSurvivalComponent::GatherHeatWarmth(const APawn* Pawn) const
{
	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;
	if (Tuning.HeatSourceRadius <= 0.f)
	{
		return 0.f;
	}

	// 열원은 질의 콜리전이 있는 액터 중 State.Burning 세기가 있는 것(양초 · 모닥불 · 불붙은 풀).
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSurvivalHeatSources), false, Pawn);
	const FVector Center = Pawn->GetActorLocation();
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(Tuning.HeatSourceRadius), Params);

	TSet<const AActor*> Visited;
	float Warmth = 0.f;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		if (!Actor || Visited.Contains(Actor))
		{
			continue;
		}
		Visited.Add(Actor);

		const float Intensity = UGrimElementalStatics::GetStateIntensity(Actor, GrimTags::State_Burning);
		if (Intensity > 0.f)
		{
			Warmth += FGrimSurvivalModel::ComputeHeatWarmth(Intensity, FVector::Dist(Center, Actor->GetActorLocation()), Tuning);
		}
	}
	return Warmth;
}

void UGrimSurvivalComponent::UpdateStatusTags(UAbilitySystemComponent* ASC, const FGrimSurvivalStatus& Status)
{
	// 복제 루스 태그 — 모든 클라이언트가 본다(마나 회복 정지 · 연출 등은 태그로 판단).
	auto SetTag = [ASC](const FGameplayTag& Tag, bool bOn)
	{
		const bool bHas = ASC->GetGameplayTagCount(Tag) > 0;
		if (bHas != bOn)
		{
			ASC->SetLooseGameplayTagCount(Tag, bOn ? 1 : 0, EGameplayTagReplicationState::TagOnly);
		}
	};
	SetTag(GrimSurvivalTags::Status_Hungry, Status.bHungry);
	SetTag(GrimSurvivalTags::Status_Starving, Status.bStarving);
	SetTag(GrimSurvivalTags::Status_Hypothermia, Status.bHypothermia);
	SetTag(GrimSurvivalTags::Status_Hyperthermia, Status.bHyperthermia);
}

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
		{ GrimSurvivalTags::Status_Hypothermia, TEXT("Hypothermia") },
		{ GrimSurvivalTags::Status_Hyperthermia, TEXT("Hyperthermia") },
		{ GrimTags::State_Wet, TEXT("Wet") },
		{ GrimTags::State_Burning, TEXT("Burning") },
	};
	for (const TPair<FGameplayTag, const TCHAR*>& Label : StatusLabels)
	{
		if (ASC->HasMatchingGameplayTag(Label.Key))
		{
			Statuses += Statuses.IsEmpty() ? Label.Value : FString(TEXT(", ")) + Label.Value;
		}
	}

	// 디버그 글꼴(엔진 기본)에 한글 글리프가 없어 라벨은 영어로 둔다. 정식 HUD 는 UI 시스템 몫.
	const FString Lines[] = {
		Time,
		FString::Printf(TEXT("HP %.0f / %.0f"),
			ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()),
			ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute())),
		FString::Printf(TEXT("Mana %.0f / %.0f"),
			ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute()),
			ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute())),
		FString::Printf(TEXT("Hunger %.1f"), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute())),
		FString::Printf(TEXT("Body %.2f C   Air %.1f C"),
			ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetBodyTemperatureAttribute()), GetAmbientTemperature()),
		Statuses.IsEmpty() ? FString(TEXT("Status -")) : TEXT("Status ") + Statuses,
	};

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
