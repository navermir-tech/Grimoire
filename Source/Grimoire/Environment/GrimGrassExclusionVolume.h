#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimGrassExclusionVolume.generated.h"

class UBoxComponent;

/**
 * 랜드스케이프 풀 제외 상자 — 상자 안에는 랜드스케이프 그래스(풀 · 꽃 포기)가 나지 않는다.
 * 건물 바닥(탑 1층 방 · 현관 · 앞마당 포장)처럼 랜드스케이프 위에 바닥 메시를 깐 곳에 놓는다(BUG-010).
 * 엔진 ALandscapeProxy::AddExclusionBox 에 상자(월드 축 정렬)를 등록한다 — 에디터 · 게임 모두.
 * 충돌 없음 · 복제 없음(풀은 클라이언트마다 그린다). 돌려 놓아도 월드 축 정렬 상자(바운드)로 잡힌다.
 */
UCLASS(ClassGroup = (Grimoire), HideCategories = (Input, Physics, Collision, Replication, HLOD, Navigation))
class GRIMOIRE_API AGrimGrassExclusionVolume : public AActor
{
	GENERATED_BODY()

public:
	AGrimGrassExclusionVolume();

	/** 풀을 막을 상자(반지름 cm) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grass")
	TObjectPtr<UBoxComponent> Box;

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 지금 상자를 엔진에 (다시) 등록한다 */
	void RegisterExclusion();
	void UnregisterExclusion();

private:
	bool bRegistered = false;
};
