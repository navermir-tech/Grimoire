#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimCookingPot.generated.h"

class APlayerState;
class UGrimInventoryComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * 조리 시설 — 아궁이(불붙는 사물, 튜토리얼은 AGrimCampfire 「Tut_Stove」) 위의 냄비 (R16 · 튜토리얼 6단계).
 *  - 다가가 F: 가방에서 레시피 재료를 넣는다(UGrimInventoryComponent::InteractWithPot → Cooking.IngredientsAdded).
 *    넣을 게 없고 끓지 않으면 넣은 것을 도로 꺼낸다.
 *  - 내용물이 레시피(Recipes.csv, Station = StationId)와 정확히 맞고, 냄비 아래(PotHeatRadius)에 불이 붙어 있으면 끓는다.
 *    불이 세면(바람으로 키우면) 빠르다. 불이 꺼지면 멈춘다(진행은 남는다).
 *  - 다 끓으면 재료가 사라지고 결과(스튜)가 냄비 앞에 픽업으로 놓인다 → 넣은 사람들에게 Cooking.Completed.
 * 냄비 자체는 마법을 막지 않는다(아궁이에 불 마법이 닿게). 세이브: 놓인 냄비의 내용물 · 진행.
 */
UCLASS()
class GRIMOIRE_API AGrimCookingPot : public AActor, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimCookingPot();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// IGrimSaveParticipant (World 「CookingPot.<이름>」 v1: int32 개수, FGrimItemInstance × 개수, float 진행)
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	FName GetStationId() const { return StationId; }
	const TArray<FGrimItemInstance>& GetContents() const { return Contents; }
	float GetProgress() const { return Progress; }
	bool IsHeated() const { return bHeated; }

	/** 지금 내용물로 끓일 레시피(정확히 맞는 것). 없으면 nullptr. */
	const FGrimRecipeDefinition* GetMatchingRecipe() const;

	/** 안내용: 가장 많이 찬 레시피의 진행(서버 · 클라이언트) */
	FGrimRecipeProgress DescribeBestRecipe() const;

	bool IsWithinReach(const APawn* Pawn, float Slack = 0.f) const;

	/** 서버: 가방에서 레시피 재료를 넣는다. 넣은 아이템 수(종류 × 개수). */
	int32 AddIngredientsFrom(UGrimInventoryComponent* Inventory);

	/** 서버: 내용물을 가방으로 꺼낸다(끓는 중이 아닐 때). 꺼낸 개수. */
	int32 TakeBackInto(UGrimInventoryComponent* Inventory);

	/** 서버: 내용물을 바로 넣는다(테스트 · 콘솔). */
	void AddContent(const FGrimItemInstance& Item);

	/** 서버: 열을 보고 진행한다(타이머 · 테스트) */
	void UpdateCooking(float DeltaSeconds);

	/** 결과가 놓이는 곳(냄비 앞 바닥 위) */
	FVector GetOutputLocation() const;

	/** 이 폰이 닿는 가장 가까운 냄비 */
	static AGrimCookingPot* FindNearestReachable(const APawn* Pawn);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Pot")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Pot")
	TObjectPtr<UStaticMeshComponent> PotMesh;

	/** 내용물(국물) — 끓으면 색이 짙어진다 */
	UPROPERTY(VisibleAnywhere, Category = "Pot")
	TObjectPtr<UStaticMeshComponent> BrothMesh;

	/** 레시피 표의 Station */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pot")
	FName StationId = TEXT("CookingPot");

	/** 결과를 놓을 곳(냄비 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pot")
	FVector OutputOffset = FVector(70.f, 0.f, 0.f);

	UPROPERTY(ReplicatedUsing = OnRep_Contents)
	TArray<FGrimItemInstance> Contents;

	UPROPERTY(Replicated)
	float Progress = 0.f;

	/** 연출(국물 색)도 같은 콜백으로 */
	UPROPERTY(ReplicatedUsing = OnRep_Contents)
	bool bHeated = false;

	UFUNCTION()
	void OnRep_Contents();

private:
	void RefreshVisuals();
	void UpdateTimer();
	void Complete(const FGrimRecipeDefinition& Recipe);
	void MergeContent(const FGrimItemInstance& Item);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrothMaterial;

	FTimerHandle CookTimer;

	/** 재료를 넣은 사람들(완성 이벤트를 받는다) */
	TArray<TWeakObjectPtr<APlayerState>> Contributors;
};
