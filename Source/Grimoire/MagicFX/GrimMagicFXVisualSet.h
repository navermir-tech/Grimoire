#pragma once

#include "CoreMinimal.h"
#include "Core/GrimSpellFXTypes.h"
#include "MagicFX/GrimMagicFXTypes.h"
#include "GrimMagicFXVisualSet.generated.h"

/**
 * 연출 묶음의 MagicFX 확장. 공통 기반 UGrimSpellVisualSet(파티클 · 소리 · 흔들림 · 속성색)에
 * 임시 연출용 색 펄스 · 자국 · 소리 세기 · 줄기 그림 값을 더한다.
 * 마법 쪽(UGrimSpellDefinition::VisualSet)은 부모 형으로 가리키므로 이 자식을 그대로 넣으면 된다.
 * 에셋(VS_*)은 Scripts/MagicFX/make_magicfx_assets.py 가 만든다 — 값은 스크립트가 정본이다.
 */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimMagicFXVisualSet : public UGrimSpellVisualSet
{
	GENERATED_BODY()

public:
	/** Auto 면 마법의 전달 방식(Delivery) → 속성 순으로 고른다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	EGrimCastMotion CastMotion = EGrimCastMotion::Auto;

	/** 시전 한 번의 책장 넘김 시간(초). 0 = 책의 기본(FlipDuration). 기본 공격은 연타 리듬에 맞춰 짧게(0.15) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0", Units = "s"))
	float PageFlipDuration = 0.f;

	/** 완드 끝 색 펄스(섬광 덩어리 + 빛) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	FGrimFXPulseSpec MuzzlePulse;

	/** 시전 때 앞으로 퍼지는 파동(원뿔 마법). Cone 모양이면 크기는 마법의 원뿔 사거리 · 반각을 따른다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	FGrimFXPulseSpec CastWave;

	/** 맞은 곳 색 펄스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	FGrimFXPulseSpec ImpactPulse;

	/** 맞은 곳 자국(그을음 · 젖음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FGrimImpactDecalSpec ImpactDecal;

	/** 파티클 크기 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0.01"))
	float MuzzleFlashScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0.01"))
	float TrailScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0.01"))
	float ImpactScale = 1.f;

	/** 한 번 터지는 파티클을 이 시간 뒤 멈춘다(엔진 템플릿 중 반복하는 것이 있어서). 0 = 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0", Units = "s"))
	float OneShotStopSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0"))
	float CastSoundVolume = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.1"))
	float CastSoundPitch = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0"))
	float ImpactSoundVolume = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.1"))
	float ImpactSoundPitch = 1.f;

	/** 맞은 소리를 이 시간 뒤 줄여 끈다(긴 임시 소리용). 0 = 끝까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0", Units = "s"))
	float ImpactSoundDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0"))
	float LoopSoundVolume = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.1"))
	float LoopSoundPitch = 1.f;

	/** 시전자 화면 흔들림 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0"))
	float CameraShakeScale = 1.f;

	/** 줄기(Stream) 몸통의 물방울 구슬 수. 0 = 구슬 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0", ClampMax = "32"))
	int32 StreamBeadCount = 0;

	/** 구슬 지름(cm) — 끝으로 갈수록 이 값의 1.6배까지 커진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0.1", Units = "cm"))
	float StreamBeadSize = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0"))
	float StreamBeadGlow = 3.f;

	/** 줄기를 그리는 비행 시간(초). 마법 정의에 줄기 값이 없으면 아래 기본 속도 · 중력과 같이 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0.05", Units = "s"))
	float StreamFlightTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0", Units = "cm/s"))
	float StreamDefaultLaunchSpeed = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stream", meta = (ClampMin = "0"))
	float StreamDefaultGravityScale = 1.f;
};
