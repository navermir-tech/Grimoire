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
| **S. 생존** | `Source/Grimoire/Survival/**`, `Content/Grimoire/Survival/`, 새 `AGrimBed`(Survival 안) | [생존](../spec/survival), [생존 시스템](./survival) | 체온은 `GetClimateExposure` + 개인 몫(젖음 · 불붙음 · 열원 · 실내). `HeatResist` · `ColdResist` 어트리뷰트를 만든다(장비는 M4 — 지금은 치트 · 테스트 GE 로 확인). 잠은 `SetTimeOfDay`. 맵 배치는 보고에 적는다 |
| **W. 날씨** | `Source/Grimoire/Environment/**`(위 두 함수의 이름 · 시그니처는 유지), `Content/Grimoire/Environment/`, `Content/Grimoire/Data/` 의 기후 표 | [세계 · 환경](../spec/world), [환경 시스템](./environment) | 비 · 번개는 `UGrimElementalStatics::ApplyStimulusToActor` 로만(물 · 빛 자극). 야외 판정(지붕)은 W 가 만들고 S 가 쓸 수 있게 공개 함수로 |
| **G. 마법 성장** | `Source/Grimoire/Magic/**`(전달 방식 코드는 건드리지 않고 마법서 · 서클 · 숙련도 · 스킬북), 새 `Source/Grimoire/UI/**`, `Content/Grimoire/Magic/` · `Content/Grimoire/UI/` | [마법 — 숙련도 · 스킬북 · 서클 · 그리모어 화면](../spec/magic), [마법 시스템 — 숙련도](./magic#숙련도-q26) | 스킬북은 월드 액터(다가가 「읽기」 — 침대처럼 직접 상호작용 예외, 제안). 인벤토리는 M4 라 아직 없다. HUD(마나 · HP · 배고픔 · 체온 · 슬롯)도 G 가 `UI/` 에 — 생존 값은 어트리뷰트를 읽기만 |

- `Grimoire.Build.cs` 는 목록 끝에 한 줄만. 새 태그는 자기 폴더의 `<시스템>Tags.*`. 맵 배치는 하지 않고 결과 보고에 적는다.
- `Config/DefaultGame.ini` 는 자기 섹션만 더한다(합칠 때 충돌을 줄이려고 **파일 끝**에).
- `docs/spec/decisions.md` 에 줄을 더하지 않는다(합칠 때 충돌). 바뀐 결정은 보고에 적으면 통합 때 옮긴다.

## 빌드 · 확인 규칙

[M1.5 작업 분할](./m15-plan#빌드-·-확인-규칙)과 같다. 새 자동화 테스트는 `Grimoire.Targeting.Zoom.*` · `Grimoire.Survival.*` · `Grimoire.Environment.*` · `Grimoire.Magic.Proficiency.*` · `Grimoire.UI.*` 아래.
