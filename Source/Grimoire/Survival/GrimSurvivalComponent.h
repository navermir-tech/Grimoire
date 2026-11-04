#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrimSurvivalComponent.generated.h"

class AGrimPlayerState;
class APawn;
class APlayerController;
class UAbilitySystemComponent;
class UCanvas;
struct FGrimSurvivalStatus;

/**
 * 생존 관리 (R03) — 초기 어트리뷰트, 주기 효과(배고픔 · HP 회복 · 체온), 임계값 상태 태그.
 * AGrimPlayerState 의 기본 하위 객체. OnAbilityActorInfoReady 에서 서버가 시작한다.
 * 계산은 FGrimSurvivalModel, 수치는 UGrimSurvivalSettings.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSurvivalComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 자기 폰 위치의 주변 기온(섭씨). 서버 · 클라이언트 어디서나. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	float GetAmbientTemperature() const;

	/** 마지막 틱의 열원 합(체감 ℃). 서버에서만 의미가 있다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	float GetLastHeatWarmth() const { return LastHeatWarmth; }

private:
	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);
	void InitializeAttributes(UAbilitySystemComponent* ASC);
	void SurvivalTick();
	float GatherHeatWarmth(const APawn* Pawn) const;
	void UpdateStatusTags(UAbilitySystemComponent* ASC, const FGrimSurvivalStatus& Status);
	void DrawDebug(UCanvas* Canvas, APlayerController* PlayerController);

	UAbilitySystemComponent* GetASC() const;
	APawn* GetAvatarPawn() const;

	FTimerHandle SurvivalTickTimer;
	FDelegateHandle ActorInfoReadyHandle;
	FDelegateHandle DebugDrawHandle;

	bool bAttributesInitialized = false;
	double LastTickWorldSeconds = 0.0;
	float LastHeatWarmth = 0.f;
};
