#include "Targeting/GrimZoomAim.h"

EGrimAimMode FGrimZoomAimMath::DecideAimMode(bool bZoomActive, bool bHasTarget, bool bLocked)
{
	if (bZoomActive)
	{
		return EGrimAimMode::Zoom;
	}
	if (!bHasTarget)
	{
		return EGrimAimMode::Free;
	}
	return bLocked ? EGrimAimMode::Locked : EGrimAimMode::Soft;
}

bool FGrimZoomInputState::Press(bool bToggleMode)
{
	const bool bOld = bActive;
	bActive = bToggleMode ? !bActive : true;
	return bOld != bActive;
}

bool FGrimZoomInputState::Release(bool bToggleMode)
{
	if (bToggleMode || !bActive)
	{
		return false;
	}
	bActive = false;
	return true;
}

bool FGrimZoomInputState::Cancel()
{
	const bool bOld = bActive;
	bActive = false;
	return bOld;
}

void FGrimZoomBlend::Tick(bool bZoomIn, float DeltaSeconds, float BlendSeconds)
{
	const float Target = bZoomIn ? 1.f : 0.f;
	if (BlendSeconds <= 0.f)
	{
		Alpha = Target;
		return;
	}
	const float Step = FMath::Max(0.f, DeltaSeconds) / BlendSeconds;
	Alpha = bZoomIn ? FMath::Min(1.f, Alpha + Step) : FMath::Max(0.f, Alpha - Step);
}

float FGrimZoomSpeedOverride::Begin(float Current, float ZoomValue)
{
	bActive = true;
	Saved = Current;
	Applied = FMath::Min(ZoomValue, Current);
	return Applied;
}

float FGrimZoomSpeedOverride::Update(float Current, float ZoomValue)
{
	if (!bActive)
	{
		return Current;
	}
	if (Current != Applied)
	{
		Saved = Current; // 다른 시스템이 바꿨다 — 새 기준
	}
	Applied = FMath::Min(ZoomValue, Saved);
	return Applied;
}

float FGrimZoomSpeedOverride::End(float Current)
{
	if (!bActive)
	{
		return Current;
	}
	bActive = false;
	return Current != Applied ? Current : Saved;
}

bool FGrimZoomFlagOverride::Begin(bool Current, bool ZoomValue)
{
	bActive = true;
	Saved = Current;
	Applied = ZoomValue;
	GuardRemaining = 0.f;
	return Applied;
}

bool FGrimZoomFlagOverride::Update(bool Current)
{
	if (!bActive)
	{
		return Current;
	}
	if (Current != Applied)
	{
		Saved = Current; // 줌 중에 누가 되돌렸다 — 그 값을 끝날 때 되돌릴 값으로
	}
	return Applied;
}

bool FGrimZoomFlagOverride::End(bool Current, float GuardSeconds)
{
	if (!bActive)
	{
		return Current;
	}
	bActive = false;
	Restored = Current != Applied ? Current : Saved;
	GuardRemaining = FMath::Max(0.f, GuardSeconds);
	return Restored;
}

bool FGrimZoomFlagOverride::TickGuard(bool Current, float DeltaSeconds, bool& OutValue)
{
	if (!IsGuarding())
	{
		return false;
	}
	GuardRemaining -= FMath::Max(0.f, DeltaSeconds);
	if (GuardRemaining > 0.f)
	{
		return false;
	}
	GuardRemaining = 0.f;
	if (Current != Restored && Current == Applied)
	{
		OutValue = Restored;
		return true;
	}
	return false;
}
