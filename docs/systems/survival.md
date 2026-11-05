# 생존 시스템

> 스펙: [생존](../spec/survival) (R03), [세계 · 환경](../spec/world) (R04)

## 어트리뷰트

`UGrimVitalAttributeSet` — ASC 에 부착, 플레이어 · 생물 공용. 마나는 `UGrimMagicAttributeSet`(Magic 시스템). PlayerState 를 고치지 않으려고 M2 어트리뷰트도 이 셋에 더했다.

| 어트리뷰트 | 범위 | 비고 |
|---|---|---|
| `Health` / `MaxHealth` | 0~Max, Max ≥ 1 | 0 → 쓰러짐(`Status.Downed`) → 사망 · 부활 |
| `Hunger` | 0~`MaxHunger`(100) | 100 = 배부름. 분당 감소 |
| `BodyTemperature` | 섭씨, `MinBodyTemperature`~`MaxBodyTemperature`(25~45) | **보이는 게이지**. 판정은 모자람 등급(아래) |
| `Fatigue` | 0~`MaxFatigue`(100) | 0 = 쌩쌩. 깨어 있으면 증가, 밤 ×1.5 |
| `HeatResist` · `ColdResist` | 제한 없음(음수 가능) | 장비 · 인챈트 · 버프 GE 가 더한다. 정수처럼 쓴다 |
| `DiseaseResist` | 0~100 | 질병 확률 × (1 − 값/100) |
| `MoveSpeedScale` · `CastSpeedScale` · `ManaCostScale` | ≥ 0, 기본 1 | 패널티 GE 가 곱한다. 이동은 생존 컴포넌트가 폰에 반영, 시전 · 마나 소모는 **마법 쪽이 읽는다** |

- 클램프는 `PreAttributeBaseChange` · `PreAttributeChange` 에서, 범위는 설정에서 읽는다.
- 초기값(`InitialMaxHealth` · `InitialHunger` · `InitialBodyTemperature` · 피곤함 0 · `InitialDiseaseResist`)은 서버가 PlayerState 마다 **한 번** 넣는다(폰이 바뀌어도 유지).

생물은 `Hunger`, `Fatigue` 만 쓴다(일꾼 관리용). 생물의 체온은 종의 적응 기후로 단순화(▢).

## 설정 `UGrimSurvivalSettings`

`DefaultGame.ini` 의 `[/Script/Grimoire.GrimSurvivalSettings]` 에 묶음마다 한 줄 — `Tuning` · `Climate` · `Fatigue` · `Disease` · `Sleep` · `Downed`. 순수 계산 `FGrimSurvivalModel` 과 테스트는 이것을 모은 `FGrimSurvivalRules` 를 받는다(`UGrimSurvivalSettings::GetRules()`). 기본값:

| 묶음 · 항목 | 값 | 뜻 |
|---|---|---|
| `Tuning.TickIntervalSeconds` | 1 | 생존 틱 간격 |
| `Tuning.HungerDrainPerMinute` · `HungryThreshold` | 1 · 20 | 가득 찬 배가 100분(약 이틀) 간다. 20 미만 = 배고픔 |
| `Tuning.HealthRegenPerSecond` | 0.5 | 배고프지 않고 피해가 없을 때 |
| `Tuning.StarvingDamagePerSecond` · `ExposureDamagePerSecond` | 0.5 · 0.2 | 굶주림 · 체온 3단계(`.Severe`) 때 HP 감소 |
| `Tuning.NormalBodyTemperature` · `BodyTemperatureTimeConstantSeconds` | 37 · 120 | 게이지 목표 · 수렴(2분이면 차이가 37% 로) |
| `Tuning.HypothermiaThreshold` · `HyperthermiaThreshold` | 35 · 39 | 게이지 안전 범위 표시(HUD 색)만 — 판정은 등급 |
| `Tuning.HeatSourceRadius` | 600cm | 이 안에 `State.Burning` 액터가 있으면 「열원 근처」 |
| `Climate.WetCold` · `BurningHeat` · `NearHeatSourceCold` · `SwimmingHeat` | 1 · 2 · 2 · 1 | 개인 몫: 젖음 추위 +1, 불붙음 더위 +2, 열원 근처 추위 −2, 물속 더위 −1 |
| `Climate.BodyTemperaturePerDeficit` | 1.5 | 목표 체온 = 37 ∓ 1.5 × 모자람 |
| `Climate.ModerateDeficit` · `SevereDeficit` | 2 · 3 | 단계 경계 |
| `Climate.Chilly…Scale` · `Hypothermia…Scale` | 0.9 · 0.75 | 추위 1 · 2단계 이동 · 시전 속도 배율 |
| `Climate.WarmManaCostScale` · `HyperthermiaManaCostScale` | 1.15 · 1.35 | 더위 1 · 2단계 마나 소모 배율 |
| `Climate.ModerateFatigueRateScale` | 1.5 | 2단계 이상이면 피곤함이 빨리 쌓인다 |
| `Fatigue.GainPerMinute` · `NightGainScale` | 1.5 · 1.5 | 하루(낮 30분 + 밤 18분)를 꼬박 새면 약 85 |
| `Fatigue.ExhaustedThreshold` · `ExhaustedCastSpeedScale` | 80 · 0.7 | `Status.Exhausted` 시전 −30% |
| `Fatigue.CollapseThreshold` · `CollapseSleepSeconds` | 100 · 10 | 기절(강제 수면) |
| `Fatigue.SleepRecoveryPerSecond` | 5 | 자는 동안(낮잠 · 기절) 회복 |
| `Disease.RawFoodStomachChance` · `StomachDurationSeconds` · `StomachHungerDrainScale` | 0.3 · 180 · 2 | 배탈 |
| `Disease.WetColdSecondsPerRoll` · `WetColdChance` · `ColdDurationSeconds` · `ColdManaRegenPenaltyPerSecond` | 60 · 0.35 · 300 · 1 | 감기 |
| `Sleep.SleepSkipRatio` | 1.0 | 서버 옵션 — 잠든 비율(1 = 전원) |
| `Sleep.SkipFadeSeconds` · `SkippedHungerScale` · `WakeAfterDawn` | 3 · 0.5 · 0.005 | 건너뛰기 페이드 · 배고픔 · 깨는 시각(해돋이 + 하루 비율) |
| `Sleep.HostileBlockRadius` · `BedInteractRadius` · `BedInteractKey` | 1500 · 250 · F | 적 검사 · 침대 거리 · 눕기 키 |
| `Sleep.SleepingMoveSpeedScale` | 0 | 자는 동안 못 움직인다 |
| `Downed.DownedSeconds` · `DownedMoveSpeedScale` | 60 · 0.25 | 쓰러짐 지속 · 기어가기 |
| `Downed.ReviveHealthFraction` · `RespawnHealthFraction` · `RespawnMinHunger` | 0.3 · 0.5 · 30 | 일으킴 · 부활 HP, 부활 때 배고픔 바닥 |

## 주기 효과 (서버 전용) — `UGrimSurvivalComponent`

- PlayerState 의 `OnAbilityActorInfoReady` 에서 서버가 초기값을 넣고 `TickIntervalSeconds` 타이머를 건다. 이동 배율 반영은 서버 · 클라이언트 모두.
- 틱마다 입력(HP · 배고픔 · 체온 · 피곤함 · 마나 · 등급 · 저항 · 밤 · 잠 · 쓰러짐 · 질병)을 모아 `FGrimSurvivalModel::ComputeTick` 으로 변화량을 구하고 **즉시 GE** 두 개로 적용한다(C++ CDO, 에셋 없음, SetByCaller 베이스 가산):
  - `UGrimSurvivalTickEffect` — `Survival.Health` · `Survival.Hunger` · `Survival.BodyTemperature`
  - `UGrimSurvivalExtraTickEffect` — `Survival.Fatigue` · `Survival.Mana`(감기). 0 이 아닐 때만.
- HP: 쓰러져 있으면 그대로. 굶주림(배고픔 0) · 체온 3단계면 감소, 아니고 배고프지 않으면 자연 회복(최대치까지).
- 상태 태그(`Survival/GrimSurvivalTags`, 복제 루스 태그 `TagOnly` — 모든 클라이언트가 본다):

| 태그 | 조건 |
|---|---|
| `Status.Hungry` · `Status.Starving` | 배고픔 < 20 · = 0 |
| `Status.Chilly` · `Status.Warm` | 추위 · 더위 모자람 1 |
| `Status.Hypothermia` · `Status.Hyperthermia` | 모자람 2 이상 |
| `Status.Hypothermia.Severe` · `Status.Hyperthermia.Severe` | 모자람 3 이상(위 태그와 함께) |
| `Status.Exhausted` · `Status.Collapsed` | 피곤함 ≥ 80 · 기절 중 |
| `Status.Sleeping` · `Status.Downed` | 자는 중 · 쓰러짐 |
| `Status.Sick.Stomach` · `Status.Sick.Cold` | 질병 GE 가 준다(상위 `Status.Sick`) |

- **패널티 GE** `UGrimSurvivalPenaltyEffect`(무한, `MultiplyCompound` SetByCaller 3개): 상태 → `FGrimSurvivalModel::ComputeModifiers` → 이동 · 시전 · 마나 소모 배율. 배율이 바뀌면 떼고 새 값으로 붙인다(동시에 하나).
  - 추위 1 · 2단계: 이동 · 시전 ×0.9 · ×0.75 / 더위 1 · 2단계: 마나 소모 ×1.15 · ×1.35 / Exhausted: 시전 ×0.7 / 잠: 이동 ×0 / 쓰러짐: 이동 ×0.25.
  - 이동: 컴포넌트가 `MoveSpeedScale` 변화를 듣고 폰의 `MaxWalkSpeed` 를 **비율로** 바꾼다(서버 · 소유 클라이언트, 바닥 0.01). 다른 감속(줌 조준)도 비율이면 서로 덮어쓰지 않는다.
  - 시전 속도 · 마나 소모: Magic 이 `CastSpeedScale` · `ManaCostScale` 을 읽어야 효과가 난다(아직 안 읽는다).
- 마나 회복은 Magic 시스템 몫 — `Status.Hungry` 태그를 보고 멈춘다. 감기는 회복이 도는 동안(배고프지 않고 덜 찼을 때) 초당 `ColdManaRegenPenaltyPerSecond` 를 깎는다.

## 체온 모델 — 등급 방식 (M2)

스펙 「더위 · 추위 등급과 저항」. 게이지(섭씨)는 보이는 연출이고 판정은 정수 등급이라, 장비를 갈아입으면 바로 결과가 맞는다.

```
날씨 몫   World    = UGrimWeatherSubsystem::GetClimateExposure(Location)   // { Heat, Cold } 정수, 날씨 갈래가 속을 바꾼다
개인 몫   Personal = 젖음 추위 +1, 불붙음 더위 +2, 열원 근처 추위 −2, 물속 더위 −1
등급      Exposure = max(0, World + Personal)                               // 더위 · 추위 따로, 소유 클라이언트에 복제(HUD)
모자람    Deficit  = max(0, ceil(Exposure − Resist))                        // Resist = HeatResist · ColdResist 어트리뷰트
목표 체온 = 37 − Deficit.Cold × 1.5 + Deficit.Heat × 1.5                    // 25~45 로 자름
BodyTemp += (목표 − BodyTemp) × (1 − exp(−dt / 120초))                       // 틱 간격과 무관한 지수 수렴
```

- 이 브랜치의 날씨 몫은 임시 식(쾌적 14~26℃, 4℃ 마다 한 등급)이라 탑 주변 밤(6℃)이 추위 2(저체온 · 목표 34℃)다. 날씨 갈래의 지역 표가 들어오면 초원은 낮 0 · 밤 추위 1(쌀쌀함)이 되고, 더위 · 추위는 서로 깎인다. 비가 오면 야외 폰이 젖어 개인 몫 추위 +1 이 붙는다. 모닥불 옆이면 추위 −2.
- 열원: `OverlapMultiByObjectType`(모든 오브젝트 타입, 반경 `HeatSourceRadius`) → 액터의 `UGrimElementalStatics::GetStateIntensity(State.Burning) > 0`. **열원 액터는 질의 콜리전이 켜진 컴포넌트가 있어야 잡힌다.**
- 물속: `State.Swimming` 태그 또는 캐릭터 이동의 `IsSwimming`.
- 실내(지붕)면 날씨 몫을 뺀다 — 날씨 갈래의 `IsActorSheltered(Pawn)` 가 참이면 `GetClimateExposureBreakdown(Location).Sheltered`(지역 몫만), 아니면 `.Total`. **통합 때 잇는다**(이 브랜치는 `GetClimateExposure` 만 부르고 TODO).
- 저항: 장비(M4) 전에는 `UGrimClimateResistEffect`(무한, `Survival.Resist.Heat` · `.Cold` 가산)로 확인한다 — 콘솔 `grim.Survival.Resist <더위> <추위>`. 장비 · 인챈트 · 「온기」 버프도 같은 모양의 GE.
- 화면에는 「추위 +3 · 내 저항 +2」 — `GetClimateExposure()` 와 `ColdResist` 어트리뷰트를 UI 가 읽는다(`GetClimateDeficit()` 도 있다).
- 지역 표 · 날씨 가산은 날씨 갈래의 데이터(`DT_ClimateExposure`), 개인 몫 · 패널티는 `UGrimSurvivalSettings.Climate`. 섭씨 기온은 작물 생장 · 얼음 · 녹기 같은 상호작용에 계속 쓴다.
- 불 마법으로 몸 데우기(「온기」 추위저항 +2 — R16)는 마법 쪽이 저항 GE 로(나중).

## 피곤함

- 깨어 있으면 분당 `GainPerMinute`, 밤 ×`NightGainScale`, 체온 2단계 이상 ×`ModerateFatigueRateScale`. 자는 동안(낮잠 · 기절 포함) 초당 `SleepRecoveryPerSecond` 회복, 쓰러져 있으면 그대로.
- `≥ 80 → Status.Exhausted`(시전 −30%), `≥ 100 → Status.Collapsed` + 그 자리에서 강제 수면 10초(일어날 수 없다) → 깬다.
- 명중 저하 · 바람 마법 「각성」은 나중.

## 질병

- `UGrimDiseaseEffect`(지속형, 시간은 SetByCaller `Survival.Disease.Duration`) 하위 클래스 2종. 이미 걸린 병은 다시 걸지 않는다. 시간으로 낫는다.

| 질병 | 원인 | 효과 |
|---|---|---|
| 배탈 `UGrimDiseaseStomachEffect` → `Status.Sick.Stomach` | 날것 섭취(`NotifyFoodEaten(true)`) 30% | 배고픔 감소 ×2, 180초 |
| 감기 `UGrimDiseaseColdEffect` → `Status.Sick.Cold` | 젖은 채 추위 모자람 ≥ 1 이 60초 쌓일 때마다 35% | 마나 회복 −1/초, 300초. 「시전 가끔 끊김」은 태그만 — 마법 쪽이 읽는다 |

- 확률 × (1 − `DiseaseResist`/100). 원인 확률 · 지속은 `UGrimSurvivalSettings.Disease`.
- 치료: 약초 아이템(GE 제거), 빛 마법 정화(`Status.Sick` 으로 `RemoveActiveEffectsWithGrantedTags` — `CureAllDiseases()`)는 나중.
- 생물 공격 · 오염 지역 원인은 생물 · 지역 시스템이 `ApplyDisease` 를 부른다(나중).

## 먹기 · 요리

- 음식 아이템 정의에 `HungerRestore`, `bRaw`, `DiseaseChance`, `Buffs`. 먹으면 `UGrimSurvivalComponent::NotifyFoodEaten(bRaw)`(M4 인벤토리 뒤).
- 요리는 사물 상태로 한다: 날고기를 땅이나 모닥불 위에 두고 불 자극 → `State.Cooked` → 요리 아이템으로 바뀐다(R16).

## 잠 · 밤 건너뛰기

- **`AGrimBed`**(침대 · 침낭 겸용 — `bBedroll` 이면 납작, 엔진 기본 큐브 임시). 「마법으로 상호작용」의 예외: 로컬 플레이어가 `BedInteractRadius` 안이면 침대가 머리 위에 `[F] Sleep` 을 띄우고, 컨트롤러의 키 상태(`WasInputKeyJustPressed(BedInteractKey)`)를 직접 읽어 생존 컴포넌트의 서버 RPC(`ServerRequestSleep(Bed)` · `ServerRequestWake`)를 부른다. 캐릭터 입력을 고치지 않으려는 임시 길 — 캐릭터 상호작용 키가 생기면 그쪽에서 같은 RPC 를 부른다.
- 서버 `StartSleeping`: 쓰러짐 · 이미 잠 · 거리(`BedInteractRadius` + 100) · 반경 `HostileBlockRadius` 안 적대(지금은 `IGrimTargetable` 종류 Enemy)를 보고 거부한다. 받아들이면 `Status.Sleeping`, 진행 중 능력 취소, 이동 ×0. **침대에서 자면 그 침대 옆(`RespawnOffset`)이 부활 지점**(생존 컴포넌트 = PlayerState 에 저장, 세이브는 다음 묶음). 결과는 `ClientSleepResult` → `OnSleepResult`.
- 자다가 적이 다가오면 깬다. 낮잠은 피곤함이 0 이 되면 저절로 깬다.
- **`UGrimSleepSubsystem`**(서버): 1초마다 · 누가 눕거나 일어날 때 접속자(관전자 · 비활성 제외) 중 잠든 비율을 센다. `≥ SleepSkipRatio` 이고 밤이면 페이드 시작 — 소유 클라이언트에 `OnNightSkipFade(3초)`(화면 연출은 UI 몫), 서버 `OnNightSkipBegin`. 페이드 중 비율이 깨지면 취소(`OnNightSkipCancelled`). 끝나면 다시 확인하고:
  - 시각 → 해돋이 + `WakeAfterDawn`. `SetTimeOfDay` 가 날짜를 유지해서, 자정 전이면 하루 끝(0.9999)으로 갔다가 날짜가 바뀐 뒤 목표로 옮긴다(**날씨 갈래에 앞으로만 가는 `SkipTo` 요청**).
  - 잠든 사람: `Fatigue = 0`, 배고픔 −(건너뛴 하루 비율 × 하루 길이(분) × `HungerDrainPerMinute` × `SkippedHungerScale`), 깨운다. `OnNightSkipped`.
- 다 못 채우면 잠든 사람만 자는 동안 피곤함이 풀린다.

## 쓰러짐 · 죽음 · 부활

- `Health ≤ 0`(어트리뷰트 변화 · 틱) → `Status.Downed` `DownedSeconds`(60초), 진행 중 능력 취소, 이동 ×0.25, HP 회복 · 피해 멈춤. 남은 시간은 `GetDownedSecondsRemaining()`(복제).
- 그 안에 `Revive()` → MaxHealth × 0.3 으로 일어난다. 지금은 콘솔 `grim.Survival.Revive` — 동료의 빛 마법 치료가 생기면 그쪽이 부른다.
- 시간이 끝나면 사망(`OnDied` — 가방 일부 드롭 `AGrimLootBag` 은 인벤토리 M4 뒤) → 부활: 마지막 침대 옆, 없으면 `GameMode->FindPlayerStart`. **폰을 새로 만들지 않고 옮긴다**(PlayerState · 마법서 유지). HP = MaxHealth × 0.5, 체온 정상, 배고픔 최소 30. `OnRespawned`.
- 거점 핵 · 마법탑 부활 지점 고르기는 거점 시스템 뒤.

## 슈퍼치트

`GrimSuperCheat 1`(Core)이 HP · 마나 · 배고픔 · 체온 값을 고정하고, 생존 틱이 `Cheat.Super` 태그를 보고 나머지를 막는다: 등급 0(패널티 · 체온 태그 없음), 피곤함 0 으로, 질병 치료 · 새로 안 걸림, 쓰러짐 없음(쓰러져 있었으면 일으킴).

## 콘솔 (개발 빌드)

콘솔은 로컬에서 돌므로 친 플레이어의 생존 컴포넌트 `ServerDebugCommand` RPC 로 서버에서 적용한다(`Survival/GrimSurvivalDebug.cpp`). 대소문자는 가리지 않는다.

| 명령 | 뜻 |
|---|---|
| `grim.Survival.Sleep` · `grim.Survival.Wake` | 눕기(반경 안 침대, 없으면 그 자리) · 일어나기 |
| `grim.Survival.Fatigue <값>` | 피곤함 설정 |
| `grim.Survival.Down` · `grim.Survival.Revive` · `grim.Survival.Die` | HP 0(쓰러짐) · 일으키기 · 바로 사망 → 부활 |
| `grim.Survival.EatRaw` · `grim.Survival.Sick <0 배탈 / 1 감기>` · `grim.Survival.Cure` | 질병 |
| `grim.Survival.Resist <더위> <추위>` | 시험용 저항 버프(0 0 이면 뗀다) |
| `grim.Survival.SpawnBed [1]` | 내 앞 1.5m 에 침대(1 = 침낭) — 맵 배치 전 시험용 |
| `grim.Survival.Dump` · `grim.Survival.At <초> <명령> [A] [B]` | 플레이어별 상태 로그 · 몇 초 뒤 명령(헤드리스 `-ExecCmds` 용, 명령 = `Dump` · `Sleep` · `Down` …) |
| `grim.Survival.ShowDebug 0` | 디버그 표시 끄기 |

## 디버그 표시

PlayerState 의 소유 로컬 플레이어 화면 왼쪽 위에 시각 · HP · 마나 · 배고픔 · 피곤함 · 체온 · 주변 기온 · 등급과 저항 · 배율 · 상태 태그 · 쓰러짐 남은 시간을 그린다(`UDebugDrawService` "Game" 채널, HUD 클래스 불필요). 엔진 디버그 글꼴에 한글이 없어 라벨은 영어. 정식 HUD 는 UI 시스템 몫.

## 범위

- M1: HP · 배고픔 · 체온(섭씨 식).
- M2(이 문서): 체온 등급 · 저항, 피곤함, 질병 2종, 잠 · 밤 건너뛰기, 쓰러짐 · 부활. 세이브(부활 지점 · 어트리뷰트)와 장비 저항은 다음 묶음 · M4.

### 테스트

`Source/Grimoire/Survival/Tests/` — `Grimoire.Survival.Climate.Exposure` · `.Climate.Status` · `.BodyTemperature.Converge` · `.BodyTemperature.Exposure` · `.Hunger` · `.Fatigue` · `.Penalty` · `.Disease` · `.Sleep` · `.AttributeClamp` · `.Effects`(빈 월드 ASC 로 GE 확인).

2인 헤드리스 확인: `Scripts/Survival/net_check_survival.ps1`(리슨 서버 + 클라이언트) — 호스트 침대 눕기 → 클라이언트 눕기로 전원 충족 → 밤 건너뜀, 클라이언트 저항 · 쓰러짐 · 일으킴, 호스트 사망 → 침대 옆 부활, 피곤함 패널티를 양쪽 덤프로 본다.
