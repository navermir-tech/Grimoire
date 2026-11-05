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

	/**
	 * 개발 · 확인용 시작 마법(1서클 셋). 게임에서는 주지 않는다 — 시작할 때 아는 마법이 없고 튜토리얼에서 스킬북으로 배운다(R31).
	 * bGrantStartingSpells 가 켜져 있거나 명령줄에 -GrimStartingSpells 가 있을 때만 준다(헤드리스 확인 스크립트가 쓴다).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Start")
	TArray<TSoftObjectPtr<UGrimSpellDefinition>> StartingSpells;

	UPROPERTY(Config, EditAnywhere, Category = "Start")
	bool bGrantStartingSpells = false;

	bool ShouldGrantStartingSpells() const;

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

	/** 서버 투사체를 시전자 지연만큼 앞으로 당겨 시작할 때의 상한(초) (spell-feel 「네트워크」). */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (ClampMin = "0", Units = "s"))
	float MaxLagCompensation = 0.15f;

	/** 조준점이 이 거리 안이면 완드 끝 대신 가슴 높이에서 쏜다(시차 보정). */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (ClampMin = "0", Units = "cm"))
	float CloseAimDistance = 200.f;

	/** 가슴 높이 = 폰 위치(캡슐 가운데) + 위로 이만큼. */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (Units = "cm"))
	float ChestHeight = 40.f;

	/** Bolt 유도: 대상이 진행 방향에서 이 각보다 벗어나면 휘지 않는다(유턴 방지). */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
	float HomingMaxAngle = 60.f;

	/** Cone 이 한 번에 맞히는 대상 상한(가까운 순). */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (ClampMin = "1"))
	int32 MaxConeTargets = 16;

	/** Stream 이 시작할 때 잡은 대상을 계속 따라가는 각(폰 조준 방향 기준). 벗어나면 대상을 놓고 정면으로. */
	UPROPERTY(Config, EditAnywhere, Category = "Delivery", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
	float StreamKeepTargetAngle = 45.f;

	/** TargetData 검증: 클라이언트 시선 시작이 폰에서 이 거리 안이어야 한다(카메라 붐 길이 + 여유). */
	UPROPERTY(Config, EditAnywhere, Category = "AimValidation", meta = (ClampMin = "0", Units = "cm"))
	float MaxAimOriginError = 800.f;

	/** TargetData 검증: 클라이언트 조준 방향과 서버가 아는 폰 조준 방향의 차 상한(소프트 락온 원뿔보다 넉넉히). */
	UPROPERTY(Config, EditAnywhere, Category = "AimValidation", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
	float MaxAimAngleError = 60.f;

	/** TargetData 검증: 조준점 · 대상이 폰에서 사거리 + 이만큼 안이어야 한다. */
	UPROPERTY(Config, EditAnywhere, Category = "AimValidation", meta = (ClampMin = "0", Units = "cm"))
	float AimRangeSlack = 300.f;
};
