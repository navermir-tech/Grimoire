# 건축 · 농사 · 생산 시스템

> 스펙: [하우징 · 거점](../spec/housing) (R11), [마법 생물](../spec/creatures) (R02 노동)
>
> 상태: M4 에서 거점 · 건축 · 시설 4종 · 밭 · 장비 · 내구도 · 수리가 `Source/Grimoire/Base/` 에 들어갔다. 실제 클래스 · 표 · 조작 · 콘솔은 [M4 작업 계획 — 거점](./m4-plan) 이 정본이고, 이 문서의 절마다 「구현:」 으로 잇는다. 「설계안」 이라 적힌 줄은 아직 코드가 없다.

## 거점 `AGrimBaseCore`

구현: `Source/Grimoire/Base/GrimBaseCore.h` 의 `AGrimBaseCore`(격자 원점 · 반경 · 주인 PlayerId · 거점 수리), 목록은 `Source/Grimoire/Base/GrimBaseSubsystem.h` 의 `UGrimBaseSubsystem`. 멤버 권한 · 작업 보드 · 부활 지점은 설계안(M5~).

- 배치하면 반경(`BaseRadius`, 서클에 따라 확장) 안이 거점. 복제 액터, 소유 `FGuid OwnerId` + 멤버 권한 `TMap<FGuid, EBasePermission>`.
- 보유: 건축 조각 목록(세이브 단위), 작업 보드(노동), 부활 지점, 시설 레지스트리.
- 거점 밖 건축은 불가(단순한 규칙). 야외 임시 모닥불 · 양초 같은 **사물 배치**는 거점 밖에서도 가능.

## 건축

### 데이터 — `Content/Grimoire/Data/BuildPieces.csv`

구현: 행 `FGrimBuildPieceRow` → `FGrimBuildPieceDefinition`(`Source/Grimoire/Base/GrimBaseTypes.h`, 읽기 `FGrimBaseDatabase`). 열은 [M4 — 데이터](./m4-plan#데이터-표-정본-런타임에-읽는다). 아래 필드 목록은 처음 설계안(DataAsset `UGrimBuildPieceDefinition`)이다.

`Mesh, Category(Foundation · Wall · Roof · Door · Stair · Pillar · Furniture · Light · Station), Cost(TMap<ItemId, int>), SnapSockets, bProvidesRoof, Insulation, Health`

### 배치 `UGrimBuilderComponent` (PlayerState, 로컬 고스트 + 서버 RPC)

구현: `Source/Grimoire/Base/GrimBuilderComponent.h` 의 `UGrimBuilderComponent`(건축 모드 · 상호작용 F · 모든 서버 RPC), 고스트 `Source/Grimoire/Base/GrimBuildGhost.h` 의 `AGrimBuildGhost`, 격자 · 지지 계산 `FGrimBuildGrid`(`Source/Grimoire/Base/GrimBaseTypes.h`), 조각 `Source/Grimoire/Base/GrimBuildPiece.h` 의 `AGrimBuildPiece`. 설계안 이름은 캐릭터에 붙는 「BuildMode」 컴포넌트였다. `SnapSockets` · HISM 묶기는 설계안 그대로(미구현) — 지금은 [M4 — 격자 · 지지](./m4-plan#격자-·-지지-q33) 규칙.

1. 건축 모드 진입 → 조각 선택 → **고스트 미리보기**(로컬, 반투명 머티리얼, 유효 초록 / 무효 빨강).
2. 스냅: **격자 스냅**(R30 — 팰월드 크기의 큰 블록, 칸 크기는 설정 `BuildGridSize` 300cm · 벽 높이 300cm — 아크와 같음, Q33 확정). 구조 조각은 거점 격자 · 이웃 조각의 `SnapSockets` 에 붙고, 첫 바닥은 지형에 직접. 가구 · 장식(`Category.Furniture` · `Light`)은 격자 없이 표면에 자유 배치.
3. 유효성: 거점 반경 안, 겹침 없음, 지지(바닥 위 또는 벽 위, 단순 규칙), 재료 보유.
4. 확정 → `Server_PlacePiece(PieceId, Transform, SnapTo)` → 서버 재검증 → `AGrimBuildPiece` 스폰(복제) + 재료 차감.

- 철거: 권한 확인 → 재료 일부 환급.
- 최적화: 같은 메시 조각은 거점 단위 `HISM` 으로 묶고 상호작용 · 피해가 필요할 때만 액터화(M4 이후, 측정 후).

### 지붕 판정

- 구현: 전용 채널 없이 날씨의 기존 위로 트레이스(WorldStatic · WorldDynamic)가 지붕 조각에 막히면 실내다 — `UGrimWeatherSubsystem::IsActorSheltered`(`Source/Grimoire/Environment/GrimWeatherSubsystem.h`).
- (처음 설계안) `bProvidesRoof` 조각은 `ECC_Roof` 채널 충돌. 생존(비 · 바람) · 상호작용(비에 불 꺼짐)은 위로 트레이스해 이 채널을 본다.

### 마법 건축 (제안 · M5+)

- 땅 마법 `GA_Spell_EarthWall`: 흙벽 조각을 재료 없이 임시 생성(내구 낮음, 1일 후 소멸). 불 마법으로 가마에서 벽돌.

## 꾸미기

- 가구 · 장식 = `Category.Furniture` 조각(기능 없음) 또는 기능 조각(침대 = 수면, 조명 = `GrimElementalState` 사물).
- 쾌적도(제안): 거점 반경 내 장식 점수 합 → `GE_Comfort`(피로 회복 ↑, 일꾼 사기).

## 농사

### 데이터 — `Content/Grimoire/Data/Crops.csv`

구현: 행 `FGrimCropRow` → `FGrimCropDefinition`, 생장 계산 `FGrimCropMath`(`Source/Grimoire/Base/GrimBaseTypes.h`). 아래는 처음 설계안(DataAsset `UGrimCropDefinition`)이다.

`Stages[](Mesh, Duration), Needs { Water: 0~1/일, Light: 0~1, TempRange }, Yield(DropTable), SeedItem, bPerennial`

### 액터 `AGrimFarmPlot` (밭)

구현: `Source/Grimoire/Base/GrimBaseStations.h` 의 `AGrimFarmPlot` — 작물 컴포넌트 없이 액터 하나(씨앗 · 생장 · 수분 · 재 · 거두기, F 로 심기 · 거두기). 튜토리얼의 `AGrimCropPlot` 은 L_Tower 에 놓인 작물 자리로 따로 남았다. 땅 마법 밭 만들기 · 바람 · 땅 마법 수확은 설계안(M5).

- 밭은 땅 마법으로 생성(`GA_Spell_Till` → 땅 자극 → `State.Tilled` → 밭 액터 스폰) 또는 건축 조각(Furniture.Planter).
- 성장: 서버 타이머(1분 간격) — `Growth += dt × f(Water, Light, Temp)`. `Water` 는 `State.Wet` 세기에서, `Light` 는 낮 또는 `State.Illuminated`(빛 마법), `Temp` 는 기온.
- 급수 경로는 **전부 속성 자극**: 비 · 물 마법 · 물 생물 노동 모두 `Water` 자극 → `State.Wet`. 불 자극 → 작물 소실 + `State.Ash`(다음 작물 생장 +20%).
- 수확: 바람 마법(가벼운 열매 떨어뜨리기) 또는 땅 마법(뿌리 뽑기) — 작물 정의가 지정(R16).

## 생산

### 시설 `AGrimWorkbench` · `AGrimStorageChest`

구현: `Source/Grimoire/Base/GrimBaseStations.h` — 공통 `AGrimStationBase`, 작업대 `AGrimWorkbench`(대기열 없이 **즉시** 제작), 저장고 `AGrimStorageChest` + `Source/Grimoire/Base/GrimItemContainerComponent.h` 의 `UGrimItemContainerComponent`(24칸), 지은 침대 · 모닥불 `AGrimBaseBed` · `AGrimBaseCampfire`. 아래 대기열 · 가열 시설 · 일꾼 배정은 설계안(가마 · 요리대 · 완드 공방은 M5).

- `StationTag`(Workbench · Kiln · Cookpot · WandWorkshop · Storage).
- 대기열: `TArray<FGrimCraftJob { RecipeRow, Remaining, Assignee }>`(복제). 서버 타이머가 진행.
- **가열 시설**은 `GrimElementalState` 를 가지며 `State.Burning` 세기가 속도 배율(불 마법으로 점화, 바람으로 ↑, 비에 꺼짐 — 지붕 필요).
- 일꾼: 작업 보드에 `Work.Craft`(적성 생물 배정 시 속도 ×1.5), `Work.Fuel`(불 생물이 가열 유지).

### 레시피 `Content/Grimoire/Data/Recipes.csv`

구현: 행 `FGrimRecipeRow`(`Source/Grimoire/Items/GrimItemTypes.h`) — 작업대는 `Station=Workbench` 행을 쓴다. 아래 열 목록은 처음 설계안이다.

`RecipeId, StationTag, Inputs[], Output, Seconds, RequiredCircle, RequiredSpell(각인용)`

### 완드 공방

- 입력: 재료(나무 · 결정 · 생물 소재) + **각인**: 해당 속성 마법을 시설에 시전해야 완성(자극 요구 행). 결과 완드 정의는 재료 조합표에서.

## 내구도 · 수리 · 인챈트 (R18 · R19)

- 구현(M4): 장비 8칸 · 닳기 · 부러진 완드 · 저항은 `Source/Grimoire/Base/GrimEquipmentComponent.h` 의 `UGrimEquipmentComponent`(표 `Content/Grimoire/Data/Equipment.csv`), 건축 · 시설 내구도 · 노후 · 불 피해 · 잔해는 `Source/Grimoire/Base/GrimStructureComponent.h` 의 `UGrimStructureComponent`, 계산 `FGrimDurabilityMath`, 수치 `UGrimBaseSettings`([M4 — 내구도 · 수리 수치](./m4-plan#내구도-·-수리-수치-ugrimbasesettings-ini)), 「수리」 볼트 `Source/Grimoire/Base/GrimRepairProjectile.h` 의 `AGrimRepairProjectile`(`DA_Spell_Repair`). 아이템 인스턴스는 `FGrimItemInstance { ItemId, Count, Durability, Enchantments, RepairCount }` — 최대 내구도는 표의 `MaxDurability` × 수리 감소.
- **완드 칸 · 기본 완드(Q71 답 2026-11-07)**: 완드 칸이 비면 주먹질이다(복제 루스 태그 `Status.WandMissing` — 부러짐 `Status.WandBroken` 과 같은 길, [마법 — 완드](./magic#완드-agrimwand)). 처음 들어온 플레이어는 `UGrimBaseSettings::StartingEquipment`(`Wand_Basic` 「기본 완드」 — 내구도 200 · 시전마다 1 · 수리 재료 나무)를 입고 시작한다(컴포넌트 `BeginPlay`, 세이브가 있으면 세이브가 이긴다). 마법탑이 10 솔에 늘 판다.
- **장비 내구도**: 장비는 아이템 인스턴스 데이터(내구도 · 수리 횟수 · 인챈트)를 가진다(인벤토리 슬롯에 같이 복제 · 저장). 서버만 깎는다 — 완드는 `UGrimSpellAbility` 의 시전 확정 때 `DurabilityCostPerCast`, 방어구는 피해 실행(`UGrimDamageExecution`) 뒤 피해량 비례. 0 → 장비 해제 + 잔해 아이템으로 교체(완드는 `bBroken` 표시만 — Q22).
- **건축 내구도**: 조각의 `Health`(세이브에 이미 있음)를 쓰고, 피해는 GAS 가 아닌 조각 전용 `ApplyStructureDamage` 하나로 받는다(습격 · 불 · 노후 모두). 불은 속성 상태 컴포넌트가 `State.Burning` 세기 × 초당 피해로 넘긴다 — 조각도 `UGrimElementalStateComponent` 를 가진다(재료 태그 `Material.Flammable` 은 나무만).
- **수리**: 대상(장비 · 조각) + 재료 아이템 + 무속성 수리 마법 「수리」(스킬북으로 배움 — R28). 수리 마법은 일반 마법과 같은 `IGrimElementalReceiver` 경로로 "수리 자극"(`Element.None` + 수리 태그)을 보내고, 받는 쪽이 재료가 맞으면 회복한다 — 상호작용이 마법이라는 R16 을 그대로 쓴다. 마법탑 NPC 대리 수리 · 지우기는 같은 함수를 NPC 가 부르고 수수료를 받는다. 수치는 `UGrimBaseSettings`(ini): 재료당 회복 25%, 수리마다 최대치 −10%, 하한 50%. 구현은 수리 자극 대신 「수리」 볼트가 맞은 대상(조각 · 시설 · 작업대에 건 장비 · 거점 핵)을 직접 고친다.
- **같은 장비 합치기**(R28, M5 설계안): 순수 계산 `FGrimEnchantMergeMath`(마인크래프트 모루 규칙 — 작업 횟수 `RepairCount` 를 `PriorWork` 로 같이 씀).
- **인챈트**(M5 설계안 — 표 `Content/Grimoire/Data/Enchants.csv` 만 있고 읽는 코드 없음): `UGrimEnchantDefinition`(DataAsset — 대상 슬롯 · 단계별 효과 GE · 비용 · 요구 서클 · 함께 못 하는 인챈트 목록). 장착 시 효과 GE 를 플레이어 ASC 에 Infinite 로 걸고 해제 시 지운다. 「자가 회복」 · 「견고」는 내구도 코드가 읽는 태그.
- 마일스톤: 내구도 · 수리 = **M4**(인벤토리 완성과 함께), 인챈트 · 각인대 = **M5**(완드 공방과 함께).

## 인벤토리 · 아이템

M3 기초(갈래 I)가 들어갔다 — 플레이어 가방(칸 수 고정) · 줍기 · 버리기 · 먹기 · 스킬북 읽기 · 아궁이/모닥불 요리. 저장고 · 시설 인벤토리, 무게 제한, 장비(내구도 · 인챈트), 아이콘은 M4. 코드는 `Source/Grimoire/Items/`.

### 표 — `Content/Grimoire/Data/Items.csv` · `Recipes.csv` (정본, 런타임에 읽는다)

- 퀘스트 표와 같은 방식: `UGrimItemSettings::GetDatabase()` 가 한 번 읽어 `FGrimItemDatabase` 로 둔다. 문제는 로그(Error) · 테스트 `Grimoire.Items.Table`. 콘솔 `Grim.Items.Reload`.
- 아이템 행(행 이름 = 아이템 ID, `FGrimItemRow`): `DisplayName · Description · Kind(Material · Food · Skillbook · Tool) · MaxStack · Weight(자리만) · Groups(묶음, `|`)` · 음식 `HungerRestore · bRaw · DiseaseChance(음수 = 생존 기본) · BuffSeconds · BuffColdResist · BuffHeatResist · BuffDiseaseResist` · 요리 `CookResult · CookSeconds` · 스킬북 `TeachesSpell(DA_Spell_<ID>)` · 바닥 모양 `Shape · Size · Color`(엔진 기본 도형 — 아트 전).
- 묶음(Groups): 레시피 재료 `#Meat` 와 퀘스트 이벤트 `Item.Obtained.<묶음>` 에 쓴다. 고기 계열은 `Meat`(튜토리얼 `Item.Obtained.Meat`).
- 레시피 행(`FGrimRecipeRow`): `DisplayName · Station(시설 ID) · Inputs("#Meat:1|Herb:1|Salt:1|Grain:1") · Output · OutputCount · CookSeconds`. 재료는 ID 재료를 먼저, 묶음 재료를 나중에 채운다. 내용물이 **정확히** 맞아야(남는 것 없음) 끓는다.

| 아이템 ID | 이름 | 종류 | 메모 |
|---|---|---|---|
| `JackalopeMeat` | 재칼로프 고기(날) | 음식 · 날것 | 배고픔 +8, 배탈(생존 기본 30%), 12초 익으면 `RoastedJackalopeMeat`. 묶음 Meat |
| `RoastedJackalopeMeat` | 구운 재칼로프 고기 | 음식 | 배고픔 +30. 묶음 Meat |
| `JackalopeFur` · `JackalopeAntler` | 재칼로프 털 · 뿔 | 재료 | 재칼로프 전리품 |
| `KelpieMane` · `KelpieWaterEssence` | 켈피 갈기 · 물 정수 | 재료 | 켈피 전리품(켈피 고기는 없음 — 생물 스펙). 묶음 Water |
| `Waterweed` | 수초 | 음식 · 날것 | 물가 채집 자리(전리품 아님). 배고픔 +3, 배탈 5% |
| `BlackDogHide` · `BlackDogFang` | 검은 개 가죽 · 송곳니 | 재료 | 검은 개 전리품 |
| `Herb` · `Salt` · `Grain` | 허브 · 소금 · 곡물 | 재료 | 요리 재료(튜토리얼 17단계 지급) |
| `Stew` | 고기 스튜 | 음식 | 배고픔 +55, 버프 추위 저항 +1 · 300초. 레시피 `MeatStew`(냄비) |
| `Skillbook_Ember` · `_WaterJet` · `_Breeze` | 스킬북 | 스킬북 | 쓰면 `UseSkillbook` |

### 아이템 인스턴스 `FGrimItemInstance`

`ItemId · Count` + M4 자리 `Durability(음수 = 없음) · Enchantments`. 내구도 · 인챈트가 있으면 쌓이지 않는다. 직렬화 `Serialize(FArchive&)` 하나를 가방 · 픽업 · 냄비가 같이 쓴다.

### 가방 `UGrimInventoryComponent` (PlayerState, 런타임에 붙임)

- **붙이는 곳**: `UGrimItemSubsystem`(월드, 서버)이 PlayerState 가 스폰되자마자 붙인다(이름 고정 `GrimInventory`, 복제되는 동적 컴포넌트). PlayerState 파일(Core)을 고치지 않으려고 퀘스트 컴포넌트와 같은 방식을 썼다 — 스폰 콜백에서 붙이므로 PostLogin 의 세이브 읽기보다 먼저 있다. 서브시스템만 두지 않은 까닭: 소유 클라이언트 복제 · 서버 RPC 는 소유자 체인(PlayerState)의 컴포넌트여야 한다.
- 칸 수 고정 `PlayerSlotCount`(24). 칸은 FastArray(`FGrimInventoryList`, 항목에 칸 번호)로 **소유 클라이언트에게만** 복제. 칸 계산은 순수 `FGrimInventoryModel`(쌓을 수 있는 칸 먼저 → 빈 칸, 옮기기 = 합치기 / 바꾸기).
- 서버 함수: `AddItem · RemoveItem · RemoveFromSlot · MoveItem · UseItem · DropItem · PickUp · InteractWithPot`. 클라이언트 RPC: `Server_MoveItem · Server_UseItem · Server_DropItem · Server_PickUp · Server_InteractPot`(서버가 거리 검사 — `PickupRadius` · `PotInteractRadius` + `ServerReachSlack`). 알림은 `Client_Notice` → `OnNotice`.
- 쓰기: 음식 = 먹기(아래 · [생존 — 먹기 · 요리](./survival#먹기-·-요리)), 스킬북 = `UGrimSpellbookComponent::UseSkillbook`(배우거나 숙련도를 받으면 하나 소모), 재료 · 도구 = 못 쓴다.
- 퀘스트 이벤트(`UGrimQuestComponent::NotifyQuestEvent`): 얻으면 `Item.Obtained.<ID>` 와 묶음마다 `Item.Obtained.<묶음>`, 먹으면 `Food.Eaten`, 냄비에 넣으면 `Cooking.IngredientsAdded`, 완성되면 넣은 사람들에게 `Cooking.Completed`. 퀘스트 `Give` 의 `Item:<ID>` 는 `UGrimItemStatics::GiveItem` 으로 가방에 하나 넣는다(표에 없는 거점 핵은 기록만).
- 세이브: `IGrimSaveParticipant`(Player, `Items.Inventory` v1 — 칸 수 + 칸마다 인스턴스). 읽을 때 표에 없는 아이템은 버리고, 칸이 줄었으면 넘친 것을 빈 칸에 넣어 본다.

### 월드 아이템 `AGrimItemPickup` · `UGrimItemStatics::SpawnPickup`

- `UGrimItemStatics::SpawnPickup(World, ItemId, Count, Location)` — 서버 전용. 아래 바닥(WorldStatic)을 찾아 그 위에 놓는다. 생물 전리품(갈래 C)이 이 이름 · 인자 그대로 부른다. 모르는 ID · 개수 0 · 클라이언트면 nullptr.
- 줍기: **다가가 F**(`InteractKey`, 침대 · NPC 와 같은 방식 — 마법 상호작용의 예외). 설정 `bAutoPickupOnOverlap` 이면 닿기만 해도 줍는다(버린 사람은 `DropAutoPickupDelay` 동안 제외). 가방이 차면 들어간 만큼만 줍고 나머지는 남는다.
- 콜리전: 투사체(WorldDynamic)에 맞고 시야 추적에 걸리며 폰은 겹쳐 지나간다. 속성 자극을 받는다(`IGrimElementalReceiver`) — 요리.
- 세이브: World `Item.Pickup.<이름>` v1(인스턴스 + 요리 진행). 게임 중 스폰된 것은 다시 스폰된다.

### 조리 시설 `AGrimCookingPot`

아궁이(불붙는 사물 — 튜토리얼은 `AGrimCampfire` 「Tut_Stove」) 위의 냄비. 튜토리얼 맵 `L_Tower` 에 `Tut_CookingPot`(`build_tower_map.py`). 다가가 F = 가방에서 레시피 재료를 넣기(넣을 게 없고 끓기 전이면 도로 꺼내기). 끓기 · 완성은 [생존 — 먹기 · 요리](./survival#먹기-·-요리). 콜리전이 없다(불 마법이 아래 아궁이에 닿게). 세이브: World `CookingPot.<이름>` v1(내용물 · 진행).

### 화면 `UGrimInventoryWidget` (C++ 위젯 트리)

- 아래 가운데: 줍기 · 냄비 안내(「[F] 줍기 — 재칼로프 고기(날)」, 「고기 스튜: 고기 1/1 · 허브 0/1 …」, 「익는 중 40%」 · 「불이 필요하다」), 그 위 알림 한 줄(`NoticeSeconds`).
- **가방 키 L**(`BagKey`): 칸 격자(6열) + 고른 칸 설명 + 사용(먹기 · 읽기) · 하나 버리기 · 모두 버리기 · 옮기기(칸 고르고 [옮기기] → 놓을 칸). 열린 동안 커서 · 시점 고정(그리모어 화면과 같은 방식, 그리모어가 열려 있으면 닫는다).
- **탭 「생물」**(R38): 데리고 다니는 생물 · 생물 보관함 목록 — 탭 클래스 `Creatures/UGrimCreatureStorageTab`(순서 15 = 장비 다음, ini 한 줄)이 목록 위젯 `UGrimCreatureStorageWidget` 을 끼운다. [생물 시스템 — 데리고 다니기 · 보관함](./creatures#데리고-다니기-·-보관함-·-거점-상자-r38).
- 스킬북 아이템의 `TeachesSpell` 이 `Skill:<ID>` 면 마법이 아니라 **노말 스킬** 책이다(예: `Skill:CreatureStorage` — 생물 보관함). 읽으면 그 스킬을 배운다.
- 키 조사: 호그와트 레거시 PC 기본 배치는 **소지품(Inventory) = L · 장비(Gear) = I** · 지도 = M · 아이템 버리기 = C. 그래서 가방은 L, I 는 장비 화면(M4)에 남긴다. 키는 PlayerController 키 상태로 본다(정식 입력 액션은 조작 갈래 몫).

### 설정 `UGrimItemSettings` — `[/Script/Grimoire.GrimItemSettings]`

`ItemsCSV · RecipesCSV · PlayerSlotCount(24) · BagKey(L) · InteractKey(F) · PickupRadius(220) · ServerReachSlack(150) · bAutoPickupOnOverlap(False) · DropAutoPickupDelay(2) · DropDistance(120) · CookHeatRadius(170) · CookCheckInterval(0.5) · ExtraCookRatePerIntensity(0.5) · DirectFireHeatSeconds(4) · DirectFireMinIntensity(0.3) · PotInteractRadius(260) · PotHeatRadius(150) · NoticeSeconds(3) · GroupDisplayNames`.

### 콘솔 (개발 빌드, 친 플레이어의 가방 → 서버)

`Grim.Items.Give <ID> [수]` · `Spawn <ID> [수]`(앞 1.5m 픽업) · `PickUp` · `Use <칸>` · `Eat <ID>` · `Drop <칸> [수]` · `DropId <ID> [수]` · `Move <칸> <칸>` · `Fire [세기]`(가까운 픽업에 불 자극) · `SpawnFire`(앞 2m 불붙은 모닥불) · `SpawnPot`(가까운 모닥불 위 냄비) · `Pot` · `Hunger <값>` · `Clear` · `Dump` · `Reload` · `At <초> <명령> [인자…]`. 2인 확인: `Scripts/Items/net_check_items.ps1`(L_Tower, 튜토리얼 11 · 17 · 18 단계까지).

### 남은 것 (M4)

- ~~저장고 · 시설 인벤토리~~(M4 — 저장고 탭, 관련성 거리 안 모두 복제 · 열람자 복제는 측정 뒤), 무게 제한(→ 운반 일꾼의 쓸모), 아이콘 · 끌어 놓기 UI, 단축 아이템(물약 휠).
- 소지품 화면은 탭 구조다(M4): 가방 · 장비(I) · 생물(R38) · 저장고 · 작업대 · 거점(뒤 셋은 그 시설을 F 로 열었을 때만). 탭은 `UGrimInventoryTab` 하위 클래스를 ini `[/Script/Grimoire.GrimItemSettings] +InventoryTabClasses=` 또는 `UGrimInventoryWidget::RegisterTabClass` 로 더한다.
- 죽으면 가방 일부(자원)를 그 자리에 떨군다(생존 스펙 「죽음」) — `UGrimSurvivalComponent::OnDied` 에 묶을 자리.
- 불에 오래 두면 타는 것(숯). ~~생산 시설의 레시피 표 통합~~(M4 — `AGrimWorkbench` 가 `Recipes.csv` 의 `Station=Workbench` 행을 쓴다).

## 세이브

- 구현: 거점 핵 · 조각 · 시설이 저마다 세이브 참여 **스폰 액터**라 클래스 · 위치와 함께 저장되고 다시 스폰된다(World `BaseCore.<이름>` · `BuildPiece.<이름>` · `Structure` · `Container` · `CropPlot.<이름>` v1), 장비는 Player `Base.Equipment` v1 — [M4 — 세이브](./m4-plan#세이브).
- (처음 설계안) 거점 핵: 조각 목록(PieceId, Transform, Health), 시설 대기열, 저장고 인벤토리, 밭 상태(Growth, Wet) — 거점 단위로 직렬화.

## M4 범위

바닥 · 벽 · 지붕 · 문 · 계단 · 기둥 6종(나무 · 돌) + 침대 + 모닥불 + 저장고 + 작업대 + 밭 1종 + 작물 2종, 장비 · 건축 내구도 · 「수리」. 일꾼 연동은 M5.
구현(클래스 · 표 · 조작 · 콘솔 · 2인 확인)은 [M4 작업 계획 — 거점](./m4-plan). 위 설계와 달라진 것: 조각 · 작물 · 장비 정의는 DataAsset 이 아니라 CSV 표(`BuildPieces.csv` · `Crops.csv` · `Equipment.csv`), 밭 액터는 `AGrimFarmPlot`(작물 컴포넌트 없이 액터 하나), 지붕 판정은 전용 채널 대신 날씨의 기존 위로 트레이스(WorldStatic · WorldDynamic), 작업대는 대기열 없이 즉시 제작, 건축 피해는 GAS 가 아닌 `UGrimStructureComponent::ApplyStructureDamage`.
