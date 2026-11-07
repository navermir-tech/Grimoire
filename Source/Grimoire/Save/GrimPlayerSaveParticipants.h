#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/GrimSaveTypes.h"
#include "GameFramework/PlayerState.h" // SetPlayerState 가 TWeakObjectPtr 에 넣는다 — 전방 선언만으로는 유니티 묶음에 따라 깨진다
#include "GrimPlayerSaveParticipants.generated.h"

class UAbilitySystemComponent;
struct FGameplayAttribute;

/**
 * 세이브 시스템이 플레이어마다 붙이는 참여자들의 공통 뼈대 — 어느 시스템 컴포넌트에도 속하지 않는 몫(PlayerState 쪽).
 * 저장 · 불러오기 때마다 UGrimSaveSubsystem 이 잠깐 만들어 쓴다(상태 없음).
 */
UCLASS(Abstract)
class GRIMOIRE_API UGrimPlayerSaveParticipantBase : public UObject, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	void SetPlayerState(APlayerState* InPlayerState) { PlayerState = InPlayerState; }

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	// 하위 클래스가 덮는다(UObject 는 CDO 를 만들어야 해서 순수 가상으로 둘 수 없다).
	virtual FName GetSaveId() const override { return NAME_None; }
	virtual void WriteSaveData(FArchive& Ar) override {}
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override {}

protected:
	UAbilitySystemComponent* GetASC() const;

	TWeakObjectPtr<APlayerState> PlayerState;
};

/**
 * Player.Attributes — UGrimSaveSettings::PlayerAttributes 의 어트리뷰트 기본값(지금은 마나 현재값).
 * 형식 v1: int32 개수, (FString 이름, float 값) × 개수. 읽을 때 모르는 이름은 건너뛴다.
 */
UCLASS()
class GRIMOIRE_API UGrimPlayerAttributesSaveParticipant : public UGrimPlayerSaveParticipantBase
{
	GENERATED_BODY()

public:
	virtual FName GetSaveId() const override { return TEXT("Player.Attributes"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** "Mana" · "GrimMagicAttributeSet.Mana" 를 ASC 의 어트리뷰트 셋에서 찾는다. */
	static bool FindAttributeByName(const UAbilitySystemComponent* ASC, const FString& Name, FGameplayAttribute& OutAttribute);
};

/**
 * Player.Transform — 폰 위치 · 바라보는 방향. 다시 접속하면 그 자리에서(UGrimSaveSettings::bRestorePlayerTransform).
 * 형식 v1: bool 폰 있음, FVector 위치, float Yaw.
 */
UCLASS()
class GRIMOIRE_API UGrimPlayerTransformSaveParticipant : public UGrimPlayerSaveParticipantBase
{
	GENERATED_BODY()

public:
	virtual FName GetSaveId() const override { return TEXT("Player.Transform"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 폰이 이미 없을 때(퇴장 — Logout 전에 폰이 먼저 사라진다) 쓸 마지막 위치. 세이브 시스템이 매 틱 기억해 둔 값. */
	void SetFallback(const FVector& Location, float Yaw)
	{
		bHasFallback = true;
		FallbackLocation = Location;
		FallbackYaw = Yaw;
	}

private:
	bool bHasFallback = false;
	FVector FallbackLocation = FVector::ZeroVector;
	float FallbackYaw = 0.f;
};
