#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GrimWand.generated.h"

class UStaticMeshComponent;

/**
 * 완드 (R01). 완드가 없으면 마법을 쓸 수 없다(Q4 제안).
 * 서버가 스폰해 폰의 손 소켓에 붙이고, 부착 · 소유는 복제된다.
 * 완드 종류는 이 클래스의 블루프린트 자식으로 만든다 — 보정값은 클래스 기본값이 데이터다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimWand : public AActor
{
	GENERATED_BODY()

public:
	AGrimWand();

	/** 투사체 · 광선이 나가는 곳(월드). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Wand")
	FTransform GetMuzzleTransform() const;

	/** 속성 위력 배율. 표에 없으면 1. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Wand")
	float GetElementPower(FGameplayTag Element) const;

	float GetManaEfficiency() const { return ManaEfficiency; }
	float GetCastSpeed() const { return CastSpeed; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wand")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 투사체가 나가는 지점. */
	UPROPERTY(VisibleAnywhere, Category = "Wand")
	TObjectPtr<USceneComponent> Muzzle;

	/** 속성 친화: 속성별 위력 배율. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wand", meta = (Categories = "Element"))
	TMap<FGameplayTag, float> ElementPower;

	/** 마나 절약 비율 (0~1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wand", meta = (ClampMin = "0", ClampMax = "1"))
	float ManaEfficiency = 0.f;

	/** 시전 속도 배율. 쿨다운을 나눈다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wand", meta = (ClampMin = "0.1"))
	float CastSpeed = 1.f;
};
