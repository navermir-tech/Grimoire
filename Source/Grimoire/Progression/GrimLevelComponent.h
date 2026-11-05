#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Core/GrimSaveTypes.h"
#include "Progression/GrimProgressionTypes.h"
#include "Magic/GrimSpellbookComponent.h"
#include "GrimLevelComponent.generated.h"

class AGrimPlayerState;
class APawn;
class UAbilitySystemComponent;
class UGrimLevelComponent;
class UGrimProgressionComponent;
class UGrimSpellDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnLevelChanged, UGrimLevelComponent*, LevelComponent, int32, NewLevel, int32, OldLevel);

/**
 * 캐릭터 레벨 (R27) — AGrimPlayerState 의 하위 객체. 서클과는 무관하다(Q30): 레벨은 최대 HP · 최대 마나 · 성장 트리 포인트만 정한다.
 *  - 누적 경험(TotalXP)이 정본이고, 레벨은 레벨 곡선 표(UGrimLevelSettings::GetCurve)로 계산한다. 상한(LevelCap)에서 경험이 멈춘다.
 *  - 최대 HP · 마나: 레벨 2 ~ 지금 레벨에서 받은 만큼을 무한 GE(UGrimGE_ProgressionBonus) 하나로 더한다. 레벨이 바뀌면 새 GE 로 바꿔 건다.
 *  - 포인트: 표의 SkillPoints 합 + BonusSkillPoints(퀘스트 보상 등). 쓴 포인트는 UGrimSkillTreeComponent 가 센다.
 *  - 경험 출처(서버): AwardExperience(다른 갈래가 부른다), 스킬북으로 새 마법 배움 · 서클 승급(델리게이트 구독), 새 기후 지역 첫 방문(주기 확인),
 *    상호작용 규칙 첫 발견(NotifyRuleDiscovered — 속성 상태 쪽 이벤트가 생기면 잇는다).
 * 세이브: Scope = Player, Id = "Progression.Level" — 누적 경험 · 보너스 포인트 · 발견 기록.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimLevelComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimLevelComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 액터(PlayerState)에서 찾는다 */
	static UGrimLevelComponent* FindFor(const AActor* Owner);

	UFUNCTION(BlueprintPure, Category = "Grimoire|Level")
	int32 GetLevel() const { return Level; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Level")
	float GetTotalXP() const { return TotalXP; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Level")
	int32 GetMaxLevel() const;

	/** 지금 레벨 안의 진행(UI 막대) */
	FGrimLevelProgress GetProgress() const;

	/** 받은 포인트 합(레벨 표 + 보너스). 남은 포인트는 UGrimSkillTreeComponent::GetAvailablePoints. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Level")
	int32 GetTotalSkillPoints() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Level")
	int32 GetBonusSkillPoints() const { return BonusSkillPoints; }

	/** 레벨이 주는 최대 HP · 마나(지금 레벨까지의 합) */
	float GetMaxHealthBonus() const;
	float GetMaxManaBonus() const;

	/**
	 * 서버: 경험치를 준다 — 다른 갈래(퀘스트 보상 · 사냥 · 제작)가 부른다. 같은 출처를 짧은 사이에 반복하면 덜 준다
	 * (UGrimLevelSettings::Sources 의 RepeatDecay · RepeatWindowSeconds, 규칙이 없으면 Default*). 실제로 준 양을 돌려준다.
	 * 레벨이 오르면 OnLevelChanged(서버 · 모든 클라이언트 — 복제).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Level")
	float AwardExperience(float Amount, FGameplayTag Source);

	/** 서버: 출처 규칙의 DefaultXP 로 AwardExperience */
	float AwardDefaultExperience(FGameplayTag Source);

	/**
	 * 서버: 「처음 한 번」 경험 — 같은 (출처, Key) 는 한 번만(세이브에 남는다). 처음이면 출처의 DefaultXP 를 주고 true.
	 * 새 지역(RegionDiscovery, 지역 id) · 상호작용 규칙(RuleDiscovery, 규칙 id) · 유적 …
	 */
	bool AwardDiscovery(FGameplayTag Source, FName Key);

	bool HasDiscovered(FGameplayTag Source, FName Key) const;

	/**
	 * 서버: 상호작용 규칙을 처음 일으켰다(도감) — AwardDiscovery(RuleDiscovery, RuleId).
	 * TODO(통합): 속성 상태 컴포넌트가 「규칙 발동 + 일으킨 플레이어」 이벤트를 주지 않는다 — 생기면 거기서 부른다.
	 */
	bool NotifyRuleDiscovered(FName RuleId);

	/** 서버: 레벨 표 밖의 포인트(퀘스트 보상 등). 음수면 뺀다(0 밑으로는 안 감). */
	void AddBonusSkillPoints(int32 Points);

	/** 서버: 누적 경험을 바로 정한다(상한에서 자른다) — 세이브 · 콘솔 */
	void SetTotalXP(float NewTotalXP);

	/** 서버: 레벨을 바로 정한다(그 레벨 문턱의 경험) — 콘솔 Grim.Level.Set */
	void SetLevel(int32 NewLevel);

	/** 개발용: 클라이언트 콘솔의 서버 경로. 출시 빌드에서는 아무것도 안 한다. */
	UFUNCTION(Server, Reliable)
	void Server_DebugAddXP(float Amount, FGameplayTag Source);

	UFUNCTION(Server, Reliable)
	void Server_DebugSetLevel(int32 NewLevel);

	/** 레벨이 바뀌었다(서버: 바로, 클라이언트: 복제 콜백). UI 레벨업 알림 · 성장 트리 포인트 갱신. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Level")
	FGrimOnLevelChanged OnLevelChanged;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Progression.Level"); }
	virtual int32 GetSaveVersion() const override { return 1; }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn);

	UFUNCTION()
	void HandleLearnResult(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, EGrimLearnResult Result);

	UFUNCTION()
	void HandleCircleUpResult(UGrimProgressionComponent* Progression, int32 Circle, bool bSuccess);

	/** 서버 타이머: 폰이 선 기후 지역 → 첫 방문이면 경험 */
	void CheckRegion();

	/** 서버: 경험 → 레벨 다시 계산, 바뀌면 GE · 알림 */
	void RefreshLevel();

	/** 서버: 레벨 보너스 GE 를 지금 레벨에 맞춘다 */
	void ApplyLevelBonus();

	UAbilitySystemComponent* GetASC() const;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	static FString MakeDiscoveryKey(FGameplayTag Source, FName Key);

	UPROPERTY(ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(Replicated)
	float TotalXP = 0.f;

	UPROPERTY(Replicated)
	int32 BonusSkillPoints = 0;

	/** 서버 전용: "출처|키" — 처음 한 번 경험 */
	TSet<FString> Discoveries;

	/** 서버 전용: 같은 출처 반복 */
	FGrimXPRepeatTracker RepeatTracker;

	/** 서버 전용: 지금 걸린 레벨 보너스 GE · 그 레벨 */
	FActiveGameplayEffectHandle LevelBonusHandle;
	int32 AppliedBonusLevel = 1;

	FTimerHandle RegionTimer;
	bool bRegionSeeded = false;
	bool bBoundSources = false;

	FDelegateHandle ActorInfoReadyHandle;
};
