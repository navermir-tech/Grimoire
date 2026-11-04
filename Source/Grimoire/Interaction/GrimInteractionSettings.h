#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimInteractionSettings.generated.h"

class UDataTable;

/**
 * 속성 상호작용 전역 설정. 값은 DefaultGame.ini 의 [/Script/Grimoire.GrimInteractionSettings] 로 덮어쓴다.
 * 반응 수치 자체는 규칙표(CSV)에 있고, 여기는 엔진 쪽 예산 · 상한만 둔다.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Grimoire Interaction"))
class GRIMOIRE_API UGrimInteractionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGrimInteractionSettings();

	/** 규칙표 원본 CSV. 프로젝트 Content 폴더 기준 경로 */
	UPROPERTY(config, EditAnywhere, Category = "Reactions")
	FString ReactionTableCSV = TEXT("Grimoire/Data/ElementalReactions.csv");

	/** 지정하면 CSV 대신 이 DataTable 에셋을 쓴다(나중에 에디터 임포트로 바꿀 때) */
	UPROPERTY(config, EditAnywhere, Category = "Reactions")
	TSoftObjectPtr<UDataTable> ReactionTableAsset;

	/** 시간 제한이 있는 상태를 갱신하는 주기(초). 그런 상태가 없으면 타이머도 없다 */
	UPROPERTY(config, EditAnywhere, Category = "State", meta = (ClampMin = "0.02"))
	float StateUpdateInterval = 0.25f;

	/** 같은 출처가 이웃에 다시 전파하기까지의 간격(초) */
	UPROPERTY(config, EditAnywhere, Category = "Propagation", meta = (ClampMin = "0.05"))
	float PropagationInterval = 1.f;

	/** 규칙표의 전파 반경 상한(cm) */
	UPROPERTY(config, EditAnywhere, Category = "Propagation", meta = (ClampMin = "0"))
	float MaxPropagationRadius = 1000.f;

	/** 한 프레임에 처리하는 전파 오버랩 쿼리 수 상한(예산) */
	UPROPERTY(config, EditAnywhere, Category = "Propagation", meta = (ClampMin = "1"))
	int32 MaxPropagationQueriesPerFrame = 200;

	/** 쿼리 한 번이 자극을 주는 이웃 수 상한 */
	UPROPERTY(config, EditAnywhere, Category = "Propagation", meta = (ClampMin = "1"))
	int32 MaxNeighborsPerQuery = 16;
};
