#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Containers/Ticker.h"
#include "GrimUISubsystem.generated.h"

class APlayerController;
class UEnhancedInputComponent;
class UGrimGrimoireWidget;
class UGrimHUDWidget;

/**
 * 로컬 플레이어마다 하나 — HUD · 그리모어 화면을 띄우고, UI 입력(IA_ToggleGrimoire · IA_SpellSet1~4)을 더한다.
 * 컨트롤러 · 캐릭터 파일을 고치지 않으려고: 컨트롤러가 바뀌는지 0.25초마다 보고, 바뀌면
 * 그 로컬 플레이어의 Enhanced Input 에 IMC_UI 를 더하고, 우리 입력 컴포넌트를 컨트롤러 입력 스택에 올린다.
 * 그리지 못하는 프로세스(-nullrhi · 전용 서버)에서는 위젯을 만들지 않는다(입력 · 콘솔은 그대로).
 */
UCLASS()
class GRIMOIRE_API UGrimUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Grimoire|UI")
	void ToggleGrimoire();

	UFUNCTION(BlueprintCallable, Category = "Grimoire|UI")
	void SetGrimoireOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Grimoire|UI")
	bool IsGrimoireOpen() const { return bGrimoireOpen; }

	/** 단축 슬롯 세트를 바꾼다(서버 RPC) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|UI")
	void SelectSlotSet(int32 SetIndex);

	/** HUD 보이기(콘솔 Grim.UI.HUD) */
	void SetHUDVisible(bool bVisible);

	UGrimHUDWidget* GetHUD() const { return HUD; }

	/** 이 프로세스의 모든 로컬 플레이어 UI 에 Fn */
	static void ForEach(TFunctionRef<void(UGrimUISubsystem&)> Fn);

private:
	bool Tick(float DeltaTime);
	void SetupForController(APlayerController* PC);
	void TearDownController();

	void HandleToggleInput();
	void HandleSlotSetInput(int32 SetIndex);

	UPROPERTY()
	TObjectPtr<UGrimHUDWidget> HUD;

	UPROPERTY()
	TObjectPtr<UGrimGrimoireWidget> Grimoire;

	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> UIInput;

	TWeakObjectPtr<APlayerController> BoundController;
	FTSTicker::FDelegateHandle TickHandle;
	bool bGrimoireOpen = false;
	bool bHUDVisible = true;
};
