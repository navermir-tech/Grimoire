// 레벨 · 성장 트리 개발 콘솔 — M2 후반 갈래 L. 끝의 [지연초]는 헤드리스 -ExecCmds 용(맵 · 접속보다 먼저 실행되므로).
// 상태를 바꾸는 명령은 로컬 플레이어의 컴포넌트를 거쳐 서버에서 적용된다(클라이언트면 Server RPC).

#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionSettings.h"
#include "Progression/GrimProgressionTags.h"
#include "Progression/GrimSkillTreeComponent.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimProgressionDebugLocal // 유니티 빌드에서 다른 디버그 파일의 도우미와 부딪히지 않게 이름을 붙인다
{
	const TCHAR* ProgressionNetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	void RunInGameWorldsAfter(float Delay, TFunction<void(UWorld*)> Fn)
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

	float ProgressionArgFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	TArray<AGrimPlayerState*> ProgressionLocalPlayerStates(UWorld* World)
	{
		TArray<AGrimPlayerState*> Result;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController())
			{
				if (AGrimPlayerState* PlayerState = PC->GetPlayerState<AGrimPlayerState>())
				{
					Result.Add(PlayerState);
				}
			}
		}
		return Result;
	}

	void DumpLevel(UWorld* World)
	{
		for (AGrimPlayerState* PlayerState : ProgressionLocalPlayerStates(World))
		{
			const UGrimLevelComponent* Level = PlayerState->GetCharacterLevel();
			const UGrimSkillTreeComponent* Tree = PlayerState->GetSkillTree();
			const UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
			if (!Level || !Tree || !ASC)
			{
				continue;
			}
			const FGrimLevelProgress Progress = Level->GetProgress();
			UE_LOG(LogGrimProgression, Display, TEXT("[%s] Level %s 레벨=%d/%d 경험=%.0f (이 레벨 %.0f ~ 다음 %.0f, %.0f%%) 포인트 받음=%d 씀=%d 남음=%d 보너스 HP+%.0f 마나+%.0f"),
				ProgressionNetModeName(World), *PlayerState->GetName(), Progress.Level, Progress.MaxLevel, Progress.TotalXP, Progress.LevelStartXP, Progress.NextLevelXP,
				Progress.Fraction * 100.f, Level->GetTotalSkillPoints(), Tree->GetSpentPoints(), Tree->GetAvailablePoints(), Level->GetMaxHealthBonus(), Level->GetMaxManaBonus());
			UE_LOG(LogGrimProgression, Display, TEXT("[%s]   최대HP=%.0f 최대마나=%.0f 더위저항=%.1f 추위저항=%.1f 질병저항=%.1f 이동x%.3f 시전x%.3f 마나소모x%.3f 서클=%d"),
				ProgressionNetModeName(World),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHeatResistAttribute()), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetDiseaseResistAttribute()), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetCastSpeedScaleAttribute()), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetManaCostScaleAttribute()),
				Tree->GetCircle());
			const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
			for (const FName Id : Definition.GetTreeIds())
			{
				TArray<FString> Names;
				for (const FName Node : Tree->GetUnlockedNodes())
				{
					if (const FGrimSkillNode* Found = Definition.Find(Node); Found && Found->Tree == Id)
					{
						Names.Add(Node.ToString());
					}
				}
				UE_LOG(LogGrimProgression, Display, TEXT("[%s]   트리 %s 씀 %d: %s"), ProgressionNetModeName(World), *Id.ToString(), Tree->GetSpentPoints(Id),
					Names.IsEmpty() ? TEXT("-") : *FString::Join(Names, TEXT(", ")));
			}
			for (const TPair<FName, FGrimSkillEffectTotal>& Pair : Tree->GetEffectTotals())
			{
				UE_LOG(LogGrimProgression, Display, TEXT("[%s]   효과 %s +%.2f x%.3f"), ProgressionNetModeName(World), *Pair.Key.ToString(), Pair.Value.Add, Pair.Value.Mul);
			}
		}
	}

	/** Grim.Level.AddXP <양> [출처 태그 끝 이름(Debug · Quest · Hunt …)] [지연초] */
	void AddXPCommand(const TArray<FString>& Args)
	{
		const float Amount = ProgressionArgFloat(Args, 0, 100.f);
		FGameplayTag Source = GrimProgressionTags::XPSource_Debug;
		if (Args.IsValidIndex(1) && !Args[1].IsNumeric())
		{
			const FGameplayTag Found = UGameplayTagsManager::Get().RequestGameplayTag(FName(*FString::Printf(TEXT("Progression.XPSource.%s"), *Args[1])), false);
			Source = Found.IsValid() ? Found : Source;
		}
		const float Delay = ProgressionArgFloat(Args, (Args.IsValidIndex(1) && !Args[1].IsNumeric()) ? 2 : 1, 0.f);
		RunInGameWorldsAfter(Delay, [Amount, Source](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : ProgressionLocalPlayerStates(World))
			{
				UGrimLevelComponent* Level = PlayerState->GetCharacterLevel();
				if (!Level)
				{
					continue;
				}
				if (PlayerState->HasAuthority())
				{
					const float Given = Level->AwardExperience(Amount, Source);
					UE_LOG(LogGrimProgression, Display, TEXT("[%s] AddXP %.0f (%s) → +%.1f, 레벨 %d"), ProgressionNetModeName(World), Amount, *Source.ToString(), Given, Level->GetLevel());
				}
				else
				{
					UE_LOG(LogGrimProgression, Display, TEXT("[%s] AddXP %.0f (%s) 요청(서버)"), ProgressionNetModeName(World), Amount, *Source.ToString());
					Level->Server_DebugAddXP(Amount, Source);
				}
			}
		});
	}

	/** Grim.Level.Set <레벨> [지연초] */
	void SetLevelCommand(const TArray<FString>& Args)
	{
		const int32 NewLevel = FMath::RoundToInt(ProgressionArgFloat(Args, 0, 1.f));
		RunInGameWorldsAfter(ProgressionArgFloat(Args, 1, 0.f), [NewLevel](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : ProgressionLocalPlayerStates(World))
			{
				if (UGrimLevelComponent* Level = PlayerState->GetCharacterLevel())
				{
					UE_LOG(LogGrimProgression, Display, TEXT("[%s] Level.Set %d"), ProgressionNetModeName(World), NewLevel);
					if (PlayerState->HasAuthority())
					{
						Level->SetLevel(NewLevel);
					}
					else
					{
						Level->Server_DebugSetLevel(NewLevel);
					}
				}
			}
		});
	}

	/** Grim.Tree.Unlock <노드 id> [지연초] */
	void UnlockCommand(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogGrimProgression, Display, TEXT("Grim.Tree.Unlock <노드 id 예: Fire.Power> [지연초]"));
			return;
		}
		const FName NodeId(*Args[0]);
		RunInGameWorldsAfter(ProgressionArgFloat(Args, 1, 0.f), [NodeId](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : ProgressionLocalPlayerStates(World))
			{
				if (UGrimSkillTreeComponent* Tree = PlayerState->GetSkillTree())
				{
					UE_LOG(LogGrimProgression, Display, TEXT("[%s] Tree.Unlock %s (지금 판정 %s)"), ProgressionNetModeName(World), *NodeId.ToString(),
						*StaticEnum<EGrimSkillUnlockResult>()->GetNameStringByValue(static_cast<int64>(Tree->CanUnlock(NodeId))));
					Tree->RequestUnlock(NodeId);
				}
			}
		});
	}

	/** Grim.Tree.Reset <트리 id: Normal · Elemental · Light · Dark> [지연초] — 비용 없이 */
	void ResetCommand(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogGrimProgression, Display, TEXT("Grim.Tree.Reset <Normal|Elemental|Light|Dark> [지연초]"));
			return;
		}
		const FName TreeId(*Args[0]);
		RunInGameWorldsAfter(ProgressionArgFloat(Args, 1, 0.f), [TreeId](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : ProgressionLocalPlayerStates(World))
			{
				if (UGrimSkillTreeComponent* Tree = PlayerState->GetSkillTree())
				{
					UE_LOG(LogGrimProgression, Display, TEXT("[%s] Tree.Reset %s"), ProgressionNetModeName(World), *TreeId.ToString());
					if (PlayerState->HasAuthority())
					{
						Tree->ResetTree(TreeId);
					}
					else
					{
						Tree->Server_DebugResetTree(TreeId);
					}
				}
			}
		});
	}

	FAutoConsoleCommand GAddXPCommand(TEXT("Grim.Level.AddXP"),
		TEXT("경험치를 준다. <양> [출처: Debug|Quest|Hunt|Craft|Skillbook …] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AddXPCommand));

	FAutoConsoleCommand GSetLevelCommand(TEXT("Grim.Level.Set"),
		TEXT("레벨을 바로 정한다. <레벨> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SetLevelCommand));

	FAutoConsoleCommand GDumpCommand(TEXT("Grim.Level.Dump"),
		TEXT("레벨 · 경험 · 포인트 · 찍은 노드 · 성장 보너스 어트리뷰트를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			RunInGameWorldsAfter(ProgressionArgFloat(Args, 0, 0.f), &DumpLevel);
		}));

	FAutoConsoleCommand GUnlockCommand(TEXT("Grim.Tree.Unlock"),
		TEXT("성장 트리 노드를 찍는다. <노드 id> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&UnlockCommand));

	FAutoConsoleCommand GResetCommand(TEXT("Grim.Tree.Reset"),
		TEXT("성장 트리 하나를 비운다(정화 의식, 비용 없이). <트리 id> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ResetCommand));
}

#endif
