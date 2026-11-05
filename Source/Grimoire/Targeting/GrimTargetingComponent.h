#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimTargetingTypes.h"
#include "Targeting/GrimTargetingMath.h"
#include "Targeting/GrimTargetingSettings.h"
#include "Targeting/GrimZoomAim.h"
#include "GrimTargetingComponent.generated.h"

class ACharacter;
class APlayerController;
class UCanvas;
class UGrimSpellDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnAimTargetChanged, UGrimTargetingComponent*, Targeting, AActor*, NewTarget, EGrimAimMode, Mode);

/**
 * 소프트 락온 · 고정 락온 (R17 · Q17 · Q18). 플레이어 컨트롤러에 붙고, 로컬 플레이어에서만 고른다.
 *
 * - 소프트: SelectionInterval 마다 사거리 구 오버랩 → IGrimTargetable → 원뿔 · 점수 → 상위 몇 개만 시야 트레이스 → 대상 하나.
 * - 고정: ToggleLock 으로 지금 대상을 고정. 카메라(컨트롤 회전)가 대상을 따라가고, 시점 입력은 옆 대상 넘기기에 쓴다.
 * - 줌인 조준(R24): 누르는 동안(옵션: 토글) 카메라를 어깨 쪽으로 당기고 시야각을 좁힌다. 자동 대상 없이 화면 가운데 트레이스.
 *   이동 감속 · 몸 방향 · State.Aiming 은 소유 클라이언트와 서버가 같이 바꾼다(컨트롤러 RPC). 캐릭터 파일은 고치지 않고 런타임에 조절한다.
 * - 마법 쪽은 IGrimAimProvider::GetCurrentAim 으로만 읽는다(UGrimTargetingStatics::FindAimProvider).
 * - 대상 표식은 임시로 디버그 선(속성색 링), 줌 조준점 · 대상 링 · 상태 글자는 디버그 캔버스. 출시 빌드에서는 그려지지 않는다 — 정식 위젯은 HUD 작업 때.
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

	/** Zoom(줌인 조준 중) · Locked · Soft · Free(대상 없음) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	EGrimAimMode GetAimMode() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting")
	bool IsLocked() const { return bLocked && CurrentTarget.IsValid(); }

	/**
	 * 고정 락온 토글(R3 · Caps Lock). 대상이 없으면 한 번 골라서 고정한다(소프트 락온이 꺼져 있어도).
	 * 줌 중이면 「떼고 나서 고정할 대상」만 바꾼다(잠시 풀린 고정이 있으면 버리고, 없으면 조준점 아래 대상을 고정 예약).
	 */
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

	// --- 줌인 조준 (R24) ---

	/** 줌인 조준 입력(IA_AimZoom 눌림 · 뗌). 누르기 / 토글 방식은 IsZoomToggleMode 를 따른다. 로컬 컨트롤러가 부른다 */
	void HandleZoomInput(bool bPressed);

	/** 입력 방식과 상관없이 줌인 조준을 켜고 끈다(콘솔 · 블루프린트 · 컷신 등). 로컬에서만 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting|Zoom")
	void SetZoomAiming(bool bAiming);

	/** 줌인 조준 중인가(로컬: 입력 상태, 서버: 소유 클라이언트가 알려 준 상태) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting|Zoom")
	bool IsZoomAiming() const { return bZoomActive; }

	/** 카메라 당김 정도 0~1(부드럽게 한 값). HUD 조준점이 이것으로 나타나고 사라진다 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting|Zoom")
	float GetZoomBlendAlpha() const { return ZoomBlend.GetEased(); }

	/** 「조준 누르기 / 토글」 옵션 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting|Zoom")
	void SetZoomToggleMode(bool bToggle) { bZoomToggleMode = bToggle; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting|Zoom")
	bool IsZoomToggleMode() const { return bZoomToggleMode; }

	/** 조준 감도 옵션(줌 중 시점 입력 배율) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Targeting|Zoom")
	void SetZoomLookSensitivityScale(float Scale) { ZoomLookSensitivityScale = FMath::Max(0.01f, Scale); }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Targeting|Zoom")
	float GetZoomLookSensitivityScale() const { return ZoomLookSensitivityScale; }

	/** 지금 시점 입력에 곱할 값(줌 중이면 조준 감도, 아니면 1). 컨트롤러의 AddYaw/PitchInput 이 쓴다 */
	float GetLookInputScale() const { return bZoomActive ? ZoomLookSensitivityScale : 1.f; }

	/** 서버: 소유 클라이언트가 RPC 로 알린 줌 상태(이동 감속 · 몸 방향 · State.Aiming 을 서버에서도 맞춘다) */
	void SetZoomAimingFromClient(bool bAiming);

	/** 콘솔 덤프용 한 줄 */
	FString Describe() const;

	/** 대상 · 모드가 바뀌었다(로컬). HUD 표식이 듣는다 */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Targeting")
	FGrimOnAimTargetChanged OnTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** 카메라 정면 트레이스(캐릭터 깊이에서 시작해 Range 만큼). 맞으면 true. OutEnd = 트레이스 끝 */
	bool TraceFromView(const FVector& ViewLocation, const FRotator& ViewRotation, float Range, FHitResult& OutHit, FVector& OutEnd) const;

	/** 줌 조준 트레이스: 조준점(맞은 곳 또는 끝)과, 맞은 액터(또는 그 부모)가 대상이 될 수 있으면 그 액터 */
	AActor* TraceZoomAim(const FVector& ViewLocation, const FRotator& ViewRotation, float Range, FVector& OutPoint, EGrimTargetKind& OutKind) const;

	void SetTarget(AActor* NewTarget, EGrimTargetKind NewKind, bool bNewLocked);
	void DrawTargetMarker() const;
	void DrawDebugCandidates() const;

	// 줌인 조준
	void SetZoomActive(bool bActive);
	void BeginZoomTargeting();
	void EndZoomTargeting();
	void UpdateZoomHover();
	bool CanRelock(const AActor* Target) const;
	/** 이동 속력 · 몸 방향 · State.Aiming (소유 클라이언트와 서버 양쪽) */
	void UpdateZoomPawnEffects(float DeltaTime);
	void BeginZoomPawnEffects(ACharacter* Character);
	void RefreshZoomPawnEffects(ACharacter* Character);
	void EndZoomPawnEffects(ACharacter* Character, float GuardSeconds);
	void SetAimingTag(ACharacter* Character, bool bAiming);
	/** 스프링암 · 시야각(로컬만) */
	void UpdateZoomCamera(float DeltaTime);
	void RestoreZoomCamera();
	void DrawZoomOverlay(UCanvas* Canvas, APlayerController* ViewPC);

	TWeakObjectPtr<AActor> CurrentTarget;
	EGrimTargetKind CurrentKind = EGrimTargetKind::None;
	bool bLocked = false;
	bool bSoftLockEnabled = true;
	EGrimTargetingReference Reference = EGrimTargetingReference::Camera;
	/** SetTarget 이 알림을 보낼 때마다 는다(줌 전환이 알림을 따로 보낼지 판단) */
	uint32 TargetBroadcastSerial = 0;

	// 줌인 조준 상태
	FGrimZoomInputState ZoomInput;
	bool bZoomActive = false;
	bool bZoomToggleMode = false;
	float ZoomLookSensitivityScale = 1.f;
	/** 줌 직전 고정 락온 대상 — 떼면 되돌린다 */
	TWeakObjectPtr<AActor> SuspendedLockTarget;
	EGrimTargetKind SuspendedLockKind = EGrimTargetKind::None;
	/** 디버그용 마지막 조준점 */
	FVector ZoomAimPoint = FVector::ZeroVector;

	// 줌 카메라(로컬)
	FGrimZoomBlend ZoomBlend;
	TWeakObjectPtr<ACharacter> CameraPawn;
	bool bCameraOverridden = false;
	float BaseArmLength = 0.f;
	FVector BaseSocketOffset = FVector::ZeroVector;
	float BaseFieldOfView = 90.f;

	// 줌 폰 효과(소유 클라이언트 · 서버)
	TWeakObjectPtr<ACharacter> EffectsPawn;
	TWeakObjectPtr<class UAbilitySystemComponent> AimingTagASC;
	bool bPawnEffectsApplied = false;
	FGrimZoomSpeedOverride SpeedOverride;
	FGrimZoomFlagOverride OrientOverride;
	FGrimZoomFlagOverride DesiredRotationOverride;

	FDelegateHandle ZoomOverlayHandle;

	float SelectionTimer = 0.f;
	FGrimOcclusionTimer Occlusion;
	FGrimLookFlickDetector LookFlick;

	/** 디버그 그리기용 마지막 후보(Grim.Targeting.Debug 1) */
	TArray<FGrimTargetCandidate> LastCandidates;
};
