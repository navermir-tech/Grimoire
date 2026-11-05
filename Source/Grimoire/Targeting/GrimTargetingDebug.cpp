// 조준 디버그 · 옵션 콘솔 명령. 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다(헤드리스 확인용).
// 옵션 화면이 생기기 전까지 Q18(소프트 락온 끄기) · 카메라/이동 기준도 여기서 바꾼다.

#include "Targeting/GrimTargetingComponent.h"
#include "Targeting/GrimTargetDummy.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimTargetingDebug
{
	/** Delay 초 뒤(0 이면 바로) 게임 월드마다 로컬 플레이어 컨트롤러와 그 조준 컴포넌트에 Fn */
	void RunOnLocalTargeting(float Delay, TFunction<void(APlayerController*, UGrimTargetingComponent*)> Fn, bool bAllControllers = false)
	{
		auto RunNow = [Fn, bAllControllers]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (!World || !World->IsGameWorld())
				{
					continue;
				}
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					APlayerController* PC = It->Get();
					if (PC && (bAllControllers || PC->IsLocalController()))
					{
						if (UGrimTargetingComponent* Targeting = PC->FindComponentByClass<UGrimTargetingComponent>())
						{
							Fn(PC, Targeting);
						}
					}
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

	void SoftLockCommand(const TArray<FString>& Args)
	{
		const bool bEnable = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) != 0 : true;
		RunOnLocalTargeting(ArgFloat(Args, 1, 0.f), [bEnable](APlayerController*, UGrimTargetingComponent* Targeting)
		{
			Targeting->SetSoftLockEnabled(bEnable);
			UE_LOG(LogGrimTargeting, Display, TEXT("소프트 락온 %s"), bEnable ? TEXT("켬") : TEXT("끔"));
		});
	}

	void ReferenceCommand(const TArray<FString>& Args)
	{
		const bool bMovement = Args.IsValidIndex(0) && Args[0].StartsWith(TEXT("M"), ESearchCase::IgnoreCase);
		RunOnLocalTargeting(ArgFloat(Args, 1, 0.f), [bMovement](APlayerController*, UGrimTargetingComponent* Targeting)
		{
			Targeting->SetTargetingReference(bMovement ? EGrimTargetingReference::Movement : EGrimTargetingReference::Camera);
			UE_LOG(LogGrimTargeting, Display, TEXT("선정 기준 = %s"), bMovement ? TEXT("이동") : TEXT("카메라"));
		});
	}

	void LockCommand(const TArray<FString>& Args)
	{
		RunOnLocalTargeting(ArgFloat(Args, 0, 0.f), [](APlayerController*, UGrimTargetingComponent* Targeting)
		{
			Targeting->ToggleLock();
			UE_LOG(LogGrimTargeting, Display, TEXT("고정 락온 토글 → %s"), *Targeting->Describe());
		});
	}

	void LookCommand(const TArray<FString>& Args)
	{
		const float Yaw = ArgFloat(Args, 0, 0.f);
		const float Pitch = ArgFloat(Args, 1, 0.f);
		RunOnLocalTargeting(ArgFloat(Args, 2, 0.f), [Yaw, Pitch](APlayerController* PC, UGrimTargetingComponent* Targeting)
		{
			PC->AddYawInput(Yaw);
			PC->AddPitchInput(Pitch);
			UE_LOG(LogGrimTargeting, Display, TEXT("시점 입력 (%.1f, %.1f) → %s"), Yaw, Pitch, *Targeting->Describe());
		});
	}

	void DumpCommand(const TArray<FString>& Args)
	{
		RunOnLocalTargeting(ArgFloat(Args, 0, 0.f), [](APlayerController* PC, UGrimTargetingComponent* Targeting)
		{
			const FGrimAimResult Aim = Targeting->GetCurrentAim(0.f);
			UE_LOG(LogGrimTargeting, Display, TEXT("[%s] %s | aim mode=%s target=%s point=%s"),
				*GetNameSafe(PC), *Targeting->Describe(), *UEnum::GetDisplayValueAsText(Aim.Mode).ToString(),
				*GetNameSafe(Aim.Target.Get()), *Aim.AimPoint.ToCompactString());
		});
	}

	/** Grim.Targeting.SpawnDummy [앞cm=800] [오른쪽cm=0] [Enemy|Creature|Object|Ally] [지연초] — 로컬 폰 앞(카메라 요 기준)에 더미 */
	void SpawnDummyCommand(const TArray<FString>& Args)
	{
		const float Forward = ArgFloat(Args, 0, 800.f);
		const float Right = ArgFloat(Args, 1, 0.f);
		const FString KindName = Args.IsValidIndex(2) ? Args[2] : TEXT("Enemy");
		const int64 KindValue = StaticEnum<EGrimTargetKind>()->GetValueByNameString(KindName);
		const EGrimTargetKind Kind = KindValue == INDEX_NONE ? EGrimTargetKind::Enemy : static_cast<EGrimTargetKind>(KindValue);

		RunOnLocalTargeting(ArgFloat(Args, 3, 0.f), [Forward, Right, Kind](APlayerController* PC, UGrimTargetingComponent*)
		{
			const APawn* Pawn = PC->GetPawn();
			if (!Pawn)
			{
				return;
			}
			const FRotator Yaw(0.f, PC->GetControlRotation().Yaw, 0.f);
			const FVector Location = Pawn->GetActorLocation() + Yaw.RotateVector(FVector(Forward, Right, 0.f));
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AGrimTargetDummy* Dummy = PC->GetWorld()->SpawnActor<AGrimTargetDummy>(Location, FRotator::ZeroRotator, Params))
			{
				Dummy->SetTargetKind(Kind);
				UE_LOG(LogGrimTargeting, Display, TEXT("더미 %s (%s) at %s"), *Dummy->GetName(), *UEnum::GetDisplayValueAsText(Kind).ToString(), *Location.ToCompactString());
			}
		});
	}

	/** Grim.Targeting.Zoom <0|1> [지연초] — 입력 방식과 상관없이 줌인 조준을 켜고 끈다(헤드리스 확인용) */
	void ZoomCommand(const TArray<FString>& Args)
	{
		const bool bEnable = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) != 0 : true;
		RunOnLocalTargeting(ArgFloat(Args, 1, 0.f), [bEnable](APlayerController*, UGrimTargetingComponent* Targeting)
		{
			Targeting->SetZoomAiming(bEnable);
			UE_LOG(LogGrimTargeting, Display, TEXT("줌인 조준 %s → %s"), bEnable ? TEXT("켬") : TEXT("끔"), *Targeting->Describe());
		});
	}

	/** Grim.Targeting.ZoomToggle <0|1> [지연초] — 「조준 누르기 / 토글」 옵션 */
	void ZoomToggleCommand(const TArray<FString>& Args)
	{
		const bool bToggle = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) != 0 : true;
		RunOnLocalTargeting(ArgFloat(Args, 1, 0.f), [bToggle](APlayerController*, UGrimTargetingComponent* Targeting)
		{
			Targeting->SetZoomToggleMode(bToggle);
			UE_LOG(LogGrimTargeting, Display, TEXT("줌인 조준 방식 = %s"), bToggle ? TEXT("토글") : TEXT("누르는 동안"));
		});
	}

	/** Grim.Targeting.DumpServer [지연초] — 서버에서 원격 플레이어까지 모든 컨트롤러의 줌 폰 효과(걷기 속력 · 몸 방향 · State.Aiming) */
	void DumpServerCommand(const TArray<FString>& Args)
	{
		RunOnLocalTargeting(ArgFloat(Args, 0, 0.f), [](APlayerController* PC, UGrimTargetingComponent* Targeting)
		{
			UE_LOG(LogGrimTargeting, Display, TEXT("[%s %s] %s"), *GetNameSafe(PC), PC->IsLocalController() ? TEXT("로컬") : TEXT("원격"), *Targeting->Describe());
		}, true);
	}

	/** Grim.Targeting.DumpAimingTags [지연초] — 이 기계가 보는 모든 폰의 State.Aiming(복제 확인용) */
	void DumpAimingTagsCommand(const TArray<FString>& Args)
	{
		auto RunNow = []()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (!World || !World->IsGameWorld())
				{
					continue;
				}
				for (TActorIterator<APawn> It(World); It; ++It)
				{
					const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(*It);
					UE_LOG(LogGrimTargeting, Display, TEXT("폰 %s (%s): State.Aiming=%d"), *It->GetName(),
						*UEnum::GetValueAsString(It->GetLocalRole()),
						ASC && ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Aiming) ? 1 : 0);
				}
			}
		};
		const float Delay = ArgFloat(Args, 0, 0.f);
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

	FAutoConsoleCommand GDumpAimingTagsCommand(TEXT("Grim.Targeting.DumpAimingTags"),
		TEXT("이 기계가 보는 모든 폰의 State.Aiming(줌 복제 확인). [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DumpAimingTagsCommand));
	FAutoConsoleCommand GDumpServerCommand(TEXT("Grim.Targeting.DumpServer"),
		TEXT("모든 컨트롤러(서버에서는 원격 플레이어 포함)의 조준 · 줌 상태를 로그로. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DumpServerCommand));
	FAutoConsoleCommand GZoomCommand(TEXT("Grim.Targeting.Zoom"),
		TEXT("줌인 조준 켜기/끄기(R24, 입력 방식 무시). <0|1> [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&ZoomCommand));
	FAutoConsoleCommand GZoomToggleCommand(TEXT("Grim.Targeting.ZoomToggle"),
		TEXT("줌인 조준 방식. 1 = 토글, 0 = 누르는 동안. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&ZoomToggleCommand));

	FAutoConsoleCommand GSoftLockCommand(TEXT("Grim.Targeting.SoftLock"),
		TEXT("소프트 락온 켜기/끄기(Q18). <0|1> [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&SoftLockCommand));
	FAutoConsoleCommand GReferenceCommand(TEXT("Grim.Targeting.Reference"),
		TEXT("선정 기준. <Camera|Movement> [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&ReferenceCommand));
	FAutoConsoleCommand GLockCommand(TEXT("Grim.Targeting.Lock"),
		TEXT("고정 락온 토글(R3 · Caps Lock 과 같음). [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&LockCommand));
	FAutoConsoleCommand GLookCommand(TEXT("Grim.Targeting.Look"),
		TEXT("시점 입력을 한 번 넣는다(고정 중 옆 대상 넘기기 확인). <요> <피치> [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&LookCommand));
	FAutoConsoleCommand GDumpCommand(TEXT("Grim.Targeting.Dump"),
		TEXT("로컬 조준 상태와 GetCurrentAim 결과를 로그로. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
	FAutoConsoleCommand GSpawnDummyCommand(TEXT("Grim.Targeting.SpawnDummy"),
		TEXT("로컬 폰 앞에 조준 더미. [앞cm=800] [오른쪽cm=0] [Enemy|Creature|Object|Ally] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnDummyCommand));
}

#endif
