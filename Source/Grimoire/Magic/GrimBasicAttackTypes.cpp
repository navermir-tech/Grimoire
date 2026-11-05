#include "Magic/GrimBasicAttackTypes.h"

bool FGrimBasicAttackMath::IsReady(double Now, double LastAttackTime, float Interval, float Tolerance)
{
	return Now - LastAttackTime >= static_cast<double>(FMath::Max(Interval - FMath::Max(Tolerance, 0.f), 0.f)) - KINDA_SMALL_NUMBER;
}

int32 FGrimBasicAttackMath::NextStep(const FGrimBasicAttackCombo& Combo, double Now, float Window, int32 Length)
{
	const int32 SafeLength = FMath::Max(Length, 1);
	if (Combo.Step <= 0 || Combo.Step >= SafeLength || Now - Combo.ChainStartTime > static_cast<double>(Window))
	{
		return 1;
	}
	return Combo.Step + 1;
}

int32 FGrimBasicAttackMath::Register(FGrimBasicAttackCombo& Combo, double Now, float Window, int32 Length)
{
	const int32 Step = NextStep(Combo, Now, Window, Length);
	if (Step == 1)
	{
		Combo.ChainStartTime = Now;
	}
	Combo.Step = Step;
	Combo.LastAttackTime = Now;
	return Step;
}

bool FGrimBasicAttackMath::IsHeavy(int32 Step, int32 Length)
{
	return Length > 0 && Step == Length;
}

int32 FGrimBasicAttackMath::VisibleStep(const FGrimBasicAttackCombo& Combo, double Now, float Window)
{
	return Combo.Step > 0 && Now - Combo.ChainStartTime <= static_cast<double>(Window) ? Combo.Step : 0;
}

float FGrimBasicAttackMath::DamageMultiplier(int32 Step, int32 Length, float HeavyMultiplier)
{
	return IsHeavy(Step, Length) ? FMath::Max(HeavyMultiplier, 0.f) : 1.f;
}

FVector FGrimBasicAttackMath::KnockbackVelocity(const FVector& TravelDirection, float Speed, float UpSpeed)
{
	const FVector Flat = FVector(TravelDirection.X, TravelDirection.Y, 0.f).GetSafeNormal();
	return Flat * Speed + FVector(0.f, 0.f, UpSpeed);
}
