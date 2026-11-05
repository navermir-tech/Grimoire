#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "GrimAppearanceComponent.generated.h"

class USceneComponent;
class USkeletalMeshComponent;

/**
 * 외형 조립 — 플레이어 폰 · NPC 공용(docs/systems/customization).
 * 복제되는 외형 하나로 몸 메시(체형) · 키 · 스켈레탈 조각(Leader Pose) · 뼈에 붙는 소품 · 색 · 얼굴 슬라이더 · 서클 고리를 만든다.
 * 외형은 서버만 바꾼다(SetAppearance). 생성 화면은 PreviewAppearance 로 내 화면에서만 미리 본다.
 * 리더 메시: 주인이 ACharacter 면 그 메시, 아니면 SetLeaderMesh 로 준 것(없으면 주인의 첫 스켈레탈 메시).
 * 전용 서버는 조각을 만들지 않는다 — 값만 갖고 복제한다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimAppearanceComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 서버: 외형을 바꾼다(복제). 서버 화면(리슨 호스트 · 단독)도 바로 조립한다. */
	void SetAppearance(const FGrimAppearance& NewAppearance);

	/** 지금 보이는 외형(미리보기 중이면 미리보기) */
	const FGrimAppearance& GetDisplayedAppearance() const { return bPreviewing ? PreviewValue : Appearance; }

	/** 복제된 정본 외형 */
	const FGrimAppearance& GetAppearance() const { return Appearance; }

	/** 내 화면에서만 보이는 미리보기(생성 화면). 복제 · 저장되지 않는다. */
	void PreviewAppearance(const FGrimAppearance& Preview);

	/** 미리보기를 끝내고 복제된 외형으로 돌아간다 */
	void ClearPreview();

	bool IsPreviewing() const { return bPreviewing; }

	/** 생성 화면 「확인」: 서버면 바로 미리보기를 끝내고, 클라이언트면 서버 결과가 복제될 때 끝낸다(잠깐 옛 모습으로 돌아가지 않게) */
	void EndPreviewOnNextReplication();

	/** 조각을 붙일 리더 메시. ACharacter 가 아닌 주인(NPC)이 BeginPlay 전에 준다. */
	void SetLeaderMesh(USkeletalMeshComponent* InLeader);
	USkeletalMeshComponent* GetLeaderMesh() const;

	/** 지금 외형으로 다시 조립(카탈로그를 다시 읽었을 때) */
	void RebuildVisuals();

	/** 이 월드의 모든 외형을 다시 조립 */
	static void RebuildAllInWorld(UWorld* World);

	// 조립 결과(디버그 · 테스트)
	int32 GetSkeletalPartCount() const { return SkeletalPartCount; }
	int32 GetStaticPieceCount() const { return StaticPieceCount; }
	int32 GetRingCount() const { return RingCount; }
	bool HasBuiltBody() const { return bBuiltBody; }

	/** 조립이 끝날 때마다(모든 기기) — NPC 가 그레이박스 도형을 숨길 때 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrimAppearanceApplied, UGrimAppearanceComponent* /*Component*/);
	FOnGrimAppearanceApplied OnAppearanceApplied;

	/** 체형에 맞게 리더의 몸 메시를 바꾸나(설정 BodyMeshes). 몸이 따로 정해진 액터는 끈다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	bool bDriveBodyMesh = true;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Appearance)
	FGrimAppearance Appearance;

	UFUNCTION()
	void OnRep_Appearance();

private:
	void ApplyVisuals(const FGrimAppearance& Source);
	void ClearSpawned();

	/** 조립해 붙인 컴포넌트들(다시 조립할 때 지운다) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Spawned;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Leader;

	FGrimAppearance PreviewValue;
	bool bPreviewing = false;
	bool bEndPreviewOnRep = false;

	FVector BaseLeaderScale = FVector::OneVector;
	bool bCapturedBaseScale = false;

	int32 SkeletalPartCount = 0;
	int32 StaticPieceCount = 0;
	int32 RingCount = 0;
	bool bBuiltBody = false;
};
