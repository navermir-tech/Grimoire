#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrimSurvivalComponent.generated.h"

/**
 * 생존 관리 (R03) — 초기 어트리뷰트, 주기 효과(배고픔 · 체온), 임계값 상태 태그.
 * AGrimPlayerState 의 기본 하위 객체. 내용은 Survival 시스템이 채운다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSurvivalComponent();
};
