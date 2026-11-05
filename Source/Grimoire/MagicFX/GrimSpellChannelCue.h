#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GrimSpellChannelCue.generated.h"

class UGrimSpellDefinition;

class UFXSystemComponent;
class UAudioComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UGrimSpellVisualSet;
class UGrimMagicFXVisualSet;

/**
 * GameplayCue.Spell.Channel — 누르는 동안 이어지는 마법(Stream 물줄기). 지속 큐(Add/Remove).
 * 있는 동안: 완드 끝에서 조준 방향으로 줄기 몸통 파티클 + 포물선 물방울 구슬 · 도는 소리,
 * 그리모어는 FlipRate 로 계속 넘기고, 오른팔은 뻗고 유지.
 * 시전자마다 하나(bUniqueInstancePerInstigator). 큐 에셋은 GC_Spell_Channel(이 클래스의 블루프린트 자식).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Grim Spell Channel Cue"))
class GRIMOIRE_API AGrimCueNotify_SpellChannel : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGrimCueNotify_SpellChannel(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool Recycle() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	void StartChannel(AActor* MyTarget, const FGameplayCueParameters& Parameters);
	void StopChannel();
	void UpdateStream();
	void EnsureBeads(int32 Count);

	UPROPERTY(Transient)
	TObjectPtr<UFXSystemComponent> TrailComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LoopAudio;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Beads;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BeadMaterial;

	UPROPERTY(Transient)
	TObjectPtr<const UGrimSpellVisualSet> ActiveVisualSet;

	TWeakObjectPtr<APawn> CasterPawn;

	/** 판정과 같은 호를 그리려고 쥐고 있는 마법 정의(UGrimSpellAbility::TraceStreamArc 에 넘긴다). */
	TWeakObjectPtr<const UGrimSpellDefinition> StreamDefinition;
	FLinearColor ActiveColor = FLinearColor::White;
	float LaunchSpeed = 1400.f;
	float GravityZ = -980.f;
	float FlightTime = 0.6f;
	bool bChanneling = false;
};
