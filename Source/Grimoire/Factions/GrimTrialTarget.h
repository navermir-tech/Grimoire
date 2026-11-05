#pragma once

#include "CoreMinimal.h"
#include "Targeting/GrimTargetDummy.h"
#include "Core/GrimElementalTypes.h"
#include "GrimTrialTarget.generated.h"

class UMaterialInstanceDynamic;

/**
 * 승급 시험의 표적 — 조준 더미(AGrimTargetDummy, 움직임 포함)에 「맞으면 센다」를 더한 것.
 * 마법이 맞으면 속성 자극이 들어오므로(IGrimElementalReceiver) 그것을 명중으로 보고, 쏜 플레이어의 퀘스트에 HitTarget 신호(액터 태그마다)를 보낸다.
 * 시험 단계 밖에서는 꺼져 있다(조준 · 명중 안 됨, 회색) — 적(Enemy)으로 잡히면 근처 침대에서 잠을 막기 때문이기도 하다.
 */
UCLASS()
class GRIMOIRE_API AGrimTrialTarget : public AGrimTargetDummy, public IGrimElementalReceiver
{
	GENERATED_BODY()

public:
	AGrimTrialTarget();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버: 시험 단계가 시작 · 끝날 때 */
	void SetTrialActive(bool bActive);

	bool IsTrialActive() const { return bTrialActive; }

	/** 켠 뒤 맞은 수(서버) */
	int32 GetHitCount() const { return HitCount; }

	// IGrimElementalReceiver
	virtual void ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* StimulusInstigator) override;
	virtual float GetElementalStateIntensity(const FGameplayTag& StateTag) const override { return 0.f; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_TrialActive();

	void ApplyActiveVisuals();

	/** 처음부터 켜 둘까(맵 시험용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trial")
	bool bStartActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trial")
	FLinearColor ActiveColor = FLinearColor(0.95f, 0.25f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trial")
	FLinearColor InactiveColor = FLinearColor(0.35f, 0.35f, 0.35f);

	/** 마법(EGrimStimulusSource::Spell)만 센다. 끄면 환경 자극(번짐 · 날씨)도 센다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trial")
	bool bCountOnlySpells = true;

	/**
	 * 이만큼 맞으면 부서진다(0 = 안 부서짐) — 기본 공격으로 과녁 부숴 보기. 부서지면 숨고 BreakTarget 신호, 다시 켜면 되살아난다.
	 * 마법은 무엇이든 한 번 = 1(기본 공격만 가려 세려면 자극에 마법 정보가 필요하다 — 통합 때 · 지금은 표가 기본 공격만 준다).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trial", meta = (ClampMin = "0"))
	int32 HitsToBreak = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TrialActive)
	bool bTrialActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_TrialActive)
	bool bBroken = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	int32 HitCount = 0;
};
