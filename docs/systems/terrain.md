# 지형 — 오픈 베타 땅 (L_OpenBeta)

> 계획: [로드맵 — 지형은 언제 찍나](./roadmap#지형은-언제-찍나-2026-11-05) 의 지형 1차, 작업 분할 [M3](./m3-plan) 갈래 W. 지역: [세계 — 지역과 크기](../spec/world#지역과-크기-r34-—-q36-확정) 1번(마법탑 · 온난 지대).
> **2차(다시 만들기, 2026-11-05)**: 1차가 「그냥 평지」였던 것을 [레벨 디자인 노트](../dev/level-design) 「6. 지역 1 다시 만들기 계획」대로 고쳤다 — 구역 높이 · 크기가 섞인 언덕 군 · 한쪽 절벽 계곡 · 폭포 · 고개 · 랜드마크 7 · POI 40 · 자동 설계 검사. 결과 숫자와 계획과 달라진 점은 노트의 「결과」.
> **마탑 구간(2026-11-05, 2차 플레이 테스트 뒤)**: 2 × 4 km 를 한 번에 다듬지 않고 탑 둘레(약 1 km²)를 먼저 완성 수준으로 — 작은 언덕이 촘촘한 구릉 · 개울 · 연못 · POI 21 · 떠 있는 한글 글씨 · 「공사 중」 경계. 아래 「[마탑 구간](#마탑-구간-먼저-완성하는-구간)」, 결과 숫자는 노트의 「7」.
> **레벨 아트 1차(2026-11-08)**: 시작 구역(마탑 구간)에 Fab 팩(StyleHex 풀 · 나무 · 바위, 지형 재질, 손그림 구름 등)을 입혔다 — `dress_openbeta.py`, 무엇을 어디에는 [레벨 디자인 노트 「8」](../dev/level-design#_8-레벨-아트-1차-—-fab-팩-시험-적용-2026-11-08).
> **레벨 디자인 2차(2026-11-08)**: 「규칙적인 느낌」을 고쳤다 — 마탑 구간에 **능선 · 안부**(손으로 그은 등성이 7 줄) + 뒤튼 능선형 노이즈 + 물방울 침식, 식생을 **포아송 원판 · 숲 무늬 · 무리 · 등성이 / 골 · 물가**로 자연스럽게(맵 전체 StyleHex), 구간마다 콘셉트 구도(돌담 · 돌 표지 · 밭 · 봉화대 …), **하늘 구역 이름**, 거대 고목 **큐브 목업**. 무엇을 왜는 [레벨 디자인 노트 「9」](../dev/level-design#_9-레벨-디자인-2차-—-능선-·-자연-분포-·-구간-구도-2026-11-08).
> **레벨 디자인 3차(2026-11-08)**: 능선을 꼭대기까지 기울어 있는 **날카로운 단면**(`bSharpCrest`, 급사면 + 완사면 · 바위 띠 · 지맥)으로, 개울을 **자연 개울**(`bNatural` — 지형이 판 V 자 골 · 소 · 여울 · 물 표면 메시 · 연못과 틈 없는 합류)로, 다리는 곡선 교점에서 계산, 돌담은 돌마다 땅에 앉혀 실제로 쌓는다(`ld3_dress.py`). 무엇을 왜: [레벨 디자인 노트 「10」](../dev/level-design#_10-레벨-디자인-3차-—-능선-·-개울-·-샘-·-돌담-·-채움-2026-11-08).
> 상태: 지형 2차 + 마탑 구간 구현됨(2026-11-06 코드 대조). 코드: `Source/Grimoire/World/`. 스크립트: `Scripts/Terrain/`. 에셋: `Content/Grimoire/Terrain/`. 맵: `Content/Grimoire/Maps/L_OpenBeta.umap`(World Partition, 게임 맵 · 에디터 시작 맵).

![L_OpenBeta 를 하늘 2.9 km 에서 — 북쪽이 위. 남쪽 탑 곶 · 목초지 · 서쪽 연못 저지대 · 가운데 구릉과 고목 · 계곡(한쪽 절벽) · 북부 고지 · 동쪽 큰 강](/art/terrain/openbeta-aerial.jpg)

## 한눈에

| 조각 | 파일 | 하는 일 |
|---|---|---|
| 레이아웃(수치) | `Scripts/Terrain/openbeta_layout.json` | 구조체 `FGrimTerrainLayout`(`Source/Grimoire/World/GrimTerrainTypes.h`)의 JSON — 시드 · 크기 · 구역 · 언덕 · 바위 능선 · 고개 · 계곡 · 강 · 개울 · 연못 · 길 · 거점 평지 · 표식 · **랜드마크** · **POI** · **설계 검사 기준** · 식생. **지형을 바꾸려면 여기를 고치고 다시 만든다** |
| 높이 지도 생성기 | `FGrimTerrainGenerator`(`Source/Grimoire/World/GrimTerrainGenerator.h`) | 순수 계산(월드 · 에셋 모름). 높이(m) · 물가 · 길 · 계곡 숲 무게 · 물과 길까지 거리 · 물 목록(폭포 포함) · 놓인 언덕 · 길 선 · 식생 자리. 830만 정점을 몇 초 |
| 설계 검사 | `FGrimTerrainAnalysis`(`Source/Grimoire/World/GrimTerrainAnalysis.h`) | 순수 계산. 경사 분포 · 구역 평균 높이 차 · 길 위 시선(높이 지도 시야) · POI 밀도 · 길 경사 — 아래 「설계 검사」 |
| 에디터 도구 | `UGrimTerrainEditorLibrary`(`Source/Grimoire/World/GrimTerrainEditorLibrary.h`) | 파이썬이 부르는 C++: 레이아웃 읽기 · 생성 캐시 · 땅 높이 · **검사 보고** · **시선 조회** · 랜드스케이프 가져오기 · 레이어 정보 · 폴리지 타입 · 식생 변환 · 브러시 볼륨 · World Partition 격자 · 미리보기 그림 · r16 내보내기 |
| 표식 | `AGrimTerrainMarker`(`Source/Grimoire/World/GrimTerrainMarker.h`) · 태그 `GrimTerrainTags`(`Source/Grimoire/World/GrimTerrainTags.h`) | 자리만 표시하는 액터 — GameplayTag `MarkerTag` + 반지름. `FindMarkers(월드, 태그)` 로 찾는다 |
| 개발 표시 글씨 | `AGrimDevLabel`(`Source/Grimoire/World/GrimDevLabel.h`) | 떠 있는 한글 글씨(제목 + 설명) — 월드 공간 위젯 컴포넌트 + Slate 글씨, 카메라 쪽으로 돈다. 출시 빌드 · `Grim.DevLabels 0` 이면 숨김. 아래 「마탑 구간」 |
| 맵 만들기 | `Scripts/Terrain/build_openbeta.py`(+ `terrain_assets.py`) | 맵을 지우고 처음부터: 설계 검사 숫자 로그 → 랜드스케이프 → 물 → 식생 → 표식 → **랜드마크 · POI 그레이박스 · 다리** → 마법탑 → 하늘 · 기후 · 지도 끝 → World Partition → 저장 |
| 랜드마크 · POI 모양 | `Scripts/Terrain/landmark_greybox.py` | 엔진 기본 도형으로 종류마다 실루엣이 다른 그레이박스(아래 「랜드마크」「POI」) + 개발 표시 글씨 · 종류 색 블록 · 「공사 중」 경계 |
| 마법탑 | `Scripts/Tutorial/tower_greybox.py` | L_Tower 와 **같은 함수**로 탑 그레이박스를 짓는다 |
| 레벨 아트 | `Scripts/Terrain/dress_openbeta.py`(+ `dress_assets.py`) | 만든 맵 위에 덧입힌다: 지형 재질 `M_Terrain_Dress`(맵 전체) · 랜드스케이프 풀 `GT_Dress_Meadow` · **맵 전체** 식생을 StyleHex 메시로(종은 생성기 Variant, 잎 색 `MI_Dress_Leaves_*`) · 길가 돌 · 손 소품 · 구름 카드. 지형을 다시 만들면 **이것도 다시 돌린다**. [레벨 디자인 「8」](../dev/level-design#_8-레벨-아트-1차-—-fab-팩-시험-적용-2026-11-08) |
| 구간 구도 · 하늘 이름(2차) | `Scripts/Terrain/section_dress.py` | `dress_openbeta.py` 가 부른다 — 콘셉트 아트대로 큰길 돌담 · 돌 표지 · 등불, 능선 꼭대기 바위 · 외딴 나무, 풍차 밭 · 곳간, 봉화대, 연못 징검돌 · 섬, 부유석 디딤돌, 고목 둥치, **하늘 구역 이름**(레이아웃 `Sections`). [레벨 디자인 「9」](../dev/level-design#구간-구도-—-콘셉트-아트대로-scripts-terrain-section-dress-py) |
| 미리 보기(2차) | `Scripts/Terrain/preview_openbeta.py` · `Scripts/Terrain/overlay_foliage.py` | 맵을 만들지 않고 레이아웃만: 설계 검사 · 등고선 지도(마탑 구간 · 세부 · 고목 · 전체) · 식생 자리 CSV → 지도 위 점. `Saved/Terrain/preview/`. 레이아웃을 고치며 1 분 안에 본다 |
| 3차 치장 | `Scripts/Terrain/ld3_dress.py` · 검사 `check_walls.py` | `dress_openbeta.py` 가 부른다 — 돌담 · 개울가 · 샘 · 짐수레 · 등성이 바위 · 빈 곳 채우기(폴리지 인스턴스). 검사는 맵에 놓인 돌담 돌이 땅이나 다른 돌에 닿았는지 라인 트레이스로 잰다 |
| 게임플레이 배치 | `Scripts/Integration/place_openbeta_gameplay.py` | 서식 구역 볼륨 3(`Habitat.*` 표식 자리). 지형을 다시 만들면 **이것도 다시 돌린다**. 냄비는 탑 그레이박스가 이미 놓으므로 있으면 건너뛴다 |
| 확인 | `Scripts/Terrain/check_openbeta.ps1` · `Scripts/Terrain/shoot_openbeta.ps1` · 콘솔 `Grim.Terrain.*`(`Source/Grimoire/World/GrimTerrainDebug.cpp`) · 테스트 `Grimoire.Terrain.*`(`Source/Grimoire/World/Tests/GrimTerrainTests.cpp`) | 아래 「확인」 |
| 이동 치트 | `Source/Grimoire/World/GrimTeleportDebug.cpp` | 콘솔 `Grim.Go <이름>` · `Grim.Go.List [거르기]` — 표식 · 서식 구역 · 생물 · 태그 액터 곁으로([치트](../dev/cheats)) |

## 만드는 법 · 다시 만드는 법

```powershell
# 1) C++ 빌드(에디터를 닫고) — CLAUDE.md 의 Build.bat
# 2) 맵 만들기(헤드리스, 창 없음). -nullrhi 가 아니라 -AllowCommandletRendering — 아래 「RHI 가 필요한 까닭」
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Terrain\build_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 3) 게임플레이 액터(서식 구역) 다시 놓기
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Integration\place_openbeta_gameplay.py" -unattended -nop4 -nosplash -nullrhi
# 3b) 레벨 아트(시작 구역) 다시 입히기 — RHI 필요, 팩 정션 필요(docs/dev/market-assets.md). 처음이면 이것 → 2) → 3) → 이것 한 번 더
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Terrain\dress_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 4) 확인
powershell -File Scripts\Terrain\check_openbeta.ps1          # 끝에 PASS
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -ExecCmds="Automation RunTests Grimoire.Terrain; Quit" -unattended -nop4 -nosplash -nullrhi -log
powershell -File Scripts\Terrain\shoot_openbeta.ps1          # 스크린숏 → docs/public/art/terrain/*.jpg
```

- 첫 실행은 셰이더 때문에 몇 분, 그 뒤는 2~3분. 끝에 로그 `build_openbeta: 개수 …` · `saved /Game/Grimoire/Maps/L_OpenBeta`. 시작할 때 설계 검사 숫자도 `LogGrimTerrain: 지형 검사:` 로 남긴다.
- 가끔 `new_level 실패 … An asset already exists` 로 멈춘다(지운 맵이 메모리에 남음). 파일은 이미 지워졌으니 **한 번 더 돌리면** 된다.
- **레이아웃만 고칠 때는 맵을 만들지 않고** 테스트 `Grimoire.Terrain.Design` 만 돌려 숫자를 본다(1~2분). 테스트가 JSON 을 바로 읽는다.
- **같은 레이아웃 · 같은 시드면 같은 땅**: 높이 · 무게 · 물 높이 · 언덕 자리 · 식생 자리가 모두 시드로 정해진다(테스트 `Grimoire.Terrain.Generator.Deterministic`). 액터 GUID 는 매번 새로 나서 `__ExternalActors__` 파일 이름이 바뀐다 — 다시 만들면 git 에는 파일 수백 개가 지워지고 새로 생긴다.
- 맵이 에디터 시작 맵이라 커맨드렛이 이미 열고 있을 수 있다 — 스크립트는 빈 맵으로 바꾸고 치운 뒤 지운다(안 그러면 `new_level` 이 「이미 있다」로 실패).
- 환경 변수 `GRIM_TERRAIN_REFRESH_ASSETS=1` 이면 `Content/Grimoire/Terrain` 의 머티리얼 · 폴리지 타입 · 레이어 정보 · 풀 타입도 지우고 새로 만든다(기본은 있으면 그대로).
- 부산물: `docs/public/art/terrain/openbeta-preview.png`(위에서 본 지도 그림, 북쪽이 위, 랜드마크 = 빨간 큰 점 · POI = 묶음마다 같은 색 작은 점), `openbeta-sector-map-v3.png`(마탑 구간 확대 — 등고선 2 m · 구간 경계 빨간 선 · 언덕 가운데 검은 점, `ExportRegionPreviewImage`), `Saved/Terrain/openbeta.r16`(16비트 높이 지도, 랜드스케이프 순서 4065 × 2033). 2차 지도는 `openbeta-preview-v2.png` 로 남겼다.

### RHI 가 필요한 까닭

`-nullrhi` 로 만들면 땅이 **보이지 않는다**(충돌은 맞아서 걷기 · 헤엄은 된다). 랜드스케이프는 편집 레이어 시스템이 GPU 로 레이어를 합쳐 최종 높이 지도 텍스처를 만드는데, RHI 가 없으면 그 텍스처가 0(= −256 m)으로 저장된다. `-AllowCommandletRendering` 이 헤드리스 커맨드렛에 RHI(D3D12)를 켜서 이를 막는다(창은 뜨지 않는다). 그래서 `build_openbeta.py` 는 `-nullrhi` 면 바로 멈추고, `CreateLandscape` 도 렌더링이 꺼져 있으면 만들지 않는다. 확인 스크립트는 「그림 높이」가 −50 m 위에서 시작해 +50 m 넘게 올라가는지 본다.

## 지도 · 좌표

![위에서 본 높이 지도(생성기 미리보기, 북쪽이 위) — 빨간 큰 점 = 랜드마크, 작은 점 = POI(묶음마다 같은 색)](/art/terrain/openbeta-preview.png)

- **지도 좌표**(레이아웃 JSON): 미터, 남서 모서리가 (0, 0), X = 동쪽, Y = 북쪽. 크기 2032 × 4064 m.
- **월드 좌표**(cm): 원점 = 마법탑 가운데(지도 (1000, 380)), **월드 +X = 북쪽, +Y = 동쪽**. 바꾸는 식: 월드 X = (지도 Y − 380) × 100, 월드 Y = (지도 X − 1000) × 100.
- 랜드스케이프도 월드 축을 따른다 — 로컬 X(4 km) = 북쪽, Y(2 km) = 동쪽. 생성기는 지도 순서로 계산하고 가져올 때 바꿔 놓는다.

## 높이를 만드는 순서(생성기)

| 단계 | 하는 일 | 레이아웃 |
|---|---|---|
| 0. 언덕 놓기 | 손 언덕 7 + 언덕 군(다각형 안에 시드로 Count 개, 크기 = 난수^SizeBias → 작은 것이 많다). 길 · 개울 · 강 · 계곡 가운데선 · 탑 · 연못 · 거점을 피하고 서로 (반지름 합 × MinSpacing) 떨어진다. 가장 가파른 곳(타원의 짧은 쪽)이 `HillMaxSlopeDeg` 를 넘으면 높이를 줄인다. 바위 노두(작은 모양)는 시드 언덕 꼭대기 둘레 · 고지에. 언덕 군마다 바꿀 수 있는 것: 가파름 상한 `MaxSlopeDeg` · 길에서 떨어지는 여유 `LineClearance` · 손 언덕과 떨어지는 배수 `HandHillSpacing`. 작은 모양에 음수 높이를 주면 오목한 골이 된다 | `Hills` · `HillFields` · `HillSharpness` · `HillMaxSlopeDeg` |
| 0b. 능선(2차) | 꺾은선을 따라 둥근 등성이 — 점마다 높이(낮은 점 = **안부**, 사이는 부드럽게), 단면은 언덕 실루엣, 진행 방향 왼쪽 · 오른쪽 밑동 폭이 달라 한쪽이 가파르다, 높이 · 폭이 길이를 따라 ± `Variation`. 언덕과 같은 버퍼(가장 큰 것 + 나머지 × 0.3) | `Ridgelines` |
| 0c. 날카로운 등성이(3차) | `bSharpCrest` 능선 — 단면 (1 − t)^p, 양쪽 폭 · 지수가 다르고(급사면 · 완사면), 꼭대기는 `CrestRound` 만 둥글린다. 등성이 선 흔들림(`CrestWobble`) · 톱니(`CrestJag`) · 비탈 골(`Gully*`) · 급사면 바위 띠(`ScarpBand*`, 군데군데). 겹치면 큰 값만(`RidgeMax`) | `Ridgelines` |
| 1. 바탕 | 구역 바닥(다각형, 경계 섞기 + 들쭉날쭉) + 구역 잔물결(`ExtraRolling*`, 덮는 순서와 상관없이 가장 센 구역 것) + **구역 뒤튼 능선 노이즈**(2차, `Ridged*` · `WarpStrength` — 능선형 다중 프랙탈을 저주파로 뒤틀어 결 있는 등성이 · 골) + 큰 노이즈 · 물결 · 둔턱(구역 `BumpMultiplier` 를 곱함) + 언덕(가장 큰 것 + 겹친 나머지 × 0.3) + 바위 노두 + 바위 능선 + 가장자리 산등성이(북쪽은 고개에서 낮아짐) | `Zones` · `Noise*` · `Rolling*` · `Bump*` · `Ridges` · `*RidgeWidth/Height` · `PassGaps` |
| 1b. 침식 골 | 경사 7° 넘는 비탈에, 내리막 방향으로 길게 뻗은 좁은 골(깊이 0.7 m 안팎) | `Gully*` |
| 1d. 물방울 침식(2차) | 사각형(`ErosionBounds*`) 안 칸마다 물방울을 떨어뜨려 비탈을 따라 흘리며 깎고 느려지면 쌓는다(Lague 방식, 한 스레드 고정 시드). 비탈에 가지 친 물길 · 골 바닥 · 언덕 밑동에 흙. 계곡 · 길 · 연못 · 탑 · 거점은 이 뒤에 깎고 고른다. 마탑 구간 130만 칸에 약 1 초 | `Erosion*` |
| 1c · 2. 계곡 | 바닥 높이 = 양쪽 둑 중 **낮은 쪽** − 깊이, 이동 평균, 하류로 내려가기만. 한쪽(고지 쪽)은 거의 곧은 절벽, 다른 쪽은 부드러운 S 자. 바닥 · 비탈 폭이 계곡을 따라 들쭉날쭉 | `Valley` |
| 2. 탑 · 해안 · 강 | 탑 언덕(꼭대기 평지 45 m), 남쪽 해안 · 곶 절벽, 큰 강(물 = 해수면) | `TowerHill` · `Coast*` · `River` |
| 2b. 자연 개울 골(3차) | `bNatural` 개울의 물 높이(오목 곡선, `ValleyDepth*` 만큼 파임, 끝 = `JoinPond` 연못 물 높이) · 폭 · 소 · 곡률을 정하고 옆면 경사 `ValleySlope*` 의 골을 판다 — 굽이 바깥은 좁고 가파르게 | `Creeks` |
| 3. 길 | 길 선을 2 m 마다 땅 높이로 재고, 경사 상한(`PathGradeDeg` 10°)을 지키는 가장 높은 「깎기」선과 가장 낮은 「메우기」선의 가운데를 고른 뒤(`PathSmoothing` 40 m 이동 평균) 그 높이로 깎고 메운다. 갈림길은 큰길 노면을 덮지 않고 큰길 높이에서 시작한다 | `Paths` · `PathGradeDeg` · `PathSmoothing` |
| 4. 개울 · 폭포 | 물 높이는 상류 → 하류로 내려가기만. 4 m 마다 1.2 m 넘게 떨어지는 가장 큰 구간(8 m 이상)을 「폭포」로 물 목록에 낸다 | `Creeks` |
| 5. 연못 · 거점 평지 | 둘레 땅에서 물 높이, 대접 모양(`Irregularity` 면 물가선이 흔들림) / 반지름 안 평평 | `Ponds` · `BaseSites` |
| 5b. 자연 개울 물길(3차) | 길 · 연못 · 거점 뒤에 판다 — 다리 밑으로 흐르고 연못 둑을 뚫는다. 땅이 물보다 낮은 곳엔 둑을 쌓는다 | `Creeks` |

언덕 실루엣 = 높이 × (1 − t)^k (1 + k t), t = 가운데 거리 / 반지름, k = 2.4 — 꼭대기가 둥글고 가운데가 곧고 밑동이 퍼지는 「둥근 삼각형」. 가장 가파른 기울기 = 높이 / 반지름 × 1.6.

## 구역 · 언덕 · 물 (지금 레이아웃)

| 구역(`Zones`, 뒤가 앞을 덮음) | 다각형(대략) | 바닥 높이 | 검사 평균 높이 |
|---|---|---|---|
| `TowerSector` 마탑 구간 | 아래 「마탑 구간」 다각형 | 27 m(목초지와 같음) + 잔물결 ±2 m(파장 55 m) | — |
| `Pasture` 남부 목초지 | 지도 Y 380~1330 | 27 m | 31.8 m |
| `CentralHills` 중부 구릉 | Y 1300~2600(서쪽은 3300 까지) | 60 m | 60.1 m |
| `PondLowland` 연못 저지대 | 서쪽 X 230~860, Y 1000~2300 | 17 m | — |
| `Riverside` 강변 | 동쪽 X 1570~, Y 450~2450 | 16 m(강 둑 16 m) | — |
| `Highland` 북부 고지 | 계곡 북쪽 ~ 북쪽 끝 | 88 m → 북쪽 끝 138 m | 121.5 m |
| 탑 곶 | (1000, 380), 반지름 170 m(2차 260 m) | 꼭대기 평지 45 m | — |

| 언덕(`Hills`, 손) | 자리 | 높이 · 지름 | 위에 있는 것 |
|---|---|---|---|
| `Hill_GiantTree` | (1100, 1980) | 55 m · 500 m(타원 1.15) | 거대 고목 — 중부 구릉에서 가장 큰 언덕 |
| `Hill_Windmill` | (1400, 870) | 15 m · 210 m(3차 — 등성이 어깨) | 풍차 |
| `Hill_Lookout` | (1380, 1580) | 36 m · 330 m | 전망 언덕(POI 전망대) |
| `Hill_StoneCircle` | (560, 3700) | 28 m · 340 m | 선돌 고리 |
| `Hill_Watchtower` | (1620, 2120) | 31 m · 240 m | 폐허 감시탑(동쪽은 강 둑 절벽) |
| `Hill_FloatingRock` | (560, 800) | 15 m · 190 m(3차 — 등성이 어깨, 꼭대기는 `Ridge_WestSpine` 58 m) | 부유석(마탑 구간 서쪽) |
| `Hill_Beacon` | (880, 960) | 15 m · 190 m(3차 — 등성이 어깨, 꼭대기는 `Ridge_WestSpine` 64 m) | 봉화 언덕 꼭대기(POI 전망대, 마탑 구간 북쪽) |

- 언덕 군(`HillFields`): 마탑 구간 `TowerSector_*`(아래) 다음에 목초지(구간 북쪽 띠만) 30 · 구릉 50 · 저지대 40 · 고지 65 개를 시도해 **언덕 295 개**(손 7 포함), 작은 모양 180 개(바위 노두 85 + 구간 둔덕 · 골 · 바위). 자리가 모자라면 덜 놓인다(로그 `언덕 군 …: n / m 개만`).
- 바위 능선 3(고지 동쪽 · 고개 양옆), 고개 `Gap_NorthPass`(지도 X 1000, 바닥 폭 80 m) — 북쪽 산등성이(60 m)가 그 둘레에서 사라져 협곡이 된다.
- 계곡: (420, 3420) → (1770, 2610), 바닥 폭 75 m(±30%), 깊이 35 m(낮은 둑 기준), 완만한 쪽 110 m, 절벽 쪽 28 m(±60%). 바닥 높이 약 15~30 m.
- 연못 5: `Pond_TowerWest`(700, 560) · `Pond_Lowland`(600, 1400, 켈피) · `Pond_Reed`(480, 1830) · `Pond_Marsh`(690, 2150) · `Pond_Upper`(1330, 3100, 고지 — 폭포 물머리).
- 개울 4: `Creek_Valley`(계곡 바닥 → 큰 강) · `Creek_Falls`(`Pond_Upper` → 계곡, **폭포 약 76 m**) · `Creek_Pasture`(개울 머리 샘 (1010, 1072) → 큰 강 — 마탑 구간 북쪽 경계, 2차보다 남쪽으로 옮김) · `Creek_TowerBrook`(봉화 언덕 기슭 샘 → `Pond_TowerWest`, 마탑 구간 안).
- 큰 강 · 가장자리 산등성이 · 해안은 1차와 같다(강 둑 폭만 70 → 50 m). 지도 끝은 높이 1 km 막는 볼륨(`MapEdge_*`).

## 길

| 길 | 경로 | 폭 · 둑 |
|---|---|---|
| `Path_Main` 큰길 | 탑 문 → 마탑 구간 언덕 사이로 휘어(S 자 3 번) → 개울 다리 → 목초지 → 고목 언덕 서쪽 기슭 → 계곡 남쪽 비탈을 내려가 **다리**(개울) → 북쪽 절벽 벽을 비스듬히 오르는 **절벽 길**(약 680 m, 75 m 오름) → 고지 → **바위 문** 바로 너머(3925) | 4 m · 14 m |
| `Path_East` | 큰길(1110, 900) → 풍차 언덕 남쪽 → 강변 → 감시탑 언덕 기슭 | 3 m · 11 m |
| `Path_West` | 큰길(1060, 1300) → 연못 저지대 → 늪 연못 | 3 m · 11 m |
| `Path_Highland` | 절벽 길 꼭대기(720, 3440) → 고지 거점 터 → 폭포 위 전망 | 3 m · 11 m |
| `Path_StoneCircle` | 큰길(800, 3570) → 선돌 언덕 | 3 m · 11 m |
| `Path_PondLoop` | 큰길(1015, 625) → 연못 북쪽(개울 다리) → 부유석 언덕 기슭(600, 735) — 마탑 구간 | 3 m · 11 m |

- 길은 언덕을 피해 휘도록 레이아웃에서 그렸고, 시드 언덕은 길에서 (반지름 × 0.6 + 폭) 떨어진다. 남는 굴곡은 깎고 메워서 길 경사 ≤ 10°(검사 상한 15°). 다 만든 땅에서 잰 가장 가파른 곳 **14.6°**(탑 문 앞 — 탑 곶을 170 m 로 줄여 2차 12.9° 보다 가파르다).
- 길이 개울을 건너는 곳(꺾은선 교차 4곳)에 널다리 그레이박스(`Bridges/`)를 놓는다.

## 랜드마크 (`Landmarks`, 표식 `Landmark.<Kind>`, 늘 로드)

| 층 | 랜드마크 | 지도 자리 | 그레이박스 실루엣(`landmark_greybox.py`) | 꼭대기(땅 위) |
|---|---|---|---|---|
| 전역 | `Landmark_Tower` 마법탑 | (1000, 380) | 가는 수직(`tower_greybox`) + 꼭대기에서 하늘로 솟는 **마나 빛줄기**(파란 원기둥, 마탑 구간 골 안에서도 방향을 잡게) | 탑 38 m · 빛줄기 90 m |
| 전역 | `Landmark_GiantTree` 거대 고목 | (1100, 1980) | **큐브 목업**(2차, 약 230 개): 판뿌리 9 갈래 · 비틀린 줄기 5 마디 + 세로 골 · 큰 가지 6(마디 3 + 잔가지) · 잎 덩어리 묶음 22(아래 치마 포함) + 가운데 덩이 3 — [레벨 디자인 「9」 고목 큐브 목업](../dev/level-design#고목-큐브-목업) | 60 m(잎 지름 약 75 m) |
| 전역 | `Landmark_RockGate` 북쪽 고개 바위 문 | (1000, 3890) | 안으로 기운 바위 기둥 둘(13 × 14 × 40 m) 사이 62 m 틈 + 꼭대기 덩어리 · 바위 | 40 m |
| 구역 | `Landmark_StoneCircle` 선돌 고리 | (560, 3700) | 반지름 14 m 선돌 12(하나 쓰러짐) + 가운데 높은 선돌 · 제단 | 9 m |
| 구역 | `Landmark_Waterfall` 폭포 | (1340, 2880) | 생성기가 찾은 폭포 위 → 아래 끝을 잇는 비스듬한 흰 띠 + 아래 물보라 + 물 볼륨 띠(`WaterVolume_Landmark_Waterfall`, `Water.Falls`) | 위 끝 |
| 구역 | `Landmark_Watchtower` 폐허 감시탑 | (1620, 2120) | 부러진 원통(지름 9 m) — 반쪽만 남은 성가퀴 · 높게 남은 벽 조각 · 떨어진 원통 덩어리 · 돌무더기 | 18 m |
| 구역 | `Landmark_Windmill` 풍차 | (1400, 870) | 돌 몸통 + 원뿔 지붕 + 서쪽(큰길)을 보는 + 자 날개 4 | 21 m |
| 구역 | `Landmark_FloatingRock` 부유석(마탑 구간) | (560, 800) | 언덕 위 공중에 뜬 바위섬(지름 18 m, 땅 위 38~51 m) + 아래로 뾰족한 밑동 + 떠 있는 조각 + 땅까지 마나 빛줄기 | 52 m |

- 실루엣이 서로 겹치지 않게(가는 수직 · 둥근 덩어리 · 두 기둥 틈 · 고리 · 흰 띠 · 부러진 원통 · 날개 · 뜬 섬).
- 랜드마크마다 위에 개발 표시 글씨(「전역 · 구역 랜드마크 — 이름」 + 무엇이 들어갈지). 레이아웃 JSON 의 `Label` · `Note`(C++ 구조체에는 없고 스크립트만 읽는다), 글씨 높이는 `LabelHeight`(없으면 `Height` + 7 m). 줄기 · 바위 · 벽은 막힘(`BlockAll`), 잎 · 날개 · 물보라는 통과하지 않는 것도 있다(고목 잎은 막힘 — 올라설 수 있다).
- 풍차 날개는 **아직 돌지 않는다**(선택 항목 — 회전 컴포넌트는 정식 에셋 때).

## POI (`Pois`, 표식 `Poi.<Kind>`, 늘 로드)

53 곳, 묶음 16 — 큰 땅 32 곳(C3~C10, 묶음마다 4) + 마탑 구간 21 곳(S1~S8, 묶음마다 2~4, 아래 「마탑 구간」). 2차의 C1 · C2(탑 앞 · 풍차)는 마탑 구간 묶음으로 바뀌었다. 자리와 실루엣만 — 놀이 내용은 각 마일스톤이 같은 표식 자리에 채운다. 레이아웃 JSON 의 `Note` 가 무엇이 놓일 자리인지 적는다.

| 종류(Kind) | 수 | 그레이박스 소품 |
|---|---|---|
| `Puzzle` 속성 퍼즐 | 11 | 무너진 벽 ㄱ 자 + 받침 위 **꺼진 모닥불**(`GrimCampfire`, 켜지 않은 채) + **마른 풀**(`GrimDryGrass`) 3 |
| `Resource` 자원 | 12 | 바위 3 + 작은 풀 덩이 7 |
| `Creature` 생물 자리 | 8 | 흙 둔덕 + 굴 입구 + 뼈 + 바위 |
| `Ruin` 폐허 · 보물 | 7 | 높이가 다른 벽 3 + 쓰러진 기둥 + 돌 상자 |
| `Viewpoint` 전망대 | 6 | 넓적 바위 + 깃대(빨간 깃발) |
| `Story` 이야기 흔적 | 5 | 꺼진 모닥불 + 돌 고리 + 상자 · 깔개 |
| `BaseSite` 거점 터 | 3 | 네 귀퉁이 말뚝 + 깃대(평지는 `BaseSites` 가 고른다) |
| `Dungeon` 던전 입구 | 1 | 돌 기둥 둘 + 상인방 + 검은 입구 + 바위 |

| 묶음 | 자리 | POI |
|---|---|---|
| S1~S8 마탑 구간 | 탑 둘레 약 1 km² | 아래 「마탑 구간」 표 |
| C3 목초지 북쪽 | 큰길 · 서쪽 갈림목 | 마른 덩굴 돌문 · 샘 · 부서진 짐수레 · 쓰러진 경계석 |
| C4 연못 저지대 | 연못 셋 둘레 | 켈피 연못가 · 갈대 군락 · 잠긴 제단 · 거점 터 |
| C5 고목 | 고목 언덕 | 고목 뿌리 전망 · 마나초 군락 · 돌 상자 · 바람 룬 돌 |
| C6 동쪽 목초지 | 강변 길 남쪽 | 개울 여울 · 광석 노두 · 전망 언덕 꼭대기 · 물새 둥지 |
| C7 감시탑 | 강변 길 끝 | 잠긴 수문 · 감시탑 아래 초소 · 버려진 마법사 짐 · 감시탑 언덕 전망 |
| C8 계곡 | 계곡 들머리 · 남쪽 비탈 · 절벽 길 | 그늘 버섯 · 검은 개 굴 · 절벽 길 동굴(던전) · 절벽 길 전망 바위 |
| C9 고지 | 폭포 위 · 고지 | 거점 터 · 폭포 위 전망 · 광석 노두 · 서리꽃 |
| C10 북쪽 고개 | 선돌 · 고개 앞 | 무너진 초소 · 선돌 룬 · 바위 둥지 · 고개 앞 돌무더기 |

## 표식 (AGrimTerrainMarker, 늘 로드)

액터 태그에도 같은 이름과 표식 이름이 들어 있다. Z 는 땅 + 50 cm.

| 표식 | 태그 | 지도 자리 | 반지름 |
|---|---|---|---|
| `Site_Tower` · `Site_TowerYard` | `Terrain.Site.Tower` · `.TowerYard` | (1000, 380) · (1000, 408) | 45 · 20 m |
| `Habitat_Jackalope` | `Habitat.Jackalope`(목초지 북쪽) | (1200, 1000) | 200 m |
| `Habitat_Kelpie` | `Habitat.Kelpie`(연못 저지대) | (580, 1600) | 220 m |
| `Habitat_BlackDog` | `Habitat.BlackDog`(계곡 어두운 숲) | (1000, 3000) | 220 m |
| `Base_TowerPond` · `Base_Lowland` · `Base_Riverside` · `Base_Highland` | `Terrain.Site.Base` | (820, 640) · (420, 1600) · (1470, 1300) · (1130, 3080) | 28 m |
| `Path_<길>_<nn>` | `Terrain.Path` | 길 꺾은점마다 | 길 반폭 |
| `Landmark_*` | `Landmark.<Kind>` | 위 표 | 10~45 m |
| `Poi_*` | `Poi.<Kind>` | 레이아웃 `Pois` | POI 반지름 |

- 서식 구역 볼륨(`AGrimHabitatVolume`)은 `place_openbeta_gameplay.py` 가 `Habitat.*` 표식 자리에 놓는다.
- 기후 볼륨 `Climate_Meadow` 하나가 지도 전체(+100 m)를 덮는다.

## 설계 검사 (`FGrimTerrainAnalysis`, 테스트 `Grimoire.Terrain.Design.*`)

[레벨 디자인 노트](../dev/level-design) 「6. 다 만든 뒤 확인할 것」 1~4 를 숫자로 잰다. 기준값은 레이아웃 `Analysis`.

| 검사 | 재는 법 | 기준 | 지금 |
|---|---|---|---|
| 1. 경사 분포 | 분석 사각형(지도 (350, 300) ~ (1650, 3700) — 가장자리 산등성이 · 강 건너 · 북쪽 산등성이 뺌)의 물 밖 땅, 4 m 격자, 1 m 중앙 차분 경사 | 5~25° ≥ 70%, 0~3° ≤ 30% | **72.1%**, 8.3%(3~5° 10.1%, 25° 넘음 9.6%) — 2차 |
| 2. 구역 높이 차 | 구역 다각형 안(뒤 구역이 덮은 곳 빼고)의 물 밖 땅 평균, 남 → 북 이웃 차 | ≥ 25 m | 목초지 31.9 → 구릉 60.1(**28.2**) → 고지 121.5(**61.4**) |
| 3. 시선 | 모든 길을 50 m 마다(170 곳), 길 높이(다리 위면 노면) + 눈높이 1.7 m 에서, 랜드마크 꼭대기(거리 한도 1.5~4.5 km) · POI 땅 + 3 m(450 m 안)까지 높이 지도가 시선을 가리는가. 폭포는 띠의 위 · 가운데 · 아래 중 하나 | 곳마다 3 개 이상 | 최소 **3** · 평균 6.9, 모자란 곳 0 |
| 4. 밀도 | 길 위 10 m 마다 가장 가까운 POI 거리 | ≤ 300 m | 가장 먼 곳 **244 m**(서쪽 갈림길) |
| 길 경사 | 다 만든 땅에서 길 가운데선 4 m 간격(개울 여울 6 m 안은 다리 자리라 뺌) | ≤ 15° | **12.9°**(갈림목 (1049, 1304)), 길 8.41 km |
| 이름 붙은 시선 | 탑 문 앞(1000, 415) · 고목 언덕 꼭대기(1096, 1976, 뿌리 둔덕 위 눈높이 4.3 m) · 봉화 언덕 꼭대기(880, 960, 1.8 m) | 고목 · 풍차 · 선돌 / 폭포 · 감시탑 · 바위 문 / 탑 · 풍차 · 부유석 · 고목 | 모두 보임 |
| 마탑 구간 | 아래 「마탑 구간」 — Design.TowerSector | 구간 경사 · 언덕 간격 · POI 간격 · 가림 · 랜드마크 | 모두 지킴 |

- 그 밖에 `Design.Shape`: 언덕 40 개 이상 · 작은 것 > 큰 것 × 2 · 높이 10~60 m · 지름 150~500 m · 가장 가파른 곳 ≤ 35°, 바위 노두 20 개 이상, 손 언덕 4(마탑 구간 시드 언덕은 구간 범위 — 지름 40~150 m · 높이 8~30 m), 계곡 깊이(둑 − 바닥 평균) 완만한 쪽 · 절벽 쪽 ≥ 30 m, 절벽 쪽 45° 넘는 곳, 폭포 20 m 이상 · 랜드마크 자리 60 m 안, 고개 바닥이 양옆 산등성이보다 30 m 낮음. `Design.Pois`: 랜드마크 7~10(종류 모두 다름, 2차의 7 종류는 꼭), POI 40~70 · 묶음 8~18(2~4 개씩) · 종류 비율(계획 ±1.5 개) · 물 밖 땅 위.
- **나무 · 소품이 가리는 것은 보지 않는다**(땅만). 나무가 많은 숲 무늬 · 계곡 숲에서는 실제로 덜 보인다.
- 레이아웃을 고치면 숫자가 바뀐다 — 특히 시선은 길을 깎은 자리(절벽 길 · 비탈길)에서 민감하다. `GetVisibleFrom` · `IsMapPointVisible`(에디터 도구)로 모자란 곳에서 보이는 자리를 찾아 POI 를 옮긴다.

## 마탑 구간 (먼저 완성하는 구간)

2차 플레이 테스트(「여전히 평지 느낌」) 뒤 [레벨 디자인 노트](../dev/level-design) 「7」 대로 탑 둘레를 먼저 완성 수준 그레이박스로 만들었다. 나머지 2 × 4 km 는 2차 그대로 두고 「공사 중」 경계로 알린다. 다 되면 한 구역씩 같은 방식으로 넓힌다.

![마탑 구간 등고선 지도(2 m 마다, 북쪽이 위) — 빨간 선 = 구간 경계, 검은 점 = 언덕 가운데, 큰 빨간 점 = 랜드마크, 작은 점 = POI(묶음마다 같은 색)](/art/terrain/openbeta-sector-map-v3.png)

| 조각 | 레이아웃 | 값 |
|---|---|---|
| 경계 | `Analysis.Sector.Polygon` · 구역 `TowerSector` | 서쪽 X 420(들판 끝) · 북쪽 개울 `Creek_Pasture`(Y 약 1080 → 강) · 동쪽 큰 강 · 남쪽 바다. 땅 1.02 km²(탑에서 약 600~750 m) |
| 능선 · 안부(3차) | `Ridgelines` | **29 줄**(날카로운 단면) — 주능선 5(서쪽 등줄기 64 m · 동쪽 등줄기 52 m · 풍차 능선 · 남동 능선 58 m · 바닷가) · 낮은 등성이 1 · 지맥 13 · 호그백 10. [레벨 디자인 「10」 땅](../dev/level-design#땅-—-등성이-생성기-bsharpcrest). 2차 7 줄은 [레벨 디자인 「9」 땅](../dev/level-design#땅-—-능선-·-안부-·-골-마탑-구간) |
| 구역 노이즈 · 침식(2차) | 구역 `TowerSector` `Ridged*` · `BumpMultiplier`, `Erosion*` | 뒤튼 능선형 노이즈 ±5 m(3차, 2차 ±9 m · 파장 360 m) · 둔턱 × 0.35 · 물방울 침식(칸당 0.6, 사각형 (420, 120) ~ (1780, 1090)). 구역 순서는 `Pasture` 다음(목초지 평균 높이에 구간 능선이 섞이지 않게 — 높이는 둘 다 27 m 라 같다) |
| 작은 언덕 | `TowerSector_Hills` | 12 개(3차 — 2차 56 개의 둥근 언덕이 「고분」으로 보였다), 지름 80~150 m, 높이 4~7 m, 길쭉함 1.6~2.4 · **방향 북동 55° ± 40°**(능선과 같은 결), 실루엣 k = 2.0, 서로 (반지름 합 × 0.75), 길에서 반지름 × 0.35, 손 언덕과 × 0.9 |
| 큰 언덕 | `Hills` | 봉화 · 부유석 · 풍차 언덕 15 m — 등성이 어깨(3차, 꼭대기는 능선이 맡는다) |
| 작은 모양 | `TowerSector_Mounds` · `_Hollows` · `_Rocks` | 둔덕 1.2~3.5 m 14 · 오목한 골 −1.2~−3 m(길쭉하게) 12 · 바위 1.5~4 m 10(2차에 줄임 — 둥근 바위 원뿔이 인공적으로 보였다. 바위는 식생 규칙이 등성이에 모은다) |
| 바닥 잔물결 | 구역 `TowerSector` `ExtraRollingAmplitude` 1.6 m · `ExtraRollingScale` 60 m | 언덕 사이 바닥도 평평하지 않게 |
| 물 | `Creek_TowerBrook` · `Pond_TowerWest` · `Creek_Pasture` | 자연 개울(3차) — 봉화 언덕 기슭 샘 → 굽이쳐 연못으로 합류(연못 길에 다리), 북쪽 경계 개울(큰길 · 동쪽 길에 다리) → 큰 강 |
| 길 | `Path_Main` 앞부분 · `Path_East` · `Path_PondLoop` | 구간 안 2.04 km. 큰길은 탑 문에서 언덕 사이로 세 번 휘고, 연못 길이 갈라진다 |
| 랜드마크 | 탑(빛줄기 90 m) · 풍차 · 부유석(언덕 위 38~51 m) + 멀리 고목 · 바위 문 · 선돌 | 골 안에서도 어디서나 하나는 보이고 96% 의 자리에서 둘 이상 |

**POI 24 곳(묶음 9)** — 가장 가까운 다른 POI 까지 76~120 m, 구간 길 위 어디서나 108 m 안(3차 — 짐수레를 길섶으로 옮기며 길가 돌 사당 · 안부 돌무더기 · 개울가 약초를 더함).

| 묶음 | POI(종류) |
|---|---|
| S1 탑 앞마당 | 꺼진 화로 제단(퍼즐) · 길손의 돌무더기(이야기) |
| S2 연못 동쪽 | 약초 밭(자원) · 연못가 거점 터(거점) |
| S3 연못 | 재칼로프 굴(생물) · 갈대 군락(자원) · 가라앉은 징검돌(퍼즐 — 얼리기) |
| S4 부유석 | 부유석 조각(자원) · 무너진 관측소(폐허) · 바람 룬 돌(퍼즐) |
| S5 봉화 언덕 | 꼭대기 전망(전망대) · 재칼로프 굴 무리(생물) · 비탈의 돌집 터(폐허) |
| S6 길가 목초지 | 부서진 짐수레(이야기 — 길섶) · 들벌 둥지(생물) · 길가 돌 사당(이야기) |
| S7 풍차 | 밀밭 · 건초 더미(자원) · 멈춘 방아(퍼즐 — 바람) · 무너진 방앗간 오두막(폐허) · 재칼로프 굴(생물) |
| S8 북쪽 개울 | 마른 덩굴 돌문(퍼즐 — 불) · 개울 머리 샘(자원) |
| S9 동쪽 길 | 안부 돌무더기(전망대) · 개울가 약초(자원) |

### 블록 · 떠 있는 글씨 · 공사 중

- **랜드마크 8** 위에 「전역 · 구역 랜드마크 — 이름」 + 무엇이 들어갈지(빨간 제목, 글씨 배율 4). **구간 POI 21** 은 종류 색 **블록**(자리 크기 원판 + 7 m 기둥: 퍼즐 주황 · 자원 초록 · 생물 빨강 · 폐허 보라 · 전망대 노랑 · 이야기 하늘 · 거점 파랑) 위에 「종류 — 이름」 + 설명(`Label` · `Note`, 배율 1.8). 구간 밖 POI 는 글씨 없이 2차 소품 그대로.
- **공사 중**: `Analysis.Sector.Fence` 꺾은선(서쪽 들판 끝 → 북쪽 개울 남쪽 둑)을 따라 40 m 마다 주황 기둥(검정 띠 둘) + 기둥 사이 주황 · 검정 줄, 4 기둥마다와 길이 지나는 곳에 「공사 중 — 마탑 구간 밖」 글씨. 기둥 47 · 글씨 15, 모두 막지 않는다(지나갈 수 있다).
- **한글**: `TextRenderComponent` 는 오프라인 폰트라 한글이 네모로 깨진다. `AGrimDevLabel` 은 월드 공간 `UWidgetComponent` 에 Slate `STextBlock`(기본 폰트 Roboto, 없는 글자는 엔진 폴백 `DroidSansFallback`)을 직접 넣어 한글이 그대로 나온다 — 위젯 블루프린트 · 폰트 에셋이 필요 없다. 스크린숏으로 확인(아래).
- **숨기기**: 출시(Shipping) 빌드는 늘 숨기고 위젯도 만들지 않는다. 개발 빌드는 콘솔 `Grim.DevLabels 0`(기본값은 `Config/DefaultEngine.ini` `[SystemSettings]` 에 `Grim.DevLabels=0` 으로 바꿀 수 있다). 데디케이티드 서버 · 커맨드렛은 위젯을 만들지 않는다.
- **거리 · 그리기**: 카메라에서 250 m(`MaxVisibleDistance`, 콘솔 `Grim.DevLabels.MaxDistance <cm>`) 밖이면 숨긴다. 글씨는 렌더 타깃에 한 번만 그리고 위젯 컴포넌트 틱은 끈다 — [성능](./performance#측정-기록).

### 하늘 구역 이름 (레벨 디자인 2차)

사용자가 구역을 알아보게 구역 위 하늘에 큰 한글 이름을 띄운다(개발 표시). 레이아웃 `Sections`(이름 · 층 · 제목 · 설명 · 자리 · 땅 위 높이 — 스크립트만 읽는다), `Scripts/Terrain/section_dress.py` 의 `section_labels` 가 `AGrimDevLabel` 을 놓는다(태그 `LevelDress.SectionLabel`, 늘 로드).

| 층(`Tier`) | 수 | 땅 위 | 모양 | 보이는 거리 |
|---|---|---|---|---|
| `Region` — 지역 1 의 구역 | 8 | 95 ~ 150 m | 금색 제목만, 글씨 120 pt × 배율 14(글자 약 17 m) | 4.5 km |
| `Sector` — 마탑 구간 소구역 | 12 | 40 ~ 75 m | 하늘색 제목 96 pt + 설명 44 pt × 배율 6 | 600 m |

- `AGrimDevLabel` 의 2차 속성: `bSectionLabel`(판 없이 검은 테두리 글씨 · 가운데 맞춤 · 위아래로도 카메라를 봄 · `Grim.DevLabels.MaxDistance` 덮어쓰기를 받지 않음) · `TitleFontSize` · `BodyFontSize`(크게 하면 렌더 타깃 해상도가 올라 멀리서 덜 뭉개진다).
- 끄기: 콘솔 `Grim.DevLabels.Sections 0`(구역 이름만) · `Grim.DevLabels 0`(개발 글씨 모두). **출시(Shipping) 빌드에서는 늘 숨기고 위젯도 만들지 않는다** — [치트](../dev/cheats).

### 구간 검사 (`FGrimTerrainAnalysis::RunSector`, 테스트 `Grimoire.Terrain.Design.TowerSector`)

| 검사 | 재는 법 | 기준(`Analysis.Sector`) | 지금 |
|---|---|---|---|
| 경사 | 구간 안 물 밖 땅, 4 m 격자 | 5~25° ≥ 60%(3차 — 급사면을 일부러 넣었다) | **63.1%**(3차, 2차 71.7%) |
| 언덕 | 가운데가 구간 안인 언덕(작은 모양 빼고) · 가장 가까운 언덕까지(가운데끼리) | 30 개 이상 · 손 2 이상 · 시드 지름 40~150 m · 높이 8~30 m · 간격 가운데값 50~120 m | 59(손 3) · 76 m(2차 — 1차 120 · 56 m) |
| 작은 모양 | 가운데가 구간 안 | 20 개 이상 | 53 |
| 물 | 구간 안 연못 · 개울 | 하나 이상씩 | 연못 1 · 개울 2 |
| POI | 구간 안 수 · 가장 가까운 다른 POI 까지 | 15~25 · 60~120 m | 21 · 94~116 m |
| POI 밀도 | 구간 안 길 10 m 마다 가장 가까운 POI | ≤ 120 m | 111 m |
| **가림** | 구간 안 길 25 m 마다(80 곳) 눈높이 1.8 m 에서 24 방향 × 100 · 150 · 200 · 250 · 300 m 앞 땅 + 1 m 가 높이 지도에 가려지는가 | 전체 ≥ **65%**(2차 — 1차 70%, 결정 기록 2026-11-08), 점 하나 ≥ 40% | **69.5%**(100 m 43% · 200 m 75% · 300 m 85%), 가장 낮은 점 42% — 1차 79.2%. 능선 사이 골은 일부러 긴 시선 통로 |
| 랜드마크 | 같은 점에서 보이는 랜드마크(ViewDistance 안) | 90% 의 점에서 2 개 이상 · 어디서나 1 개 이상 | 100% · 최소 2 · 평균 4.0 |

- 2차 레이아웃에 같은 검사를 돌리면 가림 61.6%(100 m 30%) · 언덕 간격 81 m · POI 8 곳(131~274 m) — 「평지 느낌」이 숫자로도 보인다.
- 가림은 땅만 본다(나무 · 블록 제외). 레이아웃만 고칠 때는 맵 없이 `Grimoire.Terrain.Design` 을 돌린다.

| 탑 문 앞 | 큰길 100 m | 큰길 200 m |
|---|---|---|
| ![](/art/terrain/openbeta-door-v3.jpg) | ![](/art/terrain/openbeta-path100-v3.jpg) | ![](/art/terrain/openbeta-path200-v3.jpg) |
| **큰길 400 m** | **봉화 언덕 꼭대기** | **연못가** |
| ![](/art/terrain/openbeta-path400-v3.jpg) | ![](/art/terrain/openbeta-hilltop-v3.jpg) | ![](/art/terrain/openbeta-pond-v3.jpg) |
| **북쪽 경계(공사 중)** | **하늘에서 마탑 구간** | |
| ![](/art/terrain/openbeta-fence-v3.jpg) | ![](/art/terrain/openbeta-sector-aerial-v3.jpg) | |

## 랜드스케이프 크기 (계산 근거)

| 값 | 고른 것 | 까닭 |
|---|---|---|
| 섹션 | 127칸(정점 128 = 2⁷) | 섹션이 가질 수 있는 가장 큰 크기. 섹션 하나가 LOD 단위 |
| 구성 요소 | 2 × 2 섹션 = 254칸 | 구성 요소 수를 줄여 CPU · 드로 콜을 아낀다. 무게 지도는 구성 요소당 256² 한 장 |
| 구성 요소 수 | 8(동서) × 16(남북) = 128 | 1 m 칸으로 2032 × 4064 m |
| 정점 | 2033 × 4065 (830만) | 16비트 높이 지도 약 16 MB |
| 칸 크기 | 1 m(배율 100) | 걷는 눈높이에서 기복 · 둔턱(1~3 m)이 보이는 해상도 |
| Z 배율 | 100 | 높이 범위 ±256 m, 해상도 1/128 m. 이 땅은 −25 ~ 187 m |
| LOD | 엔진 기본 · 나나이트 랜드스케이프 끔 | 그레이박스. 나나이트는 지형 3차(M7)에서 |

## World Partition · 성능

| 설정 | 값 | 까닭 |
|---|---|---|
| 런타임 해시 | RuntimeHashSet, 메인 파티션 LH 격자 | 5.8 새 맵 기본 |
| 셀 크기 | 256 m | 폴리지 파티션 격자와 같게 |
| 스트리밍 거리 | 512 m | 나무가 사라지는 거리(300~400 m)보다 멀게 |
| 랜드스케이프 프록시 | 2 × 2 구성 요소 = 32개, **늘 로드** | 땅에 구멍이 나지 않고 멀리 보인다 |
| 늘 로드 | 탑 · 앞마당 · 표식 · **랜드마크 · POI 그레이박스 · 개발 표시 글씨 44 · 공사 중 기둥 · 다리(모두 1,037 — 2차 고목 큐브 목업 약 230 포함)** · 하늘 구역 이름 20 · 하늘 · 기후 · 바다 · 지도 끝 · 시작 자리 · 서식 구역 | 랜드마크는 수 km 밖에서 보여야 하고, POI 소품도 시선 검사 거리(450 m)가 스트리밍 거리에 가깝다 |
| 공간 로드 | 식생 · 연못 · 개울 물 · 연못 볼륨 | 멀면 내린다 |

- 성능 수치는 [성능 예산 · 최적화 규칙](./performance#측정-기록)에 둔다 — 늘 로드 그레이박스가 826 개(2차 약 350 + 마탑 구간 블록 · 공사 중 표시)라 인스턴스 · 공간 로드로 옮길 차례(성능 「다음 할 일」 2). 개발 표시 글씨 44 개는 위젯 렌더 타깃이 하나씩이라 250 m 안에서만 보이고 한 번만 그리며, 출시 빌드에서는 만들지 않는다.

## 머티리얼 (자동, 임시 색)

지금 랜드스케이프 재질은 **`Dress/M_Terrain_Dress`**(레벨 아트 1차 — 같은 규칙에 StyleHex 지형 텍스처, [레벨 디자인 「8」](../dev/level-design#_8-레벨-아트-1차-—-fab-팩-시험-적용-2026-11-08)). `build_openbeta.py` 는 그 재질이 있으면 **처음부터 그것으로** 랜드스케이프를 만든다(연 맵의 재질을 바꿔 저장하면 풀 지도가 틀어진다 — 같은 절 「풀 지도」). `GRIM_TERRAIN_GREYBOX=1` 이면 아래 그레이박스 재질.

`M_Terrain_Auto` — 경사(약 28° 부터 흙, 45° 부터 바위) · 높이(90 m 위로 흙 섞임) · 레이어(`Shore` 모래 · `Path` 길 · `Forest` 계곡 숲 바닥) · 풀 출력(`GT_Terrain_Meadow`). 단색 `M_Terrain_Color` + `MI_Terrain_*`(나무 · 덤불 · 바위 · 풀, 그리고 2차의 돌 · 어두운 돌 · 나무판 · 폭포 · 깃발 · 고목 잎 · 고목 껍질 · 돛 · 지붕 · 검정 · 흙 · 뼈 · 풀 덩이), 물 `M_Terrain_Water`. 북부 고지(88 m 위)는 높이 규칙 때문에 흙빛이 섞인다.

## 물 — 단순 평면 + 물리 볼륨

| 물 | 겉모습 | 헤엄 | 태그 |
|---|---|---|---|
| 바다 + 큰 강 | 해수면(0 m) 평면 하나 | `WaterVolume_SeaRiver` | `Water` · `Water.Sea` · `Water.River` |
| 연못 5곳 | 원판 | `WaterVolume_Pond_*` | `Water` · `Water.Pond` · 연못 이름 |
| 개울 2개(계곡 · 폭포) | 12 m 조각 평면(물 높이를 따라 기울임) | 없음(얕다) | `Water` · `Water.Creek` · 개울 이름 |
| 자연 개울 2개 + 합류 연못(3차) | **물 표면 메시 하나**(`SM_Water_<개울>`, 격자 1 m · `BuildWaterSurfaceMesh`) — 물가선 = 땅과의 교선, 끝 물 높이 = 연못 물 높이, 재질 `MI_Dress_Water`(Water Materials 팩 `M_River` 인스턴스 — 아래 「물 재질」) | 연못만 `WaterVolume_*` | 같음 |
| 폭포 | 흰 띠 · 물보라(그레이박스) | `WaterVolume_Landmark_Waterfall`(띠를 따라) | `Water` · `Water.Falls` |

- 엔진 Water 플러그인을 쓰지 않은 까닭은 1차와 같다(프로젝트에 꺼져 있고, 물 몸체가 높이 지도를 두 번 깎는다).

### 물 재질

자연 개울 · 합류 연못의 표면 재질은 `MI_Dress_Water` — [Water Materials](../art/credits) 팩(tharlevfx, CC BY 4.0, [Q126](../spec/decisions#미결-질문))의 `M_River` 인스턴스다. 값은 `Scripts/Terrain/dress_assets.py` 의 `WATER_SCALARS` · `WATER_COLORS` 에만 있고, `Scripts/Terrain/apply_water_material.py` 가 물 표면 메시를 다시 구운 뒤 메시와 액터에 입힌다(맵 전체를 다시 만들지 않는다).

| 메시 데이터 | 값 | 재질에서 |
|---|---|---|
| UV0 | 개울 = (호 길이 m, 물길에서 가로 m) · 연못 = (지도 X, 지도 Y m) | 팩 재질의 물결이 UV0 U 쪽으로 흐른다 — 개울의 흐름 방향 그대로. 호 길이는 꺾은점 번호 + 앞뒤 거리로 잇는다(정수로 두면 1 m 칸마다 끊겨 격자무늬가 보였다). 연못의 U 는 지도 X 라 아주 천천히 동쪽으로 흐르는 듯 보인다 — 잔물결이 거의 없어 눈에 띄지 않는다 |
| 정점 색 R | 개울 1 · 연못 0 | 팩의 잔물결 세기(`SubtleNoise_Variation`)에 더해진다 — 개울은 고르게 일렁이고 연못은 얼룩지게 |
| 정점 색 B · A | 0 · 1 | 물가 물결 방향 · 흐름 모프 — 모프는 세기 0 으로 끈다 |

- **왜 `M_River`(가벼운 판이 아니라)**: `M_River_Cheaper` 는 볼륨 라이팅이라 하늘을 반사하지 못하고 큐브맵 자체 발광에 기댄다 — 낮에 위에서 보면 흙길처럼 보이고 시각 변화에도 따라가지 않는다. `M_River` 는 표면 조명이라 하늘 · 나무가 물에 비친다(반투명 표면 조명은 3차의 자체 재질과 같다). 성능은 [QA-LVL-034](../qa/level-art#물-재질-q126-2026-11-09).
- **끈 것**: 파도 정점 이동(`Master_Intensity` · `River1_Intensity` · `River2_Intensity` = 0 — 폭 2 ~ 5 m 개울에서 물가와 부딪힌다), 흐름 모프 노멀. **양면**은 인스턴스 재정의로 켰다 — 연못 속에서 올려다볼 수 있다.
- **샘의 동심원**(`M_Dress_SpringRipple`)은 그대로 — 물 표면 위에 뜬 작은 판(`Spring_Ripple`)이라 물 재질과 상관없다.
- 바다 · 원판 연못 5곳 · 계곡 개울은 아직 예전 그레이박스 물(`M_Terrain_Water`)이다 — 자연 개울로 바꿀 때 같은 재질을 쓴다([Q136](../spec/decisions#미결-질문)).

## 식생 — 생성기 규칙 + 인스턴스드 폴리지 (PCG 아님)

레벨 디자인 2차(2026-11-08)부터 **자연 분포**: 고정 시드 후보 → 밀도(아래 배수를 곱함)로 고르기 → **포아송 원판**(`MinSpacing`, 격자 느낌을 없앤다) → 크기 · 기울기 · 종 값. `FGrimTerrainGenerator::ScatterFoliage`(`Source/Grimoire/World/GrimTerrainGenerator.h`). 새 값이 모두 기본값인 규칙은 예전(지터 격자 · 칼 경계)과 같다.

| 밀도 배수(`FGrimTerrainFoliageRule`) | 뜻 |
|---|---|
| 숲 무늬 `ForestWeight` | 뒤튼 노이즈(`ForestWarp`) · 경계 섞기(`ForestSoftness`) 0~1 — 초원 밀도와 숲 밀도 사이를 섞는다 |
| 무리 `Clump*` | 초원의 숲덩이 · 바위 무더기와 빈터(같은 `ClumpChannel` 끼리 같은 노이즈 — 덤불이 숲덩이를 둘러싼다) |
| 숲 안 덩이 `ForestPatch*` | 숲 안에서도 몇 그루씩 모이고 작은 빈터 — 고른 간격 「과수원」 느낌을 없앤다 |
| 숲 가장자리 `ForestEdgeFactor` | 숲 무늬 0.5 언저리(덤불 띠) |
| 등성이 · 골 `RidgeFactor` · `ValleyFactor` | 둘레(`CurvatureRadius`) 평균 높이 − 가운데 — 볼록 · 오목 |
| 비탈 `SlopeFactor` · 물가 `WaterEdge*` · 길 가장자리 `PathEdge*` | 비탈 바위, 개울을 따라 늘어선 나무 · 덤불, 길가 덤불 |
| 피하기 `AvoidRules` · `AvoidDistance` | 먼저 뿌린 규칙(나무) 자리 둘레에는 놓지 않는다 |

| 규칙 | 밀도(헥타르당: 초원 · 숲 · 계곡 · 가파른 비탈) | 2차 값 | 개수 |
|---|---|---|---|
| Tree | 28(무리 안) · 70 · 0 · 0 | 포아송 3.5 m · 무리 60 m(밖 × 0.04) · 숲 안 덩이 24 m · 등성이 × 0.3 · 골 × 1.4 · 물가 × 3 · 크기 0.6~1.6(숲 · 덩이 가운데일수록 큼) · 3° | 4,732 |
| DarkTree(계곡) | 0 · 0 · 150 · 0 | 포아송 4.5 m · 3° | 2,256 |
| Bush | 10 · 22 · 40 · 0 | 포아송 2.2 m · 숲 가장자리 × 3 · 길가 × 2.2 · 물가 × 2.5 · 6° · 비탈 맞춤 · 나무 피함 | 5,974 |
| Rock | 1.4 · 1.5 · 4 · 30(32°+) | 포아송 2.5 m · 무리 45 m · 등성이 × 4 · 비탈 × 2.5 · 골 × 0.4 · 12° · 비탈 맞춤 · 나무 피함 | 3,529 |

- 규칙: 경사 상한(6° 앞부터 부드럽게) · 높이 범위 · 물가 · 길에서 떨어짐 · 탑 평지 + 40 m 비움 · 거점 평지 비움 · 랜드마크 25 m · POI 반지름 + 6 m 비움. 개수는 레이아웃이 바뀌면 달라진다.
- PCG 를 쓰지 않은 까닭은 1차와 같다(플러그인 꺼짐 · 생성기만 아는 마스크 · 커맨드렛에서 PCG 생성이 돌지 않음).
- **맵 전체**의 Tree · DarkTree · Bush · Rock 자리는 `dress_openbeta.py` 가 StyleHex 메시(`FT_Dress_*`)로 놓는다(1차는 마탑 구간만 · 밖은 원뿔). 메시는 생성기 **Variant**(같은 종끼리 무리 짓는 노이즈, `UGrimTerrainEditorLibrary::GetFoliageInstances`)로 고른다 — 자작나무 숲덩이 · 활엽수 숲 · 숲덩이 덤불 / 풀꽃 덤불 · 작은 돌 / 무더기 / 큰 바위(`dress_assets.RULE_SWAP`). 나무 잎은 우리 재질 인스턴스(`Content/Grimoire/Terrain/Dress/MI_Dress_Leaves_*`, 팩 잎 재질이 부모)로 덮어쓴다. 나무 끝 컬 480 m.
- 구간 구도 자리(풍차 42 m · 고목 48 m · 밭 · 곳간 · 봉화대 · 둥치)는 식생을 비운다(`dress_openbeta.py` 의 `CLEAR_ZONES`).
- 랜드스케이프 풀은 `GT_Dress_Meadow`(StyleHex 풀 · 꽃). 쿡 빌드는 풀 지도를 런타임에 만든다(`grass.GrassMap.UseRuntimeGeneration=1`).
- 레이아웃만 고칠 때 자리는 `Scripts/Terrain/preview_openbeta.py` → `Scripts/Terrain/overlay_foliage.py` 로 지도 위 점으로 본다.

## 마법탑

- `tower_greybox.py` 의 `TowerBuilder(origin, yaw, ground)` 가 탑 기준 좌표를 월드로 옮긴다. `build_tower` · `build_yard` 는 L_Tower 와 같은 함수.
- L_OpenBeta: 원점 = 탑 자리, Yaw 0 — 탑 문이 북쪽 큰길을 본다. 탑 평지 45 m(1차 42 m).

## 손으로 다듬을 때

| 다시 만들면 **덮어쓰는** 것 | **안 덮어쓰는** 것 |
|---|---|
| `L_OpenBeta` 전부(맵 · 외부 액터 · HLOD 층) — 손으로 깎은 지형 · 칠한 레이어 · 옮긴 나무 · 더 놓은 액터 · 랜드마크 · POI 소품 위치 · 레벨 아트(`dress_openbeta.py` 를 다시 돌려 되살린다) | `Content/Grimoire/Terrain` 의 머티리얼 · 인스턴스 · 레이어 정보 · 폴리지 타입 · 풀 타입 |
| `openbeta-preview.png` · `Saved/Terrain/openbeta.r16` | `openbeta_layout.json` · `L_Tower` · 다른 맵 |

- **모양 · 자리를 바꾸려면 JSON 을 고쳐 다시 만든다**(구역 높이 · 언덕 · 계곡 · 길 · 랜드마크 · POI). 고친 뒤 `Grimoire.Terrain.Design` 으로 숫자를 본다.
- 손 조각이 꼭 필요하면 **마지막에 한 번** 한다.
- 다시 만들면 외부 액터 파일 이름이 모두 바뀐다 — 의도한 때만 커밋한다.

## 확인

- **테스트** `Grimoire.Terrain.*`: `Layout.Size` · `Generator.Deterministic`(작은 지형 — 구역 · 손 언덕 · 언덕 군 포함) · `Generator.OpenBeta`(2 × 4 km, 탑 평지 평평, 서 · 북 산등성이 > 목초지, 바닷속, 강바닥 < 해수면, 연못 4~6 둑 > 물 > 바닥, 개울은 아래로만, 거점 평지, 서식 구역 셋) · `Generator.Ridgeline`(2차 — 능선 꼭대기 · 안부 · 양쪽 폭, 침식 결정성) · `Foliage.Rules` · `Foliage.Natural`(2차 — 포아송 간격 · 종 값 · 결정성) · `Design.Shape` · `Design.Paths` · `Design.Sightlines` · `Design.Pois`(위 「설계 검사」) · `Design.TowerSector`(위 「마탑 구간」) · `Map.OpenBeta`(외부 액터 수 · 표식 태그 — 레이아웃의 `Landmark.*` 8 · `Poi.*` 종류 포함).
- **헤드리스 게임** `check_openbeta.ps1`(-game -nullrhi): 시작 자리(탑 안, 땅 45 m) → 앞마당에서 북쪽 6초 걷기 → 목초지(재칼로프) · 계곡(검은 개) 걷기 → 시간 · 날씨 → 비 → 저지대 연못(`WaterVolume_Pond_Lowland`) 헤엄 → 표식 땅 높이 · 그림 높이(−25 ~ 187 m).
- **스크린숏** `shoot_openbeta.ps1`(-game -RenderOffscreen): 기본은 마탑 구간 `-v3`(눈높이 1.8 m — 위 「마탑 구간」 표), `-Set v2` 는 아래 2차 그림(덮어쓰니 일부러만). `-Set ld2 -RunSeconds 900` 은 레벨 디자인 2차 19 장(`docs/public/qa/ld2-*.jpg` — 구간마다 눈높이 · 하늘, 앞 다섯은 레벨 아트 1차와 같은 자리). `Grim.Terrain.ViewFrom` 의 Z 를 `+170` 처럼 주면 그 자리 **랜드스케이프** 위 cm(나무 · 고목 잎은 건너뛴다).
- **콘솔**(개발 빌드): `Grim.Go <이름>` · `Grim.Go.List`(이동 치트), `Grim.Terrain.Probe [지연]` · `Grim.Terrain.Walk <지연> <초> [Yaw]` · `Grim.Terrain.Teleport <X> <Y> [지연]` · `Grim.Terrain.ViewFrom <X> <Y> <Z|+땅 위> <Yaw> <Pitch> [지연] [스크린숏 이름]`.

![탑 문 앞에서 북쪽 — 큰길 · 겹친 구릉 · 지평선의 고목](/art/terrain/openbeta-tower-north.jpg)

![고목 언덕 꼭대기에서 북쪽 — 계곡 절벽 · 폭포 · 고지 · 멀리 바위 문](/art/terrain/openbeta-tree-north.jpg)

![고목 언덕에서 동쪽 — 강 · 감시탑](/art/terrain/openbeta-tree-east.jpg)

![고목 언덕에서 남쪽 — 목초지 · 탑 · 바다](/art/terrain/openbeta-tree-south.jpg)

![고목 언덕에서 서쪽 — 연못 저지대 · 서쪽 산등성이](/art/terrain/openbeta-tree-west.jpg)

![계곡 입구 — 큰길이 계곡 가장자리에 닿는 곳에서 폭포 쪽](/art/terrain/openbeta-valley-entrance.jpg)

![북부 고지에서 고개 바위 문 쪽](/art/terrain/openbeta-highland.jpg)

1차(비교): [남쪽 바다 위에서](/art/terrain/openbeta-aerial-south-v1.jpg) · [앞마당에서 북쪽](/art/terrain/openbeta-yard-north-v1.jpg) · [동쪽 큰 강](/art/terrain/openbeta-river-v1.jpg) · [위에서 본 지도](/art/terrain/openbeta-preview-v1.png)

## 다리 — 곡선 교점에서 계산 (3차)

생성기가 길과 **자연 개울의 실제 곡선**이 만나는 자리마다 `FGrimTerrainBridge`(자리 · 길 방향 Yaw · 길이 · 폭 · 상판 높이 · 물 높이 · 사이각)를 낸다(`FGrimTerrainSummary.Bridges`). 길은 그 구간 노면을 상판 높이로 평평하게 고정하고 양쪽을 경사 상한으로 잇는다. `landmark_greybox.build_natural_bridges` 가 값대로 짓는다. 계곡 · 폭포 쪽(자연 개울이 아닌 물)은 예전 `build_bridges`.

## 범위 밖 · 다음

- 지형 2차 나머지(M4~M6): POI 자리의 놀이(퍼즐 · 채집 · 생물 · 던전) · 거점 · 밭 · 사냥꾼 야영지 · 강 다리 · 날씨 볼륨 다듬기.
- 지형 3차(M7): 정식 머티리얼 · 식생(PCG) · 조명 · 소품 · 랜드마크 정식 에셋(풍차 회전 포함), 나나이트 랜드스케이프 검토.
- 10 × 10 km: 랜드스케이프를 공간 로드 + HLOD 로, 지역마다 기후 볼륨.
