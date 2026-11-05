#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/GrimItemTypes.h"
#include "GrimItemStatics.generated.h"

class AGrimItemPickup;
class UAbilitySystemComponent;

/** 아이템 시스템의 바깥 입구 — 생물 전리품(갈래 C) · 퀘스트 지급 · 콘솔이 부른다. */
UCLASS()
class GRIMOIRE_API UGrimItemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 서버: 월드에 아이템 픽업을 놓는다(전리품 · 요리 결과). Location 아래 바닥을 찾아 그 위에 놓는다(못 찾으면 그 자리).
	 * 모르는 아이템 · 개수 0 · 클라이언트에서 부르면 nullptr. 여러 칸 분량이어도 픽업 하나에 담는다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Items", meta = (WorldContext = "World"))
	static AGrimItemPickup* SpawnPickup(UObject* World, FName ItemId, int32 Count, FVector Location);

	/** 서버: 플레이어(폰 · 컨트롤러 · PlayerState) 가방에 넣는다. 넣은 개수(가방이 없거나 차면 0). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Items")
	static int32 GiveItem(AActor* PlayerActor, FName ItemId, int32 Count = 1);

	/** 아이템 표에 있는 ID 인가 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Items")
	static bool IsKnownItem(FName ItemId);

	/** 표시 이름(없으면 ID) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Items")
	static FText GetItemDisplayName(FName ItemId);

	/**
	 * 서버: 음식의 몸 쪽 효과 — 배고픔 회복(생존 틱 GE, 상한은 어트리뷰트 셋이 자른다)과 버프(UGrimFoodBuffEffect, 전 버프는 뗀다).
	 * 질병 굴림은 부르는 쪽(UGrimInventoryComponent — UGrimSurvivalComponent::ApplyDisease).
	 */
	static void ApplyFoodEffects(UAbilitySystemComponent* ASC, const FGrimItemDefinition& Food);

	/** Location 아래 바닥 높이를 찾아 아이템 반 높이만큼 위를 돌려준다 */
	static FVector FindDropLocation(const UWorld* World, const FVector& Location, float HalfHeight, const AActor* Ignore = nullptr);
};
