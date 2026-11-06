#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "Base/GrimBaseInteractable.h"
#include "Base/GrimBaseTypes.h"
#include "Creatures/GrimCreatureBox.h"
#include "Interaction/GrimElementalObject.h"
#include "Survival/GrimBed.h"
#include "GrimBaseStations.generated.h"

class UGrimElementalStateComponent;
class UGrimItemContainerComponent;
class UGrimStructureComponent;
class UStaticMeshComponent;

/** 지은 침대 — 기존 침대(잠 · 밤 건너뛰기 · 부활 지점)에 내구도를 붙였다. 세이브는 침대(Bed.<이름>) + 구조 컴포넌트. */
UCLASS()
class GRIMOIRE_API AGrimBaseBed : public AGrimBed
{
	GENERATED_BODY()

public:
	AGrimBaseBed();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;
};

/**
 * 지은 거점 생물 상자(R38 + M4) — 기존 상자(칸 · 풀어놓기 · 관리 화면 F)에 내구도를 붙였다. 세이브는 상자(CreatureBox.<이름>) + 구조 컴포넌트.
 * 부서지거나 헐리면 칸의 생물을 가까운 다른 상자 → 주인의 보관함 순으로 옮긴다. 둘 다 자리가 없으면 그 생물은 사라진다(경고 로그).
 */
UCLASS()
class GRIMOIRE_API AGrimBaseCreatureBox : public AGrimCreatureBox
{
	GENERATED_BODY()

public:
	AGrimBaseCreatureBox();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;
};

/**
 * 지은 모닥불 —기존 모닥불(불 마법으로 켜고 바람으로 키우고 비 · 물에 꺼짐, 열원)에 내구도를 붙였다.
 * 지붕 아래면 비에 꺼지지 않는다(날씨의 비 자극이 지붕 아래를 건너뛴다). 세이브: 스스로 참여자(BuiltCampfire.<이름>) + 속성 상태 + 구조.
 */
UCLASS()
class GRIMOIRE_API AGrimBaseCampfire : public AGrimCampfire, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	AGrimBaseCampfire();

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override {}
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override {}

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;
};

/** 시설 공통 뼈대: 몸체 메시 + 구조 컴포넌트, F = 인벤토리 탭(GetStationTabId). 스스로 세이브 참여자(다시 스폰). */
UCLASS(Abstract)
class GRIMOIRE_API AGrimStationBase : public AActor, public IGrimSaveParticipant, public IGrimBaseInteractable
{
	GENERATED_BODY()

public:
	AGrimStationBase();

	UGrimStructureComponent* GetStructure() const { return Structure; }

	// IGrimBaseInteractable
	virtual FText GetInteractPrompt(const APawn* Pawn) const override { return FText::GetEmpty(); }
	virtual void InteractLocal(UGrimBuilderComponent* Builder) override;

	// 세이브(하위 클래스가 덧쓴다)
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override {}
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override {}

protected:
	virtual void BeginPlay() override;

	/** 세이브 ID 앞머리 */
	virtual const TCHAR* GetSavePrefix() const { return TEXT("Station"); }

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;

	FLinearColor BodyColor = FLinearColor(0.45f, 0.3f, 0.15f);
};

/** 저장고(상자) — 칸(UGrimItemContainerComponent), F = 「저장고」 탭에서 가방과 옮기기. 철거 · 부서지면 내용물을 떨군다. */
UCLASS()
class GRIMOIRE_API AGrimStorageChest : public AGrimStationBase
{
	GENERATED_BODY()

public:
	AGrimStorageChest();

	static const FName TabId;

	UGrimItemContainerComponent* GetContainer() const { return Container; }

	virtual FText GetInteractPrompt(const APawn* Pawn) const override;
	virtual FName GetStationTabId() const override { return TabId; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual const TCHAR* GetSavePrefix() const override { return TEXT("Storage"); }

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimItemContainerComponent> Container;
};

/**
 * 작업대 — Recipes.csv 의 Station=Workbench 레시피를 가방 재료로 바로 만든다(대기열 · 일꾼은 M5 — 제안).
 * 「수리」 마법을 작업대에 걸면 닳은 장비를 가방의 재료로 고친다(R18 「작업대 = 수리 마법을 걸 자리」).
 */
UCLASS()
class GRIMOIRE_API AGrimWorkbench : public AGrimStationBase
{
	GENERATED_BODY()

public:
	AGrimWorkbench();

	static const FName TabId;
	static const FName StationId;

	virtual FText GetInteractPrompt(const APawn* Pawn) const override;
	virtual FName GetStationTabId() const override { return TabId; }

protected:
	virtual const TCHAR* GetSavePrefix() const override { return TEXT("Workbench"); }

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LegA;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LegB;
};

/**
 * 밭(2 × 2 m) — 씨앗을 심으면 자란다(Crops.csv). 조건: 물(속성 상태 젖음 → 물기 MoistureSeconds) · 빛(낮 또는 빛 마법) · 온도(추위 · 더위 등급).
 * 비 · 물 마법이 같은 물 자극으로 적신다(R16 — 급수 경로는 전부 속성 자극). 불이 붙으면 작물이 타 없어지고 재가 남는다(다음 작물 생장 +AshBonus).
 * F: 빈 밭이면 가방의 첫 씨앗을 심고, 다 자랐으면 거둔다. 세이브: CropPlot.<이름> v1(작물 · 생장 · 물기 · 재).
 */
UCLASS()
class GRIMOIRE_API AGrimFarmPlot : public AGrimStationBase
{
	GENERATED_BODY()

public:
	AGrimFarmPlot();

	FName GetCropId() const { return CropId; }
	float GetGrowth() const { return Growth; }
	float GetMoisture() const { return Moisture; }
	bool HasAsh() const { return bAsh; }
	bool IsMature() const;

	/** 서버 */
	bool Plant(FName InCropId);
	/** 서버: 거둔 것(아이템 · 개수)을 돌려준다. 다 자라지 않았으면 빈 배열 */
	TArray<FGrimItemInstance> Harvest();
	/** 서버: 생장을 Seconds 만큼 그 조건으로 진행(틱 · 콘솔) */
	void AdvanceGrowth(float Seconds);
	/** 지금 조건(서버) */
	FGrimCropConditions GatherConditions() const;

	virtual FText GetInteractPrompt(const APawn* Pawn) const override;
	virtual void InteractLocal(UGrimBuilderComponent* Builder) override;
	virtual void InteractServer(UGrimBuilderComponent* Builder, APawn* Pawn) override;

	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual const TCHAR* GetSavePrefix() const override { return TEXT("CropPlot"); }

	UFUNCTION()
	void OnRep_Crop();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlantMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimElementalStateComponent> ElementalState;

	UPROPERTY(ReplicatedUsing = OnRep_Crop)
	FName CropId;

	UPROPERTY(ReplicatedUsing = OnRep_Crop)
	float Growth = 0.f;

	UPROPERTY(Replicated)
	float Moisture = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Crop)
	bool bAsh = false;

private:
	void TickCrop();
	void RefreshPlant();
	void HandleElementalChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity);

	FTimerHandle CropTimer;
};
