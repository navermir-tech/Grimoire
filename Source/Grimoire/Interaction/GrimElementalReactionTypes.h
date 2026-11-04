#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GrimElementalReactionTypes.generated.h"

class UGrimElementalStateComponent;
struct FGrimElementalStateArray;

/** 규칙이 언제 검사되는가. */
UENUM(BlueprintType)
enum class EGrimReactionTrigger : uint8
{
	/** 속성 자극이 들어왔을 때 */
	Stimulus,
	/** 시간이 다 된 상태가 사라질 때(RequiredState = 사라진 상태) */
	Expire
};

/** 상태가 살아 있는 동안 주변에 주는 자극. Radius 0 = 전파 없음. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimReactionPropagation
{
	GENERATED_BODY()

	/** 이웃에 줄 자극 속성 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** 이 재질을 가진 이웃만 받는다(없음 = 모두) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (Categories = "Material"))
	FGameplayTag TargetMaterial;

	/** 반경(cm). 설정의 MaxPropagationRadius 로 잘린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0"))
	float Radius = 0.f;

	/** 전파 자극 세기 = 상태 세기 × 이 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0"))
	float IntensityScale = 1.f;

	bool IsActive() const { return Radius > 0.f && Element.IsValid(); }
};

/**
 * 반응 규칙 한 행 (DT_ElementalReactions). 원본은 Content/Grimoire/Data/ElementalReactions.csv.
 * 규칙은 위에서부터 검사하고 처음 맞는 행 하나만 적용한다 — 강한 조건(돌풍)을 약한 조건(바람) 위에 둔다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementalReactionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 사람용 설명(규칙에 영향 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString Note;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	EGrimReactionTrigger Trigger = EGrimReactionTrigger::Stimulus;

	/** 이 상태가 있어야 한다(없음 = 상관없음). Expire 행이면 사라진 상태 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "State"))
	FGameplayTag RequiredState;

	/** 이 상태 중 하나라도 있으면 맞지 않는다(예: 젖으면 불이 안 붙는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "State"))
	TArray<FGameplayTag> BlockedByStates;

	/** 이 재질이어야 한다(없음 = 상관없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "Material"))
	FGameplayTag RequiredMaterial;

	/** 들어온 자극 속성(Stimulus 행만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** 자극 세기 하한(Stimulus 행만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (ClampMin = "0"))
	float MinIntensity = 0.f;

	/** 먼저 지울 상태들 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (Categories = "State"))
	TArray<FGameplayTag> ClearStates;

	/** 세기를 바꿀 상태(없음 = 지우기만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (Categories = "State"))
	FGameplayTag ResultState;

	/** 결과 상태 세기에 더할 값. 0 이하가 되면 상태가 사라진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	float IntensityDelta = 0.f;

	/** 결과 상태 세기 상한(0 = 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = "0"))
	float MaxIntensity = 0.f;

	/** 결과 상태의 남은 시간(초)을 이 값으로 둔다. 0 = 새 상태는 무한, 있던 상태는 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = "0"))
	float Duration = 0.f;

	/** 결과 상태가 살아 있는 동안 주변에 주는 자극 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	FGrimReactionPropagation Propagate;

	/** 연출용 GameplayCue 태그(M1 은 쓰지 않음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (Categories = "GameplayCue"))
	FGameplayTag Cue;
};

/** 상태 하나. 태그 · 세기 · 남은 시간이 복제된다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementalState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Elemental")
	FGameplayTag StateTag;

	UPROPERTY(BlueprintReadOnly, Category = "Elemental")
	float Intensity = 0.f;

	/** 남은 시간(초). 음수 = 무한. 서버에서는 계속 줄지만 복제는 세기가 바뀔 때의 값만 간다(연출 참고용) */
	UPROPERTY(BlueprintReadOnly, Category = "Elemental")
	float RemainingTime = -1.f;

	/** 서버 전용: 이 상태가 주변에 주는 자극 */
	UPROPERTY(NotReplicated)
	FGrimReactionPropagation Propagation;

	/** 클라이언트 전용: 마지막으로 알린 세기(변화 델리게이트의 이전 값) */
	UPROPERTY(NotReplicated)
	float LastNotifiedIntensity = 0.f;

	bool IsTimed() const { return RemainingTime >= 0.f; }

	void PreReplicatedRemove(const FGrimElementalStateArray& InArray);
	void PostReplicatedAdd(const FGrimElementalStateArray& InArray);
	void PostReplicatedChange(const FGrimElementalStateArray& InArray);
};

/** 상태 변화 한 건(서버 계산 결과 · 클라이언트 복제 결과 공용). */
struct FGrimElementalStateChange
{
	FGameplayTag StateTag;
	float OldIntensity = 0.f;
	float NewIntensity = 0.f;
};

/** 상태 배열. 아무것도 없으면 비어 있다(= 컴포넌트가 아무 일도 하지 않는다). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementalStateArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGrimElementalState> Items;

	/** 복제 콜백을 받을 컴포넌트. 테스트에서는 없어도 된다 */
	UPROPERTY(NotReplicated)
	TObjectPtr<UGrimElementalStateComponent> Owner;

	const FGrimElementalState* Find(const FGameplayTag& StateTag) const;
	FGrimElementalState* Find(const FGameplayTag& StateTag);
	float GetIntensity(const FGameplayTag& StateTag) const;
	/** 세기가 0 인 항목(클라이언트에서 지워지는 중)은 없는 것으로 본다 */
	bool Has(const FGameplayTag& StateTag) const { const FGrimElementalState* Item = Find(StateTag); return Item && Item->Intensity > 0.f; }

	/** 세기를 바꾼다. 0 이하면 지운다. 바뀌면 OutChanges 에 한 건 더한다 */
	void SetIntensity(const FGameplayTag& StateTag, float NewIntensity, TArray<FGrimElementalStateChange>& OutChanges);
	/** 지운다. 있었으면 OutChanges 에 한 건 더한다 */
	void Remove(const FGameplayTag& StateTag, TArray<FGrimElementalStateChange>& OutChanges);

	/** 서버: 복제되는 값을 바꾸기 "직전"에 부른다 → Owner 가 휴면을 깨운다(UGrimElementalStateComponent::HandlePreStateChange) */
	void NotifyPreChange();

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGrimElementalState, FGrimElementalStateArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGrimElementalStateArray> : public TStructOpsTypeTraitsBase2<FGrimElementalStateArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
