#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Core/GrimSaveTypes.h"
#include "Save/GrimSaveGame.h"
#include "GrimSaveSubsystem.generated.h"

class AActor;
class AController;
class AGameModeBase;
class APlayerState;
class IGrimSaveParticipant;

/** 모은 참여자 하나(저장 · 불러오기 한 번 동안만 쓴다). */
struct FGrimSaveParticipantRef
{
	TWeakObjectPtr<UObject> Object;
	IGrimSaveParticipant* Interface = nullptr;
	FName SaveId;
};

/**
 * 세이브 v1 (docs/systems/world-multiplayer 「세이브」) — 서버(리슨 호스트 · 싱글)에서만 일한다. 클라이언트 월드에서는 아무것도 안 한다.
 *
 * - 세계 세이브 <Saved>/SaveGames/Grimoire/<슬롯>/WorldSave_<슬롯>.sav: 월드 서브시스템 · 놓인 액터(와 컴포넌트) · 스폰된 저장 대상 액터의 몫,
 *   게임 중 없어진 놓인 액터 목록. 슬롯 = URL 옵션 ?SaveSlot=<이름> → 없으면 설정 DefaultSlot.
 * - 플레이어 세이브 같은 폴더의 Player_<PlayerId>.sav: PlayerState · 그 컴포넌트 · 폰의 Player 몫 + Player.Attributes · Player.Transform.
 *   캐릭터는 서버(세계 슬롯)에 묶인다(Q12 — 다른 서버로 옮기기는 나중).
 * - 저장: 자동(설정 간격) · 맵을 떠날 때 · 종료(OnWorldBeginTearDown) · 플레이어 퇴장(그 플레이어) · 콘솔 Grim.Save.Now.
 * - 불러오기: 시작 다음 틱에 세계(놓인 액터의 BeginPlay 뒤), 플레이어는 폰이 준비되고 PlayerLoadDelayTicks 뒤(시작 마법 · 초기 수치 다음).
 * - 지난 파일에 있었는데 지금 참여자가 없는 레코드는 그대로 다시 쓴다(모르는 시스템 · 새 버전 · 스트리밍으로 안 올라온 액터).
 */
UCLASS()
class GRIMOIRE_API UGrimSaveSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	static UGrimSaveSubsystem* Get(const UWorld* World);

	/** 서버이고 이 월드에서 저장이 켜져 있다. */
	bool IsActive() const;

	/** 꺼진 까닭(켜져 있으면 빈 문자열). */
	const FString& GetDisabledReason() const { return DisabledReason; }

	const FString& GetSlot() const { return Slot; }

	static FString GetSlotDirectoryFor(const FString& InSlot);
	FString GetWorldSavePath() const;
	FString GetPlayerSavePath(const FString& PlayerId) const;

	// ── 저장 · 불러오기 (서버) ──

	/** 세계 + 불러오기를 마친 모든 플레이어. */
	bool SaveAll(const FString& Reason);
	bool SaveWorld(const FString& Reason);
	/** 그 플레이어(불러오기를 마친 뒤에만 — 아직이면 기본 상태가 세이브를 덮지 않게 건너뛴다). */
	bool SavePlayer(APlayerState* PlayerState, const FString& Reason);

	/** 디스크에서 세계 + 접속한 모든 플레이어를 다시 읽어 적용한다(Grim.Save.Load). */
	bool LoadAll(const FString& Reason);
	bool LoadWorld(const FString& Reason);
	bool LoadPlayer(APlayerState* PlayerState, const FString& Reason);

	/** 슬롯 폴더를 지운다(지금 슬롯이면 기억해 둔 레코드도 비운다 — 끄면 지금 상태가 새로 저장된다). */
	bool DeleteSlot(const FString& InSlot);

	/** 지금 상태 · 파일을 로그로(Grim.Save.Dump). */
	void Dump() const;

	/** 서버: 그 플레이어의 PlayerId(정했으면). */
	FString GetPlayerId(const APlayerState* PlayerState) const;
	bool IsPlayerLoaded(const APlayerState* PlayerState) const;

	// ── 모으기 (테스트 · 디버그도 쓴다) ──

	/** 레벨에 놓여 함께 로드된 액터인가(이름이 레벨 안에서 고정). */
	static bool IsPlacedActor(const AActor* Actor);

	/** 액터 자신과 컴포넌트 중 Scope 의 참여자. */
	static void GatherActorParticipants(AActor* Actor, EGrimSaveScope Scope, TArray<FGrimSaveParticipantRef>& Out);

	/** 플레이어 몫: PlayerState · 그 컴포넌트 · 폰 · 폰 컴포넌트의 Player 참여자 + 세이브 시스템의 Player.Attributes · Player.Transform. */
	static void GatherPlayerParticipants(APlayerState* PlayerState, TArray<FGrimSaveParticipantRef>& Out, TArray<TStrongObjectPtr<UObject>>& OutKeepAlive);

	/**
	 * 세계 몫: 월드 서브시스템 · 놓인 액터(와 컴포넌트) · 스스로 참여자인 프레임워크 액터(AInfo · 폰 · 컨트롤러)는 OutTopLevel,
	 * 스스로 참여자인 그 밖의 스폰된 액터는 OutSpawned(클래스 · 위치와 함께 저장 → 불러오면 다시 스폰).
	 * 스폰된 액터의 컴포넌트만 참여자면(플레이어 폰의 속성 상태 등) 저장하지 않는다.
	 */
	void GatherWorldParticipants(TArray<FGrimSaveParticipantRef>& OutTopLevel, TArray<AActor*>& OutSpawned) const;

	static void WriteRecords(const TArray<FGrimSaveParticipantRef>& Participants, TArray<FGrimSaveRecord>& OutRecords);

	/** Records 를 참여자에게 읽힌다(Priority 순서 먼저). IdRename: 저장 때 이름 → 지금 이름(다시 스폰한 액터의 이름이 바뀌었을 때). 적용한 수. */
	static int32 ApplyRecords(const TArray<FGrimSaveParticipantRef>& Participants, const TArray<FGrimSaveRecord>& Records, const TArray<FName>& Priority,
		const FString& Context, const TPair<FString, FString>* IdRename = nullptr);

private:
	struct FPlayerEntry
	{
		TWeakObjectPtr<APlayerState> PlayerState;
		FString PlayerId;
		FString IdSource;
		FString PlayerName;
		bool bLoaded = false;
		int32 ReadyTicks = 0;
		int32 SaveCount = 0;
		/** 지난 파일의 레코드 — 다시 쓸 때 모르는 레코드를 남긴다 */
		TArray<FGrimSaveRecord> StoredRecords;
		/** 마지막으로 본 폰 위치 · 방향 — 퇴장 때는 폰이 Logout 보다 먼저 사라진다 */
		bool bHasLastTransform = false;
		FVector LastLocation = FVector::ZeroVector;
		float LastYaw = 0.f;
	};

	void DecideSlot();
	void UpdatePlayers();
	FPlayerEntry* FindEntry(const APlayerState* PlayerState);
	const FPlayerEntry* FindEntry(const APlayerState* PlayerState) const;
	bool ResolveEntryId(FPlayerEntry& Entry, APlayerState* PlayerState);
	bool ApplyPlayerFile(FPlayerEntry& Entry, APlayerState* PlayerState, const FString& Reason);

	UGrimSaveGame* NewSaveGame(EGrimSaveScope Scope) const;
	bool WriteSaveGame(UGrimSaveGame* SaveGame, const FString& Path, const FString& What) const;
	/** 본 파일 · 백업이 다 깨졌으면 지우지 않고 옆으로 치운다(.corrupt-<시각>) — 손으로 살릴 수 있게. */
	static void QuarantineCorrupt(const FString& Path);

	void HandleLogout(AGameModeBase* GameMode, AController* Exiting);
	void HandleWorldBeginTearDown(UWorld* World);
	void HandleActorDestroyed(AActor* Actor);

	FString Slot;
	FString SlotSource;
	bool bEnabled = false;
	FString DisabledReason = TEXT("아직 시작 전");
	bool bWorldLoadPending = false;
	bool bWorldLoaded = false;
	bool bTornDown = false;
	double NextAutoSaveSeconds = 0.0;
	int32 WorldSaveCount = 0;

	TArray<FGrimSaveRecord> WorldStoredRecords;
	TArray<FName> RemovedActors;
	TSet<FName> DroppedWorldIds;
	TArray<FPlayerEntry> Players;

	FDelegateHandle LogoutHandle;
	FDelegateHandle TearDownHandle;
	FDelegateHandle ActorDestroyedHandle;
};
