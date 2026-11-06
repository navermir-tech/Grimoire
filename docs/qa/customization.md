# QA — 캐릭터 커스터마이징

[QA 절차](../dev/qa) · 스펙 [캐릭터 커스터마이징](../spec/customization) (R39), 설계 [커스터마이징 — 외형 조립](../systems/customization). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,appearance` · 화면 `Scripts\QA\shoot_qa.ps1`.

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-APP-001 | R39 | 외형 CSV 표(조각 · 색 · 규칙 · NPC)를 읽는다 | ① `Grimoire.Appearance.Catalog` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-APP-002 | R39 | 저장(이름으로 — 순서가 바뀌어도 읽힘, 없어진 조각은 기본값) · 복제 직렬화 왕복 | ① `Grimoire.Appearance.Serialization` · `.NetSerialize` | 통과 | ✅ 11-06 | 같음 |
| QA-APP-003 | R39 | NPC: 같은 규칙 + 같은 씨앗 = 늘 같은 얼굴 | ① `Grimoire.Appearance.NpcDeterminism` | 통과 | ✅ 11-06 | 같음 |
| QA-APP-004 | R39 · R13 | 서버 검사: 카탈로그에 없는 조각 · NPC 전용 조각 · 범위 밖 슬라이더 → 기본값, 서클 고리는 서버가 채움 | ① `Grimoire.Appearance.Sanitize` | 통과 | ✅ 11-06 | 같음 |
| QA-APP-005 | R39 | 조립: 고리 수 = 서클, 미리보기, 체형 | ① `Grimoire.Appearance.Assemble` | 통과 | ✅ 11-06 | 같음 |
| QA-APP-006 | R39 · R13 | 2인: 서버가 모든 플레이어를 씨앗 777 로 바꾸고 시험관 3 · 사서 2 를 줄 세운 뒤 양쪽 외형 해시가 같다(늦게 들어온 사람도 같은 모습) | ④ `Scripts\Character\net_check_appearance.ps1` | 끝에 `PASS` | ✅ 11-06 | `appearance.txt` — `PASS — 외형 7개가 양쪽에서 같다` |
| QA-APP-007 | R39 | 화면: 탑 NPC 규칙(시험관)으로 줄 세운 사람 — 시안 A 실루엣 · 색 규칙 | ③ 장면 `qa-lineup` | 원통 모자 · 긴 로브 실루엣, 서클 고리 | ✅ 11-06 — 3명 모두 높은 원통 모자 · 긴 로브(자리 표시 도형), 얼굴 · 키가 서로 다름 | `qa-lineup.jpg` |
| QA-APP-008 | R39 | 화면: 캐릭터 생성 화면(`GrimCreator`) — 체형 · 얼굴 · 머리 · 옷 · 색 · 무작위 · 확인 · 취소 | ③ 장면 `qa-creator` | 화면이 열리고 칸들이 보인다 | ✅ 11-06 — 「캐릭터 만들기」: 몸(체형 · 키) · 얼굴(프리셋 · 슬라이더 6 · 피부 · 눈) · 머리(모양 · 색 · 수염) · 옷 시안 A(모자 · 로브 · 스톨 · 벨트 · 망토 · 장갑 · 신발 · 바지 · 옷 주색 · 보조색 …) · 무작위 · 취소 · 확인. 다만 카메라가 등 뒤 그대로라 **고르는 얼굴이 안 보인다** — [Q102](../spec/decisions#미결-질문) | `qa-creator.jpg` |
| QA-APP-009 | R39 | 생성 화면에서 고르는 동안 **내 화면에만** 보이고, 확인하면 서버 검사 뒤 모두에게, Esc · 취소 = 되돌림 | ③④ 위젯 클릭 필요 | — | ⏸ 위젯 버튼을 누르는 자동화 수단이 없다(서버 쪽 검사 · 복제는 APP-004 · 006 으로 확인) | — |
| QA-APP-010 | R39 | 처음 들어온 캐릭터는 생성 화면 전에도 무작위 견습생 한 벌 | ④ 접속 직후 덤프 | 플레이어 둘이 같은 마네킹이 아님 | ⏸ 지금 스크립트는 `Randomize 777` 뒤에만 덤프한다(그 뒤 해시는 `8aba6b98` · `2ec88b87` 로 다름) — 접속 직후 덤프를 더해 다음 QA 에서 | `appearance_server.log` |

## 그림

![QA-APP-007 — Tower_Examiner 규칙 3명(자리 표시 도형)](/qa/qa-lineup.jpg)

![QA-APP-008 — GrimCreator 캐릭터 생성 화면](/qa/qa-creator.jpg)
