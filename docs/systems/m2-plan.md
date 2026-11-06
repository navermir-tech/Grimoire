# M2 작업 분할 — 살아남기

> 목표: **탑에서 시작해 첫 밤을 넘기고 3서클.** ([로드맵](./roadmap) M2)
>
> 네 갈래를 병렬로 만든다: M1.5 에서 남은 **줌인 조준**과 M2 의 **생존 · 날씨 · 마법 성장**. 세이브 v1 과 마법탑 튜토리얼 흐름은 이 넷이 합쳐진 뒤 다음 묶음으로 한다(모든 시스템의 데이터를 저장하고 엮어야 해서).

## 공통 기반 (main 에 있음 — 고치지 않는다)

| 파일 | 내용 |
|---|---|
| M1 · M1.5 전부 | 캐릭터 · 완드 · 마법 3종(Bolt · Stream · Cone) · 소프트/고정 락온 · 연출 큐 · 그리모어 책 · 상호작용 · 환경 · 생존 · 치트 |
| `Environment/GrimWeatherSubsystem` | `FGrimClimateExposure { Heat, Cold }` · `GetClimateExposure(Location)` — 지금은 기온에서 바로 셈하는 임시 식. **날씨 갈래가 속을 바꾸고, 생존 갈래는 이 함수만 부른다** |
| `Environment/GrimTimeOfDaySubsystem` | `SetTimeOfDay` — 잠으로 밤 건너뛰기에 쓴다 |
| `MagicFX/GrimMagicFXTags` | `State.Swimming` · `BuildMode` · `Carrying` · `Mounted` · `Aiming` (이름만 — 붙이는 쪽이 쓴다) |

## 갈래와 파일 소유

| 갈래 | 소유 | 스펙 · 설계 | 다른 갈래와 만나는 곳 |
|---|---|---|---|
| **Z. 줌인 조준** (M1.5 마무리) | `Source/Grimoire/Targeting/**`, `Character/GrimPlayerController.*`, `Content/Grimoire/Input/` 의 조준 입력 | [마법 — 조준 방식 네 가지](../spec/magic#조준-방식-네-가지-r24-—-호그와트-레거시-레퍼런스) | `GetCurrentAim` 이 `Mode = Zoom` 을 돌려준다(마법 쪽은 이미 대상 없으면 곧게 쏜다). 이동 감속 · 카메라는 런타임에 폰 · `GetCameraBoom()` 을 조절(캐릭터 파일 안 고침). `State.Aiming` 태그를 붙인다 |
| **S. 생존** | `Source/Grimoire/Survival/**`, `Content/Grimoire/Survival/`(안 생김), 새 `AGrimBed`(Survival 안) | [생존](../spec/survival), [생존 시스템](./survival) | 체온은 `GetClimateExposure` + 개인 몫(젖음 · 불붙음 · 열원 · 실내). `HeatResist` · `ColdResist` 어트리뷰트를 만든다(장비는 M4 — 지금은 치트 · 테스트 GE 로 확인). 잠은 `SetTimeOfDay`. 맵 배치는 보고에 적는다 |
| **W. 날씨** | `Source/Grimoire/Environment/**`(위 두 함수의 이름 · 시그니처는 유지), `Content/Grimoire/Environment/`, `Content/Grimoire/Data/` 의 기후 표 | [세계 · 환경](../spec/world), [환경 시스템](./environment) | 비 · 번개는 `UGrimElementalStatics::ApplyStimulusToActor` 로만(물 · 빛 자극). 야외 판정(지붕)은 W 가 만들고 S 가 쓸 수 있게 공개 함수로 |
| **G. 마법 성장** | `Source/Grimoire/Magic/**`(전달 방식 코드는 건드리지 않고 마법서 · 서클 · 숙련도 · 스킬북), 새 `Source/Grimoire/UI/**`, `Content/Grimoire/Magic/` · `Content/Grimoire/UI/` | [마법 — 숙련도 · 스킬북 · 서클 · 그리모어 화면](../spec/magic), [마법 시스템 — 숙련도](./magic#숙련도-q26) | 스킬북은 월드 액터(다가가 「읽기」 — 침대처럼 직접 상호작용 예외, 제안). 인벤토리는 M4 라 아직 없다. HUD(마나 · HP · 배고픔 · 체온 · 슬롯)도 G 가 `UI/` 에 — 생존 값은 어트리뷰트를 읽기만 |

- `Grimoire.Build.cs` 는 목록 끝에 한 줄만. 새 태그는 자기 폴더의 `<시스템>Tags.*`. 맵 배치는 하지 않고 결과 보고에 적는다.
- `Config/DefaultGame.ini` 는 자기 섹션만 더한다(합칠 때 충돌을 줄이려고 **파일 끝**에).
- `docs/spec/decisions.md` 에 줄을 더하지 않는다(합칠 때 충돌). 바뀐 결정은 보고에 적으면 통합 때 옮긴다.

## 빌드 · 확인 규칙

[M1.5 작업 분할](./m15-plan#빌드-·-확인-규칙)과 같다. 새 자동화 테스트는 `Grimoire.Targeting.Zoom.*` · `Grimoire.Survival.*` · `Grimoire.Environment.*` · `Grimoire.Magic.Proficiency.*` · `Grimoire.UI.*` 아래.

## 통합 결과 (2026-11-05)

- Z(`8187f64`) → W(`2322dfd`) → S(`80a8c94`) → G(`ee55dba`) 순으로 main 에 합쳤다. 충돌은 `Config/DefaultGame.ini` 두 번(섹션을 모두 살림)뿐.
- 통합 때 고친 것: 줌인 조준이면 마법이 대상을 따라가지 않고 겨눈 지점으로(`UGrimSpellAbility::FollowsTarget`, 테스트 `Grimoire.Magic.Delivery.ZoomAim`), 생존 체온이 지붕 아래면 날씨 몫을 뺀 지역 등급을 쓴다(`IsActorSheltered` · `GetClimateExposureBreakdown`).
- 확인: 빌드 성공, 자동화 테스트 72개 통과, `net_check_delivery.ps1` 2인 회귀 그대로. 갈래별 2인 확인은 각 보고 — 날씨 복제 · 비에 양초 꺼짐(W), 전원 잠 → 밤 건너뛰기 · 쓰러짐 · 부활(S), 줌 상태 복제(Z). G 의 복제(숙련도 · 슬롯 RPC)는 사람 2인 PIE 로 본다.
- 남은 것(다음 묶음): 세이브 v1, 마법탑 튜토리얼 흐름 · 탑 시험, 레벨 · 속성 성장 트리, 기본 공격(R26) · 호그와트식 조작(숫자키 슬롯 바로 시전 — 지금은 「고른 마법」 시전), 감기 「가끔 끊김」, HUD 의 체온 등급 · 밤 건너뛰기 페이드 · 쓰러짐 시간 표시, 맵 배치(침대 · 스킬북 · 기후 시험 볼륨), 낙뢰 피해, 정식 이펙트.
- 사람 눈으로 볼 것: HUD · 그리모어 화면(한글 글꼴 · Tab 포커스 · 커서), 줌 카메라, 침대 F 키, 날씨 연출(비 · 눈 · 모래가 같은 임시 이펙트), 시전 연출 · 그리모어 책.

### 갈래별로 만든 것

| 갈래 | 만든 것 | 주요 코드 · 스크립트 |
|---|---|---|
| Z | 줌인 조준(어깨 카메라 보간 · 이동 감속 · 토글/누르는 동안 · `State.Aiming` 복제), `GetCurrentAim` 이 `Mode = Zoom` | `Source/Grimoire/Targeting/GrimZoomAim.h` 의 `FGrimZoomAimMath`, `Source/Grimoire/Targeting/GrimTargetingComponent.h` |
| W | 기후 지역 볼륨 + 기후 표(`ClimateRegions.csv`), 지역별 날씨 상태 머신 · 복제 액터, 지붕 판정 `IsActorSheltered`, 비 자극(양초 꺼짐) · 낙뢰 | `Source/Grimoire/Environment/GrimWeatherSubsystem.h`, `Source/Grimoire/Environment/GrimClimateRegionVolume.h`, `Source/Grimoire/Environment/GrimWeatherReplicator.h`, `Scripts/Environment/net_check_weather.ps1` |
| S | 체온 등급 · 더위/추위 저항 어트리뷰트, 피곤함, 질병 2종(배탈 · 감기), 침대 · 침낭, 전원 잠이면 밤 건너뛰기, 쓰러짐 · 부활 | `Source/Grimoire/Survival/GrimBed.h`, `Source/Grimoire/Survival/GrimSleepSubsystem.h`, `Source/Grimoire/Survival/GrimSurvivalComponent.h`, `Scripts/Survival/net_check_survival.ps1` |
| G | 숙련도 D~S(등급표 `ProficiencyGrades.csv`), 스킬북 액터 · 서클 승급, 단축 슬롯, HUD · 그리모어 화면(C++ UMG) | `Source/Grimoire/Magic/GrimProficiencyTypes.h`, `Source/Grimoire/Magic/GrimProgressionComponent.h`, `Source/Grimoire/Magic/GrimSkillbook.h`, `Source/Grimoire/UI/GrimHUDWidget.h`, `Source/Grimoire/UI/GrimGrimoireWidget.h`, `Scripts/Integration/check_growth.ps1` |

- 이후: 세이브 · 튜토리얼 · 레벨 · 조작은 [M2 후반](./m2b-plan)에서 했다. 「남은 것」 중 기본 공격 · 슬롯 바로 시전 · 세이브 · 튜토리얼 · 레벨 · 성장 트리는 됐고, 정식 이펙트는 볼트만 자체 제작(2026-11-06). 감기 「가끔 끊김」 · 체온 등급 HUD 는 [생존 시스템](./survival)을 본다.

