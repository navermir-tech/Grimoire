#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// 생존 콘솔 명령(개발 빌드). 콘솔은 로컬에서 돌므로 친 플레이어의 생존 컴포넌트 서버 RPC 로 서버에서 적용한다.
// 이름은 다른 콘솔 명령처럼 grim.* (콘솔은 대소문자를 가리지 않는다 — Grim.Survival.Sleep 도 된다).

#if !UE_BUILD_SHIPPING

namespace GrimSurvivalConsolePrivate
{
	UGrimSurvivalComponent* FindLocalSurvival(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController->IsLocalController())
			{
				const AGrimPlayerState* PlayerState = PlayerController->GetPlayerState<AGrimPlayerState>();
				return PlayerState ? PlayerState->GetSurvival() : nullptr;
			}
		}
		return nullptr;
	}

	void Run(UWorld* World, const TCHAR* Command, float A = 0.f, float B = 0.f)
	{
		if (UGrimSurvivalComponent* Survival = FindLocalSurvival(World))
		{
			Survival->ServerDebugCommand(Command, A, B);
		}
	}

	float ArgOr(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	FConsoleCommandWithWorldAndArgsDelegate Make(const TCHAR* Command)
	{
		return FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Command](const TArray<FString>& Args, UWorld* World)
		{
			Run(World, Command, ArgOr(Args, 0, 0.f), ArgOr(Args, 1, 0.f));
		});
	}

	/** 이 프로세스의 게임 월드에서 본 각 플레이어의 생존 상태를 한 줄씩(헤드리스 확인용). */
	void Dump(UWorld* World)
	{
		const UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>();
		const AGameStateBase* GameState = World->GetGameState();
		if (!GameState)
		{
			return;
		}
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			const AGrimPlayerState* GrimPlayerState = Cast<AGrimPlayerState>(PlayerState);
			const UGrimSurvivalComponent* Survival = GrimPlayerState ? GrimPlayerState->GetSurvival() : nullptr;
			const UAbilitySystemComponent* ASC = GrimPlayerState ? GrimPlayerState->GetAbilitySystemComponent() : nullptr;
			if (!Survival || !ASC)
			{
				continue;
			}
			FGameplayTagContainer Tags;
			ASC->GetOwnedGameplayTags(Tags);
			FString StatusTags;
			for (const FGameplayTag& Tag : Tags)
			{
				if (Tag.ToString().StartsWith(TEXT("Status.")))
				{
					StatusTags += Tag.ToString() + TEXT(" ");
				}
			}
			const APawn* Pawn = Cast<APawn>(ASC->GetAvatarActor());
			const ACharacter* Character = Cast<ACharacter>(Pawn);
			const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
			const FGrimClimateExposure Exposure = Survival->GetClimateExposure();
			UE_LOG(LogGrimSurvival, Display,
				TEXT("[SurvivalDump %s] %s time=%.3f day=%d HP=%.1f Hunger=%.1f Fatigue=%.1f Body=%.2f Cold=%d Heat=%d Move=%.2f Cast=%.2f ManaCost=%.2f Walk=%.0f pos=%s tags=%s"),
				World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"),
				*GrimPlayerState->GetPlayerName(),
				TimeOfDay ? TimeOfDay->GetTimeOfDay() : -1.f, TimeOfDay ? TimeOfDay->GetDayIndex() : -1,
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetFatigueAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetBodyTemperatureAttribute()),
				Exposure.Cold, Exposure.Heat,
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetCastSpeedScaleAttribute()),
				ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetManaCostScaleAttribute()),
				Movement ? Movement->MaxWalkSpeed : -1.f,
				Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"),
				*StatusTags);
		}
	}

	/** grim.Survival.At <초> <명령> [A] [B] — 이 프로세스의 게임 월드마다 몇 초 뒤에. 명령은 Dump 또는 위 콘솔 명령의 끝 이름(Sleep · Down …). */
	void AtCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			return;
		}
		const float Delay = FMath::Max(0.f, FCString::Atof(*Args[0]));
		const FString Command = Args[1];
		const float A = ArgOr(Args, 2, 0.f);
		const float B = ArgOr(Args, 3, 0.f);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Command, A, B](float)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					if (Command == TEXT("Dump"))
					{
						Dump(World);
					}
					else
					{
						Run(World, *Command, A, B);
					}
				}
			}
			return false;
		}), Delay);
	}
}

static FAutoConsoleCommand GGrimSurvivalAtCommand(
	TEXT("grim.Survival.At"),
	TEXT("몇 초 뒤 생존 명령(헤드리스 확인용). 예: grim.Survival.At 30 Sleep / grim.Survival.At 40 Dump / grim.Survival.At 50 Fatigue 85"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimSurvivalConsolePrivate::AtCommand));

static FAutoConsoleCommandWithWorld GGrimSurvivalDumpCommand(
	TEXT("grim.Survival.Dump"),
	TEXT("이 월드에서 본 플레이어별 생존 상태를 로그로."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&GrimSurvivalConsolePrivate::Dump));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalSleepCommand(
	TEXT("grim.Survival.Sleep"),
	TEXT("눕는다 — 반경 안 침대가 있으면 그 침대(부활 지점이 된다), 없으면 그 자리."),
	GrimSurvivalConsolePrivate::Make(TEXT("Sleep")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalWakeCommand(
	TEXT("grim.Survival.Wake"),
	TEXT("일어난다(기절 중은 안 된다)."),
	GrimSurvivalConsolePrivate::Make(TEXT("Wake")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalDownCommand(
	TEXT("grim.Survival.Down"),
	TEXT("HP 를 0 으로 — 쓰러짐(Status.Downed) 시험."),
	GrimSurvivalConsolePrivate::Make(TEXT("Down")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalReviveCommand(
	TEXT("grim.Survival.Revive"),
	TEXT("쓰러진 자기를 일으킨다(나중엔 동료의 빛 마법)."),
	GrimSurvivalConsolePrivate::Make(TEXT("Revive")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalDieCommand(
	TEXT("grim.Survival.Die"),
	TEXT("바로 사망 → 마지막 침대(없으면 PlayerStart)에서 부활."),
	GrimSurvivalConsolePrivate::Make(TEXT("Die")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalFatigueCommand(
	TEXT("grim.Survival.Fatigue"),
	TEXT("피곤함을 값으로. 예: grim.Survival.Fatigue 85"),
	GrimSurvivalConsolePrivate::Make(TEXT("Fatigue")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalEatRawCommand(
	TEXT("grim.Survival.EatRaw"),
	TEXT("날것을 먹은 셈 — 배탈 확률 굴림."),
	GrimSurvivalConsolePrivate::Make(TEXT("EatRaw")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalSickCommand(
	TEXT("grim.Survival.Sick"),
	TEXT("질병을 건다. 0 = 배탈, 1 = 감기. 예: grim.Survival.Sick 1"),
	GrimSurvivalConsolePrivate::Make(TEXT("Sick")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalCureCommand(
	TEXT("grim.Survival.Cure"),
	TEXT("질병을 모두 낫게 한다."),
	GrimSurvivalConsolePrivate::Make(TEXT("Cure")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalSpawnBedCommand(
	TEXT("grim.Survival.SpawnBed"),
	TEXT("시험용: 내 앞 1.5m 에 침대(서버). 1 이면 침낭. 예: grim.Survival.SpawnBed 1"),
	GrimSurvivalConsolePrivate::Make(TEXT("SpawnBed")));

static FAutoConsoleCommandWithWorldAndArgs GGrimSurvivalResistCommand(
	TEXT("grim.Survival.Resist"),
	TEXT("시험용 저항 버프(장비 M4 전): 더위 추위. 예: grim.Survival.Resist 0 2 / 0 0 이면 뗀다"),
	GrimSurvivalConsolePrivate::Make(TEXT("Resist")));

#endif // !UE_BUILD_SHIPPING
