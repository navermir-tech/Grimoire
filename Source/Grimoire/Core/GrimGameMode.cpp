#include "Core/GrimGameMode.h"
#include "Core/GrimGameState.h"
#include "Core/GrimPlayerState.h"
#include "Character/GrimPlayerCharacter.h"
#include "Character/GrimPlayerController.h"

AGrimGameMode::AGrimGameMode()
{
	GameStateClass = AGrimGameState::StaticClass();
	PlayerStateClass = AGrimPlayerState::StaticClass();
	DefaultPawnClass = AGrimPlayerCharacter::StaticClass();
	PlayerControllerClass = AGrimPlayerController::StaticClass();
}
