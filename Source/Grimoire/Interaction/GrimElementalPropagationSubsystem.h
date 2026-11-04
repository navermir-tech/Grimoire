#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimElementalPropagationSubsystem.generated.h"

class UGrimElementalStateComponent;

/**
 * 상태 전파(불이 옆으로 번짐) 스케줄러. 서버 게임 월드에만 있다.
 * 전파 상태를 가진 컴포넌트가 등록하고, 출처마다 PropagationInterval 마다 한 번씩,
 * 프레임당 MaxPropagationQueriesPerFrame 개까지만 오버랩 쿼리를 돌린다. 출처가 없으면 틱하지 않는다.
 */
UCLASS()
class GRIMOIRE_API UGrimElementalPropagationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterSource(UGrimElementalStateComponent* Component);
	void UnregisterSource(UGrimElementalStateComponent* Component);
	int32 GetNumSources() const { return Sources.Num(); }

	/** CurrentTime 기준으로 때가 된 출처를 예산 안에서 처리한다. 쓴 쿼리 수를 돌려준다(테스트가 직접 부른다) */
	int32 ProcessDueSources(double CurrentTime);

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return Sources.Num() > 0; }
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FSource
	{
		TWeakObjectPtr<UGrimElementalStateComponent> Component;
		double NextTime = 0.0;
	};

	TArray<FSource> Sources;

	/** 예산이 모자라 멈춘 자리. 다음 프레임은 여기서 이어 간다(뒤쪽 출처가 굶지 않게) */
	int32 Cursor = 0;

	bool bProcessing = false;

	double GetNow() const;
};
