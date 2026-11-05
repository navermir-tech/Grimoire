#include "Core/GrimPlayerState.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimProgressionComponent.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Survival/GrimSurvivalComponent.h"
#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimSkillTreeComponent.h"

AGrimPlayerState::AGrimPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGrimAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Mixed: GE 는 소유 클라이언트에게만 전부 복제, 태그 · 큐는 모두에게. 플레이어용 권장값.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	VitalSet = CreateDefaultSubobject<UGrimVitalAttributeSet>(TEXT("VitalSet"));
	MagicSet = CreateDefaultSubobject<UGrimMagicAttributeSet>(TEXT("MagicSet"));

	Spellbook = CreateDefaultSubobject<UGrimSpellbookComponent>(TEXT("Spellbook"));
	Progression = CreateDefaultSubobject<UGrimProgressionComponent>(TEXT("Progression"));
	Survival = CreateDefaultSubobject<UGrimSurvivalComponent>(TEXT("Survival"));
	CharacterLevel = CreateDefaultSubobject<UGrimLevelComponent>(TEXT("CharacterLevel"));
	SkillTree = CreateDefaultSubobject<UGrimSkillTreeComponent>(TEXT("SkillTree"));

	// ASC 를 가진 PlayerState 는 기본값(1Hz)보다 자주 갱신해야 시전 · 수치가 늦지 않다.
	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* AGrimPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGrimPlayerState::InitAbilityActorInfoForPawn(APawn* NewPawn)
{
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, NewPawn);
	OnAbilityActorInfoReady.Broadcast(this, NewPawn);
}
