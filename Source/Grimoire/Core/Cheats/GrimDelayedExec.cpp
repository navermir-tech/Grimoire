// Grim.At <초> <명령 …> — 콘솔 명령을 몇 초 뒤에 첫 게임 월드에서 실행한다(개발 빌드).
// -ExecCmds 는 맵이 뜨기 전에 한꺼번에 돌아서, 점검 스크립트가 「맵이 뜬 뒤 · 시전 도중」 에 무엇을 하려면 지연이 필요하다.
// 예: -ExecCmds="Grim.At 20 HighResShot 1280x720, Grim.At 21.5 Grim.Magic.Cast Ember None"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommand GGrimAtCommand(
	TEXT("Grim.At"),
	TEXT("<초> <명령 …> — 콘솔 명령을 몇 초 뒤 실행. 예: Grim.At 20 HighResShot 1280x720"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("사용: Grim.At <초> <명령 …>"));
			return;
		}
		const float Delay = FMath::Max(0.f, FCString::Atof(*Args[0]));
		const FString Command = FString::Join(TArrayView<const FString>(Args).RightChop(1), TEXT(" "));
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Command](float)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					UE_LOG(LogTemp, Display, TEXT("[Grim.At] %s"), *Command);
					// 플레이어 컨트롤러를 거쳐야 뷰포트가 받는 명령(HighResShot 등)까지 닿는다.
					if (APlayerController* PlayerController = World->GetFirstPlayerController())
					{
						PlayerController->ConsoleCommand(Command);
					}
					else
					{
						GEngine->Exec(World, *Command);
					}
					break;
				}
			}
			return false;
		}), Delay);
	}));

#endif
