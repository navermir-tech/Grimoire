#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Core/GrimInteractable.h"
#include "Core/GrimSaveTypes.h"
#include "GrimHarvestCell.generated.h"

class AGrimHarvestCell;
class UInstancedStaticMeshComponent;

/** 채집점 하나 — 배치 스크립트가 셀 액터에 적는다(레벨 데이터, 복제하지 않는다). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimHarvestPoint
{
	GENERATED_BODY()

	/** HarvestNodes.csv 행 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	FName Kind;

	/** 월드 변환 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest")
	FTransform Transform;
};

/** 거둔 점 하나(서버 → 복제). 공유면 PlayerKey 가 비고, 사람별이면 그 사람의 PlayerId. */
USTRUCT()
struct GRIMOIRE_API FGrimHarvestedEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PointIndex = INDEX_NONE;

	UPROPERTY()
	FString PlayerKey;

	/** 친 횟수. HitsToHarvest 이상이면 거둔 것 */
	UPROPERTY()
	int32 Hits = 0;

	/** 서버 전용: 다시 나는 서버 시각(초). 음수 = 시간으로는 안 남(아침 · 안 남) */
	UPROPERTY(NotReplicated)
	double RespawnAt = -1.0;

	void PostReplicatedAdd(const struct FGrimHarvestedArray& InArray);
	void PostReplicatedChange(const struct FGrimHarvestedArray& InArray);
	void PreReplicatedRemove(const struct FGrimHarvestedArray& InArray);
};

USTRUCT()
struct GRIMOIRE_API FGrimHarvestedArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGrimHarvestedEntry> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<AGrimHarvestCell> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGrimHarvestedEntry, FGrimHarvestedArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGrimHarvestedArray> : public TStructOpsTypeTraitsBase2<FGrimHarvestedArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * 채집 셀 관리자 (docs/systems/meadow-dressing.md) — 셀(256 m) 하나의 줍는 것(D10 곁가지 · 돌멩이, 뒤에 D11 · D12 · D15 · 나무 · 풀).
 * 놓인 수만큼 액터를 만들지 않는다(성능 규칙 20): 셀마다 액터 1 + 종류별 ISM + 거둔 점만 FastArray 로 복제(휴면).
 * 종류(메시 · 아이템 · 개수 · 친 횟수 · 소유 · 다시 나기)는 HarvestNodes.csv — 잠정(Q151 · Q153).
 * F 는 IGrimInteractable(가장 가까운 점). 세이브: World 「Dressing.Harvest.<이름>」 v1.
 */
UCLASS()
class GRIMOIRE_API AGrimHarvestCell : public AActor, public IGrimInteractable, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimHarvestCell();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IGrimInteractable
	virtual bool GetInteractionOffer(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const override;
	virtual bool Interact(APawn* Pawn, int32 Token) override;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: int32 개수, (int32 점, FString 사람, int32 친 횟수, float 남은 초(음수 = 시간 아님))… */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 배치 스크립트 · 테스트: 점을 더한다(BeginPlay 전에) */
	void AddPoint(FName Kind, const FTransform& WorldTransform);
	const TArray<FGrimHarvestPoint>& GetPoints() const { return Points; }

	/** 이 사람(PlayerId, 공유 종류면 무시)에게 이 점이 거둔 상태인가 */
	bool IsHarvestedFor(int32 PointIndex, const FString& PlayerKey) const;

	/** 거둔 상태로 남은 점 기록 수(복제되는 것) */
	int32 GetHarvestedEntryCount() const { return Harvested.Items.Num(); }

	/** 서버: 사람 대신 거둔다(테스트 · 콘솔). 준 아이템 수(가방 없으면 0 이어도 거둔 것으로 친다) */
	int32 HarvestPoint(int32 PointIndex, const FString& PlayerKey, APawn* Receiver);

	/** 서버: 해 뜸 — 아침에 다시 나는 점을 되돌린다 */
	UFUNCTION()
	void HandleDawn();

	/** 서버: 시간으로 다시 나는 점 중 때가 된 것을 되돌린다(타이머 · 테스트) */
	void RespawnDue(double NowSeconds);

	/** 클라이언트: 복제 콜백 → 그 점의 인스턴스를 숨기거나 보인다 */
	void RefreshPoint(int32 PointIndex);

	/** 그 점이 지금 이 화면에서 보이나(인스턴스 크기가 0 이 아니다 — 테스트) */
	bool IsPointVisible(int32 PointIndex) const;

	const FGrimHarvestedArray& GetHarvested() const { return Harvested; }

	/** 테스트: 서버 배열을 받은 것처럼(복제 콜백 그대로) — 클라이언트 쪽 모습 확인 */
	void ApplyReplicatedForTest(const FGrimHarvestedArray& FromServer);

protected:
	/** 레벨 데이터. 처음 한 번만 복제(게임 중 스폰한 셀 — 콘솔 · 나중의 동적 채집점) */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Points, Category = "Harvest")
	TArray<FGrimHarvestPoint> Points;

	UFUNCTION()
	void OnRep_Points();

	UPROPERTY(Replicated)
	FGrimHarvestedArray Harvested;

private:
	void BuildInstances();
	void UpdateRespawnTimer();
	void HandleRespawnTimer();
	double GetServerSeconds() const;
	FString LocalPlayerKey() const;
	int32 FindEntry(int32 PointIndex, const FString& PlayerKey) const;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> KindMeshes;

	/** 점 → (종류 ISM 의) 인스턴스 번호 */
	TArray<int32> PointInstance;

	FVector BoundsCenter = FVector::ZeroVector;
	float BoundsRadius = 0.f;
	FTimerHandle RespawnTimer;
	bool bBoundDawn = false;
	mutable FString CachedLocalKey;
};
