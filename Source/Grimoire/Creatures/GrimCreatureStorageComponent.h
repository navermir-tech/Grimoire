#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimCreatureStorageTypes.h"
#include "Magic/GrimSpellbookComponent.h"
#include "GrimCreatureStorageComponent.generated.h"

class AGrimCreature;
class AGrimCreatureBox;
class APawn;
class APlayerController;
class APlayerState;
class UGrimCreatureStorageWidget;
class UGrimTamerComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimCreatureStorageChangedNative, class UGrimCreatureStorageComponent* /*Storage*/);

/**
 * 플레이어 한 명의 생물 데리고 다니기 · 보관함(R38) — PlayerState 에 UGrimTamingSubsystem 이 런타임에 붙인다(이름 고정 "GrimCreatureStorage").
 *  - 데리고 다니는 수(PartyLimit) · 보관함 칸 수(StorageCapacity): 서버가 서클 표 + 성장 노드 태그로 셈해 소유 클라이언트에 복제.
 *  - 생물 보관함(노말 스킬): 스킬북 「Skill:CreatureStorage」을 읽으면 배운다. 넣은 생물은 세계에서 사라지고 기록(Stored)만 남는다.
 *  - 옮기기 Transfer(From → To): 데리고 다님 · 보관함 · 거점 상자 · 풀어놓음 네 곳 사이. 상자가 끼면 그 상자 곁이어야 한다.
 *  - 새로 길들일 때 갈 곳(DecideNewTame): 파티 → 보관함 → 세계의 거점 상자 → 거절(굴림 전).
 *  - 세이브: Player 「Creatures.Storage」 v1 — 배웠나 + 보관함 기록. 데리고 다니는 생물은 UGrimTamerComponent(Creatures.Owned)가 그대로 맡는다.
 *  - 로컬: 거점 상자 관리 화면(UGrimCreatureStorageWidget)을 띄운다. 가방 「생물」 탭은 가방 화면이 같은 위젯을 따로 둔다.
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimCreatureStorageComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimCreatureStorageComponent();

	static const FName ComponentName;

	/** 폰 · 컨트롤러 · PlayerState 어느 것이든 */
	static UGrimCreatureStorageComponent* FindForActor(const AActor* Actor);

	APlayerState* GetPlayerState() const;
	APawn* GetPawn() const;
	APlayerController* GetLocalOwningController() const;
	UGrimTamerComponent* GetTamer() const;

	// ───── 읽기(서버 · 소유 클라이언트) ─────

	bool IsStorageLearned() const { return bStorageLearned; }
	int32 GetPartyLimit() const { return PartyLimit; }
	int32 GetStorageCapacity() const { return StorageCapacity; }
	const TArray<FGrimOwnedCreatureRecord>& GetStored() const { return Stored; }
	/** 세이브와 같은 PlayerId(서버가 정해 복제 — 화면이 상자에서 내 칸을 고른다) */
	const FString& GetOwnerPlayerId() const { return OwnerPlayerId; }

	/** 이 플레이어가 데리고 다니는 생물(세계 — 풀어놓은 것 · 죽은 것 빼고). 서버 · 클라이언트 */
	TArray<AGrimCreature*> GetPartyCreatures() const;
	int32 GetPartyCount() const { return GetPartyCreatures().Num(); }
	bool IsMine(const AGrimCreature* Creature) const;

	// ───── 서버 ─────

	/** 상한(서클 · 성장 태그)을 다시 셈한다 */
	void RefreshLimits();

	/** 옮기기 판정이 보는 수(상자가 있으면 그 상자) */
	FGrimCreatureStorageCounts MakeCounts(const AGrimCreatureBox* Box) const;

	/** 생물 보관함(노말 스킬)을 배운다 — 스킬북 읽기. 서클 · 이미 배움 확인 */
	EGrimLearnResult LearnStorageSkill();

	/** 서버: 노말 스킬 스킬북 읽기(아이템 표 TeachesSpell = "Skill:<ID>") — 가방이 부른다. 모르는 스킬이면 Invalid */
	static EGrimLearnResult LearnNormalSkill(AActor* PlayerActor, FName SkillId);

	/** 새로 길들인 생물이 갈 곳(굴림 전, 그 생물은 아직 내 것이 아니다) */
	EGrimNewTameDestination DecideNewTame();

	/** 길들인 직후: 파티가 아니면 보관함 · 상자로 보낸다(다음 틱 — 파크툼 명중 처리 도중에 액터를 지우지 않게) */
	void PlaceNewlyTamed(AGrimCreature* Creature, EGrimNewTameDestination Destination, bool bDeferred = true);

	/**
	 * 옮기기 From → To(종 + 씨앗). Box 는 상자가 끼면 필요(bCheckReach 면 그 상자 곁이어야 한다).
	 * 자리 · 배움 검사 → 원래 자리에서 빼기 → 새 자리에 넣기(세계로 가면 액터를 내놓고, 세계에서 오면 액터를 거둔다). 알림 · 로그.
	 */
	EGrimCreatureStorageResult Transfer(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To, bool bCheckReach);

	/** 데리고 다니는 수를 넘는 기록(접속 때 다시 부르기)을 보관함에 받는다. 자리가 없거나 안 배웠으면 false */
	bool AcceptOverflowRecord(const FGrimOwnedCreatureRecord& Record);

	/** 풀어놓은 내 생물(세계)에 주인 PlayerState 를 잇는다 */
	void BindReleasedCreatures();

	/** 개발용: 배움 · 보관함 기록을 정한다 */
	void DebugSetLearned(bool bLearned);

	// ───── RPC ─────

	UFUNCTION(Server, Reliable)
	void Server_Transfer(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To);

	/** 어디서든(서버면 바로) */
	void RequestTransfer(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To);

	/** 바뀜(서버 · 소유 클라이언트 복제) — 화면이 듣는다 */
	FGrimCreatureStorageChangedNative OnStorageChanged;

	// ───── 로컬: 거점 상자 화면 ─────

	void OpenBoxScreen(AGrimCreatureBox* Box);
	void CloseBoxScreen();
	bool IsBoxScreenOpen() const;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Creatures.Storage"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: bool 배웠나, GrimTamingMath::SerializeOwnedRecords(보관함 기록) */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Storage();

private:
	bool HasAuthority() const;
	void MarkChanged();
	void Notice(const FString& Text) const;
	/** 기록을 주인 곁에 생물로 내놓는다(폰이 없으면 PlayerState 자리) */
	AGrimCreature* SpawnNearOwner(const FGrimOwnedCreatureRecord& Record);
	FString DescribeRecord(const FGrimOwnedCreatureRecord& Record) const;
	void FinishPlaceNewlyTamed(TWeakObjectPtr<AGrimCreature> WeakCreature, EGrimNewTameDestination Destination);

	UPROPERTY(ReplicatedUsing = OnRep_Storage)
	bool bStorageLearned = false;

	UPROPERTY(ReplicatedUsing = OnRep_Storage)
	int32 PartyLimit = 1;

	UPROPERTY(ReplicatedUsing = OnRep_Storage)
	int32 StorageCapacity = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Storage)
	TArray<FGrimOwnedCreatureRecord> Stored;

	UPROPERTY(ReplicatedUsing = OnRep_Storage)
	FString OwnerPlayerId;

	FTimerHandle RefreshTimer;

	UPROPERTY(Transient)
	TObjectPtr<UGrimCreatureStorageWidget> BoxWidget;
};
