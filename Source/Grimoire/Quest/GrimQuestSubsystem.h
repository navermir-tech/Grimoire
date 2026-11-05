#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimQuestSubsystem.generated.h"

class APlayerState;
class UGrimQuestComponent;
struct FGrimQuestGive;
struct FGrimQuestStepDef;

/**
 * 퀘스트 월드 서브시스템(서버). PlayerState 마다 UGrimQuestComponent 를 붙이고(Core 를 고치지 않으려고 런타임에),
 * 주기(UGrimQuestSettings::PollIntervalSeconds)마다 조회형 목표를 보게 하고, 밤 건너뛰기(UGrimSleepSubsystem)를 신호로 넘긴다.
 * 단계 시작 · 끝의 세계 쪽 일(시각 옮기기, 시험 소품 되돌리기 · 켜기 · 끄기)도 여기서 한다.
 */
UCLASS()
class GRIMOIRE_API UGrimQuestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 이 맵에서 AutoStart 퀘스트를 시작하나(설정 AutoStartMaps) */
	bool IsAutoStartMap() const;

	/** 서버: PlayerState 에 퀘스트 컴포넌트가 없으면 붙인다 */
	UGrimQuestComponent* EnsureComponent(APlayerState* PlayerState);

	/** 서버: 단계 시작 — 시각을 옮기고, 소품을 처음 상태로 되돌려 켜고, 줄 것(Give)을 플레이어에게 놓는다 */
	void ApplyStepStart(const FGrimQuestStepDef& Step, UGrimQuestComponent* Quest);

	/**
	 * 서버: 줄 것을 놓는다 — 인벤토리(M4) 전이라 월드에: 스킬북(다가가면 읽힌다, 태그 Given_Skillbook_<ID>) · 침낭(태그 Given_Bedroll).
	 * 아이템(Item:<ID>)은 TODO(M4) — 인벤토리가 생기면 거기에 넣는다(지금은 기록만).
	 */
	void GiveItems(const TArray<FGrimQuestGive>& Gives, UGrimQuestComponent* Quest);

	/** 서버: 단계 끝 — 소품(표적)을 끈다 */
	void ApplyStepEnd(const FGrimQuestStepDef& Step);

	/** 서버: 태그의 소품 — bReset 이면 속성 사물을 처음 상태로(다르면 같은 자리에 새로 놓는다), 시험 표적은 켜고 끈다 */
	void SetPropsActive(FName Tag, bool bActive, bool bReset);

	/** 서버: 지금 모든 플레이어의 조회형 목표를 본다(주기 타이머 · 콘솔) */
	void PollNow();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;
	void ResetElementalActor(AActor* Actor);

	UFUNCTION()
	void HandleNightSkipBegin(float FadeSeconds);

	UFUNCTION()
	void HandleNightSkipped(float SkippedDays);

	UFUNCTION()
	void HandleNightSkipCancelled();

	FTimerHandle PollTimer;
	FDelegateHandle ActorSpawnedHandle;
};
