#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GrimPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UGrimTargetingComponent;

/**
 * 플레이어 컨트롤러. 로컬 플레이어에게 입력 매핑 컨텍스트를 붙인다.
 * 이동 · 시점 · 마법 액션 바인딩은 폰(AGrimPlayerCharacter)이 하고, 조준 입력(고정 락온 · 줌인 자리)은 여기서 한다.
 * 조준(소프트 · 고정 락온)은 UGrimTargetingComponent 가 맡는다 — IGrimAimProvider 라 마법 쪽이 FindAimProvider 로 찾는다.
 */
UCLASS()
class GRIMOIRE_API AGrimPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGrimPlayerController();

	/** 슈퍼치트(UGrimCheatManager::GrimSuperCheat)의 서버 경로. 출시 빌드에서는 본문이 비어 아무것도 하지 않는다. */
	UFUNCTION(Server, Reliable)
	void Server_SetSuperCheat(bool bEnable);

	UGrimTargetingComponent* GetTargetingComponent() const { return Targeting; }

	/** 고정 락온 중이면 시점 입력을 옆 대상 넘기기에 쓰고 카메라는 돌리지 않는다(카메라는 대상을 따라간다). */
	virtual void AddYawInput(float Val) override;
	virtual void AddPitchInput(float Val) override;

protected:
	/** 개발 빌드에서는 멀티 클라이언트에도 CheatManager 를 만든다(엔진 기본은 싱글 · 에디터만). */
	virtual void AddCheats(bool bForce) override;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 로컬 컨트롤러에서만 붙는다. 우선순위는 배열 순서와 무관하게 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<TSoftObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** 고정 락온 토글(R3 · Caps Lock, Q17) */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> LockOnAction;

	/** 줌인 조준 자리(R24 — 스펙 조사 중, 아직 동작 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> AimZoomAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grimoire|Targeting")
	TObjectPtr<UGrimTargetingComponent> Targeting;

private:
	void HandleLockOnPressed();
	void HandleAimZoomPressed();
	void HandleAimZoomReleased();
};
