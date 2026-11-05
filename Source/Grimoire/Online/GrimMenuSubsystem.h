#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Containers/Ticker.h"
#include "GrimMenuSubsystem.generated.h"

class APlayerController;
class UInputComponent;
class UGrimPauseMenuWidget;

/**
 * 로컬 플레이어마다 하나 — 게임 안 Esc 메뉴(이어하기 · 설정 · 메인 메뉴로 · 나가기)와 플레이어 옵션 적용.
 * 컨트롤러 · 캐릭터 파일을 고치지 않으려고 UI 서브시스템처럼 0.25초마다 컨트롤러를 보고, 바뀌면
 * Esc · 패드 Options(Menu) 키를 묶은 입력 컴포넌트를 올리고, 보관된 조준 · 시점 옵션을 적용한다(FGrimUserOptions).
 * 메인 메뉴 맵의 컨트롤러(AGrimMenuPlayerController)는 건너뛴다 — 거기는 메인 메뉴 위젯이 맡는다.
 * 그리지 못하는 프로세스에서는 위젯 없이 상태만 바뀐다(콘솔 Grim.Online.Pause).
 */
UCLASS()
class GRIMOIRE_API UGrimMenuSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Grimoire|Menu")
	void SetPauseMenuOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Grimoire|Menu")
	void TogglePauseMenu();

	UFUNCTION(BlueprintPure, Category = "Grimoire|Menu")
	bool IsPauseMenuOpen() const { return bPauseOpen; }

	static void ForEach(TFunctionRef<void(UGrimMenuSubsystem&)> Fn);

private:
	bool Tick(float DeltaTime);
	void SetupForController(APlayerController* PC);
	void TearDownController();
	void HandleMenuKey();

	UPROPERTY()
	TObjectPtr<UInputComponent> MenuInput;

	UPROPERTY()
	TObjectPtr<UGrimPauseMenuWidget> PauseMenu;

	TWeakObjectPtr<APlayerController> BoundController;
	FTSTicker::FDelegateHandle TickHandle;
	bool bPauseOpen = false;
	bool bOptionsApplied = false;
	bool bPausedGame = false;
};
