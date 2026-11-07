# QA 절차

> **사용자가 처음 테스트하는 사람이 되지 않는다.** 기능을 만든 세션(또는 QA 세션)이 QA 항목을 쓰고, 직접 돌려 결과와 증거를 남긴 뒤에 완료를 보고한다. 결과는 [QA 현황](../qa/)에 모인다.

## 완료 보고 전 QA (모든 세션)

기능 · 수정을 「끝났다」고 보고하기 전에:

1. 그 기능의 QA 항목을 `docs/qa/<영역>.md` 에 더하거나 고친다(없으면 새 영역 페이지 + sidebar 등록).
2. 아래 「검증 수단」 순서로 직접 돌리고 결과 칸(✅ · ❌ · ⏸)과 증거를 채운다.
3. ❌ 는 [버그 목록](../qa/bugs)에 재현 절차와 함께 올린다. 자기 작업 범위의 ❌ 는 고치고 다시 돌린다.
4. ⏸ 는 왜 못 돌렸는지(사람 눈 · 실제 키 입력 · 2대 PC 등)를 적는다. 보고에 ⏸ 수를 밝힌다 — 사용자가 볼 것은 ⏸ 뿐이어야 한다.
5. 보고에 「통과 n · 실패 n · 보류 n」 한 줄.
6. 렌더 · 틱 · 스폰 · 레벨 · 에셋을 바꿨으면 `Scripts\Perf\measure.ps1 -Only Render`, 복제를 바꿨으면 `-Only Net` 으로 재고 [성능 예산](../qa/perf)과 비교한 줄을 남긴다. 최적화 세션을 언제 여는지는 [성능 — 언제 재나](../systems/performance#언제-재나-·-최적화-세션).

## 항목 형식

영역 페이지마다 표 하나(필요하면 소제목으로 나눔). 한 줄 = 한 항목.

| 칸 | 내용 |
|---|---|
| ID | `QA-<영역>-<번호>` — 영역 약어는 아래 표. 번호는 지우지 않고 늘리기만 한다(지운 항목은 ~~취소선~~) |
| R | 이어지는 요구사항 `R01`~ · 미결 `Q` 번호 |
| 기능 · 무엇을 | 한 문장 — 무엇이 맞아야 하는가 |
| 방법 | 수단 번호(①~④) + 명령 · 스크립트 · 테스트 이름. 다시 돌릴 수 있게 정확히 |
| 기대 결과 | 로그 줄 · 값 · 화면에서 보여야 할 것 |
| 결과 | ✅ 통과 · ❌ 실패(버그 ID) · ⏸ 보류(이유) · 날짜 |
| 증거 | 로그 파일 · 판정 줄 · 스크린숏(`docs/public/qa/*.jpg`) |

| 영역 | 약어 | 페이지 |
|---|---|---|
| 캐릭터 · 입력 · 조준 | CHR | [character](../qa/character) |
| 마법 · 완드 · 속성 상호작용 | MAG | [magic](../qa/magic) |
| 생존 · 밤낮 · 날씨 | SUR | [survival](../qa/survival) |
| 마법탑 튜토리얼 · 성장 · 세이브 · 접속 | TUT | [tutorial](../qa/tutorial) |
| 생물 · 길들이기 · 보관함 | CRE | [creatures](../qa/creatures) |
| 커스터마이징 | APP | [customization](../qa/customization) |
| 치트 · 개발 명령 · 에디터 | CHT | [cheats](../qa/cheats) |
| 이펙트 · 화면 | FX | [fx](../qa/fx) |
| 성능 예산 | PRF | [perf](../qa/perf) |

## 검증 수단 (이 순서로)

앞 수단으로 확인되면 뒤 수단은 쓰지 않는다. 사람 손이 덜 가는 쪽이 먼저다.

| # | 수단 | 언제 | 어떻게 |
|---|---|---|---|
| ① | **자동화 테스트** | 순수 로직 · 표 · 직렬화 · 계산 | `Automation RunTests Grimoire`(아래 명령). 새 테스트는 `Source/Grimoire/<시스템>/Tests/`, 이름 `Grimoire.<시스템>.*` |
| ② | **헤드리스 시나리오** | 월드 · 액터 · 흐름(맵 로드, 스폰, 퀘스트 단계) | `-game -nullrhi` 단독 게임 + `-ExecCmds` 에 `Grim.At <초> <명령>` 연쇄, 판정은 로그 줄. 스크립트는 `Scripts/<시스템>/check_*.ps1`, 끝에 `PASS` / `FAIL` |
| ③ | **화면 확인** | 크기 · 색 · 글꼴 · UI 배치 · 이펙트 — 눈으로만 알 수 있는 것 | RHI 를 켠 단독 게임(`-game -RenderOffscreen`)에서 `Grim.At <초> shot showui`(UI 포함) 또는 `HighResShot`(장면만). JPG 로 줄여 `docs/public/qa/` 에 두고 항목에 그림을 단다. 에디터 PIE 가 꼭 필요한 것(에디터 단축키 등)만 에디터를 띄운다 |
| ④ | **2인 멀티(서버 권위)** | 복제 · RPC · 소유자 · 서버만 바꾸는 값 | 헤드리스 리슨 서버 + 클라이언트(`-game -nullrhi`, 같은 PC), 양쪽 덤프를 비교. 스크립트 `Scripts/<시스템>/net_check_*.ps1`, 끝에 `NETCHECK_RESULT fail=0` |

```powershell
# ① 전체 자동화 테스트 — 에디터를 닫고
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire; Quit" -unattended -nopause -nosplash -NullRHI -log

# ①②④ 한 번에 — 자동화 + 모든 점검 스크립트를 차례로(약 35분). 결과 Saved\QA\summary.txt
powershell -File Scripts\QA\run_all.ps1
powershell -File Scripts\QA\run_all.ps1 -Only automation,cheats    # 고른 것만

# ③ QA 스크린숏 — docs/public/qa/qa-*.jpg
powershell -File Scripts\QA\shoot_qa.ps1
```

- 빌드 · 테스트는 **자기 워크트리**에서(`Build.bat` PowerShell, 백그라운드, `Result:` · `exit=` 줄 확인). 다른 세션이 `Build.bat` 을 돌리는 중이면 `Build.bat is already running, waiting` 으로 기다린다 — 그대로 둔다.
- **새 워크트리는 에셋이 LFS 포인터다** — 맵을 열면 `Invalid value for PACKAGE_FILE_TAG` · `The summary for the package ... is invalid` 로 빈 월드가 뜨고 모든 시나리오가 실패한다. 처음에 한 번 `git lfs checkout`(공용 `.git/lfs/objects` 에서 채움, 내려받기 없음 · 1분 안쪽).
- 언리얼 프로세스는 **자기가 띄운 PID 로만** 끈다(`Start-Process -PassThru` → `Stop-Process -Id`). 이름으로 끄면(`Stop-Process -Name` · `taskkill /IM`) 다른 세션의 측정 · 점검이 같이 죽는다.
- 점검 스크립트 포트는 스크립트마다 다르다(17777~17799). 여러 세션이 같이 돌리면 `-Port` 를 바꾼다 — `run_all.ps1` 은 18700 부터 쓴다.
- 기계가 바쁘면(다른 에디터 · 빌드) 접속 · 맵 로드가 늦어 시간표가 어긋난다 → 스크립트의 `-Start` · `-Shift` · `-RunSeconds` 를 늘려 한 번 더. 두 번 실패하면 ❌.
- 새 `.ps1` 은 **UTF-8 BOM** 으로 저장한다. Windows PowerShell 5.1 은 BOM 없는 파일을 CP949 로 읽어 한글 문자열이 깨지고 구문 오류가 난다(Write 도구는 BOM 을 안 붙인다 — 만든 뒤 `[IO.File]::WriteAllText(경로, 내용, (New-Object Text.UTF8Encoding($true)))`).
- 헤드리스(`-nullrhi`)에서는 위젯 · 메시 조각이 안 만들어진다(외형 조각 수 0 등). 그런 값은 ③ 에서 본다.

- 로그 · 요약(`Saved\QA\` · `Saved\NetCheck*\` · `Saved\Perf\`)은 커밋하지 않는다 — 판정에 쓴 줄을 QA 항목의 「증거」 칸에 옮겨 적는다. 커밋하는 것은 `docs/qa/*.md` · `docs/public/qa/*.jpg` · 스크립트뿐.

## 실패 처리

- [버그 목록](../qa/bugs)에 한 줄: `BUG-<번호>` · 발견 날짜 · QA ID · 증상 · **재현 절차**(명령 그대로) · 기대 · 실제 · 증거 · **고칠 세션**(그 파일을 소유한 갈래 — `docs/systems/m*-plan.md` 의 소유 표) · 상태(열림 · 고침 · 확인됨).
- QA 세션은 기본적으로 고치지 않는다. 사소하고 명백한 것(오타 · 스크립트 시간표 · 문서 명령 틀림)만 고치고 상태를 「고침(QA)」으로 표시한다.
- 고친 세션은 그 QA 항목을 다시 돌려 ✅ 로 바꾸고 버그 상태를 「확인됨」으로.
- 사용자가 정해야 하는 것(스펙이 비었거나 두 문서가 다름)은 [결정 기록 · 미결 질문](../spec/decisions)에 Q 행으로 올리고 항목은 ⏸(Q 번호).
