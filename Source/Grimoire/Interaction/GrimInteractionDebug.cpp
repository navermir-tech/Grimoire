// 속성 사물 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 복제 · 연출을 확인할 때 쓴다(docs/systems/magic 「성능 · 네트워크 규칙」, Scripts/Integration/net_check.ps1).
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다. 지연 뒤에 그 프로세스의 모든 게임 월드에 적용한다.

#include "Interaction/GrimElementalObject.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Core/GrimElementalTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace
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

	/** Delay 초 뒤(0 이면 바로) 이 프로세스의 게임 월드마다 Fn 을 부른다 */
	void RunInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		auto RunNow = [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
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

	float ParseDelay(const TArray<FString>& Args, int32 Index)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : 0.f;
	}

	/** Grim.Interaction.Stimulus <Element> <Intensity> [Delay] [ClassNameContains] — 서버: 모든 속성 사물에 자극 */
	void StimulusCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimInteraction, Warning, TEXT("사용: Grim.Interaction.Stimulus <Element.Fire> <세기> [지연초] [클래스 이름 일부]"));
			return;
		}
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = FGameplayTag::RequestGameplayTag(FName(*Args[0]), false);
		Stimulus.Intensity = FCString::Atof(*Args[1]);
		Stimulus.Source = EGrimStimulusSource::Environment;
		const FString ClassFilter = Args.IsValidIndex(3) ? Args[3] : FString();
		if (!Stimulus.Element.IsValid())
		{
			UE_LOG(LogGrimInteraction, Warning, TEXT("Grim.Interaction.Stimulus: 태그 없음 %s"), *Args[0]);
			return;
		}

		RunInGameWorlds(ParseDelay(Args, 2), [Stimulus, ClassFilter](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			int32 Count = 0;
			for (TActorIterator<AGrimElementalObject> It(World); It; ++It)
			{
				if (ClassFilter.IsEmpty() || It->GetClass()->GetName().Contains(ClassFilter))
				{
					UGrimElementalStatics::ApplyStimulusToActor(*It, Stimulus, nullptr);
					++Count;
				}
			}
			UE_LOG(LogGrimInteraction, Display, TEXT("[%s] Stimulus %s %.2f -> %d개"), NetModeName(World), *Stimulus.Element.ToString(), Stimulus.Intensity, Count);
		});
	}

	/** Grim.Interaction.Dump [Delay] — 이 프로세스에서 본 사물 상태 · 불꽃 · 빛 */
	void DumpCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ParseDelay(Args, 0), [](UWorld* World)
		{
			for (TActorIterator<AGrimElementalObject> It(World); It; ++It)
			{
				FString States;
				for (const FGrimElementalState& Item : It->GetElementalState()->GetStates().Items)
				{
					States += FString::Printf(TEXT("%s=%.2f "), *Item.StateTag.ToString(), Item.Intensity);
				}
				const UPointLightComponent* Light = It->FindComponentByClass<UPointLightComponent>();
				const UStaticMeshComponent* Flame = FindObjectFast<UStaticMeshComponent>(*It, TEXT("FlameMesh"));
				UE_LOG(LogGrimInteraction, Display, TEXT("[%s] %s states={%s} light=%s(%.1f) flame=%s dormancy=%d"),
					NetModeName(World), *It->GetName(), *States,
					Light && Light->IsVisible() ? TEXT("on") : TEXT("off"), Light ? Light->Intensity : 0.f,
					Flame && !Flame->bHiddenInGame ? TEXT("shown") : TEXT("hidden"),
					static_cast<int32>(It->NetDormancy.GetValue()));
			}
		});
	}

	FAutoConsoleCommand GStimulusCommand(
		TEXT("Grim.Interaction.Stimulus"),
		TEXT("서버: 모든 속성 사물(또는 클래스 이름이 맞는 것)에 자극. <Element> <세기> [지연초] [클래스 이름 일부]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&StimulusCommand));

	FAutoConsoleCommand GDumpCommand(
		TEXT("Grim.Interaction.Dump"),
		TEXT("이 프로세스에서 본 속성 사물 상태 · 불꽃 · 빛을 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
}

#endif
