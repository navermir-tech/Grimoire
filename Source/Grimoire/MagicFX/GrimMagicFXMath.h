#pragma once

#include "CoreMinimal.h"
#include "MagicFX/GrimMagicFXTypes.h"

/**
 * 연출 · 동작의 순수 계산 (자동화 테스트 Grimoire.MagicFX.*).
 * 월드 · 액터를 모른다 — 책 · 애님 인스턴스 · 큐 처리기가 이 함수들에 값을 넣고 결과를 그린다.
 */
namespace GrimMagicFXMath
{
	// ---- 그리모어 떠다니기 (R21) ----

	/** 오르내림 오프셋(책 기준, Z 만). 주기 BobPeriod 의 사인. */
	GRIMOIRE_API FVector CalcBookBobOffset(float TimeSeconds, const FGrimBookFloatParams& Params);

	/** 살짝 기울기. 흔들림과 다른 주기라 같은 자세가 잘 반복되지 않는다. 각 성분 |값| ≤ TiltDegrees. */
	GRIMOIRE_API FRotator CalcBookBobTilt(float TimeSeconds, const FGrimBookFloatParams& Params);

	// ---- 책장 넘김 ----

	/** 넘김 진행 0~1 (부드럽게 시작 · 끝). Duration ≤ 0 이면 바로 1. */
	GRIMOIRE_API float CalcFlipProgress(float Elapsed, float Duration);

	/**
	 * 넘기는 판의 회전(책 기준). 판은 책등(+X 축)을 축으로 돈다.
	 * Direction > 0: 오른쪽 면(0°) → 왼쪽 면(180°), 넘기는 동안 위로 들린다. Direction < 0: 거꾸로.
	 */
	GRIMOIRE_API FQuat CalcFlipPageRotation(float Progress, int32 Direction);

	/** 넘길 때 글자 면 발광: 빠르게 Peak 까지 올랐다가 GlowDuration 동안 0 으로. */
	GRIMOIRE_API float CalcFlipGlow(float SinceFlipStart, float GlowDuration, float Peak);

	/**
	 * 마법을 바꿀 때 넘길 장 수(부호 = 방향). 슬롯 하나에 PagesPerSlot 장, 최대 MaxPages 장.
	 * FromSlot 이 INDEX_NONE(처음)이면 0 — 처음 고른 마법으로는 넘기지 않는다.
	 */
	GRIMOIRE_API int32 CalcFlipToPageCount(int32 FromSlot, int32 ToSlot, int32 PagesPerSlot, int32 MaxPages);

	/** 촤르륵 넘김의 한 장 시간: 전체 시간을 장 수로 나누되 [MinDuration, MaxDuration]. */
	GRIMOIRE_API float CalcRapidFlipDuration(int32 PageCount, float TotalDuration, float MinDuration, float MaxDuration);

	/** 누르는 동안 넘기는 간격(초). FlipRate ≤ 0 이면 0(넘기지 않음). */
	GRIMOIRE_API float CalcChannelFlipInterval(float FlipRate);

	// ---- 거두기 (Q25) ----

	/** 헤엄 · 건축 · 나르기 · 완드 없음 → 거둔다. 탈것은 조준 중이거나 최근 시전이 Hold 시간 안이면 띄운다. */
	GRIMOIRE_API bool ShouldStowGrimoire(const FGrimGrimoireStowQuery& Query);

	/** 거두기 보간 값(0 = 손 위, 1 = 허리) 을 BlendSeconds 에 걸쳐 한 걸음 옮긴다. */
	GRIMOIRE_API float StepStowAlpha(float Current, bool bStow, float DeltaSeconds, float BlendSeconds);

	// ---- 시전 동작 ----

	/** 전달 방식 이름 → 동작. Bolt · Projectile · Lob = 튕기기, Stream = 뻗고 유지, Cone · Beam = 쓸기, 모르면 Auto. */
	GRIMOIRE_API EGrimCastMotion CastMotionFromDeliveryName(const FString& DeliveryName);

	/** 속성 → 동작(전달 방식을 모를 때). 물 = 뻗기, 바람 = 쓸기, 그 밖 = 튕기기. */
	GRIMOIRE_API EGrimCastMotion CastMotionFromElementName(FName ElementTagName);

	/** 조준 높이 반영: 컴포넌트 공간 방향을 위(+Z) 쪽으로 PitchDegrees 만큼 든다(축 = 왼쪽 +X). */
	GRIMOIRE_API FVector ApplyAimPitch(const FVector& Direction, float PitchDegrees);

	/** 몸 기준 수평 회전: 양수 = 오른쪽(−X) 으로. */
	GRIMOIRE_API FVector ApplyBodyYaw(const FVector& Direction, float YawDegrees);

	/** 손 동작 한 프레임. ReleaseElapsed < 0 = 아직 놓지 않음(뻗고 유지에서만 뜻이 있다). */
	GRIMOIRE_API FGrimSpellPoseFrame EvaluateCastPose(EGrimCastMotion Motion, float Elapsed, float ReleaseElapsed, float AimPitchDegrees);

	/** 「책 받치기」 왼팔 방향을 Frame 에 채운다(Weight 0~1). */
	GRIMOIRE_API void ApplyBookHoldPose(FGrimSpellPoseFrame& Frame, float Weight);

	/** 그 동작이 끝났는가(무게가 0 으로 돌아옴). */
	GRIMOIRE_API bool IsCastMotionFinished(EGrimCastMotion Motion, float Elapsed, float ReleaseElapsed);

	/** 뻗고 유지: 줄기 큐가 없어도 이 시간까지는 뻗어 둔다. */
	constexpr float ExtendMinHoldSeconds = 0.4f;

	// ---- 줄기 · 원뿔 그림 ----

	/** 포물선 줄기의 t 초 뒤 위치. GravityZ 는 cm/s² (음수 = 아래). */
	GRIMOIRE_API FVector CalcStreamPoint(const FVector& Origin, const FVector& Direction, float LaunchSpeed, float GravityZ, float Time);

	/** 원뿔 파동 크기(cm): X · Y = 끝 지름, Z = 길이. */
	GRIMOIRE_API FVector CalcConeWaveSize(float Range, float HalfAngleDegrees);
}
