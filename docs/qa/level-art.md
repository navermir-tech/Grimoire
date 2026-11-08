# QA — 레벨 아트

[QA 절차](../dev/qa) · 무엇을 어디에 [레벨 디자인 노트 「8. 레벨 아트 1차」](../dev/level-design#_8-레벨-아트-1차-—-fab-팩-시험-적용-2026-11-08) · 팩 규칙 [마켓 · Fab 에셋 팩](../dev/market-assets). 마지막 실행 **2026-11-08**(`claude/level-dress-fccf15`).

다시 돌리기(순서 — [레벨 디자인 「8」 풀 지도](../dev/level-design#풀-지도-—-랜드스케이프는-처음부터-새-재질로-2026-11-08)):

```powershell
$Cmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
# 1) 레벨 아트 에셋(처음 한 번 · 지형 재질을 고칠 때는 $env:GRIM_DRESS_LANDSCAPE = "1")
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Terrain\dress_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 2) 맵을 새 재질로 처음부터 → 3) 서식 구역 → 4) 입히기(랜드스케이프는 그대로). 4) 만 두 번 돌려도 같은 결과
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Terrain\build_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Integration\place_openbeta_gameplay.py" -unattended -nop4 -nosplash -nullrhi
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Terrain\dress_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 전 · 후 스크린숏 → docs/public/qa/level-dress-<자리>-<Tag>.jpg
powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set dress -Tag after
```

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-LVL-001 | R16 | 시작 구역(마탑 구간) 식생이 팩 메시로 바뀌고 구역 밖은 그레이박스 그대로 | ② `dress_openbeta.py` 로그 `식생 Tree/Bush/Rock` · `개수` | 구역 안 나무 · 덤불 · 바위가 FT_Dress_* 로, 밖은 FT_Terrain_* 로(합이 생성기 자리 수와 같다) | ✅ 11-08 — 구역 안 나무 933 · 덤불 995 · 바위 194(`FT_Dress_*`), 밖 원뿔 나무 6,724 · 덤불 6,747 · 바위 3,415(`FT_Terrain_*`), 길가 123 | `dress_openbeta.py` 로그 「개수 Dress.* · Greybox.* · PathEdge.*」 |
| QA-LVL-002 | — | 다시 돌려도 같은 결과(손 소품이 쌓이지 않는다) | ② `dress_openbeta.py` 를 두 번 → 외부 액터 파일 수 | 두 번째 로그 「지난 LevelDress 액터 25개 지움」 · 「외부 액터 파일 25개 지움」, 외부 액터 파일 수가 그대로 | ✅ 11-08 — 그루터기를 뺀 뒤 1,543 → 1,543(35 → 25 개 지움 · 25 개 놓음), 그 전 실행도 1,553 → 1,553 | `dress_openbeta.py` 로그 · `Content/__ExternalActors__/Grimoire/Maps/L_OpenBeta/` 파일 수 |
| QA-LVL-003 | — | 랜드스케이프 재질이 맵 전체에서 `M_Terrain_Dress` | ② `build_openbeta.py` 로그 「랜드스케이프 재질: M_Terrain_Dress」 · `dress_openbeta.py` 로그 「랜드스케이프 재질 그대로: M_Terrain_Dress (프록시 32개)」 · ③ 스크린숏 | 두 줄 모두, 화면에 StyleHex 흙 · 풀 텍스처 · 길 | ✅ 11-08 — 두 줄 모두, 스크린숏에 StyleHex 흙 · 풀 · 길 | 로그 · `level-dress-*-after.jpg` |
| QA-LVL-004 | — | 랜드스케이프 풀(StyleHex 풀 · 꽃 · 풀포기)이 풀밭에만 나고 길 · 모래 · 바위 비탈에는 안 난다 | ③ 스크린숏 `level-dress-path100/path200/pond-after` | 길 · 연못 모래 위 풀 없음, 풀밭은 빽빽, 45 m 밖에서 사라짐 | ✅ 11-08 — 길 · 연못 모래 위 풀 없음, 풀밭 빽빽, 멀리는 맨 땅 색 | `level-dress-path100/path200/pond-after.jpg` |
| QA-LVL-005 | — | 풀이 땅 높이에 난다 — 탑 평지 · 언덕 꼭대기에도 나고 하늘에 뜬 풀이 없다 | ③ `level-dress-door-after` · `hilltop-after` · `scan-trunk-after` | 탑 문 앞 · 봉화 언덕 꼭대기 풀밭에 풀, 화면 어디에도 뜬 풀 없음 | ✅ 11-08 — 탑 문 앞 · 봉화 언덕 꼭대기에 풀, 뜬 풀 없음(맵을 새 재질로 다시 만든 뒤. 그 전에는 ❌ — [레벨 디자인 「8」 풀 지도](../dev/level-design#풀-지도-—-랜드스케이프는-처음부터-새-재질로-2026-11-08)) | `level-dress-door/hilltop/scan-trunk-after.jpg` |
| QA-LVL-006 | — | 랜드마크 · POI · 길 · 측정 자리가 그대로이고 손 소품이 막지 않는다 | ② `check_openbeta.ps1` · `check_tutorial.ps1`(맵 · 걷기 · 표식) · ③ 스크린숏 | PASS, 소품이 길 · POI 블록 위에 없다 | ✅ 11-08 — `openbeta` PASS · `tutorial` PASS(맵 · 시작 자리 · 걷기 · 헤엄 · 표식), 자동화 179/179. 전체 `run_all.ps1` 에서 맵과 무관한 2인 스크립트 `taming` 만 ❌ — 이미 열린 [BUG-007](./bugs)(고정 시계, 「탄 채 시전」 한 줄, `L_M1_Sandbox`), `storage` 는 1회 실패 뒤 다시 돌려 PASS | `Saved\QA\summary.txt` · `openbeta.txt` · `tutorial.txt` |
| QA-LVL-007 | — | 구름 카드가 하늘 컨트롤러 밤낮과 따로 놀지 않는다(밤에 어두움) | ③ `grim.Time.Set 23` 뒤 스크린숏(`level-dress-night`) | 밤 하늘에서 구름이 빛나지 않는다 | ⏸ 11-08 — `grim.Time.Set 23` 에도 장면 전체가 낮처럼 밝다([BUG-008](./bugs) 밤이 낮처럼 밝음). 구름은 장면과 같은 밝기로 빛나 보이지 않지만 어두운 밤 판단은 BUG-008 뒤 | `level-dress-night-after.jpg` |
| QA-LVL-008 | R16 | 성능 예산 안 | ① `measure.ps1 -Label level-dress -Settle 25`(전체) | QA-PRF-001~008 모두 예산 안 | ✅ 11-08 — GPU 3.80 ~ 5.14 ms · 게임 2.44 ~ 2.94 · 렌더 4.73 ~ 6.13 · p95 ≤ 7.2 · 끊김 0 · 드로 콜 ≤ 1,068 · 2.93 GB · 대역폭 ≤ 5.4 kB/s | `Saved\Perf\level-dress\summary.md`, [QA — 성능 측정 기록](./perf#측정-기록) |
| QA-LVL-009 | — | 쓰는 팩 파일만 저장소에 | `add_pack_assets.ps1 -DryRun` | 팩별 개수 · 크기, 맵 · 우리 에셋이 참조하는 것만 | ✅ 11-08 — 116 개 202.2 MB: StyleHex 107 개 127.5 MB · BlackCherry 6 개 71.6 MB · HandPaintedClouds 3 개 3.2 MB(그루터기는 134 MB 라 맵에서 뺌) | `Saved\PackDeps\summary.txt` · `used_pack_files.txt` |

**결과(11-08)**: 통과 8 · 실패 0 · 보류 1(QA-LVL-007 — BUG-008).

![밤 23시 — 장면이 낮처럼 밝다(BUG-008), 구름 카드는 장면과 같은 밝기](/qa/level-dress-night-after.jpg)

## 전 · 후 (측정 자리 가운데 시작 구역, 눈높이 1.8 m)

| 자리 | 전(그레이박스) | 후(레벨 아트 1차) |
|---|---|---|
| 탑 문 앞(`door`) | ![](/qa/level-dress-door-before.jpg) | ![](/qa/level-dress-door-after.jpg) |
| 큰길 100 m | ![](/qa/level-dress-path100-before.jpg) | ![](/qa/level-dress-path100-after.jpg) |
| 큰길 200 m(`path200`) | ![](/qa/level-dress-path200-before.jpg) | ![](/qa/level-dress-path200-after.jpg) |
| 연못가(`pond`) | ![](/qa/level-dress-pond-before.jpg) | ![](/qa/level-dress-pond-after.jpg) |
| 봉화 언덕 꼭대기(`hilltop`) | ![](/qa/level-dress-hilltop-before.jpg) | ![](/qa/level-dress-hilltop-after.jpg) |

### Q124 · Q125 판단 자료

측정 자리 아님 — 사실적 스캔(쓰러진 나무 · 그루터기)이 StyleHex 옆에 보이는 자리.

| 동쪽 목초지(풍차 쪽, 그루터기 · 쓰러진 나무) | 연못 북동(쓰러진 나무 · 큰 바위) |
|---|---|
| ![](/qa/level-dress-compare-east-after.jpg) | ![](/qa/level-dress-compare-pond-after.jpg) |
| **쓰러진 나무 스캔 12 m 앞**(가운데 오른쪽, 풀에 묻힘) | **그루터기 스캔 9 m 앞**(가운데 어두운 덩이 — 그 뒤 용량 때문에 맵에서 뺐다) |
| ![](/qa/level-dress-scan-trunk-after.jpg) | ![](/qa/level-dress-scan-stump-after.jpg) |

Megaplants 참나무(`Tree_English_Oak_Forest_01_B`, 탑 서쪽 130 m) — 시험으로 놓았다가 지금 맵에서는 뺐다. 왼쪽은 프로젝트 설정 그대로(`r.Nanite.Foliage` 꺼짐 — 줄기 · 가지만), 오른쪽은 명령줄로만 켜서(`-ini:Engine:[/Script/Engine.RendererSettings]:r.Nanite.Foliage=True`, 처음 한 번 셰이더 다시 컴파일 수 분) 찍었다. 두 그림 오른쪽 위 하늘에 뜬 풀은 그때 남아 있던 구운 풀 지도 탓이다(뒤에 고침 — [레벨 디자인 「8」 풀 지도](../dev/level-design#풀-지도-—-랜드스케이프는-처음부터-새-재질로-2026-11-08)).

| `r.Nanite.Foliage` 꺼짐(지금) | 켬(시험) |
|---|---|
| ![](/qa/level-dress-q125-oak-bare.jpg) | ![](/qa/level-dress-q125-oak-nanitefoliage.jpg) |

- 켜면 잎이 나오지만 **어두운 사진 초록**이라 밝은 연두의 StyleHex 나무 옆에서 다른 그림처럼 보인다 — 쓰려면 Q124 의 회화풍 마스터 머티리얼(채도 · 색)이 함께 필요하다. 형태(가지 구조 · 실루엣)는 가장 좋다.

## 레벨 디자인 2차 (2026-11-08)

무엇을 왜 [레벨 디자인 「9」](../dev/level-design#_9-레벨-디자인-2차-—-능선-·-자연-분포-·-구간-구도-2026-11-08). 마지막 실행 **2026-11-08**(`claude/level-design-2-0ec4f5`).

```powershell
# 레이아웃만(맵 없이, 1 분): 설계 검사 · 등고선 지도 · 식생 점 → Saved/Terrain/preview/
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Terrain\preview_openbeta.py" -unattended -nop4 -nosplash -nullrhi
python Scripts\Terrain\overlay_foliage.py
# 맵: 위 「다시 돌리기」 2) ~ 4) 그대로(build → gameplay → dress). 스크린숏(눈높이 · 하늘 19 장, 약 12 분)
powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld2 -RunSeconds 900
```

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-LVL-010 | — | 마탑 구간 땅에 능선 · 안부 · 골이 있고, 길이 골 · 안부로 지난다 | ③ `ld2-westspine` · `ld2-eastsaddle` · `ld2-hiddenvale` · `ld2-sector-aerial` · 등고선 지도 | 서쪽 등줄기 등성이 위 바위, 동쪽 길이 낮은 고갯마루를 넘고 너머에 풍차, 두 능선 사이 숨은 골. 둥근 언덕이 같은 간격으로 늘어선 「달걀판」이 아니다 | ✅ 11-08 — 서쪽 등줄기 위 큰 바위 · 외딴 나무, 동쪽 길이 안부(8 m)를 넘고 너머에 풍차, 남동 숨은 골 비탈에 침식 물길. 하늘에서 결 있는 등성이 · 골(둥근 언덕 117 → 59) | |
| QA-LVL-011 | — | 설계 검사가 지켜진다(경사 · 구역 높이 차 · 시선 · 밀도 · 길 경사 · 마탑 구간) | ① `Grimoire.Terrain.*` · `preview_openbeta.py` 의 `analysis.txt` | 모두 통과(마탑 구간 가림 기준 65 % — 결정 기록 2026-11-08) | ✅ 11-08 — 경사 72.1 % · 목초지 → 구릉 28.2 m · 시선 최소 3 · 밀도 244 m · 길 12.9°, 마탑 구간 경사 71.7 % · 가림 69.5 % · 랜드마크 100 %(최소 2) · POI 94~116 m(테스트 `Design.*` 5 통과) | |
| QA-LVL-012 | — | 능선(점마다 높이 · 양쪽 폭) · 침식이 정해진 대로 · 다시 돌려도 같다 | ① `Grimoire.Terrain.Generator.Ridgeline` | 꺾은점 꼭대기 = 바닥 + 높이, 안부가 낮다, 왼쪽 폭 밖은 바닥, 침식은 같은 시드 = 같은 높이 · 4 m 넘게 바꾸지 않는다 | ✅ 11-08 — `Generator.Ridgeline` 통과 | |
| QA-LVL-013 | — | 나무 · 덤불 · 바위가 격자로 보이지 않는다 — 숲덩이 · 빈터 · 숲 가장자리 덤불 · 등성이 바위 · 물가 나무 | ① `Grimoire.Terrain.Foliage.Natural`(포아송 간격 · 종 값) · ③ `ld2-hilltop` · `ld2-path200` · `ld2-path400` · 식생 점 지도 | 자리끼리 최소 간격을 지키고, 숲이 몇 그루씩 모인 덩이와 빈터로(고른 간격 「과수원」 아님), 종이 무리 지어 섞인다 | ✅ 11-08 — `Foliage.Natural` 통과. 1회차 숲이 과수원처럼 고른 간격 → 숲 안 덩이(24 m)를 더해 고침(`ld2-path400` 자작나무 숲덩이 · `ld2-hilltop` 덩이 · 빈터) | |
| QA-LVL-014 | — | 구간 구도가 콘셉트 아트대로 | ③ `ld2-path100` · `ld2-path200`(돌담 · 돌 표지) · `ld2-windmill`(밀밭 · 울타리 · 건초 · 곳간) · `ld2-pond`(징검돌 · 섬) · `ld2-door`(등불) | 소품이 길 · 물 · POI 블록 위에 없고 콘셉트 구도로 읽힌다 | ✅ 11-08 — 1회차 지적(등불이 크고 문 앞을 가림 · 밀밭이 회색 판 · 곳간 지붕이 뒤집힘 · 연못 섬이 물속 덩이 · 풍차 언덕을 나무가 가림)을 고친 뒤 다시 찍음 | |
| QA-LVL-015 | — | 거대 고목 큐브 목업 — 멀리서 둥근 큰 나무, 가까이서 뿌리 · 줄기 · 가지가 나뉜다 | ③ `ld2-tree-far`(약 480 m) · `ld2-tree-near`(120 m) · `ld2-tree-under`(30 m) · `ld2-aerial-tree` | 원기둥 하나가 아니라 판뿌리 · 비틀린 줄기 마디 · 가지 · 잎 덩어리 묶음, 멀리서 버섯 · 우산이 아닌 둥근 덩어리 | ✅ 11-08 — 2회차까지 멀리서 「버섯」 실루엣 → 줄기 7 → 5 마디 · 가지를 낮추고 아래 치마 묶음을 더해 둥근 참나무 덩어리로(`ld2-tree-far` · `ld2-aerial-tree`), 밑에서 판뿌리 · 비틀린 줄기 마디(`ld2-tree-under`) | |
| QA-LVL-016 | — | 하늘 구역 이름 — 구역 안 · 멀리서 읽히고, 끌 수 있고, 출시 빌드엔 없다 | ③ `ld2-labels-door` · `ld2-aerial-region` · `Grim.DevLabels.Sections 0` 뒤 `ld2-labels-off` · 코드 `AGrimDevLabel::AreSectionLabelsEnabled`(`UE_BUILD_SHIPPING` 이면 거짓) | 한글이 네모로 깨지지 않고 구역 위 하늘에 보인다, 끄면 사라진다(POI 글씨는 그대로) | ✅ 11-08 — 구역 8 · 소구역 12 가 한글로 보이고, `Grim.DevLabels.Sections 0` 이면 하늘 이름만 사라진다(POI 글씨는 남음). 출시 빌드는 코드로 확인(`UE_BUILD_SHIPPING` → 위젯을 만들지 않음) | |
| QA-LVL-017 | R16 | 걷기 · 표식 · 시작 · 탑 | ② `check_openbeta.ps1` · `check_tutorial.ps1` | PASS | ✅ 11-08 — `openbeta` PASS · `tutorial` PASS(`run_all.ps1 -Only automation,openbeta,tutorial`), 자동화 181/181(새 `Generator.Ridgeline` · `Foliage.Natural` 포함 — 처음엔 침식 상한 4 m 가 시험 능선의 가파른 비탈에 걸려 실패, 최대 6 m · 평균 0.5 m 로 고쳐 통과) | |
| QA-LVL-018 | R16 | 성능 예산 안 | ① `measure.ps1 -Label level-design-2 -Settle 25`(전체) | QA-PRF-001~008 모두 예산 안 | ✅ 11-08 — GPU 4.10 ~ 4.43 ms · 게임 2.47 ~ 3.09 · 렌더 5.17 ~ 5.53 · p95 ≤ 6.8 · 끊김 0 · 드로 콜 ≤ 1,473 · 2.86 GB · 대역폭 ≤ 5.8 kB/s — [성능 측정 기록](../systems/performance#측정-기록) | |
| QA-LVL-019 | — | 쓰는 팩 파일만 저장소에(늘어난 것만) | `add_pack_assets.ps1 -DryRun` | 1차 202 MB 에서 조금만 늘어남(돌 2 · 바위 2 메시) | ✅ 11-08 — 126 개 218.4 MB(1차 116 개 202.2 MB 에서 StyleHex 돌 2 종 메시 · 재질 · 텍스처 10 개 +16 MB). 잎 재질 인스턴스 3 은 우리 에셋(`Content/Grimoire/Terrain/Dress/`) | |

**결과(11-08)**: 통과 10 · 실패 0(QA-LVL-010 ~ 019). 사람 눈 검토는 세 회차 — 회차마다 본 것 · 고친 것은 [레벨 디자인 「9」 결과](../dev/level-design#결과-2026-11-08-—-세-번-고쳐-찍었다)

### 전 · 후 (같은 자리 · 같은 카메라, 눈높이 1.8 m)

| 자리 | 전(레벨 아트 1차) | 후(레벨 디자인 2차) |
|---|---|---|
| 탑 문 앞 | ![](/qa/level-dress-door-after.jpg) | ![](/qa/ld2-door.jpg) |
| 큰길 100 m | ![](/qa/level-dress-path100-after.jpg) | ![](/qa/ld2-path100.jpg) |
| 큰길 200 m | ![](/qa/level-dress-path200-after.jpg) | ![](/qa/ld2-path200.jpg) |
| 연못 | ![](/qa/level-dress-pond-after.jpg) | ![](/qa/ld2-pond.jpg) |
| 봉화 언덕 꼭대기 | ![](/qa/level-dress-hilltop-after.jpg) | ![](/qa/ld2-hilltop.jpg) |
| 하늘에서 마탑 구간 | ![](/art/terrain/openbeta-sector-aerial-v3.jpg) | ![](/qa/ld2-sector-aerial.jpg) |

### 구간마다 (2차 새 자리)

| 서쪽 등줄기 | 동쪽 안부 너머 풍차 | 남동 숨은 골 |
|---|---|---|
| ![](/qa/ld2-westspine.jpg) | ![](/qa/ld2-eastsaddle.jpg) | ![](/qa/ld2-hiddenvale.jpg) |
| **풍차 마당** | **큰길 400 m — 꽃길 초원** | **부유석 언덕** |
| ![](/qa/ld2-windmill.jpg) | ![](/qa/ld2-path400.jpg) | ![](/qa/ld2-floatsteps.jpg) |
| **거대 고목 — 480 m** | **거대 고목 — 120 m** | **거대 고목 — 30 m 밑** |
| ![](/qa/ld2-tree-far.jpg) | ![](/qa/ld2-tree-near.jpg) | ![](/qa/ld2-tree-under.jpg) |
| **하늘 구역 이름(탑 문 앞)** | **구역 이름 끔(`Grim.DevLabels.Sections 0`)** | **하늘 700 m — 지역 전체** |
| ![](/qa/ld2-labels-door.jpg) | ![](/qa/ld2-labels-off.jpg) | ![](/qa/ld2-aerial-region.jpg) |
| **바다 위 280 m — 콘셉트 전경 구도** | **고목 남쪽 하늘 220 m** | |
| ![](/qa/ld2-aerial-south.jpg) | ![](/qa/ld2-aerial-tree.jpg) | |

## 레벨 디자인 3차 (2026-11-08)

무엇을 왜 [레벨 디자인 「10」](../dev/level-design#_10-레벨-디자인-3차-—-능선-·-개울-·-샘-·-돌담-·-채움-2026-11-08). 마지막 실행 **2026-11-08**(`claude/level-design-3-148999`).

```powershell
# 맵: 위 「다시 돌리기」 그대로(build → gameplay → dress). 스크린숏(눈높이 · 하늘 33 장, 약 18 분)
powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld3 -RunSeconds 1300
# 돌담 뜬 돌 검사(맵에 놓인 인스턴스)
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Terrain\check_walls.py" -unattended -nop4 -nosplash -nullrhi   # → Saved/Terrain/ld3_wall_check.json
```

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-LVL-020 | — | 능선이 이어진 날카로운 등성이로 읽힌다(고분 아님) | ① `Grimoire.Terrain.Generator.SharpRidge` ③ `ld3-hilltop` · `ld3-pond` · `ld3-aerial-south` · `ld3-sector-aerial` | 꼭대기 옆 6 m 가 이미 낮다(둥근 돔 아님), 한쪽이 가파르다, 등성이가 끊기지 않는다. 눈으로: 길게 이어진 등줄기 · 지맥 · 군데군데 바위 | ✅ 11-08 — 테스트 통과. 연못 · 언덕 꼭대기에서 등줄기 실루엣이 하늘에 이어지고, 1 회차 지구라트 띠는 층을 흔들어 없앴다 | |
| QA-LVL-021 | — | 개울이 자연스럽고 연못과 틈 없이 이어진다 | ① `Generator.NaturalCreek`(물 단조 감소 · 끝 = 연못 물 높이 · 합류부에 물 있음 · V 자) ③ `ld3-creek-upper` · `ld3-creek-mid` · `ld3-pond-junction` · `ld3-pond-aerial` · `ld3-creek-aerial` | 굽이 · 폭 변화 · 소 · 돌 · 둑, 상류 V 자 → 하류 넓음. 연못 어귀에 물이 끊기지 않는다 | ✅ 11-08 — 테스트 통과, 한 메시로 이어진 물 표면(`SM_Water_Creek_TowerBrook`). 1 회차 도랑 · 모래 띠 · 흙탕물을 고쳤다 | |
| QA-LVL-022 | — | 돌담 돌이 뜨지 않는다(수치) | ① 쌓을 때 `Saved/Terrain/ld3_wall_report.json` · 맵에 놓인 뒤 `check_walls.py`(바닥 ↔ 랜드스케이프 라인 트레이스 / 아래 돌 안) | 뜬 돌 0 | ✅ 11-08 — 쌓을 때 돌 4,525: 밑돌 모서리 ≤ 7 cm, 윗단은 아래 돌에 2.9 cm 이상 묻힘. 맵 검사 돌 6,421(돌담 + 돌무더기 · 돌집 · 봉화대): 땅 3,022 · 돌 위 3,399 · **뜬 돌 0**. 처음 7 개 → 봉화대 윗단 높이 고침 | |
| QA-LVL-023 | — | 돌담이 불규칙하고 자연스럽다 | ③ `ld3-wall-a` ~ `ld3-wall-e` · `ld3-path100` · `ld3-path200` | 돌 크기 · 방향이 제각각, 끊긴 곳 · 무너진 곳 · 밑동 풀, 땅을 따라 오르내린다. 같은 간격 반복 없음 | ✅ 11-08 — 큰길 담 13 줄 · 비탈 담 8 줄 · 양 우리 6, 무너진 돌 423. 돌탑처럼 솟은 곳을 3 회차에 고쳤다 | |
| QA-LVL-024 | — | 다리가 곧고 수평, 양쪽 둑에 얹혔다 | ① `Generator.NaturalCreek`(다리 하나 · 양끝 노면 = 상판) · 로그 `다리 …: 사이각` ③ `ld3-bridge` · `ld3-bridge-top` · `ld3-bridge-side` · `ld3-fill-road780` | 널이 길 방향에 직각, 물 위 1.6 m, 양쪽이 땅에 닿는다 | ✅ 11-08 — 사이각 89° · 84° · 83°(처음 67° → 물길을 다시 그음). 다리 위 꽃을 없앴다 | |
| QA-LVL-025 | — | 샘이 바위 틈에서 솟는다 | ③ `ld3-spring` · `ld3-spring-near` | 말굽꼴 이끼 바위 · 내민 판 바위 아래 웅덩이, 풀 · 갈대, 흘러나가는 물길, 동심원 | ✅ 11-08 — 구도 확인. 거품 · 튀는 물은 없음(Q135) | |
| QA-LVL-026 | — | 짐수레가 길가에 있고 이야기가 읽힌다 | ③ `ld3-cart` · 레이아웃 `Poi_Meadow_Cart` · `Poi_Pasture_Cart` | 큰길 굽이 길섶, 기운 수레 · 빠진 바퀴 · 쏟아진 짐 | ✅ 11-08 — 길에서 3.4 m 길섶, POI 블록을 겹치지 않음(`bNoGreybox`) | |
| QA-LVL-027 | — | 빈 곳이 채워졌다 · 콘셉트 재사용 | ③ `ld3-fill-road520` · `ld3-fill-road640` · `ld3-path400` · `ld3-hiddenvale` · 표 [콘셉트 목록](../dev/level-design#콘셉트-·-에셋-목록-전수-—-어디에-몇-번) | 작은 콘셉트 여러 변형 · 큰 콘셉트 1 ~ 2 번 더 | ✅ 11-08 — 작은 소품 묶음 약 280 곳, 큰 콘셉트 추가 6 곳 | |
| QA-LVL-028 | — | 설계 검사 · 전체 자동화 · 맵 점검 | ① `Grimoire.*` ② `check_openbeta.ps1` · `check_tutorial.ps1` | 모두 통과 | ✅ 11-08 — 자동화 185/185, `openbeta` · `tutorial` PASS, 전체 경사 70.4 % · 시선 최소 3 · 구간 가림 65.5 % · 랜드마크 최소 1 | |
| QA-LVL-029 | R16 | 성능 예산 안 | ① `measure.ps1 -Label level-design-3 -Settle 25` | QA-PRF-001 ~ 008 예산 안 | ✅ 11-09 — 렌더(`level-design-3b`, 셰이더 캐시가 찬 상태): GPU 4.30 ~ 4.67 ms · 게임 3.10 ~ 3.60 · 렌더 5.53 ~ 6.15 · p95 ≤ 7.6 · 33 ms 넘는 프레임 1 번(70 초) · 드로 콜 533 ~ 1,256 · 메모리 2.61 ~ 2.91 GB. 네트워크(`level-design-3`): 서버 보냄 2.0 ~ 5.5 kB/s · 월드 틱 1.57 ~ 2.12 ms. 재질을 막 바꾼 직후 첫 측정은 셰이더 컴파일로 33 ms 넘는 프레임 5 번(GPU 4.16 ~ 5.44) — [성능 측정 기록](../systems/performance#측정-기록) | |
| QA-LVL-030 | — | 쓰는 팩 파일만 저장소에 | `add_pack_assets.ps1 -DryRun` | 수백 MB 넘지 않음 | ✅ 11-08 — 합계 146 개 236.4 MB(2차 126 개 218.4 MB + 20 개 18 MB — 돌 · 풀 메시 · 재질 · 텍스처). `WaterMaterials`(CC BY, Q126 미결)는 쓰지 않는다 | |

### 전 · 후 (같은 자리 · 같은 카메라)

| 자리 | 2차 | 3차 |
|---|---|---|
| 탑 문 앞 | ![](/qa/ld2-door.jpg) | ![](/qa/ld3-door.jpg) |
| 큰길 100 m | ![](/qa/ld2-path100.jpg) | ![](/qa/ld3-path100.jpg) |
| 큰길 200 m | ![](/qa/ld2-path200.jpg) | ![](/qa/ld3-path200.jpg) |
| 연못 | ![](/qa/ld2-pond.jpg) | ![](/qa/ld3-pond.jpg) |
| 봉화 언덕 꼭대기 | ![](/qa/ld2-hilltop.jpg) | ![](/qa/ld3-hilltop.jpg) |
| 바다 위 280 m | ![](/qa/ld2-aerial-south.jpg) | ![](/qa/ld3-aerial-south.jpg) |
| 하늘에서 마탑 구간 | ![](/qa/ld2-sector-aerial.jpg) | ![](/qa/ld3-sector-aerial.jpg) |

### 고친 곳마다

| 샘 | 상류 | 굽이 |
|---|---|---|
| ![](/qa/ld3-spring-near.jpg) | ![](/qa/ld3-creek-upper.jpg) | ![](/qa/ld3-creek-mid.jpg) |
| **다리(길에서)** | **다리(위에서)** | **연못 어귀** |
| ![](/qa/ld3-bridge.jpg) | ![](/qa/ld3-bridge-top.jpg) | ![](/qa/ld3-pond-junction.jpg) |
| **짐수레** | **돌담** | **돌담 · 비탈 담** |
| ![](/qa/ld3-cart.jpg) | ![](/qa/ld3-wall-c.jpg) | ![](/qa/ld3-wall-b.jpg) |
| **서쪽 등줄기** | **봉화 언덕 쪽** | **동쪽 등줄기** |
| ![](/qa/ld3-ridge-west.jpg) | ![](/qa/ld3-ridge-north.jpg) | ![](/qa/ld3-ridge-east.jpg) |
