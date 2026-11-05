#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/GrimElementalTypes.h"
#include "Magic/GrimSpellDelivery.h"
#include "GrimSpellDefinition.generated.h"

class UGameplayEffect;
class UGrimSpellAbility;
class AGrimSpellProjectile;
class UGrimSpellVisualSet;
class UAnimMontage;
class UTexture2D;

/**
 * 마법이 대상에 닿는 방식 (spell-feel 「전달 방식」). 속성의 물리에 맞춘다.
 * M1.5 는 Bolt · Stream · Cone 을 구현한다. Beam 은 M1 의 구 스윕 판정 그대로, Lob 은 아직 열거형만(Bolt 로 처리).
 */
UENUM(BlueprintType)
enum class EGrimSpellDelivery : uint8
{
	/** 매우 빠른 직선 섬광 + 락온 대상 쪽으로 살짝 휨. 서버 투사체 + 시전자 화면의 가짜 볼트 */
	Bolt,
	/** 누르고 있는 동안 뿜는 포물선 줄기. 서버가 StreamTickInterval 마다 포물선 구간을 스윕 */
	Stream,
	/** 짧은 원뿔 충격파. 즉시 판정, 거리에 따라 세기가 준다 */
	Cone,
	/** 대상까지 이어지는 빛줄기(후속: 빛 속성). 구 스윕 */
	Beam,
	/** 호를 그려 떨어지는 덩어리(후속: 땅 속성). 아직 Bolt 로 처리 */
	Lob
};

/**
 * 마법 하나의 정의 (R05 · R06 · R07). Asset Manager 타입 "Spell".
 * 에셋(DA_Spell_*)은 Scripts/Magic/make_spells.py 가 만든다 — 수치는 스크립트가 정본이다.
 * 연출 쪽(MagicFX)은 GameplayCue 의 SourceObject 로 이 정의를 받아 VisualSet 과 전달 방식 수치를 읽는다.
 */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimSpellDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGrimSpellDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** 이름으로 GameplayTag 를 찾는다 — 에디터 파이썬(Scripts/Magic)이 태그 속성을 채울 때 쓴다. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Magic")
	static FGameplayTag RequestTagByName(FName TagName);

	/** Bolt · Lob 처럼 투사체를 쓰는가 */
	bool UsesProjectile() const { return Delivery == EGrimSpellDelivery::Bolt || Delivery == EGrimSpellDelivery::Lob; }

	/** 판정 · 연출이 같은 포물선을 그리도록: Stream 의 비행 시간(초) = Range ÷ StreamLaunchSpeed */
	float GetStreamFlightTime() const { return StreamLaunchSpeed > 0.f ? Range / StreamLaunchSpeed : 0.f; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 마법 이름 태그(Spell.*). 어빌리티 · 피해 GE 의 에셋 태그로 붙는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (Categories = "Spell"))
	FGameplayTag SpellTag;

	/** 공격 속성(Element.*). 상성 계산에 쓴다. 큐의 AggregatedSourceTags 에도 들어간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** 배우는 데 필요한 서클 (1~10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Circle = 1;

	/** 시전 어빌리티. 대부분 공용 UGrimSpellAbility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	TSubclassOf<UGrimSpellAbility> AbilityClass;

	/** 시전할 때 한 번 드는 마나. 누르고 있는 마법(bChanneled)은 시작할 때 이만큼 + 누르는 동안 ChannelManaPerSecond. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0"))
	float ManaCost = 0.f;

	/** 누르고 있는 동안 초당 마나(Q19 — Stream 만). 모자라면 멈춘다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0", EditCondition = "bChanneled"))
	float ChannelManaPerSecond = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	/** 쿨다운 중 소유자에게 붙는 태그(Cooldown.Spell.*). 마법마다 달라야 서로 막지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	/** 시전 동작(상체 몽타주). 연출 쪽이 Cast 큐에서 재생한다. 비우면 임시 동작. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast")
	TSoftObjectPtr<UAnimMontage> CastMontage;

	/** 입력(=Cast 큐)부터 실제 발사까지(초). 완드를 튕기는 동작의 앞부분. 제안 0.08~0.12. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast", meta = (ClampMin = "0", Units = "s"))
	float CastWindup = 0.f;

	/** 누르고 있는 동안 지속(Stream). 떼면 끝난다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast")
	bool bChanneled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
	EGrimSpellDelivery Delivery = EGrimSpellDelivery::Bolt;

	/** 판정 반지름. Bolt = 투사체 구, Stream = 줄기 구간 스윕 구, Beam = 광선 스윕 구. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

	/** 최대 사거리. 조준 트레이스 · 소프트 락온 거리. Bolt 는 이 거리를 날면 사라진다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (ClampMin = "0", Units = "cm"))
	float Range = 0.f;

	/** 투사체 클래스. 비우면 AGrimSpellProjectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Bolt", meta = (EditCondition = "Delivery == EGrimSpellDelivery::Bolt || Delivery == EGrimSpellDelivery::Lob"))
	TSubclassOf<AGrimSpellProjectile> ProjectileClass;

	/** 볼트 속도(제안 6000~9000). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Bolt", meta = (ClampMin = "0", Units = "cm/s", EditCondition = "Delivery == EGrimSpellDelivery::Bolt || Delivery == EGrimSpellDelivery::Lob"))
	float BoltSpeed = 0.f;

	/** 락온 대상 유도: 초당 최대 회전 각(도). 0 = 유도 없음. 대상이 옆으로 HomingMaxAngle(설정) 넘게 벗어나면 휘지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Bolt", meta = (ClampMin = "0", Units = "deg", EditCondition = "Delivery == EGrimSpellDelivery::Bolt || Delivery == EGrimSpellDelivery::Lob"))
	float HomingStrength = 0.f;

	/** 줄기를 뿜는 속도. 조준점을 지나는 낮은 호로 쏜다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "0", Units = "cm/s", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	float StreamLaunchSpeed = 0.f;

	/** 월드 중력 배율. 클수록 빨리 떨어진다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "0", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	float StreamGravityScale = 1.f;

	/** 서버 판정 주기(초). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "0.02", Units = "s", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	float StreamTickInterval = 0.1f;

	/** 포물선을 나눠 스윕하는 구간 수. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "1", ClampMax = "32", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	int32 StreamSegments = 8;

	/** 줄기가 땅 · 사물에 떨어진 곳의 물보라 반지름. 그 안의 것도 맞는다(바닥에 놓인 양초를 낮게 스칠 때 바닥이 먼저 닿아도 꺼지게). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "0", Units = "cm", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	float StreamSplashRadius = 40.f;

	/** 한 대상에 자극을 주는 간격(초). 처음 닿으면 바로 — 양초는 한 번 스치면 꺼진다. 1.0 = 초당 자극 1회. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Stream", meta = (ClampMin = "0.05", Units = "s", EditCondition = "Delivery == EGrimSpellDelivery::Stream"))
	float StreamStimulusInterval = 1.f;

	/** 원뿔 반각(도). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Cone", meta = (ClampMin = "1", ClampMax = "89", Units = "deg", EditCondition = "Delivery == EGrimSpellDelivery::Cone"))
	float ConeHalfAngle = 30.f;

	/** 원뿔 거리. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Cone", meta = (ClampMin = "0", Units = "cm", EditCondition = "Delivery == EGrimSpellDelivery::Cone"))
	float ConeRange = 0.f;

	/** 거리별 자극 세기. Stimulus.Intensity 대신 이 곡선 값이 자극 세기가 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery|Cone", meta = (EditCondition = "Delivery == EGrimSpellDelivery::Cone"))
	FGrimConeFalloff ConeFalloff;

	/** 상성 · 완드 보정 전 기본 피해. 0 이면 피해 GE 를 적용하지 않는다. Stream 은 자극을 줄 때마다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "0"))
	float Damage = 0.f;

	/** ASC 가 있는 대상에 적용할 GE. 비우면 UGrimGE_SpellDamage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	TSubclassOf<UGameplayEffect> ImpactEffect;

	/** 사물에 주는 속성 자극 (R16). Cone 은 세기를 ConeFalloff 로 바꾼다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FGrimElementalStimulus Stimulus;

	/** 연출 묶음(섬광 · 꼬리 · 터짐 · 소리 · 속성색). 연출 쪽(MagicFX) 큐 처리기가 읽는다. 비어도 게임은 돈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UGrimSpellVisualSet> VisualSet;

	/** 투사체 임시 구 색(VisualSet 이 없을 때). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Color = FLinearColor::White;
};
