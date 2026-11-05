# 지형 — 오픈 베타 땅 (L_OpenBeta)

> 계획: [로드맵 — 지형은 언제 찍나](./roadmap#지형은-언제-찍나-2026-11-05) 의 지형 1차, 작업 분할 [M3](./m3-plan) 갈래 W. 지역: [세계 — 지역과 크기](../spec/world#지역과-크기-r34-—-q36-확정) 1번(마법탑 · 온난 지대).
> **2차(다시 만들기, 2026-11-05)**: 1차가 「그냥 평지」였던 것을 [레벨 디자인 노트](../dev/level-design) 「6. 지역 1 다시 만들기 계획」대로 고쳤다 — 구역 높이 · 크기가 섞인 언덕 군 · 한쪽 절벽 계곡 · 폭포 · 고개 · 랜드마크 7 · POI 40 · 자동 설계 검사. 결과 숫자와 계획과 달라진 점은 노트의 「결과」.
> 코드: `Source/Grimoire/World/`. 스크립트: `Scripts/Terrain/`. 에셋: `Content/Grimoire/Terrain/`. 맵: `Content/Grimoire/Maps/L_OpenBeta`(World Partition, 게임 맵 · 에디터 시작 맵).

![L_OpenBeta 를 하늘 2.9 km 에서 — 북쪽이 위. 남쪽 탑 곶 · 목초지 · 서쪽 연못 저지대 · 가운데 구릉과 고목 · 계곡(한쪽 절벽) · 북부 고지 · 동쪽 큰 강](/art/terrain/openbeta-aerial.jpg)

## 한눈에

| 조각 | 파일 | 하는 일 |
|---|---|---|
| 레이아웃(수치) | `Scripts/Terrain/openbeta_layout.json` | 구조체 `FGrimTerrainLayout` 의 JSON — 시드 · 크기 · 구역 · 언덕 · 바위 능선 · 고개 · 계곡 · 강 · 개울 · 연못 · 길 · 거점 평지 · 표식 · **랜드마크** · **POI** · **설계 검사 기준** · 식생. **지형을 바꾸려면 여기를 고치고 다시 만든다** |
| 높이 지도 생성기 | `FGrimTerrainGenerator`(`World/GrimTerrainGenerator.*`) | 순수 계산(월드 · 에셋 모름). 높이(m) · 물가 · 길 · 계곡 숲 무게 · 물과 길까지 거리 · 물 목록(폭포 포함) · 놓인 언덕 · 길 선 · 식생 자리. 830만 정점을 몇 초 |
| 설계 검사 | `FGrimTerrainAnalysis`(`World/GrimTerrainAnalysis.*`) | 순수 계산. 경사 분포 · 구역 평균 높이 차 · 길 위 시선(높이 지도 시야) · POI 밀도 · 길 경사 — 아래 「설계 검사」 |
| 에디터 도구 | `UGrimTerrainEditorLibrary`(`World/GrimTerrainEditorLibrary.*`) | 파이썬이 부르는 C++: 레이아웃 읽기 · 생성 캐시 · 땅 높이 · **검사 보고** · **시선 조회** · 랜드스케이프 가져오기 · 레이어 정보 · 폴리지 타입 · 식생 변환 · 브러시 볼륨 · World Partition 격자 · 미리보기 그림 · r16 내보내기 |
| 표식 | `AGrimTerrainMarker`(`World/GrimTerrainMarker.*`) · 태그 `GrimTerrainTags` | 자리만 표시하는 액터 — GameplayTag `MarkerTag` + 반지름. `FindMarkers(월드, 태그)` 로 찾는다 |
| 맵 만들기 | `Scripts/Terrain/build_openbeta.py`(+ `terrain_assets.py`) | 맵을 지우고 처음부터: 설계 검사 숫자 로그 → 랜드스케이프 → 물 → 식생 → 표식 → **랜드마크 · POI 그레이박스 · 다리** → 마법탑 → 하늘 · 기후 · 지도 끝 → World Partition → 저장 |
| 랜드마크 · POI 모양 | `Scripts/Terrain/landmark_greybox.py` | 엔진 기본 도형으로 종류마다 실루엣이 다른 그레이박스(아래 「랜드마크」「POI」) |
| 마법탑 | `Scripts/Tutorial/tower_greybox.py` | L_Tower 와 **같은 함수**로 탑 그레이박스를 짓는다 |
| 게임플레이 배치 | `Scripts/Integration/place_openbeta_gameplay.py` | 서식 구역 볼륨 3(`Habitat.*` 표식 자리). 지형을 다시 만들면 **이것도 다시 돌린다**. 냄비는 탑 그레이박스가 이미 놓으므로 있으면 건너뛴다 |
| 확인 | `Scripts/Terrain/check_openbeta.ps1` · `shoot_openbeta.ps1` · 콘솔 `Grim.Terrain.*` · 테스트 `Grimoire.Terrain.*` | 아래 「확인」 |

## 만드는 법 · 다시 만드는 법

```powershell
# 1) C++ 빌드(에디터를 닫고) — CLAUDE.md 의 Build.bat
# 2) 맵 만들기(헤드리스, 창 없음). -nullrhi 가 아니라 -AllowCommandletRendering — 아래 「RHI 가 필요한 까닭」
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Terrain\build_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 3) 게임플레이 액터(서식 구역) 다시 놓기
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Integration\place_openbeta_gameplay.py" -unattended -nop4 -nosplash -nullrhi
# 4) 확인
powershell -File Scripts\Terrain\check_openbeta.ps1          # 끝에 PASS
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -ExecCmds="Automation RunTests Grimoire.Terrain; Quit" -unattended -nop4 -nosplash -nullrhi -log
powershell -File Scripts\Terrain\shoot_openbeta.ps1          # 스크린숏 → docs/public/art/terrain/*.jpg
```

- 첫 실행은 셰이더 때문에 몇 분, 그 뒤는 2~3분. 끝에 로그 `build_openbeta: 개수 …` · `saved /Game/Grimoire/Maps/L_OpenBeta`. 시작할 때 설계 검사 숫자도 `LogGrimTerrain: 지형 검사:` 로 남긴다.
- **레이아웃만 고칠 때는 맵을 만들지 않고** 테스트 `Grimoire.Terrain.Design` 만 돌려 숫자를 본다(1~2분). 테스트가 JSON 을 바로 읽는다.
- **같은 레이아웃 · 같은 시드면 같은 땅**: 높이 · 무게 · 물 높이 · 언덕 자리 · 식생 자리가 모두 시드로 정해진다(테스트 `Grimoire.Terrain.Generator.Deterministic`). 액터 GUID 는 매번 새로 나서 `__ExternalActors__` 파일 이름이 바뀐다 — 다시 만들면 git 에는 파일 수백 개가 지워지고 새로 생긴다.
- 맵이 에디터 시작 맵이라 커맨드렛이 이미 열고 있을 수 있다 — 스크립트는 빈 맵으로 바꾸고 치운 뒤 지운다(안 그러면 `new_level` 이 「이미 있다」로 실패).
- 환경 변수 `GRIM_TERRAIN_REFRESH_ASSETS=1` 이면 `Content/Grimoire/Terrain` 의 머티리얼 · 폴리지 타입 · 레이어 정보 · 풀 타입도 지우고 새로 만든다(기본은 있으면 그대로).
- 부산물: `docs/public/art/terrain/openbeta-preview.png`(위에서 본 지도 그림, 북쪽이 위, 랜드마크 = 빨간 큰 점 · POI = 묶음마다 같은 색 작은 점), `Saved/Terrain/openbeta.r16`(16비트 높이 지도, 랜드스케이프 순서 4065 × 2033).

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
| 0. 언덕 놓기 | 손 언덕 5 + 언덕 군(다각형 안에 시드로 Count 개, 크기 = 난수^SizeBias → 작은 것이 많다). 길 · 개울 · 강 · 계곡 가운데선 · 탑 · 연못 · 거점을 피하고 서로 (반지름 합 × MinSpacing) 떨어진다. 가장 가파른 곳(타원의 짧은 쪽)이 `HillMaxSlopeDeg` 를 넘으면 높이를 줄인다. 바위 노두(작은 모양)는 시드 언덕 꼭대기 둘레 · 고지에 | `Hills` · `HillFields` · `HillSharpness` · `HillMaxSlopeDeg` |
| 1. 바탕 | 구역 바닥(다각형, 경계 섞기 + 들쭉날쭉) + 큰 노이즈 · 물결 · 둔턱 + 언덕(가장 큰 것 + 겹친 나머지 × 0.3) + 바위 노두 + 바위 능선 + 가장자리 산등성이(북쪽은 고개에서 낮아짐) | `Zones` · `Noise*` · `Rolling*` · `Bump*` · `Ridges` · `*RidgeWidth/Height` · `PassGaps` |
| 1b. 침식 골 | 경사 7° 넘는 비탈에, 내리막 방향으로 길게 뻗은 좁은 골(깊이 0.7 m 안팎) | `Gully*` |
| 1c · 2. 계곡 | 바닥 높이 = 양쪽 둑 중 **낮은 쪽** − 깊이, 이동 평균, 하류로 내려가기만. 한쪽(고지 쪽)은 거의 곧은 절벽, 다른 쪽은 부드러운 S 자. 바닥 · 비탈 폭이 계곡을 따라 들쭉날쭉 | `Valley` |
| 2. 탑 · 해안 · 강 | 탑 언덕(꼭대기 평지 45 m), 남쪽 해안 · 곶 절벽, 큰 강(물 = 해수면) | `TowerHill` · `Coast*` · `River` |
| 3. 길 | 길 선을 2 m 마다 땅 높이로 재고, 경사 상한(`PathGradeDeg` 10°)을 지키는 가장 높은 「깎기」선과 가장 낮은 「메우기」선의 가운데를 고른 뒤(`PathSmoothing` 40 m 이동 평균) 그 높이로 깎고 메운다. 갈림길은 큰길 노면을 덮지 않고 큰길 높이에서 시작한다 | `Paths` · `PathGradeDeg` · `PathSmoothing` |
| 4. 개울 · 폭포 | 물 높이는 상류 → 하류로 내려가기만. 4 m 마다 1.2 m 넘게 떨어지는 가장 큰 구간(8 m 이상)을 「폭포」로 물 목록에 낸다 | `Creeks` |
| 5. 연못 · 거점 평지 | 둘레 땅에서 물 높이, 대접 모양 / 반지름 안 평평 | `Ponds` · `BaseSites` |

언덕 실루엣 = 높이 × (1 − t)^k (1 + k t), t = 가운데 거리 / 반지름, k = 2.4 — 꼭대기가 둥글고 가운데가 곧고 밑동이 퍼지는 「둥근 삼각형」. 가장 가파른 기울기 = 높이 / 반지름 × 1.6.

## 구역 · 언덕 · 물 (지금 레이아웃)

| 구역(`Zones`, 뒤가 앞을 덮음) | 다각형(대략) | 바닥 높이 | 검사 평균 높이 |
|---|---|---|---|
| `Pasture` 남부 목초지 | 지도 Y 380~1330 | 27 m | 32.0 m |
| `CentralHills` 중부 구릉 | Y 1300~2600(서쪽은 3300 까지) | 60 m | 60.5 m |
| `PondLowland` 연못 저지대 | 서쪽 X 230~860, Y 1000~2300 | 17 m | — |
| `Riverside` 강변 | 동쪽 X 1570~, Y 450~2450 | 16 m(강 둑 16 m) | — |
| `Highland` 북부 고지 | 계곡 북쪽 ~ 북쪽 끝 | 88 m → 북쪽 끝 138 m | 121.7 m |
| 탑 곶 | (1000, 380), 반지름 260 m | 꼭대기 평지 45 m | — |

| 언덕(`Hills`, 손) | 자리 | 높이 · 지름 | 위에 있는 것 |
|---|---|---|---|
| `Hill_GiantTree` | (1100, 1980) | 55 m · 500 m(타원 1.15) | 거대 고목 — 중부 구릉에서 가장 큰 언덕 |
| `Hill_Windmill` | (1400, 870) | 24 m · 300 m | 풍차 |
| `Hill_Lookout` | (1380, 1580) | 36 m · 330 m | 전망 언덕(POI 전망대) |
| `Hill_StoneCircle` | (560, 3700) | 28 m · 340 m | 선돌 고리 |
| `Hill_Watchtower` | (1620, 2120) | 26 m · 240 m | 폐허 감시탑(동쪽은 강 둑 절벽) |

- 언덕 군(`HillFields`): 목초지 72 · 구릉 50 · 저지대 40 · 고지 65 개를 시도해 **언덕 232 개**(손 5 포함, 지름 250 m 아래 212 · 350 m 이상 4 — 작은 것이 많고 큰 것이 적게), 바위 노두 85 개. 자리가 모자라면 덜 놓인다(로그 `언덕 군 …: n / m 개만`).
- 바위 능선 3(고지 동쪽 · 고개 양옆), 고개 `Gap_NorthPass`(지도 X 1000, 바닥 폭 80 m) — 북쪽 산등성이(60 m)가 그 둘레에서 사라져 협곡이 된다.
- 계곡: (420, 3420) → (1770, 2610), 바닥 폭 75 m(±30%), 깊이 35 m(낮은 둑 기준), 완만한 쪽 110 m, 절벽 쪽 28 m(±60%). 바닥 높이 약 15~30 m.
- 연못 5: `Pond_TowerWest`(700, 560) · `Pond_Lowland`(600, 1400, 켈피) · `Pond_Reed`(480, 1830) · `Pond_Marsh`(690, 2150) · `Pond_Upper`(1330, 3100, 고지 — 폭포 물머리).
- 개울 3: `Creek_Valley`(계곡 바닥 → 큰 강) · `Creek_Falls`(`Pond_Upper` → 계곡, **폭포 약 76 m**) · `Creek_Pasture`(목초지 샘 → 큰 강).
- 큰 강 · 가장자리 산등성이 · 해안은 1차와 같다(강 둑 폭만 70 → 50 m). 지도 끝은 높이 1 km 막는 볼륨(`MapEdge_*`).

## 길

| 길 | 경로 | 폭 · 둑 |
|---|---|---|
| `Path_Main` 큰길 | 탑 문 → 목초지 → 고목 언덕 서쪽 기슭 → 계곡 남쪽 비탈을 내려가 **다리**(개울) → 북쪽 절벽 벽을 비스듬히 오르는 **절벽 길**(약 680 m, 75 m 오름) → 고지 → **바위 문** 바로 너머(3925) | 4 m · 14 m |
| `Path_East` | 큰길(1110, 900) → 풍차 언덕 남쪽 → 강변 → 감시탑 언덕 기슭 | 3 m · 11 m |
| `Path_West` | 큰길(1060, 1300) → 연못 저지대 → 늪 연못 | 3 m · 11 m |
| `Path_Highland` | 절벽 길 꼭대기(720, 3440) → 고지 거점 터 → 폭포 위 전망 | 3 m · 11 m |
| `Path_StoneCircle` | 큰길(800, 3570) → 선돌 언덕 | 3 m · 11 m |

- 길은 언덕을 피해 휘도록 레이아웃에서 그렸고, 시드 언덕은 길에서 (반지름 × 0.6 + 폭) 떨어진다. 남는 굴곡은 깎고 메워서 길 경사 ≤ 10°(검사 상한 15°). 다 만든 땅에서 잰 가장 가파른 곳 **12.9°**(큰길 · 서쪽 갈림목).
- 길이 개울을 건너는 곳(꺾은선 교차 2곳)에 널다리 그레이박스(`Bridges/`)를 놓는다.

## 랜드마크 (`Landmarks`, 표식 `Landmark.<Kind>`, 늘 로드)

| 층 | 랜드마크 | 지도 자리 | 그레이박스 실루엣(`landmark_greybox.py`) | 꼭대기(땅 위) |
|---|---|---|---|---|
| 전역 | `Landmark_Tower` 마법탑 | (1000, 380) | 가는 수직(`tower_greybox`) | 38 m |
| 전역 | `Landmark_GiantTree` 거대 고목 | (1100, 1980) | 굵은 줄기(지름 7 m) · 뿌리 둔덕 · 둥근 잎 덩어리 6(지름 15~32 m) | 47 m |
| 전역 | `Landmark_RockGate` 북쪽 고개 바위 문 | (1000, 3890) | 안으로 기운 바위 기둥 둘(13 × 14 × 40 m) 사이 62 m 틈 + 꼭대기 덩어리 · 바위 | 40 m |
| 구역 | `Landmark_StoneCircle` 선돌 고리 | (560, 3700) | 반지름 14 m 선돌 12(하나 쓰러짐) + 가운데 높은 선돌 · 제단 | 9 m |
| 구역 | `Landmark_Waterfall` 폭포 | (1340, 2880) | 생성기가 찾은 폭포 위 → 아래 끝을 잇는 비스듬한 흰 띠 + 아래 물보라 + 물 볼륨 띠(`WaterVolume_Landmark_Waterfall`, `Water.Falls`) | 위 끝 |
| 구역 | `Landmark_Watchtower` 폐허 감시탑 | (1620, 2120) | 부러진 원통(지름 9 m) — 반쪽만 남은 성가퀴 · 높게 남은 벽 조각 · 떨어진 원통 덩어리 · 돌무더기 | 18 m |
| 구역 | `Landmark_Windmill` 풍차 | (1400, 870) | 돌 몸통 + 원뿔 지붕 + 서쪽(큰길)을 보는 + 자 날개 4 | 21 m |

- 실루엣이 서로 겹치지 않게(가는 수직 · 둥근 덩어리 · 두 기둥 틈 · 고리 · 흰 띠 · 부러진 원통 · 날개). 줄기 · 바위 · 벽은 막힘(`BlockAll`), 잎 · 날개 · 물보라는 통과하지 않는 것도 있다(고목 잎은 막힘 — 올라설 수 있다).
- 풍차 날개는 **아직 돌지 않는다**(선택 항목 — 회전 컴포넌트는 정식 에셋 때).

## POI (`Pois`, 표식 `Poi.<Kind>`, 늘 로드)

40 곳, 묶음 10(묶음마다 4). 자리와 실루엣만 — 놀이 내용은 각 마일스톤이 같은 표식 자리에 채운다. 레이아웃 JSON 의 `Note` 가 무엇이 놓일 자리인지 적는다.

| 종류(Kind) | 수 | 그레이박스 소품 |
|---|---|---|
| `Puzzle` 속성 퍼즐 | 7 | 무너진 벽 ㄱ 자 + 받침 위 **꺼진 모닥불**(`GrimCampfire`, 켜지 않은 채) + **마른 풀**(`GrimDryGrass`) 3 |
| `Resource` 자원 | 10 | 바위 3 + 작은 풀 덩이 7 |
| `Creature` 생물 자리 | 6 | 흙 둔덕 + 굴 입구 + 뼈 + 바위 |
| `Ruin` 폐허 · 보물 | 5 | 높이가 다른 벽 3 + 쓰러진 기둥 + 돌 상자 |
| `Viewpoint` 전망대 | 5 | 넓적 바위 + 깃대(빨간 깃발) |
| `Story` 이야기 흔적 | 4 | 꺼진 모닥불 + 돌 고리 + 상자 · 깔개 |
| `BaseSite` 거점 터 | 2 | 네 귀퉁이 말뚝 + 깃대(평지는 `BaseSites` 가 고른다) |
| `Dungeon` 던전 입구 | 1 | 돌 기둥 둘 + 상인방 + 검은 입구 + 바위 |

| 묶음 | 자리 | POI |
|---|---|---|
| C1 탑 앞 목초지 | 탑 북쪽 300~400 m | 꺼진 화로 제단 · 약초 밭 · 재칼로프 굴 · 길손의 돌무더기 |
| C2 풍차 | 풍차 언덕 둘레 | 밀밭 · 무너진 방앗간 · 재칼로프 굴 무리 · 꽃밭 |
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
| 1. 경사 분포 | 분석 사각형(지도 (350, 300) ~ (1650, 3700) — 가장자리 산등성이 · 강 건너 · 북쪽 산등성이 뺌)의 물 밖 땅, 4 m 격자, 1 m 중앙 차분 경사 | 5~25° ≥ 70%, 0~3° ≤ 30% | **72.8%**, 8.6%(3~5° 10.5%, 25° 넘음 8.0%) |
| 2. 구역 높이 차 | 구역 다각형 안(뒤 구역이 덮은 곳 빼고)의 물 밖 땅 평균, 남 → 북 이웃 차 | ≥ 25 m | 목초지 32.0 → 구릉 60.5(**28.5**) → 고지 121.7(**61.2**) |
| 3. 시선 | 모든 길을 50 m 마다(159 곳), 길 높이(다리 위면 노면) + 눈높이 1.7 m 에서, 랜드마크 꼭대기(거리 한도 1.5~4.5 km) · POI 땅 + 3 m(450 m 안)까지 높이 지도가 시선을 가리는가. 폭포는 띠의 위 · 가운데 · 아래 중 하나 | 곳마다 3 개 이상 | 최소 **3** · 평균 5.1, 모자란 곳 0 |
| 4. 밀도 | 길 위 10 m 마다 가장 가까운 POI 거리 | ≤ 300 m | 가장 먼 곳 **252 m**(탑 문) |
| 길 경사 | 다 만든 땅에서 길 가운데선 4 m 간격(개울 여울 6 m 안은 다리 자리라 뺌) | ≤ 15° | **12.9°**, 길 7.85 km |
| 이름 붙은 시선 | 탑 문 앞(1000, 415) · 고목 언덕 꼭대기(1096, 1976, 뿌리 둔덕 위 눈높이 4.3 m) | 고목 · 풍차 · 선돌 / 폭포 · 감시탑 · 바위 문 | 모두 보임 |

- 그 밖에 `Design.Shape`: 언덕 40 개 이상 · 작은 것 > 큰 것 × 2 · 높이 10~60 m · 지름 150~500 m · 가장 가파른 곳 ≤ 35°, 바위 노두 20 개 이상, 손 언덕 4, 계곡 깊이(둑 − 바닥 평균) 완만한 쪽 · 절벽 쪽 ≥ 30 m, 절벽 쪽 45° 넘는 곳, 폭포 20 m 이상 · 랜드마크 자리 60 m 안, 고개 바닥이 양옆 산등성이보다 30 m 낮음. `Design.Pois`: 랜드마크 7(종류 모두 다름), POI 30~45 · 묶음 8~10(2~4 개씩) · 종류 비율(계획 ±1.5 개) · 물 밖 땅 위.
- **나무 · 소품이 가리는 것은 보지 않는다**(땅만). 나무가 많은 숲 무늬 · 계곡 숲에서는 실제로 덜 보인다.
- 레이아웃을 고치면 숫자가 바뀐다 — 특히 시선은 길을 깎은 자리(절벽 길 · 비탈길)에서 민감하다. `GetVisibleFrom` · `IsMapPointVisible`(에디터 도구)로 모자란 곳에서 보이는 자리를 찾아 POI 를 옮긴다.

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
| 늘 로드 | 탑 · 앞마당 · 표식 · **랜드마크 · POI 그레이박스(354) · 다리** · 하늘 · 기후 · 바다 · 지도 끝 · 시작 자리 · 서식 구역 | 랜드마크는 수 km 밖에서 보여야 하고, POI 소품도 시선 검사 거리(450 m)가 스트리밍 거리에 가깝다 |
| 공간 로드 | 식생 · 연못 · 개울 물 · 연못 볼륨 | 멀면 내린다 |

- 성능 수치(프레임)는 아직 재지 않았다 — 늘 로드 그레이박스가 1차보다 약 350 개 늘었다.

## 머티리얼 (자동, 임시 색)

`M_Terrain_Auto` — 경사(약 28° 부터 흙, 45° 부터 바위) · 높이(90 m 위로 흙 섞임) · 레이어(`Shore` 모래 · `Path` 길 · `Forest` 계곡 숲 바닥) · 풀 출력(`GT_Terrain_Meadow`). 단색 `M_Terrain_Color` + `MI_Terrain_*`(나무 · 덤불 · 바위 · 풀, 그리고 2차의 돌 · 어두운 돌 · 나무판 · 폭포 · 깃발 · 고목 잎 · 고목 껍질 · 돛 · 지붕 · 검정 · 흙 · 뼈 · 풀 덩이), 물 `M_Terrain_Water`. 북부 고지(88 m 위)는 높이 규칙 때문에 흙빛이 섞인다.

## 물 — 단순 평면 + 물리 볼륨

| 물 | 겉모습 | 헤엄 | 태그 |
|---|---|---|---|
| 바다 + 큰 강 | 해수면(0 m) 평면 하나 | `WaterVolume_SeaRiver` | `Water` · `Water.Sea` · `Water.River` |
| 연못 5곳 | 원판 | `WaterVolume_Pond_*` | `Water` · `Water.Pond` · 연못 이름 |
| 개울 3개 | 12 m 조각 평면(물 높이를 따라 기울임) | 없음(얕다) | `Water` · `Water.Creek` · 개울 이름 |
| 폭포 | 흰 띠 · 물보라(그레이박스) | `WaterVolume_Landmark_Waterfall`(띠를 따라) | `Water` · `Water.Falls` |

- 엔진 Water 플러그인을 쓰지 않은 까닭은 1차와 같다(프로젝트에 꺼져 있고, 물 몸체가 높이 지도를 두 번 깎는다).

## 식생 — 생성기 규칙 + 인스턴스드 폴리지 (PCG 아님)

| 규칙 | 모양(엔진 기본 도형) | 밀도(헥타르당: 초원 · 숲 무늬 · 계곡 · 가파른 비탈) | 개수 |
|---|---|---|---|
| Tree | 원통 줄기 + 원뿔 잎, 4~7 m | 2.5 · 60 · 0 · 0 | 7,638 |
| DarkTree | 원통 + 길쭉한 어두운 원뿔, 6~15 m | 0 · 0 · 150 · 0 (계곡만) | 2,735 |
| Bush | 구 | 8 · 25 · 40 · 0 | 7,796 |
| Rock | 납작한 구(묻힘) | 1.5 · 1.5 · 4 · 30(32도 이상) | 3,575 |

- 규칙: 경사 상한 · 높이 범위(나무는 120 m 까지 — 고지 대부분은 나무 없는 바람받이) · 물가 · 길에서 떨어짐 · 탑 평지 + 40 m 비움 · 거점 평지 비움 · **랜드마크 25 m · POI 반지름 + 6 m 비움**. 개수는 레이아웃이 바뀌면 달라진다(위 숫자는 지금 레이아웃).
- PCG 를 쓰지 않은 까닭은 1차와 같다(플러그인 꺼짐 · 생성기만 아는 마스크 · 커맨드렛에서 PCG 생성이 돌지 않음).

## 마법탑

- `tower_greybox.py` 의 `TowerBuilder(origin, yaw, ground)` 가 탑 기준 좌표를 월드로 옮긴다. `build_tower` · `build_yard` 는 L_Tower 와 같은 함수.
- L_OpenBeta: 원점 = 탑 자리, Yaw 0 — 탑 문이 북쪽 큰길을 본다. 탑 평지 45 m(1차 42 m).

## 손으로 다듬을 때

| 다시 만들면 **덮어쓰는** 것 | **안 덮어쓰는** 것 |
|---|---|
| `L_OpenBeta` 전부(맵 · 외부 액터 · HLOD 층) — 손으로 깎은 지형 · 칠한 레이어 · 옮긴 나무 · 더 놓은 액터 · 랜드마크 · POI 소품 위치 | `Content/Grimoire/Terrain` 의 머티리얼 · 인스턴스 · 레이어 정보 · 폴리지 타입 · 풀 타입 |
| `openbeta-preview.png` · `Saved/Terrain/openbeta.r16` | `openbeta_layout.json` · `L_Tower` · 다른 맵 |

- **모양 · 자리를 바꾸려면 JSON 을 고쳐 다시 만든다**(구역 높이 · 언덕 · 계곡 · 길 · 랜드마크 · POI). 고친 뒤 `Grimoire.Terrain.Design` 으로 숫자를 본다.
- 손 조각이 꼭 필요하면 **마지막에 한 번** 한다.
- 다시 만들면 외부 액터 파일 이름이 모두 바뀐다 — 의도한 때만 커밋한다.

## 확인

- **테스트** `Grimoire.Terrain.*`: `Layout.Size` · `Generator.Deterministic`(작은 지형 — 구역 · 손 언덕 · 언덕 군 포함) · `Generator.OpenBeta`(2 × 4 km, 탑 평지 평평, 서 · 북 산등성이 > 목초지, 바닷속, 강바닥 < 해수면, 연못 4~6 둑 > 물 > 바닥, 개울은 아래로만, 거점 평지, 서식 구역 셋) · `Foliage.Rules` · `Design.Shape` · `Design.Paths` · `Design.Sightlines` · `Design.Pois`(위 「설계 검사」) · `Map.OpenBeta`(외부 액터 수 · 표식 태그 — `Landmark.*` 7 · 레이아웃의 `Poi.*` 종류 포함).
- **헤드리스 게임** `check_openbeta.ps1`(-game -nullrhi): 시작 자리(탑 안, 땅 45 m) → 앞마당에서 북쪽 6초 걷기 → 목초지(재칼로프) · 계곡(검은 개) 걷기 → 시간 · 날씨 → 비 → 저지대 연못(`WaterVolume_Pond_Lowland`) 헤엄 → 표식 땅 높이 · 그림 높이(−25 ~ 187 m).
- **스크린숏** `shoot_openbeta.ps1`(-game -RenderOffscreen): 아래 그림들. `Grim.Terrain.ViewFrom` 의 Z 를 `+170` 처럼 주면 그 자리 **랜드스케이프** 위 cm(나무 · 고목 잎은 건너뛴다).
- **콘솔**(개발 빌드): `Grim.Terrain.Probe [지연]` · `Grim.Terrain.Walk <지연> <초> [Yaw]` · `Grim.Terrain.Teleport <X> <Y> [지연]` · `Grim.Terrain.ViewFrom <X> <Y> <Z|+땅 위> <Yaw> <Pitch> [지연] [스크린숏 이름]`.

![탑 문 앞에서 북쪽 — 큰길 · 겹친 구릉 · 지평선의 고목](/art/terrain/openbeta-tower-north.jpg)

![고목 언덕 꼭대기에서 북쪽 — 계곡 절벽 · 폭포 · 고지 · 멀리 바위 문](/art/terrain/openbeta-tree-north.jpg)

![고목 언덕에서 동쪽 — 강 · 감시탑](/art/terrain/openbeta-tree-east.jpg)

![고목 언덕에서 남쪽 — 목초지 · 탑 · 바다](/art/terrain/openbeta-tree-south.jpg)

![고목 언덕에서 서쪽 — 연못 저지대 · 서쪽 산등성이](/art/terrain/openbeta-tree-west.jpg)

![계곡 입구 — 큰길이 계곡 가장자리에 닿는 곳에서 폭포 쪽](/art/terrain/openbeta-valley-entrance.jpg)

![북부 고지에서 고개 바위 문 쪽](/art/terrain/openbeta-highland.jpg)

1차(비교): [남쪽 바다 위에서](/art/terrain/openbeta-aerial-south-v1.jpg) · [앞마당에서 북쪽](/art/terrain/openbeta-yard-north-v1.jpg) · [동쪽 큰 강](/art/terrain/openbeta-river-v1.jpg) · [위에서 본 지도](/art/terrain/openbeta-preview-v1.png)

## 범위 밖 · 다음

- 지형 2차 나머지(M4~M6): POI 자리의 놀이(퍼즐 · 채집 · 생물 · 던전) · 거점 · 밭 · 사냥꾼 야영지 · 강 다리 · 날씨 볼륨 다듬기.
- 지형 3차(M7): 정식 머티리얼 · 식생(PCG) · 조명 · 소품 · 랜드마크 정식 에셋(풍차 회전 포함), 나나이트 랜드스케이프 검토.
- 10 × 10 km: 랜드스케이프를 공간 로드 + HLOD 로, 지역마다 기후 볼륨.
