#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class UWorld;

/** LAN 찾기 · 친구 목록에서 찾은 서버 하나(백엔드가 채운다) */
struct GRIMOIRE_API FGrimFoundSession
{
	FString HostName;
	FString SlotName;
	int32 OpenSlots = 0;
	int32 MaxSlots = 0;
	int32 PingMs = 0;
	/** 백엔드 안의 결과 번호(JoinFoundSession 에 넘긴다) */
	int32 ResultIndex = INDEX_NONE;
};

/**
 * 온라인 세션 백엔드 — 세션 서브시스템(UGrimSessionSubsystem)이 이것만 부른다 (docs/systems/world-multiplayer 「세션」).
 *
 * 지금(M3): FGrimOSSSessionBackend 가 기본 온라인 서브시스템(DefaultEngine.ini [OnlineSubsystem] DefaultPlatformService=Null)의 세션 인터페이스로
 * LAN 세션을 만들고 찾는다. IP 참가는 세션 없이 바로 접속한다(콘솔 open 과 같은 길).
 * Steam 출시 때(R32): DefaultPlatformService=Steam + OnlineSubsystemSteam · SteamSockets 로 바꾸면 같은 백엔드가 Steam 세션을 쓴다 —
 * 친구 초대(ShowInviteUI · InviteFriend) · 친구 목록 「게임 참가」 · 초대 수락(OnInviteAccepted)은 자리만 있고, Null 에서는 false.
 * EOS 등 다른 길이 필요하면 이 인터페이스를 따로 구현한다.
 */
class GRIMOIRE_API IGrimSessionBackend
{
public:
	virtual ~IGrimSessionBackend() = default;

	/** 로그 · 화면용 이름(Null · Steam …) */
	virtual FName GetBackendName() const = 0;

	/** 세션 인터페이스가 있다 */
	virtual bool IsAvailable() const = 0;

	/** 리슨 서버가 열린 뒤 부른다(포트를 알아야 해서). 이미 있으면 지우고 다시 만든다. Done(성공) */
	virtual bool CreateHostSession(UWorld* World, int32 MaxPlayers, bool bLAN, const FString& SlotName, const FString& HostName, TFunction<void(bool)> Done) = 0;

	/** 이 프로세스의 게임 세션을 지운다(없으면 바로 Done(true)) */
	virtual void DestroySession(TFunction<void(bool)> Done) = 0;

	virtual bool HasSession() const = 0;

	/** LAN 찾기. Done(성공, 결과) */
	virtual bool FindSessions(bool bLAN, float TimeoutSeconds, int32 MaxResults, TFunction<void(bool, const TArray<FGrimFoundSession>&)> Done) = 0;

	/** 찾은 세션에 들어간다 → Done(성공, 접속 문자열 "ip:port") — 세션 서브시스템이 그 주소로 IP 참가와 같은 길을 탄다 */
	virtual bool JoinFoundSession(int32 ResultIndex, TFunction<void(bool, const FString&)> Done) = 0;

	// ── Steam 자리 (R32) ──

	/** 친구 초대가 되는 백엔드인가(Null = false) */
	virtual bool SupportsFriendInvites() const = 0;

	/** 오버레이의 친구 초대 창(Steam Shift+Tab) */
	virtual bool ShowInviteUI() = 0;

	/** 친구 하나 초대(친구 id 문자열 — Steam ID) */
	virtual bool InviteFriend(const FString& FriendId) = 0;

	/** 친구 목록 「게임 참가」 · 초대 수락 → 접속 문자열. 세션 서브시스템이 묶는다 */
	TFunction<void(const FString& ConnectString)> OnInviteAccepted;
};

/** 기본 온라인 서브시스템(IOnlineSubsystem::Get)의 세션 인터페이스로 구현 — Null 이면 LAN, Steam 이면 Steam 세션. */
TSharedRef<IGrimSessionBackend> MakeGrimOSSSessionBackend();
