#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "GrimMagicSettings.generated.h"

class UGrimSpellDefinition;
class AGrimWand;

/** 상성 표의 한 칸: 공격 속성이 대상 속성을 칠 때의 피해 배율 (R07). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementAffinity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affinity", meta = (Categories = "Element"))
	FGameplayTag Attacker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affinity", meta = (Categories = "Element"))
	FGameplayTag Defender;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affinity", meta = (ClampMin = "0"))
	float Multiplier = 1.f;
};

/**
 * Magic 시스템 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimMagicSettings].
 * 프로젝트 설정 > Game > Grimoire Magic 에서도 고칠 수 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Magic"))
class GRIMOIRE_API UGrimMagicSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimMagicSettings* Get() { return GetDefault<UGrimMagicSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 표에서 배율을 찾는다. 없으면 DefaultAffinity. 속성이 없으면(무속성) 1. */
	float GetAffinity(const FGameplayTag& Attacker, const FGameplayTag& Defender) const;

	/** 순수 계산 — 테스트용으로 표를 직접 받는다. */
	static float FindAffinity(const TArray<FGrimElementAffinity>& Table, float DefaultMultiplier, const FGameplayTag& Attacker, const FGameplayTag& Defender);

	/** 서클 N 의 최대 마나. 표보다 높은 서클이면 마지막 값. */
	float GetMaxManaForCircle(int32 InCircle) const;

	/** 새 플레이어가 받는 마법. */
	UPROPERTY(Config, EditAnywhere, Category = "Start")
	TArray<TSoftObjectPtr<UGrimSpellDefinition>> StartingSpells;

	/** 처음 쥐는 완드. 비우면 AGrimWand. */
	UPROPERTY(Config, EditAnywhere, Category = "Start")
	TSoftClassPtr<AGrimWand> StartingWandClass;

	/** 완드를 붙일 폰 스켈레탈 메시 소켓. 없으면 폰 루트에 붙인다. */
	UPROPERTY(Config, EditAnywhere, Category = "Start")
	FName WandSocketName = TEXT("hand_r");

	UPROPERTY(Config, EditAnywhere, Category = "Start", meta = (ClampMin = "1", ClampMax = "10"))
	int32 StartingCircle = 1;

	/** [0] = 서클 1 의 최대 마나. */
	UPROPERTY(Config, EditAnywhere, Category = "Mana")
	TArray<float> MaxManaByCircle;

	/** 초당 마나 회복. */
	UPROPERTY(Config, EditAnywhere, Category = "Mana", meta = (ClampMin = "0"))
	float ManaRegenPerSecond = 0.f;

	/** false 면 플레이어끼리 피해를 주지 않는다(속성 자극은 준다). */
	UPROPERTY(Config, EditAnywhere, Category = "Combat")
	bool bFriendlyFire = false;

	/** 표에 없는 속성 조합의 배율. */
	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (ClampMin = "0"))
	float DefaultAffinity = 1.f;

	/** 상성 표 (Q2 · Q3). */
	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (TitleProperty = "{Attacker} → {Defender} ×{Multiplier}"))
	TArray<FGrimElementAffinity> ElementAffinities;
};
