#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "Core/GrimSaveTypes.h"
#include "Creatures/GrimTamingTypes.h"
#include "GrimTamerComponent.generated.h"

class AGrimCreature;
class AGrimCreatureBox;
class ACharacter;
class APlayerController;
class APlayerState;
class UGrimCreatureMenuWidget;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGrimTamerNoticeNative, class UGrimTamerComponent* /*Tamer*/, const FString& /*Text*/);

/** 메뉴 · 콘솔이 보내는 행동 */
UENUM()
enum class EGrimTamerAction : uint8
{
	Follow,
	Wait,
	Mount,
	Feed,
	Release
};

/**
 * 플레이어 한 명의 길들이기(갈래 T) — PlayerState 에 UGrimTamingSubsystem 이 런타임에 붙인다(이름 고정 "GrimTamer", 가방과 같은 방식).
 *  - 서버: 가진 개체 기록(Records) · 세이브 참여(Player, "Creatures.Owned" v1) · 접속 · 불러오기 때 곁에 다시 부르기 · 퇴장 때 거두기.
 *  - RPC: 명령(따라오기 · 기다리기) · 이름 · 놓아주기 · 먹이 주기 · 타기 · 내리기 · 탑승 이동 입력.
 *  - 로컬 플레이어: 다가가면 [F] 안내 → 생물 메뉴(UGrimCreatureMenuWidget), 탄 동안 캐릭터에 쌓인 이동 입력을 탈것으로 보낸다.
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimTamerComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimTamerComponent();

	static const FName ComponentName;

	/** 폰 · 컨트롤러 · PlayerState 어느 것이든 */
	static UGrimTamerComponent* FindForActor(const AActor* Actor);

	APlayerState* GetPlayerState() const;
	APawn* GetPawn() const;
	/** 로컬 소유 컨트롤러(리슨 호스트 · 소유 클라이언트). 없으면 nullptr */
	APlayerController* GetLocalOwningController() const;

	/** 서버: 세이브와 같은 PlayerId(처음 부를 때 정해 기억한다) */
	FString GetPlayerId() const;

	/** 이 플레이어의 생물인가(서버 · 클라이언트 — 복제된 주인 PlayerState 로) */
	bool OwnsCreature(const AGrimCreature* Creature) const;

	// ───── 서버 ─────

	const TArray<FGrimOwnedCreatureRecord>& GetRecords() const { return Records; }
	void AddOrUpdateRecord(const FGrimOwnedCreatureRecord& Record);
	void RemoveRecord(const FGrimCreatureInstance& Instance);
	/** 세계에 있는 내 생물로 기록을 고친다 */
	void RefreshRecordsFromWorld();
	/** 기록 중 세계에 없는 것을 폰 곁에 놓는다. 놓은 수 */
	int32 SummonOwned();
	/** 세계에 있는 내 생물을 거둔다(퇴장) */
	void DespawnOwned();

	/** 서버 쪽 행동 — RPC · 콘솔이 부른다. bCheckReach 면 거리 검사 */
	bool DoAction(AGrimCreature* Creature, EGrimTamerAction Action, bool bCheckReach);
	bool DoRename(AGrimCreature* Creature, const FString& NewName, bool bCheckReach);
	bool DoDismount();

	/** 서버: 화면 한 줄(소유 클라이언트 · 리슨 호스트) */
	void SendNotice(const FString& Text);

	// ───── RPC ─────

	UFUNCTION(Server, Reliable)
	void Server_Action(AGrimCreature* Creature, EGrimTamerAction Action);

	UFUNCTION(Server, Reliable)
	void Server_Rename(AGrimCreature* Creature, const FString& NewName);

	UFUNCTION(Server, Reliable)
	void Server_Dismount();

	/**
	 * 탄 동안 이동 입력(월드 방향 · 크기 0~1)과 컨트롤 회전. 탄 사람 캐릭터의 이동 컴포넌트가 꺼져 ServerMove 가 가지 않으므로
	 * 서버의 컨트롤 회전(시전 조준 검증 · 서버 조준)을 여기서 맞춘다.
	 */
	UFUNCTION(Server, Unreliable)
	void Server_MountInput(FVector_NetQuantize100 Input, FRotator ControlRotation);

	/** 어디서든 요청(서버면 바로) */
	void RequestAction(AGrimCreature* Creature, EGrimTamerAction Action);
	void RequestRename(AGrimCreature* Creature, const FString& NewName);
	void RequestDismount();

	/** 알림(서버 · 소유 클라이언트) — 메뉴 위젯 · 로그 */
	FGrimTamerNoticeNative OnNotice;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Creatures.Owned"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	/** v1: int32 수, 항목마다 GrimTamingMath::SerializeOwnedRecord(개체 기록 · 체력 비율 · 명령 · 까닭 · 저장 시각) */
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Client, Reliable)
	void Client_Notice(const FString& Text);

private:
	bool HasAuthority() const;
	bool IsWithinReach(const AGrimCreature* Creature) const;
	bool FeedCreature(AGrimCreature* Creature);
	/** 가방에서 이 생물이 먹을 아이템(좋아하는 것 먼저). 없으면 None */
	FName FindFoodFor(const AGrimCreature* Creature, bool& bOutFavorite) const;
	void TrySummonLater();

	// 로컬
	void TickLocal(APlayerController* PC);
	bool EnsureWidget(APlayerController* PC);
	AGrimCreature* FindNearestCreature(const APawn* Pawn, bool bOwned) const;

	/** 서버: 가진 개체 */
	TArray<FGrimOwnedCreatureRecord> Records;
	mutable FString CachedPlayerId;
	bool bSummonPending = false;
	int32 SummonAttempts = 0;
	FTimerHandle SummonTimer;

	// 로컬
	UPROPERTY(Transient)
	TObjectPtr<UGrimCreatureMenuWidget> Widget;
	double NextInputSendTime = 0.0;
	FVector LastSentInput = FVector::ZeroVector;
	FRotator LastSentRotation = FRotator::ZeroRotator;
	float PromptAccumulator = 0.f;
	TWeakObjectPtr<AGrimCreature> PromptCreature;
	/** 곁의 거점 생물 상자(R38) — F 로 관리 화면 */
	TWeakObjectPtr<AGrimCreatureBox> PromptBox;
	bool bPromptIsFeed = false;
};
