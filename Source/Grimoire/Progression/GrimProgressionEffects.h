#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Progression/GrimProgressionTypes.h"
#include "GrimProgressionEffects.generated.h"

class UAbilitySystemComponent;

/**
 * 성장 보너스 — 무한 지속. 어트리뷰트마다 SetByCaller(SetByCaller.Progression.*) 수정자가 하나씩 있다:
 * MaxHealth · MaxMana · HeatResist · ColdResist · DiseaseResist 는 더하기(AddBase), MoveSpeedScale · CastSpeedScale · ManaCostScale 는 곱하기(MultiplyCompound).
 * 레벨(UGrimLevelComponent)과 성장 트리(UGrimSkillTreeComponent)가 각자 하나씩 건다 — 어트리뷰트 셋 파일은 고치지 않는다.
 * 노드 태그(Progression.Node.*)는 Spec 의 DynamicGrantedTags 로 붙는다.
 */
UCLASS()
class GRIMOIRE_API UGrimGE_ProgressionBonus : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimGE_ProgressionBonus();
};

/** GE 하나에 담는 값. 기본값 = 아무 변화 없음. */
struct GRIMOIRE_API FGrimProgressionBonus
{
	float MaxHealth = 0.f;
	float MaxMana = 0.f;
	float HeatResist = 0.f;
	float ColdResist = 0.f;
	float DiseaseResist = 0.f;
	float MoveSpeedScale = 1.f;
	float CastSpeedScale = 1.f;
	float ManaCostScale = 1.f;
	/** 어트리뷰트가 없는 효과의 노드 태그 */
	FGameplayTagContainer GrantedTags;

	bool IsNeutral() const;
};

/** 성장 효과 ↔ 어트리뷰트 · GE */
struct GRIMOIRE_API FGrimProgressionEffects
{
	/** 효과 키가 GE 로 가는 어트리뷰트 이름인가(MaxHealth …) */
	static bool IsAttributeKey(FName Key);

	/**
	 * 효과 합 → GE 값. 더하기 어트리뷰트는 Add(곱하기 조각은 무시), 배율 어트리뷰트는 Mul × (1 + Add).
	 * 나머지 키는 태그(Progression.Node.<키>)로 — 등록되지 않은 키는 OutUnknownKeys 에.
	 */
	static FGrimProgressionBonus MakeBonus(const TMap<FName, FGrimSkillEffectTotal>& Totals, TArray<FName>* OutUnknownKeys = nullptr);

	/**
	 * 서버: Bonus 를 GE 로 건다. 새 것을 먼저 걸고 옛 것(InOutHandle)을 지운다 — 최대 HP · 마나가 잠깐이라도 줄어 현재 값이 깎이지 않게.
	 * Bonus 가 중립이면 옛 것만 지운다. ASC 가 아직 준비되지 않았으면 false(나중에 다시 부른다).
	 */
	static bool ApplyBonus(UAbilitySystemComponent* ASC, const FGrimProgressionBonus& Bonus, FActiveGameplayEffectHandle& InOutHandle);
};
