# QA 현황

> Claude 가 QA 항목을 만들고 **직접** 돌린 결과다. 사용자는 ⏸(사람 눈 · 실제 키 · 결정이 필요한 것)과 ❌ 버그만 보면 된다. 절차는 [QA 절차](../dev/qa).

## 마지막 전체 실행 — 2026-11-06

브랜치 `claude/qa-9dda1c`(main `a6da3f7` = M1~M3 · 생물 보관함 · 커스터마이징 · 성능 도구 · 새 볼트 이펙트). 빌드 성공, **자동화 테스트 153/153 통과**, 점검 스크립트 17개(그중 2인 이상 헤드리스 13 — 새로 만든 `Scripts/QA/check_cheats.ps1` · `net_check_cheats.ps1` 포함) + 화면 촬영 13장 + 성능 측정.

| 영역 | ✅ 통과 | ❌ 실패 | ⏸ 보류 | 주요 실패 |
|---|---|---|---|---|
| [캐릭터 · 입력 · 조준](./character) | 16 | 0 | 2 | — |
| [마법 · 상호작용](./magic) | 20 | 1 | 1 | 투사체 틱 순환 경고(BUG-002) |
| [생존 · 밤낮 · 날씨](./survival) | 12 | 7 | 1 | R29 생존 조정 미반영(BUG-003), 밤이 낮처럼 밝음(BUG-008), 비가 안 보임(BUG-009), 아이템 2인 스크립트(BUG-007) |
| [튜토리얼 · 세이브 · 접속](./tutorial) | 14 | 2 | 2 | 탑 방 바닥 풀(BUG-010) |
| [생물 · 길들이기 · 보관함](./creatures) | 13 | 1 | 1 | 길들이기 2인 스크립트(BUG-007) |
| [커스터마이징](./customization) | 8 | 0 | 2 | — (생성 화면 카메라는 Q102) |
| [치트 · 개발 명령](./cheats) | 12 | 2 | 1 | **Shift+Esc 가 안 걸림(BUG-001)**, CHEAT 표시 안 보임(BUG-011) |
| [이펙트](./fx) | 4 | 2 | 2 | Niagara Effect Type 없음(BUG-006) |
| [성능 예산](./perf) | 8 | 1 | 1 | Effect Type(BUG-006) — 숫자는 모두 예산 안 |
| **합계** | **107** | **16** | **13** | [버그 목록](./bugs) 11건(고침(QA) 1) |

## 다시 돌리기

```powershell
powershell -File Scripts\QA\run_all.ps1          # ① 자동화 + ②④ 점검 스크립트 전부(약 35분) → Saved\QA\summary.txt
powershell -File Scripts\QA\shoot_qa.ps1         # ③ 화면 장면 11개 → docs/public/qa/
powershell -File Scripts\QA\shoot_qa.ps1 -Set Pose   # ③ 완드 · 그리모어 앞모습 · 시전 순간
powershell -File Scripts\Perf\measure.ps1 -Label <이름>   # 성능(약 7분)
```

## 사람이 볼 것(⏸) 모음

| QA | 왜 사람인가 |
|---|---|
| [QA-CHR-008](./character) | 실제 키보드 · 마우스 · 패드 손맛 |
| [QA-CHR-014](./character) | 락온 표식 · 카메라 움직임(정지 그림으로 판단 어려움) |
| [QA-MAG-031](./magic) | 상대 화면에서 내 책장이 넘어가는지(두 창 동시 촬영 시나리오 없음) |
| [QA-SUR-014](./survival) | 침대 F 키 · 잠 페이드 · 쓰러짐 표시 |
| [QA-TUT-007 · 022](./tutorial) | 스승 대화 창 · 메인 메뉴 버튼 클릭 |
| [QA-CRE-026](./creatures) | 가방(L) 안 보관함 탭 — 여는 콘솔 명령이 없음 |
| [QA-APP-009 · 010](./customization) | 생성 화면 버튼(확인 · 취소), 접속 직후 외형 |
| [QA-CHT-015](./cheats) | Shipping 빌드에 치트가 없는지(쿡 빌드 필요) |
