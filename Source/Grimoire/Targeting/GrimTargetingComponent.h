#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimTargetingTypes.h"
#include "Targeting/GrimTargetingMath.h"
#include "Targeting/GrimTargetingSettings.h"
#include "GrimTargetingComponent.generated.h"

class APlayerController;
class UGrimSpellDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnAimTargetChanged, UGrimTargetingComponent*, Targeting, AActor*, NewTarget, EGrimAimMode, Mode);

/**
 * 소프트 락온 · 고정 락온 (R17 · Q17 · Q18). 플레이어 컨트롤러에 붙고, 로컬 플레이어에서만 고른다.
 *
 * - 소프트: SelectionInterval 마다 사거리 구 오버랩 → IGrimTargetable → 원뿔 · 점수 → 상위 몇 개만 시야 트레이스 → 대상 하나.
 * - 고정: ToggleLock 으로 지금 대상을 고정. 카메라(컨트롤 회전)가 대상을 따라가고, 시점 입력은 옆 대상 넘기기에 쓴다.
 * - 마법 쪽은 IGrimAimProvider::GetCurrentAim 으로만 읽는다(UGrimTargetingStatics::FindAimProvider).
 * - 대상 표식은 임시로 디버그 선(속성색 링). 출시 빌드에서는 그려지지 않는다 — 정식 위젯은 HUD 작업 때.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimTargetingComponent : public UActorComponent, public IGrimAimProvider
{
	GENERATED_BODY()

public:
	UGrimTargetingComponent();

	// IGrimAimProvider
	virtual FGrimAimResult GetCurrentAim(float MaxRange) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** Free(대상 없음) · Soft · Locked. Zoom 은 아직 만들지 않았다(R24) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	EGrimAimMode GetAimMode() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	bool IsLocked() const { return bLocked && CurrentTarget.IsValid(); }

	/** 고정 락온 토글(R3 · Caps Lock). 대상이 없으면 한 번 골라서 고정한다(소프트 락온이 꺼져 있어도) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void ToggleLock();

	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void Unlock();

	/** 소프트 락온 켜고 끄기(Q18 옵션). 끄면 고정 락온 중이 아니면 대상이 비고 마법은 화면 중앙으로 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void SetSoftLockEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	bool IsSoftLockEnabled() const { return bSoftLockEnabled; }

	/** 카메라 기준 / 이동(스틱) 기준 선정 옵션 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting")
	void SetTargetingReference(EGrimTargetingReference NewReference);

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	EGrimTargetingReference GetTargetingReference() const { return Reference; }

	/**
	 * 컨트롤러의 AddYawInput · AddPitchInput 이 먼저 부른다. 고정 중이면 입력을 옆 대상 넘기기에 쓰고 true
	 * (카메라는 돌리지 않는다 — 카메라는 대상을 따라간다). 아니면 false.
	 */
	bool HandleLookInput(const FVector2D& YawPitch);

	/**
	 * 줌인 조준 입력(IA_AimZoom) 자리.
	 * TODO(R24): 줌인 조준은 스펙 조사 중 — 누르는 동안 Mode = Zoom, 카메라 당기기, 자동 대상 없이 화면 중앙 트레이스.
	 * 지금은 상태만 기억하고 아무것도 바꾸지 않는다.
	 */
	void SetZoomAimHeld(bool bHeld) { bZoomAimHeld = bHeld; }

	/** 콘솔 덤프용 한 줄 */
	FString Describe() const;

	/** 대상 · 모드가 바뀌었다(로컬). HUD 표식이 듣는다 */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Targeting")
	FGrimOnAimTargetChanged OnTargetChanged;

protected:
	virtual void BeginPlay() override;

	/** 한 번의 선정에 쓰는 시점: 원뿔 꼭짓점 · 축, 거리를 재는 캐릭터 위치, 카메라 */
	struct FSelectionFrame
	{
		FVector ConeOrigin = FVector::ZeroVector;
		FVector ConeAxis = FVector::ForwardVector;
		FVector PawnLocation = FVector::ZeroVector;
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		float Range = 0.f;
		/** 이동 기준이면 요 각도만 본다(높이 무시) */
		bool bFlattenAngle = false;
	};

	APlayerController* GetPlayerController() const;
	APawn* GetControlledPawn() const;
	bool IsLocallyControlled() const;
	bool MakeSelectionFrame(EGrimTargetingReference InReference, float Range, FSelectionFrame& OutFrame) const;

	/** 소프트 락온 선정 한 번. bForce 면 소프트 락온이 꺼져 있어도 고른다(고정 락온 시작용) */
	void UpdateSoftSelection(float DeltaSinceLast, bool bForce);
	void UpdateLockedTarget(float DeltaSinceLast);
	void UpdateLockedCamera(float DeltaTime);
	void TrySwitchLockedTarget(const FVector2D& ScreenDirection);

	/** 원뿔 · 거리 안의 대상 후보(점수 포함, 정렬 전) */
	void GatherCandidates(const FSelectionFrame& Frame, float HalfAngleDegrees, TArray<FGrimTargetCandidate>& OutCandidates) const;

	bool HasLineOfSight(const FVector& From, const AActor* Target, const FVector& Point) const;
	bool ReactsToSelectedSpell(const AActor* Target) const;
	const UGrimSpellDefinition* GetSelectedSpell() const;
	float GetSelectionRange() const;
	float GetLockedRange() const;
	FVector FreeAimPoint(const FVector& ViewLocation, const FRotator& ViewRotation, float MaxRange) const;

	void SetTarget(AActor* NewTarget, EGrimTargetKind NewKind, bool bNewLocked);
	void DrawTargetMarker() const;
	void DrawDebugCandidates() const;

	TWeakObjectPtr<AActor> CurrentTarget;
	EGrimTargetKind CurrentKind = EGrimTargetKind::None;
	bool bLocked = false;
	bool bSoftLockEnabled = true;
	EGrimTargetingReference Reference = EGrimTargetingReference::Camera;
	bool bZoomAimHeld = false;

	float SelectionTimer = 0.f;
	FGrimOcclusionTimer Occlusion;
	FGrimLookFlickDetector LookFlick;

	/** 디버그 그리기용 마지막 후보(Grim.Targeting.Debug 1) */
	TArray<FGrimTargetCandidate> LastCandidates;
};
