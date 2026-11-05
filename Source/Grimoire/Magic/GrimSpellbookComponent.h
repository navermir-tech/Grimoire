#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Magic/GrimProficiencyTypes.h"
#include "GrimSpellbookComponent.generated.h"

class AGrimPlayerState;
class AGrimSkillbook;
class AGrimWand;
class UGrimSpellDefinition;
class UGrimSpellbookComponent;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGrimOnSelectedSpellChanged, UGrimSpellbookComponent*, Spellbook, const UGrimSpellDefinition*, SelectedSpell);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnProficiencyGradeUp, UGrimSpellbookComponent*, Spellbook, const UGrimSpellDefinition*, Spell, int32, GradeIndex);

/** 스킬북 사용 · 배우기 결과 */
UENUM(BlueprintType)
enum class EGrimLearnResult : uint8
{
	/** 새로 배웠다(책 소모) */
	Learned,
	/** 이미 배운 마법 — 숙련도 경험을 받았다(책 소모) */
	ProficiencyGained,
	/** 이미 S(마스터) — 쓸 수 없다(책 남음) */
	AlreadyMastered,
	/** 이미 알고 숙련도도 없는 항목(기본 공격 등) — 쓸 수 없다 */
	AlreadyKnown,
	CircleTooLow,
	TooFar,
	Invalid
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnLearnResult, UGrimSpellbookComponent*, Spellbook, const UGrimSpellDefinition*, Spell, EGrimLearnResult, Result);

/** 마법 하나의 숙련도 경험 (Q26). 등급은 저장하지 않고 XP 와 표로 매번 계산한다. */
USTRUCT()
struct GRIMOIRE_API FGrimSpellProficiency : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGrimSpellDefinition> Spell;

	UPROPERTY()
	float XP = 0.f;

	/** 이 기계에서 마지막으로 본 등급(복제 안 함) — 클라이언트 등급 오름 알림용 */
	int32 LastGradeIndex = INDEX_NONE;

	void PostReplicatedAdd(const struct FGrimSpellProficiencyArray& InArraySerializer);
	void PostReplicatedChange(const struct FGrimSpellProficiencyArray& InArraySerializer);
};

USTRUCT()
struct GRIMOIRE_API FGrimSpellProficiencyArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGrimSpellProficiency> Items;

	/** 복제 콜백이 알림을 보낼 곳(복제 안 함) */
	UGrimSpellbookComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGrimSpellProficiency, FGrimSpellProficiencyArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGrimSpellProficiencyArray> : public TStructOpsTypeTraitsBase2<FGrimSpellProficiencyArray>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * 플레이어의 마법서(그리모어) — 배운 마법 · 선택 슬롯 · 완드 장착 · 숙련도 (R01 · R05 · Q26).
 * AGrimPlayerState 의 기본 하위 객체. OnAbilityActorInfoReady 에서 서버가:
 *  - 처음 한 번: 서클 · 마나 초기값, 마나 회복 GE, 다음/이전 선택 어빌리티, 시작 마법(UGrimMagicSettings::StartingSpells)을 준다.
 *  - 폰이 바뀔 때마다: 완드를 폰 메시의 손 소켓에 붙인다.
 * 선택된 마법의 Spec 만 Input.Spell.Cast 를 동적 태그로 가진다 → 시전 입력이 그 마법만 발동한다.
 * 숙련도 경험은 서버가 쌓고(시전 확정 · 의미 있는 명중), 소유 클라이언트에게만 복제한다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSpellbookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSpellbookComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** ASC(플레이어는 PlayerState 소유)에서 마법서를 찾는다. 없으면 nullptr. */
	static UGrimSpellbookComponent* FindForASC(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	AGrimWand* GetWand() const { return Wand; }

	const TArray<TObjectPtr<UGrimSpellDefinition>>& GetKnownSpells() const { return KnownSpells; }

	bool KnowsSpell(const UGrimSpellDefinition* Spell) const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	UGrimSpellDefinition* GetSelectedSpell() const;

	/** 서버: 마법을 배운다(서클 확인 → GiveAbility). 이미 알거나 서클이 모자라면 false. */
	bool LearnSpell(UGrimSpellDefinition* Spell);

	/** 배울 수 있는지(서클 · 이미 앎). 서버 · 클라이언트 모두. 배울 수 있으면 Learned. */
	EGrimLearnResult CanLearnSpell(const UGrimSpellDefinition* Spell) const;

	/** 이 마법의 스킬북을 쓰면 어떻게 되나(배움 · 숙련도 경험 · 못 씀). 서버 · 소유 클라이언트(숙련도 복제). */
	EGrimLearnResult PreviewSkillbook(const UGrimSpellDefinition* Spell) const;

	/**
	 * 서버: 스킬북 「사용」(R05) — 아이템과 무관한 한 함수. 인벤토리(M4)가 생기면 아이템 사용이 이것을 부른다.
	 *  - 모르는 마법 → 배운다(서클 확인).  - 아는 마법 → 숙련도 경험(UGrimSpellbookSettings::SkillbookXPFractionOfGrade · SkillbookFlatXP).
	 *  - 이미 S → 쓸 수 없다. 결과가 Learned · ProficiencyGained 면 책을 소모한다(부른 쪽이).
	 * 결과는 OnLearnResult 로(소유 클라이언트에도) 알린다.
	 */
	EGrimLearnResult UseSkillbook(UGrimSpellDefinition* Spell);

	/** 서버: 선택을 Step 만큼 옮긴다(끝에서 돈다). */
	void CycleSelection(int32 Step);

	/** 서버: Index 번째 마법을 선택한다. */
	void SelectSpell(int32 Index);

	/** 클라이언트 → 서버: Index 번째 마법을 선택(그리모어 화면에서 고르기). */
	UFUNCTION(Server, Reliable)
	void Server_SelectSpell(int32 Index);

	// ───── 스킬북 (R05) ─────

	/** 서버: 월드의 스킬북을 집는다(겹침 · 콘솔) — 곁에 있으면 UseSkillbook, 쓰였으면 책이 사라진다. */
	EGrimLearnResult PickUpSkillbook(AGrimSkillbook* Book, const APawn* Reader);

	/** 클라이언트 → 서버: 집기(콘솔 Grim.Magic.ReadSkillbook) */
	UFUNCTION(Server, Reliable)
	void Server_PickUpSkillbook(AGrimSkillbook* Book);

	/** 어디서든(호스트 · 클라이언트) 집기를 요청한다 — 서버면 바로, 아니면 RPC. */
	void RequestPickUpSkillbook(AGrimSkillbook* Book);

	// ───── 단축 슬롯 (호그와트 레거시: 4칸 세트 여러 개, UGrimSpellbookSettings) ─────

	/** 슬롯마다 배정된 마법(세트 0 의 0~3, 세트 1 의 0~3 …). 빈 칸 nullptr. 모두에게 복제. */
	const TArray<TObjectPtr<UGrimSpellDefinition>>& GetSlotSpells() const { return SlotSpells; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Spellbook")
	int32 GetActiveSlotSet() const { return ActiveSlotSet; }

	/** 서버: 슬롯에 마법을 놓는다(nullptr = 비우기). 같은 마법이 다른 칸에 있으면 그 칸은 비운다. */
	void AssignSlot(int32 SlotIndex, UGrimSpellDefinition* Spell);

	/** 서버: 보이는 세트를 바꾼다 */
	void SetActiveSlotSet(int32 SetIndex);

	UFUNCTION(Server, Reliable)
	void Server_AssignSlot(int32 SlotIndex, UGrimSpellDefinition* Spell);

	UFUNCTION(Server, Reliable)
	void Server_SetActiveSlotSet(int32 SetIndex);

	/** 배우기 결과(서버 · 소유 클라이언트). HUD 가 알림을 띄운다. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Spellbook")
	FGrimOnLearnResult OnLearnResult;

	// ───── 숙련도 (Q26) ─────

	/** 마법의 숙련도(등급 · 막대 · 배율). 숙련도를 쌓지 않는 마법이면 bTracked = false. 서버 · 소유 클라이언트. */
	FGrimProficiencyView GetProficiency(const UGrimSpellDefinition* Spell) const;

	/** 마법의 마나 · 쿨다운 배율(숙련도). 모르는 마법 · 숙련도 없음이면 1. */
	FGrimProficiencyMultipliers GetProficiencyMultipliers(const UGrimSpellDefinition* Spell) const;

	/** 마법별 추가 효과 값(ProficiencyExtraEffect.MaxValue × 등급 배율). 없으면 0. */
	float GetProficiencyExtraEffect(const UGrimSpellDefinition* Spell) const;

	float GetProficiencyXP(const UGrimSpellDefinition* Spell) const;

	/** 배운 마법(숙련도를 쌓는 것만)의 등급 합 · 수 — 서클 승급 조건. */
	int32 GetProficiencyGradeSum() const;
	int32 GetTrackedSpellCount() const;

	/** 서버: 시전 확정 — XPPerCast(같은 대상 반복 감쇠). */
	void AwardCastXP(const UGrimSpellDefinition* Spell, const AActor* Target);

	/** 서버: 의미 있는 명중 — XPPerMeaningfulHit(같은 대상 반복 감쇠). */
	void AwardHitXP(const UGrimSpellDefinition* Spell, const AActor* Target);

	/** 서버: 경험을 그대로 더한다(감쇠 · 배율 없음). 등급이 오르면 알림. */
	void AddProficiencyXP(const UGrimSpellDefinition* Spell, float Amount);

	/** 개발용: 클라이언트 콘솔(Grim.Magic.AddXP)의 서버 경로. 출시 빌드에서는 아무것도 안 한다. */
	UFUNCTION(Server, Reliable)
	void Server_DebugAddXP(UGrimSpellDefinition* Spell, float Amount);

	/**
	 * 이 명중이 숙련도에 「의미 있는」가 — 적 · 생물(ASC · 조준 종류), 마법 자극에 반응하는 사물.
	 * 자극을 주기 전에 불러야 한다(반응 여부를 규칙표로 미리 본다). 협동 플레이어는 아니다.
	 */
	static bool IsMeaningfulHit(const UGrimSpellDefinition* Spell, const UAbilitySystemComponent* SourceASC, const AActor* HitActor);

	/** 등급이 올랐다(서버: 계산 직후, 소유 클라이언트: 복제 콜백). */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Spellbook")
	FGrimOnProficiencyGradeUp OnProficiencyGradeUp;

	/** 선택이 바뀌거나 배운 마법이 늘었다(서버 · 클라이언트). HUD 가 듣는다. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Spellbook")
	FGrimOnSelectedSpellChanged OnSelectedSpellChanged;

	/** 복제 콜백에서 */
	void HandleProficiencyReplicated(FGrimSpellProficiency& Item, bool bAdded);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);

	/** 서버: 처음 한 번 — 어트리뷰트 초기값 · 회복 · 선택 어빌리티 · 시작 마법. */
	void InitializeSpellbook(UAbilitySystemComponent* ASC);

	/** 서버: 완드를 (없으면 만들어) 폰에 붙인다. */
	void EquipWand(APawn* Pawn);

	UAbilitySystemComponent* GetASC() const;

	FGrimSpellProficiency* FindProficiencyItem(const UGrimSpellDefinition* Spell);
	const FGrimSpellProficiency* FindProficiencyItem(const UGrimSpellDefinition* Spell) const;

	/** 서버: 반복 감쇠를 적용해 쌓는다. */
	void AwardXP(const UGrimSpellDefinition* Spell, const AActor* Target, float BaseXP, uint8 Kind);

	UFUNCTION(Client, Reliable)
	void Client_LearnResult(UGrimSpellDefinition* Spell, EGrimLearnResult Result);

	UFUNCTION()
	void OnRep_Selection();

	UPROPERTY(Replicated)
	TArray<TObjectPtr<UGrimSpellDefinition>> SlotSpells;

	UPROPERTY(Replicated)
	int32 ActiveSlotSet = 0;

	/** 결과를 서버 · 소유 클라이언트에 알린다 */
	void NotifyLearnResult(UGrimSpellDefinition* Spell, EGrimLearnResult Result);

	/** 배운 순서대로. 정의 에셋은 복제 가능한 안정된 참조다. */
	UPROPERTY(ReplicatedUsing = OnRep_Selection)
	TArray<TObjectPtr<UGrimSpellDefinition>> KnownSpells;

	UPROPERTY(ReplicatedUsing = OnRep_Selection)
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	TObjectPtr<AGrimWand> Wand;

	/** 마법별 숙련도 경험 — 소유 클라이언트에게만 복제(플레이어 세이브 대상). */
	UPROPERTY(Replicated)
	FGrimSpellProficiencyArray Proficiency;

	/** 서버 전용: KnownSpells 와 같은 순서의 어빌리티 핸들. */
	TArray<FGameplayAbilitySpecHandle> SpellHandles;

	/** 서버 전용: 같은 (마법, 대상) 반복 기록 */
	FGrimRepeatTracker RepeatTracker;

	bool bInitialized = false;

	FDelegateHandle ActorInfoReadyHandle;
};
