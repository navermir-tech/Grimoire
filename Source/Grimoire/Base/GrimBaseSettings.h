#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Base/GrimBaseTypes.h"
#include "GrimBaseSettings.generated.h"

class UGrimSpellDefinition;

/**
 * 거점 · 건축 · 농사 · 장비 내구도 설정 (docs/systems/m4-plan) — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimBaseSettings].
 * 조각 · 작물 · 장비 표는 Content/Grimoire/Data 의 CSV(런타임에 읽는다 — 아이템 표와 같은 방식).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Base"))
class GRIMOIRE_API UGrimBaseSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimBaseSettings* Get() { return GetDefault<UGrimBaseSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 표 셋을 한 번 읽어 둔다. 문제는 로그(Error). */
	static const FGrimBaseDatabase& GetDatabase();

	/** 다시 읽게 한다(콘솔 Grim.Base.Reload) */
	static void ReloadData();

	/** 거점 핵 위치 · 방향으로 격자를 만든다 */
	FGrimBuildGrid MakeGrid(const FVector& Origin, float Yaw) const;

	// ───── 표 (Content 기준) ─────

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString BuildPiecesCSV = TEXT("Grimoire/Data/BuildPieces.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString CropsCSV = TEXT("Grimoire/Data/Crops.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString EquipmentCSV = TEXT("Grimoire/Data/Equipment.csv");

	// ───── 거점 · 격자 (Q33) ─────

	/** 거점 반경(구조 조각은 이 안에만) */
	UPROPERTY(Config, EditAnywhere, Category = "Base", meta = (ClampMin = "300", Units = "cm"))
	float BaseRadius = 2500.f;

	/** 플레이어당 거점 수(Q10: 1 → 서클로 최대 3 — 서클 연동은 다음) */
	UPROPERTY(Config, EditAnywhere, Category = "Base", meta = (ClampMin = "1"))
	int32 MaxBasesPerPlayer = 1;

	/** 거점 핵끼리 떨어져야 하는 거리(반경이 겹치지 않게) */
	UPROPERTY(Config, EditAnywhere, Category = "Base", meta = (ClampMin = "0", Units = "cm"))
	float CoreSpacing = 5000.f;

	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "50", Units = "cm"))
	float CellSize = 300.f;

	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "50", Units = "cm"))
	float WallHeight = 300.f;

	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "1", Units = "cm"))
	float SlabThickness = 20.f;

	/** 0층 바닥 윗면 = 거점 핵 바닥 + 이만큼 */
	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (Units = "cm"))
	float FloorTopOffset = 30.f;

	/** 바닥 조각이 아래로 내려가는 깊이(낮은 경사를 덮는다) */
	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "1", Units = "cm"))
	float FoundationDepth = 100.f;

	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MaxLevel = 3;

	// ───── 조작 (PlayerController 키 상태 — 침대 · 가방과 같은 방식) ─────

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey BuildKey = EKeys::B;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey PrevPieceKey = EKeys::Z;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey NextPieceKey = EKeys::X;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey RotateKey = EKeys::R;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey PlaceKey = EKeys::LeftMouseButton;

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey DemolishKey = EKeys::G;

	/** 문 · 저장고 · 작업대 · 밭 · 거점 핵 */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InteractKey = EKeys::F;

	/** 장비 탭 바로 열기(호그와트 레거시 PC 의 장비 = I) */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey GearKey = EKeys::I;

	/** 건축 조준 거리(카메라에서) */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (ClampMin = "100", Units = "cm"))
	float BuildTraceDistance = 1500.f;

	/** 놓을 자리가 폰에서 이 안이어야 한다(서버 검사) */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (ClampMin = "100", Units = "cm"))
	float BuildReach = 1800.f;

	/** F 상호작용 거리(폰 → 대상의 가장 가까운 점) */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (ClampMin = "50", Units = "cm"))
	float InteractRadius = 300.f;

	/** 서버 거리 검사 여유(클라이언트 위치 오차) */
	UPROPERTY(Config, EditAnywhere, Category = "Input", meta = (ClampMin = "0", Units = "cm"))
	float ServerReachSlack = 200.f;

	// ───── 내구도 · 수리 (R18 · R28 · R30) ─────

	/** 재료 1개 = 최대 내구도의 이만큼 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0.01", ClampMax = "1"))
	float RepairFractionPerMaterial = 0.25f;

	/** 장비는 수리마다 최대치가 이만큼 준다 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0", ClampMax = "1"))
	float EquipmentMaxLossPerRepair = 0.1f;

	/** 장비 최대치 하한(처음의 비율) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0", ClampMax = "1"))
	float EquipmentMinMaxFraction = 0.5f;

	/** 노후 — 거점 안(실제 1시간에 최대치의 이만큼) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	float InsideDecayFractionPerHour = 0.004f;

	/** 노후 — 거점 밖(R30: 밖에 지은 것은 잠깐 쓰고 버린다) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	float OutsideDecayFractionPerHour = 0.5f;

	/** 서버 옵션: 노후 끄기 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability")
	bool bDecayEnabled = true;

	/** 서버 옵션: 건축 피해 배율 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	float StructureDamageScale = 1.f;

	/** 서버 옵션: 건축 최대 내구도 배율 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0.01"))
	float StructureMaxHealthScale = 1.f;

	/** 나무 조각이 탈 때 불 세기 1 당 초당 피해 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	float FireDamagePerSecond = 6.f;

	/** 노후 · 불 피해를 계산하는 간격(서버) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0.1", Units = "s"))
	float StructureTickSeconds = 2.f;

	/** 부서지면 비용의 이만큼을 잔해(픽업)로 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0", ClampMax = "1"))
	float RubbleFraction = 0.5f;

	/** 철거하면 비용의 이만큼을 가방으로 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0", ClampMax = "1"))
	float DemolishRefundFraction = 0.5f;

	/** 거점 수리(거점 핵) 마나 — 「마나 많이 듦」 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	float BaseRepairManaCost = 40.f;

	/** 「수리」 마법을 배워야 고칠 수 있다(R28) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability")
	bool bRequireRepairSpell = true;

	/** 「수리」 마법(무속성 1서클 볼트 — Scripts/Base/make_repair_spell.py) */
	UPROPERTY(Config, EditAnywhere, Category = "Durability")
	TSoftObjectPtr<UGrimSpellDefinition> RepairSpell;

	/** 방어구는 피해 이만큼마다 WearPerUse 씩 닳는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0.1"))
	float ArmorWearDamageUnit = 10.f;

	/** 장신구는 착용 이 초마다 WearPerUse 씩 닳는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "1", Units = "s"))
	float AccessoryWearIntervalSeconds = 60.f;

	/** 방어구 · 장신구가 부서지면 수리 재료를 이만큼 돌려준다 */
	UPROPERTY(Config, EditAnywhere, Category = "Durability", meta = (ClampMin = "0"))
	int32 BrokenEquipmentSalvage = 1;

	// ───── 농사 ─────

	UPROPERTY(Config, EditAnywhere, Category = "Crops", meta = (ClampMin = "0.1", Units = "s"))
	float CropTickSeconds = 5.f;

	// ───── 저장고 ─────

	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "1", ClampMax = "200"))
	int32 StorageSlotCount = 24;

	/** 저장고 · 시설이 열린 채로 있을 수 있는 거리(넘으면 탭이 닫힌다) */
	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "100", Units = "cm"))
	float StationOpenRadius = 450.f;

	// ───── 화면 ─────

	UPROPERTY(Config, EditAnywhere, Category = "UI", meta = (ClampMin = "0.5", Units = "s"))
	float NoticeSeconds = 3.f;
};
