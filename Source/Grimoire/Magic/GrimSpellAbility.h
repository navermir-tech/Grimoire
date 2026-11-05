#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Core/GrimTargetingTypes.h"
#include "Magic/GrimSpellDelivery.h"
#include "GrimSpellAbility.generated.h"

class UGrimSpellDefinition;
class AGrimWand;
class AGrimPlayerState;

/**
 * 마법 시전 공용 어빌리티 — Bolt · Stream · Cone · Beam (R01 · R07 · R16 · R17, spell-feel).
 * 무슨 마법인지는 Spec 의 SourceObject(UGrimSpellDefinition)가 정한다. 그래서 한 클래스로 여러 마법을 낸다.
 * 비용 · 쿨다운은 공용 GE(UGrimGE_SpellCost · UGrimGE_SpellCooldown)에 정의의 수치를 SetByCaller 로 넣는다.
 *
 * 네트워크 (LocalPredicted):
 *  1. 시전자(로컬)가 조준(IGrimAimProvider, 없으면 카메라 정면)을 정해 TargetData 로 서버에 보내고,
 *     Cast 큐(예측) · Stream 이면 Channel 큐(예측)를 바로 띄우고, Bolt 면 CastWindup 뒤 가짜 볼트를 쏜다.
 *  2. 서버는 TargetData 를 검증(어긋나면 서버 조준)하고 같은 예측 키로 Cast · Channel 큐를 실행 → 시전자는 두 번 보지 않는다.
 *     남은 Windup 을 기다렸다가 판정한다. Bolt 는 지연만큼 앞으로 당겨 시작(상한 MaxLagCompensation).
 *  3. 맞은 곳의 Impact 큐는 서버 확정 뒤 모두에게.
 * Bolt · Cone 은 서버가 끝내고(복제), Stream 은 시전자가 떼면 끝낸다.
 */
UCLASS()
class GRIMOIRE_API UGrimSpellAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGrimSpellAbility();

	/** Spec 의 SourceObject 에서 정의를 꺼낸다. */
	static const UGrimSpellDefinition* GetSpellDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);

	/** 이 시전자가 쥔 완드(PlayerState 의 마법서). 없으면 nullptr. */
	static AGrimWand* GetWand(const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * 서버: 맞은 대상 하나에 마법을 적용한다.
	 * 대상에 ASC 가 있으면 DamageSpec 을 적용하고, 속성 자극은 받는 쪽이 있으면 언제나 준다.
	 * StimulusIntensity 가 0 이상이면 정의의 자극 세기 대신 그 값(Cone 의 거리별 세기).
	 */
	static void ApplySpellHit(const UGrimSpellDefinition* Definition, UAbilitySystemComponent* SourceASC, const FGameplayEffectSpecHandle& DamageSpec, AActor* Instigator, AActor* HitActor, float StimulusIntensity = -1.f);

	/** 대상 액터의 ASC. 폰이 직접 갖지 않으면 그 폰의 PlayerState 에서 찾는다. */
	static UAbilitySystemComponent* FindTargetASC(AActor* Actor);

	/** 노릴 지점: IGrimTargetable::GetTargetPoint, 없으면 경계 상자 가운데. */
	static FVector GetTargetPointOf(const AActor* Actor);

	/**
	 * 마법 연출 큐를 실행한다 — 큐 파라미터 약속(GrimSpellFXTypes.h): SourceObject = 정의, Location · Normal,
	 * AggregatedSourceTags 에 Element, Instigator = 시전 폰. 처리기가 없어도 아무 일 없다.
	 * 예측 창 안(시전자 활성화 · 서버의 TargetData 콜백)이면 GAS 가 예측 · 중복 제거를 맡는다.
	 */
	static void ExecuteSpellCue(UAbilitySystemComponent* ASC, const FGameplayTag& CueTag, const UGrimSpellDefinition* Definition, AActor* InstigatorPawn, const FVector& Location, const FVector& Normal);
	static FGameplayCueParameters MakeSpellCueParameters(const UGrimSpellDefinition* Definition, AActor* InstigatorPawn, const FVector& Location, const FVector& Normal);

	/**
	 * Stream 포물선을 월드에서 따라간다 — 서버 판정과 연출(MagicFX)이 같은 모양을 쓰도록 공개.
	 * OutPoints = 실제로 지나간 점들(막힌 곳에서 끝), OutHits = 구간 스윕에 걸린 것(가까운 순, 액터 중복 제거). 막혀 끝났으면 true.
	 */
	/** 조준 결과가 대상을 따라가야 하나(대상 있음 · 줌인 조준 아님). 유도 · 조준점 덮어쓰기에 쓴다. */
	static bool FollowsTarget(const FGrimAimResult& Aim);

	static bool TraceStreamArc(const UWorld* World, const UGrimSpellDefinition* Definition, const FVector& Start, const FVector& AimPoint, const AActor* IgnoreActor, TArray<FVector>& OutPoints, TArray<FHitResult>* OutHits);

	/** 완드 보정까지 넣은 마나 비용. */
	static float GetEffectiveManaCost(const UGrimSpellDefinition* Definition, const AGrimWand* Wand);

	/** 완드 보정까지 넣은 쿨다운(초). */
	static float GetEffectiveCooldown(const UGrimSpellDefinition* Definition, const AGrimWand* Wand);

#if !UE_BUILD_SHIPPING
	/** 디버그(Grim.Magic.Cast): 이 폰의 다음 로컬 조준을 이 대상으로 고정한다. */
	static void SetDebugAimTarget(const APawn* Pawn, AActor* Target);
#endif

	//~ UGameplayAbility
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** TargetData 로 조준을 싣는다/꺼낸다(FGameplayAbilityTargetData_SingleTargetHit: TraceStart = 시선 시작, Location = 조준점, Actor = 대상, Item = 모드). */
	static FGameplayAbilityTargetDataHandle MakeAimTargetData(const FGrimAimResult& Aim);
	static bool ReadAimTargetData(const FGameplayAbilityTargetDataHandle& Data, FGrimAimResult& OutAim);

protected:
	/** 로컬: 조준 제공자(B 갈래)의 결과, 없으면 카메라 정면 트레이스. */
	FGrimAimResult ComputeLocalAim(const UGrimSpellDefinition* Definition) const;

	/** 카메라(서버는 클라이언트가 보낸 카메라 캐시 · 컨트롤 회전) 정면 트레이스. */
	FGrimAimResult ComputeViewAim(const UGrimSpellDefinition* Definition) const;

	/** 서버: 클라이언트 조준이 그럴듯한지. 아니면 false(서버 조준으로 바꾼다). 대상이 있으면 대상 조준점을 서버에서 다시 구한다. */
	bool ValidateClientAim(const UGrimSpellDefinition* Definition, FGrimAimResult& InOutAim) const;

	/** 시작점(완드 끝 또는 가슴) · 방향. */
	GrimSpellDelivery::FLaunch ComputeLaunch(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim) const;

	/** 서버: 원격 클라이언트의 TargetData 도착 */
	void OnAimTargetDataReady(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag);

	/** 권한 쪽: 조준이 정해졌다 → 큐 실행, Windup(− 지연) 뒤 판정 예약. */
	void BeginAuthorityCast(const FGrimAimResult& Aim, float OneWayLatency);

	/** 권한 쪽: 판정 */
	void DeliverNow();

	/** 시전자 클라이언트: 가짜 볼트 */
	void SpawnFakeBolt();

	void DeliverBolt(const UGrimSpellDefinition* Definition, AGrimWand* Wand, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec, float AdvanceSeconds);
	void DeliverCone(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec);
	void DeliverBeam(const UGrimSpellDefinition* Definition, const FGrimAimResult& Aim, const FGameplayEffectSpecHandle& DamageSpec);

	/** 권한 쪽 Stream: 한 번 판정 · 마나 소모. 마나가 모자라면 끝낸다. */
	void StreamTick();

	/** 시전자 쪽 · 서버의 안전장치: 끝내라는 복제가 오지 않으면 스스로 끝낸다 */
	void OnSafetyTimeout();

	FGameplayEffectSpecHandle MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const;

	float GetOneWayLatency() const;

	/** GetCooldownTags 가 돌려줄 임시 컨테이너(인스턴스별). */
	mutable FGameplayTagContainer CooldownTagsScratch;

	// 활성화 한 번의 상태 (InstancedPerActor — ActivateAbility 에서 초기화)
	FGrimAimResult CastAim;
	bool bAimReady = false;
	bool bDelivered = false;
	bool bStreamTicked = false;
	/** EndAbility 안에서 마지막 판정을 할 때 다시 끝내지 않게 */
	bool bEndingAbility = false;
	float PendingAdvance = 0.f;
	FGameplayEffectSpecHandle CastDamageSpec;
	FTimerHandle DeliverTimer;
	FTimerHandle StreamTimer;
	FTimerHandle SafetyTimer;
	FTimerHandle FakeBoltTimer;
	FDelegateHandle TargetDataDelegateHandle;
	/** Stream: 대상별 마지막 자극 시각 */
	TMap<TWeakObjectPtr<AActor>, double> StreamLastStimulus;
};
