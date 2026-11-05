#include "Magic/GrimSpellDelivery.h"

DEFINE_LOG_CATEGORY(LogGrimMagic);

namespace GrimSpellDelivery
{
	FLaunch ComputeLaunch(const FVector& Muzzle, const FVector& Chest, const FVector& AimPoint, const FVector& ViewDirection, float CloseDistance)
	{
		FLaunch Launch;
		const FVector View = ViewDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);

		Launch.bFromChest = CloseDistance > 0.f && FVector::DistSquared(Chest, AimPoint) < FMath::Square(CloseDistance);
		Launch.Start = Launch.bFromChest ? Chest : Muzzle;

		const FVector ToAim = AimPoint - Launch.Start;
		// 조준점이 시작점 뒤(카메라와 완드 사이에 맞은 점)면 시선 방향 그대로 쏜다.
		if (ToAim.IsNearlyZero() || FVector::DotProduct(ToAim, View) <= 0.f)
		{
			Launch.Direction = View;
		}
		else
		{
			Launch.Direction = ToAim.GetSafeNormal();
		}
		return Launch;
	}

	FVector SteerTowards(const FVector& Velocity, const FVector& Position, const FVector& TargetPoint, float MaxTurnDegPerSec, float MaxAngleDeg, float DeltaTime)
	{
		const float Speed = Velocity.Size();
		if (Speed <= UE_KINDA_SMALL_NUMBER || MaxTurnDegPerSec <= 0.f || DeltaTime <= 0.f)
		{
			return Velocity;
		}

		const FVector Direction = Velocity / Speed;
		const FVector ToTarget = (TargetPoint - Position).GetSafeNormal();
		if (ToTarget.IsNearlyZero())
		{
			return Velocity;
		}

		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Direction, ToTarget), -1.f, 1.f));
		if (Angle <= UE_KINDA_SMALL_NUMBER || Angle > FMath::DegreesToRadians(MaxAngleDeg))
		{
			return Velocity;
		}

		const FVector Axis = FVector::CrossProduct(Direction, ToTarget).GetSafeNormal();
		if (Axis.IsNearlyZero())
		{
			return Velocity;
		}
		const float Step = FMath::Min(Angle, FMath::DegreesToRadians(MaxTurnDegPerSec) * DeltaTime);
		return Direction.RotateAngleAxisRad(Step, Axis) * Speed;
	}

	bool SolveLaunchDirection(const FVector& Start, const FVector& Target, float Speed, float GravityZ, FVector& OutDirection)
	{
		const FVector Delta = Target - Start;
		const FVector Raw = Delta.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const float G = -GravityZ;
		const FVector2D Horizontal(Delta.X, Delta.Y);
		const float D = Horizontal.Size();

		if (G <= UE_KINDA_SMALL_NUMBER || Speed <= UE_KINDA_SMALL_NUMBER || D <= UE_KINDA_SMALL_NUMBER)
		{
			OutDirection = Raw;
			return Speed > UE_KINDA_SMALL_NUMBER;
		}

		const FVector Flat = FVector(Horizontal.X / D, Horizontal.Y / D, 0.f);
		const float V2 = Speed * Speed;
		const float H = Delta.Z;
		const float Disc = V2 * V2 - G * (G * D * D + 2.f * H * V2);
		if (Disc < 0.f)
		{
			// 닿지 않는다 — 그 방향으로 가장 멀리(45°). 조준이 그보다 높으면 조준 방향 그대로.
			const float RawElevation = FMath::Asin(FMath::Clamp(Raw.Z, -1.f, 1.f));
			const float Elevation = FMath::Max(RawElevation, UE_PI / 4.f);
			OutDirection = (RawElevation >= UE_PI / 4.f) ? Raw : (Flat * FMath::Cos(Elevation) + FVector::UpVector * FMath::Sin(Elevation));
			return false;
		}

		// 낮은 호: tanθ = (v² − √disc) / (g·d)
		const float Theta = FMath::Atan((V2 - FMath::Sqrt(Disc)) / (G * D));
		OutDirection = Flat * FMath::Cos(Theta) + FVector::UpVector * FMath::Sin(Theta);
		return true;
	}

	void ComputeArcPoints(const FVector& Start, const FVector& Velocity, float GravityZ, float TotalTime, int32 Segments, TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();
		const int32 Count = FMath::Max(Segments, 1);
		OutPoints.Reserve(Count + 1);
		for (int32 Index = 0; Index <= Count; ++Index)
		{
			const float T = TotalTime * static_cast<float>(Index) / static_cast<float>(Count);
			OutPoints.Add(Start + Velocity * T + FVector(0.f, 0.f, 0.5f * GravityZ * T * T));
		}
	}

	bool IsInCone(const FVector& Origin, const FVector& Direction, float HalfAngleDeg, float Range, const FVector& Point)
	{
		const FVector Delta = Point - Origin;
		const float DistSq = Delta.SizeSquared();
		if (DistSq > FMath::Square(Range))
		{
			return false;
		}
		if (DistSq <= UE_KINDA_SMALL_NUMBER)
		{
			return true;
		}
		const FVector Dir = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const float CosAngle = FVector::DotProduct(Delta / FMath::Sqrt(DistSq), Dir);
		return CosAngle >= FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDeg, 0.f, 180.f))) - UE_KINDA_SMALL_NUMBER;
	}

	float ConeIntensityAt(const FGrimConeFalloff& Falloff, float Range, float Distance)
	{
		if (Distance <= Falloff.NearDistance)
		{
			return Falloff.NearIntensity;
		}
		const float Span = FMath::Max(Range - Falloff.NearDistance, 1.f);
		const float T = FMath::Clamp((Distance - Falloff.NearDistance) / Span, 0.f, 1.f);
		return FMath::Lerp(Falloff.NearIntensity, Falloff.FarIntensity, FMath::Pow(T, FMath::Max(Falloff.Exponent, 0.01f)));
	}

	bool IsStimulusDue(const double* LastApplyTime, double Now, float Interval)
	{
		return !LastApplyTime || Now - *LastApplyTime >= static_cast<double>(Interval) - 1e-4;
	}

	FLagSplit SplitLag(float OneWayLatency, float Windup, float MaxCompensation)
	{
		const float Latency = FMath::Max(OneWayLatency, 0.f);
		const float Wind = FMath::Max(Windup, 0.f);
		FLagSplit Split;
		Split.WaitSeconds = FMath::Max(Wind - Latency, 0.f);
		Split.AdvanceSeconds = FMath::Clamp(Latency - Wind, 0.f, FMath::Max(MaxCompensation, 0.f));
		return Split;
	}

	bool IsClientAimPlausible(const FVector& ClientViewOrigin, const FVector& ClientAimPoint, const FVector& ServerPawnLocation, const FVector& ServerAimDirection, float MaxOriginError, float MaxAngleDeg, float MaxDistance)
	{
		if (ClientViewOrigin.ContainsNaN() || ClientAimPoint.ContainsNaN())
		{
			return false;
		}
		if (FVector::DistSquared(ClientViewOrigin, ServerPawnLocation) > FMath::Square(MaxOriginError))
		{
			return false;
		}
		if (FVector::DistSquared(ClientAimPoint, ServerPawnLocation) > FMath::Square(MaxDistance))
		{
			return false;
		}
		const FVector ClientDir = (ClientAimPoint - ClientViewOrigin).GetSafeNormal();
		const FVector ServerDir = ServerAimDirection.GetSafeNormal();
		if (ClientDir.IsNearlyZero() || ServerDir.IsNearlyZero())
		{
			return true;
		}
		return FVector::DotProduct(ClientDir, ServerDir) >= FMath::Cos(FMath::DegreesToRadians(MaxAngleDeg));
	}
}
