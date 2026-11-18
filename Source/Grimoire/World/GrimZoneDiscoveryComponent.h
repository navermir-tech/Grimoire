#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "World/GrimZoneTypes.h"
#include "GrimZoneDiscoveryComponent.generated.h"

class APlayerController;
class APlayerState;
class UGrimZoneBannerWidget;
class UGrimZoneDiscoveryComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnZonesDiscoveredChanged, UGrimZoneDiscoveryComponent* /*Zones*/);

/**
 * 한 플레이어가 본 구역 · 구역 이름 배너(docs/systems/world-zones.md). PlayerState 에 붙는다 —
 * Core 를 고치지 않으려고 UGrimZoneSubsystem(서버)이 런타임에 붙인다(복제되는 동적 컴포넌트, 이름 고정 "GrimZones").
 *  - 서버: 서브시스템 주기마다 폰 자리 → 구역(FGrimZoneMath::FindAt) → 들어섬(FGrimZoneVisitTracker) → 소유 클라이언트에 배너 RPC.
 *  - 본 구역 목록은 소유 클라이언트에게만 복제(나중에 지도 · 미니맵이 「가 본 곳」으로 쓴다). 멀티에서는 플레이어마다 따로.
 *  - 세이브: IGrimSaveParticipant(Player, "World.Zones") — 읽을 때 지금 목록과 합친다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimZoneDiscoveryComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimZoneDiscoveryComponent();

	/** 런타임에 붙는 이름(서버 · 클라이언트 같은 이름이어야 복제가 잇는다) */
	static const FName ComponentName;

	static UGrimZoneDiscoveryComponent* FindForPlayerState(const APlayerState* PlayerState);
	/** 폰 · 컨트롤러 · PlayerState 어느 것이든 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	static UGrimZoneDiscoveryComponent* FindForActor(const AActor* Actor);

	/** 본 구역(서버 · 소유 클라이언트) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	const TArray<FName>& GetDiscovered() const { return Discovered; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	bool IsDiscovered(FName Zone) const { return Discovered.Contains(Zone); }

	/** 서버: 들어선 것으로 친 지금 자리 */
	const FGrimZoneHit& GetCurrent() const { return Tracker.Current; }

	/** 본 구역이 바뀌었다(서버 · 소유 클라이언트) */
	FGrimOnZonesDiscoveredChanged OnDiscoveredChanged;

	// ───── 서버 ─────

	/** 폰 자리를 보고 들어섬을 알린다(서브시스템이 주기마다) */
	void ServerPoll(const FGrimZoneDatabase& Database, double Now);

	/** 개발 콘솔: 본 구역 · 지금 자리를 잊는다(배너를 다시 보려고) */
	void ServerResetDiscovered();

	// ───── 클라이언트 ─────

	/** 이 플레이어 화면에 배너를 띄운다(로컬 — 개발 콘솔 Grim.Zones.Show 도 쓴다) */
	void ShowBannerLocal(FName Zone, bool bFirstVisit);

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("World.Zones"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION(Client, Reliable)
	void Client_ZoneEntered(FName Zone, bool bFirstVisit);

	UFUNCTION()
	void OnRep_Discovered();

	void MarkDiscoveredChanged();
	APlayerController* GetLocalOwningController() const;

	UPROPERTY(ReplicatedUsing = OnRep_Discovered)
	TArray<FName> Discovered;

	UPROPERTY(Transient)
	TObjectPtr<UGrimZoneBannerWidget> Banner;

	/** 서버 판단 상태(Discovered 의 원본은 Tracker.Discovered — 바뀌면 복제 값에 옮긴다) */
	FGrimZoneVisitTracker Tracker;
};
