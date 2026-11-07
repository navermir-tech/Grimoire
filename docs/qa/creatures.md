# QA — 생물 · 길들이기 · 보관함

[QA 절차](../dev/qa) · 스펙 [마법 생물](../spec/creatures), 설계 [마법 생물 · 생태계](../systems/creatures). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,creatures,taming,storage,cheats`.

## 종 · 생태계 (R02 · R14 · R23)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CRE-001 | R23 | 종 3(재칼로프 · 검은 개 · 켈피) 에셋 · 개체 차이(암수 · 크기 · 색 · 돌연변이) 굴림 | ① `Grimoire.Creatures.SpeciesAssets` · `.Roll` · `.Actor` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-CRE-002 | R14 | 서식 구역 개체군(상한 · 접힘 · 펼침 · 회복) · 60 m 안에서 나옴 | ① `Grimoire.Creatures.Ecosystem` · `.Population` + ② `check_cheats.ps1`(`Grim.Go Jackalope` 뒤 덤프 > 0) | 통과 · 무리 곁에서 생물이 나옴 | ✅ 11-06 | 자동화 · `cheats.txt` |
| QA-CRE-003 | R14 | AI 판단(먹이 · 도망 · 사냥) · 전리품 표 | ① `Grimoire.Creatures.AIDecide` · `.Loot` | 통과 | ✅ 11-06 | 자동화 |
| QA-CRE-004 | R14 · R13 | 2인: 서버가 놓은 생물 · 서식 구역 · 피해(약해짐) · 죽음 · 전리품 · 밤에 검은 개가 재칼로프를 사냥 — 양쪽 덤프가 같다 | ④ `Scripts\Creatures\net_check_creatures.ps1` | 덤프 5번 같음, `NETCHECK_RESULT fail=0` | ✅ 11-06 | `creatures.txt` — 덤프 1~5 같음(8 · 8 · 8 · 3 · 3마리), `Damage 22.0 -> 7마리`, `drops={JackalopeMeat x2,JackalopeFur x1,JackalopeAntler x1}`, `생물 공격 BlackDog(불씨 눈빛) → … Element.Fire 피해 8.0` |
| QA-CRE-005 | R31 | 소형 생물 죽이면 튜토리얼 이벤트 `Creature.Killed.Small` | ④ 같은 스크립트 | `퀘스트 이벤트 Creature.Killed.Small → GrimPlayerState_0` | ✅ 11-06 | 같음 |
| QA-CRE-006 | R23 | 화면: 생물 모습 · 색 차이 · 표시 기둥 | ③ 장면 `qa-creature-mark` | 재칼로프 셋 · 초록 기둥 · 이름 · 거리 | ✅ 11-06 — 모습 · 기둥 · 거리는 보임, 개체마다 다른 털색은 이 거리 · 크기 그림으로는 구분이 어렵다 — [그림](./cheats#그림) | `qa-creature-mark.jpg` |

## 길들이기 · 충성도 · 탑승 (R23 · R33 · Q25 · Q27)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CRE-010 | R23 · R33 | 포획 확률(약해짐 · 잠 · 먹이 · 속성 맞음 · 완드 · 서클 차 · 크기) · 파크툼 에셋 · 명령 · 충성도 · 성격 · 세이브 왕복 | ① `Grimoire.Taming.*`(7) | 통과 | ✅ 11-06 | 자동화 |
| QA-CRE-011 | R23 · R33 · R13 | 2인: 클라이언트가 재칼로프를 약하게 → 파크툼 → 따라오기, 켈피를 파크툼 아쿠아 → 이름 → 타기 → 탄 채 시전 → 내리기 → 기다리기 | ④ `Scripts\Creatures\net_check_taming.ps1` | 덤프 6번 같음, `NETCHECK_RESULT fail=0` | ❌ [BUG-007](./bugs)(스크립트 시간표) — 2회차는 덤프 6번 모두 같음(길들임 · 따라오기 · 켈피 길들임 · 이름 · 타기 · 내리기 · 기다리기)이고 「탄 채 시전」 한 줄만 빠짐(fail=1), 1회차는 클라이언트가 맵 밖으로 떨어져 fail=8 | `taming.txt` · `taming-run1.txt` |

## 데리고 다니기 · 보관함 · 거점 상자 (R38)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-CRE-020 | R38 · Q42 | 서클별 데리고 다니는 수(1~2서클 3 · 3서클 4 · 5서클 5) · 보관함 칸(안 배움 10, 쿠스토디아 10 + 8 · 12 · 16, 노드 +4는 배웠을 때만) | ① `Grimoire.Creatures.Storage.Limits` | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-CRE-021 | R38 · Q64 | 가득 차면 보관함 → 거점 상자 순, 둘 다 안 되면 계약이 안 걸림 · 옮기기 규칙 | ① `Grimoire.Creatures.Storage.Rules` · `.Actor` | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-CRE-022 | R38 · Q71 | 마탑 마렌 「스킬북: 쿠스토디아 사기 (120 솔)」 · 「기본 완드 사기 (10 솔)」 늘 판다(두 번 사도 됨) | ① `Grimoire.Creatures.Storage.Shop` · `Grimoire.Quest.Table`(마렌 서비스 아홉 · `Buy:Wand_Basic`) | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-CRE-023 | R38 | 저장: 보관함 · 풀어놓은 생물 직렬화 | ① `Grimoire.Creatures.Storage.Serialize` · `.ReleasedSave` | 통과 | ✅ 11-06 | 자동화 |
| QA-CRE-024 | R38 · R13 | 2인: 각자 길들이기 → 스킬북(가방)으로 보관함 배움 → 넣기 → 꺼내기 → 상자로 → 풀어놓기 → 다시 데리고 다니기 | ④ `Scripts\Creatures\net_check_storage.ps1` | 클라이언트 · 서버가 본 클라이언트 · 호스트 덤프가 단계마다 맞음, `NETCHECK_RESULT fail=0` | ✅ 11-06 | `storage.txt` — `party=0/3 stored=1/8` → `party=1/3` → `box=yes boxEntries=1` → `released=1` → `party=1/3`, 15개 덤프 「맞음」 |
| QA-CRE-025 | R38 | 화면: 거점 생물 상자 관리 화면 | ③ 장면 `qa-creature-box`(`Grim.Taming.StorageLearn 1` · `SpawnBox 250` · `Box Open`) | 데리고 다님 · 보관함 · 상자 칸이 보인다 | ✅ 11-06 — 「거점 생물 상자 ([F] · [Esc] 닫기)」, `데리고 다님 0/3 · 보관함 0/8 · 상자 0/60(풀어놓음 0/10)`, 세 칸. 열린 동안에도 아래 `[F] 거점 생물 상자 열기` 안내가 남는다(사소) | `qa-creature-box.jpg` |
| QA-CRE-026 | R38 | 화면: 가방(L) 안 보관함 탭(호그와트 레거시처럼 인벤토리에서 바로) | ③ 사람 눈 | — | ⏸ 가방을 여는 콘솔 명령이 없어 장면을 못 만듦 | — |
| QA-CRE-027 | Q42 | 쿠스토디아 없이 처음부터 보관함 10마리(인벤토리처럼 넣고 꺼냄) → 11번째 거절 `StorageFullUnlearned` → 상자 없으면 계약 거절 · 상자 있으면 상자로(Q64 순서) → 배우면 18칸 | ① `Grimoire.Creatures.Storage.BaseCapacity` | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-CRE-028 | Q42 · R13 | 2인: 보관함 흐름이 새 칸 수로 그대로(배운 뒤 18칸) | ④ `Scripts\Creatures\net_check_storage.ps1` | `stored=1/18` 등 다섯 덤프 맞음, `NETCHECK_RESULT fail=0` | ✅ 11-07 — 2회차(1회차는 호스트 파크툼이 소품 `StaticMeshActor_1` 에 맞아 빗나감 — [BUG-007](./bugs) 계열, 클라이언트 쪽은 1회차도 다섯 덤프 맞음) | `Saved/storage_result2.txt` — 덤프 1~5 같음(`stored=1/18` → `party=1/3` → `boxEntries=1` → `released=1` → `party=1/3`), 클라이언트 · 호스트 덤프 10개 「맞음」, `NETCHECK_RESULT fail=0` |
| QA-CRE-029 | Q42 · M4 | 지은 생물 상자를 쿠스토디아 없이 헐어도 생물이 기본 보관함(10칸)으로 — 사라지지 않음 | ② `Scripts\Base\qa_creature_box.ps1`(C6) | `learned=0 … stored=1/10 box=no`, 「옮길 곳이 없다」 0줄, `QA_RESULT fail=0` | ✅ 11-07 | `Saved/QA/creature_box_2.log` — `learned=0 party=0/3 stored=0/10 box=yes boxEntries=1` → `learned=0 party=0/3 stored=1/10 box=no`, 경고 0줄, C5~C9 PASS |
| QA-CRE-030 | Q64 | 계약이 안 걸리면 까닭 알림(갈 곳 없음 · 서클 · 받지 않음) + 생물 자리 피식 연출 | ① `Grimoire.Magic.CastFailure.Classify`(글) · `Grimoire.Creatures.Storage.BaseCapacity`(거절 판정) — 알림 · 연출 전달 길은 [QA-MAG-043](./magic) 과 같은 `GrimCastFailure::Report` | 통과 | ⏸ 11-07 — 판정 · 글 · 전달 길은 ①④ 로 확인, 「파티 · 보관함 · 상자가 모두 가득한 채 파크툼」을 2인으로 만드는 시나리오는 아직 없다(생물 13마리 길들이기) | 자동화 |

## 그림

![QA-CRE-025 — 거점 생물 상자 관리 화면(Grim.Taming.Box Open)](/qa/qa-creature-box.jpg)
