#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Magic/GrimProficiencySettings.h"
#include "Core/GrimSaveTypes.h"
#include "GrimProgressionComponent.generated.h"

class UAbilitySystemComponent;
class UGrimSpellbookComponent;
class UGrimProgressionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnCircleUpResult, UGrimProgressionComponent*, Progression, int32, Circle, bool, bSuccess);

/**
 * 서클 승급 (R06 · Q122) — AGrimPlayerState 의 하위 객체. 서클 자체는 Circle 어트리뷰트(복제)가 갖는다.
 * 자격 = 서클 표(Circles.csv — UGrimProgressionSettings::GetCircleTable)의 숙련도 조건(등급별 마법 수 + 지금 서클 마법 하나 이상).
 * 마법탑 시험(Quests.csv 의 Trial)을 통과하면 Circle + 1. 자격이 모자라면 까닭을 Q64 실패 알림으로 보낸다(연출 없이 글만).
 * 서클이 오르면 마법서가 봉인된 책을 풀고 새 슬롯 세트를 채운다(UGrimSpellbookComponent::HandleCircleRaised).
 * 최대 마나는 서클에 묶지 않는다(레벨 — Q30). 사냥꾼의 「금지된 승급」(AbsorbMana, R09)도 나중에 같은 컴포넌트의 다른 경로로.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimProgressionComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimProgressionComponent();

	// 세이브 (IGrimSaveParticipant — Player 「Magic.Progression」 v1: Circle 기본값). 마법서보다 먼저 읽는다(설정 PlayerLoadOrder).
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Magic.Progression"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Progression")
	int32 GetCircle() const;

	/** 다음 서클 자격과 지금 값(서버 · 소유 클라이언트 — 숙련도가 소유자에게만 복제되므로). */
	FGrimCircleCheck EvaluateNextCircle() const;

	/** 서버: 자격이 모자란 까닭을 소유 클라이언트에 알린다(Q64 실패 알림 — 글만). 채웠으면 아무것도 하지 않는다 */
	void ReportPromotionFailure(const FGrimCircleCheck& Check) const;

	/** 서클 표의 그 서클 줄(표보다 높으면 마지막). 표가 비면 nullptr */
	static const FGrimCircleStep* GetCircleStep(int32 Circle);

	/** 서버: 조건이 맞으면(bForce 면 무시) 한 서클 올린다. 성공하면 true. */
	bool TryCircleUp(bool bForce = false);

	/** 클라이언트 → 서버: 승급 요청. bForce 는 개발 빌드에서만 듣는다. */
	UFUNCTION(Server, Reliable)
	void Server_RequestCircleUp(bool bForce);

	/** 어디서든: 서버면 바로, 아니면 RPC. */
	void RequestCircleUp(bool bForce);

	/** 승급 시도 결과(서버 · 소유 클라이언트) — HUD 알림. */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Progression")
	FGrimOnCircleUpResult OnCircleUpResult;

protected:
	UAbilitySystemComponent* GetASC() const;
	UGrimSpellbookComponent* GetSpellbook() const;

	UFUNCTION(Client, Reliable)
	void Client_CircleUpResult(int32 Circle, bool bSuccess);

	void NotifyResult(int32 Circle, bool bSuccess);
};
