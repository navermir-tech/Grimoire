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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 로컬 컨트롤러에서만 붙는다. 우선순위는 배열 순서와 무관하게 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<TSoftObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};
