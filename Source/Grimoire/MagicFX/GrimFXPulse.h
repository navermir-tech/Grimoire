#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicFX/GrimMagicFXTypes.h"
#include "GrimFXPulse.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * 짧게 커지며 사라지는 속성색 도형 + 빛 (임시 연출). 큐 처리기가 각 기기에서 로컬로 띄운다 — 복제하지 않는다.
 * 엔진 기본 파티클 템플릿은 색 파라미터가 없어서 「색이 곧 정보」를 이 펄스가 맡는다.
 */
UCLASS(NotBlueprintable, Transient)
class GRIMOIRE_API AGrimFXPulse : public AActor
{
	GENERATED_BODY()

public:
	AGrimFXPulse();

	/**
	 * 펄스를 띄운다. Direction 은 원뿔 · 원반의 축(원뿔: 꼭짓점 → 퍼지는 쪽, 원반: 표면 법선).
	 * SizeOverride 가 0 이 아니면 EndSize 대신 쓴다(원뿔 파동 = 마법 사거리).
	 */
	static AGrimFXPulse* Spawn(UWorld* World, const FGrimFXPulseSpec& Spec, const FVector& Location, const FVector& Direction, const FLinearColor& Color, const FVector& SizeOverride = FVector::ZeroVector);

	virtual void Tick(float DeltaSeconds) override;

protected:
	void Init(const FGrimFXPulseSpec& InSpec, const FVector& Direction, const FLinearColor& InColor, const FVector& SizeOverride);
	void Apply(float Alpha);

	UPROPERTY(VisibleAnywhere, Category = "Pulse")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Pulse")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Pulse")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	FGrimFXPulseSpec Spec;
	FVector StartSize = FVector::ZeroVector;
	FVector EndSize = FVector::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	float Elapsed = 0.f;
};
