#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GrimMenuGameMode.generated.h"

class UGrimMainMenuWidget;

/**
 * 메인 메뉴 맵(L_MainMenu — 월드 설정의 GameMode Override)의 게임 모드. 폰을 만들지 않고, 이 맵에서는 세이브를 쓰지 않는다
 * (세이브 시스템은 서버 월드면 기본 슬롯에 저장한다 — 메뉴의 시간 · 날씨가 플레이어 세계를 덮지 않게, 세이브 시스템이 슬롯을 정하는 동안만 설정을 끈다).
 */
UCLASS()
class GRIMOIRE_API AGrimMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGrimMenuGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 메뉴에는 폰이 없다 */
	virtual void RestartPlayer(AController* NewPlayer) override {}

private:
	void RestoreSaveSetting();

	bool bSaveSettingOverridden = false;
	bool bSavedEnabledValue = true;
};

/** 메인 메뉴 컨트롤러 — 로컬이면 메뉴 위젯을 띄우고 마우스 · UI 입력으로. HUD(UI 갈래)는 숨긴다. */
UCLASS()
class GRIMOIRE_API AGrimMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UGrimMainMenuWidget* GetMenuWidget() const { return Menu; }

private:
	UPROPERTY()
	TObjectPtr<UGrimMainMenuWidget> Menu;
};
