# M4 작업 계획 — 거점

> 목표: **비 오는 밤을 집 안에서 보낸다.** 거점 자리에서 작은 집을 짓고, 비 오는 밤에 그 안의 침대에서 자서 밤을 넘긴다 — 리슨 서버 + 클라이언트 두 사람 모두. ([로드맵](./roadmap) M4)
>
> 스펙: [하우징 · 거점](../spec/housing) (R11 · R18 · R30 · Q33), [장비](../spec/equipment) (R18 · Q21 · Q22), [생존 — 잠](../spec/survival#잠-·-밤-건너뛰기). 설계: [건축 · 농사 · 생산](./building).

## 범위

| 묶음 | 들어가는 것 | 빠지는 것(다음) |
|---|---|---|
| **거점 핵** | `AGrimBaseCore` — 놓으면 반경 `BaseRadius` 안이 거점, 격자의 원점 · 방향. 플레이어당 `MaxBasesPerPlayer`(1) | 서클로 늘리기(최대 3), 권한 UI, 부활 지점 |
| **건축 조각 6종** | 바닥 · 벽 · 지붕 · 문 · 계단 · 기둥 — 나무 · 돌 두 재료(표 행 12개). 3 m 격자 스냅(Q33), 지지 규칙, 무너짐 | 반 칸(1.5 m), 6 m 큰 문, 지형 따라 바닥 높이 |
| **시설 4종 + 밭** | 저장고(상자) · 작업대 · 침대 · 모닥불 · 밭 — 격자 없이 자유 배치(R30) | 가마 · 요리대 · 완드 공방 · 각인대(M5) |
| **농사** | 밭 + 작물 2종(곡물 · 마나초): 물(젖음) · 빛(낮) · 온도(추위 등급) · 재 비료, F 로 심기 · 거두기 | 바람 · 땅 마법 수확, 일꾼 급수(M5) |
| **인벤토리 완성** | 탭 구조(가방 · 장비 · 저장고 · 작업대 · 거점), 저장고 옮기기, 작업대 제작(즉시) | 무게 제한, 아이콘 · 끌어 놓기, 물약 휠 |
| **내구도 · 수리(R18)** | 장비 슬롯 8(Q21) · 내구도 · 닳기(완드 = 시전, 방어구 = 맞음, 장신구 = 시간) · 부러진 완드(Q22) · 저항 보정, 건축 · 시설 내구도 · 노후(안 느리게 · 밖 빠르게) · 불에 탐 · 부서지면 잔해, 「수리」 마법(스킬북) · 거점 수리 | 마법탑 NPC 대리 수리, 같은 장비 합치기 · 인챈트(M5) |

## 클래스

모두 `Source/Grimoire/Base/` (인벤토리 탭 틀만 `Items/`). 이름 충돌을 피하려고 파일 안 도우미는 `GrimBase<파일>Local` 이름공간에 둔다.

| 클래스 | 하는 일 | 복제 · 세이브 |
|---|---|---|
| `FGrimBaseDatabase` · `UGrimBaseSettings` | 표 세 개(`BuildPieces.csv` · `Crops.csv` · `Equipment.csv`)를 읽고 검사, 수치 설정(ini) | — |
| `FGrimBuildGrid` · `FGrimBuildSlot` | 격자 계산(순수): 조준점 → 칸 · 모서리 · 꼭짓점, 칸 → 월드 변환, 지지 규칙, 무너질 조각 찾기 | — |
| `FGrimDurabilityMath` · `FGrimCropMath` | 내구도 · 수리 · 노후 · 작물 생장 계산(순수) | — |
| `UGrimStructureComponent` | 건축 조각 · 시설의 내구도(조각 ID · 체력 · 노후 · 불 피해 · 수리 · 부서짐 → 잔해) | 체력 복제, World `Structure` v1 |
| `AGrimBuildPiece` | 바닥 · 벽 · 지붕 · 문 · 계단 · 기둥. 모양은 표의 범주 + 크기로 런타임에 상자를 만든다(아트 전). 문은 F 로 열고 닫는다. 나무는 `Material.Flammable` | 조각 ID · 칸 · 문 열림 복제, World `BuildPiece.<이름>` v1 |
| `AGrimBaseCore` | 거점 핵. 격자 원점 · 반경 · 주인 PlayerId. 「거점 수리」 | 복제, World `BaseCore.<이름>` v1 |
| `AGrimBaseBed` · `AGrimBaseCampfire` | 기존 `AGrimBed` · `AGrimCampfire` 에 구조 컴포넌트를 붙인 지은 판(잠 · 열원 · 비에 꺼짐은 그대로) | 침대는 기존 세이브 + 구조, 모닥불은 속성 상태 + 구조 |
| `AGrimBaseCreatureBox` | 기존 거점 생물 상자 `AGrimCreatureBox`(R38)에 구조 컴포넌트를 붙인 지은 판(조각 표 `CreatureBox`). 부서지면 생물을 다른 상자 → 주인 보관함으로 옮긴다([생물 시스템](./creatures#거점-생물-상자-agrimcreaturebox)) | 상자 세이브(`CreatureBox.<이름>`) + 구조 |
| `AGrimStorageChest` · `UGrimItemContainerComponent` | 저장고 — 칸(복제 배열, 24칸), 가방과 옮기기 | 칸 복제(관련성 거리 안 모두), World `Container` v1 |
| `AGrimWorkbench` | 작업대 — `Recipes.csv` 의 `Station=Workbench` 레시피를 즉시 만든다. 「수리」 마법을 작업대에 걸면 닳은 장비를 고친다 | 복제 |
| `AGrimFarmPlot` | 밭(2 × 2 m). 씨앗 · 생장 · 수분 · 재 · 거두기. 젖음은 속성 상태(비 · 물 마법), 불 → 작물 소실 + 재. 튜토리얼의 `AGrimCropPlot`(작물 자리 — L_Tower 에 놓임)은 이름이 맵에 저장돼 있어 그대로 두고 새 이름을 썼다 | 작물 · 생장 복제, World `CropPlot.<이름>` v1 |
| `UGrimBuilderComponent` | PlayerState 에 붙는다(런타임). 건축 모드(로컬 고스트 · 키), 상호작용(F), 모든 서버 RPC(놓기 · 철거 · 문 · 저장고 · 제작 · 심기 · 거두기 · 수리) | 소유자 RPC |
| `UGrimEquipmentComponent` | PlayerState 에 붙는다(런타임). 장비 8칸, 입기 · 벗기, 닳기, 부러진 완드 · 빈 완드 칸 태그(Q71), 시작 장비(기본 완드), 저항 GE | 칸 소유자 복제, Player `Base.Equipment` v2 |
| `UGrimBaseSubsystem` | 두 컴포넌트를 PlayerState 에 붙이고, 거점 · 조각 목록을 든다(칸 점유 · 무너짐) | 서버 · 클라 |
| `AGrimRepairProjectile` | 「수리」 마법 볼트 — 맞은 조각 · 시설을 고치고, 작업대면 장비, 거점 핵이면 거점 수리 | 마법과 같음 |
| `UGrimBaseWidget` | 건축 모드 패널(조각 목록 · 비용 · 가진 수 · 유효), 상호작용 안내, 알림 | 로컬 |
| `UGrimInventoryTab` (Items) | 인벤토리 탭 틀 — ini `InventoryTabClasses` 또는 `UGrimInventoryWidget::RegisterTabClass` 로 등록 | 로컬 |

## 데이터 (표 = 정본, 런타임에 읽는다)

아이템 표와 같은 방식이다(DataAsset 대신 CSV — 「제안」, [결정 기록](../spec/decisions)). 콘솔 `Grim.Base.Reload`, 검사 테스트 `Grimoire.Base.Table`.

- `Content/Grimoire/Data/BuildPieces.csv`: `Category(Core · Foundation · Wall · Door · Roof · Stair · Pillar · Station) · Material(Wood · Stone · None) · Cost("Wood:4|Stone:1") · MaxHealth · RepairItem · Extent · Color · StationClass(시설이면 스폰할 클래스) · bFlammable`.
- `Content/Grimoire/Data/Crops.csv`: `SeedItem · HarvestItem · HarvestCount · SeedReturn · GrowSeconds · Stages · DryGrowthScale · NightGrowthScale · MaxColdGrade · MaxHeatGrade · MoistureSeconds · AshBonus · Color`.
- `Content/Grimoire/Data/Equipment.csv`: `Slot(Wand · Head · Body · Cloak · Hands · Feet · Ring · Necklace) · MaxDurability · RepairItem · WearPerUse · ColdResist · HeatResist`.
- `Items.csv` 에 재료(`Wood` · `Stone`) · 씨앗(`GrainSeed` · `ManagrassSeed`) · 수확물(`Managrass`, 곡물은 기존 `Grain`) · 장비(`Wand_Basic` 기본 완드(Q71 — 시작 장비 · 마법탑 10 솔) · `Wand_Oak` · `FurHood` · `HideRobe`) · `Skillbook_Repair` 행, 종류에 `Equipment` · `Seed` 를 더한다. `Recipes.csv` 에 작업대 레시피 3개.
- 「수리」 마법 `DA_Spell_Repair`(무속성 1서클 볼트, 마나 15) — `Scripts/Base/make_repair_spell.py`(헤드리스).

## 격자 · 지지 (Q33)

- 원점 = 거점 핵 위치 · 방향. 칸 300 cm, 벽 높이 300 cm, 층 높이 = 벽 + 지붕 판 두께(20) = 320 cm. 바닥 윗면 = 원점 + 30 cm(바닥은 아래로 100 cm — 낮은 경사를 덮는다).
- 칸 `(i, j, k)` · 모서리 `(i, j, k, 방향)`(방향 0 = 칸 i 와 i+1 사이, 1 = j 와 j+1 사이) · 꼭짓점 `(i, j, k)` = 칸 (i, j) 의 +X +Y 모퉁이.
- 조준점 → 칸: 가까운 칸 중심, 벽 · 문은 그 칸에서 가장 가까운 모서리, 기둥은 가장 가까운 꼭짓점. 층 = 조준 높이로.
- 지지: 바닥은 0층만(땅). 벽 · 문 · 기둥 · 계단은 0층이면 이웃 칸에 바닥, 그 위층이면 아래층 지붕. 지붕은 그 칸 모서리의 벽 · 문 또는 꼭짓점 기둥, 또는 이웃 지붕(한 칸 내밀기). 받침이 사라지면 받침 없는 조각이 차례로 무너진다(잔해 떨굼).
- 구조 조각은 **거점 안에서만**(격자가 거점 핵 기준) — 「제안」. 시설(모닥불 · 침대 · 상자 · 작업대 · 밭)은 거점 밖에도 놓을 수 있고, 밖에서는 빨리 닳는다(R30).

## 내구도 · 수리 수치 (`UGrimBaseSettings`, ini)

| 값 | 기본 | 뜻 |
|---|---|---|
| `RepairFractionPerMaterial` | 0.25 | 재료 1개 = 최대 내구도 25%(R18) |
| `EquipmentMaxLossPerRepair` · `EquipmentMinMaxFraction` | 0.1 · 0.5 | 장비는 수리마다 최대치 −10%, 하한 50%(스펙 제안). 건축은 줄지 않는다(제안) |
| `InsideDecayFractionPerHour` · `OutsideDecayFractionPerHour` | 0.004 · 0.5 | 거점 안: 실제 하루 ≈ 10%, 밖: 2시간이면 부서짐(R30 제안 그대로) |
| `bDecayEnabled` · `StructureDamageScale` · `StructureMaxHealthScale` | True · 1 · 1 | 서버 옵션(팰월드 설정과 같은 자리) |
| `FireDamagePerSecond` | 6 | 불 세기 1 당 초당 피해(나무만 탄다) |
| `RubbleFraction` | 0.5 | 부서지면 비용의 절반을 잔해로 |
| `BaseRepairManaCost` | 40 | 거점 수리 마나(「마나 많이 듦」) |
| `bRequireRepairSpell` | True | 「수리」를 배워야 고친다(R28). 마법탑 대리 수리는 다음 묶음 |
| `StartingEquipment` | `Wand_Basic` | 처음 들어온 플레이어가 입고 시작(Q71 — 완드 칸이 비면 주먹질) |

## 복제 · 서버 권위

- 놓기 · 철거 · 문 · 저장고 · 제작 · 심기 · 거두기 · 수리는 모두 `UGrimBuilderComponent` 의 `Server_*` RPC → 서버가 거리(`InteractRadius + ServerReachSlack`) · 재료 · 격자 · 지지를 다시 검사한다.
- 조각은 `PieceId` · 칸만 복제하고 모양은 양쪽이 표에서 만든다. 체력은 구조 컴포넌트가 복제한다(휴면 사물은 바꾸기 전에 `FlushNetDormancy`).
- 고스트는 로컬 전용 액터(`AGrimBuildGhost`, 콜리전 없음, 세이브 대상 아님).
- 저장고 칸은 관련성 거리 안 모두에게 복제한다(열람자만 복제는 측정 뒤 — 제안).

## 세이브

- 지은 조각 · 시설 · 거점 핵은 스스로 세이브 참여자인 **스폰 액터**라 세이브 시스템이 클래스 · 위치와 함께 저장하고 다시 스폰한다([세이브](./world-multiplayer#세이브)). 구조 · 속성 상태 · 저장고 칸은 그 액터의 컴포넌트 몫으로 같이 저장된다.
- 장비: Player `Base.Equipment` v2(형식은 v1 그대로 — v1 을 읽으면 빈 칸에 시작 장비를 넣는다: v1 시절엔 완드 칸이 비어도 보이지 않는 기본 완드로 시전했으므로, Q71). 아이템 인스턴스에 `RepairCount` 를 더했다 — 옛 형식과 같은 바이트로 쓰고, 수리한 적이 있을 때만 인챈트 수의 높은 비트로 표시하고 뒤에 붙인다(옛 세이브 그대로 읽힘).

## 조작 (PlayerController 키 상태 — 침대 · 가방과 같은 방식)

| 키 | 하는 일 |
|---|---|
| **B** | 건축 모드 켜고 끄기(`State.BuildMode` — 그리모어를 거두고 시전 · 기본 공격을 막는다, `CastBlockingTags`) |
| **Z · X** | 이전 · 다음 조각 |
| **R** | 90° 돌리기 |
| **좌클릭** | 놓기 |
| **G** | 바라보는 조각 철거(재료 절반 환급) |
| **F** | 상호작용: 문 열고 닫기 · 저장고 열기 · 작업대 · 밭 심기 · 거두기 · 거점 핵 패널 |
| **L** | 소지품(가방 · 장비 · 저장고 · 작업대 · 거점 탭) |

## 테스트 (`Grimoire.Base.*`)

`Table`(표 검사) · `Grid.Snap` · `Grid.Support` · `Grid.Collapse` · `Durability.Repair` · `Durability.Equipment` · `Durability.Decay` · `Crop.Growth` · `Container.Transfer` · `Craft` · `Shelter`(테스트 월드에 바닥 · 벽 · 지붕을 놓고 날씨 지붕 판정).

## 불 · 나무 (R16)

- 나무 조각은 `Material.Wood`(새 재질). 반응 표 `Wood_Ignite`: **세기 1 이상의 불**(불 마법 · 타는 옆 조각)에만 붙어 20초 타고 2 m 안 나무 조각으로 번진다. 모닥불 · 양초의 곁불(세기 × 0.5)로는 붙지 않는다 — 처음엔 `Material.Flammable` 을 썼더니 집 안 모닥불이 나무 바닥 · 벽을 1분 안에 태웠다(2인 확인). 타는 동안 구조 컴포넌트가 초당 `FireDamagePerSecond × 세기` 를 깎는다.
- 밭은 작물이 있을 때만 `Material.Flammable` — 불이 붙으면 작물이 없어지고 재가 남는다(흙으로 덮어 끈다).

## 복제 함정

- 조각의 상자 컴포넌트는 런타임에 만들지만 이름(`Shape<n>`)을 양쪽이 같게 주고 `SetNetAddressable` 한다. 이름 없는 런타임 컴포넌트 위에 캐릭터가 서면 이동 기반(MovementBase)을 복제하지 못해 「NOT Supported」 경고가 쏟아지고 이동 보정이 깨졌다(2인 확인에서 발견).

## 콘솔 (개발 빌드, 친 플레이어의 건축 컴포넌트 → 서버)

`Grim.Base.Core [거리]` · `Place <조각> <X> <Y> <층> <방향>` · `PlaceAt <시설> <X> <Y> [로컬 X] [로컬 Y] [90° 방향]` · `Teleport <X> <Y> [층]` · `Demolish · Door · Repair · Damage · SetHealth <액터 이름> [값]` · `RepairNearest <Workbench|Core>` · `LearnRepair` · `Store · Take <아이템> [개수]` · `Craft <레시피>` · `Plot` · `Grow <초>` · `Equip <아이템>` · `Unequip · Wear <칸> [양]` · `Collapse` · `Dump` · `At <초> <명령>` · `AfterJoin <사람 수> <초> <콘솔 명령 …>`(접속자가 차면 그때부터 잰다 — 헤드리스 2인 확인의 시계). 표 다시 읽기 `Grim.Base.Reload`.

로컬(서버로 안 감): `Grim.Base.Bag <Equipment|Creatures|Storage|Workbench|Base|Close>` — 소지품 화면을 그 탭으로 연다(시설 탭이면 가장 가까운 그 시설을 F 로 연 셈). 화면 찍기용(`Grim.At <초> Shot showui`).

## 2인 확인 — `Scripts/Base/net_check_base.ps1`

리슨 서버 + 클라이언트 헤드리스(L_M1_Sandbox — 거점 핵은 호스트 앞 12 m, 샌드박스 소품을 피해). 명령은 모두 `Grim.Base.AfterJoin 2` 로 「둘 다 들어온 때」부터 잰다. 호스트가 거점 핵을 놓고, 두 사람이 나눠서 바닥 2 · 벽 5 · 문 1 · 지붕 2 · 침대 2 · 모닥불 1 · 상자 1 · 작업대 · 밭을 짓는다. 비 · 22시로 바꾸고 둘이 집 안 침대에 누우면 밤을 건너뛴다. 덤프에서 볼 것: 양쪽에 같은 조각 수 · 체력, 집 안 `sheltered=1 wet=0`, 집 안 모닥불은 비에도 켜짐, 밤 건너뜀(time≈0.19), 저장고 옮기기 · 제작 · 수리 결과.

## 완료 기준

- 빌드 성공, `Automation RunTests Grimoire` 전부 통과.
- `net_check_base.ps1` 에서 호스트 · 클라이언트 모두 짓기 → 놓기 → 잠이 된다.
- 사람 눈으로 볼 것: 고스트 색 · 위치, 건축 패널 글꼴, 문 · 계단 걷기, 인벤토리 탭.

## 확인 결과 (2026-11-06)

- 빌드 성공, 자동화 테스트 154개 통과(새 `Grimoire.Base.*` 13개).
- `net_check_base.ps1`: 양쪽에 같은 조각 10 · 지은 것 17(체력 · 자리 · 문 상태 같음). 비(weather=3) · 22시에 두 사람 모두 집 안 `sheltered=1 wet=0`, 집 안 모닥불 `burning=1 sheltered=1`, 밖의 밭 `wet>0`. 둘이 각자 침대에 누워 밤 건너뜀(time 0.921 → 0.194, day 1). 그 뒤 문 열기 · 저장고(클라이언트 넣기 5 → 호스트 꺼내기 3 → 남은 2) · 털 두건 제작 · 입기(추위 저항 +1) · 100 닳게 → 작업대 「수리」(털 3개, 101/108 · 수리 1번) · 벽 피해 100 → 「수리」(나무 3, 150/150) · 밭 심기 → 비에 젖음 → 거두기(곡물 3 · 씨앗 1) · 문 철거(나무 2 환급, 지붕은 그대로) 모두 됐다.
- 사람 눈으로 볼 것(헤드리스로 못 본 것): 고스트 위치 · 색, 건축 패널 · 탭 글꼴 · 클릭, 좌클릭 놓기가 기본 공격과 겹치지 않는지(`State.BuildMode` 시전 차단), 문 · 계단 걷기, 「수리」 볼트가 조각에 맞는지(마법 경로 — 콘솔은 같은 함수를 바로 부른다).

## R38 생물 보관과 합친 뒤 (2026-11-06)

- 소지품 「생물」 탭 = `UGrimCreatureStorageTab`(ini 한 줄, 순서 15 — 장비 다음), 지은 「생물 상자」 = 조각 표 `CreatureBox` → `AGrimBaseCreatureBox`. 탭 틀 `UGrimInventoryTab::InitializeTab` 본문을 .cpp 로 옮겼다(헤더만 넣은 탭 · 테스트에서 약한 포인터 대입이 빌드되지 않았다).
- 자동화 테스트 168개 통과(새 `Grimoire.Base.CreatureBox.Data` · `.Rescue`). `net_check_storage.ps1` `NETCHECK_RESULT fail=0`, `net_check_base.ps1` 결과는 위 「확인 결과」와 같다(조각 10 · 지은 것 17, 밤 0.921 → 0.194, 저장고 남은 나무 2, 털 두건 101/108, 벽 150/150, 곡물 3, 문 철거 뒤 9 · 16).
- 창 모드 화면 찍기 `Scripts/Base/capture_base.ps1`(`Grim.Base.Bag <탭>` · `shot showui` · `HighResShot`, 거점 핵은 `BugItGo` 로 늘 (1200, 0)) → `Saved/BaseCapture/`. 탭 6개 · 생물 상자 화면 · 집 바깥(문 닫힘 · 열림)이 다 그려졌다.
- 화면에서 본 것(고칠 것): 가방 「사용 (먹기 · 읽기 · 입기)」 글이 단추(160 px)를 넘친다, 작업대 레시피 줄이 두 줄로 접혀 아랫줄이 잘린다, 저장고 칸에서 「재칼로프 고기(날) ×3」이 칸 높이를 넘친다, 집 안 두 칸 벽 이음새로 세로 빛줄기가 샌다, 생물 상자(기본 도형)가 반쯤 땅에 묻혀 보인다(상자 몸 원점이 가운데).
- QA 항목 · 결과: [QA — M4 거점 + 생물 보관 통합](../qa/m4-base)(지은 생물 상자의 넣기 · 풀어놓기 · 철거 · 저장 뒤 다시 켜기는 `Scripts/Base/qa_creature_box.ps1`, `QA_RESULT fail=0`).
- 워크트리 함정: 새 워크트리는 LFS 에셋이 포인터 그대로일 수 있다 — `git lfs checkout`(로컬 캐시)을 하지 않으면 에셋을 읽는 테스트가 실패하거나 죽는다.
