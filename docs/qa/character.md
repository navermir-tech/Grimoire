# QA — 캐릭터 · 입력 · 조준

[QA 절차](../dev/qa) · 조작 스펙 [마법 — 완드 · 조준](../spec/magic#완드-r01). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준). 「호그와트 레거시 조작」(R27 · Q29)의 키 배치는 입력 에셋을 만드는 스크립트 `Scripts/Character/make_input.py` · `Scripts/UI/make_ui_input.py` · `Scripts/Targeting/make_targeting_input.py` 가 정본이다.

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,controls,delivery,join` · 화면 `Scripts\QA\shoot_qa.ps1`.

## 이동 · 카메라 · 조작

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CHR-001 | R27 | 3인칭 이동 · 지형 위 걷기 | ② `Scripts\Terrain\check_openbeta.ps1`(`Grim.Terrain.Walk` 로 북 · 동 · 북동 걷기) | 걸은 뒤 탐침 위치가 바뀌고 `MOVE_Walking`, 땅 위 | ✅ 11-06 — `[OK] 걷기 끝(떨어지지 않음)` | `Saved\TerrainCheck\openbeta.log` |
| QA-CHR-002 | R27 | 키 배치: 좌클릭 = 기본 공격, 1~4 = 슬롯 시전, 휠 = 세트 바꾸기, F1~F4 = 세트 고르기, T = 그리모어, Shift(누르는 동안) = 달리기, Q = 프로테고 자리, Ctrl = 회피 자리, Caps Lock = 고정 락온, 우클릭(누르는 동안) = 줌인 조준 | 코드 · 스크립트 확인(입력 에셋 생성 스크립트) + ① `Grimoire.Input.Labels` · `.SpecTags` · `.SlotMath` | 스크립트 표가 스펙과 같다 | ✅ 11-06 (배치 일치) | `make_input.py` `ACTIONS` · `make_ui_input.py` `UI_ACTIONS` · `make_targeting_input.py` |
| QA-CHR-003 | R27 | 패드: R2 탭 = 기본 공격, R2 누른 채 얼굴 버튼 = 슬롯 1~4, R2 + 십자키 = 세트, 십자 → 단독 = 그리모어, L3 = 달리기 토글 | 코드 확인 — `GrimPlayerCharacter::SpellInputPressed`(R2 누른 동안 Y · B 는 프로테고 · 회피로 안 감), `GrimUISubsystem::HandleToggleInput`(R2 누른 채 → 는 그리모어를 안 엶) | 키 겹침이 코드에서 갈린다 | ✅ 11-06 (코드) · 실제 패드 ⏸ | 패드 칸 · 버튼 짝은 호그와트 확인 전(`make_input.py` 주석 「불확실」) — [Q100](../spec/decisions#미결-질문) |
| QA-CHR-004 | R26 · Q4 | 기본 공격을 배우기 전 좌클릭은 **주먹질**(볼트 없음) | ④ `net_check_controls.ps1` 45초 | 서버 · 클라이언트 `주먹질` | ✅ 11-06 | `controls.txt` — `[Server] 주먹질` · `[Client] 주먹질` |
| QA-CHR-005 | R26 · R31 | 기본 공격 스킬북을 읽으면 배우고 좌클릭이 기본 공격 | ④ 같은 스크립트 46~49초 | `스킬북 … 집기(DA_Spell_BasicAttack) → Learned`, 입력 덤프 `BasicAttack=BasicAttack` | ✅ 11-06 | `controls.txt` |
| QA-CHR-006 | R27 | 숫자키 1 = 슬롯 1 마법(불씨)을 **바로** 시전 | ④ 51초 `Grim.Input.Press Slot1` | 클라이언트 `Press Input.Spell.Slot1 → DA_Spell_Ember`, 가짜 볼트 즉시, 양초 `Burning=1` 양쪽 | ✅ 11-06 | `controls.txt` |
| QA-CHR-007 | R27 | 슬롯 세트 2 로 바꾸면 빈 칸, 1 로 돌아오면 첫 세 칸 = 불씨 · 물줄기 · 미풍 | ④ 69~76초 | 서버가 본 클라이언트 `세트=2 … Ember=-`, 다시 `세트=1 … Ember=Slot1 WaterJet=Slot2 Breeze=Slot3` | ✅ 11-06 | `controls.txt` |
| QA-CHR-008 | R27 | 실제 키보드 · 마우스 · 패드로 누르는 손맛(달리기 Shift · 휠 · 패드 R2 콤보) | 사람 손 | — | ⏸ 실제 장치 입력은 자동화가 안 됨(콘솔 `Grim.Input.Press` 는 같은 길을 타지만 키 이벤트는 아님) | — |

## 조준 (R17 · R24)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CHR-010 | R17 | 소프트 락온: 카메라 앞 원뿔 · 점수 · 우선순위(적 > 반응 사물 > 사물) · 가림 · 개수 제한 | ① `Grimoire.Targeting.Cone` · `.Score` · `.Priority.Category` · `.Priority.SortAndCap` · `.Occlusion` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-CHR-011 | R17 · Q17 | 고정 락온 토글 · 튕겨서 옆 대상 | ① `Grimoire.Targeting.Lock.Switch` · `.Lock.Flick` | 통과 | ✅ 11-06 | 같음 |
| QA-CHR-012 | R24 | 줌인 조준: 누르는 동안/토글, 0.2초 블렌드, 이동 감속(230), 조준 방향, 대상 없이 겨눈 곳으로 곧게 | ① `Grimoire.Targeting.Zoom.*`(5) · `Grimoire.Magic.Delivery.ZoomAim` | 통과 | ✅ 11-06 | 같음 |
| QA-CHR-013 | R17 | 락온된 양초를 원격 클라이언트가 켜고 · 멀리서 키우고 · 가까이서 끈다(M1.5 완료 기준 ③) | ④ `net_check_delivery.ps1` | 양초 `Burning=1` → `2` → 없음 → 다시 켬 → 물줄기 `Wet ≥ 1`, 서버 · 클라이언트 같음 | ✅ 11-06 | `delivery.txt` — 서버 `[Bolt] DA_Spell_Ember 맞음 GrimCandle_1` … `Wet=2.00`, 클라이언트 같은 4줄 |
| QA-CHR-014 | R17 | 대상 표식(속성색 링) · 줌 조준점 · 고정 락온 카메라가 보기 좋은가 | ③ 사람 눈 | — | ⏸ 표식 · 카메라 움직임은 정지 그림으로 판단이 어렵다 — 사용자 플레이 때 | — |

## 기본 자세 · 2인

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CHR-020 | R21 | 완드가 오른손(`hand_r`)에 붙어 있고 양쪽에서 같다 | ④ `net_check.ps1` 의 `Grim.Magic.DumpWands` | 서버 · 클라이언트 모두 `GrimWand_0/1 … socket=hand_r offset=0.0cm` | ✅ 11-06 | `net_check.txt` |
| QA-CHR-021 | R21 | 화면: 오른손 완드 · 왼손 위에 펼쳐 떠 있는 그리모어 · HUD(HP · 마나 · 배고픔 · 체온 · 피곤함 막대, 슬롯 4칸 · 세트 1/4, 퀘스트) | ③ `shoot_qa.ps1`(`qa-start-hud`) · `shoot_qa.ps1 -Set Pose`(시작 자리 3 m 앞 고정 카메라) | 다 보인다 | ✅ 11-06 — 앞에서 보면 왼손 위 책 · 오른손 완드가 보인다. 뒤(평소 카메라)에서는 책이 얇은 판으로 몸에 가려 거의 안 보임 | `qa-start-hud.jpg` · `qa-pose-front.jpg` |
| QA-CHR-024 | R21 | 시전하면 완드를 휘두르고 책이 빛나며, 마나 · 쿨다운이 HUD 에 | ③ `-Set Pose` 의 `qa-pose-cast`(느린 시간 1/20 에서 슬롯 1) | 완드가 올라가고 책 가장자리가 빛남, 마나 100 → 92, 슬롯에 0.6 | ✅ 11-06 | `qa-pose-cast.jpg` |
| QA-CHR-022 | R13 | 2인: 클라이언트가 리슨 서버에 붙어 서로의 캐릭터가 보인다 | ④ 모든 `net_check_*` 의 `Join succeeded` · `Welcomed by server` + 서버 덤프의 플레이어 2명 | 접속 · 폰 2개 | ✅ 11-06 | `net_check.txt` 등 |
| QA-CHR-023 | R27 | T 로 그리모어 화면(배운 마법 · 서클별 정렬 · 속성 필터 · 슬롯) | ① `Grimoire.UI.GrimoireOrder` · `.ElementFilter` · `.SlotLayout` + ③ 장면 `qa-grimoire-ui`(`Grim.UI.Grimoire 1`) | 통과 + 화면 | ✅ 11-06 — 「그리모어」 창: 서클 · 다음 승급 조건, 탭(마법 · 성장 트리 · 도감), 단축 슬롯 세트 1~4 · 칸 4, 속성 필터 7, 마법 셋(플라미스 · 아쿠엘 · 벤투스)의 등급 D · 경험 · 마나 · 쿨타임 | 자동화 · `qa-grimoire-ui.jpg` |

## 그림

![QA-CHR-021 — 시작 자리: 3인칭 · HUD](/qa/qa-start-hud.jpg)

![QA-CHR-021 — 앞모습: 오른손 완드 · 왼손 위 그리모어(Grim.Cam 고정 카메라)](/qa/qa-pose-front.jpg)

![QA-CHR-024 — 불씨 시전 순간(느린 시간): 완드 휘두름 · 책 빛남 · 마나 92 · 쿨다운 0.6](/qa/qa-pose-cast.jpg)

![QA-CHR-023 — 그리모어 화면(Grim.UI.Grimoire 1)](/qa/qa-grimoire-ui.jpg)
