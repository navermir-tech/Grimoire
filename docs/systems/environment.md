# 환경 · 날씨 시스템

> 스펙: [세계 · 환경](../spec/world) (R04)

## 시간 `UGrimTimeOfDaySubsystem`

- `UTickableWorldSubsystem`(게임 · PIE 월드). 시각은 누적 일수 `TotalDays` — 정수부 = 날짜(`GetDayIndex`), 소수부 = 하루 비율(`GetTimeOfDay`, 0 = 자정, 0.5 = 정오).
- **복제**: 서버가 `OnWorldBeginPlay` 에서 보이지 않는 `AGrimTimeOfDayReplicator`(`AInfo`, 항상 관련)를 만들고 스냅숏 `FGrimTimeSnapshot { TotalDays, ServerSeconds, DayLengthSeconds, DayRatio }` 를 복제한다. 기반 GameState 는 고치지 않는다.
- **진행 · 외삽**: 모두가 같은 식 `TotalDays = 스냅숏.TotalDays + (GameState 서버 월드 시각 − 스냅숏.ServerSeconds) / 하루 길이` 로 계산한다(`FGrimTimeOfDayMath::ExtrapolateTotalDays`). 서버는 `ResyncIntervalSeconds`(10초)마다 · 시각을 바꿀 때 스냅숏을 새로 찍는다. 클라이언트는 새 스냅숏과 5초 이내로 어긋나면 차이를 `ClientCorrectionSeconds`(1초) 동안 지수로 메우고, 더 크면(시각 설정) 바로 맞춘다.
- 설정 `UGrimEnvironmentSettings`(`DefaultGame.ini` 의 `[/Script/Grimoire.GrimEnvironmentSettings]`): `DayLengthMinutes` 48(Q6), `DayRatio` 0.625, `StartTimeOfDay` 0.3(07:12). 낮은 정오를 가운데로 둔다 → 해돋이 04:30, 해넘이 19:30.
- 이벤트(서버 · 클라이언트가 각자 자기 시각으로): `OnNightBegin` · `OnNightEnd` · `OnHourChanged(Hour)`. 시작 직후 · 시각 점프 직후에는 내지 않는다.
- 서버 전용 `SetTimeOfDay(0~1)` · `SetDayLengthMinutes`. 콘솔 `grim.Time.Set <시>` · `grim.Time.DayLength <분>`(서버 창에서).

## 하늘 `AGrimSkyController`

- 레벨에 하나 놓는다. 자기 컴포넌트로 해 · 달 Directional Light(대기 태양 인덱스 0 · 1), Sky Atmosphere, Sky Light(실시간 캡처), Exponential Height Fog 를 가진다 — 레벨의 다른 해 · 하늘 · 안개는 뺀다. 복제하지 않고 각 머신이 서브시스템 시각으로 틱마다 갱신한다.
- 해 궤도 각 `GetSunAngleDegrees`: 해돋이 0° → 정오 90° → 해넘이 180° → 자정 270°(낮 · 밤이 각자 반 바퀴). 궤도는 `SunriseYawDegrees` 에서 떠서 `OrbitTiltDegrees` 만큼 기운 천정을 지난다. 달은 해의 맞은편.
- 밝기: 해 · 달은 지평선 근처 `HorizonFade` 에서 부드럽게 켜지고 꺼진다(`SunIntensity` 10 lux, `MoonIntensity` 0.4 lux). Sky Light 는 낮 1 · 밤 0.25.
- 에디터에서는 `PreviewTimeOfDay` 로 미리 본다. 볼류메트릭 구름 · `MPC_Environment` 는 M2.

## 기후 `AGrimClimateVolume` + `DT_Climate_*`

- 지역마다 볼륨(또는 World Partition 데이터 레이어 + 영역 정보). 내용: 기본 기온 곡선(시간 → ℃), 날씨 확률 전이표, 강수 종류(비 / 눈), 서식 생물 태그, 바이옴 태그 `Biome.Snow` 등.
- `UGrimWeatherSubsystem::GetClimateAt(Location)` 은 겹치는 볼륨의 가중 평균(경계 블렌딩).

## 날씨 `UGrimWeatherSubsystem`

- 지역(볼륨)마다 **상태 머신**: `Clear · Cloudy · Fog · Rain · Thunderstorm`(+ `Snow · Sandstorm` 지역 전용 — Q14). 전이는 기후 표의 확률(마르코프), 최소 지속 시간, 블렌드 시간(2~3분).
- 서버가 지역별 `FGrimWeatherState { Type, Intensity, WindDir, WindSpeed, Temperature, Wetness }` 를 계산 → 시간처럼 전용 복제 액터의 `RegionWeather`(TArray, 변경 시 복제). GameState 는 고치지 않는다.
- 클라이언트: 플레이어 위치의 블렌딩된 상태로 연출 — Niagara 비 · 안개(Exponential Height Fog 밀도) · 구름 두께 · 젖음(`MPC_Environment.Wetness` → 머티리얼) · 바람(풀 · 나무 WPO, 깃발) · 번개(스카이 플래시 + 라인 Niagara + 사운드 지연).

### 게임플레이 훅 (서버)

| 날씨 | 서버가 하는 일 |
|---|---|
| 비 | 반경 내 야외 `GrimElementalState` 사물에 10초마다 `Water(0.5)` 자극 → 불 꺼짐 · 작물 급수(지붕 판정은 컴포넌트 쪽 트레이스) |
| 번개 | 번개 타이머 → 높은 곳 · `Material.Conductive` · `State.Wet` 대상 가중 선택 → 그 지점에 `Wind + Light(3.0)` 자극 + 범위 피해 GE |
| 안개 | 시야 거리 값 → AI 감지 반경 · 플레이어 UI 표식 거리 감소. 바람 마법은 로컬 안개 억제 볼륨(`AGrimFogClearing`, 30초) |
| 기온 | `UGrimWeatherSubsystem::GetAmbientTemperature(Location)` → 생존 체온 |
| 마법 보정 | `GE_Weather_Rain`(불 −20% · 물 +20%)을 비 지역의 ASC 에 적용 · 해제(제안) |

## 환경이 다른 시스템에 주는 것

| 소비자 | 받는 것 |
|---|---|
| 생존 | 기온, 젖음, 풍속, 밤 여부 |
| 마법 상호작용 | 비 · 번개 자극 |
| 생물 | 시간대 · 날씨별 스폰 · 행동 전환 |
| 농사 | 비(급수), 낮(빛), 기온(생장 · 동사) |
| 던전 | 인스턴스는 자체 환경(실내 = 고정) |

## 세이브

- 시간(`DayIndex`, `TimeOfDay`), 지역별 날씨 상태 · 남은 지속 시간 → 세이브 서브시스템에 등록.

## M1 범위

밤 · 낮 사이클 + 기온 하나(전역). 지역 볼륨 · 날씨 상태 머신(비 포함) · 번개 · 안개는 M2.

### M1 기온 (구현됨)

`UGrimWeatherSubsystem::GetAmbientTemperature(Location)` — M1 은 위치와 무관한 시간대 곡선 하나: `ColdestTimeOfDay`(0.2 = 04:48)에 `MinAmbientTemperature`(6℃), `WarmestTimeOfDay`(0.6 = 14:24)에 `MaxAmbientTemperature`(22℃), 사이는 반 코사인(`FGrimTimeOfDayMath::GetAmbientTemperature`). 서버 · 클라이언트 어디서나 부를 수 있다.

### 테스트

`Grimoire.Environment.TimeOfDay.Extrapolation` · `.TimeOfDay.Night` · `.Temperature.TimeCurve` (`Source/Grimoire/Environment/Tests/`).
