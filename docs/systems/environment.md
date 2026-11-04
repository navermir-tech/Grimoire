# 환경 · 날씨 시스템

> 스펙: [세계 · 환경](../spec/world) (R04)

## 시간 `UGrimTimeOfDaySubsystem`

- `UWorldSubsystem`. 서버가 `WorldSeconds` 를 진행하고 `AGrimGameState::ReplicatedTimeOfDay`(0~1, 하루 비율)와 `DayIndex` 를 복제(1초마다 또는 변화율로). 클라이언트는 받은 값에서 **외삽**해 매 프레임 부드럽게.
- 설정: `DayLengthMinutes`(기본 48 — Q6), `DayNightRatio`(0.625). 서버 옵션.
- 이벤트: `OnHourChanged`, `OnNightBegin / OnNightEnd` → 생태계 스폰, 생존(피로 배율), 조명.
- `AGrimSkyController`(레벨에 하나): Directional Light 두 개(해 · 달) 회전, Sky Atmosphere, Volumetric Cloud, Sky Light 재캡처(저빈도), `MPC_Environment`(시간 · 밤 비율) 갱신.

## 기후 `AGrimClimateVolume` + `DT_Climate_*`

- 지역마다 볼륨(또는 World Partition 데이터 레이어 + 영역 정보). 내용: 기본 기온 곡선(시간 → ℃), 날씨 확률 전이표, 강수 종류(비 / 눈), 서식 생물 태그, 바이옴 태그 `Biome.Snow` 등.
- `UGrimWeatherSubsystem::GetClimateAt(Location)` 은 겹치는 볼륨의 가중 평균(경계 블렌딩).

## 날씨 `UGrimWeatherSubsystem`

- 지역(볼륨)마다 **상태 머신**: `Clear · Cloudy · Fog · Rain · Thunderstorm`(+ `Snow · Sandstorm` 지역 전용 — Q14). 전이는 기후 표의 확률(마르코프), 최소 지속 시간, 블렌드 시간(2~3분).
- 서버가 지역별 `FGrimWeatherState { Type, Intensity, WindDir, WindSpeed, Temperature, Wetness }` 를 계산 → `AGrimGameState::RegionWeather`(TArray, 변경 시 복제).
- 클라이언트: 플레이어 위치의 블렌딩된 상태로 연출 — Niagara 비 · 안개(Exponential Height Fog 밀도) · 구름 두께 · 젖음(`MPC_Environment.Wetness` → 머티리얼) · 바람(풀 · 나무 WPO, 깃발) · 번개(스카이 플래시 + 라인 Niagara + 사운드 지연).

### 게임플레이 훅 (서버)

| 날씨 | 서버가 하는 일 |
|---|---|
| 비 | 반경 내 야외 `GrimElementalState` 사물에 10초마다 `Water(0.5)` 자극 → 불 꺼짐 · 작물 급수(지붕 판정은 컴포넌트 쪽 트레이스) |
| 번개 | 번개 타이머 → 높은 곳 · `Material.Conductive` · `State.Wet` 대상 가중 선택 → 그 지점에 `Wind + Light(3.0)` 자극 + 범위 피해 GE |
| 안개 | 시야 거리 값 → AI 감지 반경 · 플레이어 UI 표식 거리 감소. 바람 마법은 로컬 안개 억제 볼륨(`AGrimFogClearing`, 30초) |
| 기온 | `GetAmbientTemperature` → 생존 체온 |
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

밤 · 낮 사이클 + 맑음 / 비 두 날씨 + 기온 하나(전역). 지역 볼륨 · 번개 · 안개는 M2.
