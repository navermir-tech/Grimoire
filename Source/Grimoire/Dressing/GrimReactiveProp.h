#pragma once

#include "CoreMinimal.h"
#include "Interaction/GrimElementalObject.h"
#include "Core/GrimInteractable.h"
#include "Core/GrimSaveTypes.h"
#include "GrimReactiveProp.generated.h"

class UBoxComponent;
class UNiagaraComponent;
struct FGrimReactivePropRow;

/**
 * 반응 소품 공통 액터 (docs/systems/meadow-dressing.md) — 초원 꾸밈 · 작은 흥미지점 하나.
 * 소품마다 클래스를 만들지 않는다: PropId 로 ReactiveProps.csv 한 행을 골라 생김새 · 보상 · 되돌리기를 정하고,
 * 불 · 물 · 바람 · 충격 반응은 속성 상태 단일 시스템(UGrimElementalStateComponent + ElementalReactions.csv)이 한다.
 *
 * - 서버 권위: 상태 · 보상 · 옮겨 가기 · 수집 기록은 서버만 바꾼다. 상태는 FastArray 로, 수집 기록 · 위치는 액터 복제로 간다.
 * - 평소 휴면(DORM_DormantAll), 틱 없음 — 튀어 오르는 연출(KickState) 동안만 틱을 켠다.
 * - 수집품(JournalId)은 F 로 사람마다 한 번 줍는다(IGrimInteractable) — 주운 사람에게만 사라진다.
 * - 세이브: 상태는 상태 컴포넌트가(지속 상태만), 수집 기록 · 날려 간 자리는 이 액터가(World 「Dressing.Prop.<이름>」 v1).
 */
UCLASS()
class GRIMOIRE_API AGrimReactiveProp : public AGrimElementalObject, public IGrimInteractable, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimReactiveProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FVector GetTargetPoint() const override;
	virtual bool CanBeTargetedBy(const AActor* Requester) const override;

	// IGrimInteractable
	virtual bool GetInteractionOffer(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const override;
	virtual bool Interact(APawn* Pawn, int32 Token) override;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: TArray<FString> 주운 사람, bool 날려 감, FTransform 자리 */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 서버 스폰 전에(SpawnActorDeferred) 또는 에디터에서 고른다 */
	void SetPropId(FName NewPropId);
	FName GetPropId() const { return PropId; }

	void SetJournalOverride(FName NewJournalId) { JournalOverride = NewJournalId; }
	/** 이 소품이 주는 도감 줄(덮어쓰기 → 표). 없으면 수집품이 아니다 */
	FName GetJournalId() const;

	const FGrimReactivePropRow* GetRow() const;

	/** 처음 상태(표의 InitialState)와 같은가 */
	bool IsAtInitialState() const;

	/** 서버: 처음 상태로 되돌린다(아침 · 시간 · 콘솔). 수집 기록 · 자리는 그대로 */
	void ResetToInitial();

	/** 서버: 해 뜸(UGrimTimeOfDaySubsystem::OnNightEnd)에 불린다. 테스트도 직접 부른다 */
	UFUNCTION()
	void HandleDawn();

	/** 이 플레이어(서버 PlayerId)가 이미 주웠나 */
	bool HasCollected(const FString& PlayerKey) const { return CollectedBy.Contains(PlayerKey); }
	const TArray<FString>& GetCollectedBy() const { return CollectedBy; }

	/** 이 화면의 로컬 플레이어가 주웠나(그 사람에게만 사라진다) */
	bool IsCollectedByLocalPlayer() const;

	/** 지금 보이는 모습이 바뀐 모습(AltState)인가 — 테스트 · 덤프 */
	bool IsShowingAlt() const { return bShowingAlt; }

	/** 보상을 몇 번 놓았나(서버, 테스트) */
	int32 GetRewardsSpawned() const { return RewardsSpawned; }

	/** 서버: 보상 굴림에 쓸 난수 시드 고정(테스트) */
	void SetRewardSeed(int32 Seed) { RewardStream.Initialize(Seed); }

protected:
	/** ReactiveProps.csv 행 이름 */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_PropId, Category = "Dressing", meta = (GetOptions = "GetPropIdOptions"))
	FName PropId;

	/** 도감 줄(Journal.csv) — 비우면 표 행의 JournalId. 책장마다 다른 쪽을 주려고 배치 스크립트가 적는다 */
	UPROPERTY(EditAnywhere, Replicated, Category = "Dressing")
	FName JournalOverride;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> HitBox;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AccentMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> LoopFX;

	/** 주운 사람(서버 PlayerId) — 그 사람 화면에서만 사라진다 */
	UPROPERTY(ReplicatedUsing = OnRep_CollectedBy)
	TArray<FString> CollectedBy;

	UFUNCTION()
	void OnRep_PropId();

	UFUNCTION()
	void OnRep_CollectedBy();

	UFUNCTION()
	TArray<FString> GetPropIdOptions() const;

private:
	/** 표 행을 액터에 입힌다(메시 · 재질 · 상자 · 불 · 상태 컴포넌트의 재질 · 처음 상태). 에디터 · 게임 모두 */
	void ApplyDefinition();
	void RefreshPropVisuals();
	void HandlePropStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity);
	void SpawnReward();
	void DriftWithWind();
	void StartKick();
	void ScheduleReset();

	UFUNCTION()
	void HandleNightChanged();

	bool IsNightNow() const;

	FTransform BodyBaseTransform;
	FRandomStream RewardStream;
	FTimerHandle ResetTimer;
	float KickElapsed = -1.f;
	int32 RewardsSpawned = 0;
	bool bShowingAlt = false;
	bool bDrifted = false;
	bool bBoundDawn = false;
	bool bBoundDayNight = false;
};
