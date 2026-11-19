#pragma once

#include "CoreMinimal.h"
#include "GrimZoneTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimZone, Log, All);

// 구역 이름 표시(Q130 — 젤다 · 엘든 링식) — docs/systems/world-zones.md.
// 정본은 Scripts/Terrain/openbeta_layout.json 의 Sections(이름 · 설명 · 경계). 게임은 그 생성물
// Content/Grimoire/Data/WorldZones.json 을 읽는다(Scripts/Terrain/export_zones.py). 하늘 글씨(AGrimDevLabel)와 같은 데이터다.
// 좌표: 「지도 좌표」(m, 남서 모서리 (0, 0), U = 동쪽, V = 북쪽). 월드(cm) X = 북쪽 = (V - Origin.Y) × 100, Y = 동쪽 = (U - Origin.X) × 100.

/** 구역의 층: 지역 1 의 큰 구역 / 그 안의 소구역 */
UENUM(BlueprintType)
enum class EGrimZoneTier : uint8
{
	/** 큰 구역 — 경계 = Polygon */
	Region,
	/** 소구역 — Center 가 든 Region 안을 가장 가까운 Center 로 나눈다(Radius 를 주면 그 안에서만) */
	Sector
};

/** 구역 하나(레이아웃 Sections 한 줄) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimZoneDef
{
	GENERATED_BODY()

	/** 레이아웃 이름(Sec_TowerCape · Sub_Yard) — 세이브 · 복제는 이 이름으로 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	EGrimZoneTier Tier = EGrimZoneTier::Region;

	/** 화면 · 지도에 보이는 이름 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FText Label;

	/** 게임에 보이는 한 줄(비면 Note) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FText Subtitle;

	/** 개발 설명(하늘 글씨) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FText Note;

	/** 가운데(지도 좌표 m) — 지도 이름표 자리 · 소구역 나누기 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FVector2D Center = FVector2D::ZeroVector;

	/** 소구역: 0 보다 크면 Center 에서 이 거리(m) 안에서만 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	float Radius = 0.f;

	/** 큰 구역의 경계(지도 좌표 m). 소구역은 비어 있다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FVector2D> Polygon;

	/** 소구역이 속한 큰 구역(읽을 때 Center 로 정한다). 큰 구역은 None */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Parent;

	/** 배너 · 지도에 쓸 한 줄 */
	const FText& GetDisplaySubtitle() const { return Subtitle.IsEmpty() ? Note : Subtitle; }
};

/** 구역 표 전체(데이터 파일 하나) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimZoneDatabase
{
	GENERATED_BODY()

	/** 월드 원점의 지도 좌표(m) — 레이아웃 WorldOrigin */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FVector2D WorldOrigin = FVector2D::ZeroVector;

	/** 레이아웃 순서 그대로(큰 구역 · 소구역 섞임) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FGrimZoneDef> Zones;

	bool IsEmpty() const { return Zones.Num() == 0; }
	int32 IndexOf(FName Name) const;
	const FGrimZoneDef* Find(FName Name) const;
};

/** 한 자리의 구역(없으면 None) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimZoneHit
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Region;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	FName Sector;

	bool operator==(const FGrimZoneHit& Other) const { return Region == Other.Region && Sector == Other.Sector; }
	bool operator!=(const FGrimZoneHit& Other) const { return !(*this == Other); }
};

/** 순수 계산(Grimoire.World.Zones.* 테스트) */
struct GRIMOIRE_API FGrimZoneMath
{
	/**
	 * JSON 을 읽는다 — 생성물(WorldZones.json)도 레이아웃(openbeta_layout.json)도 같은 모양(WorldOrigin · Sections)이라 둘 다 된다.
	 * 이름 겹침 · 알 수 없는 Tier · 꼭짓점 3 미만의 큰 구역은 오류(읽은 만큼은 Out 에 남는다). 소구역의 Parent 도 여기서 정한다.
	 */
	static bool ParseJson(const FString& Text, FGrimZoneDatabase& Out, FString& OutError);

	/** 짝수-홀수 규칙. 꼭짓점 3 미만이면 거짓 */
	static bool IsPointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon);

	/** 지도 좌표 한 점의 큰 구역(Polygon 이 든 첫 것) · 소구역(그 큰 구역의 가장 가까운 Center) */
	static FGrimZoneHit FindAt(const FGrimZoneDatabase& Database, const FVector2D& MapPoint);

	static FVector2D WorldToMap(const FGrimZoneDatabase& Database, const FVector& WorldLocation);
	static FVector MapToWorld(const FGrimZoneDatabase& Database, const FVector2D& MapPoint, double WorldZ = 0.0);
};

/** 들어섬 하나 — 배너로 알릴 것 */
struct FGrimZoneEntered
{
	FName Zone;
	bool bFirstVisit = false;
};

/**
 * 한 플레이어의 「어디에 들어섰나」 판단(서버, 순수 계산 — Grimoire.World.Zones.Visit 테스트).
 *  - 새 자리(큰 구역 · 소구역)에 StableSeconds 동안 머물러야 들어선 것으로 친다(경계에서 깜빡임 막기).
 *  - 들어서면 큰 구역 → 소구역 순으로 알린다. 처음이면 bFirstVisit, Discovered 에 더한다.
 *  - 다시 들어섬은 그 구역을 마지막으로 알린 지 RevisitCooldown 이 지나야 알린다(처음은 늘 알린다).
 *  - 구역 밖(None)으로 나가는 것은 알리지 않는다.
 */
struct GRIMOIRE_API FGrimZoneVisitTracker
{
	/** 본 구역(세이브 · 복제되는 목록과 같은 것) */
	TArray<FName> Discovered;

	/** 들어선 것으로 친 지금 자리 */
	FGrimZoneHit Current;

	/** Hit 을 넣고 들어섬을 OutEntered 에 더한다. 반환 = Discovered 가 바뀌었나 */
	bool Step(const FGrimZoneHit& Hit, double Now, double StableSeconds, double RevisitCooldown, TArray<FGrimZoneEntered>& OutEntered);

	/** 본 구역을 합친다(세이브에서 읽은 것 — 지금 목록을 지우지 않는다). 반환 = 바뀌었나 */
	bool MergeDiscovered(const TArray<FName>& Names);

	/** 모두 잊는다(개발 콘솔 — 배너를 다시 보려고) */
	void Reset();

private:
	FGrimZoneHit Pending;
	double PendingSince = 0.0;
	bool bHasPending = false;
	TMap<FName, double> LastAnnounced;

	void Enter(FName Zone, double Now, double RevisitCooldown, TArray<FGrimZoneEntered>& OutEntered, bool& bOutChanged);
};
