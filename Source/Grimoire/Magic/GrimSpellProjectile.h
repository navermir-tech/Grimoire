#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GrimSpellProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UGrimSpellDefinition;

/**
 * 마법 투사체. 서버만 스폰하고 위치는 복제된다(클라이언트 예측 스폰 없음).
 * 맞으면 UGrimSpellAbility::ApplySpellHit 으로 피해 GE · 속성 자극을 준다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimSpellProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrimSpellProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버: SpawnActorDeferred 와 FinishSpawning 사이에 부른다. */
	void InitSpell(const UGrimSpellDefinition* InDefinition, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Definition();

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

	void HandleHit(AActor* HitActor, const FHitResult& Hit);

	/** 정의에 맞춰 크기 · 속도 · 색을 맞춘다(서버 · 클라이언트). */
	void ApplyDefinitionVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(ReplicatedUsing = OnRep_Definition)
	TObjectPtr<const UGrimSpellDefinition> Definition;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FGameplayEffectSpecHandle DamageSpec;

	/** 한 대상을 두 번 치지 않는다. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
