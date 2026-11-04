#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/GrimElementalTypes.h"
#include "GrimSpellDefinition.generated.h"

class UGameplayEffect;
class UGrimSpellAbility;
class AGrimSpellProjectile;
class UTexture2D;

/** 마법이 대상에 닿는 방식. M1 은 투사체 · 광선만 구현한다. */
UENUM(BlueprintType)
enum class EGrimSpellDelivery : uint8
{
	Projectile,
	Beam
};

/**
 * 마법 하나의 정의 (R05 · R06 · R07). Asset Manager 타입 "Spell".
 * 에셋(DA_Spell_*)은 Scripts/Magic/make_spells.py 가 만든다 — 수치는 스크립트가 정본이다.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 마법 이름 태그(Spell.*). 어빌리티 · 피해 GE 의 에셋 태그로 붙는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (Categories = "Spell"))
	FGameplayTag SpellTag;

	/** 공격 속성(Element.*). 상성 계산에 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** 배우는 데 필요한 서클 (1~10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Circle = 1;

	/** 시전 어빌리티. 대부분 공용 UGrimSpellAbility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	TSubclassOf<UGrimSpellAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0"))
	float ManaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	/** 쿨다운 중 소유자에게 붙는 태그(Cooldown.Spell.*). 마법마다 달라야 서로 막지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	/** 입력부터 발사까지(초). 0 이면 즉시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0", Units = "s"))
	float CastTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
	EGrimSpellDelivery Delivery = EGrimSpellDelivery::Projectile;

	/** 투사체 클래스. 비우면 AGrimSpellProjectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (EditCondition = "Delivery == EGrimSpellDelivery::Projectile"))
	TSubclassOf<AGrimSpellProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (ClampMin = "0", Units = "cm/s", EditCondition = "Delivery == EGrimSpellDelivery::Projectile"))
	float ProjectileSpeed = 0.f;

	/** 투사체 · 광선의 판정 반지름. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

	/** 최대 사거리. 투사체는 이 거리를 날면 사라진다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery", meta = (ClampMin = "0", Units = "cm"))
	float Range = 0.f;

	/** 상성 · 완드 보정 전 기본 피해. 0 이면 피해 GE 를 적용하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "0"))
	float Damage = 0.f;

	/** ASC 가 있는 대상에 적용할 GE. 비우면 UGrimGE_SpellDamage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	TSubclassOf<UGameplayEffect> ImpactEffect;

	/** 사물에 주는 속성 자극 (R16). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FGrimElementalStimulus Stimulus;

	/** 투사체 · 연출 색. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Color = FLinearColor::White;
};
