# QA — 초원 꾸밈

[QA 절차](../dev/qa) · 무엇을 [스펙 — 초원 꾸밈](../spec/meadow-dressing) · 어떻게 [초원 꾸밈 구현](../systems/meadow-dressing). 마지막 실행 **2026-11-18**(`claude/meadow-dressing-1-e6c026`).

다시 돌리기:

```powershell
$Cmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
# 1) 에셋(블렌더 · PIL → 언리얼)
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup -P art/tools/blender_meadow_kit.py -- "$PWD\Saved\MeadowKit"
python art/tools/make_meadow_textures.py
python Scripts/Dressing/make_meadow_tables.py
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Dressing\make_meadow_assets.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 2) 배치(다시 돌려도 같은 결과)
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Dressing\meadow_place.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 3) 자동화 · 2인 · 스크린숏 · 성능
& $Cmd "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire.Dressing; Quit" -unattended -nopause -nosplash -NullRHI -log
powershell -File Scripts\Dressing\net_check_meadow.ps1
powershell -File Scripts\Dressing\shoot_meadow.ps1
powershell -File Scripts\Perf\measure.ps1 -Label meadow-after
```

## 항목

결과 칸은 아래 「결과」 절에서 채운다(✅ 통과 · ❌ 실패 · ⏸ 사람 눈).

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MDW-001 | R16 | 표 셋 · 에셋 · 보상 문법 · 환경 Effect Type | ① `Grimoire.Dressing.Tables` · `.Rewards` | ✅ 11-18 — 8 · 5 · 20 행, 메시 · 재질 · 이펙트 모두 열림, 이펙트 둘 다 `EFT_Grim_Ambient` | | 자동화 로그 |
| QA-MDW-002 | R16 | 규칙표 다섯 행 + 마른 풀 · 등잔 | ① `Grimoire.Dressing.ReactionRules` | ✅ 11-18 — 모두 기대대로 | | 자동화 로그 |
| QA-MDW-003 | R16 | 반응 소품 공통 액터 — 반응 · 보상 · 아침 되돌리기 · 날려 감 | ① `Grimoire.Dressing.PropReact` | ✅ 11-18 — 첫 실행에서 모닥불이 아침에 꺼짐(❌) → `HandleDawn` 이 되돌리기 규칙을 보게 고친 뒤 통과 | | 자동화 로그 |
| QA-MDW-004 | R03 | 채집 셀 관리자 — 공유 · 사람별 · 두 번 · 시간 · 아침 · 세이브 | ① `Grimoire.Dressing.HarvestRespawn` | ✅ 11-18 | | 자동화 로그 |
| QA-MDW-005 | R13 | 복제 설정 · 클라이언트 모습 | ① `Grimoire.Dressing.Replication` | ✅ 11-18 | | 자동화 로그 |
| QA-MDW-006 | — | 다른 시스템 회귀 없음 | ① 자동화 전체 `Grimoire` | ✅ 11-18 — 195/195(앞서 189 + 새 6) | | 자동화 로그 |
| QA-MDW-010 | — | 배치 수 · 비우는 곳 | ② `meadow_place.py` 로그 「개수」 · `Saved/MeadowPlace/report.json` | ✅ 11-18 — 소품 140(D01 16 · D02 24 · D03 12 · D04 26(7 곳) · D16 18 · D20 10 · D21 20 · D29 14) · 채집점 80(셀 17) · 꽃나무 10 · 꽃 띠 30(인스턴스 524). 탑에서 가장 가까운 것 260 m, 전망대 터 안 0, 땅 높이 4.5 m 아래 0. 첫 실행은 마른 풀 15 · 등잔 3 곳 · 모닥불 8 · 떠 있는 돌 4 → 규칙을 넓혀 채웠고, 꽃이 바닷가 모래에 놓여 해안선 · 땅 높이 조건을 더했다 | | 로그 · 보고서 |
| QA-MDW-011 | — | 다시 돌려도 같은 결과(쌓이지 않음) | ② `meadow_place.py` 두 번 → 외부 액터 파일 수 | ✅ 11-18 — 「지난 MeadowDress 액터 157개 걷음 · 외부 액터 파일 157개 지움」, 맵 외부 액터 파일 2,019 → 2,176 그대로(다섯 번 돌림) | | 로그 |
| QA-MDW-020 | R13 | 2인: 상태 복제 · 클라이언트 F 채집 · 사람별 책장 · 아침 재생 | ② `Scripts/Dressing/net_check_meadow.ps1` | ✅ 11-18 — `NETCHECK_RESULT fail=0`(11 항목). 첫 실행은 1 실패 — 같은 PC 두 인스턴스가 같은 PlayerId 를 읽어 호스트도 「주웠다」로 보였다 → 스크립트에 `-GrimPlayerId` 를 따로 | | `Saved/NetCheckMeadow/*.log` |
| QA-MDW-030 | R16 | D01 바람솜꽃 — 바람에 솜이 흩날리고 줄기만 남는다 | ③ `shoot_meadow.ps1` `meadow-D01-before/after` | ✅ 11-18 — 솜이 사라지고 줄기만, 솜씨앗 픽업. 흩날리는 솜 이펙트는 그림에서 거의 안 보인다(작다) | | 그림 |
| QA-MDW-031 | R16 | D02 마른 풀 — 불이 붙고, 다 타면 재 자국 | ③ `meadow-D02-before/after` · `meadow-D02-burnt` | ✅ 11-18 — 누런 풀 → 불꽃 → 재 자국 + 돌 픽업 | | 그림 |
| QA-MDW-032 | R16 | D03 목마른 들꽃 — 물에 핀다 | ③ `meadow-D03-before/after` | ⏸ 11-18 — 핀 뒤 크기 · 자세가 바뀌고 나비가 오고 꽃잎 픽업. 시든 색(갈색)은 안 난다 — 팩 꽃 재질이 꽃잎을 못 물들인다([Q205](../spec/decisions#미결-질문)) | | 그림 |
| QA-MDW-033 | R04 | D04 돌 등잔(황혼) — 불이 켜진다 | ③ `meadow-D04-before/after` | ✅ 11-18 — 그 줄의 등잔 둘에 불꽃 · 빛(늦은 오후 0.62 — 황혼은 자동 노출로 하얗게 날아가 쓰지 않음) | | 그림 |
| QA-MDW-034 | R04 | D20 식은 모닥불(황혼) — 다시 탄다 | ③ `meadow-D20-before/after` | ✅ 11-18 — 돌 고리에 큰 불꽃 · 빛 | | 그림 |
| QA-MDW-035 | R14 | D16 나비(낮) — 바람에 흩어진다 | ③ `meadow-D16-before/after` | ✅ 11-18 — 나비 여럿 → 7.5 초 뒤 없음 | | 그림 |
| QA-MDW-036 | — | D21 책장 — 센 바람에 날려 간다 | ③ `meadow-D21-before/after` | ✅ 11-18 — 4.8 m 날려 감 | | 그림 |
| QA-MDW-037 | — | D29 떠 있는 돌 — 충격에 튕긴다 | ③ `meadow-D29-before/after` | ✅ 11-18 — 위로 튄 돌(0.35 초 뒤) | | 그림 |
| QA-MDW-038 | R03 | D10 곁가지 · 돌멩이 — F 로 거두면 사라진다 | ③ `meadow-D10-branch-*` · `meadow-D10-pebble-*` | ✅ 11-18 — 가지 · 돌이 사라지고 로그 「거둠 — Wood ×1(가방 1)」 · 「Stone ×1」. 풀에 묻혀 잘 안 보여 곁가지를 1.4~1.8 배로 키웠다 | | 그림 · 로그 |
| QA-MDW-039 | — | D26 꽃나무 · D27 꽃밭 — 초원 전경 | ③ `meadow-vista-*` | ⏸ 11-18 — 흰 꽃나무는 언덕 위에서 멀리 보인다. 꽃 띠는 색이 팩 원래 색이라 구역 색이 안 난다([Q205](../spec/decisions#미결-질문)) | | 그림 |
| QA-MDW-040 | — | 성능 — 예산 안(꾸밈 몫: 드로 콜 +300 · GPU +0.5 ms · 틱 없음 · 복제 액터 100 m 안 40) | ④ `measure.ps1` 전 · 후 | ✅ 11-18 — 드로 콜 최대 +227(몫 300 안) · GPU 4.07~4.26 ms · 틱 없음 · 대역폭 그대로. 복제 객체 90 → 247 은 예산 300 안이지만 여유가 줄었다(아래) | | `Saved/Perf/meadow-*/summary.md` |
| QA-MDW-050 | R16 | 실제 마법으로 쏘아 보기(손맛) · F 안내 글 | ⏸ 사람 — 게임에서 벤투스 · 플라미스 · 아쿠엘 · 기본 공격을 소품에 | 소프트 락온이 잡히고 반응이 위 그림대로, F 안내 「줍기 — 곁가지」 · 「줍기 — 낡은 책장」 | ⏸ | — |

## 결과

전 · 후 그림은 `Scripts/Dressing/shoot_meadow.ps1`(배치 보고서의 자리마다, 1600×900). 동그라미는 소프트 락온 표식.

| 요소 | 전 | 후 |
|---|---|---|
| D01 바람솜꽃 — 바람 | ![](/qa/meadow-D01-before.jpg) | ![](/qa/meadow-D01-after.jpg) |
| D02 마른 풀 — 불 | ![](/qa/meadow-D02-before.jpg) | ![](/qa/meadow-D02-after.jpg) |
| D02 다 탄 뒤 | ![](/qa/meadow-D02-burnt.jpg) | |
| D03 목마른 들꽃 — 물 | ![](/qa/meadow-D03-before.jpg) | ![](/qa/meadow-D03-after.jpg) |
| D04 돌 등잔 — 불 | ![](/qa/meadow-D04-before.jpg) | ![](/qa/meadow-D04-after.jpg) |
| D20 식은 모닥불 — 불 | ![](/qa/meadow-D20-before.jpg) | ![](/qa/meadow-D20-after.jpg) |
| D16 나비 — 바람 | ![](/qa/meadow-D16-before.jpg) | ![](/qa/meadow-D16-after.jpg) |
| D21 책장 — 센 바람 | ![](/qa/meadow-D21-before.jpg) | ![](/qa/meadow-D21-after.jpg) |
| D29 떠 있는 돌 — 충격 | ![](/qa/meadow-D29-before.jpg) | ![](/qa/meadow-D29-after.jpg) |
| D10 곁가지 — F | ![](/qa/meadow-D10-branch-before.jpg) | ![](/qa/meadow-D10-branch-after.jpg) |
| D10 돌멩이 — F | ![](/qa/meadow-D10-pebble-before.jpg) | ![](/qa/meadow-D10-pebble-after.jpg) |

| D26 외딴 꽃나무(흰빛, 언덕 위) | D27 꽃 띠(서쪽) | 마탑 구간 전경(서쪽 하늘) |
|---|---|---|
| ![](/qa/meadow-D26.jpg) | ![](/qa/meadow-D27-pink.jpg) | ![](/qa/meadow-vista-west.jpg) |

**사람 눈으로 고친 것(그림을 보고)**

| 회차 | 본 것 | 고친 것 |
|---|---|---|
| 1 | 솜꽃 · 등잔 · 곁가지가 회색 — 메시 재질 칸이 비었다 | `StaticMesh.set_material` 로(파이썬 배열 원소 고치기는 복사본만 바뀜) |
| 1 | 「전」 그림이 이미 반응한 뒤, 「후」 그림이 다음 자리 | 지연 명령이 맵 로딩 전부터 세고, 이름 붙은 ViewFrom 은 4 초 뒤에 찍는다 — 시간표를 다시 짬 · 몇 번째 대신 보고서 좌표로 |
| 1 | 황혼 그림이 하얗게 날아감 | 자동 노출(BUG-008 계열) — 등잔 · 모닥불은 늦은 오후로 |
| 2 | 바람솜꽃 · 들꽃이 바닷가 모래 위 | 해안선 북쪽 20 m · 땅 높이 4 m 위만 |
| 2 | 곁가지가 풀에 묻혀 안 보임 | 1.4~1.8 배, 3 cm 들어 놓기 |
| 2 | 나비가 흩어진 「후」에도 보임 | 남은 입자 수명(5~7 초)만큼 기다려 찍기 |
| 3 | 시든 꽃 · 꽃 띠 색이 안 바뀜 | 팩 재질 스위치를 켜도 꽃잎은 안 물든다 — [Q205](../spec/decisions#미결-질문)로 남김 |

### 성능

`Scripts/Perf/measure.ps1 -Label meadow-before` (배치 전) · `-Label meadow-after` (배치 뒤), 같은 바이너리 · 같은 자리. **「전」 측정 때는 다른 세션의 언리얼이 하나 더 돌고 있었다**(스크립트 경고) — 그래서 프레임 · GPU ms 는 「후」가 오히려 빠르게 나와 비교에 쓰지 않는다. 드로 콜 · 복제 객체 · 대역폭은 다른 프로세스와 상관이 없어 그대로 비교한다.

| 자리 | 드로 콜 전 → 후 | GPU ms 후 | 프레임 ms 후 | 메모리 MB 전 → 후 |
|---|---|---|---|---|
| door | 1,084 → 1,117 (+33) | 4.21 | 5.30 | 2,873 → 2,907 |
| path200 | 987 → 1,028 (+41) | 4.19 | 5.19 | 2,957 → 2,967 |
| hilltop | 856 → 906 (+50) | 4.07 | 5.17 | 2,937 → 2,968 |
| pond | 989 → 1,032 (+43) | 4.26 | 5.31 | 2,963 → 3,066 |
| valley | 534 → 534 (0) | 4.17 | 5.26 | 2,957 → 3,084 |
| creatures | 1,087 → 1,210 (+123) | 4.15 | 5.23 | 2,968 → 3,102 |
| rain | 981 → 1,208 (+227) | 4.25 | 5.22 | 2,955 → 3,097 |

| 네트워크(서버, 가만히) | 전 | 후 |
|---|---|---|
| 복제 객체 · 활성 | 90 · 67 | 247 · 219 |
| 연결당 보냄 kB/s(가만히 · 비) | 2.61 · 4.79 | 2.61 · 5.11 |
| 서버 월드 틱 ms | 1.47 | 0.86 |

판정(꾸밈 몫 예산 — [스펙 「성능 예산 안의 상한」](../spec/meadow-dressing#성능-예산-안의-상한)):

- **드로 콜 ✅** — 늘어난 것 최대 +227(rain 자리, 비 쪽 길가 등잔 줄 · 꽃 띠 · 나비가 한 화면에), 몫 +300 안. 가장 많은 자리 1,210 / 예산 2,000. 꽃 띠 · 꽃나무는 폴리지(종류 11, 몫 16 안), 채집점은 셀마다 ISM 종류 하나씩.
- **GPU ✅** — 후 4.07~4.26 ms / 예산 7 ms(전 값은 다른 프로세스 때문에 못 씀 — 지난 실측 4.18~4.86 ms 와 같은 범위).
- **게임 스레드 ✅** — 소품 · 셀 모두 틱 없음(떠 있는 돌이 튈 때 1.2 초만). 게임 ms 2.76~3.17.
- **복제 ⚠ 예산 안이지만 여유가 줄었다** — 서버 복제 객체 90 → 247(+157 = 이 작업의 액터 전부, 맵에 놓인 액터라 처음 한 번은 다 보낸다). 예산 300 안. 모두 휴면이라 대역폭은 그대로(가만히 2.61 kB/s). 2차 묶음을 더하기 전에 ① 셀의 관련 거리(지금 400 m) 줄이기 ② 반응이 없는 꾸밈(나비)을 복제하지 않는 클라이언트 전용 액터로 바꾸기를 먼저 한다.
- **메모리** +30~140 MB(자리마다 다름 — 메시 8 · 텍스처 2 · 재질, 나머지는 스트리밍 차이).

### 탑 시범 브랜치와 합친 뒤 (2026-11-18)

main(`6c285fe1`)과 `claude/tower-artstyle-3be908`(`8e7a287c`)을 이 브랜치에 합쳤다. 양쪽이 고친 `L_OpenBeta` 외부 액터 17 개는 모두 폴리지 파티션 셀(`InstancedFoliageActor`)이라 탑 쪽을 받고 `meadow_place.py` 를 다시 돌렸다 — 지난 MeadowDress 157 을 걷고 156 을 놓음(탑 쪽에서 나무 하나가 빠져 채집 셀 17 → 16), 바뀐 기존 파일은 그 셀 17 개뿐이고 탑의 다른 액터(숨김 태그 · 풀 제외 볼륨 포함)는 건드리지 않았다. 빌드 성공(유니티 묶음이 바뀌어 테스트 두 파일의 이름 겹침을 고침), 자동화 195/195, `net_check_meadow.ps1` fail=0.

| 탑 앞마당 바깥 — 탑 아트와 초원 꾸밈이 함께 | 앞마당 쪽 언덕 |
|---|---|
| ![](/qa/meadow-tower-yard.jpg) | ![](/qa/meadow-tower-yard2.jpg) |

