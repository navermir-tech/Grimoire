#include "Core/GrimGameMode.h"
#include "Core/GrimGameState.h"
#include "Core/GrimPlayerState.h"

AGrimGameMode::AGrimGameMode()
{
	GameStateClass = AGrimGameState::StaticClass();
	PlayerStateClass = AGrimPlayerState::StaticClass();
}
