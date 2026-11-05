// 세이브 콘솔 명령 (docs/systems/world-multiplayer 「세이브」). 서버(리슨 호스트 · 싱글) 프로세스에서 친다 — 클라이언트에서는 알려 주기만 한다.
// 끝의 [지연초]는 헤드리스 -ExecCmds 용(맵 · 접속보다 먼저 실행되므로). Scripts/Save/check_save.ps1 이 쓴다.

#include "Save/GrimSaveSubsystem.h"
#include "Save/GrimPlayerSaveParticipants.h"
#include "Save/GrimSaveRecords.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

namespace GrimSaveDebugLocal // 유니티 빌드에서 다른 디버그 파일의 도우미와 부딪히지 않게 이름을 붙인다
{
	void ForEachGameWorldLater(float Delay, TFunction<void(UWorld*)> Fn)
	{
		auto RunNow = [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World(); World && World->IsGameWorld())
				{
					Fn(World);
				}
			}
		};
		if (Delay <= 0.f)
		{
			RunNow();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RunNow](float)
		{
			RunNow();
			return false;
		}), Delay);
	}

	float ArgFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	/** 서버 월드의 세이브 서브시스템. 아니면 이유를 로그로 남기고 nullptr. */
	UGrimSaveSubsystem* GetServerSave(UWorld* World, const TCHAR* Command)
	{
		UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(World);
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogGrimSave, Display, TEXT("[Client] %s: 세이브는 서버(호스트)가 한다 — 호스트 콘솔에서 친다"), Command);
			return nullptr;
		}
		if (!Save || !Save->IsActive())
		{
			UE_LOG(LogGrimSave, Display, TEXT("%s: 세이브 꺼짐(%s)"), Command, Save ? *Save->GetDisabledReason() : TEXT("서브시스템 없음"));
			return nullptr;
		}
		return Save;
	}

	void NowCommand(const TArray<FString>& Args)
	{
		ForEachGameWorldLater(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			if (UGrimSaveSubsystem* Save = GetServerSave(World, TEXT("Grim.Save.Now")))
			{
				Save->SaveAll(TEXT("콘솔"));
			}
		});
	}

	void LoadCommand(const TArray<FString>& Args)
	{
		ForEachGameWorldLater(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			if (UGrimSaveSubsystem* Save = GetServerSave(World, TEXT("Grim.Save.Load")))
			{
				Save->LoadAll(TEXT("콘솔"));
			}
		});
	}

	void DumpCommand(const TArray<FString>& Args)
	{
		ForEachGameWorldLater(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				UE_LOG(LogGrimSave, Display, TEXT("[Client] Grim.Save.Dump: 세이브는 서버가 한다"));
				return;
			}
			if (const UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(World))
			{
				Save->Dump();
			}
		});
	}

	void DeleteCommand(const TArray<FString>& Args)
	{
		const FString Target = Args.IsValidIndex(0) ? Args[0] : FString();
		ForEachGameWorldLater(0.f, [Target](UWorld* World)
		{
			if (UGrimSaveSubsystem* Save = GetServerSave(World, TEXT("Grim.Save.Delete")))
			{
				Save->DeleteSlot(Target);
			}
		});
	}

	FAutoConsoleCommand GNowCommand(TEXT("Grim.Save.Now"),
		TEXT("서버: 세계 + 접속한 플레이어를 지금 저장한다. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&NowCommand));

	FAutoConsoleCommand GLoadCommand(TEXT("Grim.Save.Load"),
		TEXT("서버: 세계 + 접속한 플레이어를 디스크에서 다시 읽어 적용한다(저장 뒤 새로 생긴 것은 지우지 않는다). [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&LoadCommand));

	FAutoConsoleCommand GDumpCommand(TEXT("Grim.Save.Dump"),
		TEXT("서버: 슬롯 · 레코드 · 플레이어 · 파일 상태를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));

	FAutoConsoleCommand GDeleteCommand(TEXT("Grim.Save.Delete"),
		TEXT("서버: 슬롯 폴더(세계 + 플레이어 파일)를 지운다. [슬롯(기본 = 지금 슬롯)] — 지금 슬롯이면 끌 때 지금 상태가 새로 저장된다"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DeleteCommand));

#if !UE_BUILD_SHIPPING

	/** 시작한 뒤 이 프로세스(서버)에 원격 플레이어가 접속한 횟수 — Grim.Save.AtLogins 용 */
	int32& RemoteLoginCount()
	{
		static int32 Count = 0;
		static bool bBound = false;
		if (!bBound)
		{
			bBound = true;
			FGameModeEvents::GameModePostLoginEvent.AddLambda([](AGameModeBase*, APlayerController* PC)
			{
				if (PC && !PC->IsLocalController())
				{
					++Count;
				}
			});
		}
		return Count;
	}

	/** 놀이를 시작한 게임 월드 중 로컬 플레이어가 폰을 가진 것(헤드리스 시간표의 「준비」) */
	UWorld* FindReadyWorld()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld() || !World->HasBegunPlay())
			{
				continue;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (It->Get() && It->Get()->IsLocalController() && It->Get()->GetPawn())
				{
					return World;
				}
			}
		}
		return nullptr;
	}

	/** 조건이 맞은 때부터 Delay 초 뒤 Command 를 그 월드에서 친다 */
	void RunWhen(TFunction<bool()> Condition, float Delay, const FString& Command, const TCHAR* Label)
	{
		TSharedRef<double> StartedAt = MakeShared<double>(-1.0);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Condition, Delay, Command, Label, StartedAt](float)
		{
			const double Now = FPlatformTime::Seconds();
			if (*StartedAt < 0.0)
			{
				if (!Condition())
				{
					return true;
				}
				*StartedAt = Now;
			}
			if (Now - *StartedAt < Delay)
			{
				return true;
			}
			if (UWorld* World = FindReadyWorld())
			{
				UE_LOG(LogGrimSave, Display, TEXT("%s → %s"), Label, *Command);
				GEngine->Exec(World, *Command);
				return false;
			}
			return true;
		}), 0.2f);
	}

	/**
	 * Grim.Save.At <초> <콘솔 명령…> — 이 프로세스가 「준비」된 때(놀이 시작 + 로컬 플레이어의 폰)부터 몇 초 뒤.
	 * 헤드리스 시동 시간이 들쭉날쭉해도 순서가 지켜지게(다른 명령의 [지연초]는 프로세스 시작 기준).
	 */
	void AtCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimSave, Warning, TEXT("사용: Grim.Save.At <초> <콘솔 명령…>"));
			return;
		}
		const float Delay = FMath::Max(0.f, FCString::Atof(*Args[0]));
		const FString Command = FString::Join(TArray<FString>(Args.GetData() + 1, Args.Num() - 1), TEXT(" "));
		RunWhen([]() { return FindReadyWorld() != nullptr; }, Delay, Command, TEXT("Grim.Save.At"));
	}

	/** Grim.Save.AtLogins <접속 수> <초> <콘솔 명령…> — 서버: 원격 플레이어 접속이 그 수에 이른 때부터 몇 초 뒤. */
	void AtLoginsCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogGrimSave, Warning, TEXT("사용: Grim.Save.AtLogins <접속 수> <초> <콘솔 명령…>"));
			return;
		}
		const int32 Logins = FCString::Atoi(*Args[0]);
		const float Delay = FMath::Max(0.f, FCString::Atof(*Args[1]));
		const FString Command = FString::Join(TArray<FString>(Args.GetData() + 2, Args.Num() - 2), TEXT(" "));
		RemoteLoginCount();
		RunWhen([Logins]() { return RemoteLoginCount() >= Logins; }, Delay, Command, TEXT("Grim.Save.AtLogins"));
	}

	/** Grim.Save.Quit [지연초] — 게임을 끈다(quit). 종료 저장 경로(OnWorldBeginTearDown)를 그대로 탄다. */
	void QuitCommand(const TArray<FString>& Args)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			UE_LOG(LogGrimSave, Display, TEXT("Grim.Save.Quit — 끈다"));
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World(); World && World->IsGameWorld())
				{
					GEngine->Exec(World, TEXT("quit"));
					break;
				}
			}
			return false;
		}), FMath::Max(0.f, ArgFloat(Args, 0, 0.f)));
	}

	/** Grim.Save.SetAttr <어트리뷰트> <값> [지연초] — 서버: 이 프로세스의 로컬 플레이어(호스트) 어트리뷰트 기본값(시험용). */
	void SetAttrCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimSave, Warning, TEXT("사용: Grim.Save.SetAttr <어트리뷰트 이름> <값> [지연초]"));
			return;
		}
		const FString Name = Args[0];
		const float Value = FCString::Atof(*Args[1]);
		ForEachGameWorldLater(ArgFloat(Args, 2, 0.f), [Name, Value](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				UE_LOG(LogGrimSave, Display, TEXT("[Client] Grim.Save.SetAttr: 서버(호스트)에서만"));
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* PC = It->Get();
				const IAbilitySystemInterface* Owner = PC && PC->IsLocalController() ? Cast<IAbilitySystemInterface>(PC->PlayerState) : nullptr;
				UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
				FGameplayAttribute Attribute;
				if (ASC && UGrimPlayerAttributesSaveParticipant::FindAttributeByName(ASC, Name, Attribute))
				{
					ASC->SetNumericAttributeBase(Attribute, Value);
					UE_LOG(LogGrimSave, Display, TEXT("[Server] SetAttr %s = %.2f (지금 %.2f)"), *Attribute.GetName(), Value, ASC->GetNumericAttribute(Attribute));
				}
			}
		});
	}

	FAutoConsoleCommand GAtCommand(TEXT("Grim.Save.At"),
		TEXT("준비(놀이 시작 + 로컬 폰)부터 몇 초 뒤 콘솔 명령 하나(헤드리스 확인용). 예: Grim.Save.At 5 grim.Time.Set 15"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AtCommand));

	FAutoConsoleCommand GAtLoginsCommand(TEXT("Grim.Save.AtLogins"),
		TEXT("서버: 원격 접속이 N 번에 이른 때부터 몇 초 뒤 콘솔 명령 하나(헤드리스 확인용). 예: Grim.Save.AtLogins 2 10 Grim.Save.Quit"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AtLoginsCommand));

	FAutoConsoleCommand GQuitCommand(TEXT("Grim.Save.Quit"),
		TEXT("몇 초 뒤 게임을 끈다 — 종료 저장 경로 시험. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&QuitCommand));

	FAutoConsoleCommand GSetAttrCommand(TEXT("Grim.Save.SetAttr"),
		TEXT("서버: 호스트 어트리뷰트 기본값을 바꾼다(시험용). <이름> <값> [지연초]. 예: Grim.Save.SetAttr Health 42"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SetAttrCommand));

#endif
}
