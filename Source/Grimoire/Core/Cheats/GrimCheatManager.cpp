#include "Core/Cheats/GrimCheatManager.h"
#include "Character/GrimPlayerController.h"
#include "Appearance/GrimCharacterCreatorWidget.h"
#include "Appearance/GrimPlayerAppearanceComponent.h"

void UGrimCheatManager::GrimSuperCheat(int32 bEnable)
{
	// 리슨 서버 호스트도 같은 경로 — 서버에서 부른 Server RPC 는 그 자리에서 실행된다.
	if (AGrimPlayerController* PlayerController = Cast<AGrimPlayerController>(GetOuterAPlayerController()))
	{
		PlayerController->Server_SetSuperCheat(bEnable != 0);
	}
}

void UGrimCheatManager::GrimCreator()
{
	UGrimCharacterCreatorWidget::Open(GetOuterAPlayerController());
}

void UGrimCheatManager::GrimAppearanceRandom(const FString& Seed)
{
	// 서버 RPC 는 UGrimPlayerAppearanceComponent(PlayerState) 에 있다 — 호스트면 그 자리에서
	if (UGrimPlayerAppearanceComponent* Appearance = UGrimPlayerAppearanceComponent::FindForController(GetOuterAPlayerController()))
	{
		Appearance->RequestRandomize(FCString::Atoi(*Seed));
	}
}
