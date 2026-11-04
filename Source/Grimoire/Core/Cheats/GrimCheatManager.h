#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GrimCheatManager.generated.h"

/**
 * 그리모리 개발용 치트(콘솔 명령). 출시 빌드에서는 CheatManager 가 만들어지지 않는다(UE_WITH_CHEAT_MANAGER 0).
 * 콘솔은 로컬에서 돌므로, 상태를 바꾸는 치트는 AGrimPlayerController 의 Server RPC 로 서버에서 적용한다.
 */
UCLASS()
class GRIMOIRE_API UGrimCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** GrimSuperCheat 1 = 켬, 0 = 끔. 친 플레이어에게만 HP · 마나 · 배고픔 최대, 체온 정상 고정. */
	UFUNCTION(Exec)
	void GrimSuperCheat(int32 bEnable);
};
