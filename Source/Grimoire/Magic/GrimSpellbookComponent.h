#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrimSpellbookComponent.generated.h"

/**
 * 플레이어의 마법서(그리모어) — 배운 마법 · 선택 슬롯 · 완드 장착 (R01 · R05).
 * AGrimPlayerState 의 기본 하위 객체. 내용은 Magic 시스템이 채운다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSpellbookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSpellbookComponent();
};
