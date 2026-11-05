#pragma once

#include "CoreMinimal.h"

class APlayerController;

/**
 * 플레이어가 메뉴에서 고르는 값 — 이 PC 의 GameUserSettings.ini [/Script/Grimoire.GrimUserOptions] (세이브 · PlayerId 와 따로).
 *
 * - 표시 이름: 접속할 때 로그인 URL 옵션 GrimName=<이름> 으로 서버에 가고, 서버가 그 PlayerState 이름을 바꾼다(UGrimSessionSubsystem).
 *   명령줄 -GrimName=<이름> 이 보관 값보다 앞선다(시험 · 같은 PC 의 여러 인스턴스 — 보관하지 않는다).
 * - 조준 옵션(Q18 · R24): 조준 갈래의 함수(UGrimTargetingComponent::SetSoftLockEnabled · SetZoomToggleMode · SetZoomLookSensitivityScale)를 부르기만 한다.
 *   보관 값이 없으면 조준 설정(UGrimTargetingSettings)의 기본값.
 * - 시점 감도: 컨트롤러의 시점 입력 배율(엔진 InputYaw/PitchScale — DefaultInput.ini bEnableLegacyInputScales=True 일 때)을 클래스 기본값 × 감도로.
 */
struct GRIMOIRE_API FGrimUserOptions
{
	FString PlayerName;
	bool bSoftLockEnabled = true;
	bool bZoomAimToggle = false;
	float ZoomLookSensitivity = 0.6f;
	float LookSensitivity = 1.f;

	/** 메뉴가 기억하는 마지막 입력(편의) */
	FString LastSlot;
	FString LastAddress;
	int32 LastPort = 0;
	int32 LastMaxPlayers = 0;

	/** 이름이 명령줄 -GrimName= 에서 왔다(보관하지 않는다) */
	bool bNameFromCommandLine = false;

	/** 보관 값(없으면 기본값). 표시 이름은 명령줄 -GrimName= → 보관 값 → PC 사용자 이름 → 설정 FallbackPlayerName. */
	static FGrimUserOptions Load();

	/** GameUserSettings.ini 에 쓴다(명령줄로 덮은 이름은 쓰지 않는다). */
	void Save() const;

	/** 다듬은 표시 이름(비지 않음) */
	FString GetEffectivePlayerName() const;

	/** 로컬 컨트롤러에 적용 — 조준 컴포넌트가 BeginPlay 로 기본값을 넣은 뒤에 부른다. 적용했으면 true. */
	bool ApplyTo(APlayerController* PC) const;

	/** 이 프로세스의 모든 로컬 컨트롤러에 지금 보관 값을 적용 */
	static void ApplyToAllLocalPlayers();
};
