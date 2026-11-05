#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimTargetingTypes.h"
#include "GrimTargetDummy.generated.h"

class UStaticMeshComponent;

/**
 * 테스트용 조준 대상(생물 · 적이 아직 없어서). 엔진 기본 구체 + IGrimTargetable.
 * 맵에 놓거나 콘솔 Grim.Targeting.SpawnDummy 로 만든다. 움직임(MoveAmplitude)은 각 기기가 시간으로 그린다(복제 안 함).
 */
UCLASS()
class GRIMOIRE_API AGrimTargetDummy : public AActor, public IGrimTargetable
{
	GENERATED_BODY()

public:
	AGrimTargetDummy();

	virtual void Tick(float DeltaSeconds) override;

	// IGrimTargetable
	virtual FVector GetTargetPoint() const override;
	virtual EGrimTargetKind GetTargetKind() const override { return Kind; }
	virtual bool CanBeTargetedBy(const AActor* Requester) const override { return bTargetable && !IsHidden(); }

	/** 죽음 흉내 — 끄면 소프트 · 고정 락온이 풀린다 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void SetTargetable(bool bInTargetable) { bTargetable = bInTargetable; }

	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void SetTargetKind(EGrimTargetKind InKind) { Kind = InKind; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
	EGrimTargetKind Kind = EGrimTargetKind::Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
	bool bTargetable = true;

	/** 액터 원점에서 조준점까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
	FVector TargetPointOffset = FVector(0.f, 0.f, 0.f);

	/** 왕복 이동 폭(0 = 가만히). 고정 락온 카메라 추적 확인용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Move")
	FVector MoveAmplitude = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Move", meta = (ClampMin = "0.1", Units = "s"))
	float MovePeriodSeconds = 4.f;

private:
	FVector StartLocation = FVector::ZeroVector;
};
