#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "Creatures/GrimCreatureStorageTypes.h"
#include "GrimCreatureBox.generated.h"

class AGrimCreature;
class APawn;
class APlayerState;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimCreatureBoxChangedNative, class AGrimCreatureBox* /*Box*/);

/**
 * 거점 생물 상자(R38 — 팰월드 팰 상자 참고). 놓인 곳 어디서나 쓰는 독립 액터.
 *  - 칸(Entries): 길들인 개체 기록 그대로 + 풀어놓았나. 모든 플레이어가 함께 쓰고(칸 수 합), 주인(OwnerId)만 옮긴다. 모두에게 복제(화면이 자기 것만 고른다).
 *  - 풀어놓기: 상자 둘레 BoxRoamRadius 안에 생물 액터를 내놓아 돌아다니게 한다(명령 Roam, 집 = 상자). 데리고 다니는 수에 들지 않는다.
 *  - 세이브: World 「CreatureBox.<이름>」 v1 — 칸 목록(풀어놓은 생물은 액터에서 기록을 고쳐 쓴다). 불러오면 풀어놓은 생물을 다시 내놓는다.
 *  - 옮기기는 플레이어의 UGrimCreatureStorageComponent::Transfer 가 이 상자의 서버 함수를 부른다(거리 검사도 거기서).
 * M4 거점 핵이 생기면 건축 조각(거점 핵 반경 안에만)이 되고, 풀어놓기 반경은 거점 반경으로 바뀐다 — 지금은 상자 자신의 반경.
 */
UCLASS()
class GRIMOIRE_API AGrimCreatureBox : public AActor, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimCreatureBox();

	// ───── 읽기(서버 · 클라이언트) ─────

	const TArray<FGrimCreatureBoxEntry>& GetEntries() const { return Entries; }
	int32 GetCapacity() const;
	float GetRoamRadius() const;
	int32 GetReleasedCount() const;
	/** 그 주인의 칸 수 */
	int32 CountOwnedBy(const FString& OwnerId) const;
	bool HasRoom() const { return Entries.Num() < GetCapacity(); }

	/** 폰이 관리 화면을 열 수 있는 거리 안인가(BoxManageRadius + Slack) */
	bool IsWithinManageRange(const APawn* Pawn, float Slack = 0.f) const;

	/** 이 폰이 열 수 있는 가장 가까운 상자. 없으면 nullptr */
	static AGrimCreatureBox* FindNearestUsable(const APawn* Pawn, float Slack = 0.f);

	/** 서버: 새로 길들인 생물이 넘칠 때 보낼 상자(Near 에 가장 가까운, 자리 있는 것). 없으면 nullptr */
	static AGrimCreatureBox* FindNearestWithRoom(const UWorld* World, const FVector& Near);

	/** 이 상자에 풀어놓은 그 생물 액터(세계). 없으면 nullptr */
	AGrimCreature* FindReleasedActor(FName SpeciesId, int32 Seed) const;

	// ───── 서버 ─────

	/** 칸에 넣는다(자리 검사는 부르는 쪽 — 여기서는 칸이 넘쳐도 넣지 않는다). bRelease 면 바로 풀어놓는다. 넣었으면 true */
	bool AddEntry(const FGrimOwnedCreatureRecord& Record, bool bRelease);

	/** 칸에서 꺼낸다(풀어놓은 것이면 액터에서 기록을 고쳐 쓰고 액터를 거둔다). 꺼냈으면 true */
	bool TakeEntry(FName SpeciesId, int32 Seed, FGrimOwnedCreatureRecord& OutRecord);

	/** 상자 안 → 풀어놓기(액터를 내놓는다) */
	bool ReleaseEntry(FName SpeciesId, int32 Seed);

	/** 풀어놓은 것 → 상자 안(액터를 거둔다) */
	bool RecallEntry(FName SpeciesId, int32 Seed);

	/** 풀어놓은 생물이 떠남 · 놓아줌 · 죽음 — 칸에서 지운다(액터는 건드리지 않는다) */
	void ForgetCreature(FName SpeciesId, int32 Seed);

	/** 풀어놓은 생물 액터로 칸 기록을 고친다(충성도 · 체력 · 이름) */
	void RefreshReleasedRecords();

	/** 풀어놓은 칸마다 액터가 있게(없으면 내놓고, 칸에 없는 액터는 거둔다) + 접속한 주인을 잇는다 */
	void SyncReleased();

	/** 화면 갱신(서버: 바꾼 뒤, 클라이언트: 복제) */
	FGrimCreatureBoxChangedNative OnBoxChanged;

	// ───── 세이브 (World 「CreatureBox.<이름>」 v1) ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Entries();

	/** 0 이면 설정 BoxCapacity */
	UPROPERTY(EditAnywhere, Category = "CreatureBox", meta = (ClampMin = "0"))
	int32 CapacityOverride = 0;

	/** 0 이면 설정 BoxRoamRadius */
	UPROPERTY(EditAnywhere, Category = "CreatureBox", meta = (ClampMin = "0", Units = "cm"))
	float RoamRadiusOverride = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "CreatureBox")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, Category = "CreatureBox")
	TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(VisibleAnywhere, Category = "CreatureBox")
	TObjectPtr<UStaticMeshComponent> Crest;

private:
	AGrimCreature* SpawnReleased(FGrimCreatureBoxEntry& Entry, int32 Slot);
	void MarkChanged();
	void ApplyColors();

	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FGrimCreatureBoxEntry> Entries;

	FTimerHandle SyncTimer;
};
