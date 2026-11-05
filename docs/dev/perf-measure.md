# 성능 측정 방법

예산 · 규칙은 [성능 예산 · 최적화 규칙](../systems/performance). 이 페이지는 「같은 자리에서 다시 재는」 절차다.
최적화 커밋 전후로 한 번씩 돌려 숫자를 남긴다.

## 한 번에 재기

에디터를 닫고(같은 프로젝트를 두 프로세스가 열면 셰이더 · DDC 가 서로 막는다) PowerShell 에서:

```powershell
powershell -File Scripts\Perf\measure.ps1 -Label before                 # 렌더(약 4분) + 2인 네트워크(약 2.5분)
powershell -File Scripts\Perf\measure.ps1 -Label after -Only Render     # 렌더만
powershell -File Scripts\Perf\summarize.ps1 -Dir Saved\Perf\after -Compare Saved\Perf\before   # 전 → 후 표
```

결과는 `Saved\Perf\<Label>\` 에 남는다(커밋하지 않는다 — 숫자는 [성능 문서](../systems/performance#측정-기록)에 옮긴다).

| 파일 | 내용 |
|---|---|
| `summary.md` | 요약 표 — 자리별 fps · 프레임 · 스레드 · GPU · 드로 콜, GPU 패스 상위, 네트워크, 메모리 |
| `render.log` | 렌더 실행 로그. `GrimPerf\|` 줄이 자리별 측정값 |
| `csv-<자리>.csv` | 자리별 CSV 프로파일러(`CsvProfile Start/Stop`, `r.GPUCsvStatsEnabled 1`) — 패스별 GPU 시간 |
| `memreport.txt` | 마지막에 `memreport -full` |
| `net-server.log` · `net-client.log` | 2인 헤드리스 실행. `GrimPerfNet\|` 줄이 연결당 대역폭 |

## 무엇을 재는가

**렌더**: RHI 를 켠 단독 게임(`-game -RenderOffscreen`, 1920 × 1080, VSync · 프레임 제한 끔)으로 L_OpenBeta 를 열고,
자리마다 폰을 옮겨(실제 플레이 카메라) 12초 기다린 뒤 10초 잰다. 시각은 오전(0.45)으로 고정, `GrimSuperCheat 1`.

| 자리 | 월드 cm(X, Y), Yaw | 무엇을 보려고 |
|---|---|---|
| door | 4000, 0, 0 | 탑 문 앞 — 그레이박스 · 글씨가 가장 많다 |
| path200 | 23246, 2090, −26 | 큰길 200 m — 보통 이동 |
| hilltop | 56800, −11000, 140 | 봉화 언덕 — 가장 멀리 보인다(그림자 · 폴리지 거리) |
| pond | 12500, −30000, −5 | 연못 — 물 · 반사 |
| valley | 232000, 6000, 57 | 계곡 입구 2.3 km — 스트리밍 · 숲 |
| creatures | `Grim.Go Jackalope` + 12마리 소환 | 생물 AI · 이동 |
| rain | path200 + `Grim.Weather.Set All Rain 0` | 비 Niagara · 젖은 표면 |

**네트워크**: 리슨 서버 + 클라이언트 1(`-nullrhi`) 로 가만히 · 생물 옆 · 비 구간을 20초씩 잰다. 값은 연결 하나당.

## 콘솔 명령(개발 빌드)

직접 PIE · 게임에서도 쓸 수 있다.

| 명령 | 하는 일 |
|---|---|
| `Grim.Perf.Goto <X> <Y> <Yaw> [Pitch] [지연초]` | 로컬 폰을 그 XY 땅 위로 옮기고 그쪽을 보게 한다 |
| `Grim.Perf.Sample <이름> <초> [지연초]` | 프레임 · 게임 · 렌더 · RHI · GPU ms, p95, 끊김(33 ms 초과) 수, 드로 콜, 메모리, 액터 · 생물 · 글씨 수를 `GrimPerf\|` 한 줄로 |
| `Grim.Perf.Net <이름> <초> [지연초]` | 연결당 받음 · 보냄 kB/s, 패킷/s, 복제 객체 수를 `GrimPerfNet\|` 한 줄로 |

더 깊이 볼 때: `stat unit` · `stat gpu` · `stat scenerendering` · `stat streaming` · `stat net`, Unreal Insights(`-trace=default,gpu`),
`ProfileGPU`(한 프레임 패스 나무).

## 같은 바이너리로 전후 비교

PC 상태(다른 게임 · 에디터)가 바뀌면 실행마다 프레임이 수 ms 흔들린다. 최적화를 콘솔 변수로 켜고 끌 수 있게 만들고, 예전 동작을 `-ExtraCmds` 로 준 실행과 연달아 잰다.

```powershell
powershell -File Scripts\Perf\measure.ps1 -Label legacy -Only Render -ExtraCmds "Grim.Sky.SunRotationStep 0,Grim.DevLabels.MaxDistance 10000000"
powershell -File Scripts\Perf\measure.ps1 -Label after -Only Render
powershell -File Scripts\Perf\summarize.ps1 -Dir Saved\Perf\after -Compare Saved\Perf\legacy
```

## 주의
- 실행이 중간에 끊기면(다른 프로세스가 언리얼을 끄는 등) 렌더는 빠진 자리만, 네트워크는 처음부터 최대 2번 더 잰다(`render-retry*.log`). 다른 세션의 점검 스크립트는 자기 프로세스를 **Id 로** 끈다 — 이름으로 끄면 측정이 같이 죽는다.
- 네트워크 실행은 서버 · 클라이언트 모두 `t.MaxFPS 60` — 헤드리스는 제한이 없으면 1,000 fps 로 돌아 이동 RPC 가 실제의 몇 배가 된다.

- **에디터 바이너리**(`UnrealEditor-Cmd -game`)로 잰다. 쿡한 Development · Shipping 보다 게임 스레드가 무겁다 — 절대값보다 **전후 비교**에 쓴다.
- 첫 실행은 셰이더 컴파일 · DDC 채우기로 숫자가 튄다. 기준값은 두 번째 실행부터 쓴다.
- `-RenderOffscreen` 은 창이 없을 뿐 GPU 를 똑같이 쓴다. 다른 GPU 작업(브라우저 영상 · 다른 에디터)을 끄고 잰다.
