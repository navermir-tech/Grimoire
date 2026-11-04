#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GrimElementalObject.generated.h"

class UGrimElementalStateComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 속성 상태를 가진 사물의 공통 뼈대: 몸체 메시 + 상태 컴포넌트 + 불꽃(원뿔 메시) + 포인트 라이트.
 * 상태가 바뀌면(서버 · 클라이언트 모두 OnStateChanged) 연출만 다시 맞춘다. 평소에는 휴면(DORM_DormantAll).
 * 메시는 엔진 기본 도형이라 에셋 없이 C++ 만으로 맵에 놓을 수 있다.
 */
UCLASS(Abstract)
class GRIMOIRE_API AGrimElementalObject : public AActor
{
	GENERATED_BODY()

public:
	AGrimElementalObject();

	UGrimElementalStateComponent* GetElementalState() const { return ElementalState; }

	/** 현재 상태로 불꽃 · 빛 · 색을 맞춘다 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Elemental")
	void RefreshElementalVisuals();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FlameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FlameLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimElementalStateComponent> ElementalState;

	/** 불꽃 세기 1 일 때 원뿔 크기. 세기가 커지면 √세기 배로 자란다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FVector FlameScaleAtOne = FVector(0.05f, 0.05f, 0.1f);

	/** 불꽃 세기 1 당 빛 세기(cd) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual", meta = (ClampMin = "0"))
	float LightCandelaPerIntensity = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FLinearColor BodyColor = FLinearColor(0.9f, 0.85f, 0.7f);

	/** 젖음 세기가 WetIntensityForFullColor 일 때의 색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FLinearColor WetColor = FLinearColor(0.25f, 0.3f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual", meta = (ClampMin = "0.01"))
	float WetIntensityForFullColor = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FLinearColor CharredColor = FLinearColor(0.05f, 0.04f, 0.04f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FLinearColor FlameColor = FLinearColor(1.f, 0.45f, 0.05f);

	/** BasicShapeMaterial 의 색 파라미터 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elemental|Visual")
	FName ColorParameterName = TEXT("Color");

private:
	UFUNCTION()
	void HandleElementalStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
};

/** 양초: 불로 켜고, 바람으로 키우고(최대 3), 돌풍 · 물 · 흙에 꺼진다. 가까운 양초 · 풀에 불이 옮는다. */
UCLASS()
class GRIMOIRE_API AGrimCandle : public AGrimElementalObject
{
	GENERATED_BODY()

public:
	AGrimCandle();
};

/** 모닥불: 양초와 같은 규칙, 더 크고 밝다. 생존 쪽은 GetStateIntensity(State.Burning) 으로 열원을 본다. */
UCLASS()
class GRIMOIRE_API AGrimCampfire : public AGrimElementalObject
{
	GENERATED_BODY()

public:
	AGrimCampfire();
};

/** 마른 풀: 불이 붙으면 잠시 타며 옆으로 번지고, 다 타면 State.Charred(다시 안 탄다). 밟고 지나갈 수 있다. */
UCLASS()
class GRIMOIRE_API AGrimDryGrass : public AGrimElementalObject
{
	GENERATED_BODY()

public:
	AGrimDryGrass();
};
