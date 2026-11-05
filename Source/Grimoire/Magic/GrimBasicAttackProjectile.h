#pragma once

#include "CoreMinimal.h"
#include "Magic/GrimSpellProjectile.h"
#include "GrimBasicAttackProjectile.generated.h"

class UPrimitiveComponent;

/**
 * 기본 공격(R26) 볼트. 보통은 AGrimSpellProjectile 그대로이고, 연타의 마지막(강타)이면
 *  - 더 크게 보이고(모두에게 — 처음 한 번 복제), 서버에서 맞은 대상을 뒤로 밀어낸다(캐릭터 = 날림, 물리 사물 = 속도 변화).
 * 강타 여부는 시전 어빌리티(UGrimBasicAttackAbility)가 FinishSpawning 전에 SetHeavy 로 넣는다.
 */
UCLASS()
class GRIMOIRE_API AGrimBasicAttackProjectile : public AGrimSpellProjectile
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** SpawnActorDeferred 와 FinishSpawning 사이에 부른다(서버 진짜 · 시전자 가짜 모두). */
	void SetHeavy(float InKnockbackSpeed, float InKnockbackUpSpeed, float InPhysicsSpeed, float InVisualScale);

	bool IsHeavy() const { return bHeavy; }

	/**
	 * 서버: 대상을 밀어낸다. 캐릭터는 LaunchCharacter, 물리로 움직이는 부품은 질량 무관 속도 변화.
	 * 플레이어끼리는(아군 피해가 꺼져 있으면) 밀지 않는다. 밀었으면 true.
	 */
	static bool ApplyKnockback(AActor* Target, UPrimitiveComponent* HitComponent, const AActor* Instigator, const FVector& TravelDirection,
		float Speed, float UpSpeed, float PhysicsSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult) override;

	UFUNCTION()
	void OnRep_Heavy();

	/** 정의의 반지름 × HeavyScale 로 판정 구 · 메시 크기를 맞춘다 */
	void ApplyHeavyScale();

	UPROPERTY(ReplicatedUsing = OnRep_Heavy)
	bool bHeavy = false;

	UPROPERTY(Replicated)
	float HeavyScale = 1.f;

	// 서버만 쓴다(복제 안 함)
	float KnockbackSpeed = 0.f;
	float KnockbackUpSpeed = 0.f;
	float PhysicsSpeed = 0.f;
};
