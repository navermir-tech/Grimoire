# 건축 · 농사 · 생산 시스템

> 스펙: [하우징 · 거점](../spec/housing) (R11), [마법 생물](../spec/creatures) (R02 노동)

## 거점 `AGrimBaseCore`

- 배치하면 반경(`BaseRadius`, 서클에 따라 확장) 안이 거점. 복제 액터, 소유 `FGuid OwnerId` + 멤버 권한 `TMap<FGuid, EBasePermission>`.
- 보유: 건축 조각 목록(세이브 단위), 작업 보드(노동), 부활 지점, 시설 레지스트리.
- 거점 밖 건축은 불가(단순한 규칙). 야외 임시 모닥불 · 양초 같은 **사물 배치**는 거점 밖에서도 가능.

## 건축

### 데이터 `UGrimBuildPieceDefinition`

`Mesh, Category(Foundation · Wall · Roof · Door · Stair · Pillar · Furniture · Light · Station), Cost(TMap<ItemId, int>), SnapSockets, bProvidesRoof, Insulation, Health`

### 배치 `UGrimBuildModeComponent` (플레이어 캐릭터, 로컬 + 서버 검증)

1. 건축 모드 진입 → 조각 선택 → **고스트 미리보기**(로컬, 반투명 머티리얼, 유효 초록 / 무효 빨강).
2. 스냅: 주변 조각의 `SnapSockets` 중 가장 가까운 것에 붙임(그리드 없음 — 소켓 스냅 방식). 바닥 조각은 지형에 직접.
3. 유효성: 거점 반경 안, 겹침 없음, 지지(바닥 위 또는 벽 위, 단순 규칙), 재료 보유.
4. 확정 → `Server_PlacePiece(PieceId, Transform, SnapTo)` → 서버 재검증 → `AGrimBuildPiece` 스폰(복제) + 재료 차감.

- 철거: 권한 확인 → 재료 일부 환급.
- 최적화: 같은 메시 조각은 거점 단위 `HISM` 으로 묶고 상호작용 · 피해가 필요할 때만 액터화(M4 이후, 측정 후).

### 지붕 판정

- `bProvidesRoof` 조각은 `ECC_Roof` 채널 충돌. 생존(비 · 바람) · 상호작용(비에 불 꺼짐)은 위로 트레이스해 이 채널을 본다.

### 마법 건축 (제안 · M5+)

- 땅 마법 `GA_Spell_EarthWall`: 흙벽 조각을 재료 없이 임시 생성(내구 낮음, 1일 후 소멸). 불 마법으로 가마에서 벽돌.

## 꾸미기

- 가구 · 장식 = `Category.Furniture` 조각(기능 없음) 또는 기능 조각(침대 = 수면, 조명 = `GrimElementalState` 사물).
- 쾌적도(제안): 거점 반경 내 장식 점수 합 → `GE_Comfort`(피로 회복 ↑, 일꾼 사기).

## 농사

### 데이터 `UGrimCropDefinition`

`Stages[](Mesh, Duration), Needs { Water: 0~1/일, Light: 0~1, TempRange }, Yield(DropTable), SeedItem, bPerennial`

### 액터 `AGrimCropPlot` (밭) + `UGrimCropComponent`

- 밭은 땅 마법으로 생성(`GA_Spell_Till` → 땅 자극 → `State.Tilled` → 밭 액터 스폰) 또는 건축 조각(Furniture.Planter).
- 성장: 서버 타이머(1분 간격) — `Growth += dt × f(Water, Light, Temp)`. `Water` 는 `State.Wet` 세기에서, `Light` 는 낮 또는 `State.Illuminated`(빛 마법), `Temp` 는 기온.
- 급수 경로는 **전부 속성 자극**: 비 · 물 마법 · 물 생물 노동 모두 `Water` 자극 → `State.Wet`. 불 자극 → 작물 소실 + `State.Ash`(다음 작물 생장 +20%).
- 수확: 바람 마법(가벼운 열매 떨어뜨리기) 또는 땅 마법(뿌리 뽑기) — 작물 정의가 지정(R16).

## 생산

### 시설 `AGrimCraftStation` + `UGrimCraftingComponent`

- `StationTag`(Workbench · Kiln · Cookpot · WandWorkshop · Storage).
- 대기열: `TArray<FGrimCraftJob { RecipeRow, Remaining, Assignee }>`(복제). 서버 타이머가 진행.
- **가열 시설**은 `GrimElementalState` 를 가지며 `State.Burning` 세기가 속도 배율(불 마법으로 점화, 바람으로 ↑, 비에 꺼짐 — 지붕 필요).
- 일꾼: 작업 보드에 `Work.Craft`(적성 생물 배정 시 속도 ×1.5), `Work.Fuel`(불 생물이 가열 유지).

### 레시피 `DT_Recipes`

`RecipeId, StationTag, Inputs[], Output, Seconds, RequiredCircle, RequiredSpell(각인용)`

### 완드 공방

- 입력: 재료(나무 · 결정 · 생물 소재) + **각인**: 해당 속성 마법을 시설에 시전해야 완성(자극 요구 행). 결과 완드 정의는 재료 조합표에서.

## 인벤토리 · 아이템

- `UGrimItemDefinition`(PrimaryDataAsset `Item`): `Type(Material · Food · Seed · Skillbook · Wand · Tool · Placeable), Stack, Weight, Icon, 사용 효과`.
- `UGrimInventoryComponent`(플레이어 · 저장고 · 시설): `TArray<FGrimItemSlot>`(FastArray 복제, 소유자 / 열람자에게만). 서버 함수 `Add / Remove / Move`, 클라이언트는 `Server_MoveItem` 요청.
- 무게 제한(제안) → 운반 일꾼의 쓸모.

## 세이브

- 거점 핵: 조각 목록(PieceId, Transform, Health), 시설 대기열, 저장고 인벤토리, 밭 상태(Growth, Wet) — 거점 단위로 직렬화.

## M4 범위

바닥 · 벽 · 지붕 · 문 4종 + 침대 + 모닥불 + 저장고 + 작업대 + 밭 1종 + 작물 2종. 일꾼 연동은 M5.
