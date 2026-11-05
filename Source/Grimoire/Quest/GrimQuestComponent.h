#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "Quest/GrimQuestTypes.h"
#include "GrimQuestComponent.generated.h"

class AController;
class APlayerController;
class APlayerState;
class UGrimQuestComponent;
class UGrimQuestTrackerWidget;

/** 소유 클라이언트에 보내는 퀘스트 알림 */
UENUM()
enum class EGrimQuestNotice : uint8
{
	QuestStarted,
	StepStarted,
	QuestCompleted,
	QuestFailed
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnQuestLogChanged, UGrimQuestComponent* /*Quest*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FGrimOnQuestNotice, UGrimQuestComponent* /*Quest*/, EGrimQuestNotice /*Notice*/, FName /*QuestId*/, int32 /*Step*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FGrimOnNpcResponse, UGrimQuestComponent* /*Quest*/, AActor* /*Npc*/, FName /*Context*/);

/**
 * 플레이어 한 명의 퀘스트 진행 (docs/systems/tutorial). PlayerState 에 붙는다 — Core 를 고치지 않으려고
 * UGrimQuestSubsystem(서버)이 런타임에 붙인다(복제되는 동적 컴포넌트, 이름 고정 "GrimQuest").
 *  - 서버: 표(UGrimQuestSettings::GetQuestDatabase)와 FGrimQuestModel 로 진행, 조회형 목표는 서브시스템 주기마다 세계를 본다.
 *  - 진행(FGrimQuestLog)은 소유 클라이언트에게만 복제 → 추적 창 · NPC 대사 고르기.
 *  - 보상: 시험 통과 = 서클 승급(UGrimProgressionComponent::TryCircleUp), 스킬북(UseSkillbook), 경험치(레벨 갈래 — TODO(통합)).
 *  - 세이브: IGrimSaveParticipant(Player, "Quest.Log"). 시험(시간 제한)은 저장하지 않는다 — 다시 청하면 된다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimQuestComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimQuestComponent();

	/** 런타임에 붙는 이름(서버 · 클라이언트 같은 이름이어야 복제가 잇는다) */
	static const FName ComponentName;

	static UGrimQuestComponent* FindForPlayerState(const APlayerState* PlayerState);
	static UGrimQuestComponent* FindForController(const AController* Controller);
	/** 폰 · 컨트롤러 · PlayerState 어느 것이든 */
	static UGrimQuestComponent* FindForActor(const AActor* Actor);

	const FGrimQuestLog& GetLog() const { return Log; }

	/** GameState 의 서버 월드 시각(서버 · 클라이언트 같은 기준 — 시간 제한 표시) */
	float GetServerTime() const;

	APawn* GetPawn() const;

	// ───── 서버 ─────

	EGrimQuestStartResult StartQuest(FName QuestId);

	/** 신호형 목표(대화 · 밤 넘김 · 명중) */
	void ReceiveSignal(const FGrimQuestSignal& Signal);

	/**
	 * 서버: 다른 시스템이 퀘스트 이벤트를 알린다 → Event 목표(Target = EventId)를 센다.
	 * 아직 없는 시스템이 생기면 부를 자리(이름은 docs/systems/tutorial 「이벤트」 표):
	 *  TODO(M3): Creature.Killed.Small(소형 생물 처치) · Creature.Weakened.Small(체력 30% 이하) · Creature.Tamed.Small(길들이기 성공)
	 *  있음(M3 갈래 I — Items/): Item.Obtained.<ID · 묶음>(Item.Obtained.Meat) · Cooking.IngredientsAdded · Cooking.Completed · Food.Eaten
	 *  TODO(M4): Base.Founded(거점 핵으로 첫 거점)
	 * PlayerActor 는 폰 · 컨트롤러 · PlayerState 어느 것이든.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Quest")
	static void NotifyQuestEvent(AActor* PlayerActor, FName EventId, int32 Amount = 1);

	void NotifyEvent(FName EventId, int32 Amount = 1) { ReceiveSignal({ EGrimQuestObjectiveType::Event, EventId, Amount }); }

	/** 조회형 목표를 다시 보고 단계를 넘긴다(서브시스템이 주기마다) */
	void ServerPoll();

	void FailQuest(FName QuestId);

	/** 맵 설정이 맞으면 AutoStart 퀘스트를 시작(처음 한 번) */
	void TryAutoStart();

	/** 개발용: 지금 단계의 목표를 모두 채운 것으로 */
	void DebugCompleteStep(FName QuestId);

	/** 밤 건너뛰기 페이드가 시작될 때 잠들어 있었는가를 기억했다가, 건너뛰면 신호 */
	void NotifyNightSkipBegin();
	void NotifyNightSkipped();
	void NotifyNightSkipCancelled() { bNightCreditPending = false; }

	/**
	 * NPC 에게 요청. Request = "Talk"(대화를 마쳤다 → TalkToNpc 신호) 또는 서비스 ID(Trial · Purify · Repair · Erase).
	 * 서비스 결과 맥락(Trial.Started …)은 OnNpcResponse 로 돌아온다(소유 클라이언트 · 리슨 호스트).
	 */
	void RequestNpc(AActor* Npc, FName Request);

	UFUNCTION(Server, Reliable)
	void Server_NpcRequest(AActor* Npc, FName Request);

	/** 진행이 바뀌었다(서버 · 소유 클라이언트) */
	FGrimOnQuestLogChanged OnLogChanged;

	/** 시작 · 단계 · 끝 · 실패(서버 · 소유 클라이언트) */
	FGrimOnQuestNotice OnNotice;

	/** NPC 서비스 결과(서버 · 소유 클라이언트) */
	FGrimOnNpcResponse OnNpcResponse;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Quest.Log"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Log();

	UFUNCTION(Client, Reliable)
	void Client_Notice(EGrimQuestNotice Notice, FName QuestId, int32 Step);

	UFUNCTION(Client, Reliable)
	void Client_NpcResponse(AActor* Npc, FName Context);

	/** 서버: 모델이 내놓은 변화 → 시각 · 소품 · 보상 · 알림 */
	void HandleChanges(const TArray<FGrimQuestChange>& Changes);
	void GrantRewards(const FGrimQuestDef& Def);
	void SendNotice(EGrimQuestNotice Notice, FName QuestId, int32 Step);
	void SendNpcResponse(AActor* Npc, FName Context);
	void MarkLogChanged();

	/** 서버: 서비스 하나를 처리하고 대사 맥락을 돌려준다 */
	FName ExecuteService(AActor* Npc, FName Service);

	APlayerController* GetLocalOwningController() const;

	/** 화면이 있는 소유자 쪽: 추적 창을 (없으면) 만든다. 만들었으면 true */
	bool EnsureTracker();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Log)
	FGrimQuestLog Log;

	UPROPERTY(Transient)
	TObjectPtr<UGrimQuestTrackerWidget> Tracker;

	FTimerHandle TrackerTimer;
	bool bNightCreditPending = false;
	bool bAutoStartTried = false;
};
