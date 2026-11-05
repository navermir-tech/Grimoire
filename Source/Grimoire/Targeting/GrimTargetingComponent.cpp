#include "Targeting/GrimTargetingComponent.h"
#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellbookComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"

namespace GrimTargetingComponentPrivate
{
	TAutoConsoleVariable<int32> CVarTargetingDebug(
		TEXT("Grim.Targeting.Debug"),
		0,
		TEXT("1 = 소프트 락온 후보(점수)를 그린다"),
		ECVF_Cheat);

	/** 폰과 폰에 붙은 것(완드 등)은 시야 · 후보에서 뺀다 */
	void AddPawnIgnores(FCollisionQueryParams& Params, const APawn* Pawn)
	{
		if (!Pawn)
		{
			return;
		}
		Params.AddIgnoredActor(Pawn);
		TArray<AActor*> Attached;
		Pawn->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}

	bool BelongsToPawn(const AActor* Actor, const APawn* Pawn)
	{
		return Actor == Pawn || Actor->GetOwner() == Pawn || Actor->GetAttachParentActor() == Pawn;
	}
}

UGrimTargetingComponent::UGrimTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

void UGrimTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();
	bSoftLockEnabled = Settings->bSoftLockEnabledByDefault;
	Reference = Settings->DefaultReference;

	// 선정은 로컬 플레이어만 한다. 전용 서버에는 로컬 플레이어가 없다.
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
	}
}

APlayerController* UGrimTargetingComponent::GetPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

APawn* UGrimTargetingComponent::GetControlledPawn() const
{
	const APlayerController* PC = GetPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

bool UGrimTargetingComponent::IsLocallyControlled() const
{
	const APlayerController* PC = GetPlayerController();
	return PC && PC->IsLocalController();
}

EGrimAimMode UGrimTargetingComponent::GetAimMode() const
{
	// TODO(R24): 줌인 조준(bZoomAimHeld)이 정해지면 여기서 Zoom 을 돌려준다.
	if (!CurrentTarget.IsValid())
	{
		return EGrimAimMode::Free;
	}
	return bLocked ? EGrimAimMode::Locked : EGrimAimMode::Soft;
}

const UGrimSpellDefinition* UGrimTargetingComponent::GetSelectedSpell() const
{
	const APlayerController* PC = GetPlayerController();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	return Spellbook ? Spellbook->GetSelectedSpell() : nullptr;
}

float UGrimTargetingComponent::GetSelectionRange() const
{
	const UGrimSpellDefinition* Spell = GetSelectedSpell();
	return Spell && Spell->Range > 0.f ? Spell->Range : GetDefault<UGrimTargetingSettings>()->DefaultSelectionRange;
}

float UGrimTargetingComponent::GetLockedRange() const
{
	return FMath::Max(GetDefault<UGrimTargetingSettings>()->LockedMaxRange, GetSelectionRange());
}

bool UGrimTargetingComponent::MakeSelectionFrame(EGrimTargetingReference InReference, float Range, FSelectionFrame& OutFrame) const
{
	const APlayerController* PC = GetPlayerController();
	const APawn* Pawn = GetControlledPawn();
	if (!PC || !Pawn)
	{
		return false;
	}

	PC->GetPlayerViewPoint(OutFrame.ViewLocation, OutFrame.ViewRotation);
	OutFrame.PawnLocation = Pawn->GetActorLocation();
	OutFrame.Range = Range;

	if (InReference == EGrimTargetingReference::Movement)
	{
		// 스틱 기준: 이동 입력 방향(없으면 캐릭터 정면), 높이는 보지 않는다(요 각도만).
		FVector Move = Pawn->GetLastMovementInputVector();
		Move.Z = 0.f;
		OutFrame.ConeOrigin = Pawn->GetPawnViewLocation();
		OutFrame.ConeAxis = Move.IsNearlyZero() ? Pawn->GetActorForwardVector().GetSafeNormal2D() : Move.GetSafeNormal();
		OutFrame.bFlattenAngle = true;
	}
	else
	{
		OutFrame.ConeOrigin = OutFrame.ViewLocation;
		OutFrame.ConeAxis = OutFrame.ViewRotation.Vector();
		OutFrame.bFlattenAngle = false;
	}
	return true;
}

bool UGrimTargetingComponent::ReactsToSelectedSpell(const AActor* Target) const
{
	const UGrimSpellDefinition* Spell = GetSelectedSpell();
	const UGrimElementalStateComponent* State = Target ? Target->FindComponentByClass<UGrimElementalStateComponent>() : nullptr;
	if (!Spell || !State)
	{
		return false;
	}
	FGrimElementalStimulus Stimulus = Spell->Stimulus;
	if (!Stimulus.Element.IsValid())
	{
		Stimulus.Element = Spell->Element;
	}
	// 조회만 한다 — 상태를 바꾸지 않고 휴면도 깨우지 않는다.
	return FGrimElementalReactionLibrary::FindStimulusRule(FGrimElementalReactionLibrary::GetDefaultTable(),
		State->GetStates(), State->Material, Stimulus) != nullptr;
}

void UGrimTargetingComponent::GatherCandidates(const FSelectionFrame& Frame, float HalfAngleDegrees, TArray<FGrimTargetCandidate>& OutCandidates) const
{
	using namespace GrimTargetingComponentPrivate;

	OutCandidates.Reset();
	UWorld* World = GetWorld();
	const APawn* Pawn = GetControlledPawn();
	if (!World || !Pawn || Frame.Range <= 0.f)
	{
		return;
	}
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();

	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<ECollisionChannel>& Channel : Settings->CandidateObjectChannels)
	{
		ObjectParams.AddObjectTypesToQuery(Channel);
	}
	if (!ObjectParams.IsValid())
	{
		return;
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrimTargetingCandidates), false);
	AddPawnIgnores(QueryParams, Pawn);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Frame.PawnLocation, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Frame.Range), QueryParams);

	TSet<const AActor*> Seen;
	const AActor* Current = CurrentTarget.Get();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Seen.Contains(Actor) || BelongsToPawn(Actor, Pawn))
		{
			continue;
		}
		Seen.Add(Actor);

		const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Actor);
		if (!Targetable || !Targetable->CanBeTargetedBy(Pawn))
		{
			continue;
		}
		const EGrimTargetKind Kind = Targetable->GetTargetKind();
		const EGrimTargetCategory Category = FGrimTargetingMath::Categorize(Kind, Kind == EGrimTargetKind::Object && ReactsToSelectedSpell(Actor));
		if (Category == EGrimTargetCategory::None)
		{
			continue; // 아군 · 종류 없음
		}

		FGrimTargetCandidate Candidate;
		Candidate.Actor = Actor;
		Candidate.Kind = Kind;
		Candidate.Category = Category;
		Candidate.Point = Targetable->GetTargetPoint();
		Candidate.Distance = FVector::Dist(Frame.PawnLocation, Candidate.Point);
		Candidate.bIsCurrent = Actor == Current;

		// 카메라와 캐릭터 사이(캐릭터 뒤)는 원뿔 안이어도 뺀다.
		if (FVector::DotProduct(Candidate.Point - Frame.PawnLocation, Frame.ConeAxis) < 0.f)
		{
			continue;
		}
		const FVector AnglePoint = Frame.bFlattenAngle ? FVector(Candidate.Point.X, Candidate.Point.Y, Frame.ConeOrigin.Z) : Candidate.Point;
		Candidate.AngleDegrees = FGrimTargetingMath::AngleFromAxisDegrees(Frame.ConeOrigin, Frame.ConeAxis, AnglePoint);

		const float AllowedHalfAngle = HalfAngleDegrees + (Candidate.bIsCurrent ? Settings->CurrentTargetExtraHalfAngleDegrees : 0.f);
		if (!FGrimTargetingMath::IsInsideCone(Candidate.AngleDegrees, Candidate.Distance, AllowedHalfAngle, Frame.Range))
		{
			continue;
		}
		FGrimTargetingMath::ScoreCandidate(Candidate, HalfAngleDegrees, Frame.Range, Settings->Weights);
		OutCandidates.Add(Candidate);
	}
}

bool UGrimTargetingComponent::HasLineOfSight(const FVector& From, const AActor* Target, const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimTargetingSight), false);
	GrimTargetingComponentPrivate::AddPawnIgnores(Params, GetControlledPawn());

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, From, Point, GetDefault<UGrimTargetingSettings>()->VisibilityChannel, Params))
	{
		return true;
	}
	const AActor* HitActor = Hit.GetActor();
	return HitActor && (HitActor == Target || HitActor->GetAttachParentActor() == Target);
}

void UGrimTargetingComponent::UpdateSoftSelection(float DeltaSinceLast, bool bForce)
{
	if (!bSoftLockEnabled && !bForce)
	{
		LastCandidates.Reset();
		if (CurrentTarget.IsValid())
		{
			SetTarget(nullptr, EGrimTargetKind::None, false);
		}
		return;
	}

	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();
	FSelectionFrame Frame;
	if (!MakeSelectionFrame(Reference, GetSelectionRange(), Frame))
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
		return;
	}

	TArray<FGrimTargetCandidate> Candidates;
	GatherCandidates(Frame, Settings->ConeHalfAngleDegrees, Candidates);
	FGrimTargetingMath::SortAndCap(Candidates, Settings->bEnemiesBeforeObjects, Settings->MaxCandidates);

	// 점수 순으로 상위 MaxVisibilityTraces 개만 시야를 본다. 지금 대상은 잠깐(OcclusionGraceSeconds) 가려져도 유지.
	AActor* Chosen = nullptr;
	EGrimTargetKind ChosenKind = EGrimTargetKind::None;
	const int32 TraceCount = FMath::Min(Candidates.Num(), Settings->MaxVisibilityTraces);
	for (int32 Index = 0; Index < TraceCount; ++Index)
	{
		const FGrimTargetCandidate& Candidate = Candidates[Index];
		AActor* Actor = Candidate.Actor.Get();
		if (!Actor)
		{
			continue;
		}
		const bool bVisible = HasLineOfSight(Frame.ConeOrigin, Actor, Candidate.Point);
		const bool bEligible = Candidate.bIsCurrent
			? Occlusion.Update(bVisible, DeltaSinceLast, Settings->OcclusionGraceSeconds)
			: bVisible;
		if (bEligible)
		{
			Chosen = Actor;
			ChosenKind = Candidate.Kind;
			break;
		}
	}

	LastCandidates = MoveTemp(Candidates);
	SetTarget(Chosen, ChosenKind, false);
}

void UGrimTargetingComponent::UpdateLockedTarget(float DeltaSinceLast)
{
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();
	AActor* Target = CurrentTarget.Get();
	const APawn* Pawn = GetControlledPawn();
	const APlayerController* PC = GetPlayerController();
	const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Target);

	// 죽음 · 사라짐 · 대상 불가 → 해제
	if (!Target || !Pawn || !PC || !Targetable || !Targetable->CanBeTargetedBy(Pawn))
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
		return;
	}
	// 범위 밖 → 해제
	const FVector Point = Targetable->GetTargetPoint();
	if (FVector::Dist(Pawn->GetActorLocation(), Point) > GetLockedRange())
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
		return;
	}
	// 오래 가려짐 → 해제
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	if (!Occlusion.Update(HasLineOfSight(ViewLocation, Target, Point), DeltaSinceLast, Settings->LockedOcclusionGraceSeconds))
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
	}
}

void UGrimTargetingComponent::UpdateLockedCamera(float DeltaTime)
{
	APlayerController* PC = GetPlayerController();
	const APawn* Pawn = GetControlledPawn();
	const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(CurrentTarget.Get());
	if (!PC || !Pawn || !Targetable)
	{
		return;
	}
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();

	// 스프링암 피벗에서 대상을 보는 회전으로 컨트롤 회전을 보간한다(스프링암이 컨트롤 회전을 따른다).
	FVector Pivot = Pawn->GetPawnViewLocation();
	if (const AGrimPlayerCharacter* Character = Cast<AGrimPlayerCharacter>(Pawn))
	{
		if (const USpringArmComponent* Boom = Character->GetCameraBoom())
		{
			Pivot = Boom->GetComponentLocation();
		}
	}
	FRotator Desired = (Targetable->GetTargetPoint() - Pivot).Rotation();
	Desired.Pitch = FMath::Clamp(Desired.Pitch + Settings->LockCameraPitchOffset, Settings->LockCameraMinPitch, Settings->LockCameraMaxPitch);
	Desired.Roll = 0.f;

	FRotator NewRotation = Settings->LockCameraInterpSpeed > 0.f
		? FMath::RInterpTo(PC->GetControlRotation(), Desired, DeltaTime, Settings->LockCameraInterpSpeed)
		: Desired;
	NewRotation.Roll = 0.f;
	PC->SetControlRotation(NewRotation);
}

void UGrimTargetingComponent::TrySwitchLockedTarget(const FVector2D& ScreenDirection)
{
	APlayerController* PC = GetPlayerController();
	AActor* Current = CurrentTarget.Get();
	const IGrimTargetable* CurrentTargetable = UGrimTargetingStatics::FindTargetable(Current);
	if (!PC || !CurrentTargetable)
	{
		return;
	}
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();

	FSelectionFrame Frame;
	if (!MakeSelectionFrame(EGrimTargetingReference::Camera, GetLockedRange(), Frame))
	{
		return;
	}
	FVector2D From;
	if (!FGrimTargetingMath::ProjectToViewPlane(Frame.ViewLocation, Frame.ViewRotation, CurrentTargetable->GetTargetPoint(), From))
	{
		UE_LOG(LogGrimTargeting, Verbose, TEXT("넘기기: 지금 대상이 카메라 뒤"));
		return;
	}

	// 카메라 앞 반구 · 고정 사거리 안의 후보를 화면에 투영한다.
	TArray<FGrimTargetCandidate> Candidates;
	GatherCandidates(Frame, 89.f, Candidates);
	FGrimTargetingMath::SortAndCap(Candidates, false, Settings->MaxCandidates);

	// 화면 안 판정: 가로 시야각 · 화면비(뷰포트가 없으면 16:9).
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	const float HalfFOV = FMath::DegreesToRadians((PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() : 90.f) * 0.5f);
	const float AspectRatio = ViewportX > 0 && ViewportY > 0 ? static_cast<float>(ViewportX) / ViewportY : 16.f / 9.f;
	const float MaxPlaneX = FMath::Tan(HalfFOV) * 1000.f;
	const float MaxPlaneY = MaxPlaneX / AspectRatio;

	TArray<FVector2D> ScreenPoints;
	TArray<int32> CandidateIndices;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (Candidates[Index].bIsCurrent)
		{
			continue;
		}
		FVector2D Screen;
		if (!FGrimTargetingMath::ProjectToViewPlane(Frame.ViewLocation, Frame.ViewRotation, Candidates[Index].Point, Screen))
		{
			continue;
		}
		if (FMath::Abs(Screen.X) > MaxPlaneX || FMath::Abs(Screen.Y) > MaxPlaneY)
		{
			continue; // 화면 밖
		}
		ScreenPoints.Add(Screen);
		CandidateIndices.Add(Index);
	}

	UE_LOG(LogGrimTargeting, Verbose, TEXT("넘기기: 방향 %s, 지금 %s, 화면 후보 %d/%d"),
		*ScreenDirection.ToString(), *From.ToString(), ScreenPoints.Num(), Candidates.Num());

	// 방향이 맞는 가장 가까운 것부터, 가려졌으면 다음 것(트레이스 상한까지).
	for (int32 Attempt = 0; Attempt < Settings->MaxVisibilityTraces && ScreenPoints.Num() > 0; ++Attempt)
	{
		const int32 Pick = FGrimTargetingMath::FindSwitchTarget(From, ScreenDirection, ScreenPoints, Settings->LockSwitchMaxAngleDegrees);
		if (Pick == INDEX_NONE)
		{
			return;
		}
		const FGrimTargetCandidate& Candidate = Candidates[CandidateIndices[Pick]];
		if (AActor* Actor = Candidate.Actor.Get())
		{
			if (HasLineOfSight(Frame.ViewLocation, Actor, Candidate.Point))
			{
				SetTarget(Actor, Candidate.Kind, true);
				return;
			}
		}
		ScreenPoints.RemoveAt(Pick);
		CandidateIndices.RemoveAt(Pick);
	}
}

void UGrimTargetingComponent::SetTarget(AActor* NewTarget, EGrimTargetKind NewKind, bool bNewLocked)
{
	const AActor* OldTarget = CurrentTarget.Get();
	const EGrimAimMode OldMode = GetAimMode();

	CurrentTarget = NewTarget;
	CurrentKind = NewTarget ? NewKind : EGrimTargetKind::None;
	bLocked = NewTarget && bNewLocked;

	if (OldTarget != NewTarget)
	{
		Occlusion.Reset();
	}
	if (OldTarget != NewTarget || OldMode != GetAimMode())
	{
		if (!bLocked)
		{
			LookFlick.Reset();
		}
		UE_LOG(LogGrimTargeting, Verbose, TEXT("%s: 대상 %s (%s)"), *GetNameSafe(GetOwner()), *GetNameSafe(NewTarget), *UEnum::GetValueAsString(GetAimMode()));
		OnTargetChanged.Broadcast(this, NewTarget, GetAimMode());
	}
}

void UGrimTargetingComponent::ToggleLock()
{
	if (IsLocked())
	{
		Unlock();
		return;
	}
	if (!CurrentTarget.IsValid())
	{
		UpdateSoftSelection(0.f, true);
	}
	if (AActor* Target = CurrentTarget.Get())
	{
		SetTarget(Target, CurrentKind, true);
		Occlusion.Reset();
		LookFlick.Reset();
	}
}

void UGrimTargetingComponent::Unlock()
{
	if (bLocked)
	{
		// 대상은 소프트 대상으로 남긴다. 다음 선정에서 원뿔 · 점수로 다시 판단한다.
		SetTarget(CurrentTarget.Get(), CurrentKind, false);
	}
}

void UGrimTargetingComponent::SetSoftLockEnabled(bool bEnabled)
{
	bSoftLockEnabled = bEnabled;
	if (!bEnabled && !IsLocked())
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
	}
}

void UGrimTargetingComponent::SetTargetingReference(EGrimTargetingReference NewReference)
{
	Reference = NewReference;
}

bool UGrimTargetingComponent::HandleLookInput(const FVector2D& YawPitch)
{
	if (!IsLocked())
	{
		return false;
	}
	FVector2D Direction;
	if (LookFlick.AddInput(YawPitch, GetDefault<UGrimTargetingSettings>()->LockSwitchInputThreshold, Direction))
	{
		TrySwitchLockedTarget(FGrimTargetingMath::LookInputToScreenDirection(Direction));
		LookFlick.StartCooldown(GetDefault<UGrimTargetingSettings>()->LockSwitchCooldownSeconds);
	}
	return true;
}

FVector UGrimTargetingComponent::FreeAimPoint(const FVector& ViewLocation, const FRotator& ViewRotation, float MaxRange) const
{
	const FVector Direction = ViewRotation.Vector();
	const float Range = MaxRange > 0.f ? MaxRange : GetSelectionRange();
	const APawn* Pawn = GetControlledPawn();

	// 사거리는 캐릭터에서 잰다 — 카메라가 캐릭터 뒤에 있는 만큼 시작점을 앞으로 당긴다(카메라와 캐릭터 사이 것을 맞히지 않게).
	const float CameraToPawn = Pawn ? FMath::Max(0.f, static_cast<float>(FVector::DotProduct(Pawn->GetActorLocation() - ViewLocation, Direction))) : 0.f;
	const FVector Start = ViewLocation + Direction * CameraToPawn;
	const FVector End = Start + Direction * Range;

	UWorld* World = GetWorld();
	if (!World)
	{
		return End;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimTargetingFreeAim), false);
	GrimTargetingComponentPrivate::AddPawnIgnores(Params, Pawn);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, GetDefault<UGrimTargetingSettings>()->VisibilityChannel, Params))
	{
		return Hit.ImpactPoint;
	}
	return End;
}

FGrimAimResult UGrimTargetingComponent::GetCurrentAim(float MaxRange) const
{
	FGrimAimResult Result;
	const APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return Result;
	}
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(Result.ViewOrigin, ViewRotation);

	// TODO(R24): 줌인 조준 — bZoomAimHeld 면 Mode = Zoom, 자동 대상 없이 화면 중앙 트레이스(맞은 액터를 Target 에).

	AActor* Target = CurrentTarget.Get();
	const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(Target);
	const APawn* Pawn = GetControlledPawn();
	if (Targetable && Pawn && Targetable->CanBeTargetedBy(Pawn))
	{
		const FVector Point = Targetable->GetTargetPoint();
		// 고정 대상은 사거리와 상관없이 돌려준다(마법 쪽이 사거리에서 자른다). 소프트 대상은 이 마법 사거리 안일 때만.
		if (bLocked || MaxRange <= 0.f || FVector::Dist(Pawn->GetActorLocation(), Point) <= MaxRange)
		{
			Result.Mode = bLocked ? EGrimAimMode::Locked : EGrimAimMode::Soft;
			Result.Target = Target;
			Result.TargetKind = CurrentKind;
			Result.AimPoint = Point;
			return Result;
		}
	}

	Result.Mode = EGrimAimMode::Free;
	Result.AimPoint = FreeAimPoint(Result.ViewOrigin, ViewRotation, MaxRange);
	return Result;
}

void UGrimTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocallyControlled())
	{
		return;
	}
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();
	LookFlick.Tick(DeltaTime, Settings->LockSwitchInputResetSeconds);

	// 대상 액터가 사라졌다(파괴) — 모드를 Free 로 알린다.
	if (CurrentKind != EGrimTargetKind::None && !CurrentTarget.IsValid())
	{
		CurrentKind = EGrimTargetKind::None;
		bLocked = false;
		Occlusion.Reset();
		OnTargetChanged.Broadcast(this, nullptr, EGrimAimMode::Free);
	}

	if (!GetControlledPawn())
	{
		SetTarget(nullptr, EGrimTargetKind::None, false);
		return;
	}

	SelectionTimer += DeltaTime;
	if (SelectionTimer >= Settings->SelectionInterval)
	{
		const float Elapsed = SelectionTimer;
		SelectionTimer = 0.f;
		if (IsLocked())
		{
			UpdateLockedTarget(Elapsed);
		}
		else
		{
			UpdateSoftSelection(Elapsed, false);
		}
	}

	if (IsLocked())
	{
		UpdateLockedCamera(DeltaTime);
	}

	DrawTargetMarker();
	if (GrimTargetingComponentPrivate::CVarTargetingDebug.GetValueOnGameThread() > 0)
	{
		DrawDebugCandidates();
	}
}

void UGrimTargetingComponent::DrawTargetMarker() const
{
#if ENABLE_DRAW_DEBUG
	// 임시 표식: 대상 조준점에 카메라를 보는 속성색 링. 고정이면 바깥 링 하나 더.
	const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(CurrentTarget.Get());
	const APlayerController* PC = GetPlayerController();
	UWorld* World = GetWorld();
	if (!Targetable || !PC || !World)
	{
		return;
	}
	const UGrimTargetingSettings* Settings = GetDefault<UGrimTargetingSettings>();
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector Center = Targetable->GetTargetPoint() + FVector(0.f, 0.f, Settings->MarkerHeightOffset);
	const float Radius = Settings->MarkerRadius + FVector::Dist(ViewLocation, Center) * Settings->MarkerRadiusPerDistance;
	const UGrimSpellDefinition* Spell = GetSelectedSpell();
	const FColor Color = Settings->GetElementColor(Spell ? Spell->Element : FGameplayTag());
	const FRotationMatrix ViewAxes(ViewRotation);
	const FVector YAxis = ViewAxes.GetUnitAxis(EAxis::Y);
	const FVector ZAxis = ViewAxes.GetUnitAxis(EAxis::Z);

	DrawDebugCircle(World, Center, Radius, 32, Color, false, -1.f, SDPG_Foreground, Settings->MarkerThickness, YAxis, ZAxis, false);
	if (IsLocked())
	{
		DrawDebugCircle(World, Center, Radius * Settings->LockedMarkerRadiusScale, 32, Color, false, -1.f, SDPG_Foreground, Settings->MarkerThickness, YAxis, ZAxis, false);
	}
#endif
}

void UGrimTargetingComponent::DrawDebugCandidates() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (const FGrimTargetCandidate& Candidate : LastCandidates)
	{
		const bool bChosen = Candidate.Actor.Get() == CurrentTarget.Get();
		const FColor Color = bChosen ? FColor::Green : FColor(160, 160, 160);
		DrawDebugPoint(World, Candidate.Point, 8.f, Color, false, -1.f, SDPG_Foreground);
		DrawDebugString(World, Candidate.Point + FVector(0.f, 0.f, 25.f),
			FString::Printf(TEXT("%.2f %.0f° %.0fcm"), Candidate.Score, Candidate.AngleDegrees, Candidate.Distance), nullptr, Color, 0.f);
	}
#endif
}

FString UGrimTargetingComponent::Describe() const
{
	return FString::Printf(TEXT("mode=%s target=%s kind=%s locked=%d softlock=%d ref=%s range=%.0f candidates=%d"),
		*UEnum::GetDisplayValueAsText(GetAimMode()).ToString(),
		*GetNameSafe(CurrentTarget.Get()),
		*UEnum::GetDisplayValueAsText(CurrentKind).ToString(),
		IsLocked() ? 1 : 0,
		bSoftLockEnabled ? 1 : 0,
		*UEnum::GetDisplayValueAsText(Reference).ToString(),
		GetSelectionRange(),
		LastCandidates.Num());
}
