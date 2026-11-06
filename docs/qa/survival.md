# QA — 생존 · 밤낮 · 날씨

[QA 절차](../dev/qa) · 스펙 [생존](../spec/survival) · [세계 · 환경](../spec/world), 설계 [생존](../systems/survival) · [환경 · 날씨](../systems/environment). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,survival,weather,items` · 화면 `Scripts\QA\shoot_qa.ps1`.

## 생존 스탯 (R03 · R25 · R29)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-001 | R03 | 배고픔 감소 · HP · 마나 회복(배고프면 HP 회복 없음) · 어트리뷰트 범위 | ① `Grimoire.Survival.Hunger` · `.Effects` · `.AttributeClamp` · `.Penalty` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-SUR-002 | R25 | 지역 · 날씨 등급 − 장비 저항 = 실제 등급(저항 3 이면 추위 등급이 있어도 태그 없음) | ① `Grimoire.Survival.Climate.Exposure` · `.Climate.Status` · `Grimoire.Environment.Climate.*` + ④ `net_check_survival.ps1`(클라이언트 저항 0 3) | 통과, 밤 건너뛴 뒤 클라이언트 `Cold=0` · 태그 없음 | ✅ 11-06 | `survival.txt` |
| QA-SUR-003 | R29 | **체온은 그 존에 들어가면 등급이 즉시** 적용(게이지가 천천히 따라가지 않음) | 코드 · 설정 확인 | 등급 즉시, 수렴 없음 | ❌ [BUG-003](./bugs) | `Grimoire.Survival.BodyTemperature.Converge` 가 아직 2분 지수 수렴을 시험 · `BodyTemperatureTimeConstantSeconds=120` |
| QA-SUR-004 | R29 | 체온 모자람 3 이상이 5분 → 감기(추위) · **열병**(더위) | ① `Grimoire.Survival.Disease` | 통과 + 열병 질병이 있다 | ❌ [BUG-003](./bugs) — 감기는 있음, 열병(새 질병) 없음 | 질병 표에 열병 없음(`Heatstroke` 는 더위 2단계 상태 글자) |
| QA-SUR-005 | R29 | 피곤함: 높으면 시전 속도 · 명중 저하, **탈진(≥ 80) = 달리기 불가 · 지친 걸음 · 시전 불가**, 강제 기절 없음 | ① `Grimoire.Survival.Fatigue` + ④ `net_check_survival.ps1`(피곤함 85) | 탈진에서 걸음 속도가 줄고 시전 차단 태그 | ❌ [BUG-003](./bugs) | 서버 · 클라이언트 덤프 `Fatigue=85.0 Cast=0.70 Walk=500 tags=Status.Exhausted` — 걸음 그대로, 시전은 −30% 만. 설정에 `CollapseThreshold=100` · `CollapseSleepSeconds=10`(기절) 남아 있음 |
| QA-SUR-006 | R29 | 질병 = 상태별 디버프 총칭, 치료 수단 태그(약초 · 치료 마법 · 질병별 물약 · 정화) | ① `Grimoire.Survival.Disease` + 설정 확인 | 질병 정의가 데이터(질병마다 치료 수단 태그) | ❌ [BUG-003](./bugs) — 테스트는 통과하지만 질병은 복통 · 감기 둘이 `Disease=(…)` 고정 칸이고, 치료 수단 태그 · 질병 표가 없다 | `Config/DefaultGame.ini` `Disease=(RawFoodStomachChance … ColdDurationSeconds …)` |

## 잠 · 쓰러짐 · 부활 (R29 — 2인)

`powershell -File Scripts\Survival\net_check_survival.ps1` 한 번으로 다섯 단계를 본다. 덤프는 서버 · 클라이언트에서 같은 값이어야 한다.

| ID | R | 기능 · 무엇을 | 기대 결과 | 결과 | 증거(`survival.txt`) |
|---|---|---|---|---|---|
| QA-SUR-010 | R29 | 한 사람만 자면 밤이 안 넘어간다 | 호스트 `Status.Sleeping`, `time=0.927` 그대로 | ✅ 11-06 | `[SurvivalDump Server] hej29 time=0.927 … Status.Sleeping` |
| QA-SUR-011 | R29 | **모두가** 자면 3초 페이드 뒤 밤을 건너뛰고 피곤함이 풀린다 | `밤 건너뜀 — 0.263일, 2명`, 양쪽 `time≈0.19 day=1 Fatigue≈0` | ✅ 11-06 | 서버 · 클라이언트 `time=0.193 day=1` |
| QA-SUR-012 | R29 | 쓰러짐: HP 0 → `Status.Downed`, 걸음 ×0.25 | 양쪽 `HP=0.0 Walk=125 tags=Status.Downed` | ✅ 11-06 | 같은 줄 양쪽 |
| QA-SUR-013 | R29 | 일으킴 → HP 30 쯤, 죽으면 마지막 침대 곁에서 부활(HP 50) | 클라이언트 `HP≈31`, 호스트 `부활 — 마지막 침대` · `HP≈51` · 침대 옆 위치 | ✅ 11-06 | `pos=V(X=-150, Y=130)` |
| QA-SUR-014 | R29 | 침대 · 침낭 F 키 · 잠 페이드 · 쓰러짐 시간 표시가 화면에 맞게 | ③ 사람 눈 | ⏸ 실제 F 키 · 페이드 연출은 정지 그림으로 판단 어려움 — 사용자 플레이 때 | — |

## 밤낮 · 날씨 (R04)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-020 | R04 | 서버 시간 복제 + 클라이언트 외삽, 밤 판정, 시간대 기온 곡선 | ① `Grimoire.Environment.TimeOfDay.*` · `.Temperature.TimeCurve` | 통과 | ✅ 11-06 | 자동화 |
| QA-SUR-021 | R04 · R16 | 2인 날씨: 맑음 → 비(야외 양초가 꺼지고 젖음) → 뇌우 → 낙뢰 | ④ `Scripts\Environment\net_check_weather.ps1` | 클라이언트에도 `날씨 Meadow: Clear -> Rain`, 양초 `Wet=2.00`, `Rain -> Thunderstorm`, `낙뢰 연출` 2번 | ✅ 11-06 | `weather.txt` — 서버 낙뢰 `GrimCandle_2 (500, 250, 0)` · `땅 (-1973, 1150, 0)` = 클라이언트 `낙뢰 연출` 같은 두 자리 |
| QA-SUR-022 | R04 | 날씨 블렌드 · 전환 · 낙뢰 후보 · 지역 볼륨 섞기 | ① `Grimoire.Environment.Weather.*` · `.Climate.RegionBlend` · `.Climate.Table` | 통과 | ✅ 11-06 | 자동화 |
| QA-SUR-023 | R04 | 비가 시야(100 km → 200 m) · 락온 거리(×0.90 → 뇌우 ×0.85) · 추위 등급(+1)을 바꾼다 | ④ 같은 스크립트의 위치 탐침 | `시야 20000, 락온 x0.90 … 날씨 0/1` | ✅ 11-06 | `weather.txt` |
| QA-SUR-024 | R04 | 화면: 밤 하늘 | ③ 장면 `qa-night`(`Grim.Creature.Time 0.02`) | 어두운 하늘 · 달빛 | ❌ [BUG-008](./bugs) — 자동 노출이 밤을 낮처럼 끌어올림(노출을 고정하면 캄캄함) | `qa-night.jpg` · `qa-night-fixed-exposure.jpg` |
| QA-SUR-025 | R04 | 화면: 비 | ③ 장면 `qa-rain`(`Grim.Weather.Set All Rain 0`) | 빗줄기 · 젖은 표면 | ❌ [BUG-009](./bugs) — 하늘이 조금 흐려질 뿐 빗줄기가 안 보인다 | `qa-rain.jpg` |

## 먹기 · 요리 (R03 · R16)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-030 | R03 | 가방 칸 · 쌓기 · 옮기기 · 표 · 먹기 · 요리 레시피 | ① `Grimoire.Items.*`(7) | 통과 | ✅ 11-06 | 자동화 |
| QA-SUR-031 | R16 · R13 | 2인: 날고기 줍기(소유자 복제) → 냄비 스튜(재료 넣기 · 끓는 중 막기 · 완성 픽업) → 먹기(배고픔 30 → 85 · 음식 버프 · 추위 저항 1) → 직접 불로 굽기 | ④ `Scripts\Items\net_check_items.ps1` | 스크립트 머리말의 기대 줄 | ❌ [BUG-007](./bugs)(스크립트 시간표) — 두 번 돌려 둘 다: 냄비 레시피 `MeatStew` · 끓는 중 · `Stew` 픽업 · 직접 불 `RoastedJackalopeMeat cooked=1` · 날고기 먹으면 `Status.Sick.Stomach` 는 **호스트 쪽에서 됨**, 클라이언트 넣기 · 먹기(배고픔 85 · 음식 버프 · 추위 저항 1)는 못 봄 | `items.txt` · `items-run1.txt` |
| QA-SUR-032 | R03 | 화면: HUD 의 HP · 마나 · 배고픔 · 체온 막대 | ① `Grimoire.UI.BarFraction` · `.AttributeName` + ③ `qa-start-hud` | 통과 + 막대가 보인다 | ✅ 11-06 (로직) · [그림](./character#그림) | 자동화 |

## 그림

![QA-SUR-024 — 밤(Grim.Creature.Time 0.02) — 자동 노출로 낮처럼 보인다](/qa/qa-night.jpg)

![QA-SUR-024 — 같은 밤, 자동 노출을 끔(r.EyeAdaptationQuality 0)](/qa/qa-night-fixed-exposure.jpg)

![QA-SUR-025 — 비(Grim.Weather.Set All Rain 0)](/qa/qa-rain.jpg)
