#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"
#include "GrimSpellFXTypes.generated.h"

class UFXSystemAsset;
class USoundBase;
class UCameraShakeBase;
class UNiagaraComponent;

/**
 * 마법 연출 큐 (M1.5). 마법 쪽(Magic)이 시전 · 명중 때 이 태그로 GameplayCue 를 실행하고,
 * 연출 쪽(MagicFX)이 큐 처리기에서 VisualSet 을 읽어 그린다.
 *
 * FGameplayCueParameters 약속:
 *  - SourceObject       = 그 마법의 UGrimSpellDefinition (VisualSet 은 정의에서 찾는다)
 *  - Location · Normal  = Cast: 완드 끝 · 발사 방향 / Impact: 맞은 곳 · 표면 법선
 *  - AggregatedSourceTags 에 마법의 Element.* 태그
 *  - Instigator         = 시전 폰 (그리모어 책장 넘김은 이 폰의 책에서)
 */
namespace GrimCueTags
{
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Cast);     // GameplayCue.Spell.Cast — 한 번 (Bolt · Cone · Stream 시작)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Channel);  // GameplayCue.Spell.Channel — 지속 (Stream 누르는 동안, Add/Remove)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Impact);   // GameplayCue.Spell.Impact — 맞은 곳
}

/** 마법 하나의 연출 묶음. 엔진 기본 파티클 · Niagara 어느 쪽이든 넣을 수 있게 UFXSystemAsset 으로 받는다. */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimSpellVisualSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 완드 끝 섬광 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TObjectPtr<UFXSystemAsset> MuzzleFlash;

	/** 볼트 꼬리 · 줄기 몸통 · 원뿔 파동 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TObjectPtr<UFXSystemAsset> Trail;

	/** 맞은 곳 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TObjectPtr<UFXSystemAsset> Impact;

	/** 볼트 투사체의 색 구를 숨긴다 — 꼬리(Trail) 이펙트가 머리 빛까지 그릴 때 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	bool bHideProjectileMesh = false;

	/**
	 * 볼트 발사 빛줄기(Q85): 볼트가 보이기 시작할 때 완드 끝부터 한 번 긋는 빛줄기(연출만, 판정 없음).
	 * 가까운 표적은 비행이 몇 프레임이라 꼬리가 거의 안 보이고, 원격 시전자의 볼트는 지연만큼 앞에서 나타난다 — 그 구간을 잇는다.
	 * 시스템 +Z = 날아가는 쪽, 사용자 변수 Length(cm) · DrawTime(초) · Color 를 투사체가 넣는다. 비우면 긋지 않는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TObjectPtr<UFXSystemAsset> LaunchStreak;

	/** 발사 빛줄기 길이 = 볼트 속력 × 이 시간(맞을 곳이 더 가까우면 거기까지) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0", Units = "s"))
	float LaunchStreakSeconds = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> CastSound;

	/** Stream 처럼 누르는 동안 도는 소리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> LoopSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> ImpactSound;

	/** 아주 약한 흔들림(시전자 화면만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> CameraShake;

	/** 트레일 · 대상 표식 · 그리모어 글자 빛 공통 색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor ElementColor = FLinearColor::White;

	/** Niagara 이펙트에 마법 색을 입힌다 — 시스템이 드러낸 색 변수(이름에 Color) 모두에. 팩마다 이름이 달라서
	 *  (Color · FlameColor · BaseColor · BurstColor …) 하나만 고르지 않는다. 연기 · 흙 · 먼지 색은 두어 탁하지 않게 한다. */
	static void TintNiagara(UNiagaraComponent* Component, const FLinearColor& Color);
};
