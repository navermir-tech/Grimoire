#include "Online/GrimSessionSubsystem.h"
#include "Online/GrimOnlineSettings.h"
#include "Online/GrimUserOptions.h"
#include "Save/GrimSaveGame.h"
#include "Save/GrimSaveRecords.h"
#include "Save/GrimSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMapsSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IPAddress.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SocketSubsystem.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogGrimOnline);

#define LOCTEXT_NAMESPACE "GrimOnline"

namespace GrimSessionSubsystemLocal // 유니티 빌드 이름 충돌 방지
{
	double Now()
	{
		return FPlatformTime::Seconds();
	}

	FString JoinArgs(const TArray<FString>& Args, int32 From)
	{
		FString Out;
		for (int32 Index = From; Index < Args.Num(); ++Index)
		{
			if (!Out.IsEmpty())
			{
				Out += TEXT(" ");
			}
			Out += Args[Index];
		}
		return Out;
	}

	void LogResult(bool bOk, const FText& Error)
	{
		if (!bOk)
		{
			UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 안 됨: %s"), *Error.ToString());
		}
	}

	FAutoConsoleCommand GHostCommand(TEXT("Grim.Online.Host"),
		TEXT("서버 열기(리슨). <슬롯> [최대 인원]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const FString Slot = Args.Num() > 0 ? Args[0] : FString();
			const int32 Max = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : UGrimOnlineSettings::Get()->DefaultMaxPlayers;
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session)
			{
				FText Error;
				LogResult(Session.HostServer(Slot, Max, Error), Error);
			});
		}));

	FAutoConsoleCommand GSoloCommand(TEXT("Grim.Online.Solo"),
		TEXT("혼자 시작 · 이어하기(리슨 아님). <슬롯>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const FString Slot = Args.Num() > 0 ? Args[0] : FString();
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session)
			{
				FText Error;
				const bool bOk = UGrimSessionSubsystem::DoesSlotExist(Slot)
					? Session.ContinueGame(Slot, false, 1, Error)
					: Session.StartNewGame(Slot, false, 1, Error);
				LogResult(bOk, Error);
			});
		}));

	FAutoConsoleCommand GJoinCommand(TEXT("Grim.Online.Join"),
		TEXT("IP 참가. <주소[:포트]> [포트]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const FString Address = Args.Num() > 0 ? Args[0] : FString();
			const FString Port = Args.Num() > 1 ? Args[1] : FString();
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session)
			{
				FText Error;
				LogResult(Session.JoinServer(Address, Port, Error), Error);
			});
		}));

	FAutoConsoleCommand GCancelCommand(TEXT("Grim.Online.Cancel"),
		TEXT("접속 그만두기"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session) { Session.CancelConnect(); });
		}));

	FAutoConsoleCommand GFindLANCommand(TEXT("Grim.Online.FindLAN"),
		TEXT("LAN 의 서버 찾기(OSS 세션)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session) { Session.FindLANSessions(); });
		}));

	FAutoConsoleCommand GJoinLANCommand(TEXT("Grim.Online.JoinLAN"),
		TEXT("LAN 찾기 결과로 참가. <번호(1부터)>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) - 1 : 0;
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session)
			{
				FText Error;
				LogResult(Session.JoinLANSession(Index, Error), Error);
			});
		}));

	FAutoConsoleCommand GLeaveCommand(TEXT("Grim.Online.Leave"),
		TEXT("메인 메뉴로(호스트면 저장 후 서버를 닫는다)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session) { Session.ReturnToMainMenu(); });
		}));

	FAutoConsoleCommand GQuitCommand(TEXT("Grim.Online.Quit"),
		TEXT("게임 끄기(호스트면 저장 후)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session) { Session.QuitGame(); });
		}));

	FAutoConsoleCommand GInviteCommand(TEXT("Grim.Online.Invite"),
		TEXT("친구 초대(Steam 자리 — Null 에서는 안 됨)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session)
			{
				FText Error;
				LogResult(Session.InviteFriends(Error), Error);
			});
		}));

	FAutoConsoleCommand GNameCommand(TEXT("Grim.Online.Name"),
		TEXT("표시 이름 정하기(GameUserSettings.ini 에 보관). <이름…> — 비우면 지금 이름"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session)
			{
				if (Args.Num() > 0)
				{
					Session.SetPlayerName(JoinArgs(Args, 0));
				}
				UE_LOG(LogGrimOnline, Display, TEXT("[Online] 표시 이름 = %s"), *Session.GetPlayerName());
			});
		}));

	FAutoConsoleCommand GSlotsCommand(TEXT("Grim.Online.Slots"),
		TEXT("이어하기 목록(세이브 폴더)"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			const TArray<FGrimSaveSlotInfo> Slots = UGrimSessionSubsystem::ListSaveSlots();
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] 세이브 슬롯 %d개"), Slots.Num());
			for (const FGrimSaveSlotInfo& Info : Slots)
			{
				UE_LOG(LogGrimOnline, Display, TEXT("[Online]   %s 읽음=%d 맵=%s 저장=%s 횟수=%d 플레이어=%d"),
					*Info.Slot, Info.bReadable ? 1 : 0, *Info.MapName, *Info.SavedAt.ToString(), Info.SaveCount, Info.PlayerFiles);
			}
		}));

	FAutoConsoleCommand GStatusCommand(TEXT("Grim.Online.Status"),
		TEXT("접속 상태 · 메시지 · 인원"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimSessionSubsystem::ForEach([](UGrimSessionSubsystem& Session) { Session.DumpStatus(); });
		}));

	FAutoConsoleCommand GWhenCommand(TEXT("Grim.Online.When"),
		TEXT("(개발) 일이 생긴 뒤 몇 초에 명령. <Menu|Hosting|Connected|Standalone|Failed|Lost|Login|Logout|Rejected>[번째] <초> <명령…> — 예: Grim.Online.When Menu2 3 quit"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (Args.Num() < 3)
			{
				UE_LOG(LogGrimOnline, Warning, TEXT("Grim.Online.When <이벤트[번째]> <초> <명령…>"));
				return;
			}
			FString Event = Args[0];
			int32 Digits = 0;
			while (Digits < Event.Len() && FChar::IsDigit(Event[Event.Len() - 1 - Digits]))
			{
				++Digits;
			}
			const int32 Occurrence = Digits > 0 ? FMath::Max(1, FCString::Atoi(*Event.Right(Digits))) : 1;
			Event.LeftChopInline(Digits);
			const float Delay = FCString::Atof(*Args[1]);
			const FString Command = JoinArgs(Args, 2);
			UGrimSessionSubsystem::ForEach([&](UGrimSessionSubsystem& Session) { Session.ScheduleOnEvent(Event, Occurrence, Delay, Command); });
		}));
}

// ───────────────────────── 수명 ─────────────────────────

void UGrimSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Backend = MakeGrimOSSSessionBackend();
	TWeakObjectPtr<UGrimSessionSubsystem> WeakThis(this);
	Backend->OnInviteAccepted = [WeakThis](const FString& Connect)
	{
		UGrimSessionSubsystem* Self = WeakThis.Get();
		FGrimServerAddress Address;
		FText Error;
		if (Self && FGrimOnlineMath::ParseAddress(Connect, UGrimOnlineSettings::Get()->DefaultPort, Address, &Error))
		{
			Self->TravelToAddress(Address, Error);
		}
	};

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.1f);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}
	PreLoginHandle = FGameModeEvents::GameModePreLoginEvent.AddUObject(this, &ThisClass::HandlePreLogin);
	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &ThisClass::HandlePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &ThisClass::HandleLogout);
}

void UGrimSessionSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	FGameModeEvents::GameModePreLoginEvent.Remove(PreLoginHandle);
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	if (Backend)
	{
		Backend->OnInviteAccepted = nullptr;
		Backend.Reset();
	}
	Super::Deinitialize();
}

UGrimSessionSubsystem* UGrimSessionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UGrimSessionSubsystem>() : nullptr;
}

void UGrimSessionSubsystem::ForEach(TFunctionRef<void(UGrimSessionSubsystem&)> Fn)
{
	if (!GEngine)
	{
		return;
	}
	TSet<const UGameInstance*> Seen;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		const UGameInstance* GameInstance = Context.OwningGameInstance;
		if (!GameInstance || Seen.Contains(GameInstance))
		{
			continue;
		}
		Seen.Add(GameInstance);
		if (UGrimSessionSubsystem* Session = GameInstance->GetSubsystem<UGrimSessionSubsystem>())
		{
			Fn(*Session);
		}
	}
}

UWorld* UGrimSessionSubsystem::GetGameWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

bool UGrimSessionSubsystem::IsOurWorld(const UWorld* World) const
{
	return World && World->GetGameInstance() == GetGameInstance();
}

bool UGrimSessionSubsystem::IsMainMenuWorld(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}
	const FString Package = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	return Package.Equals(UGrimOnlineSettings::Get()->GetMainMenuPackage(), ESearchCase::IgnoreCase);
}

const TCHAR* UGrimSessionSubsystem::StateName(EGrimConnectionState InState)
{
	switch (InState)
	{
	case EGrimConnectionState::Menu: return TEXT("메인 메뉴");
	case EGrimConnectionState::Opening: return TEXT("맵 여는 중");
	case EGrimConnectionState::Connecting: return TEXT("접속 중");
	case EGrimConnectionState::Connected: return TEXT("접속됨");
	case EGrimConnectionState::Hosting: return TEXT("호스트");
	case EGrimConnectionState::Standalone: return TEXT("혼자");
	}
	return TEXT("?");
}

void UGrimSessionSubsystem::SetState(EGrimConnectionState NewState)
{
	if (State != NewState)
	{
		State = NewState;
		OnChanged.Broadcast();
	}
}

// ───────────────────────── 메시지 ─────────────────────────

void UGrimSessionSubsystem::ClearMessage()
{
	if (!Message.IsEmpty())
	{
		Message = FText::GetEmpty();
		bMessageIsError = false;
		OnChanged.Broadcast();
	}
}

void UGrimSessionSubsystem::PostMessage(const FText& Text, bool bError)
{
	Message = Text;
	bMessageIsError = bError;
	OnChanged.Broadcast();
}

// ───────────────────────── 열기 · 참가 ─────────────────────────

bool UGrimSessionSubsystem::StartNewGame(const FString& Slot, bool bHost, int32 MaxPlayers, FText& OutError)
{
	if (!FGrimOnlineMath::ValidateSlotName(Slot, &OutError))
	{
		return false;
	}
	if (DoesSlotExist(Slot))
	{
		OutError = FText::Format(LOCTEXT("SlotExists", "「{0}」 슬롯이 이미 있다 — 이어하기에서 고르거나 다른 이름을 적어 주세요"), FText::FromString(Slot));
		return false;
	}
	return OpenGameMap(Slot, bHost, MaxPlayers, OutError);
}

bool UGrimSessionSubsystem::ContinueGame(const FString& Slot, bool bHost, int32 MaxPlayers, FText& OutError)
{
	if (!FGrimOnlineMath::ValidateSlotName(Slot, &OutError))
	{
		return false;
	}
	if (!DoesSlotExist(Slot))
	{
		OutError = FText::Format(LOCTEXT("SlotMissing", "「{0}」 슬롯이 없다"), FText::FromString(Slot));
		return false;
	}
	return OpenGameMap(Slot, bHost, MaxPlayers, OutError);
}

bool UGrimSessionSubsystem::HostServer(const FString& Slot, int32 MaxPlayers, FText& OutError)
{
	if (!FGrimOnlineMath::ValidateSlotName(Slot, &OutError))
	{
		return false;
	}
	return OpenGameMap(Slot, true, MaxPlayers, OutError);
}

bool UGrimSessionSubsystem::OpenGameMap(const FString& Slot, bool bListen, int32 MaxPlayers, FText& OutError)
{
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	UWorld* World = GetGameWorld();
	const FString Map = Settings->GetGameMapPackage();
	if (!World || !GEngine)
	{
		OutError = LOCTEXT("NoWorld", "아직 게임이 준비되지 않았다");
		return false;
	}
	if (Map.IsEmpty() || !FPackageName::DoesPackageExist(Map))
	{
		OutError = FText::Format(LOCTEXT("NoGameMap", "게임 맵 {0} 이 없다(설정 GameMap)"), FText::FromString(Map));
		return false;
	}
	if (State == EGrimConnectionState::Connecting || State == EGrimConnectionState::Opening)
	{
		OutError = LOCTEXT("Busy", "이미 여는 · 접속하는 중이다");
		return false;
	}

	const int32 Max = bListen ? FGrimOnlineMath::ClampMaxPlayers(MaxPlayers, Settings->MaxPlayersLimit) : 1;
	const FString URL = FGrimOnlineMath::MakeHostURL(Map, Slot, Max, bListen, GetPlayerName());
	bHostRequested = bListen;
	RequestedSlot = Slot;
	bLeaving = false;
	ClearMessage();
	RememberLastInput(&Slot, nullptr, nullptr, bListen ? &Max : nullptr);
	SetState(EGrimConnectionState::Opening);
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] %s: %s"), bListen ? TEXT("서버 열기") : TEXT("혼자 시작"), *URL);
	GEngine->SetClientTravel(World, *URL, TRAVEL_Absolute);
	return true;
}

bool UGrimSessionSubsystem::JoinByAddress(const FString& Address, FText& OutError)
{
	return JoinServer(Address, FString(), OutError);
}

bool UGrimSessionSubsystem::JoinServer(const FString& Address, const FString& PortText, FText& OutError)
{
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	FGrimServerAddress Parsed;
	if (!FGrimOnlineMath::ParseAddress(Address, Settings->DefaultPort, Parsed, &OutError))
	{
		return false;
	}
	if (!PortText.TrimStartAndEnd().IsEmpty())
	{
		int32 Port = 0;
		if (!FGrimOnlineMath::ParsePort(PortText, Settings->DefaultPort, Port, &OutError))
		{
			return false;
		}
		Parsed.Port = Port;
	}
	RememberLastInput(nullptr, &Parsed.Host, &Parsed.Port, nullptr);
	return TravelToAddress(Parsed, OutError);
}

bool UGrimSessionSubsystem::TravelToAddress(const FGrimServerAddress& Address, FText& OutError)
{
	UWorld* World = GetGameWorld();
	if (!World || !GEngine)
	{
		OutError = LOCTEXT("NoWorld", "아직 게임이 준비되지 않았다");
		return false;
	}
	if (State == EGrimConnectionState::Connecting || State == EGrimConnectionState::Opening)
	{
		OutError = LOCTEXT("Busy", "이미 여는 · 접속하는 중이다");
		return false;
	}
	const FString URL = FGrimOnlineMath::MakeJoinURL(Address, GetPlayerName());
	TargetAddress = FGrimOnlineMath::FormatAddress(Address);
	ConnectStartSeconds = GrimSessionSubsystemLocal::Now();
	bLeaving = false;
	ClearMessage();
	SetState(EGrimConnectionState::Connecting);
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 접속 시도: %s (제한 %.0f초)"), *TargetAddress, UGrimOnlineSettings::Get()->GetConnectTimeoutSeconds());
	GEngine->SetClientTravel(World, *URL, TRAVEL_Absolute);
	return true;
}

void UGrimSessionSubsystem::CancelConnect()
{
	if (State != EGrimConnectionState::Connecting)
	{
		return;
	}
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 접속 그만둠: %s"), *TargetAddress);
	SetState(EGrimConnectionState::Menu);
	CancelPendingConnection();
}

void UGrimSessionSubsystem::CancelPendingConnection()
{
	// 엔진의 CancelPending 은 protected — 콘솔 disconnect 와 같은 길(?closed)로 접속 대기를 버리고 메인 메뉴(기본 맵)를 다시 연다.
	UWorld* World = GetGameWorld();
	const FWorldContext* Context = GetGameInstance() ? GetGameInstance()->GetWorldContext() : nullptr;
	if (World && GEngine && Context && Context->PendingNetGame)
	{
		bLeaving = true;
		GEngine->SetClientTravel(World, TEXT("?closed"), TRAVEL_Absolute);
	}
}

bool UGrimSessionSubsystem::FindLANSessions()
{
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	if (!Backend || !Backend->IsAvailable())
	{
		PostMessage(LOCTEXT("NoSessions", "온라인 세션을 쓸 수 없다 — IP 참가를 써 주세요"), true);
		return false;
	}
	if (bSearchingLAN)
	{
		return true;
	}
	TWeakObjectPtr<UGrimSessionSubsystem> WeakThis(this);
	const bool bStarted = Backend->FindSessions(Settings->bLANSession, Settings->LANSearchSeconds, Settings->LANSearchMaxResults,
		[WeakThis](bool bOk, const TArray<FGrimFoundSession>& Found)
		{
			UGrimSessionSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->bSearchingLAN = false;
			Self->LANResults = Found;
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] LAN 찾기 끝: %d개%s"), Found.Num(), bOk ? TEXT("") : TEXT(" (실패)"));
			for (int32 Index = 0; Index < Found.Num(); ++Index)
			{
				const FGrimFoundSession& Entry = Found[Index];
				UE_LOG(LogGrimOnline, Display, TEXT("[Online]   %d. %s · 슬롯 %s · 빈자리 %d/%d · %dms"),
					Index + 1, *Entry.HostName, *Entry.SlotName, Entry.OpenSlots, Entry.MaxSlots, Entry.PingMs);
			}
			Self->OnChanged.Broadcast();
		});
	if (!bStarted)
	{
		PostMessage(LOCTEXT("FindFailed", "LAN 찾기를 시작하지 못했다"), true);
		return false;
	}
	bSearchingLAN = true;
	LANResults.Reset();
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] LAN 찾기 시작(%s)"), *Backend->GetBackendName().ToString());
	OnChanged.Broadcast();
	return true;
}

bool UGrimSessionSubsystem::JoinLANSession(int32 Index, FText& OutError)
{
	if (!Backend || !LANResults.IsValidIndex(Index))
	{
		OutError = LOCTEXT("NoSuchLAN", "그 서버가 목록에 없다 — 다시 찾아 주세요");
		return false;
	}
	TWeakObjectPtr<UGrimSessionSubsystem> WeakThis(this);
	const FString Host = LANResults[Index].HostName;
	const bool bStarted = Backend->JoinFoundSession(LANResults[Index].ResultIndex, [WeakThis, Host](bool bOk, const FString& Connect)
	{
		UGrimSessionSubsystem* Self = WeakThis.Get();
		if (!Self)
		{
			return;
		}
		FGrimServerAddress Address;
		FText Error;
		if (!bOk || !FGrimOnlineMath::ParseAddress(Connect, UGrimOnlineSettings::Get()->DefaultPort, Address, &Error))
		{
			Self->PostMessage(FText::Format(LOCTEXT("LANJoinFailed", "{0} 의 세션에 들어가지 못했다"), FText::FromString(Host)), true);
			return;
		}
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] LAN 세션 %s → %s"), *Host, *Connect);
		if (!Self->TravelToAddress(Address, Error))
		{
			Self->PostMessage(Error, true);
		}
	});
	if (!bStarted)
	{
		OutError = LOCTEXT("LANJoinStart", "세션 참가를 시작하지 못했다");
	}
	return bStarted;
}

void UGrimSessionSubsystem::ReturnToMainMenu()
{
	UWorld* World = GetGameWorld();
	if (!World || !GEngine)
	{
		return;
	}
	bLeaving = true;
	if (State == EGrimConnectionState::Hosting)
	{
		int32 Players = 0, Max = 0;
		GetPlayerCounts(Players, Max);
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 호스트가 메인 메뉴로 — 세계를 저장하고 서버를 닫는다(접속 %d명)"), Players);
	}
	else
	{
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 메인 메뉴로 (%s)"), StateName(State));
	}
	if (Backend)
	{
		Backend->DestroySession(nullptr);
	}
	const FString Menu = UGrimOnlineSettings::Get()->GetMainMenuPackage();
	const bool bHasMenu = !Menu.IsEmpty() && FPackageName::DoesPackageExist(Menu);
	SetState(EGrimConnectionState::Opening);
	GEngine->SetClientTravel(World, bHasMenu ? *Menu : TEXT("?closed"), TRAVEL_Absolute);
}

void UGrimSessionSubsystem::QuitGame()
{
	UWorld* World = GetGameWorld();
	bLeaving = true;
	if (State == EGrimConnectionState::Hosting || State == EGrimConnectionState::Standalone)
	{
		int32 Players = 0, Max = 0;
		GetPlayerCounts(Players, Max);
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 호스트가 나간다 — 종료 저장 뒤 끈다(접속 %d명)"), Players);
	}
	else
	{
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 게임을 끈다 (%s)"), StateName(State));
	}
	if (GEngine)
	{
		GEngine->Exec(World, TEXT("quit"));
	}
}

bool UGrimSessionSubsystem::InviteFriends(FText& OutError)
{
	if (!Backend || !Backend->SupportsFriendInvites())
	{
		OutError = LOCTEXT("InviteSteam", "친구 초대는 Steam 출시 때 — 지금은 「IP 참가」에 이 PC 주소를 알려 주세요");
		return false;
	}
	if (State != EGrimConnectionState::Hosting)
	{
		OutError = LOCTEXT("InviteHostFirst", "서버를 연 뒤에 초대할 수 있다");
		return false;
	}
	if (!Backend->ShowInviteUI())
	{
		OutError = LOCTEXT("InviteUIFailed", "초대 창을 열지 못했다");
		return false;
	}
	return true;
}

// ───────────────────────── 상태 ─────────────────────────

float UGrimSessionSubsystem::GetConnectingSeconds() const
{
	return State == EGrimConnectionState::Connecting ? float(GrimSessionSubsystemLocal::Now() - ConnectStartSeconds) : 0.f;
}

bool UGrimSessionSubsystem::IsServer() const
{
	return State == EGrimConnectionState::Hosting || State == EGrimConnectionState::Standalone;
}

void UGrimSessionSubsystem::GetPlayerCounts(int32& OutPlayers, int32& OutMax) const
{
	OutPlayers = 0;
	OutMax = 0;
	const UWorld* World = GetGameWorld();
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	if (GameMode)
	{
		OutPlayers = GameMode->GetNumPlayers();
		OutMax = GameMode->GameSession ? GameMode->GameSession->MaxPlayers : 0;
	}
}

FString UGrimSessionSubsystem::GetCurrentSlot() const
{
	const UWorld* World = GetGameWorld();
	if (const UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(World); Save && Save->IsActive())
	{
		return Save->GetSlot();
	}
	const TCHAR* Option = World ? World->URL.GetOption(TEXT("SaveSlot="), nullptr) : nullptr;
	return Option ? FString(Option) : FString();
}

bool UGrimSessionSubsystem::SupportsFriendInvites() const
{
	return Backend && Backend->SupportsFriendInvites();
}

FName UGrimSessionSubsystem::GetBackendName() const
{
	return Backend ? Backend->GetBackendName() : NAME_None;
}

FString UGrimSessionSubsystem::GetLocalAddressText()
{
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return FString();
	}
	bool bCanBindAll = false;
	const TSharedRef<FInternetAddr> Address = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
	return Address->IsValid() ? Address->ToString(false) : FString();
}

int32 UGrimSessionSubsystem::GetListenPort()
{
	int32 Port = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("Port="), Port) && Port > 0)
	{
		return Port;
	}
	return FURL::UrlConfig.DefaultPort > 0 ? FURL::UrlConfig.DefaultPort : UGrimOnlineSettings::Get()->DefaultPort;
}

void UGrimSessionSubsystem::DumpStatus() const
{
	int32 Players = 0, Max = 0;
	GetPlayerCounts(Players, Max);
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 상태 %s · 주소 %s · 인원 %d/%d · 슬롯 %s · 이름 %s · 백엔드 %s · 세션 %d · 메시지 「%s」"),
		StateName(State), *TargetAddress, Players, Max, *GetCurrentSlot(), *GetPlayerName(), *GetBackendName().ToString(),
		(Backend && Backend->HasSession()) ? 1 : 0, *Message.ToString());
}

void UGrimSessionSubsystem::RememberLastInput(const FString* Slot, const FString* Address, const int32* Port, const int32* MaxPlayers) const
{
	FGrimUserOptions Options = FGrimUserOptions::Load();
	if (Slot) { Options.LastSlot = *Slot; }
	if (Address) { Options.LastAddress = *Address; }
	if (Port) { Options.LastPort = *Port; }
	if (MaxPlayers) { Options.LastMaxPlayers = *MaxPlayers; }
	Options.Save();
}

// ───────────────────────── 세이브 슬롯(읽기만) ─────────────────────────

bool UGrimSessionSubsystem::DoesSlotExist(const FString& Slot)
{
	if (!FGrimOnlineMath::ValidateSlotName(Slot))
	{
		return false;
	}
	const FString Path = FPaths::Combine(UGrimSaveSubsystem::GetSlotDirectoryFor(Slot), FString::Printf(TEXT("WorldSave_%s.sav"), *Slot));
	IFileManager& Files = IFileManager::Get();
	return Files.FileExists(*Path) || Files.FileExists(*FGrimSaveFiles::GetBackupPath(Path));
}

TArray<FGrimSaveSlotInfo> UGrimSessionSubsystem::ListSaveSlots()
{
	TArray<FGrimSaveSlotInfo> Result;
	IFileManager& Files = IFileManager::Get();
	const FString Root = UGrimSaveSubsystem::GetSlotDirectoryFor(FString());
	TArray<FString> Directories;
	Files.FindFiles(Directories, *FPaths::Combine(Root, TEXT("*")), false, true);
	for (const FString& Slot : Directories)
	{
		if (!DoesSlotExist(Slot))
		{
			continue;
		}
		const FString Directory = UGrimSaveSubsystem::GetSlotDirectoryFor(Slot);
		const FString Path = FPaths::Combine(Directory, FString::Printf(TEXT("WorldSave_%s.sav"), *Slot));
		FGrimSaveSlotInfo& Info = Result.AddDefaulted_GetRef();
		Info.Slot = Slot;

		bool bUsedBackup = false;
		bool bAnyFile = false;
		TArray<FString> Problems;
		if (const UGrimSaveGame* SaveGame = FGrimSaveFiles::LoadWithFallback(Path, bUsedBackup, bAnyFile, Problems))
		{
			Info.bReadable = true;
			Info.MapName = SaveGame->MapName;
			Info.SavedAt = SaveGame->SavedAtUtc;
			Info.SaveCount = SaveGame->SaveCount;
		}
		else
		{
			Info.SavedAt = Files.GetTimeStamp(*Path);
		}

		TArray<FString> PlayerFiles;
		Files.FindFiles(PlayerFiles, *FPaths::Combine(Directory, TEXT("Player_*.sav")), true, false);
		Info.PlayerFiles = PlayerFiles.Num();
	}
	Result.Sort([](const FGrimSaveSlotInfo& A, const FGrimSaveSlotInfo& B) { return A.SavedAt > B.SavedAt; });
	return Result;
}

// ───────────────────────── 표시 이름 ─────────────────────────

FString UGrimSessionSubsystem::GetPlayerName() const
{
	return FGrimUserOptions::Load().GetEffectivePlayerName();
}

void UGrimSessionSubsystem::SetPlayerName(const FString& Name)
{
	FGrimUserOptions Options = FGrimUserOptions::Load();
	Options.PlayerName = FGrimOnlineMath::SanitizePlayerName(Name, UGrimOnlineSettings::Get()->PlayerNameMaxLength);
	Options.bNameFromCommandLine = false;
	Options.Save();
	OnChanged.Broadcast();
}

// ───────────────────────── 예약 명령(개발) ─────────────────────────

void UGrimSessionSubsystem::ScheduleOnEvent(const FString& EventName, int32 Occurrence, float DelaySeconds, const FString& Command)
{
	FScheduled& Entry = Scheduled.AddDefaulted_GetRef();
	Entry.Event = EventName;
	Entry.Occurrence = FMath::Max(1, Occurrence);
	Entry.Delay = FMath::Max(0.f, DelaySeconds);
	Entry.Command = Command;
	const int32* Count = EventCounts.Find(EventName.ToLower());
	if (Count && *Count >= Entry.Occurrence)
	{
		Entry.FireAt = GrimSessionSubsystemLocal::Now() + Entry.Delay;
	}
}

void UGrimSessionSubsystem::RaiseEvent(const FString& EventName)
{
	int32& Count = EventCounts.FindOrAdd(EventName.ToLower());
	++Count;
	for (FScheduled& Entry : Scheduled)
	{
		if (Entry.FireAt < 0.0 && Entry.Occurrence == Count && Entry.Event.Equals(EventName, ESearchCase::IgnoreCase))
		{
			Entry.FireAt = GrimSessionSubsystemLocal::Now() + Entry.Delay;
		}
	}
}

bool UGrimSessionSubsystem::Tick(float DeltaTime)
{
	const double Time = GrimSessionSubsystemLocal::Now();

	// 접속 제한 시간
	if (State == EGrimConnectionState::Connecting && Time - ConnectStartSeconds > UGrimOnlineSettings::Get()->GetConnectTimeoutSeconds())
	{
		const FText Text = FText::Format(LOCTEXT("ConnectTimeout", "{0} 에 {1}초 안에 들어가지 못했다 — 주소 · 포트 · 방화벽 · 서버가 열렸는지 확인해 주세요"),
			FText::FromString(TargetAddress), FText::AsNumber(FMath::RoundToInt(UGrimOnlineSettings::Get()->GetConnectTimeoutSeconds())));
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 접속 실패: %s"), *Text.ToString());
		PostMessage(Text, true);
		SetState(EGrimConnectionState::Menu);
		CancelPendingConnection();
		RaiseEvent(TEXT("Failed"));
	}

	// 예약 명령
	for (int32 Index = 0; Index < Scheduled.Num();)
	{
		if (Scheduled[Index].FireAt >= 0.0 && Time >= Scheduled[Index].FireAt)
		{
			const FScheduled Entry = Scheduled[Index];
			Scheduled.RemoveAt(Index);
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] 예약 명령(%s#%d +%.1f초): %s"), *Entry.Event, Entry.Occurrence, Entry.Delay, *Entry.Command);
			if (GEngine)
			{
				GEngine->Exec(GetGameWorld(), *Entry.Command);
			}
			continue;
		}
		++Index;
	}
	return true;
}

// ───────────────────────── 엔진 이벤트 ─────────────────────────

void UGrimSessionSubsystem::EnsureFailureReturnsToMenu()
{
	if (bDefaultMapRedirected || GIsEditor || !UGrimOnlineSettings::Get()->bReturnToMainMenuOnFailure)
	{
		return;
	}
	bDefaultMapRedirected = true;
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	const FString Menu = Settings->GetMainMenuPackage();
	if (Menu.IsEmpty() || !FPackageName::DoesPackageExist(Menu))
	{
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 메인 메뉴 맵 %s 이 없다 — 실패 · 끊김 때 엔진 기본 맵으로 돌아간다"), *Menu);
		return;
	}
	if (!UGameMapsSettings::GetGameDefaultMap().Equals(Menu, ESearchCase::IgnoreCase))
	{
		UGameMapsSettings::SetGameDefaultMap(Settings->MainMenuMap.ToString());
		UE_LOG(LogGrimOnline, Log, TEXT("[Online] 실패 · 끊김 때 돌아갈 기본 맵을 이 프로세스에서만 %s 로 둔다(ini 의 GameDefaultMap 은 그대로)"), *Menu);
	}
}

void UGrimSessionSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!IsOurWorld(World) || !World->IsGameWorld())
	{
		return;
	}
	EnsureFailureReturnsToMenu();
	bLeaving = false;
	const FString MapName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());

	switch (World->GetNetMode())
	{
	case NM_Client:
		SetState(EGrimConnectionState::Connected);
		ClearMessage();
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 접속됨: %s (맵 %s, %.1f초)"), *TargetAddress, *MapName, float(GrimSessionSubsystemLocal::Now() - ConnectStartSeconds));
		RaiseEvent(TEXT("Connected"));
		break;

	case NM_ListenServer:
	case NM_DedicatedServer:
	{
		SetState(EGrimConnectionState::Hosting);
		int32 Port = GetListenPort();
		if (const UNetDriver* NetDriver = World->GetNetDriver())
		{
			if (const TSharedPtr<const FInternetAddr> Local = const_cast<UNetDriver*>(NetDriver)->GetLocalAddr(); Local.IsValid() && Local->GetPort() > 0)
			{
				Port = Local->GetPort();
			}
		}
		int32 Players = 0, Max = 0;
		GetPlayerCounts(Players, Max);
		TargetAddress = FString::Printf(TEXT("%s:%d"), *GetLocalAddressText(), Port);
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 서버 열림: 맵 %s · 주소 %s · 최대 %d명 · 슬롯 %s"), *MapName, *TargetAddress, Max, *GetCurrentSlot());
		if (bHostRequested && UGrimOnlineSettings::Get()->bCreateOnlineSession)
		{
			CreateHostSessionFor(World);
		}
		bHostRequested = false;
		RaiseEvent(TEXT("Hosting"));
		break;
	}

	case NM_Standalone:
	default:
		bHostRequested = false;
		if (IsMainMenuWorld(World))
		{
			SetState(EGrimConnectionState::Menu);
			if (Backend && Backend->HasSession())
			{
				Backend->DestroySession(nullptr);
			}
			if (HasMessage())
			{
				UE_LOG(LogGrimOnline, Display, TEXT("[Online] 메인 메뉴 — 메시지: %s"), *Message.ToString());
			}
			else
			{
				UE_LOG(LogGrimOnline, Display, TEXT("[Online] 메인 메뉴"));
			}
			RaiseEvent(TEXT("Menu"));
		}
		else
		{
			SetState(EGrimConnectionState::Standalone);
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] 혼자: 맵 %s · 슬롯 %s"), *MapName, *GetCurrentSlot());
			RaiseEvent(TEXT("Standalone"));
		}
		break;
	}
}

void UGrimSessionSubsystem::CreateHostSessionFor(UWorld* World)
{
	if (!Backend || !Backend->IsAvailable())
	{
		return;
	}
	int32 Players = 0, Max = 0;
	GetPlayerCounts(Players, Max);
	const FString Slot = GetCurrentSlot();
	const bool bStarted = Backend->CreateHostSession(World, Max > 0 ? Max : UGrimOnlineSettings::Get()->DefaultMaxPlayers, UGrimOnlineSettings::Get()->bLANSession,
		Slot, GetPlayerName(), [Backend = Backend](bool bOk)
		{
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] 온라인 세션(%s) %s"), Backend ? *Backend->GetBackendName().ToString() : TEXT("?"),
				bOk ? TEXT("만듦 — LAN 찾기에 보인다") : TEXT("만들지 못함 — IP 참가는 된다"));
		});
	if (!bStarted)
	{
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 온라인 세션을 만들지 못했다 — IP 참가는 된다"));
	}
}

void UGrimSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	bool bOurs = IsOurWorld(World);
	if (!bOurs && NetDriver && GEngine)
	{
		const FWorldContext* Context = GEngine->GetWorldContextFromPendingNetGameNetDriver(NetDriver);
		bOurs = Context && Context->OwningGameInstance == GetGameInstance();
	}
	if (!bOurs)
	{
		return;
	}
	const bool bPendingDriver = NetDriver && NetDriver->NetDriverName == NAME_PendingNetDriver;

	if (State == EGrimConnectionState::Connecting && (bPendingDriver || !World || World->GetNetMode() != NM_ListenServer))
	{
		const FText Text = FGrimOnlineMath::DescribeNetworkFailure(FailureType, ErrorString, true);
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 접속 실패: %s — %s [%s]"), *TargetAddress, *Text.ToString(), ENetworkFailure::ToString(FailureType));
		PostMessage(Text, true);
		SetState(EGrimConnectionState::Menu);
		RaiseEvent(TEXT("Failed"));
		return;
	}
	if (State == EGrimConnectionState::Connected && !bLeaving)
	{
		const FText Text = FGrimOnlineMath::DescribeNetworkFailure(FailureType, ErrorString, false);
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 접속 끊김: %s — %s [%s]"), *TargetAddress, *Text.ToString(), ENetworkFailure::ToString(FailureType));
		PostMessage(Text, true);
		SetState(EGrimConnectionState::Opening);
		RaiseEvent(TEXT("Lost"));
		return;
	}
	if (FailureType == ENetworkFailure::NetDriverListenFailure)
	{
		const FText Text = FGrimOnlineMath::DescribeNetworkFailure(FailureType, ErrorString, false);
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] %s"), *Text.ToString());
		PostMessage(Text, true);
		return;
	}
	UE_LOG(LogGrimOnline, Log, TEXT("[Online] 네트워크 이벤트 %s: %s (상태 %s)"), ENetworkFailure::ToString(FailureType), *ErrorString, StateName(State));
}

void UGrimSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (!IsOurWorld(World))
	{
		return;
	}
	const FText Text = FGrimOnlineMath::DescribeTravelFailure(FailureType, ErrorString);
	if (State == EGrimConnectionState::Connecting)
	{
		UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 접속 실패: %s — %s [%s]"), *TargetAddress, *Text.ToString(), ETravelFailure::ToString(FailureType));
		PostMessage(Text, true);
		SetState(EGrimConnectionState::Menu);
		RaiseEvent(TEXT("Failed"));
		return;
	}
	UE_LOG(LogGrimOnline, Warning, TEXT("[Online] 이동 실패: %s [%s]"), *Text.ToString(), ETravelFailure::ToString(FailureType));
	PostMessage(Text, true);
	if (State == EGrimConnectionState::Opening)
	{
		SetState(EGrimConnectionState::Menu);
	}
}

void UGrimSessionSubsystem::HandlePreLogin(AGameModeBase* GameMode, const FUniqueNetIdRepl& NewPlayer, FString& ErrorMessage)
{
	if (!GameMode || !IsOurWorld(GameMode->GetWorld()) || ErrorMessage.IsEmpty())
	{
		return;
	}
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 접속 거절: %s"), *ErrorMessage);
	RaiseEvent(TEXT("Rejected"));
}

void UGrimSessionSubsystem::HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (!GameMode || !NewPlayer || !IsOurWorld(GameMode->GetWorld()))
	{
		return;
	}
	const int32 MaxLen = UGrimOnlineSettings::Get()->PlayerNameMaxLength;
	const bool bLocal = NewPlayer->IsLocalController();
	FString Name;
	if (bLocal)
	{
		Name = GetPlayerName();
	}
	else if (const UNetConnection* Connection = NewPlayer->GetNetConnection())
	{
		Name = FGrimOnlineMath::ParseNameOption(Connection->RequestURL, MaxLen);
	}
	if (!Name.IsEmpty())
	{
		GameMode->ChangeName(NewPlayer, Name, false);
	}
	int32 Players = 0, Max = 0;
	GetPlayerCounts(Players, Max);
	const APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>();
	const FString Shown = PlayerState ? PlayerState->GetPlayerName() : Name;
	if (bLocal)
	{
		UE_LOG(LogGrimOnline, Log, TEXT("[Online] 로컬 플레이어 이름: %s"), *Shown);
		return;
	}
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 접속: %s (%s) — %d/%d명"), *Shown, Name.IsEmpty() ? TEXT("이름 옵션 없음") : TEXT("GrimName"), Players, Max);
	RaiseEvent(TEXT("Login"));
}

void UGrimSessionSubsystem::HandleLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (!GameMode || !IsOurWorld(GameMode->GetWorld()))
	{
		return;
	}
	const APlayerController* PC = Cast<APlayerController>(Exiting);
	if (!PC || PC->IsLocalController())
	{
		return;
	}
	const APlayerState* PlayerState = PC->GetPlayerState<APlayerState>();
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 나감: %s"), PlayerState ? *PlayerState->GetPlayerName() : TEXT("?"));
	RaiseEvent(TEXT("Logout"));
}

#undef LOCTEXT_NAMESPACE
