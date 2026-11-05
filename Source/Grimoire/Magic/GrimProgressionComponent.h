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
 * 서클 승급 (R06) — AGrimPlayerState 의 하위 객체. 서클 자체는 Circle 어트리뷰트(복제)가 갖는다.
 * 조건(UGrimProgressionSettings::CircleRequirements — 배운 마법 수 · 숙련도 등급 합)을 확인하고 Circle + 1.
 * 최대 마나는 서클에 묶지 않는다(캐릭터 레벨이 정한다 — 다음 묶음). 마법탑 시험(의식)은 튜토리얼 묶음에서 이 앞에 붙는다.
 * 사냥꾼의 「금지된 승급」(AbsorbMana, R09)도 나중에 같은 컴포넌트의 다른 경로로.
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

	/** 다음 서클 조건과 지금 값(서버 · 소유 클라이언트 — 숙련도가 소유자에게만 복제되므로). */
	FGrimCircleCheck EvaluateNextCircle() const;

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
