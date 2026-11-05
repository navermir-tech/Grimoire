#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GrimBaseInteractable.generated.h"

class APawn;
class UGrimBuilderComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimBaseInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 거점의 「다가가 F」 대상(문 · 저장고 · 작업대 · 밭 · 거점 핵). 침대 · 줍기 · 냄비처럼 마법 상호작용(R16)의 예외다.
 * 로컬 건축 컴포넌트가 가장 가까운 대상을 골라 안내를 띄우고 F 에 InteractLocal 을 부른다.
 * 화면만 여는 일(저장고 · 작업대 · 거점 핵)은 로컬에서, 상태를 바꾸는 일(문 · 밭)은 Server_Interact → InteractServer.
 */
class GRIMOIRE_API IGrimBaseInteractable
{
	GENERATED_BODY()

public:
	/** 안내 한 줄(키 이름 없이 — 「문 열기」) */
	virtual FText GetInteractPrompt(const APawn* Pawn) const = 0;

	virtual bool CanInteract(const APawn* Pawn) const { return true; }

	/** 로컬: F 를 눌렀다 */
	virtual void InteractLocal(UGrimBuilderComponent* Builder) = 0;

	/** 서버: Server_Interact 가 거리 검사 뒤 부른다 */
	virtual void InteractServer(UGrimBuilderComponent* Builder, APawn* Pawn) {}

	/** 인벤토리에서 열 탭(None = 탭 없음). 열린 동안 이 대상이 「지금 연 시설」이다 */
	virtual FName GetStationTabId() const { return NAME_None; }
};
