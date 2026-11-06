#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Base/GrimBaseTypes.h"
#include "GrimBuilderComponent.generated.h"

class AGrimBaseCore;
class AGrimBuildGhost;
class AGrimFarmPlot;
class AGrimStorageChest;
class AGrimWorkbench;
class APawn;
class APlayerController;
class UGrimBaseWidget;
class UGrimBuilderComponent;
class UGrimInventoryComponent;
struct FHitResult;

/** 놓기 검사 결과 */
UENUM()
enum class EGrimBuildResult : uint8
{
	Ok,
	UnknownPiece,
	NoPawn,
	OutOfReach,
	/** 구조 조각인데 거점 핵이 없다 · 반경 밖 */
	OutsideBase,
	Occupied,
	NoSupport,
	NotEnoughMaterials,
	TooManyBases,
	TooCloseToBase,
	/** 사람 · 사물과 겹친다 */
	Blocked,
	NoGround,
};

/** 소유 클라이언트에 보내는 알림(화면 한 줄) */
UENUM()
enum class EGrimBaseNotice : uint8
{
	Placed,
	PlaceFailed,
	Demolished,
	NotOwner,
	Crafted,
	CraftFailed,
	Repaired,
	NothingToRepair,
	NeedRepairSpell,
	NoRepairMaterials,
	NoMana,
	BaseRepaired,
	Planted,
	NoSeed,
	Harvested,
	StillGrowing,
	StorageFull,
	BagFull,
	Stored,
	TookOut,
};

DECLARE_MULTICAST_DELEGATE_FourParams(FGrimOnBaseNotice, UGrimBuilderComponent* /*Builder*/, EGrimBaseNotice /*Notice*/, FName /*Item*/, int32 /*CountOrResult*/);

/** 로컬 건축 미리보기 */
struct GRIMOIRE_API FGrimBuildPreview
{
	bool bHasAim = false;
	EGrimBuildResult Result = EGrimBuildResult::OutsideBase;
	FName PieceId;
	TWeakObjectPtr<AGrimBaseCore> Core;
	FGrimBuildSlot Slot;
	FTransform Transform;
};

/**
 * 건축 · 거점 상호작용 (docs/systems/m4-plan). PlayerState 에 붙는다 — 거점 서브시스템(서버)이 런타임에 붙인다(이름 고정 "GrimBuilder").
 *  - 로컬(키 상태 — 침대 · 가방과 같은 방식): B 건축 모드, Z · X 조각 고르기, R 돌리기, 좌클릭 놓기, G 철거, F 상호작용, I 장비 탭.
 *    고스트(AGrimBuildGhost)는 로컬 전용, 유효 검사는 서버와 같은 CheckPlacement(복제된 조각 · 가방으로).
 *  - 서버 RPC: 놓기 · 철거 · 상호작용(문 · 밭) · 저장고 옮기기 · 제작 · 수리 — 서버가 거리 · 재료 · 격자 · 지지를 다시 검사한다.
 *  - 건축 모드는 State.BuildMode(복제 루스 태그) — 그리모어를 거두고(Q25) 시전 · 기본 공격을 막는다(CastBlockingTags).
 */
UCLASS(ClassGroup = (Grimoire), meta = (BlueprintSpawnableComponent))
class GRIMOIRE_API UGrimBuilderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrimBuilderComponent();

	static const FName ComponentName;

	static UGrimBuilderComponent* FindForActor(const AActor* Actor);

	APawn* GetPawn() const;
	UGrimInventoryComponent* GetBag() const;

	/** 로컬 소유 컨트롤러(리슨 호스트 · 소유 클라이언트). 없으면 nullptr */
	APlayerController* GetLocalOwningController() const;

	/** 세이브 시스템의 PlayerId(없으면 플레이어 이름) — 거점 주인 */
	FString GetOwnerKey() const;

	// ───── 건축 모드 · 미리보기 (로컬) ─────

	bool IsBuildMode() const { return bBuildMode; }
	void SetBuildMode(bool bOn);
	FName GetSelectedPiece() const;
	int32 GetSelectedIndex() const { return SelectedIndex; }
	void CyclePiece(int32 Step);
	void Rotate() { RotationQuarter = (RotationQuarter + 1) % 4; }
	int32 GetRotationQuarter() const { return RotationQuarter; }
	const FGrimBuildPreview& GetPreview() const { return Preview; }

	/** 상호작용 대상(로컬, 0.1초마다 고른다) */
	AActor* GetInteractTarget() const { return InteractTarget.Get(); }

	/** 인벤토리에서 시설 탭을 연다(로컬) */
	void OpenStation(AActor* Station);
	AActor* GetOpenStation() const { return OpenStationActor.Get(); }
	void CloseStation();

	// ───── 검사 (서버 · 로컬 공통) ─────

	/** 놓을 수 있나. 구조 조각은 Core + Slot, 시설 · 거점 핵은 FreeTransform. OutTransform = 실제로 놓을 변환. */
	EGrimBuildResult CheckPlacement(FName PieceId, const AGrimBaseCore* Core, const FGrimBuildSlot& Slot, const FTransform& FreeTransform,
		bool bCheckReach, FTransform& OutTransform) const;

	static FText DescribeResult(EGrimBuildResult Result);
	static FText DescribeNotice(EGrimBaseNotice Notice, FName Item, int32 CountOrResult);

	/** 「수리」 마법을 아는가(R28) — 설정이 끄면 늘 참 */
	bool KnowsRepairSpell() const;

	// ───── 서버 ─────

	EGrimBuildResult PlacePiece(FName PieceId, AGrimBaseCore* Core, const FGrimBuildSlot& Slot, const FTransform& FreeTransform, bool bCheckReach);
	bool Demolish(AActor* Target, bool bCheckReach);
	bool InteractWith(AActor* Target, bool bCheckReach);
	bool TransferStorage(AGrimStorageChest* Chest, bool bToChest, int32 SlotIndex, int32 Count, bool bCheckReach);
	bool Craft(AGrimWorkbench* Workbench, FName RecipeId, bool bCheckReach);
	bool PlantOrHarvest(AGrimFarmPlot* Plot);

	/**
	 * 「수리」(R18 · R28): 조각 · 시설 = 가방의 재료로 그것을, 작업대 = 내 장비를, 거점 핵 = 거점의 닳은 것을 저장고 재료로(마나 BaseRepairManaCost 더).
	 * bFromSpell = 마법 볼트가 맞힌 것(마법 마나는 이미 냈다). 아니면(탭 단추 · 콘솔) 「수리」 마법의 마나를 여기서 낸다. 고친 재료 수(못 고치면 0).
	 */
	int32 PerformRepair(AActor* Target, bool bFromSpell, bool bCheckReach);

	void SendNotice(EGrimBaseNotice Notice, FName Item = NAME_None, int32 CountOrResult = 0);

	FGrimOnBaseNotice OnNotice;

	// ───── RPC ─────

	UFUNCTION(Server, Reliable)
	void Server_SetBuildMode(bool bOn);

	UFUNCTION(Server, Reliable)
	void Server_PlacePiece(FName PieceId, AGrimBaseCore* Core, FGrimBuildSlot Slot, FTransform FreeTransform);

	UFUNCTION(Server, Reliable)
	void Server_Demolish(AActor* Target);

	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Target);

	UFUNCTION(Server, Reliable)
	void Server_TransferStorage(AGrimStorageChest* Chest, bool bToChest, int32 SlotIndex, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_Craft(AGrimWorkbench* Workbench, FName RecipeId);

	UFUNCTION(Server, Reliable)
	void Server_Repair(AActor* Target);

	/** 콘솔 명령을 서버에서(개발 빌드만). docs/systems/m4-plan 「콘솔」 */
	UFUNCTION(Server, Reliable)
	void ServerDebugCommand(const FString& Command, const FString& Arg, const TArray<int32>& Numbers);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Client, Reliable)
	void Client_Notice(EGrimBaseNotice Notice, FName Item, int32 CountOrResult);

private:
	bool HasAuthority() const;
	bool IsWithinReach(const AActor* Target, float Radius) const;
	bool TryLocalSetup();

	void HandleLocalInput();
	void UpdatePreview();
	void UpdateInteractTarget();
	void PlaceFromPreview();
	void DemolishAimed();
	bool TraceAim(FHitResult& OutHit) const;
	AActor* TraceAimedActor() const;

	/** 서버: 가방에 넣고 넘친 것은 바닥에 */
	void GiveOrDrop(const FGrimItemInstance& Item, const FVector& DropLocation);
	/** 서버: 마나를 낸다(모자라면 false) */
	bool SpendMana(float Amount);

	bool bBuildMode = false;
	int32 SelectedIndex = 0;
	int32 RotationQuarter = 0;
	FGrimBuildPreview Preview;
	TWeakObjectPtr<AActor> InteractTarget;
	TWeakObjectPtr<AActor> OpenStationActor;
	float InteractAccumulator = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<AGrimBuildGhost> Ghost;

	UPROPERTY(Transient)
	TObjectPtr<UGrimBaseWidget> Widget;

	bool bLocalSetupDone = false;
};
