#pragma once

#include "CoreMinimal.h"
#include "Magic/GrimSpellProjectile.h"
#include "GrimPactumProjectile.generated.h"

/**
 * 파크툼(계약 마법 — 길들이기) 볼트. 정의 DA_Spell_Pactum*(Scripts/Magic/make_pactum_spells.py)의 ProjectileClass.
 * 생물에 맞으면 피해 · 자극 대신 서버 포획 판정(UGrimTamingSubsystem::TryTame)을 하고, 숙련도 명중 경험(Q26)을 준다.
 * 생물이 아닌 것에 맞으면 보통 볼트처럼(피해 0 · 자극 세기 0 — 사실상 터짐만).
 */
UCLASS()
class GRIMOIRE_API AGrimPactumProjectile : public AGrimSpellProjectile
{
	GENERATED_BODY()

protected:
	virtual void HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult) override;
};
