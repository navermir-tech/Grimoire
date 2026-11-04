#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GrimoireGameMode.generated.h"

/**
 * 그리모리 기본 게임 모드.
 * 블루프린트로 파생(BP_GrimoireGameMode)해서 폰 · 플레이어 컨트롤러 · HUD 클래스를 지정한다.
 */
UCLASS()
class GRIMOIRE_API AGrimoireGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGrimoireGameMode();
};
