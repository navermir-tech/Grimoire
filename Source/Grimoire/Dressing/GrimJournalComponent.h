#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "GrimJournalComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FGrimOnJournalEntry, class UGrimJournalComponent* /*Journal*/, FName /*EntryId*/);

/**
 * 도감 「발견」 기록 — 도감 탭이 생기기 전의 임시 목록(docs/spec/meadow-dressing.md 「구현 때 먼저 있어야 할 것」).
 * 플레이어마다 하나, PlayerState 에 붙는다(UGrimDressingSubsystem 이 런타임에 — 가방과 같은 방식). 소유자에게만 복제.
 * 줄 내용은 Journal.csv. 새 줄은 위쪽 알림(HUD 토스트)으로 보이고, 콘솔 Grim.Journal 로 목록을 본다.
 * 세이브: Player 「Dressing.Journal」 v1(TArray<FName>).
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimJournalComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimJournalComponent();

	static const FName ComponentName;

	/** 폰 · 컨트롤러 · PlayerState 어느 것을 주어도 그 플레이어의 도감 */
	static UGrimJournalComponent* Find(const AActor* PlayerActor);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Dressing.Journal"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 서버: 새 줄이면 더하고 true. 쪽 묶음이 설정만큼 찰 때마다 한 번 보상(잠정 — Q152) */
	bool AddEntry(FName EntryId);

	bool HasEntry(FName EntryId) const { return Entries.Contains(EntryId); }
	const TArray<FName>& GetEntries() const { return Entries; }

	/** 표의 Group 이 같은 줄 수 */
	int32 CountGroup(FName Group) const;

	/** 새 줄을 받았을 때(소유 클라이언트 · 리슨 호스트) */
	FGrimOnJournalEntry OnEntryAdded;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FName> Entries;

	UFUNCTION()
	void OnRep_Entries();

	UFUNCTION(Client, Reliable)
	void Client_Discovered(FName EntryId, int32 GroupCount);

private:
	void ShowDiscovered(FName EntryId, int32 GroupCount);
};
