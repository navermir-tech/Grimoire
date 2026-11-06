# 환경 · 날씨 시스템

> 스펙: [세계 · 환경](../spec/world) (R04)
>
> 상태: M1 · M2(갈래 W) 구현됨(2026-11-06 코드 대조). 코드 `Source/Grimoire/Environment/`.

## 시간 `UGrimTimeOfDaySubsystem`

구현: `Source/Grimoire/Environment/GrimTimeOfDaySubsystem.h`, 복제 `Source/Grimoire/Environment/GrimTimeOfDayReplicator.h`, 순수 계산 `Source/Grimoire/Environment/GrimTimeOfDayMath.h`, 설정 `Source/Grimoire/Environment/GrimEnvironmentSettings.h`.

- `UTickableWorldSubsystem`(게임 · PIE 월드). 시각은 누적 일수 `TotalDays` — 정수부 = 날짜(`GetDayIndex`), 소수부 = 하루 비율(`GetTimeOfDay`, 0 = 자정, 0.5 = 정오).
- **복제**: 서버가 `OnWorldBeginPlay` 에서 보이지 않는 `AGrimTimeOfDayReplicator`(`AInfo`, 항상 관련)를 만들고 스냅숏 `FGrimTimeSnapshot { TotalDays, ServerSeconds, DayLengthSeconds, DayRatio }` 를 복제한다. 기반 GameState 는 고치지 않는다.
- **진행 · 외삽**: 모두가 같은 식 `TotalDays = 스냅숏.TotalDays + (GameState 서버 월드 시각 − 스냅숏.ServerSeconds) / 하루 길이` 로 계산한다(`FGrimTimeOfDayMath::ExtrapolateTotalDays`). 서버는 `ResyncIntervalSeconds`(10초)마다 · 시각을 바꿀 때 스냅숏을 새로 찍는다. 클라이언트는 새 스냅숏과 5초 이내로 어긋나면 차이를 `ClientCorrectionSeconds`(1초) 동안 지수로 메우고, 더 크면(시각 설정) 바로 맞춘다.
- 설정 `UGrimEnvironmentSettings`(`DefaultGame.ini` 의 `[/Script/Grimoire.GrimEnvironmentSettings]`): `DayLengthMinutes` 48(Q6 확정), `DayRatio` 0.625, `StartTimeOfDay` 0.3(07:12). 낮은 정오를 가운데로 둔다 → 해돋이 04:30, 해넘이 19:30.
- 이벤트(서버 · 클라이언트가 각자 자기 시각으로): `OnNightBegin` · `OnNightEnd` · `OnHourChanged(Hour)`. 시작 직후 · 시각 점프 직후에는 내지 않는다.
- 서버 전용 `SetTimeOfDay(0~1)` · `SetDayLengthMinutes`. 콘솔 `grim.Time.Set <시>` · `grim.Time.DayLength <분>`(서버 창에서).

## 하늘 `AGrimSkyController`

구현: `Source/Grimoire/Environment/GrimSkyController.h`.

- 레벨에 하나 놓는다(배치 스크립트 `Scripts/Tutorial/tower_greybox.py` · `Scripts/Terrain/build_openbeta.py` · `Scripts/Integration/place_m1_props.py` 가 `Sky` 로 놓는다). 자기 컴포넌트로 해 · 달 Directional Light(대기 태양 인덱스 0 · 1), Sky Atmosphere, Sky Light(실시간 캡처), Exponential Height Fog 를 가진다 — 레벨의 다른 해 · 하늘 · 안개는 뺀다. 복제하지 않고 각 머신이 서브시스템 시각으로 틱마다 갱신한다.
- 해 궤도 각 `GetSunAngleDegrees`: 해돋이 0° → 정오 90° → 해넘이 180° → 자정 270°(낮 · 밤이 각자 반 바퀴). 궤도는 `SunriseYawDegrees` 에서 떠서 `OrbitTiltDegrees` 만큼 기운 천정을 지난다. 달은 해의 맞은편.
- 밝기: 해 · 달은 지평선 근처 `HorizonFade` 에서 부드럽게 켜지고 꺼진다(`SunIntensity` 10 lux, `MoonIntensity` 0.4 lux). Sky Light 는 낮 1 · 밤 0.25.
- **그림자 캐시**: 해 · 달 빛은 방향이 `SunRotationStepDegrees`(기본 0.1°, 48분 하루면 약 0.8초) 넘게 바뀔 때만 돌린다 — 방향광이 돌 때마다 가상 섀도 맵 캐시가 통째로 무효가 된다(GPU 그림자 2 ~ 2.6 ms → 0.4 ~ 0.6 ms, [성능](./performance#측정-기록)). 콘솔 `Grim.Sky.SunRotationStep <도>`(0 = 매 프레임).
- 에디터에서는 `PreviewTimeOfDay` 로 미리 본다. 날씨 연출은 아래 「날씨 — 연출」. 볼류메트릭 구름 · `MPC_Environment`(젖음 머티리얼) · 바람(풀 · 나무)은 아직 없다.

## 기후 지역 `AGrimClimateRegionVolume` + 기후 표

구현: `Source/Grimoire/Environment/GrimClimateRegionVolume.h`, 행 구조체 · 등급 `Source/Grimoire/Environment/GrimClimateTypes.h`.

- **기후 표** `Content/Grimoire/Data/ClimateRegions.csv`(행 구조체 `FGrimClimateRow`, 행 이름 = 지역 id). 런타임에 CSV 를 DataTable 로 읽는다(반응 규칙표와 같은 방식, `Grimoire/Data` 는 패키징 때 함께 간다). 설정 `ClimateTableAsset` 을 주면 DataTable 에셋을 대신 쓴다. 표가 없거나 기본 지역 행이 없으면 환경 설정의 전역 곡선(M1)으로 만든 기본 지역 하나로 돈다.
- 한 행: 기온 곡선(`MinTemperature` · `MaxTemperature` · `ColdestTimeOfDay` · `WarmestTimeOfDay`, M1 과 같은 반 코사인), 시간대 등급(`DayHeat` · `DayCold` · `NightHeat` · `NightCold`), 날씨 가산(`RainCold` · `SnowCold` · `SandstormHeat` · `ClearNoonHeat`), 강수 `Precipitation`(Rain / Snow), 전용 날씨 허용(`bAllowSnow` · `bAllowSandstorm` — Q14), `InitialWeather`, `MinWeatherSeconds`, 전이표 `FromClear` … `FromSandstorm`(각 행 = 다음 날씨 가중치 `(Clear=6,Cloudy=3,…)`).
- 지금 행(스펙의 제안 지역): `Meadow`(탑 주변 초원, **기본 지역**) · `DeepForest` · `SnowPeak` · `VolcanicWaste` · `StormCoast` · `ShadowValley`. 등급은 스펙 「체온 모델」 예 그대로(초원 낮 0 · 밤 추위 1, 설산 낮 추위 2 · 밤 4, 화산 낮 더위 3 · 밤 1).
- **볼륨**: 상자(`UBoxComponent` `Bounds`, 충돌 없음 · 복제 없음 — 레벨에 놓인 그대로 모든 머신에 있다) + `RegionId` + `BlendDistance`(기본 20m). 가중치는 바깥 0, 경계에서 안쪽으로 `BlendDistance` 들어가면 1(`FGrimWeatherMath::GetBoxBlendWeight`, 회전 · 크기 반영). 겹친 볼륨 합이 1 을 넘으면 나눠 합 1, 남는 몫은 기본 지역(`NormalizeRegionWeights`). 같은 `RegionId` 볼륨 여럿은 날씨 하나를 함께 쓴다. 표에 없는 id 는 경고하고 무시한다.
- 볼륨은 `BeginPlay` 에 서브시스템에 등록한다(지역 수만큼이라 적다 — 상호작용 사물 목록과는 다르다).
- 지금 맵에 놓인 것: 탑 · 오픈 베타 맵의 `Climate_Meadow`(`Scripts/Tutorial/tower_greybox.py` · `Scripts/Terrain/build_openbeta.py`). 다른 지역 볼륨은 시험 배치 스크립트만 놓는다(아래).

## 날씨 `UGrimWeatherSubsystem`

구현: `Source/Grimoire/Environment/GrimWeatherSubsystem.h`, 복제 `Source/Grimoire/Environment/GrimWeatherReplicator.h`, 순수 계산 `Source/Grimoire/Environment/GrimWeatherMath.h`, 설정 `Source/Grimoire/Environment/GrimWeatherSettings.h`, 콘솔 `Source/Grimoire/Environment/GrimWeatherDebug.cpp`.

- `UTickableWorldSubsystem`(게임 · PIE). 지역(표의 행)마다 **상태 머신** 하나: `EGrimWeatherType` = `Clear · Cloudy · Fog · Rain · Thunderstorm · Snow · Sandstorm`. Snow · Sandstorm 은 그 행이 허용할 때만 고른다(Q14). `Precipitation = Snow` 인 지역은 고른 Rain · Thunderstorm 이 Snow 가 된다.
- **전이(서버)**: 한 날씨가 `MinWeatherSeconds`(지역마다, 기본 6분) 지난 뒤부터 `TransitionCheckIntervalSeconds`(60초)마다 지금 날씨의 전이표 행에서 가중치 비례로 다음 날씨를 고른다(자기 자신 가중치 = 머묾, `FGrimWeatherMath::PickNextWeather`). 바뀌면 `BlendSeconds`(150초 = 2분 30초) 동안 스무스스텝으로 섞는다. 블렌드 도중 또 바뀌면 지금 더 큰 쪽에서 새로 시작한다.
- **복제**: 서버가 `OnWorldBeginPlay` 에 보이지 않는 `AGrimWeatherReplicator`(`AInfo`, 항상 관련)를 만들고 `RegionWeather`(`TArray<FGrimRegionWeather { RegionId, Type, Previous, TransitionStartServerSeconds, TransitionSeconds }>`)를 **바뀔 때만** 보낸다(`ForceNetUpdate`). 블렌드 진행은 서버 · 클라이언트가 같은 서버 월드 시각(`GameState`)으로 각자 계산한다 — 시간 복제와 같은 방식, GameState 는 고치지 않는다. 낙뢰 연출은 같은 액터의 `MulticastLightningStrike`(Unreliable).
- **섞임 조회**: 위치 → 지역 가중치 × 지역의 시간 블렌드 = `FGrimWeatherMix`(날씨 종류별 비율, 합 1). 날씨 종류 공통 값(`FGrimWeatherProfile` — 기온 가산 · 비 · 눈 · 모래 양 · 시야 거리 · 소프트 락온 배율 · 연출 배율)은 설정에 있고 섞임 비율로 섞는다(`GetBlendedProfileAt`).
- 이벤트: `OnRegionWeatherChanged(RegionId, New, Old)` — 서버는 전이 시작 때, 클라이언트는 복제가 왔을 때. `OnLightningStrike(Location)` — 모든 머신(멀티캐스트).

### 공개 함수

| 함수 | 쓰는 쪽 | 내용 |
|---|---|---|
| `GetAmbientTemperature(Location)` | 생존 · 농사 · 얼음 | 지역 기온 곡선 + 날씨 `TemperatureOffset`(흐림 −1.5 · 비 −3 · 뇌우 −4 · 눈 −4 · 모래폭풍 +3 · 안개 −1 ℃), 지역 경계에서 섞음 |
| `GetClimateExposure(Location)` | 생존(체온) | 지역 · 시간대 등급 + 날씨 가산, 더위와 추위를 서로 깎은 정수 등급 |
| `GetClimateExposureBreakdown(Location)` | 생존 | `Region` · `Weather` 몫(각각 반올림, 깎지 않음), `Total`(= 위), `Sheltered`(지역 몫만 깎은 것 — 실내) |
| `IsSheltered(Location, IgnoreActor)` | 생존(실내면 날씨 몫 빼기) · 날씨 자신 | 위로 `ShelterTraceDistance`(30m) 라인 트레이스, 월드 스태틱 · 다이내믹에 막히면 true. `Location` 은 대상 위(머리 위)로 준다 |
| `IsActorSheltered(Actor)` | 비 · 낙뢰 | 액터 충돌 바운드 꼭대기 + 5cm 에서 `IsSheltered`(자신 · 붙은 액터 제외) |
| `GetWeatherAt` · `GetWeatherWeightAt` · `GetRainAmountAt` · `GetRegionAt` · `GetRegionWeather` | 누구나 | 가장 큰 날씨 · 날씨 비율 · 비 양 · 가장 큰 지역 · 지역 상태 |
| `GetVisibilityDistance(Location)` · `GetSoftLockRangeScale(Location)` | AI 감지 · UI 표식 · 조준 | 시야 거리(cm, 안개 30m) · 소프트 락온 사거리 배율(안개 0.6). **값만 공개** — 조준 쪽 연결은 통합 때 |
| `SetRegionWeather(RegionId, Type, BlendSeconds)` | 서버 · 콘솔 · 나중의 세이브 | 허용되지 않는 날씨는 가까운 것으로(Snow → Rain, Sandstorm → Cloudy) |

### 등급 (docs/spec/survival 「체온 모델」)

- 지역 · 시간대: 밤(`FGrimTimeOfDayMath::IsNight`)이면 `Night*`, 아니면 `Day*` 칸.
- 날씨: 비 · 뇌우 비율 × `RainCold`, 눈 × `SnowCold`, 모래폭풍 × `SandstormHeat`, 한낮(`NoonStartHour` 11 ~ `NoonEndHour` 16시)이면 맑음 × `ClearNoonHeat`(화산 황야만 1).
- 지역 경계 · 날씨 블렌드 동안 실수로 섞은 뒤 반올림한다(0.5 는 올림). `Total` 은 지역 + 날씨를 더한 뒤 더위와 추위를 서로 깎는다(더위 3 · 추위 1 → 더위 2) — 「보통 한쪽만 0 보다 크다」.

### 게임플레이 훅 (서버)

| 날씨 | 서버가 하는 일 |
|---|---|
| 비 · 뇌우 | `RainStimulusIntervalSeconds`(10초)마다, 어느 지역이든 비 양이 `RainStimulusMinAmount`(0.5) 이상이면 **플레이어마다 반경 `RainStimulusRadius`(50m) 구 오버랩**(월드 다이내믹 · 폰 · 물리 몸체) → 속성 수신자가 있고 그 자리의 비 양이 0.5 이상이며 `IsActorSheltered` 가 아니면 `Water(0.5, Weather)` 자극. 한 번에 `MaxRainTargetsPerPass`(64)개까지(지붕 트레이스 상한). 반응 표대로 켜진 양초 · 모닥불은 꺼지며 젖고(Water_Douse), 다른 것은 젖는다(Water_Soak, 20초 — 10초마다 다시 적신다). 상호작용 사물 목록 · 전파 서브시스템은 쓰지 않는다([마법 「성능 · 네트워크 규칙」](./magic#성능-·-네트워크-규칙)) |
| 뇌우 | 뇌우 비율이 `LightningMinAmount`(0.5) 이상인 지역마다 `LightningMin/MaxIntervalSeconds`(8~25초) 무작위 간격. 그 지역(가중치 0.5 이상)에 선 플레이어 하나를 골라 반경 30m 의 야외 속성 대상에 점수(기본 1 + 플레이어보다 높은 m × 0.5 + 젖음 세기 × 2 + `Material.Conductive` 3)로 가중 추첨 → `Light(3.0, Weather)` 자극. `LightningGroundChance`(0.4) 또는 후보가 없으면 빈 땅(연출만). 범위 피해 GE 는 아직 없다 |
| 안개 | `GetVisibilityDistance` · `GetSoftLockRangeScale` 값만. 바람 마법의 안개 걷기(`AGrimFogClearing`)는 나중 |
| 눈 · 모래폭풍 | 등급 · 기온 · 연출만 |
| 마법 보정 | `GE_Weather_Rain`(불 −20% · 물 +20%) — 아직 없음(제안) |

### 연출 (`AGrimSkyController`, 클라이언트)

- 틱마다 로컬 카메라 위치의 `GetBlendedProfileAt` → 해 · 달 세기 × `SunIntensityScale`, 하늘빛 × `SkyLightScale`, 높이 안개 밀도 = 처음 밀도 + `FogDensityAdd`. 전용 서버는 건너뛴다.
- 강수: `RainFX` · `SnowFX` · `DustFX`(Niagara 컴포넌트)를 카메라 위 `PrecipitationHeight`(6m)에 뒤집어 두고 양이 `PrecipitationVisibleThreshold`(0.15) 이상일 때만 켠다. **임시**: 에셋은 엔진 기본 템플릿 `FountainLightweight`(셋 다), 번개 터짐은 `RadialBurst` — 정식 이펙트로 바꿀 때 설정의 경로만 고친다(지금 에셋은 쿠킹 목록에 없다 — 패키징 전에 정식 에셋으로).
- 번개 섬광: `OnLightningStrike` → 하늘빛 + `LightningFlashSkyLight`(6)를 `LightningFlashSeconds`(0.25초) 동안 줄여 가며, 그 지점에 터짐 이펙트. 천둥 소리 · 번개 줄기는 아직 없다.

### 설정 · 콘솔

- `UGrimWeatherSettings` = `DefaultGame.ini` `[/Script/Grimoire.GrimWeatherSettings]`: 표 경로 · 기본 지역 · 한낮 시간 · 전이 간격 · 블렌드 · 난수 씨앗(0 = 시계) · 비 자극 · 지붕 · 낙뢰 · 연출 값, 날씨 종류별 `ClearProfile` … `SandstormProfile`.
- 콘솔(개발 빌드, 서버 월드에서): `Grim.Weather.Set <지역|All> <날씨> [블렌드초] [지연초]`, `Grim.Weather.Dump [지연초]`(지역 날씨 + 로컬 플레이어 위치의 지역 · 날씨 · 기온 · 등급 · 비 · 시야 · 지붕), `Grim.Weather.Probe <X> <Y> <Z> [지연초]`(그 위치의 지역 · 날씨 · 기온 · 몫별 등급 · 지붕), `Grim.Weather.Strike <지역> [지연초]`(바로 낙뢰 하나).
- 2인 확인: `Scripts/Environment/net_check_weather.ps1` — 헤드리스 리슨 서버 + 클라이언트, 초원을 비로 바꿔 야외 양초가 꺼지고 젖는지, 양쪽 덤프가 같은지 본다(`Saved/NetCheckWeather/`).
- 시험 배치: `Scripts/Environment/place_climate_test.py` — 맵(기본 `L_M1_Sandbox`, 환경 변수 `GRIM_CLIMATE_TEST_MAP` 로 바꾼다)에 설산 · 화산 황야 · 폭풍 해안 볼륨과 지붕 + 양초 둘(지붕 아래 · 밖)을 놓는다(태그 `ClimateTest`, 다시 돌리면 지우고 다시 놓는다).
- 환경 에셋 가져오기: `Scripts/Environment/import_env_asset.py` — 블렌더 GLB 를 `/Game/Grimoire/Environment/<종류>/<이름>/` 로(Nanite 켬). 환경 변수 `GRIM_ENV_GLB` · `GRIM_ENV_DEST`.

## 환경이 다른 시스템에 주는 것

| 소비자 | 받는 것 |
|---|---|
| 생존 | 기온, 더위 · 추위 등급(몫별), 지붕(`IsSheltered`), 젖음(비 자극이 `State.Wet` 을 준다), 밤 여부. 풍속은 아직 없다 |
| 마법 상호작용 | 비(물) · 낙뢰(빛) 자극 — `UGrimElementalStatics::ApplyStimulusToActor`, 출처 `Weather` |
| 조준 · AI · UI | 시야 거리, 소프트 락온 배율(값만) |
| 생물 | 시간대 · 날씨별 스폰 · 행동 전환 |
| 농사 | 비(급수), 낮(빛), 기온(생장 · 동사) |
| 던전 | 인스턴스는 자체 환경(실내 = 고정) |

## 세이브

두 서브시스템이 `IGrimSaveParticipant`(`Source/Grimoire/Core/GrimSaveTypes.h`), World 범위:

- `Environment.TimeOfDay` v1 — 누적 일수 `TotalDays`. 하루 길이는 서버 옵션이라 저장하지 않는다.
- `Environment.Weather` v1 — 지역마다 id · 날씨 · 그 날씨가 된 지 몇 초(최소 지속을 잇는다). 읽으면 블렌드 없이 그 날씨로, 표에 없는 지역은 건너뛴다.

## 범위

- M1: 밤 · 낮 사이클 + 전역 기온 하나. 환경 설정의 `MinAmbientTemperature` 6℃ · `MaxAmbientTemperature` 22℃ · `ColdestTimeOfDay` 0.2 · `WarmestTimeOfDay` 0.6 은 이제 기후 표가 없을 때의 기본 지역 곡선으로만 쓴다.
- M2(갈래 W): 기후 지역 볼륨 · 기후 표 · 지역별 날씨 상태 머신 · 복제 · 등급 · 기온 · 지붕 판정 · 비 자극 · 낙뢰(자극만) · 안개 값 · 하늘 연출(임시 이펙트).
- 세이브(시간 · 지역 날씨)는 들어갔다.
- 아직 없음: 낙뢰 피해 GE · 비의 마법 보정 GE · 안개 걷기 · 바람(풍속 · 풍향) · 젖음 머티리얼(`MPC_Environment`) · 천둥 소리 · 정식 이펙트 · 볼류메트릭 구름 · 소프트 락온 배율의 조준 쪽 연결 · 앞으로만 가는 `SkipTo`(잠 건너뛰기용).

### 테스트

`Source/Grimoire/Environment/Tests/`:

- 시간 · 기온(M1): `Grimoire.Environment.TimeOfDay.Extrapolation` · `.TimeOfDay.Night` · `.Temperature.TimeCurve`
- 날씨 · 기후(M2): `Grimoire.Environment.Weather.Transition`(마르코프 · 전용 날씨 허용 · 강수 변환 · 빈도) · `.Weather.Blend`(시간 블렌드) · `.Weather.Lightning`(가중 추첨 · 점수) · `.Climate.RegionBlend`(상자 가중치 · 정규화) · `.Climate.Exposure`(등급 · 반올림 · 서로 깎기 · 한낮) · `.Climate.Table`(CSV 를 읽어 스펙 예 · 허용되지 않는 전이 없음 · 기온 곡선)
