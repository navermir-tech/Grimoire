#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "GrimElementalTypes.generated.h"

/** 자극이 어디서 왔는가. 반응 규칙이 출처에 따라 달라질 수 있다(예: 전파된 불은 약하다). */
UENUM(BlueprintType)
enum class EGrimStimulusSource : uint8
{
	Spell,
	Weather,
	Propagation,
	Creature,
	Environment
};

/**
 * 속성 자극 (R16). 마법 · 날씨 · 생물 · 전파가 모두 이 하나의 형태로 세계에 닿는다.
 * 예: 불씨 마법 = { Element.Fire, 1.0, Spell }, 비 = { Element.Water, 0.5, Weather }
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementalStimulus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental", meta = (Categories = "Element"))
	FGameplayTag Element;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental", meta = (ClampMin = "0"))
	float Intensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental")
	EGrimStimulusSource Source = EGrimStimulusSource::Spell;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimElementalReceiver : public UInterface
{
	GENERATED_BODY()
};

/**
 * 속성 자극을 받는 쪽. 액터 자신 또는 그 액터의 컴포넌트 하나가 구현한다
 * (보통 Interaction 시스템의 UGrimElementalStateComponent).
 * 마법 · 날씨 쪽은 구현체를 몰라도 UGrimElementalStatics 로 자극을 보낸다.
 */
class GRIMOIRE_API IGrimElementalReceiver
{
	GENERATED_BODY()

public:
	/** 서버에서만 호출된다. 반응 규칙을 적용해 상태를 바꾼다. */
	virtual void ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* Instigator) = 0;

	/** 현재 상태의 세기. 상태가 없으면 0. (예: State.Burning → 불꽃 크기) */
	virtual float GetElementalStateIntensity(const FGameplayTag& StateTag) const = 0;
};

UCLASS()
class GRIMOIRE_API UGrimElementalStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 액터 또는 그 컴포넌트 중 IGrimElementalReceiver 구현체를 찾는다. 없으면 nullptr. */
	static IGrimElementalReceiver* FindReceiver(const AActor* Actor);

	/** 대상에 속성 자극을 준다(서버 전용). 받는 쪽이 없으면 false. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Elemental")
	static bool ApplyStimulusToActor(AActor* Target, const FGrimElementalStimulus& Stimulus, AActor* Instigator);

	/** 대상의 속성 상태 세기. 받는 쪽이 없거나 상태가 없으면 0. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Elemental")
	static float GetStateIntensity(const AActor* Target, FGameplayTag StateTag);
};
