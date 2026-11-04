#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GrimGameState.generated.h"

/** 모두에게 복제되는 세계 상태. 시간 · 날씨 같은 세계 단위 상태는 각 시스템이 자기 복제 액터나 서브시스템으로 가진다. */
UCLASS()
class GRIMOIRE_API AGrimGameState : public AGameStateBase
{
	GENERATED_BODY()
};
