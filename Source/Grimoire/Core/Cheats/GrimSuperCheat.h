#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimSuperCheat.generated.h"

class UAbilitySystemComponent;

/**
 * GE_Cheat_Super — 슈퍼치트(무한 지속). 현재 값을 Override 로 고정한다:
 * Health ← MaxHealth, Mana ← MaxMana(대상 어트리뷰트, 스냅샷 없음 — 최대치가 바뀌면 따라간다),
 * Hunger ← MaxHunger, BodyTemperature ← NormalBodyTemperature(설정 값을 SetByCaller 로).
 * 비용 · 피해 · 생존 틱은 베이스 값을 바꾸지만 현재 값은 계속 최대다. 태그 Cheat.Super 를 준다.
 * 에셋 없이 C++ CDO 로 쓴다 — 켜고 끄기는 GrimSuperCheat::SetEnabled.
 */
UCLASS()
class GRIMOIRE_API UGrimSuperCheatEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimSuperCheatEffect();

	static const FName HungerMagnitudeName;
	static const FName BodyTemperatureMagnitudeName;
};

namespace GrimSuperCheat
{
	/**
	 * 서버에서 켜고 끈다. 켜면 생존 상태 태그(배고픔 · 저체온 …)를 바로 떼고,
	 * 끄면 베이스 값을 최대(체온은 정상)로 채운 뒤 GE 를 떼어 원래 흐름(감소 · 회복)으로 돌아간다.
	 */
	GRIMOIRE_API void SetEnabled(UAbilitySystemComponent* ASC, bool bEnable);

	GRIMOIRE_API bool IsEnabled(const UAbilitySystemComponent* ASC);
}
