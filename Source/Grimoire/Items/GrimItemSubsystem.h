#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimItemSubsystem.generated.h"

class APlayerState;
class UGrimInventoryComponent;

/**
 * 아이템 월드 서브시스템(서버). PlayerState 가 생기자마자 가방(UGrimInventoryComponent)을 붙인다 —
 * PlayerState 파일(Core)을 고치지 않으려고 런타임에(퀘스트 컴포넌트와 같은 방식). 세이브가 플레이어 몫을 읽기 전에 붙어 있어야
 * 가방도 되돌려진다(PlayerState 스폰 콜백에서 붙이므로 PostLogin 의 세이브 읽기보다 먼저).
 */
UCLASS()
class GRIMOIRE_API UGrimItemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 서버: PlayerState 에 가방이 없으면 붙인다 */
	UGrimInventoryComponent* EnsureInventory(APlayerState* PlayerState);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;

	FDelegateHandle ActorSpawnedHandle;
};
