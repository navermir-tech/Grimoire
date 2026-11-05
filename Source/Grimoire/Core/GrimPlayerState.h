#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GrimPlayerState.generated.h"

class UGrimAbilitySystemComponent;
class UGrimVitalAttributeSet;
class UGrimMagicAttributeSet;
class UGrimSpellbookComponent;
class UGrimProgressionComponent;
class UGrimSurvivalComponent;
class UGrimLevelComponent;
class UGrimSkillTreeComponent;

/**
 * 플레이어 상태. ASC 와 어트리뷰트 · 마법서 · 생존을 가진다 — 폰이 죽거나 바뀌어도(탑승) 유지된다.
 * 폰은 PossessedBy(서버) · OnRep_PlayerState(클라이언트)에서 InitAbilityActorInfoForPawn 을 불러야 한다.
 */
UCLASS()
class GRIMOIRE_API AGrimPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGrimPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UGrimAbilitySystemComponent* GetGrimAbilitySystemComponent() const { return AbilitySystemComponent; }
	UGrimVitalAttributeSet* GetVitalSet() const { return VitalSet; }
	UGrimMagicAttributeSet* GetMagicSet() const { return MagicSet; }
	UGrimSpellbookComponent* GetSpellbook() const { return Spellbook; }
	UGrimProgressionComponent* GetProgression() const { return Progression; }
	UGrimSurvivalComponent* GetSurvival() const { return Survival; }
	UGrimLevelComponent* GetCharacterLevel() const { return CharacterLevel; }
	UGrimSkillTreeComponent* GetSkillTree() const { return SkillTree; }

	/** ASC 의 Owner = 이 PlayerState, Avatar = 폰. 서버 · 클라이언트 양쪽에서 호출한다. */
	void InitAbilityActorInfoForPawn(APawn* NewPawn);

	/** ASC 초기화 직후(서버 · 클라이언트 모두). 마법서 · 생존 컴포넌트가 여기서 시작한다. */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGrimAbilityActorInfoReady, AGrimPlayerState* /*PlayerState*/, APawn* /*Pawn*/);
	FOnGrimAbilityActorInfoReady OnAbilityActorInfoReady;

private:
	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Abilities")
	TObjectPtr<UGrimAbilitySystemComponent> AbilitySystemComponent;

	// ASC 는 소유자의 하위 객체인 AttributeSet 을 자동으로 등록한다.
	UPROPERTY()
	TObjectPtr<UGrimVitalAttributeSet> VitalSet;

	UPROPERTY()
	TObjectPtr<UGrimMagicAttributeSet> MagicSet;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Magic")
	TObjectPtr<UGrimSpellbookComponent> Spellbook;

	/** 서클 승급 (R06, M2) */
	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Magic")
	TObjectPtr<UGrimProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Survival")
	TObjectPtr<UGrimSurvivalComponent> Survival;

	/** 캐릭터 레벨 · 성장 트리 (R27, M2 후반) */
	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Progression")
	TObjectPtr<UGrimLevelComponent> CharacterLevel;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Progression")
	TObjectPtr<UGrimSkillTreeComponent> SkillTree;
};
