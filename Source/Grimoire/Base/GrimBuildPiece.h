#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GrimSaveTypes.h"
#include "Base/GrimBaseInteractable.h"
#include "Base/GrimBaseTypes.h"
#include "GrimBuildPiece.generated.h"

class AGrimBaseCore;
class UGrimElementalStateComponent;
class UGrimStructureComponent;
class UStaticMeshComponent;

/**
 * 격자 구조 조각 — 바닥 · 벽 · 문 · 지붕 · 계단 · 기둥 (R30 큰 블록 · Q33 3 m).
 *  - 조각 ID(구조 컴포넌트)와 격자 자리(Slot)만 복제하고, 모양은 양쪽이 표 · 격자 설정으로 런타임 상자를 만든다(아트 전).
 *  - 콜리전은 WorldStatic 막음 — 날씨 지붕 판정(위로 트레이스)이 지붕 조각에 걸린다(R25 · R29).
 *  - 나무 조각은 속성 상태에 Material.Flammable — 불 마법에 붙어 타며 구조 컴포넌트가 내구도를 깎는다(R16 · R18).
 *  - 문은 F 로 열고 닫는다(문짝 콜리전 · 보임).
 *  - 세이브: 스스로 참여자(World 「BuildPiece.<이름>」 v1 — 자리 · 문) → 다시 스폰. 내구도 · 속성 상태는 컴포넌트 몫.
 */
UCLASS()
class GRIMOIRE_API AGrimBuildPiece : public AActor, public IGrimSaveParticipant, public IGrimBaseInteractable
{
	GENERATED_BODY()

public:
	AGrimBuildPiece();

	/** 서버: 스폰 직후(FinishSpawning 전) */
	void InitializePiece(FName PieceId, const FGrimBuildSlot& InSlot);

	const FGrimBuildSlot& GetSlot() const { return Slot; }
	FName GetPieceId() const;
	const FGrimBuildPieceDefinition* GetDefinition() const;
	UGrimStructureComponent* GetStructure() const { return Structure; }

	/** 들어 있는 거점(격자 원점) — 없으면 nullptr */
	AGrimBaseCore* GetBaseCore() const;

	bool IsDoor() const { return Slot.Category == EGrimBuildCategory::Door; }
	bool IsDoorOpen() const { return bDoorOpen; }

	/** 서버: 문 열고 닫기 */
	void SetDoorOpen(bool bOpen);

	// IGrimBaseInteractable — 문만
	virtual FText GetInteractPrompt(const APawn* Pawn) const override;
	virtual bool CanInteract(const APawn* Pawn) const override { return IsDoor(); }
	virtual void InteractLocal(UGrimBuilderComponent* Builder) override;
	virtual void InteractServer(UGrimBuilderComponent* Builder, APawn* Pawn) override;

	// 세이브
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Shape();

	UFUNCTION()
	void OnRep_DoorOpen();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimStructureComponent> Structure;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGrimElementalStateComponent> ElementalState;

	UPROPERTY(ReplicatedUsing = OnRep_Shape)
	FGrimBuildSlot Slot;

	UPROPERTY(ReplicatedUsing = OnRep_DoorOpen)
	bool bDoorOpen = false;

private:
	void RebuildShape();
	void ApplyDoorState();
	void ApplyHealthTint();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ShapeComponents;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorLeaf;

	FName BuiltForPiece;
	bool bRegistered = false;
};
