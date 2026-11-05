#include "MagicFX/GrimMagicFXMath.h"

namespace
{
	/** a~b 구간에서 0→1 로 부드럽게. */
	float Ramp(float T, float A, float B)
	{
		if (B <= A)
		{
			return T >= B ? 1.f : 0.f;
		}
		const float X = FMath::Clamp((T - A) / (B - A), 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	// 컴포넌트 공간 축(마네킹: 메시가 −90° 돌아 있어 앞 = +Y, 위 = +Z, 오른쪽 = −X).
	const FVector AxisForward(0.f, 1.f, 0.f);
	const FVector AxisUp(0.f, 0.f, 1.f);
	const FVector AxisRight(-1.f, 0.f, 0.f);

	// 동작별 시간(초). 레퍼런스(호그와트 레거시)를 보고 정한 제안값 — spell-feel 「시전 동작」 표와 같이 고친다.
	constexpr float FlickAttack = 0.07f;
	constexpr float FlickReleaseStart = 0.2f;
	constexpr float FlickEnd = 0.45f;
	constexpr float ExtendAttack = 0.12f;
	constexpr float ExtendRelease = 0.25f;
	constexpr float SweepAttack = 0.06f;
	constexpr float SweepReleaseStart = 0.32f;
	constexpr float SweepEnd = 0.55f;
	constexpr float SweepFromYaw = 55.f;
	constexpr float SweepToYaw = -40.f;
}

namespace GrimMagicFXMath
{
	FVector CalcBookBobOffset(float TimeSeconds, const FGrimBookFloatParams& Params)
	{
		const float Period = FMath::Max(Params.BobPeriod, 0.1f);
		return FVector(0.f, 0.f, Params.BobAmplitude * FMath::Sin(UE_TWO_PI * TimeSeconds / Period));
	}

	FRotator CalcBookBobTilt(float TimeSeconds, const FGrimBookFloatParams& Params)
	{
		const float Period = FMath::Max(Params.BobPeriod, 0.1f);
		const float Pitch = Params.TiltDegrees * FMath::Sin(UE_TWO_PI * TimeSeconds / (Period * 1.3f));
		const float Roll = Params.TiltDegrees * 0.7f * FMath::Cos(UE_TWO_PI * TimeSeconds / (Period * 1.7f));
		return FRotator(Pitch, 0.f, Roll);
	}

	float CalcFlipProgress(float Elapsed, float Duration)
	{
		if (Duration <= 0.f)
		{
			return 1.f;
		}
		return Ramp(Elapsed, 0.f, Duration);
	}

	FQuat CalcFlipPageRotation(float Progress, int32 Direction)
	{
		const float P = FMath::Clamp(Progress, 0.f, 1.f);
		const float AngleDegrees = Direction >= 0 ? 180.f * P : 180.f * (1.f - P);
		// +X 축 회전: 오른쪽(+Y) 면이 위(+Z)를 지나 왼쪽(−Y) 으로 간다.
		return FQuat(FVector::ForwardVector, FMath::DegreesToRadians(AngleDegrees));
	}

	float CalcFlipGlow(float SinceFlipStart, float GlowDuration, float Peak)
	{
		if (GlowDuration <= 0.f || SinceFlipStart < 0.f || SinceFlipStart >= GlowDuration)
		{
			return 0.f;
		}
		const float Rise = GlowDuration * 0.15f;
		if (SinceFlipStart < Rise)
		{
			return Peak * (SinceFlipStart / Rise);
		}
		const float Fall = (SinceFlipStart - Rise) / (GlowDuration - Rise);
		return Peak * FMath::Square(1.f - Fall);
	}

	int32 CalcFlipToPageCount(int32 FromSlot, int32 ToSlot, int32 PagesPerSlot, int32 MaxPages)
	{
		if (FromSlot == INDEX_NONE || ToSlot == INDEX_NONE || FromSlot == ToSlot)
		{
			return 0;
		}
		const int32 Max = FMath::Max(MaxPages, 1);
		const int32 Pages = (ToSlot - FromSlot) * FMath::Max(PagesPerSlot, 1);
		return FMath::Clamp(Pages, -Max, Max);
	}

	float CalcRapidFlipDuration(int32 PageCount, float TotalDuration, float MinDuration, float MaxDuration)
	{
		const int32 Count = FMath::Max(FMath::Abs(PageCount), 1);
		return FMath::Clamp(TotalDuration / Count, MinDuration, FMath::Max(MinDuration, MaxDuration));
	}

	float CalcChannelFlipInterval(float FlipRate)
	{
		return FlipRate > 0.f ? 1.f / FlipRate : 0.f;
	}

	bool ShouldStowGrimoire(const FGrimGrimoireStowQuery& Query)
	{
		if (!Query.bHasWand || Query.bSwimming || Query.bBuildMode || Query.bCarrying)
		{
			return true;
		}
		if (Query.bMounted)
		{
			const bool bInCombat = Query.bAiming || Query.SecondsSinceCombat < Query.MountedCombatHoldSeconds;
			return !bInCombat;
		}
		return false;
	}

	float StepStowAlpha(float Current, bool bStow, float DeltaSeconds, float BlendSeconds)
	{
		const float Target = bStow ? 1.f : 0.f;
		if (BlendSeconds <= 0.f)
		{
			return Target;
		}
		return FMath::FInterpConstantTo(Current, Target, DeltaSeconds, 1.f / BlendSeconds);
	}

	EGrimCastMotion CastMotionFromDeliveryName(const FString& DeliveryName)
	{
		// "EGrimSpellDelivery::Bolt" 처럼 열거형 이름이 붙어 와도 끝 이름만 본다.
		FString Name = DeliveryName;
		int32 Colon = INDEX_NONE;
		if (Name.FindLastChar(TEXT(':'), Colon))
		{
			Name.RightChopInline(Colon + 1);
		}

		if (Name.Equals(TEXT("Bolt"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Projectile"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Lob"), ESearchCase::IgnoreCase))
		{
			return EGrimCastMotion::Flick;
		}
		if (Name.Equals(TEXT("Stream"), ESearchCase::IgnoreCase))
		{
			return EGrimCastMotion::Extend;
		}
		if (Name.Equals(TEXT("Cone"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Beam"), ESearchCase::IgnoreCase))
		{
			return EGrimCastMotion::Sweep;
		}
		return EGrimCastMotion::Auto;
	}

	EGrimCastMotion CastMotionFromElementName(FName ElementTagName)
	{
		const FString Name = ElementTagName.ToString();
		if (Name == TEXT("Element.Water"))
		{
			return EGrimCastMotion::Extend;
		}
		if (Name == TEXT("Element.Wind"))
		{
			return EGrimCastMotion::Sweep;
		}
		return EGrimCastMotion::Flick;
	}

	FVector ApplyAimPitch(const FVector& Direction, float PitchDegrees)
	{
		// +X(왼쪽) 축으로 +각 회전하면 앞(+Y) 이 위(+Z) 로 든다.
		return FQuat(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(PitchDegrees)).RotateVector(Direction);
	}

	FVector ApplyBodyYaw(const FVector& Direction, float YawDegrees)
	{
		// +Z 축으로 +각 회전하면 앞(+Y) 이 오른쪽(−X) 으로 돈다.
		return FQuat(FVector(0.f, 0.f, 1.f), FMath::DegreesToRadians(YawDegrees)).RotateVector(Direction);
	}

	FGrimSpellPoseFrame EvaluateCastPose(EGrimCastMotion Motion, float Elapsed, float ReleaseElapsed, float AimPitchDegrees)
	{
		FGrimSpellPoseFrame Frame;
		const float T = FMath::Max(Elapsed, 0.f);
		const float Pitch = FMath::Clamp(AimPitchDegrees, -60.f, 60.f);

		switch (Motion)
		{
		case EGrimCastMotion::Flick:
		{
			// 완드를 들었다(손목을 젖혔다) 앞으로 튕긴다.
			Frame.RightArmWeight = Ramp(T, 0.f, FlickAttack) * (1.f - Ramp(T, FlickReleaseStart, FlickEnd));
			const FVector Upper = (AxisRight * 0.35f + AxisForward - AxisUp * 0.25f).GetSafeNormal();
			const FVector Cocked = (AxisRight * 0.15f + AxisForward * 0.55f + AxisUp * 0.8f).GetSafeNormal();
			const FVector Snapped = (AxisRight * 0.08f + AxisForward + AxisUp * 0.05f).GetSafeNormal();
			const float Snap = Ramp(T, 0.03f, 0.13f);
			Frame.RightUpperArmDir = ApplyAimPitch(Upper, Pitch * 0.6f);
			Frame.RightForearmDir = ApplyAimPitch(FMath::Lerp(Cocked, Snapped, Snap).GetSafeNormal(), Pitch);
			// 튕기는 순간 상체가 살짝 따라 돈다.
			Frame.SpineYawDegrees = -6.f * FMath::Sin(UE_PI * FMath::Clamp(T / 0.3f, 0.f, 1.f));
			Frame.SpinePitchDegrees = 3.f * Frame.RightArmWeight;
			break;
		}
		case EGrimCastMotion::Extend:
		{
			// 앞으로 뻗고 버틴다. 놓으면 ExtendRelease 동안 돌아온다.
			const float Release = ReleaseElapsed < 0.f ? 1.f : 1.f - Ramp(ReleaseElapsed, 0.f, ExtendRelease);
			Frame.RightArmWeight = Ramp(T, 0.f, ExtendAttack) * Release;
			Frame.RightUpperArmDir = ApplyAimPitch((AxisRight * 0.2f + AxisForward).GetSafeNormal(), Pitch);
			Frame.RightForearmDir = ApplyAimPitch((AxisRight * 0.05f + AxisForward + AxisUp * 0.05f).GetSafeNormal(), Pitch);
			Frame.SpineYawDegrees = -4.f * Frame.RightArmWeight;
			Frame.SpinePitchDegrees = 4.f * Frame.RightArmWeight;
			break;
		}
		case EGrimCastMotion::Sweep:
		{
			// 오른쪽에서 왼쪽으로 크게 쓴다.
			Frame.RightArmWeight = Ramp(T, 0.f, SweepAttack) * (1.f - Ramp(T, SweepReleaseStart, SweepEnd));
			const float Yaw = FMath::Lerp(SweepFromYaw, SweepToYaw, Ramp(T, 0.05f, SweepReleaseStart));
			Frame.RightUpperArmDir = ApplyAimPitch(ApplyBodyYaw((AxisForward - AxisUp * 0.1f).GetSafeNormal(), Yaw), Pitch * 0.5f);
			Frame.RightForearmDir = ApplyAimPitch(ApplyBodyYaw(AxisForward, Yaw * 1.15f), Pitch * 0.5f);
			Frame.SpineYawDegrees = 0.15f * Yaw * Frame.RightArmWeight;
			Frame.SpinePitchDegrees = 2.f * Frame.RightArmWeight;
			break;
		}
		default:
			break;
		}
		return Frame;
	}

	void ApplyBookHoldPose(FGrimSpellPoseFrame& Frame, float Weight)
	{
		// 왼팔은 몸 옆으로 조금 내리고, 아래팔은 앞으로(살짝 안쪽 · 위) — 손바닥이 가슴 앞 왼쪽에 온다.
		const FVector Left(1.f, 0.f, 0.f);
		Frame.LeftHoldWeight = FMath::Clamp(Weight, 0.f, 1.f);
		Frame.LeftUpperArmDir = (Left * 0.25f + AxisForward * 0.3f - AxisUp).GetSafeNormal();
		Frame.LeftForearmDir = (AxisForward - Left * 0.25f + AxisUp * 0.25f).GetSafeNormal();
	}

	bool IsCastMotionFinished(EGrimCastMotion Motion, float Elapsed, float ReleaseElapsed)
	{
		switch (Motion)
		{
		case EGrimCastMotion::Flick:
			return Elapsed >= FlickEnd;
		case EGrimCastMotion::Extend:
			return ReleaseElapsed >= ExtendRelease;
		case EGrimCastMotion::Sweep:
			return Elapsed >= SweepEnd;
		default:
			return true;
		}
	}

	FVector CalcStreamPoint(const FVector& Origin, const FVector& Direction, float LaunchSpeed, float GravityZ, float Time)
	{
		return Origin + Direction.GetSafeNormal() * LaunchSpeed * Time + FVector(0.f, 0.f, 0.5f * GravityZ * Time * Time);
	}

	FVector CalcConeWaveSize(float Range, float HalfAngleDegrees)
	{
		const float Length = FMath::Max(Range, 1.f);
		const float Diameter = 2.f * Length * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 1.f, 80.f)));
		return FVector(Diameter, Diameter, Length);
	}
}
