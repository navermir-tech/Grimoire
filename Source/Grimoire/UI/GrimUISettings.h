#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "GrimUISettings.generated.h"

class UInputAction;
class UInputMappingContext;

/** HUD 막대 하나 — 어트리뷰트를 이름으로 읽는다(없으면 숨김). 생존 쪽 값은 읽기만 한다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimVitalBarConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FText Label;

	/** "Health" 또는 "GrimVitalAttributeSet.Health" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FString Attribute;

	/** 최대값 어트리뷰트(예 "MaxHealth"). 비우거나 없으면 MaxValue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FString MaxAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	float MaxValue = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FLinearColor Color = FLinearColor::White;

	/** 숫자 형식: 소수 자리 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0", ClampMax = "2"))
	int32 Decimals = 0;

	/** 숫자 뒤에 붙일 단위(예 "°C") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FString Unit;
};

/** 속성 표시(이름 · 색) — 그리모어 필터 · 슬롯 테두리 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimElementDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (Categories = "Element"))
	FGameplayTag Element;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FLinearColor Color = FLinearColor::White;
};

/**
 * HUD · 그리모어 화면 설정 — [/Script/Grimoire.GrimUISettings]. 위젯 트리는 C++(UGrimHUDWidget · UGrimGrimoireWidget)가 짠다.
 * 입력은 Content/Grimoire/UI 의 IA/IMC 를 UGrimUISubsystem 이 로컬 플레이어에 더한다(컨트롤러 파일을 고치지 않는다).
 * 슬롯 세트 · 칸 수는 마법서 쪽 UGrimSpellbookSettings(서버도 알아야 해서).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire UI"))
class GRIMOIRE_API UGrimUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimUISettings* Get() { return GetDefault<UGrimUISettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 속성 이름 · 색. 하위 태그(Element.Water.Ice)는 부모 항목을 따른다. 없으면 흰색 · 태그 이름. */
	const FGrimElementDisplay* FindElementDisplay(const FGameplayTag& Element) const;

	/** 서클 호칭(예 2 → 수습 마법사). 없으면 빈 글. */
	FText GetCircleTitle(int32 Circle) const;

	/** HUD 를 띄운다(콘솔 Grim.UI.HUD 0/1 로도) */
	UPROPERTY(Config, EditAnywhere, Category = "HUD")
	bool bShowHUD = true;

	/** HUD 를 띄우면 생존 디버그 글(grim.Survival.ShowDebug)을 끈다 — 콘솔로 다시 켤 수 있다. */
	UPROPERTY(Config, EditAnywhere, Category = "HUD")
	bool bHideSurvivalDebugWithHUD = true;

	UPROPERTY(Config, EditAnywhere, Category = "HUD", meta = (TitleProperty = "Label"))
	TArray<FGrimVitalBarConfig> VitalBars;

	/** 슬롯 줄 왼쪽에 숙련도 없는 항목(기본 공격, R26) 자리를 따로 둔다. 그런 항목은 슬롯에 넣지 않는다. */
	UPROPERTY(Config, EditAnywhere, Category = "HUD")
	bool bSeparateBasicAttackSlot = true;

	UPROPERTY(Config, EditAnywhere, Category = "HUD", meta = (ClampMin = "0.5", Units = "s"))
	float ToastSeconds = 3.f;

	UPROPERTY(Config, EditAnywhere, Category = "UI", meta = (TitleProperty = "Name"))
	TArray<FGrimElementDisplay> Elements;

	/** [0] = 1서클 호칭 */
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TArray<FText> CircleTitles;

	/** 최고 등급(S) 금테 · 글자 색 */
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	FLinearColor MasterColor = FLinearColor(1.f, 0.78f, 0.25f);

	/** 마법 실패 알림 글 색(Q64) */
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	FLinearColor CastFailColor = FLinearColor(1.f, 0.6f, 0.5f);

	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputContext;

	/** 그리모어 화면 열고 닫기 */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> ToggleGrimoireAction;

	/** 슬롯 세트 1, 2, … 로 바꾸기(호그와트 레거시 PC = F1~F4). 세트 수(UGrimSpellbookSettings)보다 많으면 남는 것은 무시 */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TArray<TSoftObjectPtr<UInputAction>> SlotSetActions;

	/** 매핑 컨텍스트 우선순위(컨트롤러의 기본 컨텍스트는 0) */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	int32 InputPriority = 1;
};
