#include "MagicFX/GrimSpellCameraShake.h"

void UGrimKickShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UGrimKickShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	ElapsedTime = 0.f;
	bStopped = false;
}

void UGrimKickShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime += Params.DeltaTime;
	Evaluate(ElapsedTime, Params.GetTotalScale(), OutResult);
}

void UGrimKickShakePattern::ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime = Params.AbsoluteTime;
	Evaluate(ElapsedTime, Params.GetTotalScale(), OutResult);
}

bool UGrimKickShakePattern::IsFinishedImpl() const
{
	return bStopped || ElapsedTime >= Duration;
}

void UGrimKickShakePattern::StopShakePatternImpl(const FCameraShakePatternStopParams& Params)
{
	bStopped = true;
}

void UGrimKickShakePattern::Evaluate(float Time, float Scale, FCameraShakePatternUpdateResult& OutResult) const
{
	if (Time >= Duration || Duration <= 0.f)
	{
		return;
	}
	// 감쇠 사인: 처음이 가장 세고 Duration 에서 0.
	const float Decay = FMath::Square(1.f - Time / Duration);
	const float Wave = FMath::Sin(UE_TWO_PI * Frequency * Time);
	OutResult.Rotation = FRotator(PitchAmplitude * Wave * Decay, YawAmplitude * FMath::Cos(UE_TWO_PI * Frequency * 0.7f * Time) * Decay, 0.f);
	OutResult.Location = FVector(-PushBack * Decay, 0.f, 0.f);
	// 기본 플래그: 부모가 배율 · 플레이 공간을 적용한다. Scale 은 부모가 곱하므로 여기서는 쓰지 않는다.
	(void)Scale;
}

UGrimSpellCameraShake::UGrimSpellCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGrimKickShakePattern>(TEXT("RootShakePattern")))
{
	bSingleInstance = true;
}
