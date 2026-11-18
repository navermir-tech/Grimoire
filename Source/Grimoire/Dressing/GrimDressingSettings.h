#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimDressingSettings.generated.h"

/**
 * 초원 꾸밈 · 작은 흥미지점 설정(docs/systems/meadow-dressing.md). 값은 DefaultGame.ini 의 [/Script/Grimoire.GrimDressingSettings].
 * 소품 종류 · 보상 · 재생은 표(CSV)에, 여기는 표 경로와 전역 값만.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Grimoire Dressing"))
class GRIMOIRE_API UGrimDressingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 반응 소품 종류(프로젝트 Content 기준) */
	UPROPERTY(config, EditAnywhere, Category = "Tables")
	FString ReactivePropsCSV = TEXT("Grimoire/Data/ReactiveProps.csv");

	/** 채집점 종류 */
	UPROPERTY(config, EditAnywhere, Category = "Tables")
	FString HarvestNodesCSV = TEXT("Grimoire/Data/HarvestNodes.csv");

	/** 도감 줄 */
	UPROPERTY(config, EditAnywhere, Category = "Tables")
	FString JournalCSV = TEXT("Grimoire/Data/Journal.csv");

	/** 수집품(책장)을 F 로 주울 수 있는 거리(cm) */
	UPROPERTY(config, EditAnywhere, Category = "Interact", meta = (ClampMin = "50"))
	float CollectRadius = 220.f;

	/** 서버가 거리를 다시 잴 때 더 봐주는 거리(cm — 지연 · 움직임) */
	UPROPERTY(config, EditAnywhere, Category = "Interact", meta = (ClampMin = "0"))
	float ServerReachSlack = 150.f;

	/** 도감 「쪽(Page)」 묶음이 이만큼 찰 때마다 한 번 보상 — 잠정(Q152: 새 화폐 없이 도감 쪽이 차면 한 번씩). 0 = 없음 */
	UPROPERTY(config, EditAnywhere, Category = "Journal", meta = (ClampMin = "0"))
	int32 PageMilestoneEvery = 5;

	/** 그 보상 아이템(Items.csv). 비우면 알림만 */
	UPROPERTY(config, EditAnywhere, Category = "Journal")
	FName PageMilestoneItem = TEXT("ManagrassSeed");

	UPROPERTY(config, EditAnywhere, Category = "Journal", meta = (ClampMin = "1"))
	int32 PageMilestoneCount = 2;

	/** 바람에 날려 옮겨 갈 때 아래로 바닥을 찾는 깊이(cm) */
	UPROPERTY(config, EditAnywhere, Category = "Motion", meta = (ClampMin = "100"))
	float DriftGroundTraceDepth = 3000.f;
};
