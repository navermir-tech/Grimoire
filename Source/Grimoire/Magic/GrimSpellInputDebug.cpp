// 조작 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 기본 공격 · 슬롯 바로 시전이 서버 · 클라이언트에서 같게 나오는지 볼 때 쓴다
// (Scripts/Integration/net_check_controls.ps1). 실제 입력과 같은 길(AGrimPlayerCharacter::SpellInputPressed → ASC 입력 태그)로 누른다.
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다.

#include "Magic/GrimBasicAttackAbility.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellSlotInput.h"
#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGrimInputDebug, Log, All);

namespace GrimSpellInputDebugLocal // 유니티 빌드에서 GrimMagicDebug 의 같은 이름 도우미와 부딪히지 않게 이름을 붙인다
{
	const TCHAR* InputNetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	void InputRunLater(float Delay, TFunction<void()> Fn)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Fn](float)
		{
			Fn();
			return false;
		}), FMath::Max(Delay, 0.001f));
	}

	void InputRunInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		InputRunLater(Delay, [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World(); World && World->IsGameWorld())
				{
					Fn(World);
				}
			}
		});
	}

	float InputParseFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	/** 클래스 이름에 Filter 가 든 액터 중 월드 원점에 가장 가까운 것 — 서버 · 클라이언트가 같은 것을 고른다 */
	AActor* InputFindTarget(UWorld* World, const FString& Filter)
	{
		if (Filter.IsEmpty() || Filter == TEXT("None"))
		{
			return nullptr;
		}
		AActor* Best = nullptr;
		double BestDistSq = TNumericLimits<double>::Max();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(Filter) && It->GetActorLocation().SizeSquared() < BestDistSq)
			{
				BestDistSq = It->GetActorLocation().SizeSquared();
				Best = *It;
			}
		}
		return Best;
	}

	/** 입력 이름(Basic · Slot1~4 · Protego · Dodge) → 태그 */
	FGameplayTag ParseInput(const FString& Name)
	{
		if (Name.Equals(TEXT("Basic"), ESearchCase::IgnoreCase))
		{
			return GrimInputTags::Input_Spell_BasicAttack;
		}
		if (Name.Equals(TEXT("Protego"), ESearchCase::IgnoreCase))
		{
			return GrimInputTags::Input_Action_Protego;
		}
		if (Name.Equals(TEXT("Dodge"), ESearchCase::IgnoreCase))
		{
			return GrimInputTags::Input_Action_Dodge;
		}
		if (Name.StartsWith(TEXT("Slot"), ESearchCase::IgnoreCase))
		{
			return FGrimSpellSlotInputMath::GetSlotInputTag(FCString::Atoi(*Name.Mid(4)) - 1);
		}
		return FGameplayTag();
	}

	AGrimPlayerCharacter* LocalCharacter(UWorld* World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		return PC ? Cast<AGrimPlayerCharacter>(PC->GetPawn()) : nullptr;
	}

	/**
	 * Grim.Input.Press <Basic|Slot1~4|Protego|Dodge> [TargetClass|None] [HoldSeconds] [Delay] [Repeat] [Interval]
	 * 로컬 플레이어가 그 입력을 누른다(캐릭터 입력 함수 → ASC 입력 태그). 대상이 있으면 보도록 돌리고 조준을 고정한 뒤 0.5초 뒤 누른다.
	 * Repeat 번, Interval 초 간격(기본 공격 연타 확인). HoldSeconds 뒤 뗀다.
	 */
	void PressCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1 || !ParseInput(Args[0]).IsValid())
		{
			UE_LOG(LogGrimInputDebug, Warning, TEXT("사용: Grim.Input.Press <Basic|Slot1~4|Protego|Dodge> [대상 클래스|None] [누르는 초] [지연초] [횟수] [간격초]"));
			return;
		}
		const FGameplayTag InputTag = ParseInput(Args[0]);
		const FString Filter = Args.IsValidIndex(1) ? Args[1] : FString();
		const float Hold = InputParseFloat(Args, 2, 0.f);
		const int32 Repeat = FMath::Max(1, static_cast<int32>(InputParseFloat(Args, 4, 1.f)));
		const float Interval = InputParseFloat(Args, 5, 0.35f);

		InputRunInGameWorlds(InputParseFloat(Args, 3, 0.f), [InputTag, Filter, Hold, Repeat, Interval](UWorld* World)
		{
			AGrimPlayerCharacter* Character = LocalCharacter(World);
			if (!Character)
			{
				UE_LOG(LogGrimInputDebug, Warning, TEXT("[%s] Press: 로컬 캐릭터 없음"), InputNetModeName(World));
				return;
			}
			if (AActor* Target = InputFindTarget(World, Filter))
			{
				if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
				{
					PC->SetControlRotation((UGrimSpellAbility::GetTargetPointOf(Target) - Character->GetPawnViewLocation()).Rotation());
				}
				UGrimSpellAbility::SetDebugAimTarget(Character, Target);
			}
			const FString WorldName = InputNetModeName(World);
			TWeakObjectPtr<AGrimPlayerCharacter> WeakCharacter = Character;
			for (int32 Index = 0; Index < Repeat; ++Index)
			{
				InputRunLater(0.5f + Interval * Index, [WeakCharacter, InputTag, Hold, WorldName, Index]()
				{
					AGrimPlayerCharacter* Live = WeakCharacter.Get();
					if (!Live)
					{
						return;
					}
					const UGrimSpellDefinition* Spell = GrimSpellSlotInput::FindSpellForInput(Live->GetAbilitySystemComponent(), InputTag);
					UE_LOG(LogGrimInputDebug, Display, TEXT("[%s] Press %s #%d → %s"), *WorldName, *InputTag.ToString(), Index + 1, Spell ? *Spell->GetName() : TEXT("(빈 칸)"));
					Live->SpellInputPressed(InputTag);
					InputRunLater(Hold, [WeakCharacter, InputTag]()
					{
						if (AGrimPlayerCharacter* Still = WeakCharacter.Get())
						{
							Still->SpellInputReleased(InputTag);
						}
					});
				});
			}
		});
	}

	/** Grim.Input.SlotSet <1부터> [Delay] — 로컬 플레이어가 보이는 슬롯 세트를 바꾼다(패드 R2 + 십자키 · F1~F4 와 같은 요청) */
	void SlotSetCommand(const TArray<FString>& Args)
	{
		const int32 SetIndex = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) - 1 : 0;
		InputRunInGameWorlds(InputParseFloat(Args, 1, 0.f), [SetIndex](UWorld* World)
		{
			if (AGrimPlayerCharacter* Character = LocalCharacter(World))
			{
				Character->RequestSlotSet(SetIndex);
				UE_LOG(LogGrimInputDebug, Display, TEXT("[%s] SlotSet → %d"), InputNetModeName(World), SetIndex + 1);
			}
		});
	}

	/** Grim.Input.Dump [Delay] — 이 프로세스에서 본 플레이어마다: 보이는 세트, 마법 Spec 의 입력 태그, 기본 공격 연타 번째 */
	void DumpCommand(const TArray<FString>& Args)
	{
		InputRunInGameWorlds(InputParseFloat(Args, 0, 0.f), [](UWorld* World)
		{
			for (TActorIterator<AGrimPlayerState> It(World); It; ++It)
			{
				const UGrimSpellbookComponent* Spellbook = It->GetSpellbook();
				const UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent();
				if (!Spellbook || !ASC)
				{
					continue;
				}
				TArray<FString> Lines;
				for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
				{
					const UGrimSpellDefinition* Spell = Cast<UGrimSpellDefinition>(Spec.SourceObject.Get());
					if (!Spell)
					{
						continue;
					}
					FString Tags;
					for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
					{
						if (FGrimSpellSlotInputMath::IsManagedInputTag(Tag))
						{
							Tags += Tag.ToString().Replace(TEXT("Input.Spell."), TEXT(""));
						}
					}
					Lines.Add(FString::Printf(TEXT("%s=%s"), *Spell->GetName().Replace(TEXT("DA_Spell_"), TEXT("")), Tags.IsEmpty() ? TEXT("-") : *Tags));
				}
				Lines.Sort();
				if (Lines.IsEmpty())
				{
					continue; // 남의 PlayerState(어빌리티 Spec 은 소유자에게만 복제된다)
				}
				UE_LOG(LogGrimInputDebug, Display, TEXT("[%s] Input %s 세트=%d 연타=%d %s"), InputNetModeName(World), *It->GetPlayerName(),
					Spellbook->GetActiveSlotSet() + 1, UGrimBasicAttackAbility::GetVisibleComboStep(ASC), *FString::Join(Lines, TEXT(" ")));
			}
		});
	}

	FAutoConsoleCommand GPressCommand(
		TEXT("Grim.Input.Press"),
		TEXT("로컬 플레이어가 조작 입력을 누른다(실제 입력과 같은 길). <Basic|Slot1~4|Protego|Dodge> [대상 클래스|None] [누르는 초] [지연초] [횟수] [간격초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&PressCommand));

	FAutoConsoleCommand GSlotSetCommand(
		TEXT("Grim.Input.SlotSet"),
		TEXT("로컬 플레이어가 보이는 슬롯 세트를 바꾼다. <1부터> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SlotSetCommand));

	FAutoConsoleCommand GDumpCommand(
		TEXT("Grim.Input.Dump"),
		TEXT("플레이어마다 보이는 세트 · 마법 Spec 입력 태그 · 기본 공격 연타를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
}

#endif
