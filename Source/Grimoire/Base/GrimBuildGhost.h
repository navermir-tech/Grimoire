#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimBuildGhost.generated.h"

class UStaticMeshComponent;

/**
 * 건축 고스트(로컬 전용 — 복제 · 콜리전 · 세이브 없음). 건축 컴포넌트가 로컬 플레이어 쪽에서만 스폰한다.
 * 모양은 구조 조각이면 격자 모양, 시설이면 표의 Extent 상자. 유효하면 초록, 아니면 빨강.
 */
UCLASS(NotPlaceable, Transient)
class GRIMOIRE_API AGrimBuildGhost : public AActor
{
	GENERATED_BODY()

public:
	AGrimBuildGhost();

	/** 조각이 바뀌면 모양을 다시 만든다 */
	void ShowPiece(FName PieceId);
	void SetValid(bool bValid);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Boxes;

	FName ShownPiece;
	int8 ShownValid = -1;
};
