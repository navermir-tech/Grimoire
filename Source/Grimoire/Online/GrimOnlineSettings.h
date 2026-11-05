#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "GrimOnlineSettings.generated.h"

/**
 * 접속 · 메뉴 설정 (docs/systems/world-multiplayer 「세션」). 값은 DefaultGame.ini 의 [/Script/Grimoire.GrimOnlineSettings].
 * 플레이어가 고르는 것(표시 이름 · 조준 옵션)은 여기가 아니라 GameUserSettings.ini — FGrimUserOptions.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Grimoire Online"))
class GRIMOIRE_API UGrimOnlineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimOnlineSettings* Get() { return GetDefault<UGrimOnlineSettings>(); }

	/** 메인 메뉴 맵. 접속 실패 · 끊김 · 「메인 메뉴로」가 여기로 돌아온다 */
	UPROPERTY(config, EditAnywhere, Category = "Maps", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath MainMenuMap = FSoftObjectPath(TEXT("/Game/Grimoire/Maps/L_MainMenu.L_MainMenu"));

	/** 새로 시작 · 이어하기 · 서버 열기가 여는 게임 맵(오픈 베타 지형이 들어오면 L_OpenBeta 로) */
	UPROPERTY(config, EditAnywhere, Category = "Maps", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath GameMap = FSoftObjectPath(TEXT("/Game/Grimoire/Maps/L_Tower.L_Tower"));

	/**
	 * 실패 · 끊김 때 엔진은 기본 맵(GameDefaultMap)으로 돌아간다(?closed). 켜면 첫 맵을 연 뒤 이 프로세스에서만 기본 맵을 메인 메뉴로 바꿔 둔다
	 * (ini 는 고치지 않는다 — 시작 맵은 그대로). 기본 맵이 메인 메뉴가 되면 할 일이 없다. 에디터(PIE)에서는 하지 않는다.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Maps")
	bool bReturnToMainMenuOnFailure = true;

	/** 서버 포트 · 참가 포트 칸의 기본값(엔진 [URL] Port 와 같게). 서버 포트는 명령줄 -port= 가 앞선다 */
	UPROPERTY(config, EditAnywhere, Category = "Connect", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 DefaultPort = 7777;

	/** 서버 열기의 기본 인원 */
	UPROPERTY(config, EditAnywhere, Category = "Connect", meta = (ClampMin = "1"))
	int32 DefaultMaxPlayers = 8;

	/** 최대 동시 접속(Q11 — 8, 호스트 포함) */
	UPROPERTY(config, EditAnywhere, Category = "Connect", meta = (ClampMin = "1"))
	int32 MaxPlayersLimit = 8;

	/** IP 참가가 이 초 안에 들어가지 못하면 그만두고 메시지(엔진 기본 InitialConnectTimeout 보다 짧게). 명령줄 -GrimConnectTimeout= 가 앞선다 */
	UPROPERTY(config, EditAnywhere, Category = "Connect", meta = (ClampMin = "1", Units = "s"))
	float ConnectTimeoutSeconds = 20.f;

	/** 서버를 열면 온라인 세션(OSS)을 만든다 — LAN 찾기에 보인다. Steam 으로 바꾸면 친구 초대 · 「게임 참가」가 이 세션을 쓴다 */
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bCreateOnlineSession = true;

	/** 세션을 LAN 으로(OSS Null 은 LAN 만 된다). Steam 으로 바꾸면 끈다 */
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bLANSession = true;

	/** LAN 찾기 제한 시간 */
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "0.5", Units = "s"))
	float LANSearchSeconds = 3.f;

	/** LAN 찾기 결과 최대 수 */
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "1"))
	int32 LANSearchMaxResults = 20;

	/** 표시 이름 최대 글자 수(엔진이 로그인 이름을 20자로 자른다) */
	UPROPERTY(config, EditAnywhere, Category = "Player", meta = (ClampMin = "1", ClampMax = "20"))
	int32 PlayerNameMaxLength = 16;

	/** 표시 이름이 비었을 때(없으면 PC 사용자 이름) */
	UPROPERTY(config, EditAnywhere, Category = "Player")
	FString FallbackPlayerName = TEXT("마법사");

	/** 시점 감도 슬라이더 범위(평소 시점 입력 배율) */
	UPROPERTY(config, EditAnywhere, Category = "Options", meta = (ClampMin = "0.05"))
	FVector2D LookSensitivityRange = FVector2D(0.2, 3.0);

	/** 조준 감도(줌인 조준 중 배율) 슬라이더 범위 */
	UPROPERTY(config, EditAnywhere, Category = "Options", meta = (ClampMin = "0.05"))
	FVector2D ZoomSensitivityRange = FVector2D(0.1, 2.0);

	/** 혼자(단독 · 친구 없는 호스트 아님) 일 때 Esc 메뉴가 게임을 멈춘다. 협동 서버는 멈추지 않는다 */
	UPROPERTY(config, EditAnywhere, Category = "Menu")
	bool bPauseWhenStandalone = true;

	/** 연결 시간 초과(명령줄 우선) */
	float GetConnectTimeoutSeconds() const;

	FString GetMainMenuPackage() const;
	FString GetGameMapPackage() const;
};
