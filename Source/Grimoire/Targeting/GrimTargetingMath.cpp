#include "Targeting/GrimTargetingMath.h"
#include "Targeting/GrimTargetingSettings.h"

float FGrimTargetingMath::AngleFromAxisDegrees(const FVector& Origin, const FVector& Axis, const FVector& Point)
{
	const FVector AxisDir = Axis.GetSafeNormal();
	const FVector ToPoint = (Point - Origin).GetSafeNormal();
	if (AxisDir.IsZero() || ToPoint.IsZero())
	{
		return 0.f;
	}
	const float Cos = FMath::Clamp(static_cast<float>(FVector::DotProduct(AxisDir, ToPoint)), -1.f, 1.f);
	return FMath::RadiansToDegrees(FMath::Acos(Cos));
}

bool FGrimTargetingMath::IsInsideCone(float AngleDegrees, float Distance, float HalfAngleDegrees, float Range)
{
	return AngleDegrees <= HalfAngleDegrees && Distance <= Range;
}

EGrimTargetCategory FGrimTargetingMath::Categorize(EGrimTargetKind Kind, bool bReactsToSelectedSpell)
{
	switch (Kind)
	{
	case EGrimTargetKind::Enemy:
		return EGrimTargetCategory::Enemy;
	case EGrimTargetKind::Creature:
		return EGrimTargetCategory::Creature;
	case EGrimTargetKind::Object:
		return bReactsToSelectedSpell ? EGrimTargetCategory::ReactiveObject : EGrimTargetCategory::Object;
	case EGrimTargetKind::Ally:
	case EGrimTargetKind::None:
	default:
		return EGrimTargetCategory::None;
	}
}

float FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory Category, const FGrimTargetScoreWeights& Weights)
{
	switch (Category)
	{
	case EGrimTargetCategory::Enemy:
		return Weights.EnemyKind;
	case EGrimTargetCategory::Creature:
		return Weights.CreatureKind;
	case EGrimTargetCategory::ReactiveObject:
		return Weights.ReactiveObjectKind;
	case EGrimTargetCategory::Object:
		return Weights.ObjectKind;
	case EGrimTargetCategory::None:
	default:
		return 0.f;
	}
}

float FGrimTargetingMath::Score(float AngleDegrees, float HalfAngleDegrees, float Distance, float Range,
	EGrimTargetCategory Category, bool bIsCurrent, const FGrimTargetScoreWeights& Weights)
{
	const float CenterTerm = HalfAngleDegrees > 0.f ? 1.f - FMath::Clamp(AngleDegrees / HalfAngleDegrees, 0.f, 1.f) : 0.f;
	const float DistanceTerm = Range > 0.f ? 1.f - FMath::Clamp(Distance / Range, 0.f, 1.f) : 0.f;
	return Weights.Center * CenterTerm
		+ Weights.Distance * DistanceTerm
		+ Weights.Kind * GetCategoryWeight(Category, Weights)
		+ (bIsCurrent ? Weights.Keep : 0.f);
}

void FGrimTargetingMath::ScoreCandidate(FGrimTargetCandidate& Candidate, float HalfAngleDegrees, float Range, const FGrimTargetScoreWeights& Weights)
{
	Candidate.Score = Score(Candidate.AngleDegrees, HalfAngleDegrees, Candidate.Distance, Range, Candidate.Category, Candidate.bIsCurrent, Weights);
}

void FGrimTargetingMath::SortAndCap(TArray<FGrimTargetCandidate>& Candidates, bool bEnemiesFirst, int32 MaxCount)
{
	Candidates.StableSort([bEnemiesFirst](const FGrimTargetCandidate& A, const FGrimTargetCandidate& B)
	{
		if (bEnemiesFirst)
		{
			const bool bAEnemy = A.Category == EGrimTargetCategory::Enemy;
			const bool bBEnemy = B.Category == EGrimTargetCategory::Enemy;
			if (bAEnemy != bBEnemy)
			{
				return bAEnemy;
			}
		}
		return A.Score > B.Score;
	});
	if (MaxCount >= 0 && Candidates.Num() > MaxCount)
	{
		Candidates.SetNum(MaxCount);
	}
}

bool FGrimTargetingMath::ProjectToViewPlane(const FVector& ViewLocation, const FRotator& ViewRotation, const FVector& Point, FVector2D& OutPlane)
{
	const FVector Local = ViewRotation.UnrotateVector(Point - ViewLocation);
	if (Local.X <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	OutPlane = FVector2D(Local.Y / Local.X, -Local.Z / Local.X) * 1000.0;
	return true;
}

int32 FGrimTargetingMath::FindSwitchTarget(const FVector2D& From, const FVector2D& Direction, TConstArrayView<FVector2D> Candidates, float MaxAngleDegrees)
{
	const FVector2D Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return INDEX_NONE;
	}
	const float MaxAngle = FMath::Max(MaxAngleDegrees, 1.f);

	int32 BestIndex = INDEX_NONE;
	float BestCost = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FVector2D Delta = Candidates[Index] - From;
		const float Length = Delta.Size();
		if (Length < 1.f)
		{
			continue; // 지금 대상과 겹친 자리 — 넘길 곳이 아니다
		}
		const float Cos = FMath::Clamp(static_cast<float>(FVector2D::DotProduct(Delta / Length, Dir)), -1.f, 1.f);
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(Cos));
		if (Angle > MaxAngle)
		{
			continue;
		}
		// 가까울수록, 방향이 맞을수록. 방향이 허용 끝이면 거리가 두 배인 셈.
		const float Cost = Length * (1.f + Angle / MaxAngle);
		if (Cost < BestCost)
		{
			BestCost = Cost;
			BestIndex = Index;
		}
	}
	return BestIndex;
}

bool FGrimOcclusionTimer::Update(bool bVisible, float DeltaSeconds, float GraceSeconds)
{
	if (bVisible)
	{
		OccludedSeconds = 0.f;
		return true;
	}
	OccludedSeconds += FMath::Max(DeltaSeconds, 0.f);
	return OccludedSeconds <= GraceSeconds;
}

bool FGrimLookFlickDetector::AddInput(const FVector2D& YawPitch, float Threshold, FVector2D& OutDirection)
{
	IdleSeconds = 0.f;
	if (CooldownRemaining > 0.f)
	{
		return false;
	}
	Accumulated += YawPitch;
	if (Accumulated.Size() < Threshold)
	{
		return false;
	}
	OutDirection = Accumulated.GetSafeNormal();
	Accumulated = FVector2D::ZeroVector;
	return !OutDirection.IsNearlyZero();
}

void FGrimLookFlickDetector::Tick(float DeltaSeconds, float ResetSeconds)
{
	CooldownRemaining = FMath::Max(CooldownRemaining - DeltaSeconds, 0.f);
	IdleSeconds += DeltaSeconds;
	if (IdleSeconds > ResetSeconds)
	{
		Accumulated = FVector2D::ZeroVector;
	}
}
