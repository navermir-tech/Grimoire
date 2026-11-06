#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "Base/GrimBaseInteractable.h"
#include "Base/GrimBaseTypes.h"
#include "GrimBaseCore.generated.h"

class UGrimStructureComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * 거점 핵(마법진) — 놓으면 반경(UGrimBaseSettings::BaseRadius) 안이 거점이고, 이 위치 · 방향이 건축 격자의 원점이다(Q33).
 * 주인은 PlayerId(세이브 시스템의 것, 없으면 플레이어 이름). M4 는 파티 공용 — 누구나 짓고 고친다(권한 UI 는 다음).
 * F = 인벤토리 「거점」 탭(조각 수 · 닳은 조각 · 거점 수리).
 * 세이브: 스스로 참여자(World 「BaseCore.<이름>」 v1 — 주인) → 클래스 · 위치와 함께 다시 스폰. 내구도는 구조 컴포넌트.
 */
UCLASS()
class GRIMOIRE_API AGrimBaseCore : public AActor, public IGrimSaveParticipant, public IGrimBaseInteractable
{
	GENERATED_BODY()

public:
	AGrimBaseCore();

	static const FName TabId;

	FGrimBuildGrid GetGrid() const;

	/** 수평 거리로 반경 안인가 */
	bool IsInside(const FVector& Location, float Slack = 0.f) const;

	const FString& GetOwnerId() const { return OwnerId; }
	const FString& GetOwnerName() const { return OwnerName; }

	/** 서버: 스폰 직후 */
	void SetOwnerInfo(const FString& InOwnerId, const FString& InOwnerName);

	UGrimStructureComponent* GetStructure() const { return Structure; }

	// IGrimBaseInteractable
	virtual FText GetInteractPrompt(const APawn* Pawn) const override;
	virtual void InteractLocal(UGrimBuilderComponent* Builder) override;
	virtual FName GetStationTabId() const override { return TabId; }

	// 세이브
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pedestal;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Crystal;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Ring;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;

	UPROPERTY(Replicated)
	FString OwnerId;

	UPROPERTY(Replicated)
	FString OwnerName;
};
