// 마법 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 완드 부착 · 전달 방식 복제를 확인할 때 쓴다
// (Scripts/Integration/net_check.ps1 · net_check_delivery.ps1). 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다.

#include "Magic/GrimWand.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGrimMagicDebug, Log, All);

namespace GrimMagicDebugLocal // 유니티 빌드에서 GrimInteractionDebug 의 같은 이름 도우미와 부딪히지 않게 이름을 붙인다
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

	void RunLater(float Delay, TFunction<void()> Fn)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Fn](float)
		{
			Fn();
			return false;
		}), FMath::Max(Delay, 0.001f));
	}

	float ParseFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	/** 클래스 이름에 Filter 가 든 액터 중 월드 원점에 가장 가까운 것 — 서버 · 클라이언트가 같은 것을 고른다 */
	AActor* FindTestTarget(UWorld* World, const FString& Filter)
	{
		if (Filter.IsEmpty() || Filter == TEXT("None"))
		{
			return nullptr;
		}
		AActor* Best = nullptr;
		double BestDistSq = TNumericLimits<double>::Max();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->GetClass()->GetName().Contains(Filter))
			{
				continue;
			}
			const double DistSq = It->GetActorLocation().SizeSquared();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
		return Best;
	}

	void DumpWandsInWorld(UWorld* World)
	{
		const TCHAR* NetMode = NetModeName(World);
		for (TActorIterator<AGrimWand> It(World); It; ++It)
		{
			const USceneComponent* Parent = It->GetRootComponent()->GetAttachParent();
			const FName Socket = It->GetRootComponent()->GetAttachSocketName();
			// 손 소켓에서 완드 원점까지 거리 — 제대로 붙었으면 0 이다.
			const float Offset = Parent ? FVector::Dist(Parent->GetSocketLocation(Socket), It->GetActorLocation()) : -1.f;
			UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s] %s owner=%s parent=%s socket=%s offset=%.1fcm muzzle=%s"),
				NetMode, *It->GetName(), *GetNameSafe(It->GetOwner()),
				Parent ? *FString::Printf(TEXT("%s.%s"), *GetNameSafe(Parent->GetOwner()), *Parent->GetName()) : TEXT("None"),
				*Socket.ToString(), Offset, *It->GetMuzzleTransform().GetLocation().ToCompactString());

			// 쥐는 자세를 정할 때 쓰는 값: 손 소켓 공간에서 본 엄지 · 가운뎃손가락 · 완드 끝 · 폰 앞 방향.
			if (const USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(Parent))
			{
				const FTransform Hand = SkelMesh->GetSocketTransform(Socket);
				auto Local = [&Hand](const FVector& World) { return Hand.InverseTransformPositionNoScale(World).GetSafeNormal(); };
				UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s]   hand space: thumb=%s middle=%s muzzle=%s pawnFwd=%s pawnUp=%s"), NetMode,
					*Local(SkelMesh->GetSocketLocation(TEXT("thumb_02_r"))).ToCompactString(),
					*Local(SkelMesh->GetSocketLocation(TEXT("middle_02_r"))).ToCompactString(),
					*Local(It->GetMuzzleTransform().GetLocation()).ToCompactString(),
					*Hand.InverseTransformVectorNoScale(SkelMesh->GetOwner()->GetActorForwardVector()).ToCompactString(),
					*Hand.InverseTransformVectorNoScale(SkelMesh->GetOwner()->GetActorUpVector()).ToCompactString());
			}
		}
	}

	/** Grim.Magic.DumpWands [Delay] — 이 프로세스에서 본 완드의 부착 부모 · 소켓 · 손에서 떨어진 거리 */
	void DumpWandsCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ParseFloat(Args, 0, 0.f), [](UWorld* World) { DumpWandsInWorld(World); });
	}

	/**
	 * Grim.Magic.Place <TargetClass> <DistanceCm> [Delay] — 서버: 원격 클라이언트 폰을 대상(원점에 가장 가까운 그 클래스)의 −X 쪽
	 * Distance 만큼 떨어진 곳으로 옮기고 대상을 보게 한다.
	 */
	void PlaceCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimMagicDebug, Warning, TEXT("사용: Grim.Magic.Place <대상 클래스 이름 일부> <거리cm> [지연초]"));
			return;
		}
		const FString Filter = Args[0];
		const float Distance = FCString::Atof(*Args[1]);
		RunInGameWorlds(ParseFloat(Args, 2, 0.f), [Filter, Distance](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			AActor* Target = FindTestTarget(World, Filter);
			if (!Target)
			{
				UE_LOG(LogGrimMagicDebug, Warning, TEXT("[%s] Place: 대상 없음 %s"), NetModeName(World), *Filter);
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				APawn* Pawn = PC ? PC->GetPawn() : nullptr;
				if (!Pawn)
				{
					continue;
				}
				if (PC->IsLocalController())
				{
					// 리슨 호스트의 폰은 사선 밖(옆 4m)으로 비켜 둔다 — 시작 지점에 서 있으면 원격 클라이언트의 마법을 몸으로 막는다.
					FVector Aside = Target->GetActorLocation() + FVector(-Distance * 0.5f, 400.f, 0.f);
					Aside.Z = Pawn->GetActorLocation().Z;
					Pawn->SetActorLocation(Aside, false, nullptr, ETeleportType::TeleportPhysics);
					continue;
				}
				const FVector TargetPoint = UGrimSpellAbility::GetTargetPointOf(Target);
				FVector NewLocation = Target->GetActorLocation() - FVector(Distance, 0.f, 0.f);
				NewLocation.Z = Pawn->GetActorLocation().Z;
				Pawn->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				const FRotator Look = (TargetPoint - Pawn->GetPawnViewLocation()).Rotation();
				PC->SetControlRotation(Look);
				PC->ClientSetLocation(NewLocation, Look);
				UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s] Place %s → %s 앞 %.0fcm"), NetModeName(World), *Pawn->GetName(), *Target->GetName(), Distance);
			}
		});
	}

	/**
	 * Grim.Magic.Cast <Spell> [TargetClass] [HoldSeconds] [Delay] — 로컬 플레이어가 정의 이름에 Spell 이 든 마법을 시전한다.
	 * 대상(원점에 가장 가까운 그 클래스)을 보도록 돌린 뒤 0.5초 뒤에 누르고(서버가 새 시선을 받도록), HoldSeconds 뒤에 뗀다.
	 * 조준은 그 대상으로 고정된다(소프트 락온 대신).
	 */
	void CastCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimMagicDebug, Warning, TEXT("사용: Grim.Magic.Cast <마법 이름 일부> [대상 클래스|None] [누르는 초] [지연초]"));
			return;
		}
		const FString SpellName = Args[0];
		const FString Filter = Args.IsValidIndex(1) ? Args[1] : FString();
		const float Hold = ParseFloat(Args, 2, 0.f);

		RunInGameWorlds(ParseFloat(Args, 3, 0.f), [SpellName, Filter, Hold](UWorld* World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			UAbilitySystemComponent* ASC = Pawn ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn->GetPlayerState()) : nullptr;
			if (!ASC)
			{
				UE_LOG(LogGrimMagicDebug, Warning, TEXT("[%s] Cast: 로컬 폰 · ASC 없음"), NetModeName(World));
				return;
			}

			FGameplayAbilitySpecHandle Handle;
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (const UGrimSpellDefinition* Definition = Cast<UGrimSpellDefinition>(Spec.SourceObject.Get()); Definition && Definition->GetName().Contains(SpellName))
				{
					Handle = Spec.Handle;
					break;
				}
			}
			if (!Handle.IsValid())
			{
				UE_LOG(LogGrimMagicDebug, Warning, TEXT("[%s] Cast: 마법 없음 %s"), NetModeName(World), *SpellName);
				return;
			}

			if (AActor* Target = FindTestTarget(World, Filter))
			{
				PC->SetControlRotation((UGrimSpellAbility::GetTargetPointOf(Target) - Pawn->GetPawnViewLocation()).Rotation());
				UGrimSpellAbility::SetDebugAimTarget(Pawn, Target);
			}

			TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
			const FString WorldName = NetModeName(World);
			RunLater(0.5f, [WeakASC, Handle, Hold, SpellName, WorldName]()
			{
				UAbilitySystemComponent* ASC = WeakASC.Get();
				FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
				if (!Spec)
				{
					return;
				}
				Spec->InputPressed = true;
				const bool bActivated = ASC->TryActivateAbility(Handle);
				UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s] Cast %s 발동=%d 누름=%.1fs"), *WorldName, *SpellName, bActivated ? 1 : 0, Hold);

				RunLater(Hold, [WeakASC, Handle]()
				{
					UAbilitySystemComponent* ASC = WeakASC.Get();
					FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
					if (!Spec)
					{
						return;
					}
					Spec->InputPressed = false;
					for (UGameplayAbility* Instance : Spec->GetAbilityInstances())
					{
						if (Instance && Instance->IsActive())
						{
							Instance->InputReleased(Handle, ASC->AbilityActorInfo.Get(), Instance->GetCurrentActivationInfo());
						}
					}
				});
			});
		});
	}

	FAutoConsoleCommand GDumpWandsCommand(
		TEXT("Grim.Magic.DumpWands"),
		TEXT("이 프로세스에서 본 완드의 부착 부모 · 소켓 · 손에서 떨어진 거리를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpWandsCommand));

	FAutoConsoleCommand GPlaceCommand(
		TEXT("Grim.Magic.Place"),
		TEXT("서버: 원격 클라이언트 폰을 대상 앞 거리만큼에 두고 대상을 보게. <대상 클래스> <거리cm> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&PlaceCommand));

	FAutoConsoleCommand GCastCommand(
		TEXT("Grim.Magic.Cast"),
		TEXT("로컬 플레이어가 마법을 시전(대상 고정 조준). <마법 이름 일부> [대상 클래스|None] [누르는 초] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&CastCommand));
}

#endif
