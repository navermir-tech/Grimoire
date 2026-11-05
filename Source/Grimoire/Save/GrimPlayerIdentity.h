#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "GrimPlayerIdentity.generated.h"

class APlayerState;

/**
 * 플레이어 세이브 Player_<PlayerId> 의 PlayerId (docs/systems/world-multiplayer 「세이브」 — PlayerId).
 *
 * OnlineSubsystemNull 의 UniqueNetId 는 첫 번째 인스턴스 · 에디터 밖일 때만 「호스트 이름-로그인 id」로 고정되고,
 * 에디터(PIE) · 같은 PC 의 두 번째 인스턴스에서는 실행마다 새 GUID 다(OnlineIdentityNull.cpp GenerateRandomUserId).
 * 그래서 각 PC 가 자기 GUID 를 한 번 만들어 GameUserSettings.ini 에 보관하고, 접속할 때 로그인 URL 옵션 GrimPlayerId=<id> 로 보낸다
 * (UGrimLocalPlayer::GetGameLoginOptions — 엔진이 NMT_Login 에 붙이고, 서버는 UNetConnection::RequestURL 에서 읽는다).
 * Steam · EOS 로 바꾸면 고정 UniqueNetId 를 쓰도록 ResolveServerSide 의 순서를 바꾼다.
 */
struct GRIMOIRE_API FGrimPlayerIdentity
{
	/** 로그인 URL 옵션 이름 */
	static const TCHAR* LoginOptionName;

	/**
	 * 이 PC 의 PlayerId. 순서: 명령줄 -GrimPlayerId=<id>(시험 · 같은 PC 에 여러 인스턴스) → GameUserSettings.ini 의 보관 값 → 새 GUID(보관).
	 * PIE 의 두 번째 창부터는 "-PIE<n>" 을 붙인다(한 프로세스의 PIE 창들이 같은 ini 를 쓰므로).
	 */
	static FString GetLocalPlayerId(const ULocalPlayer* LocalPlayer);

	/**
	 * 서버: 그 PlayerState 의 PlayerId. 로컬(호스트) → GetLocalPlayerId, 원격 → 로그인 URL 의 GrimPlayerId
	 * → 없으면 UniqueNetId 문자열("Net-…") → 그것도 없으면 플레이어 이름("Name-…"). 봇 · 컨트롤러 없음이면 빈 문자열.
	 */
	static FString ResolveServerSide(const APlayerState* PlayerState, FString* OutSource = nullptr);

	/** 로그인 URL(옵션 포함)에서 GrimPlayerId 를 꺼낸다(다듬은 값). 없으면 빈 문자열. */
	static FString ParseLoginOption(const FString& RequestURL);
};

/**
 * 로그인 때 GrimPlayerId 옵션을 보내는 로컬 플레이어. DefaultEngine.ini [/Script/Engine.Engine] LocalPlayerClassName 으로 쓴다.
 */
UCLASS()
class GRIMOIRE_API UGrimLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	virtual FString GetGameLoginOptions() const override;
};
