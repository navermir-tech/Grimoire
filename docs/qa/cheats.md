# QA — 치트 · 개발 명령 · 에디터

[QA 절차](../dev/qa) · 명령 설명은 [테스트 치트](../dev/cheats). 마지막 실행 **2026-11-06**, 브랜치 `claude/qa-9dda1c`(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\check_cheats.ps1`(헤드리스 단독 게임 L_OpenBeta, 약 1.5분, 끝에 `PASS`, 로그 `Saved\QA\cheats.log`) · 2인 `powershell -File Scripts\QA\net_check_cheats.ps1`(약 1.5분, 로그 `Saved\QA\netcheats_*.log`).

- 함정: `Grim.Taming.AfterJoin` 안에서 바로 친 치트(CheatManager exec — `GrimSuperCheat` 등)가 안 닿는다. `Grim.At 0 <치트>` 로 한 번 감싼다(`Grim.At` 은 플레이어 컨트롤러로 친다).

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CHT-001 | — | `Grim.At <초> <명령>` 이 맵이 뜬 뒤 그 시각에 명령을 친다 | ② `check_cheats.ps1` — 모든 단계가 `Grim.At` 으로 돈다 | `[Grim.At] <명령>` 줄이 시간표 차례대로, 각 명령의 결과 줄이 뒤따름 | ✅ 11-06 | `cheats.log` 의 `[Grim.At]` 18줄 |
| QA-CHT-002 | — | `Grim.Go.List <이름 일부>` 가 갈 곳을 보여 준다 | ② `Grim.Go.List Jack` | `[Grim.Go.List] … Jackalope …` 줄 | ✅ 11-06 | `[OK] Grim.Go.List 가 재칼로프 자리를 보여 준다` |
| QA-CHT-003 | R14 | `Grim.Go Jackalope` 는 서식 구역 가운데가 아니라 **실제 무리** 곁으로 가고, 가면 생물이 나온다(60 m 안) | ② `Grim.Go Jackalope` → 4초 뒤 `Grim.Creature.Dump` | `[Grim.Go] Jackalope(…) 곁으로 이동`, `Dump 생물 n` 의 n > 0 | ✅ 11-06 | `[OK] … 이동을 알린다` · `[OK] … 덤프 ① > 0` |
| QA-CHT-004 | — | `Grim.Go Tut_` 처럼 탑 안 대상이면 그 층 바닥에 선다 | ② `Grim.Go Tut_` → `Grim.Terrain.Probe` | `[Grim.Go] Tut_… 곁으로 이동`, 탐침의 플레이어 높이 = 그 층 바닥 + 캡슐 반 높이 | ✅ 11-06 | `Tut_Kitchen(태그) 곁으로 이동 — Z=4500` → `[Probe] 플레이어 (700, 800, 4598) 아래 땅 4500` |
| QA-CHT-005 | — | 없는 이름은 찾지 못했다고 알린다 | ② `Grim.Go NoSuchPlaceQA` | `「NoSuchPlaceQA」 를 찾지 못했다` | ✅ 11-06 | 같은 로그 |
| QA-CHT-006 | — | `Grim.Go` 를 호스트가 치면 모든 플레이어가 함께 옮겨진다 | ④ `Scripts\QA\net_check_cheats.ps1`(새로 만듦, L_OpenBeta 2인) | 둘 다 50 m 넘게 옮겨지고 서로 15 m 안 | ✅ 11-06 | `netcheats.txt` — 호스트 (−150, 800) → (70676, 21939), 클라이언트 (−86, 854) → (70676, 22139) |
| QA-CHT-007 | R14 | `Grim.Spawn Jack 3` 이 앞 6 m 에 3마리를 놓는다 | ② → 2초 뒤 덤프 | `Spawn Jackalope x3`, `Jackalope × 3 를 앞 6m 에 놓았다`, 덤프가 3 이상 늘어남 | ✅ 11-06 | `[OK] … 3마리` · `[OK] 덤프 ② = ① + 3 이상` |
| QA-CHT-008 | — | 인자 없는 `Grim.Spawn` 은 종 목록을 보여 준다 | ② `Grim.Spawn` | `사용: Grim.Spawn <종> … 종: … Jackalope …` | ✅ 11-06 | 같은 로그 |
| QA-CHT-009 | — | `Grim.Creature.Mark 1/0` 켬 · 끔 | ② | `생물 표시 켬 …` · `생물 표시 끔` | ✅ 11-06 | 같은 로그 |
| QA-CHT-010 | — | 표시가 화면에 기둥(초록 순함 · 빨강 사나움 · 노랑 안 나온 무리) · 가장 가까운 생물 줄로 보인다 | ③ `shoot_qa.ps1` 장면 `qa-creature-mark` | 기둥 · 이름 · 거리 글씨 | ✅ 11-06 — 재칼로프마다 초록 기둥 · `Jackalope 15m`, 노란 `BlackDog 무리(아직 안 나옴) 1984m`, 왼쪽 위 `가장 가까운 생물: Jackalope 14m 왼쪽 앞` | `docs/public/qa/qa-creature-mark.jpg` |
| QA-CHT-011 | R03 | `GrimSuperCheat 1` 이 HP · 마나 · 배고픔을 최대로 **고정**, `0` 이면 원래 흐름 | ① `Grimoire.Cheat.Super` + ② `GrimSuperCheat 1` → `Grim.Save.SetAttr Health 10` · `Hunger 5` → `GrimSuperCheat 0` → `Health 10` | 켠 동안 `SetAttr … (지금 최대)`, 끈 뒤 `(지금 10.00)`, 서버 로그 `슈퍼치트 켬/끔` | ✅ 11-06 | 자동화 153/153 · `[OK]` 4줄 |
| QA-CHT-012 | — | 켜 있으면 화면 왼쪽 위에 노란 `CHEAT Super ON` | ③ 장면 `qa-supercheat` | 글씨가 보인다 | ❌ [BUG-011](./bugs) — HUD 가 생존 디버그를 꺼서 글씨가 안 그려진다(치트 효과는 걸림: 배고픔 99 → 100) | `docs/public/qa/qa-supercheat.jpg` |
| QA-CHT-013 | R13 | 멀티 클라이언트도 자기 콘솔의 `GrimSuperCheat` 가 서버에서 자기에게만 걸린다 | ④ `net_check_cheats.ps1` — 클라이언트 `GrimSuperCheat 1` → 둘 다 `grim.Survival.Down` | 클라이언트 HP 그대로, 호스트는 쓰러짐, 서버 로그 `슈퍼치트 켬` 한 번 | ✅ 11-06 (결과 메시지 `GrimSuperCheat ON` 은 화면 전용이라 ⏸) | `netcheats.txt` — 클라이언트 `HP 100 → 100`, 호스트 `HP 0` |
| QA-CHT-014 | — | **에디터 PIE 종료 키 Shift+Esc**(보조 Ctrl+Esc), Esc 단독으로는 안 꺼짐 | ③ 에디터를 띄워(`UnrealEditor.exe Grimoire.uproject -abslog=Saved\QA\editor.log`) 시작 로그 확인 | `PIE 종료 단축키: Shift+Esc (보조 Ctrl+Esc)` | ❌ [BUG-001](./bugs) | QA 에디터(11-06) · 사용자 에디터(11-05) 둘 다 `PlayWorld.StopPlaySession 명령을 찾지 못했다 — Esc 가 그대로다` — 실제 키 누름은 등록부터 실패라 생략 |
| QA-CHT-015 | — | 치트는 출시 빌드(Shipping)에 없다 | 코드 확인 — `#if !UE_BUILD_SHIPPING` · `UE_WITH_CHEAT_MANAGER` | Shipping 에서 명령이 없음 | ⏸ Shipping 쿡 빌드를 아직 안 만듦 | — |

## 그림

![QA-CHT-010 — Grim.Creature.Mark 1: 재칼로프 무리 곁 기둥과 가장 가까운 생물 줄](/qa/qa-creature-mark.jpg)

![QA-CHT-012 — GrimSuperCheat 1: 왼쪽 위 CHEAT Super ON](/qa/qa-supercheat.jpg)
