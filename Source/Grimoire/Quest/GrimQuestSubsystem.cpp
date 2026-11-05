#include "Quest/GrimQuestSubsystem.h"
#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTypes.h"
#include "Factions/GrimNoCombatVolume.h"
#include "Factions/GrimTrialTarget.h"
#include "GameFramework/Pawn.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimSkillbook.h"
#include "Magic/GrimSpellDefinition.h"
#include "Survival/GrimBed.h"
#include "Survival/GrimSleepSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

bool UGrimQuestSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimQuestSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

void UGrimQuestSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsServer())
	{
		return;
	}

	// 표를 미리 읽어 문제를 시작 로그에 남긴다.
	UGrimQuestSettings::GetQuestDatabase();

	// PlayerState 가 생기자마자 붙인다 — 세이브(갈래 V)가 플레이어 몫을 읽기 전에 컴포넌트가 있어야 퀘스트 진행도 되돌려진다.
	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			EnsureComponent(PlayerState);
		}
	}));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsureComponent(PlayerState);
		}
	}

	InWorld.GetTimerManager().SetTimer(PollTimer, FTimerDelegate::CreateUObject(this, &UGrimQuestSubsystem::PollNow),
		UGrimQuestSettings::Get()->PollIntervalSeconds, true);

	if (UGrimSleepSubsystem* Sleep = InWorld.GetSubsystem<UGrimSleepSubsystem>())
	{
		Sleep->OnNightSkipBegin.AddDynamic(this, &UGrimQuestSubsystem::HandleNightSkipBegin);
		Sleep->OnNightSkipped.AddDynamic(this, &UGrimQuestSubsystem::HandleNightSkipped);
		Sleep->OnNightSkipCancelled.AddDynamic(this, &UGrimQuestSubsystem::HandleNightSkipCancelled);
	}
}

void UGrimQuestSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

bool UGrimQuestSubsystem::IsAutoStartMap() const
{
	const TArray<FString>& Maps = UGrimQuestSettings::Get()->AutoStartMaps;
	if (Maps.Num() == 0)
	{
		return true;
	}
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const FString MapName = UWorld::RemovePIEPrefix(World->GetMapName());
	return Maps.ContainsByPredicate([&MapName](const FString& Name) { return MapName.Equals(Name, ESearchCase::IgnoreCase); });
}

UGrimQuestComponent* UGrimQuestSubsystem::EnsureComponent(APlayerState* PlayerState)
{
	if (!PlayerState || !IsServer() || !Cast<AGrimPlayerState>(PlayerState))
	{
		return nullptr;
	}
	if (UGrimQuestComponent* Existing = UGrimQuestComponent::FindForPlayerState(PlayerState))
	{
		return Existing;
	}
	UGrimQuestComponent* Quest = NewObject<UGrimQuestComponent>(PlayerState, UGrimQuestComponent::StaticClass(), UGrimQuestComponent::ComponentName);
	PlayerState->AddInstanceComponent(Quest);
	Quest->RegisterComponent();
	UE_LOG(LogGrimQuest, Log, TEXT("[Server] %s 에 퀘스트 컴포넌트를 붙였다"), *GetNameSafe(PlayerState));
	return Quest;
}

void UGrimQuestSubsystem::PollNow()
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState || !IsServer())
	{
		return;
	}
	// 반복 중에 배열이 바뀌지 않게 복사(퀘스트 진행이 액터를 새로 놓을 수 있다).
	const TArray<TObjectPtr<APlayerState>> Players = GameState->PlayerArray;
	for (APlayerState* PlayerState : Players)
	{
		if (!Cast<AGrimPlayerState>(PlayerState))
		{
			continue;
		}
		if (UGrimQuestComponent* Quest = EnsureComponent(PlayerState))
		{
			// 폰이 생긴 뒤에 시작(위치 목표 · 줄 것이 폰을 본다).
			if (APawn* Pawn = PlayerState->GetPawn())
			{
				Quest->TryAutoStart();
				// 폰이 스폰될 때는 아직 PlayerState(ASC)가 없어 볼륨 겹침이 태그를 못 붙인다 — 주기마다 맞춘다.
				AGrimNoCombatVolume::RefreshPawn(Pawn);
			}
			Quest->ServerPoll();
		}
	}
}

void UGrimQuestSubsystem::GiveItems(const TArray<FGrimQuestGive>& Gives, UGrimQuestComponent* Quest)
{
	UWorld* World = GetWorld();
	const APawn* Pawn = Quest ? Quest->GetPawn() : nullptr;
	if (!World || !IsServer() || Gives.Num() == 0)
	{
		return;
	}
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const float FeetOffset = Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;

	for (int32 Index = 0; Index < Gives.Num(); ++Index)
	{
		const FGrimQuestGive& Give = Gives[Index];
		if (Give.Kind == FGrimQuestGive::EKind::Item)
		{
			// TODO(M4): 인벤토리가 생기면 아이템 Give.Id 를 플레이어 가방에 넣는다.
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] 아이템 %s 지급 — 인벤토리(M4) 전이라 기록만"), *Give.Id.ToString());
			continue;
		}

		// 놓을 곳: 태그 액터 곁, 없으면 플레이어 앞(여럿이면 옆으로 벌린다). 높이는 바닥 기준.
		FVector Location;
		FRotator Rotation = FRotator::ZeroRotator;
		const AActor* Anchor = nullptr;
		if (!Give.AtTag.IsNone())
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(Give.AtTag))
				{
					Anchor = *It;
					break;
				}
			}
		}
		if (Anchor)
		{
			Location = Anchor->GetActorLocation() + FVector(0.f, 90.f * Index, 0.f);
			Rotation = FRotator(0.f, Anchor->GetActorRotation().Yaw, 0.f);
		}
		else if (Pawn)
		{
			const FRotator Facing(0.f, Pawn->GetActorRotation().Yaw, 0.f);
			const float Side = (Index - (Gives.Num() - 1) * 0.5f) * 90.f;
			Location = Pawn->GetActorLocation() + Facing.RotateVector(FVector(150.f, Side, 0.f)) - FVector(0.f, 0.f, FeetOffset);
			Rotation = Facing;
		}
		else
		{
			UE_LOG(LogGrimQuest, Warning, TEXT("[Server] 줄 곳이 없다(폰 · 태그 %s 없음)"), *Give.AtTag.ToString());
			continue;
		}

		if (Give.Kind == FGrimQuestGive::EKind::Skillbook)
		{
			UGrimSpellDefinition* Spell = UGrimQuestSettings::LoadSpell(Give.Id);
			if (!Spell)
			{
				// TODO(통합): 기본 공격(갈래 K) · 길들이기(M3, 이름 미정) 정의가 생기면 그 ID 로.
				UE_LOG(LogGrimQuest, Warning, TEXT("[Server] 스킬북 %s 를 줄 수 없다 — 마법 정의 %s 가 없다"), *Give.Id.ToString(),
					*UGrimQuestSettings::GetSpellAssetPath(Give.Id));
				continue;
			}
			const FTransform Transform(Rotation, Location + FVector(0.f, 0.f, 40.f));
			AGrimSkillbook* Book = World->SpawnActorDeferred<AGrimSkillbook>(AGrimSkillbook::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Book)
			{
				Book->SetTeachesSpell(Spell);
				Book->Tags.Add(FName(*FString::Printf(TEXT("Given_Skillbook_%s"), *Give.Id.ToString())));
				Book->FinishSpawning(Transform);
				UE_LOG(LogGrimQuest, Display, TEXT("[Server] 스킬북 %s 를 놓았다 %s"), *Give.Id.ToString(), *Transform.GetLocation().ToCompactString());
			}
		}
		else if (Give.Kind == FGrimQuestGive::EKind::Bedroll)
		{
			const FTransform Transform(Rotation, Location);
			AGrimBed* Bedroll = World->SpawnActorDeferred<AGrimBed>(AGrimBed::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Bedroll)
			{
				Bedroll->SetBedroll(true);
				Bedroll->Tags.Add(TEXT("Given_Bedroll"));
				Bedroll->FinishSpawning(Transform);
				UE_LOG(LogGrimQuest, Display, TEXT("[Server] 침낭을 놓았다 %s"), *Transform.GetLocation().ToCompactString());
			}
		}
	}
}

void UGrimQuestSubsystem::ApplyStepStart(const FGrimQuestStepDef& Step, UGrimQuestComponent* Quest)
{
	UWorld* World = GetWorld();
	if (!World || !IsServer())
	{
		return;
	}
	if (UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
	{
		float NewTime = 0.f;
		bool bValid = true;
		if (FGrimQuestDatabase::ParseTimeOfDay(Step.TimeOfDayOnStart, TimeOfDay->GetDayRatio(), NewTime, bValid))
		{
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] 시각을 옮긴다: %s → %.3f(%.1f시)"), *Step.TimeOfDayOnStart, NewTime, NewTime * 24.f);
			TimeOfDay->SetTimeOfDay(NewTime);
		}
	}
	if (!Step.PropsTag.IsNone())
	{
		SetPropsActive(Step.PropsTag, true, true);
	}
	GiveItems(Step.Gives, Quest);
}

void UGrimQuestSubsystem::ApplyStepEnd(const FGrimQuestStepDef& Step)
{
	if (!Step.PropsTag.IsNone())
	{
		SetPropsActive(Step.PropsTag, false, false);
	}
}

void UGrimQuestSubsystem::SetPropsActive(FName Tag, bool bActive, bool bReset)
{
	UWorld* World = GetWorld();
	if (!World || !IsServer() || Tag.IsNone())
	{
		return;
	}
	TArray<AActor*> Props;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag) && !It->IsActorBeingDestroyed())
		{
			Props.Add(*It);
		}
	}
	int32 Reset = 0;
	for (AActor* Prop : Props)
	{
		if (AGrimTrialTarget* Target = Cast<AGrimTrialTarget>(Prop))
		{
			Target->SetTrialActive(bActive);
		}
		else if (bReset && Prop->FindComponentByClass<UGrimElementalStateComponent>())
		{
			ResetElementalActor(Prop);
			++Reset;
		}
	}
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 소품 %s: %d 개 %s%s"), *Tag.ToString(), Props.Num(), bActive ? TEXT("켬") : TEXT("끔"),
		bReset ? *FString::Printf(TEXT(", 되돌림 검사 %d"), Reset) : TEXT(""));
}

void UGrimQuestSubsystem::ResetElementalActor(AActor* Actor)
{
	UGrimElementalStateComponent* State = Actor ? Actor->FindComponentByClass<UGrimElementalStateComponent>() : nullptr;
	UWorld* World = GetWorld();
	if (!State || !World)
	{
		return;
	}

	// 지금 상태가 처음 상태(InitialStates)와 같으면 그대로 둔다.
	bool bDiffers = false;
	for (const FGrimElementalState& Item : State->GetStates().Items)
	{
		if (!FMath::IsNearlyEqual(Item.Intensity, State->InitialStates.FindRef(Item.StateTag)))
		{
			bDiffers = true;
		}
	}
	for (const TPair<FGameplayTag, float>& Pair : State->InitialStates)
	{
		if (!FMath::IsNearlyEqual(State->GetStateIntensity(Pair.Key), Pair.Value))
		{
			bDiffers = true;
		}
	}
	if (!bDiffers)
	{
		return;
	}

	// 상태 컴포넌트에는 「처음으로」가 없다(속성 갈래 파일) — 같은 클래스를 같은 자리에 새로 놓고 옛것을 지운다.
	const FTransform Transform = Actor->GetActorTransform();
	AActor* Fresh = World->SpawnActorDeferred<AActor>(Actor->GetClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Fresh)
	{
		return;
	}
	Fresh->Tags = Actor->Tags;
	if (UGrimElementalStateComponent* FreshState = Fresh->FindComponentByClass<UGrimElementalStateComponent>())
	{
		FreshState->InitialStates = State->InitialStates;
		FreshState->Material = State->Material;
		FreshState->ReactionTable = State->ReactionTable;
	}
	Fresh->FinishSpawning(Transform);
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 소품 %s 를 처음 상태로(새로 놓음 %s)"), *Actor->GetName(), *Fresh->GetName());
	Actor->Destroy();
}

void UGrimQuestSubsystem::HandleNightSkipBegin(float FadeSeconds)
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (UGrimQuestComponent* Quest = UGrimQuestComponent::FindForPlayerState(PlayerState))
		{
			Quest->NotifyNightSkipBegin();
		}
	}
}

void UGrimQuestSubsystem::HandleNightSkipped(float SkippedDays)
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 밤을 건너뛰었다(%.3f일) — 잠든 사람에게 신호"), SkippedDays);
	const TArray<TObjectPtr<APlayerState>> Players = GameState->PlayerArray;
	for (APlayerState* PlayerState : Players)
	{
		if (UGrimQuestComponent* Quest = UGrimQuestComponent::FindForPlayerState(PlayerState))
		{
			Quest->NotifyNightSkipped();
		}
	}
}

void UGrimQuestSubsystem::HandleNightSkipCancelled()
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (UGrimQuestComponent* Quest = UGrimQuestComponent::FindForPlayerState(PlayerState))
		{
			Quest->NotifyNightSkipCancelled();
		}
	}
}
