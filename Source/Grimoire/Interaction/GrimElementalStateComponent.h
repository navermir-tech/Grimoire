#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimElementalTypes.h"
#include "Interaction/GrimElementalReactionTypes.h"
#include "Core/GrimSaveTypes.h"
#include "GrimElementalStateComponent.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGrimElementalStateChangedSignature,
	UGrimElementalStateComponent*, Component, FGameplayTag, StateTag, float, OldIntensity, float, NewIntensity);
DECLARE_MULTICAST_DELEGATE_FourParams(FGrimElementalStateChangedNative,
	UGrimElementalStateComponent* /*Component*/, FGameplayTag /*StateTag*/, float /*OldIntensity*/, float /*NewIntensity*/);

/**
 * 속성 상태 (R16). 사물 · 생물 · 플레이어가 가진다.
 * - 서버: 자극 → 규칙표 → 상태 변화 → 복제(FastArray) · 휴면 깨우기 · 델리게이트.
 * - 클라이언트: 복제 콜백에서 같은 델리게이트가 불린다 → 연출은 OnStateChanged 에 묶는다.
 * - 상태가 없으면 틱도 타이머도 없다. 시간 제한 상태가 있을 때만 타이머, 전파 상태가 있을 때만 전파 서브시스템에 등록.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimElementalStateComponent : public UActorComponent, public IGrimElementalReceiver, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimElementalStateComponent();

	// 세이브 (IGrimSaveParticipant — World 「Elemental.<액터 이름>」 v1): 지속 상태만(남은 시간이 없는 것 — 켜진 양초 · 탄 풀)과 그 전파.
	// 시간이 다 되면 사라지는 상태(젖음 · 타는 풀)는 저장하지 않는다. 세이브 시스템은 레벨에 놓인 액터의 것만 모은다(플레이어 폰 제외).
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 재질(반응 조건). 예: 양초 = Material.Flammable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental", meta = (Categories = "Material"))
	FGameplayTagContainer Material;

	/** BeginPlay 때 서버가 넣는 상태(예: 처음부터 켜진 모닥불 = State.Burning 1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental", meta = (Categories = "State"))
	TMap<FGameplayTag, float> InitialStates;

	/** 비우면 설정의 기본 규칙표를 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental")
	TObjectPtr<UDataTable> ReactionTable;

	/** 상태 세기가 바뀔 때(서버 · 클라이언트 모두). 새 세기 0 = 사라짐 */
	UPROPERTY(BlueprintAssignable, Category = "Elemental")
	FGrimElementalStateChangedSignature OnStateChanged;

	/** OnStateChanged 의 C++ 판(람다 · 약한 바인딩용). 같은 때 불린다 */
	FGrimElementalStateChangedNative OnStateChangedNative;

	// IGrimElementalReceiver
	virtual void ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* Instigator) override;
	virtual float GetElementalStateIntensity(const FGameplayTag& StateTag) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Elemental")
	void ApplyStimulus(const FGrimElementalStimulus& Stimulus, AActor* Instigator) { ApplyElementalStimulus(Stimulus, Instigator); }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Elemental")
	float GetStateIntensity(FGameplayTag StateTag) const { return GetElementalStateIntensity(StateTag); }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Elemental")
	bool HasState(FGameplayTag StateTag) const { return States.Has(StateTag); }

	const FGrimElementalStateArray& GetStates() const { return States; }

	/** 서버: 시간 제한 상태를 DeltaSeconds 만큼 진행한다(타이머가 부른다. 테스트는 직접 부른다) */
	void AdvanceTime(float DeltaSeconds);

	/** 서버: 전파 상태마다 반경 오버랩 → 이웃에 Propagation 자극. 쿼리 한 번에 InOutBudget 을 1 쓴다 */
	void PropagateToNeighbors(int32& InOutBudget);

	bool HasActivePropagation() const;
	bool IsStateTimerActive() const;

	/** 서버: 상태 배열이 바뀌기 직전(FGrimElementalStateArray::NotifyPreChange)에 휴면을 깨운다 */
	void HandlePreStateChange();

	/** 복제 콜백(FGrimElementalState)에서 부른다 */
	void HandleReplicatedStateChange(const FGameplayTag& StateTag, float OldIntensity, float NewIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	FGrimElementalStateArray States;

	FTimerHandle StateTimerHandle;
	bool bRegisteredForPropagation = false;

	bool HasAuthorityToChange() const;
	const UDataTable* GetReactionTable() const;
	void CommitChanges(const TArray<FGrimElementalStateChange>& Changes);
	void UpdateTimerAndPropagation();
	void HandleStateTimer();
};
