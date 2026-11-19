#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "World/GrimZoneTypes.h"
#include "GrimZoneSettings.generated.h"

/** 구역 이름 배너 한 가지 모양(처음 · 다시 × 큰 구역 · 소구역) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimZoneBannerStyle
{
	GENERATED_BODY()

	/** 거짓이면 이 경우엔 띄우지 않는다 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner")
	bool bEnabled = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float FadeInSeconds = 1.f;

	/** 다 나타난 뒤 머무는 시간 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float HoldSeconds = 3.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "0", Units = "s"))
	float FadeOutSeconds = 1.5f;

	/** 이름 글씨 크기(Slate 포인트) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "8"))
	int32 TitleSize = 44;

	/** 한 줄 설명 글씨 크기 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner", meta = (ClampMin = "8"))
	int32 SubtitleSize = 18;

	/** 한 줄 설명을 보이나(거짓이면 이름만) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner")
	bool bShowSubtitle = true;

	/** 이름 아래 가는 장식 줄을 긋나 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner")
	bool bShowRule = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Banner")
	FLinearColor TitleColor = FLinearColor(1.f, 0.9f, 0.7f, 1.f);

	float GetTotalSeconds() const { return FadeInSeconds + HoldSeconds + FadeOutSeconds; }
};

/**
 * 구역 이름 표시 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimZoneSettings] (docs/systems/world-zones.md).
 * 구역 이름 · 설명 · 경계는 여기가 아니라 레이아웃 Sections(→ Content/Grimoire/Data/WorldZones.json).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Zones"))
class GRIMOIRE_API UGrimZoneSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimZoneSettings* Get() { return GetDefault<UGrimZoneSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 구역 표를 한 번 읽어 둔다(문제는 로그 Error). 서버 · 클라이언트 모두 같은 파일을 읽는다 */
	static const FGrimZoneDatabase& GetDatabase();

	/** 다시 읽게 한다(콘솔 Grim.Zones.Reload) */
	static void ReloadData();

	/** 구역 표(Content 폴더 기준). 생성물 — 정본은 Scripts/Terrain/openbeta_layout.json 의 Sections */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	FString ZonesFile = TEXT("Grimoire/Data/WorldZones.json");

	/** 구역이 있는 맵(짧은 이름). 비우면 모든 맵 — 레이아웃 좌표는 지역 1(L_OpenBeta)의 것이라 다른 맵에서는 맞지 않는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TArray<FString> Maps;

	// ───── 들어섬 판단(서버) ─────

	/** 서버가 플레이어 자리를 보는 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Visit", meta = (ClampMin = "0.05", Units = "s"))
	float PollIntervalSeconds = 0.5f;

	/** 새 구역에 이만큼 머물러야 「들어섰다」로 친다 — 경계를 따라 걸을 때 깜빡이지 않게 */
	UPROPERTY(Config, EditAnywhere, Category = "Visit", meta = (ClampMin = "0", Units = "s"))
	float StableSeconds = 1.f;

	/** 같은 구역의 「다시 들어섬」 배너를 이 시간 안에는 또 띄우지 않는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Visit", meta = (ClampMin = "0", Units = "s"))
	float RevisitCooldownSeconds = 180.f;

	// ───── 배너(클라이언트) ─────

	/** 거짓이면 배너를 아예 띄우지 않는다(본 구역 기록은 그대로 쌓인다) */
	UPROPERTY(Config, EditAnywhere, Category = "Banner")
	bool bShowBanners = true;

	/** 배너 위쪽 끝 자리 — 화면 높이에 대한 비율(0 = 맨 위) */
	UPROPERTY(Config, EditAnywhere, Category = "Banner", meta = (ClampMin = "0", ClampMax = "0.9"))
	float BannerTopFraction = 0.12f;

	/** 기다리는 배너가 이보다 많으면 오래된 「다시 들어섬」부터 버린다 */
	UPROPERTY(Config, EditAnywhere, Category = "Banner", meta = (ClampMin = "1"))
	int32 MaxQueuedBanners = 3;

	/** 큰 구역에 처음 */
	UPROPERTY(Config, EditAnywhere, Category = "Banner")
	FGrimZoneBannerStyle RegionFirst;

	/** 큰 구역에 다시 */
	UPROPERTY(Config, EditAnywhere, Category = "Banner")
	FGrimZoneBannerStyle RegionRevisit;

	/** 소구역에 처음 */
	UPROPERTY(Config, EditAnywhere, Category = "Banner")
	FGrimZoneBannerStyle SectorFirst;

	/** 소구역에 다시 */
	UPROPERTY(Config, EditAnywhere, Category = "Banner")
	FGrimZoneBannerStyle SectorRevisit;

	const FGrimZoneBannerStyle& GetStyle(EGrimZoneTier Tier, bool bFirstVisit) const;
};
