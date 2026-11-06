# 커스터마이징 — 기술 설계

> 스펙: [캐릭터 커스터마이징](../spec/customization) (R39). 코드: `Source/Grimoire/Appearance/`. 데이터: `Content/Grimoire/Data/Appearance/*.csv`.
>
> 상태: v1 · 2026-11-06 — 데이터 모델 · 조립 · NPC 생성 · 저장 · 복제 · 최소 생성 화면 · 치트 구현(아래 산정 표 C1~C7). 아트는 자리 표시(엔진 기본 도형 + UE 마네킹) — `AppearanceParts.csv` 의 `MeshA` · `MeshB` 는 모두 비어 있고 소품은 `/Engine/BasicShapes/*` 다.

## 기술 선택

### 후보 비교 (UE 5.8)

| 방식 | 하는 일 | 우리 파이프라인(GPT → Tripo → 블렌더)과 | 성능 | 텍스트로 다룰 수 있나* | 판단 |
|---|---|---|---|---|---|
| **모듈식 스켈레탈 조각 + Leader Pose** | 같은 스켈레톤에 스킨된 조각 메시들이 리더 메시의 뼈를 그대로 따른다 | ✅ 조각 하나 = 블렌더에서 마네킹 스켈레톤에 가중치를 옮긴 메시 하나. Tripo 결과를 그대로 받는다 | 조각마다 드로우 콜 · 스키닝은 하지만 애님 평가는 한 번 | ✅ 조각 목록 · 색이 CSV | **채택** |
| Copy Pose From Mesh | 다른 스켈레톤의 포즈를 복사 | 스켈레톤이 다를 때만 필요 | Leader Pose 보다 비쌈(조각마다 포즈 복사) | ✅ | 필요 없음 — 모든 조각을 한 스켈레톤에 맞춘다 |
| Skeletal Mesh Merge (런타임 합치기) | 조각들을 한 메시로 굽는다 | ✅ 위와 같은 조각을 그대로 쓴다 | ✅ 드로우 콜 · 컴포넌트 최소. 합치는 순간 비용 · 메모리(인스턴스마다 메시) | ✅ | **나중에 군중 NPC 용으로 추가**(조립 코드 뒤에 숨긴다) |
| Mutable (Customizable Object) | 그래프로 메시 · 텍스처를 합쳐 하나로 구움 | △ 강력하지만 조각 · 규칙이 에디터 그래프(바이너리 .uasset)에 산다 — 블루프린트처럼 사람이 에디터에서만 고친다 | ✅ 런타임 결과가 한 메시 · 합친 텍스처 | ✗ | 보류 — 조각이 수백 개가 되거나 텍스처 합치기가 꼭 필요할 때 다시 본다 |
| 모프 타깃 | 정점 이동으로 얼굴 · 몸 모양 변화 | ✅ 블렌더 셰이프 키를 스크립트로 만든다 | 활성 모프만큼 GPU 비용(작음) | ✅ 슬라이더 표 | **얼굴 슬라이더에 채택** |
| MetaHuman | 사실적인 얼굴 · 몸 · 그룸 | ✗ 사실적 비율 · 자체 스켈레톤 체계 — 페인터리 스타일라이즈드 · Tripo 옷과 맞추기 어렵다 | ✗ 얼굴 리그 · 그룸 · LOD 가 무겁다(8인 멀티 + NPC) | ✗ | 미채택 |

\* 이 프로젝트는 `.uasset` 을 텍스트로 고치지 않는다(CLAUDE.md). 데이터가 CSV · ini 에 있으면 코드 세션이 직접 더하고 고칠 수 있다.

### 결정

1. **공용 스켈레톤 = UE 마네킹(`SK_Mannequin`)**. 지금 애님(`ABP_GrimPlayer` · 마네킹 애님)을 그대로 쓰고, 체형 A/B 는 마네킹 Manny/Quinn 비율에서 시작한다.
2. **옷 · 몸 · 머리 · 얼굴 = 같은 스켈레톤의 스켈레탈 조각, 리더 = 캐릭터 메시.** 조각 컴포넌트는 `SetLeaderPoseComponent` + `bUseBoundsFromLeaderPoseComponent`.
3. **단단한 소품(모자 · 고리 · 주머니 · 책 · 버클) = 뼈에 붙는 스태틱 메시.** 스키닝이 없어 싸고, Tripo 결과를 그대로 쓴다(Nanite 는 끔 — 작은 소품).
4. **얼굴 = 체형별 머리 메시 하나 + 모프 타깃.** 얼굴 프리셋은 메시가 아니라 슬라이더 값 묶음이다. 머리 메시는 블렌더에서 한 번 다듬고(Tripo 머리 → 리토폴로지), 슬라이더마다 셰이프 키를 스크립트로 만든다.
5. **색 = 재질 파라미터.** 모든 조각 재질이 같은 이름의 벡터 파라미터(`Primary` · `Secondary` · `Leather` · `Metal` · `Lining` · `Skin` · `Hair` · `Eye` · `Under`(바지 · 속옷 층, 고정 색) — 코드 `EGrimAppearanceChannel`)를 읽는다. 마스터 재질 `M_GrimCharacter_Master` 하나에서 인스턴스를 만든다(아트 작업 항목 A1 — 아직 없음). 지금 몸은 마네킹 재질의 `Paint Tint`(설정 `BodyTintParameter`), 자리 표시 소품은 `Color`(설정 `PlaceholderColorParameter`)로 칠한다. 서클 고리는 스톨의 고리 소켓에 스태틱 고리를 서클 수만큼 붙인다(재질 `RingCount` 파라미터도 함께 준다 — 수놓은 고리로 바꿀 때를 위해).
6. **군중 NPC(10명 넘게 한 화면)** 는 같은 조각을 Skeletal Mesh Merge 로 굽는 길을 나중에 붙인다 — 조립 컴포넌트의 출력만 바뀐다.

### 성능 예산 (제안 — 측정 뒤 고친다)

| | 스켈레탈 조각 | 스태틱 소품 | 비고 |
|---|---|---|---|
| 플레이어(최대 8) | ≤ 8 | ≤ 10 | 서클 고리 10개 포함 |
| 이름 있는 NPC | ≤ 7 | ≤ 8 | |
| 군중 NPC | 1(합친 메시) | ≤ 3 | Merge 길이 생기면 |

- 작은 소품(고리 · 단추)은 25m 밖에서 숨긴다(설정 `SmallPieceCullDistance`).
- 전용 서버(`NM_DedicatedServer`)는 조각을 만들지 않는다 — 외형 값만 갖고 복제한다.
- 조각 메시 · 재질은 소프트 참조로 두고 필요한 것만 읽는다.

## 데이터 모델

### 외형 구조체 `FGrimAppearance` (복제 · 저장되는 것 전부)

| 필드 | 형식 | 뜻 |
|---|---|---|
| `BodyType` | uint8 | 0 = 체형 A, 1 = 체형 B |
| `Height` | uint8 | 128 = 기준 키, 0~255 → 설정 `HeightRange`(±6%) |
| `Parts` | FName × 칸 수 | 칸(`EGrimAppearanceSlot`)마다 조각 ID. None = 비움 |
| `Morphs` | uint8 × 슬라이더 수 | 128 = 0. 표 `AppearanceMorphs.csv` 순서 |
| `SkinColor` · `EyeColor` · `HairColor` · `ClothPrimary` · `ClothSecondary` | uint8 | 팔레트 칸 번호 |
| `Element` | uint8 | 안감 · 완드 보석 속성(0 없음 · 1 물 · 2 불 · 3 바람 · 4 땅 · 5 빛 · 6 어둠) |
| `Circle` | uint8 | 스톨 고리 수(0~10). 플레이어는 서버가 서클 어트리뷰트에서 채운다 |
| `Rule` · `Seed` | FName · int32 | NPC 생성 규칙과 씨앗(플레이어 = 없음 · 0). 디버그 · 다시 굴리기용 |

- **복제(`NetSerialize`)**: 조각 = 카탈로그 정렬 번호 uint16(0 = 비움), 나머지 uint8, 규칙 = 번호, 씨앗 int32. 대략 40바이트. 카탈로그는 서버 · 클라이언트가 같은 CSV 에서 같은 순서(ID 정렬)로 만든다.
- **저장(`Serialize`)**: 조각 · 슬라이더 · 규칙은 **이름**으로, 버전 바이트를 앞에 둔다. 읽은 뒤 `Sanitize` 로 없어진 이름 · 범위 밖 값을 고친다.
- `GetHash()` = 저장 형식 바이트의 CRC — 서버 · 클라이언트가 같은 외형을 보는지 비교할 때(넷 확인 스크립트).

### 카탈로그 — CSV 표 (`Content/Grimoire/Data/Appearance/`)

| 파일 | 행 = | 주요 칸 |
|---|---|---|
| `AppearanceParts.csv` | 조각 하나 | `Slot` · `DisplayName` · `BodyTypes`(A\|B) · `MeshA` · `MeshB`(스켈레탈, 비우면 소품만) · `Creator`(플레이어가 고를 수 있나) · `HidesHair` · `Morphs`(얼굴 조각의 기본 슬라이더 값 `Id=값\|…`) |
| `AppearancePieces.csv` | 조각에 붙는 스태틱 소품 하나 | `Part` · `Mesh` · `Bone` · `Offset`(마네킹 기준 자세의 그 뼈 위치에서, 메시 공간 cm) · `Rotation` · `Scale` · `Channel`(색 칸) · `Repeat`(`Circle` = 서클 수만큼) · `Step`(반복 간격) · `Face`(얼굴 슬라이더 자리 표시를 받나) · `Small`(멀리서 숨기는 작은 소품) · `BodyTypes`(비우면 두 체형) |
| `AppearancePalettes.csv` | 색 하나 | `Palette`(Skin · Eye · Hair · Cloth · Element · Fixed) · `Hex` · `Creator` |
| `AppearanceMorphs.csv` | 얼굴 슬라이더 하나 | `DisplayName` · `MorphTarget`(머리 메시의 모프 이름) · `PlaceholderScale`(자리 표시 머리에 주는 크기 변화) |
| `NpcAppearanceRules.csv` | 역할 규칙 하나 | `Player`(플레이어 규칙인가), 칸마다 후보 `조각:가중치\|…`, `BodyA` · `BodyB` 가중치, 팔레트 후보, `CircleMin/Max`, `Elements`, `GreyHair`(새치 확률) · `GreyHairColor`, `MorphJitter` · `HeightJitter`. 지금 규칙: `Player_Start` · `Tower_Apprentice` · `Tower_Instructor` · `Tower_Librarian` · `Tower_Examiner` |
| `NpcAppearance.csv` | 이름 있는 NPC 하나 | `Rule` · `Seed`(0 = NPC 이름 해시) · `Circle` · `Element`(0 = 규칙대로). 지금은 `Master`(마렌) = `Tower_Instructor` 하나. 표에 없는 탑 NPC 는 설정 `DefaultTowerNpcRule`(`Tower_Apprentice`) |

- 표는 런타임에 읽는다(다른 시스템 CSV 와 같은 방식). 콘솔 `Grim.Appearance.Reload` 로 다시 읽고 월드의 외형을 다시 조립한다. 경로는 `[/Script/Grimoire.GrimAppearanceSettings]` 의 `PartsCSV` · `PiecesCSV` · `PalettesCSV` · `MorphsCSV` · `RulesCSV` · `NpcsCSV`.
- 에디터 DataAsset 길: 클래스 `UGrimAppearanceCatalogAsset`(DataTable 여섯 개를 가리킨다, `Source/Grimoire/Appearance/GrimAppearanceTypes.h`)은 있다 — 에셋을 만들어 설정 `CatalogAsset` 에 넣으면 CSV 대신 그것을 쓴다. 지금은 비어 있어 CSV 를 쓴다.
- 플레이어 시작 · 무작위는 규칙 `Player_Start`(설정 `PlayerRule`)을 쓴다(플레이어용 조각만 후보).

### 클래스

모두 `Source/Grimoire/Appearance/` 에 있다.

| 클래스 | 붙는 곳 | 하는 일 |
|---|---|---|
| `UGrimAppearanceSettings` (`GrimAppearanceSettings.h`) | `DefaultGame.ini` `[/Script/Grimoire.GrimAppearanceSettings]` | 표 경로, 체형별 몸 메시, 몸 재질 칸 → 색 칸, 키 범위, 자리 표시 재질, NPC 기본 애님, 소품 숨김 거리, 새 캐릭터에 생성 화면 열기 |
| `FGrimAppearanceCatalog` · `FGrimAppearance` (`GrimAppearanceTypes.h`) | 전역(한 번 읽음) | 표 → 정렬된 조각 · 팔레트 · 슬라이더 · 규칙, ID ↔ 번호 |
| `FGrimAppearanceMath` | 순수 함수 | `Sanitize` · `GenerateFromRule`(씨앗 → 외형, 결정적) · 팔레트 색 찾기 |
| `UGrimAppearanceComponent` (`GrimAppearanceComponent.h`) | 플레이어 폰(`AGrimPlayerCharacter`) · 마법탑 NPC(`AGrimTowerNPC`) · 더미 | 복제되는 외형 하나 → 몸 메시 · 조각 컴포넌트 · 소품 · 색 · 모프 · 고리 조립. 생성 화면 미리보기(로컬만) |
| `UGrimPlayerAppearanceComponent` (`GrimPlayerAppearanceComponent.h`) | `AGrimPlayerState`(하위 객체 `Appearance`) | 플레이어 외형의 정본. 세이브 「Player.Appearance」, 서버 RPC(생성 화면 결과 · 무작위), 서클이 바뀌면 고리 수 갱신, 폰이 바뀌면 새 폰에 넣기 |
| `AGrimAppearanceDummy` (`GrimAppearanceDummy.h`) | 치트 줄 세우기 · 나중의 군중 NPC 바탕 | 마네킹 메시 + 기본 애님 + 외형 컴포넌트 |
| `UGrimCharacterCreatorWidget` (`GrimCharacterCreatorWidget.h`) | 화면 | C++ 로 짠 최소 생성 화면(칸마다 ◀ ▶ · 슬라이더 · 무작위 · 확인 · 취소) |

### 흐름

```
플레이어 접속(서버)
  PlayerState 시작 ─ 무작위 견습생 한 벌(Player_Start, 씨앗 무작위)
  세이브 읽기 ─ 「Player.Appearance」 있으면 덮고 bCustomized = true
  폰 지정(OnPawnSet) · 서클 변화 ─ 폰의 UGrimAppearanceComponent::SetAppearance (서버)
                                      │ 복제(OnRep)
                          모든 화면: RebuildVisuals

생성 화면(클라이언트) ─ 고르는 동안 PreviewAppearance(로컬)
  확인 → Server_SubmitAppearance → Sanitize(플레이어 규칙) → 정본 갱신 → 폰 → 복제
  취소 → 복제된 값으로 되돌림

NPC(맵에 놓인 마법탑 NPC) 시작(서버)
  NpcAppearance.csv[NpcId] → 규칙 · 씨앗(없으면 NpcId 해시) → GenerateFromRule → SetAppearance → 복제
```

- 마법탑 NPC(`AGrimTowerNPC`)는 몸이 조립되면 그레이박스 도형(원통 · 구 · 원뿔)을 숨긴다. 퀘스트 게시판은 외형을 쓰지 않는다.
- 조각 위치는 **마네킹 기준 자세에서의 뼈 위치 + 메시 공간 오프셋**으로 적는다 → 실행 중에 뼈 기준 상대 변환으로 바꿔 붙인다. 뼈 축 방향을 몰라도 CSV 를 쓸 수 있다.

## 아트 파이프라인 — 옷 조각 하나가 지나는 길

1. GPT: 설정화(앞 · 뒤 A 포즈) + 조각 분해도 → `art/characters/` (첫 결과: 견습생 A, 아래 그림)
2. GPT: 조각마다 Tripo 입력 4면 시트(조각만, 마네킹 없이)
3. Tripo Studio 멀티뷰 → GLB
4. 블렌더(새 스크립트 `art/tools/blender_fit_to_mannequin.py` — 산정 표 A3, 미구현):
   - 마네킹 FBX(체형 A/B)를 불러 조각을 몸에 맞춰 크기 · 위치를 맞춘다
   - 몸에서 가중치를 옮긴다(Data Transfer, 가까운 면 보간) → 겹침 뚫림 확인용 포즈 3개 렌더
   - 몸에 가려지는 면 지우기 · LOD 2단계 · 재질 슬롯 이름을 색 칸 이름으로
5. 언리얼 가져오기 스크립트(`Scripts/Character/import_outfit_part.py` — 산정 표 A4, 미구현): 스켈레톤 = `SK_Mannequin`, 재질 = `M_GrimCharacter_Master` 인스턴스 → `Content/Grimoire/Characters/Parts/<칸>/SKM_<조각>`
6. `AppearanceParts.csv` 의 `MeshA` · `MeshB` 칸에 경로를 적으면 끝(코드 그대로)

![견습생 A — 앞 · 뒤](/art/characters/outfit-a-apprentice-turnaround.png)

![견습생 A — 조각 분해도(로브 · 스톨 · 고리 1·4·7 · 벨트 · 장화 · 장갑)](/art/characters/outfit-a-apprentice-pieces.png)

- 두 그림의 로브가 조금 다르다(분해도 쪽이 소매가 넓고 덧댄 천이 많다). **Tripo 입력은 턴어라운드 쪽을 기준**으로 하고, 덧댄 천은 텍스처 디테일로만.

## 산정 — 작업 항목 · 크기 · 순서

크기: **S** ≤ 반나절 · **M** 1~2일 · **L** 3~5일 (한 사람 + 코드 세션 기준). ✅ = 이 브랜치에서 끝남.

### 코드 · UI · 데이터

| # | 항목 | 크기 | 상태 |
|---|---|---|---|
| C1 | 외형 구조체 · 저장 · 복제 직렬화 · 카탈로그(CSV) · Sanitize | M | ✅ |
| C2 | NPC 규칙 생성(씨앗 결정적) · 이름 있는 NPC 표 | M | ✅ |
| C3 | 조립 컴포넌트(Leader Pose 조각 · 소품 · 색 · 모프 · 고리 · 키) | M | ✅ |
| C4 | 플레이어 정본 컴포넌트(세이브 · RPC · 서클 연동 · 폰 교체) | S | ✅ |
| C5 | 마법탑 NPC · 줄 세우기 더미에 외형 | S | ✅ |
| C6 | 최소 생성 화면(◀ ▶ · 슬라이더 · 무작위 · 확인 · 취소) | M | ✅ |
| C7 | 치트 · 콘솔 · 2인 넷 확인 스크립트 · 자동화 테스트 | S | ✅ |
| C8 | 생성 화면 다듬기: 카메라 클로즈업 · 회전 · 탭 · 패드 조작 · 팔레트 칸 그림 | L | |
| C9 | 새 캐릭터면 첫 접속에 생성 화면(코드는 있고 설정 `bOpenCreatorForNewCharacter=False` 로 꺼 둠) · 마법탑 「거울」 상호작용(미구현) | S | |
| C10 | 장비 외형 연결(장착 장비 → 외형 칸, 외형 칸 · 도감) — M4 장비와 함께 | M | |
| C11 | 군중 NPC 용 Skeletal Mesh Merge 길 · 거리별 조각 줄이기 | M | |
| C12 | 무리 NPC 얼굴 겹침 피하기 · 지역 규칙(E~H) · 사냥꾼 규칙 | S | |

### 아트 (시안 A 첫 슬라이스)

| # | 항목 | 수 | 크기 | 비고 |
|---|---|---|---|---|
| A1 | 마스터 재질 `M_GrimCharacter_Master`(색 칸 파라미터 · 고리 수) | 1 | M | 사용자가 에디터에서 · 스펙은 이 문서 |
| A2 | 기본 몸 체형 A · B (마네킹 비율, 손 · 발 · 목 이음매) | 2 | L | Tripo 사람 몸은 비율 · 위상이 들쭉날쭉 → 블렌더에서 마네킹에 맞춰 다듬는 쪽 |
| A3 | 블렌더 「마네킹에 맞추기 · 가중치 옮기기」 스크립트 | 1 | M | 모든 옷 조각이 지나간다 |
| A4 | 언리얼 가져오기 스크립트(옷 조각) | 1 | S | `Scripts/Environment/import_env_asset.py` 를 바탕으로 |
| A5 | 머리 메시(체형별) + 얼굴 셰이프 키 6 | 2 | L | 얼굴 프리셋 8 = 값 묶음(메시 아님) |
| A6 | 머리 모양 | 8 | M×8 | 카드식 머리카락(알파 텍스처는 GPT) |
| A7 | 수염 | 3 | S×3 | |
| A8 | 견습생 A: 로브 · 스톨 · 벨트(소품 4) · 바지 · 장화 · 장갑 | 6 조각 × 체형 2 | L | 첫 목표 — 이것만 있어도 플레이어 · 견습생 NPC 가 선다 |
| A9 | 교관 겉옷 · 사서 로브 · 베레모 · 책 벨트 · 낮은 신 | 5 | L | |
| A10 | 시험관 로브 · 원통 모자 · 어깨 망토 · 의식 지팡이(소품) | 4 | L | |
| A11 | 서클 고리(놋쇠) · 버클 · 책 · 주머니 소품 | 6 | S | Tripo 소품 |
| A12 | 팔레트 · 얼굴 프리셋 값 다듬기(화면 보며) | — | S | |

### 순서

1. **지금(이 브랜치)**: C1~C7 — 자리 표시로 끝까지 돈다(생성 화면 → 서버 → 복제 → 저장, NPC 규칙 생성).
2. **다음**: A1 → A3 · A4 → A2(체형 A 먼저) → A8(견습생 A, 체형 A) — 자리 표시를 실제 조각으로 바꾸는 첫 줄. 여기서 성능 예산을 잰다.
3. A5 · A6(머리 · 얼굴) → C8(생성 화면 다듬기) · C9.
4. A9 · A10(교관 · 사서 · 시험관) → 튜토리얼 NPC 네 명이 시안 A 그대로.
5. C10(장비 M4 와 함께) · C11 · C12, 지역 의상은 그 지역 때.

## 콘솔 · 치트 · 테스트

- 치트(개발 빌드, `UGrimCheatManager` — `Source/Grimoire/Core/Cheats/GrimCheatManager.h`): `GrimCreator` · `GrimAppearanceRandom [씨앗]` — [테스트 치트](../dev/cheats#캐릭터-외형).
- 콘솔(`Source/Grimoire/Appearance/GrimAppearanceDebug.cpp`): `Grim.Appearance.Dump` · `Grim.Appearance.Randomize [씨앗]`(서버, 모든 플레이어) · `Grim.Appearance.Lineup <규칙> [수] [씨앗]`(서버) · `Grim.Appearance.Reload`.
- 자동화(`Source/Grimoire/Appearance/Tests/`): `Grimoire.Appearance.Catalog` · `.Serialization` · `.NetSerialize` · `.NpcDeterminism` · `.Sanitize` · `.Assemble`.
- 2인 헤드리스: `powershell -File Scripts\Character\net_check_appearance.ps1` — 서버가 외형을 바꾸고 더미를 세운 뒤 양쪽 `Grim.Appearance.Dump` 의 해시를 비교해 `PASS` / `FAIL`.
