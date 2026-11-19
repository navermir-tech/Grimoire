#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimDressingSubsystem.generated.h"

class APlayerState;
class UGrimJournalComponent;

/**
 * 초원 꾸밈 월드 서브시스템(서버). PlayerState 가 생기자마자 도감(UGrimJournalComponent)을 붙인다 —
 * 가방(UGrimItemSubsystem)과 같은 방식: PlayerState 파일(Core)을 고치지 않고, 세이브가 플레이어 몫을 읽기 전에 붙는다.
 */
UCLASS()
class GRIMOIRE_API UGrimDressingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 서버: PlayerState 에 도감이 없으면 붙인다 */
	UGrimJournalComponent* EnsureJournal(APlayerState* PlayerState);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	FDelegateHandle ActorSpawnedHandle;
};
