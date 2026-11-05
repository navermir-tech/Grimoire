#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GrimElementalObject.h"
#include "GrimTutorialProps.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;

/**
 * 작물 자리(튜토리얼 임시) — 농사(M4) 전까지 「작물에 물 주기」를 진짜 속성 상태로 판정하려는 흙 두둑.
 * 속성 상태 사물이라 물 마법(아쿠엘)이 닿으면 State.Wet 이 붙는다(규칙표 Water_Soak — 재질 조건 없음). 불은 붙지 않는다(재질 없음).
 * TODO(M4): 작물 시스템이 생기면 진짜 작물로 바꾸고, 퀘스트 목표는 그대로 State.Wet 또는 작물 이벤트로.
 */
UCLASS()
class GRIMOIRE_API AGrimCropPlot : public AGrimElementalObject
{
	GENERATED_BODY()

public:
	AGrimCropPlot();

protected:
	/** 새싹(장식) */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Sprout;
};

/**
 * 퀘스트 발동 볼륨 — 플레이어 폰이 들어오면(서버) QuestToStart 를 시작한다(시작 조건이 안 맞으면 조용히 넘어간다).
 * 예: 사냥꾼 거점에 처음 들어가면 「사냥꾼의 흔적」(튜토리얼을 끝낸 뒤). EventOnEnter 가 있으면 이벤트 신호도 보낸다.
 */
UCLASS()
class GRIMOIRE_API AGrimQuestTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	AGrimQuestTriggerVolume();

	virtual void BeginPlay() override;

	UBoxComponent* GetBounds() const { return Bounds; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Quest")
	TObjectPtr<UBoxComponent> Bounds;

	/** 들어오면 시작할 퀘스트(Quests.csv 의 행 이름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestToStart;

	/** 들어오면 보낼 이벤트(Event 목표) — 비우면 안 보낸다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EventOnEnter;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
