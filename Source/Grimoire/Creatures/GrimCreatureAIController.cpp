#include "Creatures/GrimCreatureAIController.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTags.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace GrimCreatureAILocal // 유니티 빌드 이름 충돌 방지
{
	FVector RandomOffset2D(float Radius)
	{
		const FVector2D Point = FMath::RandPointInCircle(Radius);
		return FVector(Point.X, Point.Y, 0.f);
	}
}

AGrimCreatureAIController::AGrimCreatureAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGrimCreatureAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (UWorld* World = GetWorld())
	{
		const float Interval = FMath::Max(0.05f, UGrimCreatureSettings::Get()->AI.ThinkInterval);
		// 생물마다 판단 시점을 흩는다(한 프레임에 몰리지 않게).
		World->GetTimerManager().SetTimer(ThinkTimer, this, &AGrimCreatureAIController::Think, Interval, true, FMath::FRandRange(0.f, Interval));
	}
}

void AGrimCreatureAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThinkTimer);
	}
	StopMoving();
	Super::OnUnPossess();
}

void AGrimCreatureAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* MyPawn = GetPawn();
	if (!bHasMoveGoal || !MyPawn)
	{
		return;
	}
	const FVector ToGoal = (MoveGoal - MyPawn->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
	if (ToGoal.SizeSquared() <= FMath::Square(MoveAcceptance))
	{
		bHasMoveGoal = false;
		return;
	}
	MyPawn->AddMovementInput(ToGoal.GetSafeNormal(), 1.f);
}

void AGrimCreatureAIController::MoveTowards(const FVector& Goal, float Acceptance)
{
	MoveGoal = Goal;
	MoveAcceptance = Acceptance;
	bHasMoveGoal = true;
}

void AGrimCreatureAIController::StopMoving()
{
	bHasMoveGoal = false;
}

bool AGrimCreatureAIController::IsDark() const
{
	const UGrimEcosystemSubsystem* Ecosystem = GetWorld() ? GetWorld()->GetSubsystem<UGrimEcosystemSubsystem>() : nullptr;
	bool bNight = false;
	float Hours = 12.f;
	if (Ecosystem)
	{
		Ecosystem->GetTimeOfDay(bNight, Hours);
	}
	return bNight && !bLightSeen;
}

void AGrimCreatureAIController::GatherSenses(AGrimCreature* Creature, FGrimCreatureSenses& OutSenses)
{
	UWorld* World = GetWorld();
	const UGrimCreatureSpecies* Species = Creature->GetSpecies();
	const FGrimCreatureAITuning& AI = UGrimCreatureSettings::Get()->AI;
	const UGrimEcosystemSubsystem* Ecosystem = World->GetSubsystem<UGrimEcosystemSubsystem>();
	const FVector Me = Creature->GetActorLocation();
	const float Perception = Species->PerceptionRadius * (Creature->GetAIState() == EGrimCreatureAIState::Sleep ? AI.SleepPerceptionScale : 1.f);

	// 플레이어(서버는 모든 플레이어 컨트롤러를 안다)
	NearestPlayer = nullptr;
	NearestPlayerDistance = TNumericLimits<float>::Max();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn)
		{
			continue;
		}
		const float Distance = FVector::Dist(Me, PlayerPawn->GetActorLocation());
		if (Distance < NearestPlayerDistance)
		{
			NearestPlayerDistance = Distance;
			NearestPlayer = PlayerPawn;
		}
	}

	// 포식자 · 먹이(먹이 관계 표)
	NearestPredator = nullptr;
	BestPrey = nullptr;
	float PredatorDistance = TNumericLimits<float>::Max();
	float PreyDistance = TNumericLimits<float>::Max();
	int32 PreyPriority = TNumericLimits<int32>::Lowest();
	if (Ecosystem)
	{
		const FName MyId = Creature->GetInstance().SpeciesId;
		for (const TWeakObjectPtr<AGrimCreature>& Weak : Ecosystem->GetCreatures())
		{
			AGrimCreature* Other = Weak.Get();
			if (!Other || Other == Creature || Other->IsDead())
			{
				continue;
			}
			const FName OtherId = Other->GetInstance().SpeciesId;
			const float Distance = FVector::Dist(Me, Other->GetActorLocation());
			// 숨은 포식자는 알아채지 못한다(그림자 걸음).
			if (Distance <= Perception && !Other->IsShadowHidden() && Ecosystem->IsPreyOf(OtherId, MyId) && Distance < PredatorDistance)
			{
				PredatorDistance = Distance;
				NearestPredator = Other;
			}
			if (Distance <= AI.HuntRadius && Ecosystem->IsPreyOf(MyId, OtherId))
			{
				const int32 Priority = Ecosystem->GetHuntPriority(MyId, OtherId);
				if (Priority > PreyPriority || (Priority == PreyPriority && Distance < PreyDistance))
				{
					PreyPriority = Priority;
					PreyDistance = Distance;
					BestPrey = Other;
				}
			}
		}
	}

	// 불빛(타는 사물 · 생물) — 빛을 피하거나 그림자 걸음을 쓰는 종만 본다
	NearestLight = nullptr;
	bLightSeen = false;
	if (Species->bAvoidsLight || Species->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_ShadowStep))
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimCreatureLight), false, Creature);
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, Me, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), FCollisionShape::MakeSphere(AI.LightAvoidRadius), Params);
		float LightDistance = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (!Actor || Actor == Creature || UGrimElementalStatics::GetStateIntensity(Actor, GrimTags::State_Burning) <= 0.f)
			{
				continue;
			}
			const float Distance = FVector::Dist(Me, Actor->GetActorLocation());
			if (Distance < LightDistance)
			{
				LightDistance = Distance;
				NearestLight = Actor;
				bLightSeen = true;
			}
		}
	}

	AActor* Provoker = Creature->GetProvoker();
	if (Provoker && FVector::Dist(Me, Provoker->GetActorLocation()) > AI.ChaseGiveUpDistance)
	{
		Provoker = nullptr;
	}

	OutSenses.HealthRatio = Creature->GetHealthRatio();
	OutSenses.bBurning = Creature->GetElementalState() && Creature->GetElementalState()->HasState(GrimTags::State_Burning);
	OutSenses.bActiveTime = Ecosystem ? Ecosystem->IsActiveNow(Species->Activity) : true;
	OutSenses.bThreatNear = NearestPlayerDistance <= Perception || NearestPredator.IsValid();
	OutSenses.bHostileInAggroRange = NearestPlayerDistance <= Species->AggroRadius;
	OutSenses.bProvoked = Provoker != nullptr;
	OutSenses.bPreyNear = BestPrey.IsValid();
	OutSenses.bLightNear = bLightSeen;
}

void AGrimCreatureAIController::Think()
{
	AGrimCreature* Creature = GetPawn<AGrimCreature>();
	UWorld* World = GetWorld();
	if (!Creature || !World || !Creature->GetSpecies() || !Creature->HasAuthority())
	{
		return;
	}
	if (Creature->IsDead())
	{
		StopMoving();
		World->GetTimerManager().ClearTimer(ThinkTimer);
		return;
	}

	const FGrimCreatureAITuning& AI = UGrimCreatureSettings::Get()->AI;
	const FGrimCreatureAbilityTuning& Abilities = UGrimCreatureSettings::Get()->Abilities;
	GatherSenses(Creature, LastSenses);
	// 길들인 생물은 주인 곁 판단(갈래 T)
	if (Creature->GetInstance().IsTamed())
	{
		ThinkTamed(Creature);
		return;
	}
	LastSenses.bEnraged = Creature->GetTaming() && Creature->GetTaming()->IsEnraged();
	const FGrimCreatureTemperamentInfo Info = Creature->GetTemperamentInfo();
	EGrimCreatureAIState State = GrimCreatureMath::Decide(LastSenses, Info);
	const FVector Me = Creature->GetActorLocation();
	const double Now = World->GetTimeSeconds();

	switch (State)
	{
	case EGrimCreatureAIState::Flee:
	{
		Creature->SetRunning(true);
		// 무엇에서 도망치나: 불빛 → 때린 이 → 포식자 → 플레이어. 불붙었는데 위협이 없으면 아무 데로나.
		const AActor* Threat = nullptr;
		if (Info.bAvoidsLight && NearestLight.IsValid())
		{
			Threat = NearestLight.Get();
		}
		else if (AActor* Provoker = Creature->GetProvoker())
		{
			Threat = Provoker;
		}
		else if (NearestPredator.IsValid())
		{
			Threat = NearestPredator.Get();
		}
		else if (NearestPlayer.IsValid())
		{
			Threat = NearestPlayer.Get();
		}

		if (Threat && FVector::Dist(Me, Threat->GetActorLocation()) >= AI.FleeSafeDistance && !LastSenses.bBurning)
		{
			// 충분히 멀어졌다 — 멈춰서 지켜본다.
			StopMoving();
			State = EGrimCreatureAIState::Idle;
			break;
		}
		FVector Away = Threat ? (Me - Threat->GetActorLocation()).GetSafeNormal2D() : Creature->GetActorForwardVector().GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = GrimCreatureAILocal::RandomOffset2D(1.f).GetSafeNormal();
		}
		// 곧게만 달리지 않게 조금 흔든다.
		Away = Away.RotateAngleAxis(FMath::FRandRange(-30.f, 30.f), FVector::UpVector);
		MoveTowards(Me + Away * AI.MoveStepDistance, 50.f);
		CalmUntil = 0.0;
		break;
	}
	case EGrimCreatureAIState::Fight:
	case EGrimCreatureAIState::Hunt:
	{
		AActor* Target = nullptr;
		if (State == EGrimCreatureAIState::Hunt)
		{
			Target = BestPrey.Get();
		}
		else
		{
			Target = Creature->GetProvoker();
			if (!Target)
			{
				Target = NearestPlayer.Get();
			}
		}
		if (!Target)
		{
			StopMoving();
			State = EGrimCreatureAIState::Idle;
			break;
		}
		Creature->SetRunning(true);
		const float SurfaceDistance = Creature->GetSurfaceDistanceTo(Target);
		// 준비된 공격의 사거리까지 다가간다(원거리 기술이 쿨다운이면 물러서지 않고 붙는다).
		if (SurfaceDistance <= Creature->GetReadyAttackRange() * 0.9f)
		{
			StopMoving();
			Creature->TryAttack(Target);
		}
		else
		{
			MoveTowards(Target->GetActorLocation(), 10.f);
			// 그림자 걸음: 어두운 곳에서 멀리 있는 표적에게 숨어 다가간다.
			if (Creature->CanShadowStep() && IsDark() && SurfaceDistance > Abilities.ShadowStepMinTargetDistance)
			{
				Creature->StartShadowStep();
			}
		}
		CalmUntil = 0.0;
		break;
	}
	case EGrimCreatureAIState::Sleep:
		StopMoving();
		Creature->SetRunning(false);
		CalmUntil = 0.0;
		break;
	default:
	{
		// 한가함: 가만히 · 배회 · 풀 뜯기를 번갈아.
		Creature->SetRunning(false);
		const bool bCalmState = CalmState == EGrimCreatureAIState::Idle || CalmState == EGrimCreatureAIState::Wander || CalmState == EGrimCreatureAIState::Graze;
		if (Now >= CalmUntil || !bCalmState)
		{
			CalmState = GrimCreatureMath::PickCalmActivity(FMath::FRand(), Info.Diet, AI.CalmWeights);
			CalmUntil = Now + FMath::FRandRange(AI.CalmActivitySeconds.X, FMath::Max(AI.CalmActivitySeconds.X, AI.CalmActivitySeconds.Y));
			if (CalmState == EGrimCreatureAIState::Wander)
			{
				MoveTowards(Creature->GetHomeLocation() + GrimCreatureAILocal::RandomOffset2D(AI.WanderRadius), 60.f);
			}
			else
			{
				StopMoving();
			}
		}
		else if (CalmState == EGrimCreatureAIState::Wander && !bHasMoveGoal)
		{
			// 다 왔다 — 남은 시간은 가만히.
			CalmState = EGrimCreatureAIState::Idle;
		}
		State = CalmState;
		break;
	}
	}

	Creature->SetAIState(State);
}

void AGrimCreatureAIController::ThinkTamed(AGrimCreature* Creature)
{
	UWorld* World = GetWorld();
	const UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	const UGrimMountComponent* Mount = Creature->GetMount();
	const FGrimCreatureAITuning& AI = UGrimCreatureSettings::Get()->AI;
	const FGrimTamedBehaviorTuning& Behavior = UGrimTamingSettings::Get()->Behavior;
	APawn* OwnerPawn = Taming ? Taming->GetOwnerPawn() : nullptr;
	const FVector Me = Creature->GetActorLocation();

	// 플레이어(주인 · 친구)에게 맞은 것은 화내지 않는다 — 다른 생물에게 맞았을 때만 맞선다.
	AActor* Provoker = Creature->GetProvoker();
	if (const APawn* ProvokerPawn = Cast<APawn>(Provoker); ProvokerPawn && ProvokerPawn->GetPlayerState())
	{
		Provoker = nullptr;
	}
	if (Provoker && FVector::Dist(Me, Provoker->GetActorLocation()) > AI.ChaseGiveUpDistance)
	{
		Provoker = nullptr;
	}

	const FGrimCreatureTemperamentInfo Info = Creature->GetTemperamentInfo();
	FGrimTamedSenses Senses;
	Senses.bRidden = Mount && Mount->IsRidden();
	Senses.bKnockedOut = Taming && Taming->IsKnockedOut();
	Senses.bBurning = LastSenses.bBurning;
	Senses.bProvoked = Provoker != nullptr;
	Senses.bCanAttack = Info.bCanAttack;
	Senses.bTimid = Info.Temperament == EGrimCreatureTemperament::Timid;
	Senses.HealthRatio = LastSenses.HealthRatio;
	Senses.FleeHealthRatio = Info.FleeHealthRatio;
	Senses.Command = Taming ? Taming->GetCommand() : EGrimTamedCommand::Follow;
	Senses.OwnerDistance = OwnerPawn ? static_cast<float>(FVector::Dist2D(Me, OwnerPawn->GetActorLocation())) : -1.f;
	Senses.FollowDistance = Behavior.FollowDistance;
	const EGrimCreatureAIState State = GrimTamingMath::DecideTamed(Senses);

	switch (State)
	{
	case EGrimCreatureAIState::Ridden:
		// 탄 사람 입력으로 움직인다(UGrimMountComponent) — AI 는 손대지 않는다.
		StopMoving();
		break;
	case EGrimCreatureAIState::Flee:
	{
		Creature->SetRunning(true);
		FVector Away = Provoker ? (Me - Provoker->GetActorLocation()).GetSafeNormal2D() : Creature->GetActorForwardVector().GetSafeNormal2D();
		if (OwnerPawn && Provoker)
		{
			// 주인 쪽으로 물러난다.
			Away = (OwnerPawn->GetActorLocation() - Me).GetSafeNormal2D();
		}
		MoveTowards(Me + Away * AI.MoveStepDistance, 50.f);
		break;
	}
	case EGrimCreatureAIState::Fight:
		Creature->SetRunning(true);
		if (Provoker && Creature->GetSurfaceDistanceTo(Provoker) <= Creature->GetReadyAttackRange() * 0.9f)
		{
			StopMoving();
			Creature->TryAttack(Provoker);
		}
		else if (Provoker)
		{
			MoveTowards(Provoker->GetActorLocation(), 10.f);
		}
		break;
	case EGrimCreatureAIState::Follow:
		if (OwnerPawn)
		{
			if (Senses.OwnerDistance > Behavior.FollowTeleportDistance)
			{
				// 너무 멀다(지형에 걸림) — 주인 곁으로 옮겨 온다.
				const FVector Behind = OwnerPawn->GetActorLocation() - OwnerPawn->GetActorForwardVector().GetSafeNormal2D() * Behavior.SummonDistance;
				FVector Ground = Behind;
				if (UGrimEcosystemSubsystem::FindGround(World, Behind, Ground))
				{
					Creature->TeleportTo(Ground + FVector(0.f, 0.f, Creature->GetSimpleCollisionHalfHeight() + 2.f), Creature->GetActorRotation());
				}
			}
			Creature->SetRunning(Senses.OwnerDistance > Behavior.FollowRunDistance);
			MoveTowards(OwnerPawn->GetActorLocation(), Behavior.FollowDistance * 0.6f);
		}
		break;
	case EGrimCreatureAIState::Wander:
	{
		// 거점에 풀어놓음(R38): 집(내놓은 곳) 둘레를 천천히 배회한다. 상자에서 너무 멀어지면 집으로 되돌린다.
		const UGrimCreatureStorageSettings* StorageSettings = UGrimCreatureStorageSettings::Get();
		Creature->SetRunning(false);
		const FVector Home = Creature->GetHomeLocation();
		const AGrimCreatureBox* Box = Taming ? Taming->GetHomeBox() : nullptr;
		const FVector Anchor = Box ? Box->GetActorLocation() : Home;
		if (FVector::Dist2D(Me, Anchor) > StorageSettings->BoxRoamRadius * StorageSettings->RoamLeashScale)
		{
			FVector Ground = Home;
			if (UGrimEcosystemSubsystem::FindGround(World, Home, Ground))
			{
				Creature->TeleportTo(Ground + FVector(0.f, 0.f, Creature->GetSimpleCollisionHalfHeight() + 2.f), Creature->GetActorRotation());
			}
			StopMoving();
		}
		const double Now = World->GetTimeSeconds();
		if (Now >= CalmUntil)
		{
			CalmUntil = Now + FMath::FRandRange(AI.CalmActivitySeconds.X, FMath::Max(AI.CalmActivitySeconds.X, AI.CalmActivitySeconds.Y));
			if (FMath::FRand() < 0.6f)
			{
				MoveTowards(Home + GrimCreatureAILocal::RandomOffset2D(StorageSettings->RoamWanderRadius), 60.f);
			}
			else
			{
				StopMoving();
			}
		}
		break;
	}
	default:
		// Stay · Idle · Sleep(기절) — 제자리
		Creature->SetRunning(false);
		StopMoving();
		break;
	}
	Creature->SetAIState(State);
}
