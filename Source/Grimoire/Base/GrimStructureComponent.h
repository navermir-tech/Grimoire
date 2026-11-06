#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GrimSaveTypes.h"
#include "Base/GrimBaseTypes.h"
#include "GrimStructureComponent.generated.h"

class AGrimBaseCore;
class UGrimStructureComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FGrimOnStructureChanged, UGrimStructureComponent* /*Structure*/);

/**
 * 지은 것(건축 조각 · 시설 · 거점 핵)의 내구도 (R18 · R30, docs/systems/m4-plan 「내구도 · 수리」).
 *  - 조각 ID(BuildPieces.csv 행)와 체력을 복제한다. 바꾸는 일은 서버만.
 *  - 서버 틱(StructureTickSeconds): 노후(거점 안 느리게 · 밖 빠르게), 나무면 State.Burning 세기 × FireDamagePerSecond.
 *  - 피해는 ApplyStructureDamage 하나로(습격 · 불 · 노후 · 콘솔). 0 이 되면 잔해(비용 × RubbleFraction)를 떨구고 액터를 없앤다.
 *  - 수리: 재료 1개 = 최대 × RepairFractionPerMaterial. 재료 빼기는 부르는 쪽(「수리」 마법 · 거점 수리).
 *  - 세이브: World 「Structure.<액터 이름>」 v1(조각 ID · 체력). 지은 액터는 스스로 참여자라 클래스 · 위치와 함께 다시 스폰된다.
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimStructureComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimStructureComponent();

	static UGrimStructureComponent* FindOn(const AActor* Actor);

	/** 서버: 스폰 직후(FinishSpawning 전) 조각 ID 를 정한다 — 체력은 가득 */
	void InitializePiece(FName InPieceId);

	FName GetPieceId() const { return PieceId; }
	const FGrimBuildPieceDefinition* GetDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Base")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Base")
	float GetMaxHealth() const;

	bool IsDamaged() const { return GetHealth() < GetMaxHealth() - KINDA_SMALL_NUMBER; }

	/** 수리 재료 아이템(없으면 None — 못 고침) */
	FName GetRepairItem() const;

	/** 가득 채우는 데 드는 재료 수 */
	int32 GetMaterialsNeeded() const;

	/** 이 액터가 들어 있는 거점(없으면 nullptr) */
	AGrimBaseCore* FindBaseCore() const;

	// ───── 서버 ─────

	/** 피해(설정 StructureDamageScale 이 곱해진다). 0 이 되면 부서진다. */
	void ApplyStructureDamage(float Amount, const TCHAR* Cause);

	/** 재료 Materials 개만큼 고친다(재료는 부르는 쪽이 이미 뺐다) */
	void ApplyRepairMaterials(int32 Materials);

	/** 콘솔 · 테스트 */
	void SetHealth(float NewHealth);

	/** 부서짐: 잔해를 떨구고(bDropRubble) 액터를 없앤다. 받침이 사라진 조각은 거점 서브시스템이 차례로 무너뜨린다. */
	void Break(bool bDropRubble, const TCHAR* Cause);

	/** 비용 × Fraction 의 아이템 목록(잔해 · 철거 환급) */
	TArray<FGrimItemInstance> GetRefundItems(float Fraction) const;

	FGrimOnStructureChanged OnStructureChanged;

	// ───── 세이브 ─────

	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override;
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Structure();

private:
	void TickStructure();
	void SetHealthInternal(float NewHealth);

	UPROPERTY(ReplicatedUsing = OnRep_Structure)
	FName PieceId;

	/** 음수 = 가득(아직 정하지 않음) */
	UPROPERTY(ReplicatedUsing = OnRep_Structure)
	float Health = -1.f;

	FTimerHandle TickTimer;
	bool bBroken = false;
};
