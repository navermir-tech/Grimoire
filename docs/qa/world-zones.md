# QA — 구역 이름 · 크레딧

> [구역 이름 표시 · 크레딧 탭](../systems/world-zones)(Q130 · Q137). 절차는 [QA 절차](../dev/qa). 마지막 실행 2026-11-18, 브랜치 `claude/zone-name-credits-c14a54`.
> 통과 16 · 실패 0 · 보류 2(사람 눈 · 실제 입력).

## 구역 이름 표시 (Q130)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-ZON-001 | Q130 | 게임이 읽는 구역 표가 레이아웃 정본과 같다 | ① `Grimoire.World.Zones.DataMatchesLayout` · ② `python Scripts/Terrain/export_zones.py --check` | 구역 20 · 원점 · 이름 · 한 줄 · 가운데 · 경계가 같다 / 「WorldZones.json 은 레이아웃과 같다」 | ✅ 11-18 | 자동화 197/197 |
| QA-ZON-002 | Q130 | 큰 구역 경계가 지역 1 땅을 빈틈 · 겹침 없이 덮고, 랜드마크가 맞는 구역에 든다 | ① `Grimoire.World.Zones.DataMatchesLayout`(50 m 격자) | 빈틈 0 · 겹침 0, 마법탑 = 마탑 곶 · 고목 = 중부 구릉 · 바위 문 = 북쪽 고개 · 폭포 = 작은 계곡 · 감시탑 = 강변 · 풍차 = 풍차 언덕 | ✅ 11-18 — 처음 그린 경계는 겹침 2 곳(목초지 다각형이 꼬임)으로 ❌ → 고쳐 통과 | 자동화 |
| QA-ZON-003 | Q130 | 표 읽기 · 잘못된 표 알림 | ① `Grimoire.World.Zones.Parse` | 소구역의 큰 구역을 정한다, 이름 겹침 · 모르는 Tier · 경계 없는 큰 구역 · 떠도는 소구역을 알린다 | ✅ 11-18 | 자동화 |
| QA-ZON-004 | Q130 | 자리 → 구역(다각형 · 보로노이 · 반지름) · 월드 ↔ 지도 좌표 | ① `Grimoire.World.Zones.FindAt` | 오목 다각형 홈은 밖, 반지름 밖 = 소구역 없음, 좌표 왕복 | ✅ 11-18 | 자동화 |
| QA-ZON-005 | Q130 | 들어섬 규칙 | ① `Grimoire.World.Zones.Visit` | 1 초 머물러야 · 큰 구역 → 소구역 순 · 경계 깜빡임 무시 · 「다시」는 쿨다운 뒤 · 밖으로는 조용 · 세이브 목록 합치기 | ✅ 11-18 | 자동화 |
| QA-ZON-006 | Q130 | 본 구역 세이브 형식 `World.Zones` | ① `Grimoire.World.Zones.Save` | 쓰고 읽어 같다, 두 번 읽어도 겹치지 않는다 | ✅ 11-18 | 자동화 |
| QA-ZON-007 | Q130 | 배너 시간 · 줄 · 설정 | ① `Grimoire.World.Zones.Banner` | 불투명도 0 → 1 → 0, 「다시」부터 버림, 소구역 다시 = 끔 | ✅ 11-18 | 자동화 |
| QA-ZON-008 | Q130 | 게임에서 처음 들어서면 화면 위쪽에 이름 + 한 줄이 뜨고, 큰 구역 다음 소구역이 차례로 뜬다 | ③ `powershell -File Scripts\QA\shoot_qa.ps1 -Set Zones`(`Grim.Zones Reset` 뒤 시작 자리) | 「마탑 곶」 + 「바다로 뻗은 곶, 마법탑이 지키는 첫 땅」(금빛, 큰 글씨) → 「마탑 앞마당」 + 한 줄(하늘빛, 작은 글씨) | ✅ 11-18 | `docs/public/qa/qa-zone-region-first.jpg` · `qa-zone-sector-first.jpg`(아래). `shoot.log`: 「GrimPlayerState_0 가 Sec_TowerCape 에 들어섬(처음)」 → 「Sub_Yard … (처음)」. 시작 자리가 마법탑 안이라 배경은 탑 방 |
| QA-ZON-009 | Q130 | 본 구역이 세이브에 남는다 — 다시 켜면 「처음」이 아니다 | ② `powershell -File Scripts\QA\check_zones.ps1`(혼자 두 번, 슬롯 ZoneCheck) | 1번: 마탑 곶 · 앞마당 「처음」 + 배너. 2번: 「처음」 없음, 마탑 곶 「다시」 작은 배너, 앞마당 배너 없음, 본 구역 2 | ✅ 11-18 — PASS 8/8 | `Saved\QA\zones_run1.log` · `zones_run2.log`: 1번 「GrimPlayerState_0 가 Sec_TowerCape 에 들어섬(처음)」 · 「배너: Sub_Yard 「마탑 앞마당」 (처음)」, 2번 「Sec_TowerCape 에 들어섬(다시)」 · 「본 구역 2」 |
| QA-ZON-010 | Q130 | 멀티 — 본 구역 · 배너는 플레이어마다 | ④ `powershell -File Scripts\QA\net_check_zones.ps1`(리슨 + 클라이언트, `Grim.Go GiantTree`) | 두 플레이어 각자 마탑 곶 · 앞마당 · 중부 구릉 「처음」, 배너는 자기 화면에만(호스트 · 클라이언트 각 마탑 곶 1 · 중부 구릉 1), 덤프 본 구역 3 × 2 | ✅ 11-18 — PASS 7/7 | `Saved\QA
etzones_server.log` · `netzones_client.log`: 서버 GrimPlayerState_0 · _1 각각 「처음」 3 번, 클라이언트 「[Client] 배너: Sec_CentralHills 「중부 구릉 · 거대 고목」 (처음)」 |
| QA-ZON-011 | Q130 | 하늘 글씨는 개발용 그대로 | ① 코드 확인 — `AGrimDevLabel` 은 고치지 않았다(출시 빌드 숨김 · `Grim.DevLabels.Sections`) | 바뀐 것 없음 | ✅ 11-18 | `git diff main -- Source/Grimoire/World/GrimDevLabel.*` 비어 있음 |
| QA-ZON-012 | Q130 | 실제로 걸어서 경계를 넘을 때의 느낌(배너 크기 · 시간 · 자리가 젤다 · 엘든 링 같은가) | 사람 — 에디터에서 `L_OpenBeta` 를 걸어 마탑 곶 → 남부 목초지 | 크게 · 잠깐 · 방해되지 않게 | ⏸ 사람 눈(느낌 · 취향) | — |

## 크레딧 확인 탭 (Q137)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-ZON-020 | Q137 | 크레딧 정본 데이터가 읽히고 문서와 같다 | ① `Grimoire.UI.Credits.Data` · ② `npm run docs:check` · `node Scripts/Docs/credits.mjs` | 필수 칸 있음, 문서에 같은 영어 · 한국어 표기 한 줄 · 엔진 표기 / 「크레딧: 데이터와 문서가 같다」 | ✅ 11-18 | 자동화 · docs:check 오류 0 |
| QA-ZON-021 | Q137 | 설정 칸에 「설정 · 크레딧 확인」 탭, 크레딧 줄이 쌓인다 | ① `Grimoire.UI.Credits.Panel` | 탭 바뀜, 줄 수 = 제목 · 소개 3 + 표기 × 5 + 엔진 표기 2, 모르는 탭 = 설정 | ✅ 11-18 | 자동화 |
| QA-ZON-022 | Q137 | 게임 안 Esc → 설정 → 크레딧 확인 | ③ `shoot_qa.ps1 -Set Zones`(`Grim.Online.Settings 0` · `1`) | 설정 탭(금빛 탭 + 칸들) · 크레딧 탭(Water Materials — tharlevfx, 링크, CC BY 4.0, 고친 점, 영어 표기, 엔진 표기) | ✅ 11-18 | `qa-settings-tab.jpg` · `qa-credits-tab.jpg`(아래, 이름 칸은 가렸다) |
| QA-ZON-023 | Q137 | 메인 메뉴 → 설정 → 크레딧 확인 | ③ `shoot_qa.ps1 -Set MenuCredits -Start 20` | 위와 같은 크레딧 탭이 메인 메뉴 판 안에 | ✅ 11-18 | `qa-menu-credits-tab.jpg`(아래, 이름 줄은 가렸다) |
| QA-ZON-024 | Q137 | 출시본 `CREDITS.txt` 를 같은 데이터에서 | ② `node Scripts/Docs/credits.mjs --txt <폴더>/CREDITS.txt` | UTF-8 BOM · CRLF, 제목 · 소개 · 표기(링크 · 라이선스 · 영어/한국어 한 줄) · 엔진 표기 | ✅ 11-18 | 아래 「CREDITS.txt」 |
| QA-ZON-025 | Q137 | 마우스로 탭 누르기 · 휠 스크롤 · 패드로 탭 옮기기 | 사람 | 누르면 바뀌고 길면 스크롤 | ⏸ 실제 마우스 · 패드 입력 | — |

## 그림

| 처음 — 큰 구역(마탑 곶) | 처음 — 소구역(마탑 앞마당, 나타나는 중) |
|---|---|
| ![](/qa/qa-zone-region-first.jpg) | ![](/qa/qa-zone-sector-first.jpg) |
| **Esc → 설정 탭** | **Esc → 크레딧 확인** |
| ![](/qa/qa-settings-tab.jpg) | ![](/qa/qa-credits-tab.jpg) |
| **메인 메뉴 → 크레딧 확인** | |
| ![](/qa/qa-menu-credits-tab.jpg) | |

- 화면은 `-RenderOffscreen` 으로도 UI 까지 찍혔다(`-Windowed` 필요 없음).
- 단독 실행이라 Esc 메뉴가 게임을 멈춘 상태다(`bPauseWhenStandalone`).

## CREDITS.txt (생성 결과)

```text
그리모리 (Grimoire) — 크레딧 / Credits
========================================

이 게임은 아래 에셋을 크리에이티브 커먼즈 저작자 표시(CC BY) 라이선스로 씁니다.
This game uses the following assets under Creative Commons Attribution (CC BY) licenses.

* Water Materials — tharlevfx (Fab)
  https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861
  CC BY 4.0 — https://creativecommons.org/licenses/by/4.0/
  Water Materials by tharlevfx (https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861), licensed under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Colors, wave scale, speed and strength and opacity were modified; vertex waves were disabled.
  tharlevfx 의 Water Materials — CC BY 4.0. 색 · 물결 크기와 속도와 세기 · 투명도를 고치고 정점 파도를 껐다.

Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.
```
