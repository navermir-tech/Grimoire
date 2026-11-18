# QA — 생존 · 밤낮 · 날씨

[QA 절차](../dev/qa) · 스펙 [생존](../spec/survival) · [세계 · 환경](../spec/world), 설계 [생존](../systems/survival) · [환경 · 날씨](../systems/environment). 마지막 실행 **2026-11-08**(브랜치 `claude/answers-q116-032c8c` — Q102 · Q116~Q120 항목만, 나머지 행은 적힌 날짜 그대로)(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,survival,weather,items` · 화면 `Scripts\QA\shoot_qa.ps1` · 밤 화면 `Scripts\Environment\shoot_night.ps1 -Tag after`.

## 생존 스탯 (R03 · R25 · R29)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-001 | R03 | 배고픔 감소 · HP · 마나 회복(배고프면 HP 회복 없음) · 어트리뷰트 범위 | ① `Grimoire.Survival.Hunger` · `.Effects` · `.AttributeClamp` · `.Penalty` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-SUR-002 | R25 | 지역 · 날씨 등급 − 장비 저항 = 실제 등급(저항 3 이면 추위 등급이 있어도 태그 없음) | ① `Grimoire.Survival.Climate.Exposure` · `.Climate.Status` · `Grimoire.Environment.Climate.*` + ④ `net_check_survival.ps1`(클라이언트 저항 0 3) | 통과, 밤 건너뛴 뒤 클라이언트 `Cold=0` · 태그 없음 | ✅ 11-06 | `survival.txt` |
| QA-SUR-003 | R29 | **체온은 그 존에 들어가면 등급이 즉시** 적용(게이지가 천천히 따라가지 않음) | 코드 · 설정 확인 | 등급 즉시, 수렴 없음 | ❌ [BUG-003](./bugs) | `Grimoire.Survival.BodyTemperature.Converge` 가 아직 2분 지수 수렴을 시험 · `BodyTemperatureTimeConstantSeconds=120` |
| QA-SUR-004 | R29 | 체온 모자람 3 이상이 5분 → 감기(추위) · **열병**(더위) | ① `Grimoire.Survival.Disease` | 통과 + 열병 질병이 있다 | ❌ [BUG-003](./bugs) — 감기는 있음, 열병(새 질병) 없음 | 질병 표에 열병 없음(`Heatstroke` 는 더위 2단계 상태 글자) |
| QA-SUR-005 | R29 · Q116 | 피곤함: **탈진(≥ 80) = 달리기 불가 · 지친 걸음 · 마법 시전 불가**(주먹질은 됨), 강제 기절 없음. 탈진 전 「시전 속도 · 명중 저하」는 Q132 | ① `Grimoire.Survival.Fatigue` · `.Penalty` · `Grimoire.Magic.Proficiency.Spellbook`(차단 태그) · `Grimoire.Magic.CastFailure.Classify` + ④ `net_check_survival.ps1`(피곤함 85 → 슬롯 1 → 기본 공격) | 탈진에서 걸음 ×0.7 · 시전 배율 그대로 · 슬롯 마법은 「마법 실패 Exhausted」 · 기본 공격은 주먹질, 설정에 기절 키 없음 | ✅ 11-08(브랜치 `claude/answers-q116-032c8c`) — 서버 · 클라이언트 덤프 `Fatigue=85.0 Move=0.70 Cast=1.00 Walk=350 tags=Status.Exhausted`(걸음 500 → 350), 슬롯 1 → `[Server] 마법 실패 … Exhausted` · `[Client] 마법 실패 알림 Exhausted: 너무 지쳐 마법을 쓸 수 없다 — 잠을 자야 풀린다` · 양쪽 피식 연출, 기본 공격 → 양쪽 `주먹질`. 탈진 전 저하는 아직 없음(Q132) | `survival.txt` · 자동화 181/181 |
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
| QA-SUR-024 | R04 · Q101 | 화면: 밤 하늘 | ③ 장면 `qa-night`(`Grim.Creature.Time 0.02`) — 자세한 것은 아래 「밤 화면」 | 어두운 하늘 · 달빛 | ✅ 11-18 — [BUG-008](./bugs) 고침, QA-SUR-037~043 | `night-meadow-after.jpg`(아래 `qa-night.jpg` 는 고치기 전) |
| QA-SUR-025 | R04 | 화면: 비 | ③ 장면 `qa-rain`(`Grim.Weather.Set All Rain 0`) | 빗줄기 · 젖은 표면 | ❌ [BUG-009](./bugs) — 하늘이 조금 흐려질 뿐 빗줄기가 안 보인다 | `qa-rain.jpg` |

## 밤 화면 (Q101)

2026-11-18, 브랜치 `claude/night-exposure-1a1880`. 목표: 달빛 아래 땅 · 나무 · 캐릭터 윤곽이 또렷하고 색은 많이 빠진 밤(호그와트 레거시 밤 정도), 불 곁은 확실히 밝게, 낮 · 노을 · 탑 안은 그대로 — 설계 [환경 「밤 화면」](../systems/environment#밤-화면-q101-·-bug-008-2026-11-18).

방법: `powershell -File Scripts\Environment\shoot_night.ps1 -Tag before|after` — L_OpenBeta 큰길 200 m(성능 자리 `path200`) · 탑 문 앞 · 탑 홀(고정 카메라)에서 같은 자리 · 같은 카메라로 12장면, 하루 길이를 늘려 시각을 멈춘다. 「전」 은 같은 스크립트로 고치기 전 코드(`c1486f6a`)를 빌드해 찍었다. 수는 그림 가운데 60 % 의 평균 밝기(0~255, 스크립트가 출력). 캐릭터 외형은 실행마다 무작위라 다르다.

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-037 | Q101 | 밤 섞임: 낮 · 해넘이 순간 0, 한밤 1, 하루 내내 한 번에 튀지 않음, 낮은 달은 최저 높이로, 해넘이 노출 하한 ≤ 밤 하한 | ① `Grimoire.Environment.Sky.NightBlend` | 통과 | ✅ 11-18 | 자동화 |
| QA-SUR-038 | Q101 | 밤 초원(0.02): 하늘 · 땅 · 나무 · 캐릭터 윤곽이 보이고 색이 빠진다 — 낮처럼 밝지도, 캄캄하지도 않다 | ③ `night-meadow` · `night-tower` | 밝기 낮의 1/3 쯤, 실루엣 | ✅ 11-18 — 초원 150.3 → 55.8, 탑 앞 145.0 → 47.5(낮 159.7). 나무 · 길 · 캐릭터 윤곽, 풀은 회녹색, 탑 문 안 불빛만 밝다 | [그림](#밤-화면-그림) |
| QA-SUR-039 | Q101 | 해질녘 · 새벽에 튀지 않는다: 낮 → 해 낮음 → 해넘이 → 막 진 뒤 → 어스름 → 한밤이 차례로 어두워진다 | ③ `night-day` · `night-dusk-a`~`d` · `night-meadow` | 해넘이부터 밤까지 밝기가 줄곧 줄거나 밤 수준에 머문다(노을이 하얗게 뜨거나, 막 진 뒤가 낮처럼 밝아지지 않음) | ✅ 11-18 — 159.7 · 144.6 · 89.7 · 54.0 · 39.6 · 55.8(전: 161.4 · 145.0 · 93.3 · 31.8 · 39.6 · 150.3 — 전에는 막 진 뒤 깊이 꺼졌다가 한밤에 낮처럼 밝아졌다) | 같음 |
| QA-SUR-040 | Q101 | 낮 · 노을은 그대로 | ③ `night-day` · `night-dusk-a` · `night-dusk-b` 전후 | 밝기 · 색이 같다(±4) | ✅ 11-18 — 161.4 → 159.7 · 145.0 → 144.6 · 93.3 → 89.7. 그림도 같은 노을색(가중치 0 이라 후처리를 건너뜀) | 같음 |
| QA-SUR-041 | Q101 | 탑 안(등불 켠 홀)은 밤 · 낮 모두 그대로 | ③ `night-room-night` · `night-room-day` 전후 | 같다(±3) | ✅ 11-18 — 밤 154.0 → 153.9, 낮 158.2 → 158.0. 실내 판정(지붕 + 둘레 벽)을 넣기 전에는 밤 홀이 138 로 어두워졌다 | 같음 |
| QA-SUR-042 | Q101 | 모닥불 · 불 마법 곁이 확실히 밝다 | ③ `night-campfire`(옆 고정 카메라) · `night-ember-cast`(완드 섬광, 게임 0.1초) · `night-ember-hit`(땅에 맞음, 0.2초) | 어두운 둘레 속에서 불 곁 땅 · 캐릭터가 밝고 따뜻한 색 | ✅ 11-18 — 모닥불 177.5(곁 땅 · 캐릭터가 하얗게 밝다 — 조금 과하다), 불씨 섬광 80.7 · 터짐 90.6(같은 자리 불 없이 약 64), 맞은 둘레 땅이 주황 | 같음 |
| QA-SUR-043 | — | 성능: 후처리 1개 · 실내 트레이스(초당 36) · 불씨 점광원(그림자 없음, 0.6초)이 예산 안 | `Scripts\Perf\measure.ps1 -Only Render -Settle 25` 낮 · 밤(`-ExtraCmds "Grim.Creature.Time 0.02 6"`) 전후 | GPU ≤ 7 ms · 게임 ≤ 9 ms · 렌더 ≤ 10 ms | ⚠️ 11-18 — GPU 전 4.18 ~ 4.69 → 후 4.17 ~ 4.85 ms(낮 · 밤), 게임 스레드 2.77 ~ 3.44 → 2.89 ~ 3.73 ms 로 예산 안. 렌더 스레드는 다른 세션이 같은 PC 에서 게임을 띄운 동안 재서(측정 스크립트 경고) 자리마다 5.5 ~ 11.2 ms 로 흔들렸다 — 방해 없던 자리(낮 문 앞 5.83 · 언덕 5.45, 밤 생물 자리 5.69)는 전(5.2 ~ 6.2)과 같다. 합친 뒤 조용할 때 다시 잰다 | `Saved\Perf\night-before*` · `night-after*` |

### 밤 화면 그림

같은 자리 · 같은 카메라의 전(`-before`) · 후(`-after`).

![밤 초원 — 전: 자동 노출이 낮처럼 끌어올림](/qa/night-meadow-before.jpg)

![밤 초원 — 후: 달빛 아래 윤곽 · 색 빠짐](/qa/night-meadow-after.jpg)

![탑 문 앞 밤 — 전](/qa/night-tower-before.jpg)

![탑 문 앞 밤 — 후: 문 안 불빛만 밝다](/qa/night-tower-after.jpg)

![불씨가 땅에 맞은 순간 — 전](/qa/night-ember-hit-before.jpg)

![불씨가 땅에 맞은 순간 — 후: 맞은 둘레가 주황으로 밝다](/qa/night-ember-hit-after.jpg)

![불씨 완드 섬광 — 후](/qa/night-ember-cast-after.jpg)

![모닥불 — 전](/qa/night-campfire-before.jpg)

![모닥불 — 후](/qa/night-campfire-after.jpg)

![해넘이 0.8125 — 전](/qa/night-dusk-b-before.jpg)

![해넘이 0.8125 — 후(그대로)](/qa/night-dusk-b-after.jpg)

![막 진 뒤 0.819 — 전](/qa/night-dusk-c-before.jpg)

![막 진 뒤 0.819 — 후](/qa/night-dusk-c-after.jpg)

![어스름 0.83 — 후](/qa/night-dusk-d-after.jpg)

![탑 홀 밤 — 전](/qa/night-room-night-before.jpg)

![탑 홀 밤 — 후(그대로)](/qa/night-room-night-after.jpg)

![한낮 — 후(그대로)](/qa/night-day-after.jpg)

## 먹기 · 요리 (R03 · R16)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-SUR-030 | R03 | 가방 칸 · 쌓기 · 옮기기 · 표 · 먹기 · 요리 레시피 | ① `Grimoire.Items.*`(9) | 통과 | ✅ 11-06 | 자동화 |
| QA-SUR-031 | R16 · R13 | 2인: 날고기 줍기(소유자 복제) → 냄비 스튜(재료 넣기 · 끓는 중 막기 · 완성 픽업) → 먹기(배고픔 30 → 85 · 음식 버프 · 추위 저항 1) → 직접 불로 굽기 | ④ `Scripts\Items\net_check_items.ps1` | 스크립트 머리말의 기대 줄 | ❌ [BUG-007](./bugs)(스크립트 시간표) — 두 번 돌려 둘 다: 냄비 레시피 `MeatStew` · 끓는 중 · `Stew` 픽업 · 직접 불 `RoastedJackalopeMeat cooked=1` · 날고기 먹으면 `Status.Sick.Stomach` 는 **호스트 쪽에서 됨**, 클라이언트 넣기 · 먹기(배고픔 85 · 음식 버프 · 추위 저항 1)는 못 봄 | `items.txt` · `items-run1.txt` |
| QA-SUR-032 | R03 | 화면: HUD 의 HP · 마나 · 배고픔 · 체온 막대 | ① `Grimoire.UI.BarFraction` · `.AttributeName` + ③ `qa-start-hud` | 통과 + 막대가 보인다 | ✅ 11-06 (로직) · [그림](./character#그림) | 자동화 |
| QA-SUR-033 | R03 | 가방 칸 더블클릭 판정: 같은 칸 · `SlotDoubleClickSeconds` 안 두 번만 사용, 세 번째는 새 첫 누름, 느린 두 번 · 다른 칸은 아님. 사용 가능 = 음식 · 스킬북 · 장비 | ① `Grimoire.Items.BagDoubleClick.Window` | 통과 | ✅ 11-08 | 자동화 |
| QA-SUR-034 | R03 | 더블클릭 사용: 한 번 = 고르기만, 두 번 = 음식 하나 먹음(배고픔 · 「먹었다」 알림), 재료 · 빈 칸 = 줄지 않고 알림도 없음, 장비 = 입기(「입었다」 알림), 옮기기 중의 누름은 쓰지 않음 | ① `Grimoire.Items.BagDoubleClick.Use` | 통과 | ✅ 11-08 | 자동화 |
| QA-SUR-035 | R03 · R13 | 2인: 클라이언트 화면의 더블클릭이 `Server_UseItem` 으로 서버에서 처리되고 소유자에게 복제된다(호스트도 같은 입력) | ④ `Scripts\Items\net_check_bag_doubleclick.ps1` | 끝 줄 `NETCHECK_RESULT fail=0` | ✅ 11-08 | PASS 10 · 클라 고기 3 → 3 → 3 → 2 · 배고픔 29.9 → 59.8 · 털 3 그대로 · 두건 입음 · 서버 「먹었다」 2줄(클라 · 호스트) |
| QA-SUR-036 | R03 | 화면: 실제 칸 단추에 OS 마우스 메시지 경로로 더블클릭 입력 — 개수 ×3 → ×2, 배고픔 막대 30 → 60, 「먹었다」 알림, 재료는 고르기만 | ③ `Scripts\QA\shoot_qa.ps1 -Set Bag`(`Grim.Items.BagMouse`) | 그림과 `shoot.log` 의 덤프 | ✅ 11-08 · [그림](#그림) | `[BagInput] 칸 0 슬레이트 더블클릭 라우팅 됨` → 덤프 `RoastedJackalopeMeat×2` · Hunger 59.9 |

## 그림

![QA-SUR-024 — 고치기 전(11-06) 밤 — 자동 노출로 낮처럼 보인다](/qa/qa-night.jpg)

![QA-SUR-024 — 고치기 전(11-06) 같은 밤, 자동 노출을 끔(r.EyeAdaptationQuality 0) — 캄캄하다](/qa/qa-night-fixed-exposure.jpg)

![QA-SUR-025 — 비(Grim.Weather.Set All Rain 0)](/qa/qa-rain.jpg)

![QA-SUR-036 — 가방: 칸을 고른 뒤(구운 고기 ×3, 배고픔 30)](/qa/qa-bag-selected.jpg)

![QA-SUR-036 — 음식 칸 더블클릭(×2, 배고픔 60, 「먹었다」 알림)](/qa/qa-bag-doubleclick-food.jpg)

![QA-SUR-036 — 재료 칸 더블클릭(고르기만, 털 ×3 그대로)](/qa/qa-bag-doubleclick-material.jpg)
