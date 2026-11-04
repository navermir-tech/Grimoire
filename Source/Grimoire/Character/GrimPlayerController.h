#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GrimPlayerController.generated.h"

class UInputMappingContext;

/**
 * 플레이어 컨트롤러. 로컬 플레이어에게 입력 매핑 컨텍스트를 붙인다.
 * 액션 → 동작 바인딩은 폰(AGrimPlayerCharacter)이 한다.
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

protected:
	/** 개발 빌드에서는 멀티 클라이언트에도 CheatManager 를 만든다(엔진 기본은 싱글 · 에디터만). */
	virtual void AddCheats(bool bForce) override;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 로컬 컨트롤러에서만 붙는다. 우선순위는 배열 순서와 무관하게 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<TSoftObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
