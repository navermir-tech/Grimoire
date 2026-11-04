#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GrimAbilitySystemComponent.generated.h"

/**
 * 그리모리 공용 ASC. 플레이어는 AGrimPlayerState 가, 생물은 생물 자신이 소유한다.
 * 입력은 "입력 태그"로 받는다: 어빌리티를 줄 때 Spec.GetDynamicSpecSourceTags() 에 Input.* 태그를 넣으면
 * 그 태그가 눌렸을 때 발동된다. 캐릭터는 어떤 어빌리티가 있는지 몰라도 된다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGrimAbilitySystemComponent();

	/** 입력 태그가 눌림 — 로컬 컨트롤러에서 호출한다. 어빌리티 발동은 GAS 예측을 따른다. */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** 입력 태그가 떼어짐. */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
};
