#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Survival/GrimSurvivalModel.h"
#include "GrimSurvivalComponent.generated.h"

class AGrimBed;
class AGrimPlayerState;
class APawn;
class APlayerController;
class UAbilitySystemComponent;
class UCanvas;
struct FOnAttributeChangeData;

/** 잠 요청의 결과. 거부 이유는 소유 클라이언트에게 알린다(UI 문구는 UI 몫). */
UENUM(BlueprintType)
enum class EGrimSleepResult : uint8
{
	Started,
	AlreadySleeping,
	HostileNearby,
	TooFar,
	Downed,
	NotAllowed,
	Woke,
};

/** 질병 종류(최소 2종). */
UENUM(BlueprintType)
enum class EGrimDisease : uint8
{
	Stomach,
	Cold,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnSleepResult, EGrimSleepResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnNightSkipFade, float, FadeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrimOnSurvivalEvent);

/**
 * 생존 관리 (R03) — 초기 어트리뷰트, 주기 효과(배고픔 · HP · 체온 등급 · 피곤함 · 질병), 상태 태그 · 패널티 GE,
 * 잠 · 쓰러짐 · 부활. AGrimPlayerState 의 기본 하위 객체(그래서 소유 클라이언트가 서버 RPC 를 부를 수 있다).
 * 계산은 FGrimSurvivalModel, 수치는 UGrimSurvivalSettings.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimSurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimSurvivalComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── 읽기 (서버 · 클라이언트) ──

	/** 자기 폰 위치의 주변 기온(섭씨). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	float GetAmbientTemperature() const;

	/** 마지막 틱의 더위 · 추위 등급(날씨 몫 + 개인 몫). 소유 클라이언트에 복제 — HUD 의 「추위 +3」. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	FGrimClimateExposure GetClimateExposure() const { return ClimateExposure; }

	/** 지금 저항으로 본 모자람(HUD 의 「내 저항 +2」 옆). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	FGrimClimateExposure GetClimateDeficit() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	bool IsSleeping() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	bool IsDowned() const;

	/** 쓰러짐이 끝나 사망하기까지 남은 초(쓰러져 있지 않으면 0). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Survival")
	float GetDownedSecondsRemaining() const;

	/** 서버: 마지막으로 잔 침대 옆. 없으면 false(PlayerStart 로 부활). 세이브는 다음 묶음. */
	bool GetRespawnPoint(FTransform& OutTransform) const;

	// ── 서버 전용 ──

	/** 눕는다. Bed 가 없으면 그 자리(콘솔 · 기절). bForced = 기절(적 · 거리 검사 없음). */
	EGrimSleepResult StartSleeping(AGrimBed* Bed, bool bForced = false);
	void StopSleeping();

	/** UGrimSleepSubsystem 이 밤을 건너뛸 때: 피곤함 0, 건너뛴 시간 × 배고픔, 깨운다. */
	void ApplyNightSkip(float SkippedDays, float DayLengthSeconds);

	/** 밤 건너뛰기 페이드 시작을 소유 클라이언트에 알린다. */
	void NotifyNightSkipFade(float FadeSeconds);

	/** 날것 · 익힌 음식을 먹었다(음식 시스템이 부른다). 날것이면 배탈 확률. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Survival")
	void NotifyFoodEaten(bool bRaw);

	/** 질병을 건다(이미 있으면 그대로). 확률 굴림은 부르는 쪽. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Survival")
	void ApplyDisease(EGrimDisease Disease);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Survival")
	void CureAllDiseases();

	/** 시험용 저항 버프(장비 M4 전). 둘 다 0 이면 뗀다. */
	void SetTestClimateResist(float Heat, float Cold);

	/** 쓰러진 사람을 일으킨다(나중엔 빛 마법 치료 — 지금은 콘솔). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Survival")
	void Revive();

	/** HP 를 0 으로(시험용) — 쓰러짐으로 이어진다. */
	void ForceDown();

	/** 피곤함을 값으로(시험용). */
	void SetFatigue(float Value);

	// ── RPC ──

	/** 소유 클라이언트 → 서버: 침대에 눕기. Bed 가 없으면 반경 안 가장 가까운 침대, 그것도 없으면(개발 빌드) 그 자리. */
	UFUNCTION(Server, Reliable)
	void ServerRequestSleep(AGrimBed* Bed);

	UFUNCTION(Server, Reliable)
	void ServerRequestWake();

	/** 콘솔 명령을 서버에서(개발 빌드만 동작). */
	UFUNCTION(Server, Reliable)
	void ServerDebugCommand(const FString& Command, float A, float B);

	// ── 이벤트 ──

	/** 소유 클라이언트(와 서버): 잠 요청 결과. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Survival")
	FGrimOnSleepResult OnSleepResult;

	/** 소유 클라이언트: 밤 건너뛰기 페이드 시작 — 화면을 어둡게 하는 것은 UI 몫. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Survival")
	FGrimOnNightSkipFade OnNightSkipFade;

	/** 서버: 쓰러짐 · 사망(가방 떨굼은 인벤토리 M4 뒤) · 부활. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Survival")
	FGrimOnSurvivalEvent OnDowned;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Survival")
	FGrimOnSurvivalEvent OnDied;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Survival")
	FGrimOnSurvivalEvent OnRespawned;

protected:
	UFUNCTION(Client, Reliable)
	void ClientSleepResult(EGrimSleepResult Result);

	UFUNCTION(Client, Reliable)
	void ClientNightSkipFade(float FadeSeconds);

private:
	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);
	void InitializeAttributes(UAbilitySystemComponent* ASC);
	void SurvivalTick();

	FGrimClimateExposure GatherClimateExposure(const APawn* Pawn, bool bCheat) const;
	bool HasHeatSourceNearby(const APawn* Pawn) const;
	bool HasHostileNearby(const APawn* Pawn) const;
	AGrimBed* FindNearestBed(const APawn* Pawn) const;

	void UpdateStatusTags(UAbilitySystemComponent* ASC, const FGrimSurvivalStatus& Status);
	void UpdatePenaltyEffect(UAbilitySystemComponent* ASC, const FGrimSurvivalModifiers& Modifiers);
	/** 마지막 상태 + 지금 잠 · 쓰러짐으로 패널티를 다시 맞춘다(틱을 기다리지 않고). */
	void RefreshPenalty();
	void RollDisease(EGrimDisease Disease, float BaseChance);

	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMoveSpeedScaleChanged(const FOnAttributeChangeData& Data);
	/** 폰의 MaxWalkSpeed 에 MoveSpeedScale 을 반영한다(서버 · 소유 클라이언트). 비율로 바꿔 다른 감속과 겹친다. */
	void ApplyMoveSpeedScale();

	void EnterDowned();
	void HandleDownedExpired();
	void DieAndRespawn();
	void HandleCollapseFinished();

	void SetLooseTag(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, bool bOn);
	void NotifySleepSubsystem();
	void SendSleepResult(EGrimSleepResult Result);

	void DrawDebug(UCanvas* Canvas, APlayerController* PlayerController);

	UAbilitySystemComponent* GetASC() const;
	APawn* GetAvatarPawn() const;

	/** 마지막 틱의 등급(날씨 + 개인). 소유자에게만 복제. */
	UPROPERTY(Replicated)
	FGrimClimateExposure ClimateExposure;

	/** 쓰러짐이 끝나는 서버 월드 시각(0 = 쓰러져 있지 않음). */
	UPROPERTY(Replicated)
	float DownedEndServerTime = 0.f;

	FTimerHandle SurvivalTickTimer;
	FTimerHandle DownedTimer;
	FTimerHandle CollapseTimer;
	FDelegateHandle ActorInfoReadyHandle;
	FDelegateHandle DebugDrawHandle;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MoveSpeedChangedHandle;

	bool bAttributesInitialized = false;
	double LastTickWorldSeconds = 0.0;

	/** 젖은 채 추운 시간(감기 굴림 주기). */
	float WetColdSeconds = 0.f;

	FGrimSurvivalStatus LastStatus;
	FActiveGameplayEffectHandle PenaltyHandle;
	FGrimSurvivalModifiers AppliedModifiers;
	FActiveGameplayEffectHandle TestResistHandle;

	/** 지금 누운 침대(그 자리에서 자면 비어 있다). */
	TWeakObjectPtr<AGrimBed> CurrentBed;

	bool bHasRespawnPoint = false;
	FTransform RespawnTransform;

	/** MaxWalkSpeed 에 이미 곱해 둔 배율과 그 폰. */
	TWeakObjectPtr<APawn> SpeedScaledPawn;
	float AppliedMoveSpeedScale = 1.f;
};
