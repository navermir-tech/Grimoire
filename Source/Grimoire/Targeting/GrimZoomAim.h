#pragma once

#include "CoreMinimal.h"
#include "Core/GrimTargetingTypes.h"

/**
 * 줌인 조준(R24)의 순수 계산. 월드 · 액터를 모르고 값만 다뤄 자동화 테스트(Grimoire.Targeting.Zoom.*)가 직접 부른다.
 * 트레이스 · 카메라 · 폰 조절은 UGrimTargetingComponent 가 하고 판단만 여기서 한다.
 */
struct GRIMOIRE_API FGrimZoomAimMath
{
	/** 줌 · 대상 · 고정 → 조준 모드. 줌 중이면 대상이 있든 없든 Zoom(자동 대상 없음 — 대상은 조준점 아래 것) */
	static EGrimAimMode DecideAimMode(bool bZoomActive, bool bHasTarget, bool bLocked);
};

/**
 * 「누르는 동안 / 토글」 입력 상태(호그와트 레거시의 조준 옵션).
 * 누르기 방식: 눌림 = 켬, 뗌 = 끔. 토글 방식: 눌림 = 뒤집기, 뗌 = 그대로.
 */
struct GRIMOIRE_API FGrimZoomInputState
{
	bool bActive = false;

	/** 눌림. 바뀌었으면 true */
	bool Press(bool bToggleMode);

	/** 뗌. 바뀌었으면 true */
	bool Release(bool bToggleMode);

	/** 폰을 잃는 등 — 끈다. 바뀌었으면 true */
	bool Cancel();
};

/** 카메라 당기기 보간 값(0 = 평소, 1 = 줌). 선형으로 움직이고 쓸 때는 SmoothStep 으로 부드럽게. */
struct GRIMOIRE_API FGrimZoomBlend
{
	float Alpha = 0.f;

	/** bZoomIn 쪽으로 BlendSeconds 에 걸쳐 움직인다. BlendSeconds ≤ 0 이면 바로 */
	void Tick(bool bZoomIn, float DeltaSeconds, float BlendSeconds);

	float GetEased() const { return FMath::SmoothStep(0.f, 1.f, Alpha); }

	bool IsFullyOut() const { return Alpha <= 0.f; }
	bool IsFullyIn() const { return Alpha >= 1.f; }
};

/**
 * 줌 동안 폰 이동 속력(MaxWalkSpeed)을 줄이고 끝나면 되돌린다.
 * 줌 중에 다른 시스템이 값을 바꾸면(무게 · 추위 감속 등) 그 값을 새 기준으로 삼고, 줌 속력은 기준보다 빠르게 하지 않는다.
 */
struct GRIMOIRE_API FGrimZoomSpeedOverride
{
	bool bActive = false;
	float Saved = 0.f;
	float Applied = 0.f;

	/** 줌 시작. 폰에 넣을 값 = min(줌 속력, 지금 값) */
	float Begin(float Current, float ZoomValue);

	/** 줌 중 매 틱. 누가 바꿨으면 기준을 옮긴다. 폰에 넣을 값 */
	float Update(float Current, float ZoomValue);

	/** 줌 끝. 되돌릴 값(줌 중 마지막에 누가 바꿨으면 그 값을 그대로 둔다) */
	float End(float Current);
};

/**
 * 줌 동안 폰의 bool 설정(이동 방향 보기 끄기 · 컨트롤 방향 보기 켜기)을 바꾸고 되돌린다.
 * 캐릭터의 「시전 때 몸 돌리기」도 bOrientRotationToMovement 를 잠깐 껐다가 되돌린다 — 둘이 겹쳐도 꼬이지 않게
 *  - 줌 중에 누가 값을 바꾸면 그것을 되돌릴 값으로 기억하고 다시 줌 값을 넣는다,
 *  - 되돌린 뒤 GuardSeconds 가 끝날 때 값이 「줌 값」으로 돌아가 있으면(줌 중에 시작한 시전 돌리기가 줌 값을 기억했다가
 *    끝나며 넣은 것) 한 번 바로잡는다. 줌 값이 아닌 값은 다른 쪽의 정당한 값으로 보고 둔다.
 */
struct GRIMOIRE_API FGrimZoomFlagOverride
{
	bool bActive = false;
	bool Saved = false;
	bool Applied = false;
	bool Restored = false;
	float GuardRemaining = 0.f;

	bool Begin(bool Current, bool ZoomValue);
	bool Update(bool Current);
	bool End(bool Current, float GuardSeconds);

	/** 되돌린 뒤의 감시. 감시가 끝나는 순간 값이 되돌린 값과 다르면 true 와 바로잡을 값 */
	bool TickGuard(bool Current, float DeltaSeconds, bool& OutValue);

	bool IsGuarding() const { return !bActive && GuardRemaining > 0.f; }
};
