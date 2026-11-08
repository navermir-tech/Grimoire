#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Items/GrimItemTypes.h"
#include "GrimItemSettings.generated.h"

/**
 * 아이템 · 가방 · 요리 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimItemSettings].
 * 아이템 · 요리 표는 Content/Grimoire/Data 의 CSV(런타임에 읽는다 — 퀘스트 표와 같은 방식).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Items"))
class GRIMOIRE_API UGrimItemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimItemSettings* Get() { return GetDefault<UGrimItemSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 표 두 개를 한 번 읽어 둔다. 문제는 로그(Error)에 남기고 읽은 만큼 쓴다. */
	static const FGrimItemDatabase& GetDatabase();

	/** 다시 읽게 한다(콘솔 Grim.Items.Reload) */
	static void ReloadData();

	/** Content 기준 경로의 파일을 읽는다 */
	static bool LoadContentFile(const FString& RelativePath, FString& OutText);

	// ───── 표 (Content 폴더 기준) ─────

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString ItemsCSV = TEXT("Grimoire/Data/Items.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Tables")
	FString RecipesCSV = TEXT("Grimoire/Data/Recipes.csv");

	// ───── 가방 ─────

	/** 플레이어 가방 칸 수(고정 — 늘리기 · 무게 제한은 M4) */
	UPROPERTY(Config, EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "200"))
	int32 PlayerSlotCount = 24;

	/**
	 * 소지품 화면의 탭 클래스(UGrimInventoryTab 하위, 「가방」 탭 다음에 GetOrder 순으로). 데이터로 등록 — 갈래마다 한 줄씩 더한다.
	 * 코드로는 UGrimInventoryWidget::RegisterTabClass.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Inventory", meta = (MetaClass = "/Script/Grimoire.GrimInventoryTab"))
	TArray<FSoftClassPath> InventoryTabClasses;

	/** 가방 화면 키. 호그와트 레거시 PC 배치: 소지품(Inventory) = L, 장비(Gear) = I — 장비 화면(M4)에 I 를 남긴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey BagKey = EKeys::L;

	/** 줍기 · 냄비에 넣기 키 — 침대 · NPC 와 같이 「다가가 F」(마법 상호작용의 예외) */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InteractKey = EKeys::F;

	// ───── 줍기 · 버리기 ─────

	/** 다가가 F 로 주울 수 있는 거리(폰 위치 기준) */
	UPROPERTY(Config, EditAnywhere, Category = "Pickup", meta = (ClampMin = "0", Units = "cm"))
	float PickupRadius = 220.f;

	/** 서버가 줍기 · 넣기 요청을 받을 때 거리 여유(클라이언트 위치 오차) */
	UPROPERTY(Config, EditAnywhere, Category = "Pickup", meta = (ClampMin = "0", Units = "cm"))
	float ServerReachSlack = 150.f;

	/** 닿으면 저절로 줍기(겹침, 서버). 끄면 F 로만 */
	UPROPERTY(Config, EditAnywhere, Category = "Pickup")
	bool bAutoPickupOnOverlap = false;

	/** 버린 사람이 이 시간 동안은 저절로 줍지 않는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Pickup", meta = (ClampMin = "0", Units = "s"))
	float DropAutoPickupDelay = 2.f;

	/** 버리면 폰 앞 이 거리에 떨어진다 */
	UPROPERTY(Config, EditAnywhere, Category = "Pickup", meta = (ClampMin = "0", Units = "cm"))
	float DropDistance = 120.f;

	// ───── 요리 (R16) ─────

	/** 바닥의 날고기가 이 반경 안의 불(State.Burning — 모닥불 · 아궁이, 양초 · 폰 제외)로 익는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0", Units = "cm"))
	float CookHeatRadius = 170.f;

	/** 열을 보는 간격(서버 타이머) */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0.05", Units = "s"))
	float CookCheckInterval = 0.5f;

	/** 불 세기 1 을 넘는 만큼 빨라진다(세기 3 = 1 + 2 × 이 값 배) — 바람으로 키운 불은 요리가 빠르다 */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0"))
	float ExtraCookRatePerIntensity = 0.5f;

	/** 불 마법이 직접 닿으면 이 시간 동안 그 세기로 달궈진다 */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0", Units = "s"))
	float DirectFireHeatSeconds = 4.f;

	/** 이 세기보다 약한 불 자극은 달구지 못한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0"))
	float DirectFireMinIntensity = 0.3f;

	/** 냄비에 다가가 F 로 재료를 넣을 수 있는 거리 */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0", Units = "cm"))
	float PotInteractRadius = 260.f;

	/** 냄비 아래 불을 찾는 반경(냄비 위치 기준) */
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", meta = (ClampMin = "0", Units = "cm"))
	float PotHeatRadius = 150.f;

	// ───── 화면 ─────

	/** 「얻음 · 먹음」 알림 한 줄이 떠 있는 초 */
	UPROPERTY(Config, EditAnywhere, Category = "UI", meta = (ClampMin = "0", Units = "s"))
	float NoticeSeconds = 3.f;

	/** 가방 칸을 이 시간 안에 두 번 누르면 더블클릭(바로 사용). 기본값은 Windows 기본 더블클릭 시간 */
	UPROPERTY(Config, EditAnywhere, Category = "UI", meta = (ClampMin = "0.05", Units = "s"))
	float SlotDoubleClickSeconds = 0.5f;

	/** 묶음(#Meat) 의 화면 이름 — 냄비 안내 「고기 1/1」. 없으면 묶음 ID 그대로 */
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TMap<FName, FString> GroupDisplayNames;

	/** 묶음 화면 이름(없으면 ID) */
	static FString GetGroupDisplayName(FName Group);
};
