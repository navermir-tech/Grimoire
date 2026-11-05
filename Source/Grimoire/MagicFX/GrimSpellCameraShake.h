#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "GrimSpellCameraShake.generated.h"

/** 짧게 튀었다 잦아드는 흔들림(감쇠 사인). 시전 · 큰 피격의 「손맛」용 — 아주 약하게. */
UCLASS(meta = (DisplayName = "Grim Kick Shake Pattern"))
class GRIMOIRE_API UGrimKickShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Kick", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.16f;

	/** 위아래(피치) 최대 각도 */
	UPROPERTY(EditAnywhere, Category = "Kick", meta = (Units = "deg"))
	float PitchAmplitude = 0.35f;

	/** 좌우(요) 최대 각도 */
	UPROPERTY(EditAnywhere, Category = "Kick", meta = (Units = "deg"))
	float YawAmplitude = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Kick", meta = (ClampMin = "0.1", Units = "Hz"))
	float Frequency = 18.f;

	/** 앞뒤로 살짝 밀림(cm) */
	UPROPERTY(EditAnywhere, Category = "Kick", meta = (Units = "cm"))
	float PushBack = 0.4f;

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual void ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;
	virtual void StopShakePatternImpl(const FCameraShakePatternStopParams& Params) override;

private:
	void Evaluate(float Time, float Scale, FCameraShakePatternUpdateResult& OutResult) const;

	float ElapsedTime = 0.f;
	bool bStopped = false;
};

/** 시전 흔들림. VisualSet 의 CameraShake 에 넣는다(시전자 화면만). */
UCLASS()
class GRIMOIRE_API UGrimSpellCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UGrimSpellCameraShake(const FObjectInitializer& ObjectInitializer);
};
