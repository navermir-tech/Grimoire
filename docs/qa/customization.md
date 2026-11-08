# QA — 캐릭터 커스터마이징

[QA 절차](../dev/qa) · 스펙 [캐릭터 커스터마이징](../spec/customization) (R39), 설계 [커스터마이징 — 외형 조립](../systems/customization). 마지막 실행 **2026-11-08**(브랜치 `claude/answers-q116-032c8c` — Q102 · Q116~Q120 항목만, 나머지 행은 적힌 날짜 그대로)(main `a6da3f7` 기준).

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
| QA-APP-008 | R39 · Q102 | 화면: 캐릭터 생성 화면(`GrimCreator`) — 칸들 + **정면 카메라**: 열면 전신 정면(캐릭터는 판에 가리지 않게 오른쪽), 얼굴 칸 → 얼굴 가까이, 끌어서(콘솔 `Grim.Appearance.CreatorOrbit`) 돌림, 몸 칸 → 다시 전신 | ③ `Scripts\QA\shoot_qa.ps1 -Set Creator` + ① `Grimoire.Appearance.CreatorCamera` | 네 장: 정면 전신 · 얼굴 · 45° 돌린 얼굴 · 45° 전신 | ✅ 11-08(브랜치 `claude/answers-q116-032c8c`) — 네 장 모두 기대대로. 얼굴은 머리 · 어깨까지(마네킹 `head` 뼈가 두개골 아래라 +14 cm 올림). 자리 표시 머리 메시(흰 돔)가 마네킹 얼굴을 덮어 얼굴 생김새는 아직 안 보인다(아트 자리 표시). HUD 퀘스트 · 슬롯이 생성 화면 위에 그대로 보인다 | `qa-creator.jpg` · `qa-creator-face.jpg` · `qa-creator-orbit.jpg` · `qa-creator-body-orbit.jpg` |
| QA-APP-011 | Q102 | 실제 마우스로 판 밖 빈 곳을 끌면 돌고, 판의 버튼 · 슬라이더는 그대로 눌린다(끌기가 가로채지 않음) · 닫으면 평소 카메라 | ③ 사람 눈(에디터 PIE 에서 `GrimCreator`) | 끌기로 돈다 · 버튼 눌림 · 닫으면 등 뒤 카메라 | ⏸ 마우스 끌기를 자동으로 줄 수단이 없다 — 같은 각을 콘솔로 준 화면은 QA-APP-008 | — |
| QA-APP-009 | R39 | 생성 화면에서 고르는 동안 **내 화면에만** 보이고, 확인하면 서버 검사 뒤 모두에게, Esc · 취소 = 되돌림 | ③④ 위젯 클릭 필요 | — | ⏸ 위젯 버튼을 누르는 자동화 수단이 없다(서버 쪽 검사 · 복제는 APP-004 · 006 으로 확인) | — |
| QA-APP-010 | R39 | 처음 들어온 캐릭터는 생성 화면 전에도 무작위 견습생 한 벌 | ④ 접속 직후 덤프 | 플레이어 둘이 같은 마네킹이 아님 | ⏸ 지금 스크립트는 `Randomize 777` 뒤에만 덤프한다(그 뒤 해시는 `8aba6b98` · `2ec88b87` 로 다름) — 접속 직후 덤프를 더해 다음 QA 에서 | `appearance_server.log` |

## 그림

![QA-APP-007 — Tower_Examiner 규칙 3명(자리 표시 도형)](/qa/qa-lineup.jpg)

![QA-APP-008 — GrimCreator 캐릭터 생성 화면, 정면 전신(Q102)](/qa/qa-creator.jpg)

![QA-APP-008 — 얼굴 칸 → 얼굴 가까이](/qa/qa-creator-face.jpg)

![QA-APP-008 — 끌어서 45° 돌림(얼굴)](/qa/qa-creator-orbit.jpg)

![QA-APP-008 — 몸 칸 → 다시 전신(45°)](/qa/qa-creator-body-orbit.jpg)
