#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimBaseSubsystem.generated.h"

class AGrimBaseCore;
class AGrimBuildPiece;
class APlayerState;
class UGrimStructureComponent;

/**
 * 거점 서브시스템 (서버 · 클라이언트). docs/systems/m4-plan.
 *  - 서버: PlayerState 가 스폰되면 건축(UGrimBuilderComponent) · 장비(UGrimEquipmentComponent) 컴포넌트를 붙인다(가방과 같은 방식).
 *  - 양쪽: 거점 핵 · 구조 조각 · 지은 것 목록(복제된 것을 클라이언트도 든다 — 고스트의 유효 검사).
 *  - 서버: 조각이 부서지면 다음 틱에 받침 없는 조각을 차례로 무너뜨린다.
 */
UCLASS()
class GRIMOIRE_API UGrimBaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGrimBaseSubsystem* Get(const UObject* WorldContext);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void RegisterCore(AGrimBaseCore* Core);
	void UnregisterCore(AGrimBaseCore* Core);
	void RegisterPiece(AGrimBuildPiece* Piece);
	void UnregisterPiece(AGrimBuildPiece* Piece);
	void RegisterStructure(UGrimStructureComponent* Structure);
	void UnregisterStructure(UGrimStructureComponent* Structure);

	TArray<AGrimBaseCore*> GetCores() const;
	TArray<AGrimBuildPiece*> GetPieces(const AGrimBaseCore* Core = nullptr) const;
	TArray<UGrimStructureComponent*> GetStructures() const;

	/** 위치가 들어 있는 거점(반경 안, 수평 거리). 여럿이면 가장 가까운 것. */
	AGrimBaseCore* FindBaseAt(const FVector& Location) const;

	/** 이 플레이어가 주인인 거점 수 */
	int32 CountCoresOwnedBy(const FString& OwnerId) const;

	/** 서버: 지은 것이 부서졌다 — 구조 조각이면 다음 틱에 무너짐 검사 */
	void NotifyStructureBroken(UGrimStructureComponent* Structure);

	/** 서버: 받침 없는 조각을 지금 무너뜨린다(잔해 떨굼). 무너진 수. */
	int32 CollapseUnsupported();

	/** 서버: PlayerState 에 건축 · 장비 컴포넌트를 붙인다(이미 있으면 그대로) */
	void EnsurePlayerComponents(APlayerState* PlayerState);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;

	TArray<TWeakObjectPtr<AGrimBaseCore>> Cores;
	TArray<TWeakObjectPtr<AGrimBuildPiece>> Pieces;
	TArray<TWeakObjectPtr<UGrimStructureComponent>> Structures;

	FDelegateHandle ActorSpawnedHandle;
	FTimerHandle CollapseTimer;
};
