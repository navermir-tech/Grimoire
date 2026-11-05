#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimElementalPropagationSubsystem.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimInteractionSettings.h"
#include "Engine/DataTable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGrimElementalStateComponent::UGrimElementalStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	States.Owner = this;
}

void UGrimElementalStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimElementalStateComponent, States);
}

void UGrimElementalStateComponent::BeginPlay()
{
	Super::BeginPlay();
	States.Owner = this;

	if (HasAuthorityToChange() && InitialStates.Num() > 0)
	{
		TArray<FGrimElementalStateChange> Changes;
		for (const TPair<FGameplayTag, float>& Pair : InitialStates)
		{
			States.SetIntensity(Pair.Key, Pair.Value, Changes);
		}
		CommitChanges(Changes);
	}
}

void UGrimElementalStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StateTimerHandle);
		if (bRegisteredForPropagation)
		{
			if (UGrimElementalPropagationSubsystem* Propagation = World->GetSubsystem<UGrimElementalPropagationSubsystem>())
			{
				Propagation->UnregisterSource(this);
			}
			bRegisteredForPropagation = false;
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool UGrimElementalStateComponent::HasAuthorityToChange() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

const UDataTable* UGrimElementalStateComponent::GetReactionTable() const
{
	return ReactionTable ? ReactionTable.Get() : FGrimElementalReactionLibrary::GetDefaultTable();
}

void UGrimElementalStateComponent::ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* Instigator)
{
	if (!HasAuthorityToChange())
	{
		return;
	}

	TArray<FGrimElementalStateChange> Changes;
	FGrimElementalReactionLibrary::ApplyStimulus(GetReactionTable(), States, Material, Stimulus,
		GetDefault<UGrimInteractionSettings>()->MaxPropagationRadius, Changes);
	CommitChanges(Changes);
}

float UGrimElementalStateComponent::GetElementalStateIntensity(const FGameplayTag& StateTag) const
{
	return States.GetIntensity(StateTag);
}

void UGrimElementalStateComponent::AdvanceTime(float DeltaSeconds)
{
	if (!HasAuthorityToChange())
	{
		return;
	}

	TArray<FGrimElementalStateChange> Changes;
	FGrimElementalReactionLibrary::AdvanceTime(GetReactionTable(), States, Material, DeltaSeconds,
		GetDefault<UGrimInteractionSettings>()->MaxPropagationRadius, Changes);
	CommitChanges(Changes);
}

void UGrimElementalStateComponent::CommitChanges(const TArray<FGrimElementalStateChange>& Changes)
{
	if (Changes.Num() > 0)
	{
		for (const FGrimElementalStateChange& Change : Changes)
		{
			OnStateChanged.Broadcast(this, Change.StateTag, Change.OldIntensity, Change.NewIntensity);
			OnStateChangedNative.Broadcast(this, Change.StateTag, Change.OldIntensity, Change.NewIntensity);
		}
	}
	UpdateTimerAndPropagation();
}

void UGrimElementalStateComponent::UpdateTimerAndPropagation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const bool bNeedsTimer = States.Items.ContainsByPredicate([](const FGrimElementalState& Item) { return Item.IsTimed(); });
	FTimerManager& TimerManager = World->GetTimerManager();
	if (bNeedsTimer && !TimerManager.IsTimerActive(StateTimerHandle))
	{
		const float Interval = GetDefault<UGrimInteractionSettings>()->StateUpdateInterval;
		TimerManager.SetTimer(StateTimerHandle, this, &UGrimElementalStateComponent::HandleStateTimer, Interval, true);
	}
	else if (!bNeedsTimer && StateTimerHandle.IsValid())
	{
		TimerManager.ClearTimer(StateTimerHandle);
	}

	const bool bNeedsPropagation = HasActivePropagation();
	if (bNeedsPropagation != bRegisteredForPropagation)
	{
		if (UGrimElementalPropagationSubsystem* Propagation = World->GetSubsystem<UGrimElementalPropagationSubsystem>())
		{
			if (bNeedsPropagation)
			{
				Propagation->RegisterSource(this);
			}
			else
			{
				Propagation->UnregisterSource(this);
			}
			bRegisteredForPropagation = bNeedsPropagation;
		}
	}
}

void UGrimElementalStateComponent::HandleStateTimer()
{
	AdvanceTime(GetDefault<UGrimInteractionSettings>()->StateUpdateInterval);
}

bool UGrimElementalStateComponent::IsStateTimerActive() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(StateTimerHandle);
}

bool UGrimElementalStateComponent::HasActivePropagation() const
{
	return States.Items.ContainsByPredicate([](const FGrimElementalState& Item) { return Item.Propagation.IsActive(); });
}

void UGrimElementalStateComponent::PropagateToNeighbors(int32& InOutBudget)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !HasAuthorityToChange())
	{
		return;
	}

	const int32 MaxNeighbors = GetDefault<UGrimInteractionSettings>()->MaxNeighborsPerQuery;
	const FVector Origin = Owner->GetActorLocation();

	// 이웃에 자극을 주는 동안 이 배열이 바뀌지 않도록 복사해 둔다.
	TArray<FGrimElementalState, TInlineAllocator<2>> Sources;
	for (const FGrimElementalState& Item : States.Items)
	{
		if (Item.Propagation.IsActive())
		{
			Sources.Add(Item);
		}
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrimElementalPropagation), false, Owner);
	const FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::InitType::AllObjects);

	for (const FGrimElementalState& Source : Sources)
	{
		if (InOutBudget <= 0)
		{
			return;
		}
		--InOutBudget;

		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams,
			FCollisionShape::MakeSphere(Source.Propagation.Radius), QueryParams);

		FGrimElementalStimulus Stimulus;
		Stimulus.Element = Source.Propagation.Element;
		Stimulus.Intensity = Source.Intensity * Source.Propagation.IntensityScale;
		Stimulus.Source = EGrimStimulusSource::Propagation;

		TSet<AActor*, DefaultKeyFuncs<AActor*>, TInlineSetAllocator<16>> Visited;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Neighbor = Overlap.GetActor();
			if (!Neighbor || Neighbor == Owner || Visited.Contains(Neighbor))
			{
				continue;
			}
			Visited.Add(Neighbor);

			if (Source.Propagation.TargetMaterial.IsValid())
			{
				const UGrimElementalStateComponent* NeighborState = Neighbor->FindComponentByClass<UGrimElementalStateComponent>();
				if (!NeighborState || !NeighborState->Material.HasTag(Source.Propagation.TargetMaterial))
				{
					continue;
				}
			}

			UGrimElementalStatics::ApplyStimulusToActor(Neighbor, Stimulus, Owner);
			if (Visited.Num() >= MaxNeighbors)
			{
				break;
			}
		}
	}
}

void UGrimElementalStateComponent::HandlePreStateChange()
{
	// 사물은 DORM_DormantAll 로 잠들어 있다 → 바뀌기 "전"에 깨운다.
	// 깨울 때 엔진이 지금 값을 클라이언트가 가진 기준으로 삼으므로(UNetConnection::FlushDormancyForObject),
	// 바꾼 "뒤"에 깨우면 그 변화는 기준에 묻혀 보내지지 않는다 — 원격 클라이언트에서 불이 안 보이던 원인.
	AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		Owner->FlushNetDormancy();
	}
}

void UGrimElementalStateComponent::HandleReplicatedStateChange(const FGameplayTag& StateTag, float OldIntensity, float NewIntensity)
{
	OnStateChanged.Broadcast(this, StateTag, OldIntensity, NewIntensity);
	OnStateChangedNative.Broadcast(this, StateTag, OldIntensity, NewIntensity);
}

// ───────────────────────── 세이브 (Elemental.<액터> v1) ─────────────────────────

namespace GrimElementalSaveLocal // 유니티 빌드 이름 충돌 방지
{
	FGameplayTag TagFromString(const FString& Name)
	{
		return Name.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*Name), false);
	}

	struct FSavedState
	{
		FGameplayTag StateTag;
		float Intensity = 0.f;
		FGrimReactionPropagation Propagation;
	};
}

FName UGrimElementalStateComponent::GetSaveId() const
{
	const AActor* Owner = GetOwner();
	return FName(*FString::Printf(TEXT("Elemental.%s"), Owner ? *Owner->GetName() : TEXT("None")));
}

void UGrimElementalStateComponent::WriteSaveData(FArchive& Ar)
{
	TArray<const FGrimElementalState*> Persistent;
	for (const FGrimElementalState& Item : States.Items)
	{
		if (!Item.IsTimed() && Item.Intensity > 0.f && Item.StateTag.IsValid())
		{
			Persistent.Add(&Item);
		}
	}

	int32 Count = Persistent.Num();
	Ar << Count;
	for (const FGrimElementalState* Item : Persistent)
	{
		FString Tag = Item->StateTag.ToString();
		float Intensity = Item->Intensity;
		FString Element = Item->Propagation.Element.IsValid() ? Item->Propagation.Element.ToString() : FString();
		FString TargetMaterial = Item->Propagation.TargetMaterial.IsValid() ? Item->Propagation.TargetMaterial.ToString() : FString();
		float Radius = Item->Propagation.Radius;
		float IntensityScale = Item->Propagation.IntensityScale;
		Ar << Tag << Intensity << Element << TargetMaterial << Radius << IntensityScale;
	}
}

void UGrimElementalStateComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	using namespace GrimElementalSaveLocal;
	int32 Count = 0;
	Ar << Count;
	if (Count < 0 || Count > 256)
	{
		Ar.SetError();
		return;
	}
	TArray<FSavedState> Saved;
	for (int32 i = 0; i < Count && !Ar.IsError(); ++i)
	{
		FString Tag, Element, TargetMaterial;
		float Intensity = 0.f, Radius = 0.f, IntensityScale = 1.f;
		Ar << Tag << Intensity << Element << TargetMaterial << Radius << IntensityScale;
		FSavedState& State = Saved.AddDefaulted_GetRef();
		State.StateTag = TagFromString(Tag);
		State.Intensity = Intensity;
		State.Propagation.Element = TagFromString(Element);
		State.Propagation.TargetMaterial = TagFromString(TargetMaterial);
		State.Propagation.Radius = Radius;
		State.Propagation.IntensityScale = IntensityScale;
	}
	if (!HasAuthorityToChange() || Ar.IsError())
	{
		return;
	}

	// 바꾸기 "전"에 휴면을 깨우는 것은 SetIntensity · Remove 안의 NotifyPreChange 가 한다(HandlePreStateChange 주석).
	TArray<FGrimElementalStateChange> Changes;
	TArray<FGameplayTag> ToRemove;
	for (const FGrimElementalState& Item : States.Items)
	{
		const bool bInSave = Saved.ContainsByPredicate([&Item](const FSavedState& State) { return State.StateTag == Item.StateTag; });
		if (!Item.IsTimed() && !bInSave)
		{
			ToRemove.Add(Item.StateTag);
		}
	}
	for (const FGameplayTag& Tag : ToRemove)
	{
		States.Remove(Tag, Changes);
	}
	for (const FSavedState& State : Saved)
	{
		if (!State.StateTag.IsValid() || State.Intensity <= 0.f)
		{
			continue;
		}
		States.SetIntensity(State.StateTag, State.Intensity, Changes);
		if (FGrimElementalState* Item = States.Find(State.StateTag))
		{
			if (Item->IsTimed())
			{
				States.NotifyPreChange();
				Item->RemainingTime = -1.f;
				States.MarkItemDirty(*Item);
			}
			Item->Propagation = State.Propagation;
		}
	}
	// 바뀐 것이 없어도 타이머 · 전파 등록은 다시 맞춘다.
	CommitChanges(Changes);
}
