// 외형 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 외형 복제를 확인할 때 쓴다(Scripts/Character/net_check_appearance.ps1).
// 지연은 Grim.At <초> <명령> 으로 준다(docs/dev/cheats). 이 프로세스의 모든 게임 월드에 적용한다.

#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceDummy.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Appearance/GrimPlayerAppearanceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimAppearanceDebugLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	void ForGameWorlds(TFunctionRef<void(UWorld*)> Fn)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && World->IsGameWorld())
			{
				Fn(World);
			}
		}
	}

	/** 덤프 한 줄의 열쇠 — 서버 · 클라이언트에서 같은 사람을 같은 이름으로(액터 이름은 기기마다 다르다) */
	FString DescribeOwner(const UGrimAppearanceComponent* Component)
	{
		const AActor* Owner = Component->GetOwner();
		if (const APawn* Pawn = Cast<APawn>(Owner))
		{
			if (const APlayerState* PlayerState = Pawn->GetPlayerState())
			{
				return FString::Printf(TEXT("Player:%s"), *PlayerState->GetPlayerName());
			}
		}
		const FGrimAppearance& Appearance = Component->GetAppearance();
		if (!Appearance.Rule.IsNone())
		{
			return FString::Printf(TEXT("Npc:%s#%d"), *Appearance.Rule.ToString(), Appearance.Seed);
		}
		return FString::Printf(TEXT("Actor:%s"), *GetNameSafe(Owner));
	}

	FAutoConsoleCommand GDumpCommand(
		TEXT("Grim.Appearance.Dump"),
		TEXT("외형 컴포넌트마다 열쇠 · 해시 · 조립 결과 · 외형을 로그에 남긴다(LogGrimAppearance Display)."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			ForGameWorlds([](UWorld* World)
			{
				int32 Count = 0;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					const UGrimAppearanceComponent* Component = It->FindComponentByClass<UGrimAppearanceComponent>();
					if (!Component || !Component->GetAppearance().IsSet())
					{
						continue;
					}
					++Count;
					UE_LOG(LogGrimAppearance, Display, TEXT("[AppearanceDump] %s key=%s hash=%08x skel=%d static=%d rings=%d | %s"),
						NetModeName(World), *DescribeOwner(Component), Component->GetAppearance().GetHash(),
						Component->GetSkeletalPartCount(), Component->GetStaticPieceCount(), Component->GetRingCount(),
						*Component->GetAppearance().ToString());
				}
				UE_LOG(LogGrimAppearance, Display, TEXT("[AppearanceDump] %s total=%d"), NetModeName(World), Count);
			});
		}));

	FAutoConsoleCommand GRandomizeCommand(
		TEXT("Grim.Appearance.Randomize"),
		TEXT("서버: 모든 플레이어 외형을 무작위로(씨앗을 주면 씨앗 + 순번). 예: Grim.Appearance.Randomize 777"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const int32 Seed = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			ForGameWorlds([Seed](UWorld* World)
			{
				if (World->GetNetMode() == NM_Client || !World->GetGameState())
				{
					return;
				}
				int32 Index = 0;
				for (APlayerState* PlayerState : World->GetGameState()->PlayerArray)
				{
					if (UGrimPlayerAppearanceComponent* Player = UGrimPlayerAppearanceComponent::Find(PlayerState))
					{
						Player->RandomizeAuthority(Seed == 0 ? 0 : Seed + Index);
						UE_LOG(LogGrimAppearance, Display, TEXT("[Appearance] %s 무작위: %s"), *PlayerState->GetPlayerName(), *Player->GetAppearance().ToString());
					}
					++Index;
				}
			});
		}));

	FAutoConsoleCommand GLineupCommand(
		TEXT("Grim.Appearance.Lineup"),
		TEXT("서버: 첫 플레이어 앞에 규칙대로 사람을 줄 세운다. Grim.Appearance.Lineup <규칙> [수=5] [씨앗=1]. 규칙 없이 치면 규칙 목록"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
			if (Args.Num() == 0)
			{
				for (const FGrimAppearanceRule& Rule : Catalog.Rules)
				{
					UE_LOG(LogGrimAppearance, Display, TEXT("[Appearance] 규칙 %s — %s"), *Rule.Id.ToString(), *Rule.Row.DisplayName);
				}
				return;
			}
			const FName RuleId(*Args[0]);
			if (!Catalog.FindRule(RuleId))
			{
				UE_LOG(LogGrimAppearance, Warning, TEXT("[Appearance] 규칙 %s 가 없다"), *Args[0]);
				return;
			}
			const int32 Count = FMath::Clamp(Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 5, 1, 30);
			const int32 Seed = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 1;
			ForGameWorlds([&](UWorld* World)
			{
				if (World->GetNetMode() == NM_Client)
				{
					return;
				}
				const APlayerController* Controller = World->GetFirstPlayerController();
				const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
				if (!Pawn && World->GetGameState() && World->GetGameState()->PlayerArray.Num() > 0)
				{
					Pawn = World->GetGameState()->PlayerArray[0]->GetPawn();
				}
				const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
				const FVector Forward = Pawn ? Pawn->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
				const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
				for (int32 Index = 0; Index < Count; ++Index)
				{
					FVector Location = Origin + Forward * 500.f + Right * (Index - (Count - 1) * 0.5f) * 120.f;
					FHitResult Hit;
					if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 300.f), Location - FVector(0.f, 0.f, 1000.f), ECC_Visibility))
					{
						Location = Hit.ImpactPoint;
					}
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AGrimAppearanceDummy* Dummy = World->SpawnActor<AGrimAppearanceDummy>(Location, (-Forward).Rotation(), Params);
					if (Dummy)
					{
						Dummy->GenerateAppearance(RuleId, Seed + Index);
						UE_LOG(LogGrimAppearance, Display, TEXT("[Appearance] 줄 세움 %s#%d: %s"), *RuleId.ToString(), Seed + Index,
							*Dummy->GetAppearanceComponent()->GetAppearance().ToString());
					}
				}
			});
		}));

	FAutoConsoleCommand GReloadCommand(
		TEXT("Grim.Appearance.Reload"),
		TEXT("외형 표(CSV)를 다시 읽고 월드의 외형을 다시 조립한다."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UGrimAppearanceSettings::ReloadCatalog();
			ForGameWorlds([](UWorld* World) { UGrimAppearanceComponent::RebuildAllInWorld(World); });
		}));
}

#endif
