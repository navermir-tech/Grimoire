#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimSaveTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimItemPickup.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FHitResult;

/**
 * 월드에 떨어진 아이템(전리품 · 버린 것 · 요리 결과). 다가가 F 로 줍는다(UGrimInventoryWidget → Server_PickUp).
 * 설정 bAutoPickupOnOverlap 이면 닿기만 해도 줍는다(서버 겹침).
 * 요리(R16): 날것(표에 CookResult)은 속성 자극을 받는다(IGrimElementalReceiver) —
 *  불 마법이 직접 닿거나(DirectFireHeatSeconds 동안 그 세기로), 근처에 불붙은 모닥불 · 아궁이가 있으면(CookHeatRadius) 익어 가고,
 *  다 익으면 State.Cooked → 결과 아이템으로 바뀐다. 물 · 흙 자극은 달군 열을 식힌다.
 * 모양은 엔진 기본 도형 + 표의 색(아트 전 임시). 세이브: World 「Item.Pickup.<이름>」 v1(게임 중 스폰된 것은 다시 스폰된다).
 */
UCLASS()
class GRIMOIRE_API AGrimItemPickup : public AActor, public IGrimElementalReceiver, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimItemPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// IGrimElementalReceiver (서버)
	virtual void ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* InInstigator) override;
	virtual float GetElementalStateIntensity(const FGameplayTag& StateTag) const override;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: FGrimItemInstance, float 요리 진행 */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	const FGrimItemInstance& GetItem() const { return Item; }

	/** 서버: 담긴 아이템을 바꾼다(스폰 직후 · 일부만 주웠을 때 · 익었을 때). 개수가 0 이면 사라진다. */
	void SetItem(const FGrimItemInstance& NewItem);

	/** 요리 진행(초, 불 세기 1 기준) · 필요한 초. 날것이 아니면 0 · 0. */
	float GetCookProgress() const { return CookProgress; }
	float GetCookSeconds() const;

	/** 지금 달궈지고 있나(복제 — 안내 글) */
	bool IsHeated() const { return bHeated; }

	/** 방금 익어서 바뀐 것인가(State.Cooked) */
	bool WasCooked() const { return bCooked; }

	/** 폰이 주울 수 있는 거리 안인가(PickupRadius + Slack) */
	bool IsWithinReach(const APawn* Pawn, float Slack = 0.f) const;

	/** 서버: 버린 사람은 잠시 저절로 줍지 않는다 */
	void BlockAutoPickupFor(const APawn* Pawn, float Seconds);

	/** 서버: 열을 한 번 본다 — 주변 불 · 직접 불, 진행, 다 익으면 바꾼다(타이머 · 테스트가 부른다). */
	void UpdateCooking(float DeltaSeconds);

	/** 주변에서 가장 센 불(State.Burning) 세기 — 양초 · 폰 · 픽업 제외. Radius 안에 없으면 0. */
	static float FindHeatIntensityNear(const UWorld* World, const FVector& Location, float Radius, const AActor* Ignore);

	/** 이 폰이 주울 수 있는 가장 가까운 픽업 */
	static AGrimItemPickup* FindNearestReachable(const APawn* Pawn);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 맵에 직접 놓을 때 담을 것 */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Item, Category = "Pickup")
	FGrimItemInstance Item;

	UPROPERTY(Replicated)
	float CookProgress = 0.f;

	UPROPERTY(Replicated)
	bool bHeated = false;

	UPROPERTY(Replicated)
	bool bCooked = false;

	UFUNCTION()
	void OnRep_Item();

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void RefreshVisuals();
	void UpdateCookTimer();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	FTimerHandle CookTimer;

	/** 직접 불 자극으로 달궈진 남은 초 · 세기 */
	float DirectHeatRemaining = 0.f;
	float DirectHeatIntensity = 0.f;

	TWeakObjectPtr<const APawn> AutoPickupBlockedPawn;
	double AutoPickupBlockedUntil = 0.0;
};
