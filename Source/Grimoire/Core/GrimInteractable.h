#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Interface.h"
#include "GrimInteractable.generated.h"

class APawn;

/** F 로 할 수 있는 일 하나(가장 가까운 것 고르기 · 안내 글). */
struct FGrimInteractionOffer
{
	/** 폰에서 대상 자리까지 거리(cm) — 작을수록 먼저 */
	float Distance = TNumericLimits<float>::Max();

	/** 화면 안내(예: 「책장 줍기」). 키 이름은 부르는 쪽이 붙인다 */
	FText Prompt;

	/** 대상 안의 무엇(셀 관리자의 점 번호 등). 서버가 다시 검사한다 */
	int32 Token = INDEX_NONE;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * F(상호작용 키)로 다룰 수 있는 사물 — 아이템 픽업 · 냄비 밖의 것(꾸밈 소품 · 채집 셀 관리자 · 책장).
 * 가방 위젯(UGrimInventoryWidget)이 픽업 · 냄비가 없을 때 UGrimInteractableSubsystem::FindBest 로 고르고,
 * UGrimInventoryComponent::Server_Interact 가 서버에서 Interact 를 부른다(서버가 거리를 다시 잰다).
 */
class GRIMOIRE_API IGrimInteractable
{
	GENERATED_BODY()

public:
	/** 이 폰이 지금 할 수 있는 일이 있으면 true 와 내용. 클라이언트 · 서버 모두에서 불린다(복제된 상태만 읽는다) */
	virtual bool GetInteractionOffer(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const = 0;

	/** 서버: 한다. Token = 그 제안의 Token. 했으면 true */
	virtual bool Interact(APawn* Pawn, int32 Token) = 0;
};

/**
 * F 대상 목록(월드 서브시스템). 대상은 BeginPlay 에서 등록하고 EndPlay 에서 뺀다.
 * 오버랩 대신 목록을 훑는다 — 셀 관리자처럼 충돌 없이 넓은 범위를 맡는 대상도 받기 위해서다(대상 수는 수십).
 */
UCLASS()
class GRIMOIRE_API UGrimInteractableSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(UObject* Interactable);
	void Unregister(UObject* Interactable);

	/** 이 폰이 할 수 있는 가장 가까운 일. 없으면 nullptr */
	UObject* FindBest(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const;

	int32 Num() const { return Interactables.Num(); }

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UObject>> Interactables;
};
