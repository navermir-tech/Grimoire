#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/** 그리모어 목록에 놓을 항목 하나의 정렬 열쇠 */
struct GRIMOIRE_API FGrimGrimoireEntryKey
{
	/** 배운 순서(마법서의 KnownSpells 번호) */
	int32 KnownIndex = 0;
	int32 Circle = 1;
	/** 숙련도를 쌓는가 — false(기본 공격 등)는 맨 앞 */
	bool bTracked = true;
	FGameplayTag Element;
};

/** HUD 슬롯 배치 결과 */
struct GRIMOIRE_API FGrimSlotLayout
{
	/** 따로 둔 기본 공격 자리의 마법 번호(없으면 INDEX_NONE) */
	int32 BasicIndex = INDEX_NONE;
	/** 보이는 세트의 슬롯마다 마법 번호(빈 칸 INDEX_NONE). 길이 = 세트당 슬롯 수 */
	TArray<int32> Slots;
	/** 보이는 세트 번호(0부터) */
	int32 SetIndex = 0;
	/** 전체 세트 수 */
	int32 Sets = 1;
};

/** HUD · 그리모어 화면의 순수 계산 — 위젯 · 월드를 모른다(테스트 Grimoire.UI.*). */
struct GRIMOIRE_API FGrimUIMath
{
	/** (Value − Min) ÷ (Max − Min), 0~1. 범위가 없으면 0. */
	static float BarFraction(float Value, float Min, float Max);

	/** "Set.Prop" → (Set, Prop) 이면 true, "Prop" 만이면 OutSet 비우고 false. 앞뒤 공백은 뗀다. */
	static bool SplitAttributeName(const FString& Name, FString& OutSet, FString& OutProperty);

	/** 어트리뷰트 셋 클래스 이름 비교: "GrimVitalAttributeSet" 과 "UGrimVitalAttributeSet" 을 같게 본다. */
	static bool AttributeSetNameMatches(const FString& ClassName, const FString& Wanted);

	/** 남은 쿨다운 비율 0~1(남은 ÷ 전체). */
	static float CooldownFraction(float Remaining, float Duration);

	/** 필터가 비면 모두, 아니면 속성이 필터(또는 그 하위)인가. */
	static bool MatchesElementFilter(const FGameplayTag& Element, const FGameplayTag& Filter);

	/** 그리모어 목록 순서: 숙련도 없는 항목(기본 공격) → 서클 오름차순 → 배운 순서. 필터에 맞는 것만. 결과 = 입력 배열 번호. */
	static TArray<int32> BuildGrimoireOrder(TConstArrayView<FGrimGrimoireEntryKey> Entries, const FGameplayTag& Filter);

	/**
	 * 슬롯 배치(호그와트 레거시 — 4칸 세트 여러 개, 수는 설정값). SlotKnownIndices[i] = 슬롯 i 에 놓인 마법의 배운 번호(빈 칸 INDEX_NONE),
	 * bTracked[k] = 배운 마법 k 가 숙련도를 쌓는가. bSeparateBasic 이면 숙련도 없는 첫 항목(기본 공격, R26)을 따로 둔다.
	 * 보이는 세트 = ActiveSet(범위로 자름).
	 */
	static FGrimSlotLayout ComputeSlotLayout(TConstArrayView<int32> SlotKnownIndices, TConstArrayView<bool> bTracked, int32 ActiveSet, int32 SlotsPerSet, int32 Sets, bool bSeparateBasic);

	/** 등급 글자: 숙련도가 없으면 「—」 */
	static FText GradeLabel(bool bTracked, FName Grade);

	/** 쿨다운 숫자: 1초 미만은 소수 한 자리, 그 위는 정수 */
	static FText FormatCooldown(float Seconds);
};
