#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Math/RandomStream.h"
#include "Environment/GrimClimateTypes.h"
#include "GrimWeatherSubsystem.generated.h"

class AGrimClimateRegionVolume;
class AGrimWeatherReplicator;
class APawn;
class UDataTable;

/**
 * 더위 · 추위 정수 등급 (R25). 지역 · 시간대 · 날씨가 정하고, 생존 쪽이 장비 저항 합과 비교한다
 * (docs/spec/survival 「체온 모델」). 둘 다 0 이상이고 보통 한쪽만 0 보다 크다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimClimateExposure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 Heat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 Cold = 0;
};

/**
 * 등급을 몫별로 나눈 것. 생존 쪽이 「지붕 있는 실내는 날씨 몫을 뺀다」에 쓴다:
 * IsSheltered 이면 Sheltered, 아니면 Total(= GetClimateExposure).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimClimateExposureBreakdown
{
	GENERATED_BODY()

	/** 지역 · 시간대 몫(반올림, 서로 깎지 않음) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	FGrimClimateExposure Region;

	/** 날씨 몫(반올림, 서로 깎지 않음) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	FGrimClimateExposure Weather;

	/** 지역 + 날씨, 더위와 추위를 서로 깎은 값 */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	FGrimClimateExposure Total;

	/** 지역 몫만(실내), 서로 깎은 값 */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	FGrimClimateExposure Sheltered;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGrimOnRegionWeatherChanged, FName, RegionId, EGrimWeatherType, NewWeather, EGrimWeatherType, OldWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnLightningStrike, FVector, Location);

/**
 * 기후 · 날씨 (R04, docs/systems/environment).
 * - 지역: 기후 표(ClimateRegions.csv) 한 행 = 지역 하나. 레벨의 AGrimClimateRegionVolume 이 지역 영역, 밖은 기본 지역. 경계에서 섞인다.
 * - 날씨: 지역마다 상태 머신 하나(서버) — 마르코프 전이 · 최소 지속 · 블렌드. AGrimWeatherReplicator 가 바뀔 때만 복제.
 * - 서버 훅: 비 → 야외 속성 사물 · 폰에 물 자극(플레이어 둘레 오버랩, 상한), 뇌우 → 낙뢰(빛 자극).
 * - 조회(서버 · 클라이언트 어디서나): 기온 · 등급 · 지붕 · 날씨 섞임 · 시야.
 */
UCLASS()
class GRIMOIRE_API UGrimWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// ---- 기온 · 등급 (생존 쪽이 부른다 — 이름 · 시그니처 유지)

	/** 그 위치의 주변 기온(섭씨): 지역 기온 곡선 + 날씨 기온 가산, 지역 경계에서 섞음. 서버 · 클라이언트 어디서나. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetAmbientTemperature(FVector Location) const;

	/**
	 * 그 위치의 더위 · 추위 등급: 지역 · 시간대 + 날씨 몫(젖음 · 열원 · 지붕 같은 개인 몫은 생존 쪽이 더한다).
	 * 더위와 추위는 서로 깎는다. 실내(IsSheltered)면 GetClimateExposureBreakdown 의 Sheltered 를 쓴다.
	 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	FGrimClimateExposure GetClimateExposure(FVector Location) const;

	/** 등급을 지역 몫 · 날씨 몫으로 나눠 준다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	FGrimClimateExposureBreakdown GetClimateExposureBreakdown(FVector Location) const;

	// ---- 지붕

	/**
	 * Location 에서 위로 ShelterTraceDistance 만큼 트레이스(월드 스태틱 · 다이내믹)해 막히면 true — 지붕 있는 곳.
	 * Location 은 대상 위(머리 위 · 사물 꼭대기)여야 한다. 대상 자신은 IgnoreActor 로 뺀다.
	 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather", meta = (AdvancedDisplay = "IgnoreActor"))
	bool IsSheltered(FVector Location, const AActor* IgnoreActor = nullptr) const;

	/** 액터 충돌 바운드 꼭대기에서 IsSheltered(자신 · 붙은 액터는 뺀다). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	bool IsActorSheltered(const AActor* Actor) const;

	// ---- 날씨 조회

	/** 그 위치에서 가장 큰 날씨. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	EGrimWeatherType GetWeatherAt(FVector Location) const;

	/** 그 위치에서 날씨 Type 의 섞임 비율(0~1). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetWeatherWeightAt(FVector Location, EGrimWeatherType Type) const;

	/** 그 위치의 날씨 공통 값을 섞임 비율로 섞은 것(시야 · 비 양 · 연출). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	FGrimWeatherProfile GetBlendedProfileAt(FVector Location) const;

	/** 비 양(0~1). 뇌우 포함 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetRainAmountAt(FVector Location) const;

	/** 시야 거리(cm) — 안개 · 비 · 모래폭풍에서 준다. AI 감지 · UI 표식 쪽이 읽는다 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetVisibilityDistance(FVector Location) const;

	/** 소프트 락온 사거리 배율(1 = 그대로). 조준 쪽이 읽는다 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetSoftLockRangeScale(FVector Location) const;

	/** 그 위치에서 가중치가 가장 큰 지역 id. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	FName GetRegionAt(FVector Location) const;

	/** 지역 날씨 상태. 없는 지역이면 false. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	bool GetRegionWeather(FName RegionId, FGrimRegionWeather& OutWeather) const;

	const TArray<FGrimRegionWeather>& GetAllRegionWeather() const { return RegionWeather; }
	const FGrimClimateRow* FindRegionRow(FName RegionId) const;
	FName GetDefaultRegionId() const;

	/** 기후 표 CSV 를 읽어 행 이름 · 행을 표 순서대로 더한다(테스트도 쓴다). 행이 하나도 없으면 false. */
	static bool LoadClimateRowsFromCSV(const FString& FilePath, TArray<FName>& OutIds, TArray<FGrimClimateRow>& OutRows, TArray<FString>& OutProblems);
	static void CopyClimateRows(const UDataTable* Table, TArray<FName>& OutIds, TArray<FGrimClimateRow>& OutRows);

	/** C++: 그 위치의 날씨 섞임(지역 블렌드 × 시간 블렌드). */
	FGrimWeatherMix GetWeatherMixAt(const FVector& Location) const;

	// ---- 서버 조작

	/** 서버 전용. 지역 날씨를 바꾼다(허용되지 않는 날씨는 가까운 것으로). BlendSeconds < 0 이면 설정값. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Weather")
	bool SetRegionWeather(FName RegionId, EGrimWeatherType NewWeather, float BlendSeconds = -1.f);

	/** 서버 전용. 그 지역에 바로 낙뢰 하나(뇌우가 아니어도 — 시험용). 친 곳이 없으면 false. */
	bool ForceLightningStrike(FName RegionId);

	// ---- 볼륨 · 복제기에서

	void RegisterRegionVolume(AGrimClimateRegionVolume* Volume);
	void UnregisterRegionVolume(AGrimClimateRegionVolume* Volume);
	void ApplyReplicatedRegionWeather(const TArray<FGrimRegionWeather>& NewRegionWeather);
	void HandleLightningStrike(const FVector& Location);

	/** 지역 날씨가 바뀔 때(서버: 전이 시작, 클라이언트: 복제 도착). */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Weather")
	FGrimOnRegionWeatherChanged OnRegionWeatherChanged;

	/** 낙뢰 연출(모든 머신 — 멀티캐스트). */
	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Weather")
	FGrimOnLightningStrike OnLightningStrike;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FRegionSample
	{
		int32 RegionIndex = INDEX_NONE;
		float Weight = 0.f;
	};
	using FRegionSamples = TArray<FRegionSample, TInlineAllocator<8>>;

	/** 서버 전용 지역 진행 상태 */
	struct FRegionRuntime
	{
		double StateStartSeconds = 0.0;
		double NextCheckSeconds = 0.0;
		/** 0 = 아직 예약 안 함 */
		double NextLightningSeconds = 0.0;
	};

	void LoadClimateTable();
	void InitializeRegionWeather(double NowSeconds);
	int32 FindRegionIndex(FName RegionId) const;
	void GatherRegionWeights(const FVector& Location, FRegionSamples& OutSamples) const;
	float GetRegionWeightAt(int32 RegionIndex, const FVector& Location) const;

	bool IsServer() const;
	double GetServerSeconds() const;
	float GetTimeOfDay() const;
	float GetDayRatio() const;

	void UpdateStateMachine(double NowSeconds);
	void StartTransition(int32 RegionIndex, EGrimWeatherType NewWeather, float BlendSeconds, double NowSeconds);
	void Publish();
	void RunRainPass();
	void UpdateLightning(double NowSeconds);
	bool StrikeLightning(int32 RegionIndex);
	void CollectPlayerPawns(TArray<APawn*>& OutPawns) const;

	/** 기후 표 행(표 순서). 표가 없으면 환경 설정의 전역 곡선으로 만든 기본 지역 하나. */
	TArray<FName> RegionIds;
	TArray<FGrimClimateRow> Rows;
	int32 DefaultRegionIndex = 0;

	/** 지역 날씨(Rows 와 같은 순서). 서버가 정하고 클라이언트는 복제로 받는다. */
	TArray<FGrimRegionWeather> RegionWeather;
	TArray<FRegionRuntime> Runtime;

	TArray<TWeakObjectPtr<AGrimClimateRegionVolume>> Volumes;

	UPROPERTY()
	TObjectPtr<AGrimWeatherReplicator> Replicator;

	FRandomStream Random;
	double NextRainPassSeconds = 0.0;
	bool bReceivedReplication = false;
};
