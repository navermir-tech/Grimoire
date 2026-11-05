#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Net/Core/Connection/NetEnums.h"
#include "Online/GrimOnlineMath.h"
#include "Online/GrimSessionBackend.h"
#include "GrimSessionSubsystem.generated.h"

class AController;
class AGameModeBase;
class APlayerController;
class IGrimSessionBackend;
class UNetDriver;
class UWorld;
struct FUniqueNetIdRepl;

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimOnline, Log, All);

/** 이 프로세스가 지금 어디에 있나 */
UENUM(BlueprintType)
enum class EGrimConnectionState : uint8
{
	/** 메인 메뉴 */
	Menu,
	/** 맵을 여는 중(혼자 · 서버 열기) */
	Opening,
	/** 서버에 접속 중(IP 참가 · LAN) */
	Connecting,
	/** 클라이언트로 들어가 있음 */
	Connected,
	/** 리슨 서버(호스트 플레이) */
	Hosting,
	/** 혼자(리슨 아님) */
	Standalone
};

/** 이어하기 목록 한 줄 — 세이브 폴더를 읽기만 한다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	FString Slot;

	/** 세계 세이브를 읽었나(깨졌으면 false — 이어하면 세이브 시스템이 백업 · 새로 시작을 정한다) */
	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	bool bReadable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	int32 SaveCount = 0;

	/** 그 세계의 플레이어 파일 수(호스트 포함) */
	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|Online")
	int32 PlayerFiles = 0;
};

/**
 * 접속 · 세션 (docs/systems/world-multiplayer 「세션」) — 게임 인스턴스마다 하나, 맵을 옮겨도 남는다.
 *
 * - 열기: 게임 맵(설정 GameMap)을 「<맵>?listen?SaveSlot=<슬롯>?MaxPlayers=<N>」으로 연다(혼자는 ?listen 없이). 세이브는 슬롯으로 세이브 시스템이 한다.
 *   리슨 서버가 열리면 온라인 세션(OSS — 지금 Null 의 LAN)을 만든다 → LAN 찾기에 보인다.
 * - IP 참가: 「<ip>:<port>?GrimName=<이름>」으로 SetClientTravel — 콘솔 open 과 같은 길. 설정 시간 안에 못 들어가면 그만두고 메시지.
 * - 실패 · 끊김: 엔진 네트워크 오류 · 이동 실패 델리게이트 → 한국어 메시지를 남기고 메인 메뉴로(메뉴가 메시지를 보여 준다).
 * - 표시 이름: 서버가 로그인 URL 의 GrimName 을 읽어 PlayerState 이름을 바꾼다(세이브의 PlayerId 와 따로).
 * - Steam(R32): 백엔드 IGrimSessionBackend 를 바꾸면 친구 초대 · 「게임 참가」가 붙는다 — 지금은 자리만.
 * - 콘솔 Grim.Online.* (메뉴 함수를 명령줄 · 헤드리스 확인에서 부른다).
 */
UCLASS()
class GRIMOIRE_API UGrimSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UGrimSessionSubsystem* Get(const UObject* WorldContextObject);

	/** 이 프로세스의 모든 게임 인스턴스(콘솔용) */
	static void ForEach(TFunctionRef<void(UGrimSessionSubsystem&)> Fn);

	// ── 메뉴 함수 ──

	/** 새로 시작: 없는 슬롯만. bHost = 친구를 받는 리슨 서버 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool StartNewGame(const FString& Slot, bool bHost, int32 MaxPlayers, FText& OutError);

	/** 이어하기: 있는 슬롯만 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool ContinueGame(const FString& Slot, bool bHost, int32 MaxPlayers, FText& OutError);

	/** 서버 열기: 슬롯이 있으면 이어서, 없으면 새로 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool HostServer(const FString& Slot, int32 MaxPlayers, FText& OutError);

	/** 「주소[:포트]」로 참가 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool JoinByAddress(const FString& Address, FText& OutError);

	/** 주소 칸 · 포트 칸 따로(포트 칸이 비면 주소의 포트 → 기본 포트) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool JoinServer(const FString& Address, const FString& PortText, FText& OutError);

	/** 접속 중이면 그만둔다 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	void CancelConnect();

	/** LAN 찾기 시작(결과는 OnChanged 뒤 GetLANResults) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool FindLANSessions();

	bool JoinLANSession(int32 Index, FText& OutError);

	/** 메인 메뉴로. 호스트면 세이브 시스템이 맵을 떠날 때 저장하고 서버가 닫힌다(접속자는 끊김 메시지를 받는다) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	void ReturnToMainMenu();

	/** 게임 끄기. 호스트면 종료 저장(세이브 시스템 — quit 도 맵을 떠나는 길) 뒤에 꺼진다 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	void QuitGame();

	/** 친구 초대(Steam 자리). Null 이면 false 와 까닭 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Online")
	bool InviteFriends(FText& OutError);

	// ── 상태 ──

	UFUNCTION(BlueprintPure, Category = "Grimoire|Online")
	EGrimConnectionState GetState() const { return State; }

	/** 접속 중 · 접속한 주소 */
	const FString& GetTargetAddress() const { return TargetAddress; }

	/** 접속 중이면 몇 초째 */
	float GetConnectingSeconds() const;

	/** 서버(호스트 · 혼자)인가 */
	bool IsServer() const;

	/** 지금 맵의 접속 인원 · 최대(서버에서만 — 아니면 0) */
	void GetPlayerCounts(int32& OutPlayers, int32& OutMax) const;

	/** 지금 슬롯(서버에서만) */
	FString GetCurrentSlot() const;

	bool IsSearchingLAN() const { return bSearchingLAN; }
	const TArray<FGrimFoundSession>& GetLANResults() const { return LANResults; }

	bool SupportsFriendInvites() const;
	FName GetBackendName() const;

	/** 이 PC 의 주소(서버 열기 화면 안내) */
	static FString GetLocalAddressText();

	/** 서버 포트(명령줄 -port= → 설정 DefaultPort) */
	static int32 GetListenPort();

	// ── 메시지(메뉴가 보여 준다) ──

	bool HasMessage() const { return !Message.IsEmpty(); }
	const FText& GetMessage() const { return Message; }
	bool IsMessageError() const { return bMessageIsError; }
	void ClearMessage();
	void PostMessage(const FText& Text, bool bError);

	/** 상태 · 메시지 · LAN 결과가 바뀜 */
	FSimpleMulticastDelegate OnChanged;

	// ── 세이브 슬롯(읽기만) ──

	/** 세이브 폴더의 세계 슬롯들(최근 저장 먼저) */
	static TArray<FGrimSaveSlotInfo> ListSaveSlots();
	static bool DoesSlotExist(const FString& Slot);

	// ── 표시 이름 ──

	FString GetPlayerName() const;
	void SetPlayerName(const FString& Name);

	// ── 개발: 일이 생긴 뒤 몇 초에 명령 (헤드리스 확인) ──

	/** 이벤트 이름(Menu · Hosting · Connected · Standalone · Failed · Lost · Login · Logout · Rejected)과 그 이벤트의 몇 번째(1부터)에 맞춰 명령을 건다 */
	void ScheduleOnEvent(const FString& EventName, int32 Occurrence, float DelaySeconds, const FString& Command);

	/** 상태 · 메시지 · 인원을 로그로(Grim.Online.Status) */
	void DumpStatus() const;

	static const TCHAR* StateName(EGrimConnectionState InState);

private:
	struct FScheduled
	{
		FString Event;
		int32 Occurrence = 1;
		float Delay = 0.f;
		FString Command;
		double FireAt = -1.0;
	};

	bool OpenGameMap(const FString& Slot, bool bListen, int32 MaxPlayers, FText& OutError);
	bool TravelToAddress(const FGrimServerAddress& Address, FText& OutError);
	void CancelPendingConnection();
	void SetState(EGrimConnectionState NewState);
	void RaiseEvent(const FString& EventName);
	void RememberLastInput(const FString* Slot, const FString* Address, const int32* Port, const int32* MaxPlayers) const;
	UWorld* GetGameWorld() const;
	bool IsOurWorld(const UWorld* World) const;
	bool IsMainMenuWorld(const UWorld* World) const;

	bool Tick(float DeltaTime);
	void HandlePostLoadMap(UWorld* World);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void HandlePreLogin(AGameModeBase* GameMode, const FUniqueNetIdRepl& NewPlayer, FString& ErrorMessage);
	void HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void HandleLogout(AGameModeBase* GameMode, AController* Exiting);
	void CreateHostSessionFor(UWorld* World);
	void EnsureFailureReturnsToMenu();

	TSharedPtr<IGrimSessionBackend> Backend;
	EGrimConnectionState State = EGrimConnectionState::Menu;
	FString TargetAddress;
	double ConnectStartSeconds = 0.0;
	/** 우리가 떠나는 중(메인 메뉴로 · 나가기) — 이때 끊김은 메시지로 남기지 않는다 */
	bool bLeaving = false;
	/** 메뉴 · 콘솔로 서버를 열었다 — 리슨 맵이 열리면 세션을 만든다 */
	bool bHostRequested = false;
	FString RequestedSlot;
	bool bSearchingLAN = false;
	bool bDefaultMapRedirected = false;
	TArray<FGrimFoundSession> LANResults;

	FText Message;
	bool bMessageIsError = false;

	TMap<FString, int32> EventCounts;
	TArray<FScheduled> Scheduled;

	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle PreLoginHandle;
	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
};
