#include "Core/Cheats/GrimCheatManager.h"
#include "Character/GrimPlayerController.h"

void UGrimCheatManager::GrimSuperCheat(int32 bEnable)
{
	// 리슨 서버 호스트도 같은 경로 — 서버에서 부른 Server RPC 는 그 자리에서 실행된다.
	if (AGrimPlayerController* PlayerController = Cast<AGrimPlayerController>(GetOuterAPlayerController()))
	{
		PlayerController->Server_SetSuperCheat(bEnable != 0);
	}
}
