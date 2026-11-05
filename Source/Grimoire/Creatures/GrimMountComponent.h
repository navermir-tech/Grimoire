#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GrimMountComponent.generated.h"

class AGrimCreature;
class ACharacter;
class APhysicsVolume;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FGrimMountChangedNative, class UGrimMountComponent* /*Mount*/, ACharacter* /*Rider*/, bool /*bMounted*/);

/**
 * 탈것(켈피 — 스펙 「탈것」 · Q9 · Q25). AGrimCreature 의 기본 하위 객체(탈것 종에서만 일한다).
 *
 * Possess 대신 「탄 사람 입력 중계」를 쓴다(docs/systems/creatures 「탑승」): 플레이어 컨트롤러는 계속 자기 캐릭터를 조종하고,
 * 캐릭터는 생물 캡슐(메시에 Saddle 소켓이 있으면 그 소켓)에 붙어 이동이 꺼진다. 이동 입력은 캐릭터에 쌓인 그대로를
 * UGrimTamerComponent 가 서버로 보내고(Server_MountInput), 서버가 생물에 AddMovementInput 한다.
 * 그래서 ASC 아바타 · 조준 · 카메라 · 가방 · 세이브(PC->GetPawn())가 그대로이고 탄 채 시전도 같은 길로 간다(Q9).
 *  - Rider 는 복제되고, 각 기기가 OnRep 에서 붙이기 · 충돌 끄기 · 탄 사람 ASC 의 State.Mounted(그리모어 거두기 Q25) · 카메라 붐을 맞춘다.
 *  - 물 위 달리기: 물 볼륨(PhysicsVolume bWaterVolume)에 들어가면 수면 높이에 붙여 Flying 으로 달린다. 물 볼륨이 없는 맵은
 *    WaterSurfaceTag 표면을 밟고 달린다(충돌이 있는 표면 — 자리).
 *  - 반항 단계면 BuckCheckSeconds 마다 떨어뜨릴지 굴린다.
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimMountComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimMountComponent();

	AGrimCreature* GetCreature() const;
	ACharacter* GetRider() const { return Rider; }
	bool IsRidden() const { return Rider != nullptr; }

	/** 탈것 종인가(종 bMountable) */
	bool IsMountable() const;

	/** 이 캐릭터가 타고 있는 탈것(없으면 nullptr) */
	static UGrimMountComponent* FindMountOf(const AActor* RiderActor);

	/** 물 위를 달리는 중(서버 · 덤프) */
	bool IsWaterRunning() const { return bWaterRunning; }

	/** 지금 탑승 속도(cm/s) */
	float GetRideSpeed() const;

	// ───── 서버 ─────

	/** 태운다. 못 태우면 false 와 까닭(화면 한 줄) */
	bool TryMount(ACharacter* InRider, FString& OutReason);

	/** 내린다(bThrown = 떨어뜨림). */
	void Dismount(bool bThrown);

	/** 탄 사람 이동 입력(월드 방향, 크기 0~1) */
	void SetRiderInput(const FVector& WorldInput);

	FGrimMountChangedNative OnMountChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Rider();

private:
	/** 각 기기: 붙이기 · 떼기 연출(여러 번 불러도 같다) */
	void ApplyRiderLocal();
	void AttachRider(ACharacter* InRider);
	void DetachRider(ACharacter* OldRider);
	void TickWaterRun(float DeltaTime);
	void TickBuck();
	void ApplyRideSpeed();

	UPROPERTY(ReplicatedUsing = OnRep_Rider)
	TObjectPtr<ACharacter> Rider;

	/** 이 기기에서 붙여 둔 탄 사람 */
	TWeakObjectPtr<ACharacter> AppliedRider;
	float SavedArmLength = -1.f;
	TEnumAsByte<ECollisionResponse> SavedWorldDynamicResponse = ECR_Block;

	// 서버
	FVector PendingInput = FVector::ZeroVector;
	double LastInputTime = -1000.0;
	double NextBuckCheckTime = 0.0;
	bool bWaterRunning = false;
	TWeakObjectPtr<APhysicsVolume> WaterVolume;
	bool bLoggedTaggedSurface = false;
};
