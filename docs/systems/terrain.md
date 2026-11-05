# 지형 1차 — 오픈 베타 땅 (L_OpenBeta)

> 계획: [로드맵 — 지형은 언제 찍나](./roadmap#지형은-언제-찍나-2026-11-05) 의 지형 1차, 작업 분할 [M3](./m3-plan) 갈래 W. 지역: [세계 — 지역과 크기](../spec/world#지역과-크기-r34-—-q36-확정) 1번(마법탑 · 온난 지대).
> 코드: `Source/Grimoire/World/`. 스크립트: `Scripts/Terrain/`. 에셋: `Content/Grimoire/Terrain/`. 맵: `Content/Grimoire/Maps/L_OpenBeta`(World Partition). 기본 맵은 아직 `L_Tower` — 통합 때 바꾼다.

![L_OpenBeta 를 남쪽 바다 위에서 — 탑 언덕 · 초원 · 연못 · 길 · 동쪽 큰 강 · 산등성이](/art/terrain/openbeta-aerial-south.jpg)

## 한눈에

| 조각 | 파일 | 하는 일 |
|---|---|---|
| 레이아웃(수치) | `Scripts/Terrain/openbeta_layout.json` | 시드 · 크기 · 노이즈 · 탑 언덕 · 계곡 · 강 · 개울 · 연못 · 길 · 거점 평지 · 표식 · 식생 밀도. 구조체 `FGrimTerrainLayout` 의 JSON — **지형을 바꾸려면 여기를 고치고 다시 만든다** |
| 높이 지도 생성기 | `FGrimTerrainGenerator`(`World/GrimTerrainGenerator.*`) | 순수 계산(월드 · 에셋 모름). 높이(m) · 물가 · 길 · 계곡 숲 무게 · 물과 길까지 거리 · 물 목록 · 식생 자리. 830만 정점을 1초 안팎 |
| 에디터 도구 | `UGrimTerrainEditorLibrary`(`World/GrimTerrainEditorLibrary.*`) | 파이썬이 부르는 C++: 레이아웃 읽기 · 생성 캐시 · 땅 높이 · 랜드스케이프 가져오기(World Partition 격자) · 레이어 정보 · 폴리지 타입 · 식생 변환 · 브러시 볼륨 · World Partition 격자 · 미리보기 그림 · r16 내보내기 |
| 표식 | `AGrimTerrainMarker`(`World/GrimTerrainMarker.*`) · 태그 `GrimTerrainTags` | 자리만 표시하는 액터 — GameplayTag `MarkerTag` + 반지름. `FindMarkers(월드, 태그)` 로 찾는다 |
| 맵 만들기 | `Scripts/Terrain/build_openbeta.py`(+ `terrain_assets.py`) | 맵을 지우고 처음부터: 랜드스케이프 → 물 → 식생 → 표식 → 마법탑 → 하늘 · 기후 · 지도 끝 → World Partition 설정 → 저장 |
| 마법탑 | `Scripts/Tutorial/tower_greybox.py` | L_Tower 와 **같은 함수**로 탑 그레이박스를 짓는다(`build_tower_map.py` 도 이것을 쓴다) |
| 확인 | `Scripts/Terrain/check_openbeta.ps1` · 콘솔 `Grim.Terrain.*` · 테스트 `Grimoire.Terrain.*` | 아래 「확인」 |

## 만드는 법 · 다시 만드는 법

```powershell
# 1) C++ 빌드(에디터를 닫고) — CLAUDE.md 의 Build.bat
# 2) 맵 만들기(헤드리스, 창 없음). -nullrhi 가 아니라 -AllowCommandletRendering — 아래 「RHI 가 필요한 까닭」
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -run=pythonscript -script="$PWD\Scripts\Terrain\build_openbeta.py" -unattended -nop4 -nosplash -AllowCommandletRendering
# 3) 확인
powershell -File Scripts\Terrain\check_openbeta.ps1          # 끝에 PASS
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
  -ExecCmds="Automation RunTests Grimoire.Terrain; Quit" -unattended -nop4 -nosplash -nullrhi -log
```

- 첫 실행은 셰이더 때문에 3분쯤, 그 뒤는 1분 안팎. 끝에 로그 `build_openbeta: 개수 …` · `saved /Game/Grimoire/Maps/L_OpenBeta`.
- **같은 레이아웃 · 같은 시드면 같은 땅**: 높이 · 무게 · 물 높이 · 식생 자리가 모두 시드로 정해진다(생성기는 정점마다 독립이라 병렬로 돌려도 같다 — 테스트 `Grimoire.Terrain.Generator.Deterministic`). 다만 액터 GUID 는 매번 새로 나서 `__ExternalActors__` 파일 이름이 바뀐다 — 다시 만들면 git 에는 파일 수백 개가 지워지고 새로 생긴다.
- 환경 변수 `GRIM_TERRAIN_REFRESH_ASSETS=1` 이면 `Content/Grimoire/Terrain` 의 머티리얼 · 폴리지 타입 · 레이어 정보 · 풀 타입도 지우고 새로 만든다(기본은 있으면 그대로 — 손으로 고친 색을 지킨다).
- 부산물: `docs/public/art/terrain/openbeta-preview.png`(위에서 본 지도 그림, 북쪽이 위), `Saved/Terrain/openbeta.r16`(16비트 높이 지도, 랜드스케이프 순서 4065 × 2033 — 에디터 「랜드스케이프 가져오기」에 바로 넣을 수 있다).

### RHI 가 필요한 까닭

`-nullrhi` 로 만들면 땅이 **보이지 않는다**(충돌은 맞아서 걷기 · 헤엄은 된다). 랜드스케이프는 편집 레이어 시스템이 GPU 로 레이어를 합쳐 최종 높이 지도 텍스처를 만드는데, RHI 가 없으면 그 텍스처가 0(= −256 m)으로 저장된다 — 로그에 `had incorrect bounds … new CachedLocalBox … Z=-257` 이 찍힌다. `-AllowCommandletRendering` 이 헤드리스 커맨드렛에 RHI(D3D12)를 켜서 이를 막는다(창은 뜨지 않는다). 그래서 `build_openbeta.py` 는 `-nullrhi` 면 바로 멈추고, `CreateLandscape` 도 렌더링이 꺼져 있으면 만들지 않는다. 확인 스크립트는 「그림 높이」(랜드스케이프 구성 요소의 렌더 범위)가 −50 m 위에서 시작해 +50 m 넘게 올라가는지 본다.

## 지도 · 좌표

![위에서 본 높이 지도(생성기 미리보기, 북쪽이 위)](/art/terrain/openbeta-preview.png)

- **지도 좌표**(레이아웃 JSON): 미터, 남서 모서리가 (0, 0), X = 동쪽, Y = 북쪽. 크기 2032 × 4064 m.
- **월드 좌표**(cm): 원점 = 마법탑 가운데(지도 (1000, 380)), **월드 +X = 북쪽, +Y = 동쪽**. 하늘(`AGrimSkyController` 해돋이 Yaw 90 = +Y)과 언리얼 위에서 보기(위 = +X, 오른쪽 = +Y)에 맞춘 것이다. 바꾸는 식: 월드 X = (지도 Y − 380) × 100, 월드 Y = (지도 X − 1000) × 100.
- 랜드스케이프도 월드 축을 따른다 — 로컬 X(4 km) = 북쪽, Y(2 km) = 동쪽. 생성기는 지도 순서로 계산하고 가져올 때 바꿔 놓는다.

| 지역 | 지도 좌표(m) | 모습 |
|---|---|---|
| 남쪽 바다 · 해안 | Y < 230 ± 45 | 모래밭(폭 120 m)으로 내려가 바닷속 −25 m. 탑 앞은 곶이 남쪽으로 튀어나와 절벽(폭 55 m) |
| 탑 언덕 | 가운데 (1000, 380), 꼭대기 평지 반지름 45 m, 비탈 230 m | 꼭대기 42 m 완전한 평지. 남쪽은 바다 절벽, 북쪽은 초원으로 완만히 |
| 초원 | 가운데 | 22 m 에서 북쪽으로 16 m 오름 + 노이즈(9 m · 380 m) + 구릉(4 m · 150 m) + 잔결 |
| 작은 계곡 | (520, 3780) → (1720, 2760) 꺾은선, 폭 220 m · 깊이 16 m | 북서에서 남동으로 큰 강까지. 바닥에 개울, 어두운 숲(계곡 숲 바닥 레이어 · DarkTree) |
| 개울 2개 | `Creek_Valley`(계곡 따라 큰 강으로), `Creek_Meadow`(초원 연못 → 큰 강) | 폭 4~5 m, 물은 둘레 땅보다 0.8~0.9 m 아래, 하류로 내려가기만 한다 |
| 연못 4곳 | `Pond_TowerWest`(700, 520) r38 · `Pond_Meadow`(1180, 1350) r45 · `Pond_Reed`(760, 2000) r32 · `Pond_Upper`(1250, 3450) r36 | 둘레 땅 높이에서 물 높이를 정하고 대접 모양(깊이 2.5~3.5 m)으로 판다. 둑은 물보다 높다 |
| 큰 강 | 동쪽, 북쪽 끝 → 남동 바다, 폭 70 m · 깊이 5 m | 물 = 해수면(바다와 이어짐). 양쪽 둑 70 m |
| 가장자리 산등성이 | 서 230 m · 110 m, 북 280 m · 140 m, 동(강 건너) 150 m · 75 m | 지도 끝을 자연스럽게 막는다. 폭이 ±50% 들쭉날쭉 |
| 지도 끝 | 네 변 | 높이 1 km 막는 볼륨(`MapEdge_*`) — 바다로 헤엄쳐 나가지 못하게 |

## 표식 (AGrimTerrainMarker, 늘 로드)

액터 태그에도 같은 이름(`Habitat.Kelpie` 등)과 표식 이름이 들어 있다. Z 는 땅 + 50 cm.

| 표식 | 태그 | 월드 (X, Y) cm | 땅 높이 | 반지름 |
|---|---|---|---|---|
| `Site_Tower` | `Terrain.Site.Tower` | (0, 0) | 42.0 m | 45 m |
| `Site_TowerYard` | `Terrain.Site.TowerYard` | (2800, 0) | 42.0 m | 20 m |
| `Habitat_Jackalope` | `Habitat.Jackalope`(초원) | (87000, −18000) | 24.2 m | 200 m |
| `Habitat_Kelpie` | `Habitat.Kelpie`(초원 연못 · 개울 어귀) | (95000, 25000) | 23.8 m | 150 m |
| `Habitat_BlackDog` | `Habitat.BlackDog`(계곡 어두운 숲) | (283000, −10000) | 19.4 m | 230 m |
| `Base_EastMeadow` | `Terrain.Site.Base` | (52000, 35000) | 22.4 m | 28 m |
| `Base_WestMeadow` | `Terrain.Site.Base` | (117000, −36000) | 27.5 m | 28 m |
| `Base_Creekside` | `Terrain.Site.Base` | (187000, 33000) | 28.9 m | 28 m |
| `Base_NorthMeadow` | `Terrain.Site.Base` | (310000, 48000) | 36.8 m | 28 m |
| `Path_TowerToValley_00` ~ `_09` | `Terrain.Path` | (1800, 0) → (277000, −8000) | 탑 문 → 초원 → 계곡 | 2 m |

- 생물 갈래의 서식 볼륨 · 스포너는 **통합 때** `Habitat.*` 표식 자리에 놓는다(`AGrimTerrainMarker::FindMarkers(World, Habitat 태그)`).
- 거점 평지는 생성기가 둘레를 평평하게 고른 자리다(반지름 안 높이 차 < 5 cm — 테스트).
- 기후 볼륨 `Climate_Meadow` 하나가 지도 전체(+100 m)를 덮는다.

## 랜드스케이프 크기 (계산 근거)

| 값 | 고른 것 | 까닭 |
|---|---|---|
| 섹션 | 127칸(정점 128 = 2⁷) | 섹션이 가질 수 있는 가장 큰 크기. 섹션 하나가 LOD 단위 — 128 정점이면 LOD 0~7 |
| 구성 요소 | 2 × 2 섹션 = 254칸 | 구성 요소 수를 줄여 CPU · 드로 콜을 아낀다(구성 요소 하나 = 드로 콜 한 묶음). 무게 지도는 구성 요소당 256² 한 장(2의 거듭제곱 조건) |
| 구성 요소 수 | 8(동서) × 16(남북) = 128 | 1 m 칸으로 2032 × 4064 m — 2 × 4 km 에 가장 가까운 권장 크기(에픽 권장표의 2033 × 2033 = 127칸 · 2×2 · 8×8 을 남북으로 두 배). 128개는 권장 한도(약 1024)보다 훨씬 적다 |
| 정점 | 2033 × 4065 (830만) | 16비트 높이 지도 약 16 MB |
| 칸 크기 | 1 m(배율 100) | 걷는 눈높이에서 기복이 보이는 해상도. 더 성기면 개울 · 길이 뭉개진다 |
| Z 배율 | 100 | 높이 범위 ±256 m, 해상도 1/128 m. 이 땅은 −25 ~ 180 m |
| LOD | 엔진 기본(화면 크기 기준 LOD, 섹션별) · 나나이트 랜드스케이프 끔 | 그레이박스라 기본으로 충분. 나나이트는 빌드 단계와 디스크가 늘어 지형 3차(M7)에서 다시 본다 |

## World Partition · 성능

| 설정 | 값 | 까닭 |
|---|---|---|
| 런타임 해시 | RuntimeHashSet, 메인 파티션 LH 격자 | 5.8 새 맵 기본 |
| 셀 크기 | 256 m(25600 cm) | 폴리지 파티션 격자(`InstancedFoliageGridSize` 기본 25600)와 같게 — 폴리지 액터 하나가 셀 하나에 들어간다 |
| 스트리밍 거리 | 512 m(51200 cm) | 나무가 사라지는 거리(300~400 m)보다 멀어야 셀 경계에서 나무가 뚝 끊기지 않는다. 덤불 120 m · 바위 200 m |
| 랜드스케이프 프록시 | 2 × 2 구성 요소(508 m) = 32개, **늘 로드** | 땅에 구멍이 나지 않고(HLOD 빌드 없이도 탑에서 멀리 보인다), 2 × 4 km 높이 · 무게 텍스처는 100 MB 안팎이라 늘 둬도 된다. 10 × 10 km 로 넓힐 때 공간 로드 + 랜드스케이프 HLOD 로 바꾼다 |
| 늘 로드 | 탑 · 앞마당(114개) · 표식 · 하늘 · 기후 · 바다 평면 · 바다 볼륨 · 지도 끝 · 시작 자리 | 시작 자리는 플레이어보다 먼저 있어야 하고, L_Tower 에서 온 튜토리얼 액터는 셀이 내렸다 올라오는 것(EndPlay → BeginPlay)을 겪어 본 적이 없다 — 시험할 때 `AGrimNoCombatVolume` 이 두 번째 BeginPlay 에서 ensure 를 냈다 |
| 공간 로드 | 식생(폴리지 액터 142개) · 연못 · 개울 물 · 연못 볼륨 | 멀면 내린다 |

- 성능 수치(프레임)는 아직 재지 않았다 — 통합 뒤 PIE 2인으로 잰다([로드맵 리스크](./roadmap#기술-리스크)).

## 머티리얼 (자동, 임시 색)

`M_Terrain_Auto` — 파라미터로 색 · 문턱을 바꾼다(에디터에서 머티리얼을 열거나 인스턴스를 만들어).

- **경사**(1 − 법선 Z): 약 28도부터 흙(`DirtSlopeStart` 0.12), 약 45도부터 바위(`RockSlopeStart` 0.30).
- **높이**: 90 m 위로 흙이 섞인다(`HighStart` 9000 cm).
- **레이어**(생성기가 칠한다, 무게 섞지 않음 — 레이어마다 0~1 따로): `Shore` → 모래(물가 · 물속 · 해변), `Path` → 길, `Forest` → 계곡 숲 바닥(어둡게). 손으로 더 칠해도 된다(다시 만들면 덮인다).
- **풀**: 머티리얼 풀 출력(`Meadow`) = 바위 · 흙 · 모래 · 길 · 계곡 숲이 아닌 곳 → 랜드스케이프 풀 타입 `GT_Terrain_Meadow`(작은 원뿔, 런타임에 GPU 가 뿌린다 — 저장되는 인스턴스가 없다).
- 단색 `M_Terrain_Color` + `MI_Terrain_*`(나무 줄기 · 잎 · 어두운 잎 · 덤불 · 바위 · 풀), 물 `M_Terrain_Water`(반투명).

## 물 — 단순 평면 + 물리 볼륨

| 물 | 겉모습 | 헤엄 | 태그 |
|---|---|---|---|
| 바다 + 큰 강 | 해수면(0 m) 평면 하나가 지도 전체 + 2 km 를 덮는다 — 땅이 0 m 아래인 곳(바다 · 강바닥)만 보인다 | `WaterVolume_SeaRiver`(물리 볼륨 `bWaterVolume`, −40 ~ 0 m, 지도 전체) | `Water` · `Water.Sea` · `Water.River` |
| 연못 4곳 | 원판(원통을 납작하게), 반지름 = 물가선 + 둑 30% | `WaterVolume_Pond_*`(바닥 ~ 물 높이) | `Water` · `Water.Pond` · 연못 이름 |
| 개울 2개 | 12 m 조각 평면(물 높이를 따라 기울임) | 없음(얕다 — 젖음은 통합 때 태그로) | `Water` · `Water.Creek` · 개울 이름 |

- **엔진 Water 플러그인을 쓰지 않은 까닭**: 프로젝트에 켜져 있지 않고(켜려면 공용 `.uproject` 를 고친다), 실험 기능이며, 물 몸체가 랜드스케이프를 편집 레이어 브러시로 깎아 생성기의 높이 지도와 두 번 깎인다. 물 높이는 생성기가 이미 안다 — 평면 + 엔진 표준 물리 볼륨이 가장 쉽고, 헤엄은 캐릭터 이동의 `MOVE_Swimming` 으로 확인했다.
- 헤엄 · 젖음 · 켈피의 「물 위 달리기」는 `Water.*` 태그 · 물리 볼륨으로 찾는다.

## 식생 — 생성기 규칙 + 인스턴스드 폴리지 (PCG 아님)

| 규칙 | 모양(엔진 기본 도형) | 밀도(헥타르당: 초원 · 숲 무늬 · 계곡 · 가파른 비탈) | 개수 |
|---|---|---|---|
| Tree | 원통 줄기 + 원뿔 잎, 4~7 m | 2.5 · 70 · 0 · 0 | 11,564 |
| DarkTree | 원통 + 길쭉한 어두운 원뿔, 6~15 m | 0 · 0 · 150 · 0 (계곡만) | 3,764 |
| Bush | 구 | 8 · 25 · 40 · 0 | 8,561 |
| Rock | 납작한 구(묻힘) | 1.5 · 1.5 · 4 · 30(32도 이상) | 3,512 |

- 규칙: 경사 상한 · 높이 범위 · 물가에서 떨어짐 · 길에서 떨어짐 · 탑 평지 + 40 m 비움 · 거점 평지 비움. 숲 무늬 = 노이즈(340 m)가 문턱 0.18 을 넘는 곳. 지터 격자 + 칸마다 고정 시드 → 다시 만들어도 같은 자리. 밀도 · 규칙은 JSON `Foliage`.
- 나무 줄기 · 바위는 막힘(`BlockAll`), 잎 · 덤불은 통과. 사라지는 거리: 나무 300~400 m, 덤불 90~120 m, 바위 150~200 m.
- **PCG 를 쓰지 않은 까닭**: ① PCG 플러그인이 프로젝트에 켜져 있지 않다(공용 `.uproject`). ② 규칙이 생성기만 아는 마스크(물 · 길까지 거리, 계곡)를 쓴다 — PCG 가 보려면 텍스처 · 스플라인으로 따로 내보내야 한다. ③ 헤드리스 커맨드렛은 월드가 틱하지 않아 PCG 생성이 돌지 않는다. 대안으로 허락된 「인스턴스드 폴리지」를 C++ 계산 + `InstancedFoliageActor.add_instances`(World Partition 격자로 나뉨)로 했다. 정식 식생(지형 3차, M7)에서 PCG 로 옮길 때 이 규칙을 그대로 PCG 그래프로 옮기면 된다.

## 마법탑

- `tower_greybox.py` 의 `TowerBuilder(origin, yaw, ground)` 가 탑 기준 좌표를 월드로 옮긴다. `build_tower(b)` = 탑 1층 · 방 · NPC · 시험의 방, `build_yard(b)` = 앞마당 · 초원 소품 · 쉼터 · 거점 자리 · 사냥꾼 거점 · 나무, `build_ground_and_sky(b)` = L_Tower 전용 땅 · 하늘 · 기후.
- `build_tower_map.py` 는 이 함수들을 같은 순서로 부를 뿐 — **L_Tower 결과는 그대로다**(바꾸기 전 · 뒤에 L_Tower 를 만들어 액터 117개의 이름 · 클래스 · 위치 · 회전 · 크기 · 폴더 · 태그 · 상자 크기 · 메시를 비교해 같음을 확인).
- L_OpenBeta: 원점 = 탑 자리, Yaw 0 — 탑 문(탑 기준 +X)이 북쪽 초원과 길을 본다. 높이는 지형 높이(탑 평지 42 m, 밖 소품은 그 자리 땅 높이). 움직이는 표적의 진폭(월드 벡터)도 Yaw 에 맞춰 돌린다.
- 튜토리얼 자동 시작은 아직 `AutoStartMaps=L_Tower` 만 — L_OpenBeta 를 기본 맵으로 바꿀 때 함께 넣는다(통합).

## 손으로 다듬을 때

| 다시 만들면 **덮어쓰는** 것 | **안 덮어쓰는** 것 |
|---|---|
| `L_OpenBeta` 전부 — 맵 · `__ExternalActors__` · `__ExternalObjects__` · HLOD 층 에셋. 손으로 깎은 지형 · 칠한 레이어 · 옮긴 나무 · 더 놓은 액터가 모두 사라진다 | `Content/Grimoire/Terrain` 의 머티리얼 · 인스턴스 · 레이어 정보 · 폴리지 타입 · 풀 타입(환경 변수 `GRIM_TERRAIN_REFRESH_ASSETS=1` 일 때만 새로) |
| `docs/public/art/terrain/openbeta-preview.png` · `Saved/Terrain/openbeta.r16` | `openbeta_layout.json` · `L_Tower` · 다른 맵 |

- **모양을 바꾸려면 JSON 을 고쳐 다시 만든다**(연못 자리 · 강 굽이 · 계곡 깊이 · 밀도). 몇 분이면 끝나고 테스트가 규칙(평지 · 둑 · 물 흐름)을 지켜 준다.
- 손 조각이 꼭 필요하면 **마지막에 한 번** 한다 — 그 뒤로는 이 스크립트를 돌리지 않거나, 손 작업 전에 `openbeta.r16` 을 받아 두고 고친 높이 지도를 다시 넣는다(이때 표식 · 물 높이는 생성기 값이라 직접 맞춘다).
- 색 · 문턱(머티리얼 파라미터), 폴리지 타입의 거리 · 그림자는 언제 고쳐도 남는다.
- 다시 만들면 외부 액터 파일 이름이 모두 바뀐다(수백 개 삭제 + 추가) — 의도한 때만 커밋한다.

## 확인

- **테스트** `Grimoire.Terrain.*`: `Layout.Size`(크기 · 높이 값 변환 · 좌표 축) · `Generator.Deterministic`(작은 지형 — 같은 시드 같은 결과, 탑 평지 · 연못) · `Generator.OpenBeta`(실제 JSON: 2 × 4 km, 탑 평지 평평, 서 · 북 산등성이 > 초원, 남쪽 바닷속, 강바닥 < 해수면, 연못 둑 > 물 > 바닥, 개울은 아래로만, 거점 평지, 서식 구역 셋) · `Foliage.Rules`(규칙마다 뿌려지고 물 · 길 · 높이 · 탑 규칙을 지킴) · `Map.OpenBeta`(에셋 레지스트리로 맵을 열지 않고 외부 액터를 센다 — 랜드스케이프 1 · 프록시 · 폴리지 · 물 볼륨 · 시작 자리 1 · 기후 1 · 하늘 1 · 탑 스승 1 · 표식 태그 7종).
- **헤드리스 게임** `check_openbeta.ps1`(-game -nullrhi, L_OpenBeta): 시작 자리(탑 안, 땅 42 m) → 앞마당에서 북쪽으로 6초 걷기(33 m, 떨어지지 않음) → 초원 · 계곡 걷기 → 시간이 흐름 · 지역 Meadow 날씨 → 비 → 연못 속 `MOVE_Swimming` · `WaterVolume_Pond_Meadow` → 표식 19개 땅 높이 · 랜드스케이프 그림 높이(−25 ~ 180 m).
- **콘솔**(개발 빌드): `Grim.Terrain.Probe [지연]`(플레이어 · 표식 땅 높이, 프록시 수, 그림 높이, 시간, 날씨) · `Grim.Terrain.Walk <지연> <초> [Yaw]` · `Grim.Terrain.Teleport <X> <Y> [지연]` · `Grim.Terrain.ViewFrom <X> <Y> <Z> <Yaw> <Pitch> [지연] [스크린숏 이름]`(Saved/TerrainCheck 에 PNG — RHI 를 켠 `-game -RenderOffscreen` 에서).

![탑 앞마당에서 북쪽 — 길 · 초원 나무 · 덤불 · 바위 · 풀](/art/terrain/openbeta-yard-north.jpg)

![동쪽 큰 강과 초원 개울 어귀, 강 건너 산등성이](/art/terrain/openbeta-river.jpg)

## 범위 밖 · 다음

- 지형 2차(M4~M6): 거점 자리 · 밭 · 사냥꾼 야영지 · 던전 입구 · 다리 · 여울, 날씨 볼륨 다듬기.
- 지형 3차(M7): 정식 머티리얼 · 식생(PCG) · 조명 · 소품, 나나이트 랜드스케이프 검토.
- 10 × 10 km: 랜드스케이프를 공간 로드 + HLOD 로, 지역마다 기후 볼륨.
