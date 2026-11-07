#pragma once

#include "CoreMinimal.h"
#include "Magic/GrimSpellProjectile.h"
#include "GrimRepairProjectile.generated.h"

/**
 * 「레스티투오」(무속성 2서클 수리 볼트 — R18 · R28 · Q121 · Q123). 정의 DA_Spell_Repair(Scripts/Base/make_repair_spell.py)의 ProjectileClass.
 * 맞은 것이 지은 것(구조 컴포넌트 — 조각 · 시설 · 작업대 · 거점 핵의 몸)이면 가방의 재료로 그 하나를 고친다 — UGrimBuilderComponent::PerformRepair(bFromSpell).
 * 장비 수리는 작업대 「수리」 탭, 거점 전체는 거점 핵 「전체 수리」(막시마)다. 그 밖에 맞으면 보통 볼트처럼(피해 0 · 자극 0).
 */
UCLASS()
class GRIMOIRE_API AGrimRepairProjectile : public AGrimSpellProjectile
{
	GENERATED_BODY()

protected:
	virtual void HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult) override;
};
