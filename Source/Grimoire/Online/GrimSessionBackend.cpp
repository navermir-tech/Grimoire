#include "Online/GrimSessionBackend.h"
#include "Online/GrimSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"

namespace GrimSessionBackendLocal // 유니티 빌드 이름 충돌 방지
{
	const FName SlotKey(TEXT("GRIMSLOT"));
	const FName HostKey(TEXT("GRIMHOST"));

	/**
	 * 기본 온라인 서브시스템의 세션 인터페이스. 지금은 Null(LAN), Steam 출시 때 Steam.
	 * 세션 이름은 엔진 기본 NAME_GameSession 하나(한 프로세스에 게임 하나).
	 */
	class FGrimOSSSessionBackend final : public IGrimSessionBackend, public TSharedFromThis<FGrimOSSSessionBackend>
	{
	public:
		void Init()
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions)
			{
				UE_LOG(LogGrimOnline, Log, TEXT("[Online] 온라인 세션 인터페이스 없음 — LAN 찾기 · 친구 초대는 안 되고 IP 참가만 된다"));
				return;
			}
			TWeakPtr<FGrimOSSSessionBackend> Weak = AsShared();
			CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateLambda([Weak](FName Name, bool bOk)
			{
				if (TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin()) { Self->HandleCreateComplete(Name, bOk); }
			}));
			FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateLambda([Weak](bool bOk)
			{
				if (TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin()) { Self->HandleFindComplete(bOk); }
			}));
			JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateLambda([Weak](FName Name, EOnJoinSessionCompleteResult::Type Result)
			{
				if (TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin()) { Self->HandleJoinComplete(Name, Result); }
			}));
			InviteHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateLambda(
				[Weak](const bool bOk, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& Result)
			{
				if (TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin()) { Self->HandleInviteAccepted(bOk, Result); }
			}));
			UE_LOG(LogGrimOnline, Log, TEXT("[Online] 세션 백엔드: %s"), *GetBackendName().ToString());
		}

		virtual ~FGrimOSSSessionBackend() override
		{
			if (IOnlineSessionPtr Sessions = GetSessions())
			{
				Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
				Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
				Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
				Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
			}
		}

		virtual FName GetBackendName() const override
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			return OSS ? OSS->GetSubsystemName() : FName(TEXT("None"));
		}

		virtual bool IsAvailable() const override { return GetSessions().IsValid(); }

		virtual bool HasSession() const override
		{
			IOnlineSessionPtr Sessions = GetSessions();
			return Sessions && Sessions->GetNamedSession(NAME_GameSession) != nullptr;
		}

		virtual bool CreateHostSession(UWorld* World, int32 MaxPlayers, bool bLAN, const FString& SlotName, const FString& HostName, TFunction<void(bool)> Done) override
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions)
			{
				return false;
			}
			FOnlineSessionSettings Settings;
			Settings.NumPublicConnections = FMath::Max(1, MaxPlayers);
			Settings.bIsLANMatch = bLAN;
			Settings.bShouldAdvertise = true;
			Settings.bAllowJoinInProgress = true;
			// Null 은 존재(presence) · 로비가 없다. Steam 이면 친구 목록 「게임 참가」가 존재 · 로비를 쓴다.
			const bool bNull = IsNull();
			Settings.bUsesPresence = !bNull;
			Settings.bAllowJoinViaPresence = !bNull;
			Settings.bUseLobbiesIfAvailable = !bNull;
			Settings.Set(SlotKey, SlotName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			Settings.Set(HostKey, HostName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			CreateDone = MoveTemp(Done);
			auto DoCreate = [Weak = TWeakPtr<FGrimOSSSessionBackend>(AsShared()), Settings]()
			{
				TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin();
				IOnlineSessionPtr S = Self ? Self->GetSessions() : nullptr;
				if (!S || !S->CreateSession(0, NAME_GameSession, Settings))
				{
					if (Self) { Self->FinishCreate(false); }
				}
			};
			if (Sessions->GetNamedSession(NAME_GameSession))
			{
				Sessions->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateLambda([DoCreate](FName, bool) { DoCreate(); }));
			}
			else
			{
				DoCreate();
			}
			return true;
		}

		virtual void DestroySession(TFunction<void(bool)> Done) override
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions || !Sessions->GetNamedSession(NAME_GameSession))
			{
				if (Done) { Done(true); }
				return;
			}
			Sessions->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateLambda([Done](FName, bool bOk)
			{
				if (Done) { Done(bOk); }
			}));
		}

		virtual bool FindSessions(bool bLAN, float TimeoutSeconds, int32 MaxResults, TFunction<void(bool, const TArray<FGrimFoundSession>&)> Done) override
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions || Search.IsValid())
			{
				return false;
			}
			Search = MakeShared<FOnlineSessionSearch>();
			Search->bIsLanQuery = bLAN;
			Search->MaxSearchResults = FMath::Max(1, MaxResults);
			Search->TimeoutInSeconds = TimeoutSeconds;
			FindDone = MoveTemp(Done);
			if (!Sessions->FindSessions(0, Search.ToSharedRef()))
			{
				Search.Reset();
				FindDone = nullptr;
				return false;
			}
			return true;
		}

		virtual bool JoinFoundSession(int32 ResultIndex, TFunction<void(bool, const FString&)> Done) override
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions || !LastResults.IsValidIndex(ResultIndex))
			{
				return false;
			}
			return JoinResult(LastResults[ResultIndex], MoveTemp(Done));
		}

		virtual bool SupportsFriendInvites() const override
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			return OSS && !IsNull() && OSS->GetExternalUIInterface().IsValid();
		}

		virtual bool ShowInviteUI() override
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			IOnlineExternalUIPtr UI = (OSS && !IsNull()) ? OSS->GetExternalUIInterface() : nullptr;
			return UI && HasSession() && UI->ShowInviteUI(0, NAME_GameSession);
		}

		virtual bool InviteFriend(const FString& FriendId) override
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			IOnlineSessionPtr Sessions = GetSessions();
			IOnlineIdentityPtr Identity = OSS ? OSS->GetIdentityInterface() : nullptr;
			if (!Sessions || !Identity || IsNull() || !HasSession())
			{
				return false;
			}
			const FUniqueNetIdPtr Friend = Identity->CreateUniquePlayerId(FriendId);
			return Friend.IsValid() && Sessions->SendSessionInviteToFriend(0, NAME_GameSession, *Friend);
		}

	private:
		static IOnlineSessionPtr GetSessions()
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			return OSS ? OSS->GetSessionInterface() : nullptr;
		}

		static bool IsNull()
		{
			const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
			return !OSS || OSS->GetSubsystemName() == NULL_SUBSYSTEM;
		}

		void FinishCreate(bool bOk)
		{
			TFunction<void(bool)> Done = MoveTemp(CreateDone);
			CreateDone = nullptr;
			if (Done) { Done(bOk); }
		}

		void HandleCreateComplete(FName Name, bool bOk)
		{
			if (Name != NAME_GameSession)
			{
				return;
			}
			if (bOk)
			{
				if (IOnlineSessionPtr Sessions = GetSessions())
				{
					Sessions->StartSession(NAME_GameSession);
				}
			}
			FinishCreate(bOk);
		}

		void HandleFindComplete(bool bOk)
		{
			TArray<FGrimFoundSession> Found;
			LastResults.Reset();
			if (Search.IsValid())
			{
				LastResults = Search->SearchResults;
				for (int32 Index = 0; Index < LastResults.Num(); ++Index)
				{
					const FOnlineSessionSearchResult& Result = LastResults[Index];
					FGrimFoundSession& Entry = Found.AddDefaulted_GetRef();
					Entry.ResultIndex = Index;
					Entry.HostName = Result.Session.OwningUserName;
					Result.Session.SessionSettings.Get(HostKey, Entry.HostName);
					Result.Session.SessionSettings.Get(SlotKey, Entry.SlotName);
					Entry.MaxSlots = Result.Session.SessionSettings.NumPublicConnections;
					Entry.OpenSlots = Result.Session.NumOpenPublicConnections;
					Entry.PingMs = Result.PingInMs;
				}
			}
			Search.Reset();
			TFunction<void(bool, const TArray<FGrimFoundSession>&)> Done = MoveTemp(FindDone);
			FindDone = nullptr;
			if (Done) { Done(bOk, Found); }
		}

		bool JoinResult(const FOnlineSessionSearchResult& Result, TFunction<void(bool, const FString&)> Done)
		{
			IOnlineSessionPtr Sessions = GetSessions();
			if (!Sessions)
			{
				return false;
			}
			JoinDone = MoveTemp(Done);
			auto DoJoin = [Weak = TWeakPtr<FGrimOSSSessionBackend>(AsShared()), Result]()
			{
				TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin();
				IOnlineSessionPtr S = Self ? Self->GetSessions() : nullptr;
				if (!S || !S->JoinSession(0, NAME_GameSession, Result))
				{
					if (Self) { Self->FinishJoin(false, FString()); }
				}
			};
			// 지난 세션(메뉴로 돌아오기 전 것)이 남아 있으면 지우고 들어간다.
			if (Sessions->GetNamedSession(NAME_GameSession))
			{
				Sessions->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateLambda([DoJoin](FName, bool) { DoJoin(); }));
			}
			else
			{
				DoJoin();
			}
			return true;
		}

		void FinishJoin(bool bOk, const FString& Connect)
		{
			TFunction<void(bool, const FString&)> Done = MoveTemp(JoinDone);
			JoinDone = nullptr;
			if (Done) { Done(bOk, Connect); }
		}

		void HandleJoinComplete(FName Name, EOnJoinSessionCompleteResult::Type Result)
		{
			if (Name != NAME_GameSession)
			{
				return;
			}
			FString Connect;
			IOnlineSessionPtr Sessions = GetSessions();
			const bool bOk = Result == EOnJoinSessionCompleteResult::Success && Sessions && Sessions->GetResolvedConnectString(NAME_GameSession, Connect);
			FinishJoin(bOk, Connect);
		}

		void HandleInviteAccepted(bool bOk, const FOnlineSessionSearchResult& Result)
		{
			if (!bOk || !Result.IsValid())
			{
				return;
			}
			UE_LOG(LogGrimOnline, Display, TEXT("[Online] 초대 수락 · 게임 참가 — 세션에 들어간다"));
			TWeakPtr<FGrimOSSSessionBackend> Weak = AsShared();
			JoinResult(Result, [Weak](bool bJoined, const FString& Connect)
			{
				TSharedPtr<FGrimOSSSessionBackend> Self = Weak.Pin();
				if (Self && bJoined && Self->OnInviteAccepted)
				{
					Self->OnInviteAccepted(Connect);
				}
			});
		}

		TSharedPtr<FOnlineSessionSearch> Search;
		TArray<FOnlineSessionSearchResult> LastResults;
		TFunction<void(bool)> CreateDone;
		TFunction<void(bool, const TArray<FGrimFoundSession>&)> FindDone;
		TFunction<void(bool, const FString&)> JoinDone;
		FDelegateHandle CreateHandle;
		FDelegateHandle FindHandle;
		FDelegateHandle JoinHandle;
		FDelegateHandle InviteHandle;
	};
}

TSharedRef<IGrimSessionBackend> MakeGrimOSSSessionBackend()
{
	TSharedRef<GrimSessionBackendLocal::FGrimOSSSessionBackend> Backend = MakeShared<GrimSessionBackendLocal::FGrimOSSSessionBackend>();
	Backend->Init();
	return Backend;
}
