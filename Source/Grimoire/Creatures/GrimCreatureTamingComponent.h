#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/GrimTamingTypes.h"
#include "GrimCreatureTamingComponent.generated.h"

class AGrimCreature;
class APawn;
class APlayerState;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimTamingChangedNative, class UGrimCreatureTamingComponent* /*Taming*/);

/**
 * 생물 하나의 길들이기 · 충성도 상태(R02 · 스펙 「충성도」). AGrimCreature 의 기본 하위 객체 — 야생 생물에게도 있다(먹이 기록 · 지침).
 *  - 주인 · 충성도 · 이름은 개체 기록(FGrimCreatureInstance, 생물이 복제)에, 명령 · 최근 까닭 · 기절은 여기서 복제한다.
 *  - 충성도 단계 태그 Creature.Loyalty.* 와 Creature.State.Tamed 는 각 기기가 복제된 값으로 붙인다(루즈 태그는 복제되지 않는다).
 *  - 서버: TickSeconds 마다 고단함(지침) · 굶김 · 쉼 · 다친 채 · 혹사 · 환경 · 함께 지낸 날 · 주인 부재 · 탐험 · 다툼을 센다.
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimCreatureTamingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimCreatureTamingComponent();

	AGrimCreature* GetCreature() const;

	bool IsTamed() const;
	float GetLoyalty() const;
	EGrimLoyaltyStage GetStage() const;
	EGrimTamedCommand GetCommand() const { return Command; }
	const TArray<FGrimLoyaltyReason>& GetRecentReasons() const { return RecentReasons; }
	bool IsKnockedOut() const { return bKnockedOut; }
	APlayerState* GetOwnerPlayerState() const { return OwnerPlayerState; }
	/** 주인의 폰(접속 중이면) */
	APawn* GetOwnerPawn() const;
	/** 화면 이름: 이름이 있으면 그것, 없으면 종 이름 */
	FString GetDisplayName() const;
	float GetExertion() const { return Exertion; }

	// ───── 서버 ─────

	/** 길들인다(주인 · 충성도 시작 값 · 까닭 「계약」 · 개체군에서 빼기는 부르는 쪽). */
	void Claim(APlayerState* NewOwner, const FString& OwnerPlayerId);

	/** 세이브 기록으로 되돌린다(접속 때 다시 부르기). OfflineSeconds 만큼 주인 부재를 센다. */
	void RestoreOwned(APlayerState* NewOwner, const FGrimOwnedCreatureRecord& Record, double OfflineSeconds);

	/** 지금 상태를 기록으로 */
	FGrimOwnedCreatureRecord MakeRecord() const;

	/** 충성도 사건 하나. 0 이 되면 떠난다. 실제 바뀐 값 */
	float ApplyLoyaltyEvent(EGrimLoyaltyEvent Event, float Scale = 1.f);

	/** 개발용: 충성도를 정한다(까닭 없음) */
	void DebugSetLoyalty(float NewLoyalty);

	void SetCommand(EGrimTamedCommand NewCommand);
	void SetNickname(const FString& NewName);

	/** 놓아주기 · 떠남: 야생으로(주인 · 충성도 · 이름을 지우고 그 자리를 집으로). 주인 가방의 기록은 부르는 쪽(서브시스템)이 지운다 */
	void ReleaseToWild(bool bLeftByLoyalty);

	/** 먹이를 받았다(야생이면 포획 확률 기록, 길들였으면 충성도) */
	void RecordFed(APlayerState* Feeder, bool bFavorite);

	/** Feeder 가 FedMemorySeconds 안에 먹이를 줬나 */
	bool WasFedBy(const APlayerState* Feeder, bool& bOutFavorite) const;

	/** 체력이 0 이 됐다 — 길들인 생물이면 죽지 않고 쓰러진다(true). 야생이면 false(죽음) */
	bool HandleKnockedOut();

	/** 함께 싸워 이김(서브시스템이 다른 생물의 죽음에서) */
	void NotifyWonTogether();

	/** 계약에 저항(실패) — EnrageSeconds 동안 화나 Caster 에게 덤빈다 */
	void Enrage(AActor* Caster);

	bool IsEnraged() const;
	bool IsExhausted() const;

	/** 상태가 바뀌었다(서버 · 클라이언트) — 메뉴가 듣는다 */
	FGrimTamingChangedNative OnTamingChanged;

	/** 생물이 개체 기록 복제를 받았다(AGrimCreature::OnRep_Instance) */
	void HandleInstanceReplicated();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_TamingState();

private:
	void ServerTick();
	void RefreshLocalTags();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void Revive();
	void EndEnrage();
	void SetLoyaltyServer(float NewLoyalty);
	void MarkChanged();

	UPROPERTY(ReplicatedUsing = OnRep_TamingState)
	EGrimTamedCommand Command = EGrimTamedCommand::Follow;

	UPROPERTY(ReplicatedUsing = OnRep_TamingState)
	TArray<FGrimLoyaltyReason> RecentReasons;

	UPROPERTY(ReplicatedUsing = OnRep_TamingState)
	TObjectPtr<APlayerState> OwnerPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_TamingState)
	bool bKnockedOut = false;

	// 서버 상태
	FTimerHandle TickTimer;
	FTimerHandle ReviveTimer;
	FTimerHandle EnrageTimer;
	FDelegateHandle HealthHandle;
	double LastTickTime = 0.0;
	float Exertion = 0.f;
	TWeakObjectPtr<APlayerState> LastFeeder;
	double LastFedTime = -1.0e9;
	bool bLastFedFavorite = false;
	double LastStarveEventTime = -1.0e9;
	float RestAccum = 0.f;
	float InjuredAccum = 0.f;
	float OverworkAccum = 0.f;
	float GoodEnvAccum = 0.f;
	float BadEnvAccum = 0.f;
	float DayAccum = 0.f;
	float AbsentAccum = 0.f;
	bool bAbsentFired = false;
	double LastQuarrelTime = -1.0e9;
	FVector LastExplorePoint = FVector::ZeroVector;
	bool bHasExplorePoint = false;
	bool bEnraged = false;
	bool bLocalTamedTag = false;
	FGameplayTag LocalStageTag;
};
