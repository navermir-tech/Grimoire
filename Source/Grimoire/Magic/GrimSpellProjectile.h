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
class UGrimSpellVisualSet;
class UFXSystemAsset;

/**
 * 마법 투사체 — Bolt (spell-feel 「네트워크」).
 *  - 진짜: 서버만 스폰, 위치 복제. 맞으면 UGrimSpellAbility::ApplySpellHit + Impact 큐. 시전자 지연만큼 앞으로 당겨 시작한다(CatchUp).
 *  - 가짜: 시전자 클라이언트가 누르는 즉시 스스로 스폰(복제 안 함, 판정 없음, 지형에서 멈춤). 진짜가 사라지면 같이 사라진다.
 *  - 시전자 클라이언트에서는 진짜 복제본을 숨긴다(가짜가 대신 보인다). 다른 클라이언트는 진짜를 본다.
 * 꼬리는 연출 묶음의 Trail(+ TrailAccent)을 붙인다(움직이는 액터라 큐 대신 여기서). 묶음은 정의의 VisualSet, 없으면 속성 기본 묶음
 * (부모 속성으로 거슬러 — Q113, UGrimMagicFXStatics::FindDefaultVisualSet). 그것도 없으면 속성색 구만 보인다.
 * 원격 시전자의 지연을 당긴 볼트(LaunchAdvanceSeconds > 0)만 보이는 사본이 완드 끝부터 볼트 자리까지 VisualSet.LaunchStreak 을
 * 한 번 긋는다(Q85 · Q110). 혼자 · 호스트는 완드 끝에서 날아가므로 긋지 않는다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimSpellProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** 맞은 것이 아니라 감지용 영역(트리거 상자)인가 — 질의 전용이고 아무것도 막지 않는 도형. 마법 판정(물줄기 · 원뿔 · 빛줄기)도 쓴다 */
	static bool IsSensorComponent(const UPrimitiveComponent* Component);

	AGrimSpellProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** 서버: SpawnActorDeferred 와 FinishSpawning 사이에 부른다. HomingTarget 이 있으면 정의의 HomingStrength 로 휜다. */
	void InitSpell(const UGrimSpellDefinition* InDefinition, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec, AActor* InHomingTarget);

	/** 시전자 클라이언트: 가짜 볼트로 만든다. SpawnActorDeferred 와 FinishSpawning 사이에 부른다. */
	void InitFake(const UGrimSpellDefinition* InDefinition, AActor* InHomingTarget);

	/** 서버: 스폰 직후 Seconds 만큼 미리 날린다(지연 보정). 그 사이에 맞으면 바로 처리된다. 당긴 시간은 복제돼 발사 빛줄기를 긋게 한다(Q110). */
	void CatchUp(float Seconds);

	bool IsFake() const { return bFake; }
	const UGrimSpellDefinition* GetDefinition() const { return Definition; }

	/** 원격 지연으로 앞당긴 시간(초, 0 = 완드 끝에서 쏨). 처음 한 번 복제 */
	float GetLaunchAdvanceSeconds() const { return LaunchAdvanceSeconds; }

	/** 이 볼트의 연출 묶음 — 정의의 VisualSet, 없으면 속성 기본 묶음(볼트용 먼저, 부모 속성으로 거슬러) */
	const UGrimSpellVisualSet* GetVisualSet() const;

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

	/** 원격 지연으로 앞당긴 볼트만, 완드 끝부터 볼트 자리까지 빛줄기를 한 번 긋는다(Q85 · Q110, 연출만). 보이는 사본에서 한 번. */
	void SpawnLaunchStreak();

	UFXSystemComponent* SpawnAttachedFX(UFXSystemAsset* Asset, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(Transient)
	TObjectPtr<UFXSystemComponent> TrailComponent;

	/** 속성 덧그림(어둠 연기 가닥 · 흙 돌가루 — Q87) */
	UPROPERTY(Transient)
	TObjectPtr<UFXSystemComponent> TrailAccentComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Definition)
	TObjectPtr<const UGrimSpellDefinition> Definition;

	/** 원격 지연으로 앞당긴 시간(서버 CatchUp 이 넣는다, 처음 한 번 복제) — 0 보다 크면 발사 빛줄기를 긋는다(Q110) */
	UPROPERTY(Replicated)
	float LaunchAdvanceSeconds = 0.f;

	/** 이 사본이 시작한 자리(완드를 모를 때 발사 빛줄기의 시작) */
	FVector LaunchOrigin = FVector::ZeroVector;

	/** GetVisualSet 이 찾은 것(정의가 바뀌면 다시) */
	UPROPERTY(Transient)
	mutable TObjectPtr<const UGrimSpellVisualSet> CachedVisualSet;

	UPROPERTY(Transient)
	mutable TObjectPtr<const UGrimSpellDefinition> CachedVisualSetFor;

	/** 유도 대상. 복제(처음 한 번) — 클라이언트 사본도 같은 쪽으로 휘어 보정이 작다. */
	UPROPERTY(Replicated)
	TWeakObjectPtr<AActor> HomingTarget;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FGameplayEffectSpecHandle DamageSpec;

	bool bFake = false;

	/** 한 대상을 두 번 치지 않는다. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
