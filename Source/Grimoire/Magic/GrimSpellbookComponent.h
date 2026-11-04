#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GrimSpellbookComponent.generated.h"

class AGrimPlayerState;
class AGrimWand;
class UGrimSpellDefinition;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGrimOnSelectedSpellChanged, UGrimSpellbookComponent*, Spellbook, const UGrimSpellDefinition*, SelectedSpell);

/**
 * 플레이어의 마법서(그리모어) — 배운 마법 · 선택 슬롯 · 완드 장착 (R01 · R05).
 * AGrimPlayerState 의 기본 하위 객체. OnAbilityActorInfoReady 에서 서버가:
 *  - 처음 한 번: 서클 · 마나 초기값, 마나 회복 GE, 다음/이전 선택 어빌리티, 시작 마법(UGrimMagicSettings::StartingSpells)을 준다.
 *  - 폰이 바뀔 때마다: 완드를 폰 메시의 손 소켓에 붙인다.
 * 선택된 마법의 Spec 만 Input.Spell.Cast 를 동적 태그로 가진다 → 시전 입력이 그 마법만 발동한다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSpellbookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSpellbookComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	AGrimWand* GetWand() const { return Wand; }

	const TArray<TObjectPtr<UGrimSpellDefinition>>& GetKnownSpells() const { return KnownSpells; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	UGrimSpellDefinition* GetSelectedSpell() const;

	/** 서버: 마법을 배운다(서클 확인 → GiveAbility). 이미 알거나 서클이 모자라면 false. */
	bool LearnSpell(UGrimSpellDefinition* Spell);

	/** 서버: 선택을 Step 만큼 옮긴다(끝에서 돈다). */
	void CycleSelection(int32 Step);

	/** 서버: Index 번째 마법을 선택한다. */
	void SelectSpell(int32 Index);

	/** 선택이 바뀌거나 배운 마법이 늘었다(서버 · 클라이언트). HUD 가 듣는다. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Spellbook")
	FGrimOnSelectedSpellChanged OnSelectedSpellChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);

	/** 서버: 처음 한 번 — 어트리뷰트 초기값 · 회복 · 선택 어빌리티 · 시작 마법. */
	void InitializeSpellbook(UAbilitySystemComponent* ASC);

	/** 서버: 완드를 (없으면 만들어) 폰에 붙인다. */
	void EquipWand(APawn* Pawn);

	UAbilitySystemComponent* GetASC() const;

	UFUNCTION()
	void OnRep_Selection();

	/** 배운 순서대로. 정의 에셋은 복제 가능한 안정된 참조다. */
	UPROPERTY(ReplicatedUsing = OnRep_Selection)
	TArray<TObjectPtr<UGrimSpellDefinition>> KnownSpells;

	UPROPERTY(ReplicatedUsing = OnRep_Selection)
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	TObjectPtr<AGrimWand> Wand;

	/** 서버 전용: KnownSpells 와 같은 순서의 어빌리티 핸들. */
	TArray<FGameplayAbilitySpecHandle> SpellHandles;

	bool bInitialized = false;

	FDelegateHandle ActorInfoReadyHandle;
};
