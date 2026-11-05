#include "Progression/GrimSkillTreeComponent.h"
#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionEffects.h"
#include "Progression/GrimProgressionSettings.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UGrimSkillTreeComponent::UGrimSkillTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimSkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 모두에게 — 다른 플레이어의 갈래(역할)를 보여 줄 수 있게. 노드 id 몇십 개라 작다.
	DOREPLIFETIME(UGrimSkillTreeComponent, UnlockedNodes);
}

UGrimSkillTreeComponent* UGrimSkillTreeComponent::FindFor(const AActor* Owner)
{
	return Owner ? Owner->FindComponentByClass<UGrimSkillTreeComponent>() : nullptr;
}

UGrimLevelComponent* UGrimSkillTreeComponent::GetLevelComponent() const
{
	return UGrimLevelComponent::FindFor(GetOwner());
}

UAbilitySystemComponent* UGrimSkillTreeComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

void UGrimSkillTreeComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		ActorInfoReadyHandle = PlayerState->OnAbilityActorInfoReady.AddUObject(this, &ThisClass::HandleAbilityActorInfoReady);
		if (const UAbilitySystemComponent* ASC = GetASC(); ASC && ASC->AbilityActorInfo.IsValid())
		{
			if (APawn* Pawn = Cast<APawn>(ASC->GetAvatarActor()))
			{
				HandleAbilityActorInfoReady(PlayerState, Pawn);
			}
		}
	}
}

void UGrimSkillTreeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		PlayerState->OnAbilityActorInfoReady.Remove(ActorInfoReadyHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimSkillTreeComponent::HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn)
{
	if (GetOwner()->HasAuthority() && bBonusDirty)
	{
		ApplyBonus();
	}
}

// ───────────────────────── 읽기 ─────────────────────────

int32 UGrimSkillTreeComponent::GetSpentPoints(FName Tree) const
{
	return FGrimSkillTreeMath::GetSpentPoints(UGrimSkillTreeSettings::GetDefinition(), UnlockedNodes, Tree);
}

int32 UGrimSkillTreeComponent::GetAvailablePoints() const
{
	const UGrimLevelComponent* Level = GetLevelComponent();
	const int32 Total = Level ? Level->GetTotalSkillPoints() : 0;
	return FMath::Max(Total - GetSpentPoints(), 0);
}

int32 UGrimSkillTreeComponent::GetCircle() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC ? FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute())) : 1;
}

EGrimSkillUnlockResult UGrimSkillTreeComponent::CanUnlock(FName NodeId) const
{
	return FGrimSkillTreeMath::CanUnlock(UGrimSkillTreeSettings::GetDefinition(), UnlockedNodes, NodeId, GetAvailablePoints(), GetCircle());
}

FGrimSkillEffectTotal UGrimSkillTreeComponent::GetEffectTotal(FName Key) const
{
	const FGrimSkillEffectTotal* Total = EffectTotals.Find(Key);
	return Total ? *Total : FGrimSkillEffectTotal();
}

// ───────────────────────── 찍기 · 비우기 ─────────────────────────

EGrimSkillUnlockResult UGrimSkillTreeComponent::TryUnlock(FName NodeId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return EGrimSkillUnlockResult::Invalid;
	}
	const EGrimSkillUnlockResult Result = CanUnlock(NodeId);
	if (Result == EGrimSkillUnlockResult::Unlocked)
	{
		UnlockedNodes.Add(NodeId);
		HandleUnlockedChanged();
	}
	UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 성장 트리 %s 찍기 → %s (남은 포인트 %d)"), *GetNameSafe(GetOwner()), *NodeId.ToString(),
		*StaticEnum<EGrimSkillUnlockResult>()->GetNameStringByValue(static_cast<int64>(Result)), GetAvailablePoints());
	NotifyUnlockResult(NodeId, Result);
	return Result;
}

void UGrimSkillTreeComponent::Server_TryUnlock_Implementation(FName NodeId)
{
	TryUnlock(NodeId);
}

void UGrimSkillTreeComponent::RequestUnlock(FName NodeId)
{
	if (GetOwner()->HasAuthority())
	{
		TryUnlock(NodeId);
	}
	else
	{
		Server_TryUnlock(NodeId);
	}
}

bool UGrimSkillTreeComponent::ResetTree(FName TreeId, const FGrimTreeResetCostCheck& CostCheck)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	const int32 Refund = FGrimSkillTreeMath::GetSpentPoints(Definition, UnlockedNodes, TreeId);
	TArray<FName> Remaining = FGrimSkillTreeMath::RemoveTree(Definition, UnlockedNodes, TreeId);
	if (Remaining.Num() == UnlockedNodes.Num())
	{
		UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 정화 의식: 트리 %s 에 찍은 노드가 없다"), *GetNameSafe(GetOwner()), *TreeId.ToString());
		return false;
	}
	if (CostCheck.IsBound() && !CostCheck.Execute(TreeId, Refund))
	{
		UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 정화 의식: 트리 %s 비용을 내지 못했다"), *GetNameSafe(GetOwner()), *TreeId.ToString());
		return false;
	}
	UnlockedNodes = MoveTemp(Remaining);
	HandleUnlockedChanged();
	UE_LOG(LogGrimProgression, Display, TEXT("[Server] %s 정화 의식: 트리 %s 를 비웠다(포인트 %d 돌려받음, 남은 포인트 %d)"), *GetNameSafe(GetOwner()), *TreeId.ToString(),
		Refund, GetAvailablePoints());
	return true;
}

void UGrimSkillTreeComponent::Server_DebugResetTree_Implementation(FName TreeId)
{
#if !UE_BUILD_SHIPPING
	ResetTree(TreeId);
#endif
}

void UGrimSkillTreeComponent::HandleUnlockedChanged()
{
	FGrimSkillTreeMath::SumEffects(UGrimSkillTreeSettings::GetDefinition(), UnlockedNodes, EffectTotals);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		bBonusDirty = true;
		ApplyBonus();
	}
	OnTreeChanged.Broadcast(this);
}

void UGrimSkillTreeComponent::ApplyBonus()
{
	TArray<FName> UnknownKeys;
	const FGrimProgressionBonus Bonus = FGrimProgressionEffects::MakeBonus(EffectTotals, &UnknownKeys);
	for (const FName Key : UnknownKeys)
	{
		UE_LOG(LogGrimProgression, Warning, TEXT("성장 트리 효과 키 %s: 어트리뷰트도 등록된 태그(Progression.Node.%s)도 아니다 — 합계에만 남는다"), *Key.ToString(), *Key.ToString());
	}
	if (FGrimProgressionEffects::ApplyBonus(GetASC(), Bonus, BonusHandle))
	{
		bBonusDirty = false;
	}
}

void UGrimSkillTreeComponent::NotifyUnlockResult(FName NodeId, EGrimSkillUnlockResult Result)
{
	OnUnlockResult.Broadcast(this, NodeId, Result);
	// 리슨 호스트는 위에서 이미 알렸다 — 원격 소유 클라이언트에게만 보낸다.
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PC && !PC->IsLocalController())
	{
		Client_UnlockResult(NodeId, Result);
	}
}

void UGrimSkillTreeComponent::Client_UnlockResult_Implementation(FName NodeId, EGrimSkillUnlockResult Result)
{
	OnUnlockResult.Broadcast(this, NodeId, Result);
}

void UGrimSkillTreeComponent::OnRep_UnlockedNodes()
{
	HandleUnlockedChanged();
}

// ───────────────────────── 세이브 ─────────────────────────

void UGrimSkillTreeComponent::WriteSaveData(FArchive& Ar)
{
	TArray<FString> Saved;
	for (const FName Id : UnlockedNodes)
	{
		Saved.Add(Id.ToString());
	}
	Ar << Saved;
}

void UGrimSkillTreeComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FString> Saved;
	Ar << Saved;
	if (Ar.IsError())
	{
		UE_LOG(LogGrimProgression, Error, TEXT("성장 트리 세이브(v%d)를 읽지 못했다"), Version);
		return;
	}
	// 표에서 빠진 노드는 버린다(포인트는 자연히 돌아온다). 선행 · 포인트는 다시 따지지 않는다 — 레벨이 어느 쪽이 먼저 읽힐지 모른다.
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	UnlockedNodes.Reset();
	for (const FString& Id : Saved)
	{
		const FName Name(*Id);
		if (Definition.Find(Name))
		{
			UnlockedNodes.AddUnique(Name);
		}
		else
		{
			UE_LOG(LogGrimProgression, Warning, TEXT("성장 트리 세이브: 표에 없는 노드 %s 를 버린다"), *Id);
		}
	}
	HandleUnlockedChanged();
}
