#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GrimSpellProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UFXSystemComponent;
class UAbilitySystemComponent;
class UGrimSpellDefinition;

/**
 * 마법 투사체 — Bolt (spell-feel 「네트워크」).
 *  - 진짜: 서버만 스폰, 위치 복제. 맞으면 UGrimSpellAbility::ApplySpellHit + Impact 큐. 시전자 지연만큼 앞으로 당겨 시작한다(CatchUp).
 *  - 가짜: 시전자 클라이언트가 누르는 즉시 스스로 스폰(복제 안 함, 판정 없음, 지형에서 멈춤). 진짜가 사라지면 같이 사라진다.
 *  - 시전자 클라이언트에서는 진짜 복제본을 숨긴다(가짜가 대신 보인다). 다른 클라이언트는 진짜를 본다.
 * 꼬리는 정의의 VisualSet.Trail 을 붙인다(움직이는 액터라 큐 대신 여기서). 없으면 속성색 구만 보인다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimSpellProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrimSpellProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** 서버: SpawnActorDeferred 와 FinishSpawning 사이에 부른다. HomingTarget 이 있으면 정의의 HomingStrength 로 휜다. */
	void InitSpell(const UGrimSpellDefinition* InDefinition, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec, AActor* InHomingTarget);

	/** 시전자 클라이언트: 가짜 볼트로 만든다. SpawnActorDeferred 와 FinishSpawning 사이에 부른다. */
	void InitFake(const UGrimSpellDefinition* InDefinition, AActor* InHomingTarget);

	/** 서버: 스폰 직후 Seconds 만큼 미리 날린다(지연 보정). 그 사이에 맞으면 바로 처리된다. */
	void CatchUp(float Seconds);

	bool IsFake() const { return bFake; }
	const UGrimSpellDefinition* GetDefinition() const { return Definition; }

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	void OnRep_Definition();

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

	/** 서버: 맞음 처리. 자식(기본 공격 강타)이 앞에 밀어냄을 더한다. */
	virtual void HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult);

	/** 정의에 맞춰 크기 · 속도 · 색 · 유도를 맞춘다(서버 · 클라이언트). */
	void ApplyDefinitionVisuals();

	/** 시전자 클라이언트에서 본 진짜 투사체인가(가짜가 대신 보이므로 숨긴다) */
	bool IsHiddenForLocalCaster() const;

	void StartTrail();

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(Transient)
	TObjectPtr<UFXSystemComponent> TrailComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Definition)
	TObjectPtr<const UGrimSpellDefinition> Definition;

	/** 유도 대상. 복제(처음 한 번) — 클라이언트 사본도 같은 쪽으로 휘어 보정이 작다. */
	UPROPERTY(Replicated)
	TWeakObjectPtr<AActor> HomingTarget;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FGameplayEffectSpecHandle DamageSpec;

	bool bFake = false;

	/** 한 대상을 두 번 치지 않는다. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
