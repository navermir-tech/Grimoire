#pragma once

#include "CoreMinimal.h"
#include "Magic/GrimSpellProjectile.h"
#include "GrimRepairProjectile.generated.h"

/**
 * 「수리」(무속성 1서클 — R18 · R28) 볼트. 정의 DA_Spell_Repair(Scripts/Base/make_repair_spell.py)의 ProjectileClass.
 * 맞은 것이 지은 것(구조 컴포넌트)이면 가방의 재료로 고치고, 작업대면 시전자의 장비를, 거점 핵이면 거점 수리(저장고 재료 · 마나 더)를 한다
 * — 모두 UGrimBuilderComponent::PerformRepair(마법 마나는 시전 때 이미 냈다). 그 밖에 맞으면 보통 볼트처럼(피해 0 · 자극 0).
 */
UCLASS()
class GRIMOIRE_API AGrimRepairProjectile : public AGrimSpellProjectile
{
	GENERATED_BODY()

protected:
	virtual void HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult) override;
};
