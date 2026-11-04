#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GrimGameMode.generated.h"

/**
 * 그리모리 게임 모드(서버 전용). 싱글도 "혼자 있는 리슨 서버"로 같은 경로를 탄다.
 * 기본 폰 · 컨트롤러 클래스는 Character 시스템이 정한다.
 */
UCLASS()
class GRIMOIRE_API AGrimGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGrimGameMode();
};
