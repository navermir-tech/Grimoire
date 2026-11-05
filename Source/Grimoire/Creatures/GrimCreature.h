#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimTargetingTypes.h"
#include "Creatures/GrimCreatureTypes.h"
#include "Creatures/GrimCreatureMath.h"
#include "GrimCreature.generated.h"

class UGrimAbilitySystemComponent;
class UGrimVitalAttributeSet;
class UGrimElementalStateComponent;
class UGrimCreatureSpecies;
class AGrimHabitatVolume;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UGrimCreatureTamingComponent;
class UGrimMountComponent;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGrimCreatureDiedNative, AGrimCreature* /*Creature*/, AActor* /*Killer*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FGrimCreatureLootNative, AGrimCreature* /*Creature*/, FName /*ItemId*/, int32 /*Count*/, FVector /*Location*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGrimCreatureDiedSignature, AGrimCreature*, Creature, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGrimCreatureLootSignature, AGrimCreature*, Creature, FName, ItemId, int32, Count, FVector, Location);

/**
 * 마법 생물 폰(R02 · R14 · R23 · R35). 서버 권위 — 개체 굴림 · 피해 · 죽음 · 전리품 · AI 판단은 서버만.
 * - 자체 ASC(Minimal 복제) + UGrimVitalAttributeSet(체력) + 루즈 태그 Element.<주> · Element.<부> · SubElement.<부>(Q35 주 · 부 구분).
 * - 속성 상태: UGrimElementalStateComponent 재사용(재질 = 두 속성 몫, 설정 ElementMaterials). 자극은 이 액터가 받아 넘긴다.
 * - 조준: 종류 Creature(공격적 종이면 Enemy). 죽음 · 그림자 걸음 중엔 조준 안 됨.
 * - 모양: 엔진 기본 도형 부위(종 정의 BodyParts)를 각 기기가 개체 기록으로 만든다. 색 = 부위마다 다이내믹 머티리얼 Color.
 * - 길들이기 · 충성도는 UGrimCreatureTamingComponent, 탑승은 UGrimMountComponent(갈래 T). 길들인 생물은 체력 0 이면 죽지 않고 쓰러진다.
 */
UCLASS()
class GRIMOIRE_API AGrimCreature : public ACharacter, public IAbilitySystemInterface, public IGrimTargetable, public IGrimElementalReceiver
{
	GENERATED_BODY()

public:
	AGrimCreature(const FObjectInitializer& ObjectInitializer);

	/** 서버: 종 · 개체로 생물 하나를 놓는다(바닥 높이는 부르는 쪽이 맞춘다 — Location = 발 위치) */
	static AGrimCreature* SpawnCreature(UWorld* World, UGrimCreatureSpecies* Species, const FGrimCreatureInstance& Instance, const FVector& FootLocation, float Yaw, float HealthRatio = 1.f, AGrimHabitatVolume* Habitat = nullptr, int32 HerdId = 0);

	/** 서버: 스폰 직후 · BeginPlay 전(SpawnActorDeferred 사이)에 부른다 */
	void InitCreature(UGrimCreatureSpecies* InSpecies, const FGrimCreatureInstance& InInstance, float InHealthRatio, AGrimHabitatVolume* InHabitat, int32 InHerdId);

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// IGrimTargetable
	virtual FVector GetTargetPoint() const override;
	virtual EGrimTargetKind GetTargetKind() const override;
	virtual bool CanBeTargetedBy(const AActor* Requester) const override;

	// IGrimElementalReceiver — 상태 컴포넌트로 넘기고, 빛 자극이면 그림자 걸음을 풀고, 마법이면 쏜 이에게 화난다
	virtual void ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* StimulusInstigator) override;
	virtual float GetElementalStateIntensity(const FGameplayTag& StateTag) const override;

	UGrimCreatureSpecies* GetSpecies() const { return Species; }
	const FGrimCreatureInstance& GetInstance() const { return Instance; }
	const FGrimCreatureDeath& GetDeath() const { return Death; }
	EGrimCreatureAIState GetAIState() const { return AIState; }
	UGrimElementalStateComponent* GetElementalState() const { return ElementalState; }
	AGrimHabitatVolume* GetHabitat() const { return Habitat.Get(); }
	int32 GetHerdId() const { return HerdId; }
	FVector GetHomeLocation() const { return HomeLocation; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	bool IsDead() const { return Death.bDead; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	bool IsShadowHidden() const { return bShadowHidden; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	float GetHealthRatio() const;

	/** 종 기질(AI 판단 입력) */
	FGrimCreatureTemperamentInfo GetTemperamentInfo() const;

	// ── 서버: AI 가 부른다 ──

	void SetAIState(EGrimCreatureAIState NewState);
	void SetRunning(bool bRun);

	/** 지금 쓸 수 있는 공격 중 하나로 대상을 친다(거리 · 쿨다운 확인). 쳤으면 true */
	bool TryAttack(AActor* Target);

	/** 사거리(가장 긴 공격) */
	float GetMaxAttackRange() const;

	/** 지금 다가갈 거리: 준비된 공격 중 가장 긴 사거리, 준비된 것이 없으면 가장 짧은 사거리(붙어서 기다린다) */
	float GetReadyAttackRange() const;

	/** 대상 캡슐 겉면까지 거리 */
	float GetSurfaceDistanceTo(const AActor* Other) const;

	/** 어빌리티가 친다 — 이번 공격의 대상 */
	AActor* GetPendingAttackTarget() const { return PendingAttackTarget.Get(); }

	/** 최근 때린 상대(기억 시간 안이면) */
	AActor* GetProvoker() const;

	void StartShadowStep();
	void EndShadowStep(bool bStartCooldown);
	bool CanShadowStep() const;

	/** 서버: 디버그 · 시험 — Source ASC(없으면 자기 ASC)로 마법 피해 GE 를 준다 */
	void ApplyDebugDamage(float Amount, const FGameplayTag& Element, UAbilitySystemComponent* SourceASC);

	/** 서버: 즉시 죽인다(디버그) */
	void DebugKill(AActor* Killer);

	/** 서버: 접기 전 기록(위치 · 체력 비율 포함) */
	FGrimFoldedCreature MakeFoldedRecord() const;

	/** 죽음 · 전리품(서버). 갈래 I 의 줍기 아이템은 OnLootDropped 를 듣거나 아래 SpawnLootPickups 자리에서 부른다 */
	FGrimCreatureDiedNative OnCreatureDiedNative;
	FGrimCreatureLootNative OnLootDroppedNative;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Creature")
	FGrimCreatureDiedSignature OnCreatureDied;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Creature")
	FGrimCreatureLootSignature OnLootDropped;

	/** 서버 통계(덤프): 씨앗 퍼뜨리기로 자극을 준 횟수 */
	int32 GetSeedSpreadCount() const { return SeedSpreadCount; }

	/** 접힌 기록 · 서식 구역 없이 콘솔로 놓았는가 */
	bool IsLoose() const { return !Habitat.IsValid(); }

	// ── 길들이기 · 탑승(갈래 T) ──

	UGrimCreatureTamingComponent* GetTaming() const { return Taming; }
	UGrimMountComponent* GetMount() const { return Mount; }

	/** 서버: 개체 기록을 바꾼다(주인 · 충성도 · 이름) — 복제되고 각 기기가 태그를 맞춘다 */
	void SetInstanceServer(const FGrimCreatureInstance& NewInstance);

	/** 서버: 이 상대에게 화나게(맞은 것처럼) · 화를 푼다 */
	void Provoke(AActor* By);
	void ClearProvoker();

	/** 서버: 배회의 중심(놓아주기 · 길들인 뒤) */
	void SetHomeLocation(const FVector& Location) { HomeLocation = Location; }

	/** 서버: 체력을 비율로 정한다(약해짐 표지도 맞춘다) */
	void SetHealthRatioServer(float Ratio);

	/** 서버: 서식 구역에서 떼어 낸다(길들임 — 개체군에서 빠졌다) */
	void DetachFromHabitat() { Habitat = nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Instance();

	UFUNCTION()
	void OnRep_AIState();

	UFUNCTION()
	void OnRep_ShadowHidden();

	UFUNCTION()
	void OnRep_Death();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimVitalAttributeSet> VitalAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimElementalStateComponent> ElementalState;

	/** 기본 도형 부위의 부모(크기 · 눕기 · 웅크리기를 여기서) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BodyRoot;

	/** 길들이기 · 충성도(갈래 T) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimCreatureTamingComponent> Taming;

	/** 탈것(갈래 T — 탈것 종에서만 일한다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGrimMountComponent> Mount;

private:
	/** 서버 · 클라이언트: 종 · 개체로 모양 · 색 · 캡슐 · 태그를 맞춘다(여러 번 불러도 같다) */
	void ApplySpeciesLocal();
	void BuildBodyParts();
	void RefreshBodyVisibility();
	void RefreshPose();

	/** 서버: 능력치 · 공격 어빌리티 · 이동 */
	void ApplySpeciesServer();

	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleElementalStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity);
	void Die(AActor* Killer, AActor* KillerPawn);
	void SpawnLootPickups(const TArray<FGrimCreatureLootDrop>& Drops);
	void NotifyQuestEvent(AActor* PlayerActor, const FString& Prefix) const;
	void NoteAttacker(AActor* InstigatorActor, AActor* CauserActor);

	void TickBurning();
	void TickSeedSpread();

	UPROPERTY(ReplicatedUsing = OnRep_Instance)
	TObjectPtr<UGrimCreatureSpecies> Species;

	UPROPERTY(ReplicatedUsing = OnRep_Instance)
	FGrimCreatureInstance Instance;

	UPROPERTY(ReplicatedUsing = OnRep_AIState)
	EGrimCreatureAIState AIState = EGrimCreatureAIState::Idle;

	UPROPERTY(ReplicatedUsing = OnRep_ShadowHidden)
	bool bShadowHidden = false;

	UPROPERTY(ReplicatedUsing = OnRep_Death)
	FGrimCreatureDeath Death;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BodyPartMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyPartMaterials;

	/** BodyPartMeshes 와 같은 순서의 종 부위 번호 */
	TArray<int32> BodyPartIndices;
	FName BuiltForSpecies;
	int32 BuiltForSeed = INDEX_NONE;
	bool bLocalSpeciesApplied = false;
	bool bLoggedAppear = false;
	bool bLoggedDeath = false;

	// 서버 상태
	TWeakObjectPtr<AGrimHabitatVolume> Habitat;
	int32 HerdId = 0;
	float InitialHealthRatio = 1.f;
	FVector HomeLocation = FVector::ZeroVector;
	bool bServerInitialized = false;
	bool bWeakenedNotified = false;
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> AttackHandles;
	TMap<FGameplayTag, double> NextAttackTime;
	TWeakObjectPtr<AActor> PendingAttackTarget;
	TWeakObjectPtr<AActor> LastAttacker;
	TWeakObjectPtr<AActor> LastDamagerForQuest;
	double LastAttackedTime = -1000.0;
	double NextShadowStepTime = 0.0;
	double BurningSince = -1.0;
	int32 SeedSpreadCount = 0;
	FTimerHandle ShadowStepTimer;
	FTimerHandle BurnTimer;
	FTimerHandle SeedTimer;
	FDelegateHandle HealthChangedHandle;
};
