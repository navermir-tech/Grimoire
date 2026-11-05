# 생존 시스템

> 스펙: [생존](../spec/survival) (R03), [세계 · 환경](../spec/world) (R04)

## 어트리뷰트

`UGrimVitalAttributeSet` — ASC 에 부착, 플레이어 · 생물 공용. 마나는 `UGrimMagicAttributeSet`(Magic 시스템).

| 어트리뷰트 | 범위 | 비고 |
|---|---|---|
| `Health` / `MaxHealth` | 0~Max, Max ≥ 1 | 0 → 쓰러짐(`Status.Downed`) → 사망(M2) |
| `Hunger` | 0~`MaxHunger`(100) | 100 = 배부름. 분당 감소 |
| `BodyTemperature` | 섭씨, `MinBodyTemperature`~`MaxBodyTemperature`(25~45) | 안전 35~39. 목표 체온 쪽으로 수렴 |
| `Fatigue` (M2) | 0~100 | 0 = 쌩쌩. 깨어 있으면 증가, 밤 ×1.5 |
| `DiseaseResist` (M2) | 0~100 | 질병 GE 적용 확률에 영향 |

- 클램프는 `PreAttributeBaseChange` · `PreAttributeChange` 에서, 범위는 설정에서 읽는다.
- 초기값은 설정(`InitialMaxHealth` · `InitialHunger` · `InitialBodyTemperature`)을 서버가 PlayerState 마다 **한 번** 넣는다(폰이 바뀌어도 유지).

생물은 `Hunger`, `Fatigue` 만 쓴다(일꾼 관리용). 생물의 체온은 종의 적응 기후로 단순화(▢).

## 설정 `UGrimSurvivalSettings`

`DefaultGame.ini` 의 `[/Script/Grimoire.GrimSurvivalSettings]` 에 `Tuning=(...)` 한 줄(`FGrimSurvivalTuning`). 순수 계산 `FGrimSurvivalModel` 과 테스트가 이 구조체만 받는다. 기본값:

| 항목 | 값 | 뜻 |
|---|---|---|
| `TickIntervalSeconds` | 1 | 생존 틱 간격 |
| `HungerDrainPerMinute` · `HungryThreshold` | 1 · 20 | 가득 찬 배가 100분(약 이틀) 간다. 20 미만 = 배고픔 |
| `HealthRegenPerSecond` | 0.5 | 배고프지 않고 피해가 없을 때 |
| `StarvingDamagePerSecond` · `ExposureDamagePerSecond` | 0.5 · 0.2 | 굶주림 · 저체온/과열 때 HP 감소 |
| `NormalBodyTemperature` · `ComfortTemperature` · `BodyTemperatureSensitivity` | 37 · 20 · 0.25 | 목표 체온 식(아래) |
| `BodyTemperatureTimeConstantSeconds` | 120 | 2분이면 목표와의 차이가 37% 로 |
| `HypothermiaThreshold` · `HyperthermiaThreshold` | 35 · 39 | 안전 범위 |
| `HeatSourceRadius` · `WarmthPerBurningIntensity` | 600cm · 15℃ | 열원 세기 1 이 바로 옆에서 체감 +15℃, 반경 끝에서 0(선형) |
| `CoolingPerWetIntensity` | 4℃ | 젖음 세기 1 당 체감 −4℃ |

## 주기 효과 (서버 전용) — `UGrimSurvivalComponent`

- PlayerState 의 `OnAbilityActorInfoReady` 에서 서버가 초기값을 넣고 `TickIntervalSeconds` 타이머를 건다.
- 틱마다 입력(HP · 배고픔 · 체온 · 주변 기온 · 열원 · 젖음)을 모아 `FGrimSurvivalModel::ComputeTick` 으로 변화량을 구하고, **즉시 GE `UGrimSurvivalTickEffect`**(C++ CDO, 에셋 없음) 하나에 SetByCaller(`Survival.Health` · `Survival.Hunger` · `Survival.BodyTemperature`, 베이스 가산)로 적용한다. 열원이 시시각각 바뀌어 주기 GE 의 고정 크기 대신 매 틱 계산한 값을 쓴다.
- HP: 굶주림(배고픔 0) · 저체온/과열이면 감소, 아니고 배고프지 않으면 자연 회복(최대치까지).
- 임계값 → 태그(`Survival/GrimSurvivalTags`, 복제 루스 태그 `TagOnly` — 모든 클라이언트가 본다): `Hunger < 20 → Status.Hungry`, `Hunger == 0 → Status.Starving`, `BodyTemperature < 35 → Status.Hypothermia`, `> 39 → Status.Hyperthermia`.
- 마나 회복은 Magic 시스템 몫 — `Status.Hungry` 태그를 보고 멈춘다.
- (M2) `Fatigue > 80 → Status.Exhausted`(CastSpeed −30%), `Fatigue == 100 → Status.Collapsed`(강제 수면 10초). 상태별 GE(`GE_Status_*`) 연결.

## 체온 모델

**M2 부터: 등급 방식(스펙 「더위 · 추위 등급과 저항」).** M1 의 섭씨 식(아래)은 등급을 계산하는 재료로 남긴다.

```
노출 등급  Exposure = UGrimWeatherSubsystem::GetClimateExposure(Location)   // { Heat, Cold } 정수
           = 지역 표(시간대 곡선) + 날씨 가산 + 상태(젖음 · 불붙음) + 주변(열원 −, 그늘 −, 실내면 날씨 몫 0)
저항 합    Resist   = Σ 장착 장비 · 인챈트 · 장신구 · 버프 GE 의 HeatResist / ColdResist   // MagicSet 옆 SurvivalSet 어트리뷰트
모자람     Deficit  = max(0, Exposure.Cold − Resist.Cold) 또는 max(0, Exposure.Heat − Resist.Heat)
목표 체온  = 37 − Deficit_Cold × 1.5 + Deficit_Heat × 1.5        // 범위 25~45 로 자름
BodyTemp  += (목표 − BodyTemp) × (1 − exp(−dt / 120초))           // 지금과 같은 지수 수렴
```

- `HeatResist` · `ColdResist` 는 어트리뷰트(정수처럼 쓰는 float). 장비 장착 시 Infinite GE 로 더하고 해제 시 뺀다 — 인챈트 · 마법 버프와 같은 길.
- 상태 태그는 체온이 아니라 **모자람**으로 건다: `Deficit 1 → Status.Chilly / Status.Warm`, `2 → Status.Hypothermia / Status.Hyperthermia`, `3+ → .Severe`(HP 감소). 게이지는 보이는 연출, 판정은 등급이라 장비를 갈아입으면 바로 결과가 맞는다.
- 지역 표 · 날씨 가산 · 상태 가산 · 열원 감산은 데이터(`DT_ClimateExposure` + `UGrimSurvivalSettings`). 섭씨 기온은 작물 생장 · 얼음 · 녹기 같은 상호작용에 계속 쓴다.

### M1 섭씨 식 (구현됨)

```
체감 기온  = AmbientTemp(UGrimWeatherSubsystem::GetAmbientTemperature)
           + Σ 열원(반경 내 액터의 State.Burning 세기 × 15℃ × (1 − 거리/반경))
           − 젖음(자기 폰의 State.Wet 세기 × 4℃)
           (M2: + 의복 단열, − 바람 × 0.3℃, 실내면 바람 · 비 0)
목표 체온  = 37 + (체감 기온 − 20) × 0.25                    // 범위 25~45 로 자름
BodyTemp  += (목표 − BodyTemp) × (1 − exp(−dt / 120초))      // 틱 간격과 무관한 지수 수렴
```

- 주변 기온으로 **바로** 수렴하지 않고 체감 기온을 목표 체온으로 바꾼다 — 그래야 22℃ 한낮에 정상 체온(37.5)이다. 결과: 추운 밤(6℃)엔 목표 33.5 → 몇 분 뒤 저체온, 모닥불(세기 1) 1.5m 옆이면 36.3 으로 안전, 젖으면 −1℃.
- 열원은 `OverlapMultiByObjectType`(모든 오브젝트 타입) 반경 질의 → 액터별 `UGrimElementalStatics::GetStateIntensity(State.Burning)`. **열원 액터는 질의 콜리전이 켜진 컴포넌트가 있어야 잡힌다.**
- 범위 이탈 → `Status.Hypothermia` / `Status.Hyperthermia` → HP 감소(M1). 이동 · 시전 패널티 GE 는 M2.
- 불 마법으로 몸 데우기: 불씨를 자기에게 쓰면 `State.Warmed`(열원 취급, 2분) — R16 의 "마법으로 해결"(M2).

## 디버그 표시

PlayerState 의 소유 로컬 플레이어 화면 왼쪽 위에 시각 · HP · 마나 · 배고픔 · 체온 · 주변 기온 · 상태 태그를 그린다(`UDebugDrawService` "Game" 채널, HUD 클래스 불필요). 콘솔 `grim.Survival.ShowDebug 0` 으로 끈다. 엔진 디버그 글꼴에 한글이 없어 라벨은 영어. 정식 HUD 는 UI 시스템 몫.

## 질병

- `GE_Disease_*`(지속형, 태그 `Status.Sick.Cough` …). 원인별 적용 확률: 날것 섭취, `State.Wet` + `Hypothermia` 지속, 특정 생물 공격, 오염 지역 볼륨.
- 치료: 약초 아이템(GE 제거), 빛 마법 `GA_Spell_Purify`(태그 `Status.Sick.*` 제거), 시간 경과.

## 먹기 · 요리 · 잠

- 음식 아이템 정의에 `HungerRestore`, `bRaw`, `DiseaseChance`, `Buffs`.
- 요리는 사물 상태로 한다: 날고기를 땅이나 모닥불 위에 두고 불 자극 → `State.Cooked` → 요리 아이템으로 바뀐다(R16).
- 잠: 침대 · 침낭(`AGrimBed`, 직접 상호작용 예외)에 누우면 `State.Sleeping`. 서버 `UGrimSleepSubsystem` 이 잠든 접속자 비율을 세어 `SleepSkipRatio`(기본 1.0, 서버 옵션) 이상이고 밤이면 `UGrimTimeOfDaySubsystem::SkipTo(SunriseTimeOfDay)` — 3초 페이드 후 시간 이동, 잠든 사람 `Fatigue = 0`, 건너뛴 게임 시간 × `SleepHungerRate` 만큼 배고픔 감소. 반경 `SleepBlockRadius` 안에 적대 AI 가 있으면 눕기 거부. 낮잠은 시간 이동 없이 `Fatigue` 만 회복. 마지막 침대 = 부활 지점(플레이어 세이브).

## 죽음 · 부활

- `Health == 0` → `Status.Downed`(60초, 동료가 빛 마법으로 회복 가능) → 사망.
- 사망: 가방 일부 드롭(`AGrimLootBag`), `AGrimGameMode::RespawnPlayer`(거점 핵 또는 마법탑). PlayerState 유지(서클 · 마법).

## M1 범위

HP · 배고픔 · 체온(마나는 Magic). 질병 · 피로 · 쓰러짐 · 상태 GE 는 M2.

### 테스트

`Grimoire.Survival.BodyTemperature.Converge` · `.BodyTemperature.Exposure` · `.Hunger` · `.AttributeClamp` (`Source/Grimoire/Survival/Tests/`).
