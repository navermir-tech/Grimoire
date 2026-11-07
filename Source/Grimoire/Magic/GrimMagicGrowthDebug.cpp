// 마법 성장(숙련도 · 스킬북 · 서클) 개발 콘솔 — M2 갈래 G. 끝의 [지연초]는 헤드리스 -ExecCmds 용(맵 · 접속보다 먼저 실행되므로).
// 상태를 바꾸는 명령은 로컬 플레이어의 마법서 · 승급 컴포넌트를 거쳐 서버에서 적용된다(클라이언트면 Server RPC).

#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSkillbook.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimMagicGrowthDebugLocal // 유니티 빌드에서 다른 디버그 파일의 도우미와 부딪히지 않게 이름을 붙인다
{
	const TCHAR* GrowthNetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	void RunInGameWorldsLater(float Delay, TFunction<void(UWorld*)> Fn)
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

	/** 이 월드의 로컬 플레이어 PlayerState 들 */
	TArray<AGrimPlayerState*> LocalPlayerStates(UWorld* World)
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

	/** 이름에 Filter 가 든 마법 정의: 배운 것 → 에셋 레지스트리 순으로 찾는다. */
	UGrimSpellDefinition* FindSpellByName(const UGrimSpellbookComponent* Spellbook, const FString& Filter)
	{
		if (Spellbook)
		{
			for (UGrimSpellDefinition* Spell : Spellbook->GetKnownSpells())
			{
				if (Spell && Spell->GetName().Contains(Filter))
				{
					return Spell;
				}
			}
		}
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(UGrimSpellDefinition::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Contains(Filter))
			{
				return Cast<UGrimSpellDefinition>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	void DumpProficiency(UWorld* World)
	{
		for (AGrimPlayerState* PlayerState : LocalPlayerStates(World))
		{
			const UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
			const UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
			const UGrimProgressionComponent* Progression = PlayerState->GetProgression();
			if (!ASC || !Spellbook || !Progression)
			{
				continue;
			}
			const FGrimCircleCheck Check = Progression->EvaluateNextCircle();
			FString Grades;
			for (const FGrimCircleGradeCheck& Grade : Check.Grades)
			{
				Grades += FString::Printf(TEXT(" %s이상 %d/%d"), *Grade.Grade.ToString(), Grade.Have, Grade.Required);
			}
			UE_LOG(LogGrimMagic, Display, TEXT("[%s] Proficiency %s 서클=%d 마나=%.0f/%.0f 다음=%s%s 지금서클마법 %d/%d 충족=%d 슬롯세트=%d 봉인=%d"), GrowthNetModeName(World), *PlayerState->GetName(),
				Check.CurrentCircle, ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute()), ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute()),
				Check.bAtMax ? TEXT("없음") : *FString::FromInt(Check.NextCircle), *Grades, Check.CurrentCircleSpells, Check.RequiredCurrentCircleSpells, Check.bMet ? 1 : 0,
				Spellbook->GetUnlockedSlotSets(), Spellbook->GetSealedSpells().Num());
			for (const UGrimSpellDefinition* Spell : Spellbook->GetKnownSpells())
			{
				const FGrimProficiencyView View = Spellbook->GetProficiency(Spell);
				UE_LOG(LogGrimMagic, Display, TEXT("[%s]   %s 서클%d 등급=%s(%d) XP=%.1f/%.1f 마나x%.2f 쿨x%.2f"), GrowthNetModeName(World), *GetNameSafe(Spell), Spell ? Spell->Circle : 0,
					View.bTracked ? *View.Grade.ToString() : TEXT("-"), View.GradeIndex, View.XP, View.NextGradeXP, View.Multipliers.ManaCost, View.Multipliers.Cooldown);
			}
		}
	}

	/** Grim.Magic.CircleUp [강제 0|1] [지연초] */
	void CircleUpCommand(const TArray<FString>& Args)
	{
		const bool bForce = ArgFloat(Args, 0, 0.f) != 0.f;
		RunInGameWorldsLater(ArgFloat(Args, 1, 0.f), [bForce](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : LocalPlayerStates(World))
			{
				if (UGrimProgressionComponent* Progression = PlayerState->GetProgression())
				{
					UE_LOG(LogGrimMagic, Display, TEXT("[%s] CircleUp 요청 강제=%d"), GrowthNetModeName(World), bForce ? 1 : 0);
					Progression->RequestCircleUp(bForce);
				}
			}
		});
	}

	/** Grim.Magic.ReadSkillbook [지연초] — 로컬 플레이어가 곁의 스킬북을 집어 쓴다(겹침과 같은 길). */
	void ReadSkillbookCommand(const TArray<FString>& Args)
	{
		RunInGameWorldsLater(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : LocalPlayerStates(World))
			{
				AGrimSkillbook* Book = AGrimSkillbook::FindNearestReadable(PlayerState->GetPawn());
				UE_LOG(LogGrimMagic, Display, TEXT("[%s] ReadSkillbook 책=%s 마법=%s"), GrowthNetModeName(World), *GetNameSafe(Book), Book ? *GetNameSafe(Book->GetTeachesSpell()) : TEXT("-"));
				if (Book && PlayerState->GetSpellbook())
				{
					PlayerState->GetSpellbook()->RequestPickUpSkillbook(Book);
				}
			}
		});
	}

	/** Grim.Magic.SpawnSkillbook <마법 이름 일부> [앞cm] [지연초] — 서버: 플레이어마다 앞에 스킬북을 놓는다. */
	void SpawnSkillbookCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimMagic, Warning, TEXT("사용: Grim.Magic.SpawnSkillbook <마법 이름 일부> [앞cm] [지연초]"));
			return;
		}
		const FString Filter = Args[0];
		const float Forward = ArgFloat(Args, 1, 150.f);
		RunInGameWorldsLater(ArgFloat(Args, 2, 0.f), [Filter, Forward](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			UGrimSpellDefinition* Spell = FindSpellByName(nullptr, Filter);
			if (!Spell)
			{
				UE_LOG(LogGrimMagic, Warning, TEXT("[%s] SpawnSkillbook: 마법 없음 %s"), GrowthNetModeName(World), *Filter);
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
				if (!Pawn)
				{
					continue;
				}
				FVector Location = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * Forward;
				Location.Z -= 90.f; // 캡슐 가운데 → 발밑 근처
				AGrimSkillbook* Book = World->SpawnActorDeferred<AGrimSkillbook>(AGrimSkillbook::StaticClass(), FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (Book)
				{
					Book->SetTeachesSpell(Spell);
					Book->FinishSpawning(FTransform(Location));
					UE_LOG(LogGrimMagic, Display, TEXT("[%s] SpawnSkillbook %s (%s) at %s"), GrowthNetModeName(World), *Book->GetName(), *Spell->GetName(), *Location.ToCompactString());
				}
			}
		});
	}

	/** Grim.Magic.AddXP <마법 이름 일부> <양> [지연초] */
	void AddXPCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimMagic, Warning, TEXT("사용: Grim.Magic.AddXP <마법 이름 일부> <양> [지연초]"));
			return;
		}
		const FString Filter = Args[0];
		const float Amount = FCString::Atof(*Args[1]);
		RunInGameWorldsLater(ArgFloat(Args, 2, 0.f), [Filter, Amount](UWorld* World)
		{
			for (AGrimPlayerState* PlayerState : LocalPlayerStates(World))
			{
				UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
				UGrimSpellDefinition* Spell = FindSpellByName(Spellbook, Filter);
				if (!Spellbook || !Spell)
				{
					UE_LOG(LogGrimMagic, Warning, TEXT("[%s] AddXP: 마법 없음 %s"), GrowthNetModeName(World), *Filter);
					continue;
				}
				if (PlayerState->HasAuthority())
				{
					Spellbook->AddProficiencyXP(Spell, Amount);
				}
				else
				{
					Spellbook->Server_DebugAddXP(Spell, Amount);
				}
			}
		});
	}

	/** Grim.Magic.XPRate <배율> — 이 프로세스(서버)의 GlobalXPRate 를 바꾼다(저장하지 않음). */
	void XPRateCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimMagic, Display, TEXT("GlobalXPRate = %.2f"), UGrimProficiencySettings::Get()->GlobalXPRate);
			return;
		}
		GetMutableDefault<UGrimProficiencySettings>()->GlobalXPRate = FMath::Max(0.f, FCString::Atof(*Args[0]));
		UE_LOG(LogGrimMagic, Display, TEXT("GlobalXPRate = %.2f"), UGrimProficiencySettings::Get()->GlobalXPRate);
	}

	FAutoConsoleCommand GCircleUpCommand(TEXT("Grim.Magic.CircleUp"),
		TEXT("로컬 플레이어 서클 승급(조건 확인). [강제 0|1] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&CircleUpCommand));

	FAutoConsoleCommand GReadSkillbookCommand(TEXT("Grim.Magic.ReadSkillbook"),
		TEXT("로컬 플레이어가 곁의 스킬북을 읽는다. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ReadSkillbookCommand));

	FAutoConsoleCommand GSpawnSkillbookCommand(TEXT("Grim.Magic.SpawnSkillbook"),
		TEXT("서버: 플레이어마다 앞에 스킬북. <마법 이름 일부> [앞cm] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnSkillbookCommand));

	FAutoConsoleCommand GAddXPCommand(TEXT("Grim.Magic.AddXP"),
		TEXT("로컬 플레이어 마법 숙련도 경험 더하기. <마법 이름 일부> <양> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AddXPCommand));

	FAutoConsoleCommand GXPRateCommand(TEXT("Grim.Magic.XPRate"),
		TEXT("서버 숙련도 경험 배율(GlobalXPRate). [배율]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&XPRateCommand));

	FAutoConsoleCommand GProficiencyDumpCommand(TEXT("Grim.Magic.Proficiency"),
		TEXT("로컬 플레이어의 서클 · 숙련도 · 승급 조건을 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			RunInGameWorldsLater(ArgFloat(Args, 0, 0.f), [](UWorld* World) { DumpProficiency(World); });
		}));
}

#endif
