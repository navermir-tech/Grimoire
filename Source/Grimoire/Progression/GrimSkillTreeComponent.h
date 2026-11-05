#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Core/GrimSaveTypes.h"
#include "Progression/GrimProgressionTypes.h"
#include "GrimSkillTreeComponent.generated.h"

class AGrimPlayerState;
class APawn;
class UAbilitySystemComponent;
class UGrimLevelComponent;
class UGrimSkillTreeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnSkillTreeChanged, UGrimSkillTreeComponent*, SkillTree);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnSkillUnlockResult, UGrimSkillTreeComponent*, SkillTree, FName, NodeId, EGrimSkillUnlockResult, Result);

/**
 * 정화 의식의 비용 확인 — 비울 트리 · 돌려받을 포인트를 받고, 비용(재화)을 냈으면 true.
 * 재화가 아직 없어 비워 두면 공짜로 비운다(TODO: 인벤토리 · 재화가 생기면 정화 의식 쪽이 채운다).
 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FGrimTreeResetCostCheck, FName /*TreeId*/, int32 /*RefundPoints*/);

/**
 * 속성 성장 트리 (R27) — AGrimPlayerState 의 하위 객체. 노드 · 비용 · 선행 · 효과는 트리 표(UGrimSkillTreeSettings::GetDefinition).
 *  - 포인트: 레벨이 준 합(UGrimLevelComponent::GetTotalSkillPoints) − 찍은 노드 비용 합. 따로 저장하지 않는다.
 *  - 효과: 찍은 노드의 효과 합을 무한 GE(UGrimGE_ProgressionBonus) 하나로 건다 — 어트리뷰트가 있는 키는 수정자로,
 *    없는 키(위력 · 상호작용 세기 · 특성)는 태그 Progression.Node.<키> 와 GetEffectValue 합계로만(TODO: 마법 · 상호작용 쪽이 읽는다).
 *  - 다시 찍기(Q31): ResetTree — 마법탑 정화 의식이 부른다, 한 번에 트리 하나.
 * 세이브: Scope = Player, Id = "Progression.SkillTree" — 찍은 노드 id.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSkillTreeComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimSkillTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static UGrimSkillTreeComponent* FindFor(const AActor* Owner);

	const TArray<FName>& GetUnlockedNodes() const { return UnlockedNodes; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|SkillTree")
	bool IsUnlocked(FName NodeId) const { return UnlockedNodes.Contains(NodeId); }

	/** 쓴 포인트(Tree 를 주면 그 트리만) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|SkillTree")
	int32 GetSpentPoints(FName Tree = NAME_None) const;

	/** 남은 포인트 = 받은 포인트 − 쓴 포인트(0 밑으로 안 감) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|SkillTree")
	int32 GetAvailablePoints() const;

	/** 지금 서클(Circle 어트리뷰트) — 큰 노드 조건 */
	int32 GetCircle() const;

	/** 지금 찍을 수 있는가(서버 · 클라이언트 — UI) */
	EGrimSkillUnlockResult CanUnlock(FName NodeId) const;

	/** 서버: 노드를 찍는다. 결과는 OnUnlockResult(서버 · 소유 클라이언트). */
	EGrimSkillUnlockResult TryUnlock(FName NodeId);

	/** 클라이언트 → 서버: 찍기(그리모어 「성장 트리」 탭의 찍기 버튼) */
	UFUNCTION(Server, Reliable)
	void Server_TryUnlock(FName NodeId);

	/** 어디서든: 서버면 바로, 아니면 RPC */
	void RequestUnlock(FName NodeId);

	/**
	 * 서버: 트리 하나를 비우고 포인트를 돌려준다(Q31 — 마법탑 「정화 의식」이 부른다).
	 * CostCheck 가 묶여 있으면 (트리, 돌려받을 포인트)로 먼저 묻고 false 면 그만둔다. 비울 노드가 없으면 false.
	 */
	bool ResetTree(FName TreeId, const FGrimTreeResetCostCheck& CostCheck = FGrimTreeResetCostCheck());

	/** 개발용: 클라이언트 콘솔(Grim.Tree.Reset)의 서버 경로 — 비용 없이. 출시 빌드에서는 아무것도 안 한다. */
	UFUNCTION(Server, Reliable)
	void Server_DebugResetTree(FName TreeId);

	/** 효과 합(서버 · 클라이언트). 더하기 키는 Add, 곱하기 키는 Mul. 없는 키 → Add 0 · Mul 1. */
	FGrimSkillEffectTotal GetEffectTotal(FName Key) const;

	/** 더하기 효과 합(예: GetEffectValue("Fire.IgniteStrength") = 1) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|SkillTree")
	float GetEffectValue(FName Key) const { return GetEffectTotal(Key).Add; }

	const TMap<FName, FGrimSkillEffectTotal>& GetEffectTotals() const { return EffectTotals; }

	/** 찍은 노드가 바뀌었다(서버 · 클라이언트) */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|SkillTree")
	FGrimOnSkillTreeChanged OnTreeChanged;

	/** 찍기 결과(서버 · 소유 클라이언트) */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|SkillTree")
	FGrimOnSkillUnlockResult OnUnlockResult;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Progression.SkillTree"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);

	UGrimLevelComponent* GetLevelComponent() const;
	UAbilitySystemComponent* GetASC() const;

	/** 서버 · 클라이언트: 찍은 노드가 바뀐 뒤 — 효과 합을 다시 셈하고(서버는 GE 도) 알린다 */
	void HandleUnlockedChanged();

	/** 서버: 효과 합을 GE 로 */
	void ApplyBonus();

	void NotifyUnlockResult(FName NodeId, EGrimSkillUnlockResult Result);

	UFUNCTION(Client, Reliable)
	void Client_UnlockResult(FName NodeId, EGrimSkillUnlockResult Result);

	UFUNCTION()
	void OnRep_UnlockedNodes();

	/** 찍은 순서대로 */
	UPROPERTY(ReplicatedUsing = OnRep_UnlockedNodes)
	TArray<FName> UnlockedNodes;

	/** 서버 · 클라이언트: UnlockedNodes 의 효과 합(복제 안 함, 다시 셈) */
	TMap<FName, FGrimSkillEffectTotal> EffectTotals;

	/** 서버 전용 */
	FActiveGameplayEffectHandle BonusHandle;
	bool bBonusDirty = true;

	FDelegateHandle ActorInfoReadyHandle;
};
