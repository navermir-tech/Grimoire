#pragma once

#include "CoreMinimal.h"
#include "Interaction/GrimElementalReactionTypes.h"

class UDataTable;
struct FGrimElementalStimulus;

DECLARE_LOG_CATEGORY_EXTERN(LogGrimInteraction, Log, All);

/**
 * 반응 규칙 조회 · 적용. 액터 · 월드를 모르는 순수 로직이라 테스트가 직접 부른다.
 * 컴포넌트는 결과(상태 변화 목록)를 받아 복제 · 델리게이트 · 타이머를 처리한다.
 */
struct GRIMOIRE_API FGrimElementalReactionLibrary
{
	/** 설정의 규칙표(에셋 또는 CSV)를 한 번 읽어 둔 것. 읽지 못하면 nullptr */
	static const UDataTable* GetDefaultTable();

	/** 다음 GetDefaultTable 에서 다시 읽는다(콘솔 Grim.Interaction.ReloadReactions) */
	static void ResetDefaultTable();

	/** CSV 파일을 DataTable 로 읽는다. 파싱 문제는 OutProblems 에 */
	static UDataTable* LoadTableFromCSVFile(const FString& FilePath, TArray<FString>& OutProblems);

	/** 자극에 맞는 첫 규칙. 없으면 nullptr */
	static const FGrimElementalReactionRow* FindStimulusRule(const UDataTable* Table, const FGrimElementalStateArray& States,
		const FGameplayTagContainer& Material, const FGrimElementalStimulus& Stimulus);

	/** 시간이 다 돼 사라진 상태에 맞는 첫 Expire 규칙. 없으면 nullptr */
	static const FGrimElementalReactionRow* FindExpireRule(const UDataTable* Table, const FGrimElementalStateArray& States,
		const FGameplayTagContainer& Material, const FGameplayTag& ExpiredState);

	/** 규칙 결과를 상태 배열에 적용한다. MaxPropagationRadius 로 전파 반경을 자른다 */
	static void ApplyRule(const FGrimElementalReactionRow& Rule, FGrimElementalStateArray& States,
		float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges);

	/** 찾기 + 적용. 규칙이 맞았으면 true */
	static bool ApplyStimulus(const UDataTable* Table, FGrimElementalStateArray& States, const FGameplayTagContainer& Material,
		const FGrimElementalStimulus& Stimulus, float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges);

	/** 시간 제한 상태를 DeltaSeconds 만큼 줄이고, 다 된 상태를 지운 뒤 Expire 규칙을 적용한다 */
	static void AdvanceTime(const UDataTable* Table, FGrimElementalStateArray& States, const FGameplayTagContainer& Material,
		float DeltaSeconds, float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges);
};
