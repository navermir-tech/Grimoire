#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "GrimBed.generated.h"

class UStaticMeshComponent;
class APawn;
class APlayerController;

/**
 * 침대 · 침낭 (스펙 「잠 · 밤 건너뛰기」). 「마법으로 상호작용」의 예외 — 다가가 키(설정 BedInteractKey)로 직접 눕는다.
 * 로컬 플레이어가 반경 안이면 이 액터가 키를 직접 읽고(캐릭터 입력을 고치지 않으려고), 생존 컴포넌트의 서버 RPC 로 요청한다.
 * 모양은 엔진 기본 도형(큐브) 임시. 마지막으로 잔 침대가 부활 지점이 된다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimBed : public AActor, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimBed();

	// 세이브 (IGrimSaveParticipant — World 「Bed.<이름>」 v1: 침낭인가). 게임 중 놓은 침대(grim.Survival.SpawnBed · 제작 M4)는
	// 세이브 시스템이 클래스 · 위치와 함께 저장해 다시 스폰한다. 부활 지점은 플레이어 몫(Survival.Vitals)이 위치로 가진다.
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 부활할 곳(침대 옆). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	FTransform GetRespawnTransform() const;

	/** 폰이 눕기 거리 안인가(서버 검증에도 쓴다). Slack 만큼 너그럽게. */
	bool IsWithinReach(const APawn* Pawn, float Slack = 0.f) const;

	bool IsBedroll() const { return bBedroll; }

	/** 스폰 직후(FinishSpawning 전) 모양을 고른다. */
	void SetBedroll(bool bInBedroll) { bBedroll = bInBedroll; }

protected:
	/** true = 침낭(납작, 베개 없음). 기능은 같다 — 내구도는 제작(M4) 몫. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Bedroll, Category = "Grimoire|Survival")
	bool bBedroll = false;

	UFUNCTION()
	void OnRep_Bedroll() { ApplyShape(); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 부활 위치(침대 기준 로컬). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|Survival", meta = (MakeEditWidget))
	FVector RespawnOffset = FVector(0.f, 130.f, 100.f);

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Survival")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Survival")
	TObjectPtr<UStaticMeshComponent> Mattress;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|Survival")
	TObjectPtr<UStaticMeshComponent> Pillow;

private:
	void ApplyShape();
	void HandleLocalPlayer(APlayerController* PlayerController);
};
