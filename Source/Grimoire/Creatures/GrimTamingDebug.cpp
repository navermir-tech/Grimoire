// 길들이기 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 파크툼 · 따라오기 · 탑승 · 탄 채 시전을 확인할 때 쓴다(Scripts/Creatures/net_check_taming.ps1).
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다(생물 디버그 명령과 같은 방식).

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Creatures/GrimTamingTags.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellbookComponent.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "Quest/GrimQuestSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimTamingDebugLocal // 유니티 빌드 이름 충돌 방지
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("Server");
		case NM_DedicatedServer: return TEXT("Server");
		default: return TEXT("Standalone");
		}
	}

	void RunLater(float Delay, TFunction<void()> Fn)
	{
		if (Delay <= 0.f)
		{
			Fn();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Fn](float)
		{
			Fn();
			return false;
		}), Delay);
	}

	void RunInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		RunLater(Delay, [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					Fn(World);
				}
			}
		});
	}

	float ArgFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	bool MatchesSpecies(const AGrimCreature* Creature, const FString& Filter)
	{
		return Filter.IsEmpty() || Filter == TEXT("*") || Creature->GetInstance().SpeciesId.ToString().Equals(Filter, ESearchCase::IgnoreCase);
	}

	/** 폰에 가장 가까운 생물(bTamed: 길들인 것만 / 야생만) */
	AGrimCreature* FindNearest(UWorld* World, const APawn* Pawn, const FString& Filter, int32 TamedMode)
	{
		AGrimCreature* Best = nullptr;
		float BestDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AGrimCreature> It(World); It; ++It)
		{
			if (It->IsDead() || !MatchesSpecies(*It, Filter))
			{
				continue;
			}
			const bool bTamed = It->GetInstance().IsTamed();
			if ((TamedMode == 1 && !bTamed) || (TamedMode == 0 && bTamed))
			{
				continue;
			}
			const float Distance = Pawn ? FVector::Dist(Pawn->GetActorLocation(), It->GetActorLocation()) : 0.f;
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}

	/** Grim.Taming.Grant [서클=3] [지연초] — 서버: 모든 플레이어에게 서클을 올리고 파크툼(무속성 · 속성 5)을 가르친다 */
	void GrantCommand(const TArray<FString>& Args)
	{
		const int32 Circle = FMath::Max(1, static_cast<int32>(ArgFloat(Args, 0, 3.f)));
		RunInGameWorlds(ArgFloat(Args, 1, 0.f), [Circle](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client || !World->GetGameState())
			{
				return;
			}
			static const TCHAR* Spells[] = { TEXT("Pactum"), TEXT("Pactum_Ignis"), TEXT("Pactum_Aqua"), TEXT("Pactum_Ventus"), TEXT("Pactum_Terra"), TEXT("Pactum_Umbra") };
			for (APlayerState* PlayerState : World->GetGameState()->PlayerArray)
			{
				AGrimPlayerState* GrimState = Cast<AGrimPlayerState>(PlayerState);
				UAbilitySystemComponent* ASC = GrimState ? GrimState->GetAbilitySystemComponent() : nullptr;
				UGrimSpellbookComponent* Spellbook = GrimState ? GrimState->GetSpellbook() : nullptr;
				if (!ASC || !Spellbook)
				{
					continue;
				}
				const float Current = ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute());
				ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), FMath::Max<float>(Current, Circle));
				int32 Learned = 0;
				for (const TCHAR* Id : Spells)
				{
					if (UGrimSpellDefinition* Spell = UGrimQuestSettings::LoadSpell(Id); Spell && !Spellbook->KnowsSpell(Spell) && Spellbook->LearnSpell(Spell))
					{
						++Learned;
					}
				}
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] Grant %s 서클 %d 파크툼 %d개"), NetModeName(World), *PlayerState->GetPlayerName(), Circle, Learned);
			}
		});
	}

	/**
	 * Grim.Taming.Cast <마법 에셋 이름> <종 id|*> [누르는 초] [지연초] — 로컬 플레이어가 그 종의 가장 가까운 야생 생물을 겨눠 시전한다.
	 * 에셋 이름이 꼭 같은 것을 먼저, 없으면 이름에 든 것. 탄 채 시전도 이 명령으로 본다.
	 */
	void CastCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("사용: Grim.Taming.Cast <마법 에셋 이름> <종 id|*> [누르는 초] [지연초]"));
			return;
		}
		const FString SpellName = Args[0];
		const FString Filter = Args[1];
		const float Hold = ArgFloat(Args, 2, 0.f);
		RunInGameWorlds(ArgFloat(Args, 3, 0.f), [SpellName, Filter, Hold](UWorld* World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			UAbilitySystemComponent* ASC = Pawn ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn->GetPlayerState()) : nullptr;
			if (!ASC || !PC->IsLocalController())
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Cast: 로컬 폰 · ASC 없음"), NetModeName(World));
				return;
			}
			FGameplayAbilitySpecHandle Handle;
			for (int32 Pass = 0; Pass < 2 && !Handle.IsValid(); ++Pass)
			{
				for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
				{
					const UGrimSpellDefinition* Definition = Cast<UGrimSpellDefinition>(Spec.SourceObject.Get());
					if (Definition && (Pass == 0 ? Definition->GetName().Equals(SpellName, ESearchCase::IgnoreCase) : Definition->GetName().Contains(SpellName)))
					{
						Handle = Spec.Handle;
						break;
					}
				}
			}
			AGrimCreature* Target = FindNearest(World, Pawn, Filter, 0);
			if (!Handle.IsValid() || !Target)
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Cast: 마법 %s(%d) 또는 대상 %s(%d) 없음"), NetModeName(World), *SpellName, Handle.IsValid() ? 1 : 0, *Filter, Target ? 1 : 0);
				return;
			}
			PC->SetControlRotation((UGrimSpellAbility::GetTargetPointOf(Target) - Pawn->GetPawnViewLocation()).Rotation());
			UGrimSpellAbility::SetDebugAimTarget(Pawn, Target);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
			TWeakObjectPtr<APlayerController> WeakPC = PC;
			TWeakObjectPtr<AGrimCreature> WeakTarget = Target;
			const FString WorldName = NetModeName(World);
			auto DoCast = [WeakASC, WeakPC, WeakTarget, Handle, Hold, SpellName, WorldName]()
			{
				UAbilitySystemComponent* ASC = WeakASC.Get();
				FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
				if (!Spec)
				{
					return;
				}
				if (WeakPC.IsValid() && WeakTarget.IsValid() && WeakPC->GetPawn())
				{
					UGrimSpellAbility::SetDebugAimTarget(WeakPC->GetPawn(), WeakTarget.Get());
				}
				const bool bMounted = WeakPC.IsValid() && UGrimMountComponent::FindMountOf(WeakPC->GetPawn()) != nullptr;
				Spec->InputPressed = true;
				const bool bActivated = ASC->TryActivateAbility(Handle);
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Cast %s → %s 발동=%d 탄채=%d"), *WorldName, *SpellName, WeakTarget.IsValid() ? *WeakTarget->GetInstance().SpeciesId.ToString() : TEXT("?"),
					bActivated ? 1 : 0, bMounted ? 1 : 0);
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
			};
			// 0.6초 동안 매 프레임 대상(도망치는 생물)을 보도록 돌려 서버가 새 시선을 받게 한 뒤 누른다.
			const double CastTime = FPlatformTime::Seconds() + 0.6;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakPC, WeakTarget, CastTime, DoCast](float)
			{
				if (WeakPC.IsValid() && WeakTarget.IsValid() && WeakPC->GetPawn())
				{
					WeakPC->SetControlRotation((UGrimSpellAbility::GetTargetPointOf(WeakTarget.Get()) - WeakPC->GetPawn()->GetPawnViewLocation()).Rotation());
				}
				if (FPlatformTime::Seconds() < CastTime)
				{
					return true;
				}
				DoCast();
				return false;
			}));
		});
	}

	/** 서버: 원격 클라이언트 폰들을 그 종의 가장 가까운 야생 생물 앞 거리만큼에 두고 그쪽을 보게(탄 사람은 건너뛴다) */
	void PlaceCommand(const TArray<FString>& Args)
	{
		const FString Filter = Args.IsValidIndex(0) ? Args[0] : FString();
		const float Distance = ArgFloat(Args, 1, 600.f);
		RunInGameWorlds(ArgFloat(Args, 2, 0.f), [Filter, Distance](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				APawn* Pawn = PC ? PC->GetPawn() : nullptr;
				if (!Pawn || PC->IsLocalController() || UGrimMountComponent::FindMountOf(Pawn))
				{
					continue;
				}
				AGrimCreature* Target = FindNearest(World, Pawn, Filter, 0);
				if (!Target)
				{
					UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Place: 대상 %s 없음"), NetModeName(World), *Filter);
					continue;
				}
				FVector Away = (Pawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
				if (Away.IsNearlyZero())
				{
					Away = FVector::BackwardVector;
				}
				FVector NewLocation = Target->GetActorLocation() + Away * Distance;
				NewLocation.Z = Pawn->GetActorLocation().Z;
				Pawn->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				const FRotator Look = (UGrimSpellAbility::GetTargetPointOf(Target) - Pawn->GetPawnViewLocation()).Rotation();
				PC->SetControlRotation(Look);
				PC->ClientSetLocation(NewLocation, Look);
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Place %s → %s seed=%d 앞 %.0fcm"), NetModeName(World), *Pawn->GetName(), *Target->GetInstance().SpeciesId.ToString(), Target->GetInstance().Seed, Distance);
			}
		});
	}

	/** 서버: 원격 클라이언트 폰 앞(시선 방향) 거리에 그 종을 놓는다(탄 채 시전 표적) */
	void SpawnNearClientCommand(const TArray<FString>& Args)
	{
		const FName SpeciesId(Args.IsValidIndex(0) ? *Args[0] : TEXT("Jackalope"));
		const float Distance = ArgFloat(Args, 1, 800.f);
		RunInGameWorlds(ArgFloat(Args, 2, 0.f), [SpeciesId, Distance](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(SpeciesId);
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				APawn* Pawn = PC ? PC->GetPawn() : nullptr;
				if (!Pawn || PC->IsLocalController() || !Species)
				{
					continue;
				}
				const FVector Forward = FRotator(0.f, PC->GetControlRotation().Yaw, 0.f).Vector();
				const FVector Point = Pawn->GetActorLocation() + Forward * Distance;
				FVector Ground = Point;
				if (!UGrimEcosystemSubsystem::FindGround(World, Point, Ground))
				{
					UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.SpawnNearClient: 바닥 없음(맵 밖) — 놓지 않는다"), NetModeName(World));
					continue;
				}
				const FGrimCreatureInstance Instance = GrimCreatureMath::RollInstance(*Species, FMath::Rand() + 1, UGrimCreatureSettings::Get()->Roll);
				AGrimCreature::SpawnCreature(World, Species, Instance, Ground, 0.f);
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.SpawnNearClient %s 앞 %.0fcm"), NetModeName(World), *SpeciesId.ToString(), Distance);
			}
		});
	}

	/** Grim.Taming.Act <Follow|Wait|Mount|Feed|Release|Dismount|Name> [종 id|*] [이름] [지연초] — 로컬 플레이어: 가장 가까운 내 생물에게(먹이는 야생도) */
	void ActCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("사용: Grim.Taming.Act <Follow|Wait|Mount|Feed|Release|Dismount|Name> [종 id|*] [이름] [지연초]"));
			return;
		}
		const FString Action = Args[0];
		const FString Filter = Args.IsValidIndex(1) ? Args[1] : FString();
		const FString Name = Args.IsValidIndex(2) ? Args[2] : FString();
		RunInGameWorlds(ArgFloat(Args, 3, 0.f), [Action, Filter, Name](UWorld* World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			APawn* Pawn = PC && PC->IsLocalController() ? PC->GetPawn() : nullptr;
			UGrimTamerComponent* Tamer = UGrimTamerComponent::FindForActor(PC);
			if (!Pawn || !Tamer)
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Act: 로컬 폰 · 길들이기 컴포넌트 없음"), NetModeName(World));
				return;
			}
			if (Action.Equals(TEXT("Dismount"), ESearchCase::IgnoreCase))
			{
				Tamer->RequestDismount();
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Act Dismount"), NetModeName(World));
				return;
			}
			const bool bFeed = Action.Equals(TEXT("Feed"), ESearchCase::IgnoreCase);
			AGrimCreature* Target = FindNearest(World, Pawn, Filter, bFeed ? -1 : 1);
			if (!Target || (!bFeed && !Tamer->OwnsCreature(Target)))
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Act %s: 대상 %s 없음"), NetModeName(World), *Action, *Filter);
				return;
			}
			if (Action.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				Tamer->RequestRename(Target, Name);
			}
			else
			{
				static const TPair<const TCHAR*, EGrimTamerAction> Map[] = {
					{ TEXT("Follow"), EGrimTamerAction::Follow }, { TEXT("Wait"), EGrimTamerAction::Wait }, { TEXT("Mount"), EGrimTamerAction::Mount },
					{ TEXT("Feed"), EGrimTamerAction::Feed }, { TEXT("Release"), EGrimTamerAction::Release } };
				const TPair<const TCHAR*, EGrimTamerAction>* Found = nullptr;
				for (const TPair<const TCHAR*, EGrimTamerAction>& Pair : Map)
				{
					if (Action.Equals(Pair.Key, ESearchCase::IgnoreCase))
					{
						Found = &Pair;
					}
				}
				if (!Found)
				{
					UE_LOG(LogGrimTaming, Warning, TEXT("Taming.Act: 모르는 행동 %s"), *Action);
					return;
				}
				Tamer->RequestAction(Target, Found->Value);
			}
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Act %s → %s seed=%d"), NetModeName(World), *Action, *Target->GetInstance().SpeciesId.ToString(), Target->GetInstance().Seed);
		});
	}

	/** Grim.Taming.Move <앞 0~1> <옆 -1~1> <초> [지연초] — 로컬 플레이어: 카메라 기준 이동 입력을 넣는다(헤드리스 탑승 이동) */
	void MoveCommand(const TArray<FString>& Args)
	{
		const float Forward = ArgFloat(Args, 0, 1.f);
		const float Right = ArgFloat(Args, 1, 0.f);
		const float Seconds = ArgFloat(Args, 2, 2.f);
		RunInGameWorlds(ArgFloat(Args, 3, 0.f), [Forward, Right, Seconds](UWorld* World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			if (!PC || !PC->IsLocalController())
			{
				return;
			}
			TWeakObjectPtr<APlayerController> WeakPC = PC;
			const double EndTime = FPlatformTime::Seconds() + Seconds;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakPC, Forward, Right, EndTime](float)
			{
				APawn* Pawn = WeakPC.IsValid() ? WeakPC->GetPawn() : nullptr;
				if (!Pawn || FPlatformTime::Seconds() >= EndTime)
				{
					return false;
				}
				const FRotator Yaw(0.f, WeakPC->GetControlRotation().Yaw, 0.f);
				Pawn->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
				Pawn->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
				return true;
			}));
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Move %.1f %.1f %.1fs"), NetModeName(World), Forward, Right, Seconds);
		});
	}

	/** Grim.Taming.Loyalty <사건 이름|Set> [값] [종 id|*] [지연초] — 서버: 길들인 생물에 충성도 사건(또는 값) */
	void LoyaltyCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("사용: Grim.Taming.Loyalty <Fed|Starved|...|Set> [값] [종 id|*] [지연초]"));
			return;
		}
		const FString EventName = Args[0];
		const float Value = ArgFloat(Args, 1, 1.f);
		const FString Filter = Args.IsValidIndex(2) ? Args[2] : FString();
		RunInGameWorlds(ArgFloat(Args, 3, 0.f), [EventName, Value, Filter](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			const UEnum* Enum = StaticEnum<EGrimLoyaltyEvent>();
			const int64 EventValue = Enum ? Enum->GetValueByNameString(EventName) : INDEX_NONE;
			int32 Count = 0;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				UGrimCreatureTamingComponent* Taming = It->GetTaming();
				if (!Taming || !Taming->IsTamed() || !MatchesSpecies(*It, Filter))
				{
					continue;
				}
				if (EventName.Equals(TEXT("Set"), ESearchCase::IgnoreCase))
				{
					Taming->DebugSetLoyalty(Value);
				}
				else if (EventValue != INDEX_NONE)
				{
					Taming->ApplyLoyaltyEvent(static_cast<EGrimLoyaltyEvent>(EventValue), FMath::Max(0.f, Value));
				}
				++Count;
			}
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Loyalty %s %.1f -> %d마리"), NetModeName(World), *EventName, Value, Count);
		});
	}

	/** Grim.Taming.Dump [지연초] — 이 프로세스에서 본 생물의 길들이기 상태(seed 로 서버 · 클라이언트를 잇는다) */
	void DumpCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			TArray<AGrimCreature*> List;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				if (!It->IsDead())
				{
					List.Add(*It);
				}
			}
			List.Sort([](const AGrimCreature& A, const AGrimCreature& B) { return A.GetInstance().Seed < B.GetInstance().Seed; });
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] TamingDump 생물 %d"), NetModeName(World), List.Num());
			for (const AGrimCreature* Creature : List)
			{
				const FGrimCreatureInstance& Instance = Creature->GetInstance();
				const UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
				const UGrimMountComponent* Mount = Creature->GetMount();
				const ACharacter* Rider = Mount ? Mount->GetRider() : nullptr;
				const UAbilitySystemComponent* RiderASC = Rider ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Rider) : nullptr;
				const bool bRiderTag = RiderASC && RiderASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Mounted);
				const bool bAttached = Rider && Rider->GetAttachParentActor() == Creature;
				FString Reasons;
				if (Taming)
				{
					for (const FGrimLoyaltyReason& Reason : Taming->GetRecentReasons())
					{
						Reasons += FString::Printf(TEXT("%s%s%+.0f"), Reasons.IsEmpty() ? TEXT("") : TEXT(","), GrimTamingMath::EventName(Reason.Event), Reason.Delta);
					}
				}
				const UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent();
				FString StageTag;
				if (ASC && Taming && Taming->IsTamed())
				{
					StageTag = ASC->HasMatchingGameplayTag(GrimTamingTags::GetStageTag(Taming->GetStage())) ? TEXT("ok") : TEXT("missing");
				}
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] tame seed=%d %s owner=%s loyalty=%.0f nick=\"%s\" cmd=%s ko=%d rider=%s riderTag=%d attached=%d reasons={%s} stageTag=%s"),
					NetModeName(World), Instance.Seed, *Instance.SpeciesId.ToString(), Instance.OwnerId.IsEmpty() ? TEXT("-") : *Instance.OwnerId, Instance.Loyalty, *Instance.Nickname,
					Taming && Taming->GetCommand() == EGrimTamedCommand::Wait ? TEXT("Wait") : TEXT("Follow"), Taming && Taming->IsKnockedOut() ? 1 : 0,
					Rider ? (Rider->GetPlayerState() ? *Rider->GetPlayerState()->GetPlayerName() : TEXT("?")) : TEXT("-"), bRiderTag ? 1 : 0, bAttached ? 1 : 0, *Reasons, StageTag.IsEmpty() ? TEXT("-") : *StageTag);
				// 위치 · AI 는 순간 다를 수 있어 따로 찍는다(비교에서 뺀다).
				const UEnum* StateEnum = StaticEnum<EGrimCreatureAIState>();
				const APawn* OwnerPawn = Taming ? Taming->GetOwnerPawn() : nullptr;
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] tamepos seed=%d pos=(%.0f,%.0f) state=%s ownerDist=%.0f water=%d move=%s"), NetModeName(World), Instance.Seed,
					Creature->GetActorLocation().X, Creature->GetActorLocation().Y,
					StateEnum ? *StateEnum->GetNameStringByValue(static_cast<int64>(Creature->GetAIState())) : TEXT("?"),
					OwnerPawn ? FVector::Dist(OwnerPawn->GetActorLocation(), Creature->GetActorLocation()) : -1.f, Mount && Mount->IsWaterRunning() ? 1 : 0,
					Creature->GetCharacterMovement() ? *Creature->GetCharacterMovement()->GetMovementName() : TEXT("?"));
			}
		});
	}

	FAutoConsoleCommand GGrantCommand(TEXT("Grim.Taming.Grant"), TEXT("서버: 모든 플레이어 서클을 올리고 파크툼을 가르친다. [서클=3] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&GrantCommand));
	FAutoConsoleCommand GCastCommand(TEXT("Grim.Taming.Cast"), TEXT("로컬 플레이어가 가장 가까운 야생 생물에 시전. <마법 에셋 이름> <종 id|*> [누르는 초] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&CastCommand));
	FAutoConsoleCommand GActCommand(TEXT("Grim.Taming.Act"), TEXT("로컬 플레이어: 내 생물에게 행동. <Follow|Wait|Mount|Feed|Release|Dismount|Name> [종 id|*] [이름] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&ActCommand));
	FAutoConsoleCommand GMoveCommand(TEXT("Grim.Taming.Move"), TEXT("로컬 플레이어 이동 입력. <앞> <옆> <초> [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&MoveCommand));
	FAutoConsoleCommand GLoyaltyCommand(TEXT("Grim.Taming.Loyalty"), TEXT("서버: 길들인 생물 충성도 사건 · 값. <사건|Set> [값] [종 id|*] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&LoyaltyCommand));
	FAutoConsoleCommand GPlaceCommand(TEXT("Grim.Taming.Place"), TEXT("서버: 원격 클라이언트를 그 종의 야생 생물 앞에. <종 id|*> [거리cm] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&PlaceCommand));
	FAutoConsoleCommand GSpawnNearCommand(TEXT("Grim.Taming.SpawnNearClient"), TEXT("서버: 원격 클라이언트 앞에 생물. <종 id> [거리cm] [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnNearClientCommand));
	FAutoConsoleCommand GDumpCommand(TEXT("Grim.Taming.Dump"), TEXT("이 프로세스에서 본 생물의 길들이기 상태. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
}

#endif
