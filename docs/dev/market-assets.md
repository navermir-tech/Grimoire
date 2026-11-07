# 마켓 · Fab 에셋 팩

> 2026-11-07 결정: 받은 팩은 **로컬에만** 두고, 레벨에서 **실제로 쓰는 에셋만** 저장소(LFS)에 올린다. 팩을 통째로 올리면 수 GB 라 GitHub LFS 무료 한도를 넘는다.
> 후보 목록 · 라이선스는 [Fab 무료 에셋 후보](../art/fab-free-assets).

## 규칙

- 팩은 들어온 폴더 그대로 둔다(`Content/<팩 이름>/`, Megascans 는 `Content/Fab/Megascans/`). `Content/Grimoire/` 로 옮기지 않는다.
- `.gitignore` 가 `Content/` 아래를 기본으로 뺀다. 저장소에 들어가는 폴더는 `Content/Grimoire/`, 엔진 샘플(`Characters` · `Input` · `NiagaraExamples`), 우리 맵의 `__ExternalActors__/Grimoire/` · `__ExternalObjects__/Grimoire/` 뿐이다.
- 그래서 새 팩을 받아도 `git status` 에 뜨지 않는다. 받은 팩은 아래 「받은 팩」 표에 한 줄 더한다.

## 쓰는 에셋만 올리기

레벨(우리 맵)에 팩 에셋을 놓으면 그 에셋과 의존하는 것(머티리얼 · 텍스처)이 저장소에 없어서, 다른 PC · 워크트리에서는 빈 메시가 된다. 커밋 전에:

1. 에디터에서 놓은 에셋을 우클릭 → **참조 뷰어(Reference Viewer)** 로 의존 에셋을 확인한다.
2. 그 파일들만 강제로 더한다: `git add -f Content/<팩>/<경로>.uasset`(`.gitignore` 를 넘는다). 한 번 추적되면 그다음부터는 보통 파일처럼 바뀐 것이 보인다.
3. 커밋 전에 `git status` 로 팩 전체가 섞이지 않았는지 본다.

자동으로 의존 파일을 모으는 스크립트는 레벨 아트 작업을 시작할 때 만든다.

## 워크트리 · 다른 PC

- 팩은 `C:\Dev\Grimoire` 에만 있다. 새 워크트리에는 없다. 팩을 보는 작업(레벨 아트 · 배치)은 워크트리 없이 메인 체크아웃에서 하거나, 워크트리에 정션을 건다:
  `New-Item -ItemType Junction -Path <워크트리>\Content\<팩> -Target C:\Dev\Grimoire\Content\<팩>`
- 다른 PC 에서는 아래 표대로 다시 받는다.

## 다시 받는 법

- **UE 5.8 지원 팩**: 에디터 메뉴 **창 → Fab** → 내 라이브러리 → **프로젝트에 추가**.
- **5.4 까지만 지원하는 팩**(Project Nature 등): 런처는 5.8 프로젝트를 고르게 해 주지 않는다. 받기 전용 빈 프로젝트 `문서\Unreal Projects\FabImport54\FabImport54.uproject`(내용: `"EngineAssociation": "5.4"` 만 있는 .uproject + 빈 `Content`)를 두고 런처 **라이브러리 → Fab 라이브러리 → 프로젝트에 추가 → FabImport54** 로 받은 뒤, `Content\<팩>` 폴더를 `C:\Dev\Grimoire\Content\` 로 복사한다. 엔진 5.4 는 설치하지 않아도 된다. 5.8 에디터가 열 때 맞춰 읽는다.
- 런처는 `문서\Unreal Projects` 안의 프로젝트만 찾는다 — Grimoire(`C:\Dev`)는 목록에 나오지 않는다.

## 받은 팩 (이 PC, 2026-11-07)

| 폴더 | 크기 | 무엇 · 출처 |
|---|---|---|
| `PN_FoliageCollection` | 1.5 GB | [temperate Vegetation: Foliage Collection](https://www.fab.com/listings/6a5ae8db-d80f-4b23-b276-87da390cfe56) — 풀 · 바닥 식생 · 들꽃 · 지형 재질. 5.4 팩(FabImport54 로 받음). 둘러보기 맵 `Maps/ContentShowroom` |
| `Medieval_House_Props_V2` | 2.0 GB | 중세 집 소품(실내) |
| `Iceland_Environment` | 1.2 GB | 바위 · 이끼 환경. 예시 맵 `Example_Map` |
| `BlackCherryBrokenTrunks01` | 1.0 GB | [Black Cherry Broken Trunks 01](https://www.fab.com/listings/2ee3d260-9ed0-434c-aff2-168ff07e7c90) — 쓰러진 나무 · 둥치 |
| `B3D` | 0.8 GB | 모델 팩(`Models` · `_Shared_Assets`) |
| `MWLandscapeAutoMaterial` | 0.7 GB | 지형 자동 재질 |
| `Stump_Scanned` | 0.6 GB | [Stump Scanned](https://www.fab.com/listings/5f433961-8d90-49ab-a991-e4c7a78acccf) — 그루터기 |
| `Gothic_Environment` | 0.3 GB | 고딕 환경 키트 |
| `Medieval_Room_Kit` | 0.3 GB | 중세 방 모듈 키트(실내) |
| `Megaplant_Library` | 0.2 GB | Megaplants — `Tree_English_Oak`. 플러그인 `ProceduralVegetationEditor` 필요(`Grimoire.uproject` 에 켬) |
| `StyleHex_Studio` | 0.2 GB | StyleHex 무료 팩 셋 — Stylized Forest Sample · Grass Flowers Foliage · Rocks and Stones |
| `Fab` | 0.1 GB | Quixel Megascans 개별 에셋(`Megascans/3D` — 중세 서고 책 · 의자 · 샹들리에 · 두루마리, 농가 통 · 바구니 · 솥 · 맷돌, 교회 폐허 벽 · 아치, 깔개 등) |
| `WaterMaterials` | 0.1 GB | [Water Materials](https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861) — CC BY(크레딧 필요, [Q126](../spec/decisions#미결-질문)) |
| `EnchantedPotionRoom` | 41 MB | [EnchantedPotionRoom_Free](https://www.fab.com/listings/6bee477a-f8a8-42d5-b987-c6937305ab58) — 물약 방(실내) |
| `GoodSkyV3` | 18 MB | 하늘 |
| `HandPaintedClouds` | 5 MB | 손그림 구름 |
| `__ExternalActors__/Scene_Saloon` | — | 위 팩 가운데 하나의 예시 맵(`Scene_Saloon`) 배치 데이터 |

이름만 있는 행은 Fab 「내 라이브러리」에서 정확한 목록 이름을 확인해 링크를 채운다.
