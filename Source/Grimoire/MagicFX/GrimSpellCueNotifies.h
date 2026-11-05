#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GrimSpellCueNotifies.generated.h"

/**
 * GameplayCue.Spell.Cast — 시전 한 번 (Bolt · Cone · Stream 시작).
 * 완드 끝 섬광(파티클 + 속성색 펄스) · 소리 · 아주 약한 흔들림(시전자만) · 원뿔 파동(쓸기 동작일 때),
 * 그리고 시전자의 그리모어 책장 한 장 · 상체 시전 동작.
 * 큐 에셋 GC_Spell_Cast(이 클래스의 블루프린트 자식)가 /Game/Grimoire/MagicFX/Cues 에 있어야 큐 관리자가 찾는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Grim Spell Cast Cue"))
class GRIMOIRE_API UGrimCueNotify_SpellCast : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGrimCueNotify_SpellCast(const FObjectInitializer& ObjectInitializer);

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

/**
 * GameplayCue.Spell.Impact — 맞은 곳. 터짐 파티클 + 속성색 펄스 · 소리 · 자국(그을음 · 젖음 데칼).
 * Location · Normal = 맞은 곳 · 표면 법선. 큐 에셋은 GC_Spell_Impact.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Grim Spell Impact Cue"))
class GRIMOIRE_API UGrimCueNotify_SpellImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGrimCueNotify_SpellImpact(const FObjectInitializer& ObjectInitializer);

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
