# 초원 꾸밈 · 작은 흥미지점 — 구현

> 관련 요구사항: R16(모든 상호작용은 마법으로) · R03 · R04 · R14. 기획(무엇)은 [스펙 — 초원 꾸밈](../spec/meadow-dressing), 이 문서는 어떻게 만들었나다.
>
> 상태: **1차 묶음 구현(2026-11-18)** — 요소 11(D01 · D02 · D03 · D04 · D10 · D16 · D20 · D21 · D26 · D27 · D29)과 공통 기반 둘(반응 소품 공통 액터 · 채집 셀 관리자)을 `L_OpenBeta` 마탑 구간에 놓았다. 줍기 · 보상 · 다시 나기 · 불 번짐 값은 기획안 제안을 쓴 **잠정값 — Q151~Q157** 이며 모두 표(CSV)에 있다.

## 한 줄로

소품마다 클래스를 만들지 않는다. **반응 소품 공통 액터 하나**(`AGrimReactiveProp`)가 표의 한 행(`PropId`)으로 생김새 · 보상 · 되돌리기를 정하고, 불 · 물 · 바람 · 충격 반응은 [속성 상태 단일 시스템](./magic#속성-상호작용-엔진-r16)(`UGrimElementalStateComponent` + `ElementalReactions.csv`)이 한다. 줍는 것은 **셀(256 m)마다 액터 하나**(`AGrimHarvestCell`)가 ISM 과 「거둔 점」 FastArray 로 맡는다.

## 구조

| 클래스 | 하는 일 | 복제 | 구현 |
|---|---|---|---|
| `AGrimReactiveProp` | 반응 소품 공통 액터. `AGrimElementalObject`(몸 · 불꽃 · 빛 · 상태 컴포넌트 · 소프트 락온)를 이어받아 표의 메시 · 재질 · 맞는 상자(HitBox) · 바뀐 모습 · 덧붙은 메시 · 고리/한 번 이펙트 · 보상 · 되돌리기 · 바람에 날려 감 · 튀어 오름 · 수집(F)을 입힌다 | 휴면(DormantAll). `PropId` · `JournalOverride`(처음만) · `CollectedBy` · 자리(날려 갈 때만 깨운다) · 상태는 상태 컴포넌트의 FastArray | `Source/Grimoire/Dressing/GrimReactiveProp.h` |
| `AGrimHarvestCell` | 채집 셀 관리자. 점(종류 · 변환)은 레벨 데이터, 종류별 ISM 으로 그리고, 거둔 점만 FastArray 로 복제. 공유 · 사람별 · 여러 번 쳐야 하는 것 · 아침/시간 재생 | 휴면, `Points`(처음만) · `Harvested`(FastArray). 늘 로드(`bIsSpatiallyLoaded` 끔 — 16 개 남짓) | `Source/Grimoire/Dressing/GrimHarvestCell.h` |
| `UGrimJournalComponent` | 도감 「발견」 임시 목록(도감 탭 전). 새 줄은 HUD 위쪽 알림, 쪽(Page)이 설정만큼 찰 때마다 한 번 보상 | 소유자만 | `Source/Grimoire/Dressing/GrimJournalComponent.h` |
| `UGrimDressingSubsystem` | 서버: PlayerState 가 생기자마자 도감을 붙인다(가방 `UGrimItemSubsystem` 과 같은 방식 — PlayerState 파일을 고치지 않는다) | — | `Source/Grimoire/Dressing/GrimDressingSubsystem.h` |
| `IGrimInteractable` · `UGrimInteractableSubsystem` | F 대상 약속(Core). 픽업 · 냄비가 없을 때 가방 위젯이 가장 가까운 대상을 고르고 `UGrimInventoryComponent::Server_Interact` 로 서버가 거리를 다시 재고 한다 | — | `Source/Grimoire/Core/GrimInteractable.h` |
| `UGrimDressingSettings` | 표 경로 · 줍기 거리 · 쪽 보상(잠정 — Q152) | — | `Source/Grimoire/Dressing/GrimDressingSettings.h` |

상태 컴포넌트에 `UGrimElementalStateComponent::ResetStates`(규칙을 거치지 않고 처음 상태로 — 아침 되돌리기)를 더했다.

### 상태 → 모습 · 보상

1. 서버: 자극 → 규칙표 → 상태 변화(복제) → 소품의 `OnStateChangedNative` 처리기
   - `RewardState` 가 새로 생기면 `Rewards`(「아이템:최소-최대:무게|…」 + `NothingWeight`)에서 하나를 굴려 픽업으로 놓는다.
   - `DriftState`(책장 State.Blown)가 새로 생기면 가장 가까운 플레이어 반대쪽(대개 시전자)으로 `DriftDistance` 만큼 옮기고 바닥에 앉힌다.
   - `ResetRule` = `Dawn` 이면 해 뜸(`UGrimTimeOfDaySubsystem::OnNightEnd`)에 처음 상태로, `Seconds` 면 처음과 달라진 뒤 `ResetSeconds`.
2. 모두(서버 · 클라이언트): `AltState` 가 있으면 `AltMesh` · `AltMaterial` · `AltScale` · `AltRotation` · `bAltHidesBody`, 덧붙은 메시는 `AccentShow`(Always · Default · Alt), 고리 이펙트는 `LoopFXWhen`(Always · Day · Night · Alt) 이고 `LoopFXHideState` 가 있으면 끈다. `BurstState` 가 새로 생기면 `BurstFX` 한 번, `KickState` 면 몸이 튀어 올랐다 돌아온다(그 1.2초만 틱).
3. 불(State.Burning)은 공통 액터의 원뿔 불꽃 · 점광(그림자 끔 · `LightMaxDrawDistance` 30 m). 불 붙은 소품은 생존의 열원 규칙(둘레 불)과 요리(픽업 곁 불)에 그대로 잡힌다.

### 반응 규칙(ElementalReactions.csv 에 더한 다섯 행)

모두 `Water_Soak` 위(처음 맞는 행 하나만 — 시든 꽃이 물에 「젖음」 대신 「핌」). 재질 태그 · 상태 태그는 `Source/Grimoire/Dressing/GrimDressingTags.h`.

| 행 | 재질 | 자극 | 결과 |
|---|---|---|---|
| `Seedhead_Scatter` | `Material.Seedhead` | 바람 0.5 이상(젖었으면 위의 `Wind_Dry` 가 먼저 말린다) | `State.Spent`(지속) |
| `Thirsty_Revive` | `Material.Thirsty` + `State.Wilted` | 물 0.3 이상(비 0.5 포함) | Wilted 지우고 `State.Bloomed` |
| `Paper_Blown` | `Material.Paper` | 바람 1.0 이상 | `State.Blown` 1.5초 |
| `Swarm_Scatter` | `Material.Swarm` | 바람 0.3 이상 | `State.Scattered` 20초 |
| `Floating_Struck` | `Material.Floating` | 충격(무속성 `Element.None`) | `State.Struck` 1.2초 |

마른 풀(D02) · 등잔(D04) · 모닥불(D20)은 있던 행(`Grass_Ignite` · `Grass_BurnOut` · `Flammable_Ignite` · `Gust_Extinguish` · `Water_Douse`)을 그대로 쓴다 — 마른 풀은 8초 타며 2.5 m 안의 불붙는 것으로 번지고(잠정 — Q154: 마른 풀 · 불붙는 소품만, 땅의 풀밭은 안 탄다), 다 타면 `State.Charred`.

## 데이터

| 표 · 에셋 | 내용 | 만드는 것 |
|---|---|---|
| `Content/Grimoire/Data/ReactiveProps.csv` | 반응 소품 8 종(행 = PropId) — 몸 · 상자 · 재질 태그 · 처음 상태 · 바뀐 모습 · 덧붙은 메시 · 이펙트 · 불 · 보상 · 되돌리기 · 움직임 · 도감 | `Scripts/Dressing/make_meadow_tables.py`(값의 정본, CSV 를 손으로 고쳐도 된다) |
| `Content/Grimoire/Data/HarvestNodes.csv` | 채집점 종류(곁가지 셋 · 돌멩이 둘) — 메시 · 아이템 · 개수 · 친 횟수 · 소유 · 재생 · 닿는 거리 | 같은 스크립트 |
| `Content/Grimoire/Data/Journal.csv` | 도감 줄(책장 20 쪽 「들판 일지」 — 잠정, Q157) | 같은 스크립트 |
| `Content/Grimoire/Data/Items.csv` | 새 재료 `WindCotton`(솜씨앗) · `WildPetal`(들꽃잎) · `Ash`(재) | 손 |
| `Content/Grimoire/Dressing/` | 메시 `SM_Meadow_*` 8(블렌더) · 재질 `M_Meadow_Painted` · `M_Meadow_FloatStone`(WPO — 떠서 돈다) · `M_Meadow_Butterfly`(날갯짓 스프라이트) · `M_Meadow_Puff` · 색만 바꾼 `MI_Meadow_*` · `NS_Meadow_Butterflies` · `NS_Meadow_CottonBurst` · 폴리지 타입 `FT_Meadow_Blossom*`(3) · `FT_Meadow_Flower*`(8) | `art/tools/blender_meadow_kit.py` → `art/tools/make_meadow_textures.py` → `Scripts/Dressing/make_meadow_assets.py` |
| `Content/Grimoire/Environment/FX/EFT_Grim_Ambient` | 환경 이펙트 Effect Type — 거리 컬 60 m · 전체 40 · 시스템당 24 · 멀어지면 끄고 다가오면 다시(성능 규칙 24, [BUG-006](../qa/bugs) 의 환경 몫) | `Scripts/Dressing/make_meadow_assets.py` |

설정(`Config/DefaultGame.ini` 에 섹션이 없으면 기본값): `[/Script/Grimoire.GrimDressingSettings]` — `ReactivePropsCSV` · `HarvestNodesCSV` · `JournalCSV` · `CollectRadius`(220) · `ServerReachSlack`(150) · `PageMilestoneEvery`(5) · `PageMilestoneItem`(ManagrassSeed) · `PageMilestoneCount`(2).

에셋은 사 오지 않았다(art-direction 「리소스 만드는 길」): 등잔 · 솜꽃 · 곁가지 · 재 자국 · 장작은 블렌더 스크립트, 나비 · 솜 텍스처는 PIL, 나머지는 이미 쓰던 StyleHex 메시의 색만 바꾼 재질 인스턴스와 `EnchantedPotionRoom`(4 MB) 모닥불 돌 · 종이. Tripo 는 쓰지 않았다(크레딧 0).

### 잠정값(Q151~Q157 — 표에서 바꾼다)

| Q | 지금 넣은 값 | 어디 |
|---|---|---|
| Q151 줍기와 R16 | 땅에 놓인 것(곁가지 · 돌멩이 · 책장)은 F, 달린 것(솜 · 꽃잎)은 마법으로 떨군 뒤 F(픽업) | `HarvestNodes.csv` · `ReactiveProps.csv` 의 `Rewards` |
| Q152 모으면 무엇을 | 새 화폐 없이 책장 5 쪽마다 마나초 씨앗 2 | `UGrimDressingSettings` `PageMilestone*` |
| Q153 다시 나기 · 멀티 소유 | 자원 · 반응은 다음 아침에 다시(세계 공유), 책장은 사람별 한 번 | `ResetRule` · `Ownership` · `CollectedBy` |
| Q154 불 번짐 | 마른 풀 → 2.5 m 안 불붙는 것만. 땅 풀밭은 안 탄다 | `ElementalReactions.csv` `Grass_Ignite` |
| Q155 에셋 | 팩(StyleHex · EnchantedPotionRoom) 먼저, 나머지는 블렌더 | 위 표 |
| Q156 마법 흔적 | 떠 있는 돌 14 · 나비 · 솜만(시골 쪽이 많다) | 배치 수 |
| Q157 누가 살았나 | 책장 20 쪽 — 탑을 드나들던 길손 · 떠난 농가 · 사냥꾼의 옅은 복선 | `Journal.csv` |

## 배치 (`Scripts/Dressing/meadow_place.py`)

`L_OpenBeta` 를 열어 지난번 것(태그 `MeadowDress` 액터 · `FT_Meadow_*` 인스턴스)을 걷고, 시드 고정 규칙으로 다시 놓는다(손 배치 없음). 같은 시드면 같은 자리.

- **비우는 곳**: 남쪽 바닷가(해안선 북쪽 20 m · 땅 높이 4 m 아래 — 구간 다각형이 모래까지 들어온다), `dress_openbeta.clear_of` 와 같은 길 · 개울 · 연못 · 랜드마크 · POI · 거점 터 · 측정 자리 + **마법탑 둘레 260 m**(탑 실외 · 앞마당 · 석등 줄은 다른 브랜치가 고치는 중) + 전망대 터(`Pads`) + 맵의 돌담 돌 · 바위 · 통나무 · 블록아웃 소품 · 나무 둥치(폴리지 인스턴스 자리를 읽어 잰다).
- **분포 규칙**(스펙 「4」): 같은 종류의 멈추는 것은 120 m 안에 하나(꽃밭 60 · 책장 70 · 꽃나무 150), 등잔만 줄(굽이 바깥 3~5 개, 9~11 m), 떠 있는 돌은 부유석 쪽으로 갈수록 촘촘하고 옆으로 흔든다(곧은 줄 금지), 색은 구역마다 하나(서쪽 분홍 · 가운데 흰빛 · 동쪽 노랑, 물가 꽃밭만 푸른빛).
- **가르치는 순서**: 바람솜꽃 · 마른 풀 · 들꽃은 탑에서 가까운 쪽부터, 등잔은 탑에서 300~900 m 굽이 — 기획안의 「0~300 m」 띠는 탑 비움(260 m) 때문에 그 바깥으로 밀렸다.
- 끝에 `Saved/MeadowPlace/report.json`(종류별 수 · 자리)을 남긴다. `dress_openbeta.py` · `build_openbeta.py` 로 맵을 다시 만들면 이것도 다시 돌린다.

배치 수(2026-11-18 실행, 기획안 수 그대로): 바람솜꽃 16 · 마른 풀 24 · 목마른 들꽃 12 · 돌 등잔 26(굽이 7 곳) · 나비 18 · 식은 모닥불 10 · 책장 20(넷은 모닥불 곁 — 장면 S1) · 떠 있는 돌 14 · 곁가지 40 · 돌멩이 40(셀 17) · 꽃나무 10 · 꽃 띠 30(폴리지 524). 맵 외부 액터 +157. 처음 규칙(120 m 간격 · 길가 3~12 m · 굽이 18° 이상)으로는 마른 풀 15 · 등잔 3 곳 · 모닥불 8 · 떠 있는 돌 4 라, 마른 풀은 길에서 150 m 까지 · 그래도 모자라면 90 m 간격, 등잔은 굽이 10° 이상 · 탑에서 1 km 까지, 떠 있는 돌은 방향 ±30° 로 넓혔다.

## 콘솔 · 확인

- 콘솔(개발 빌드): `Grim.Meadow Dump` · `Grim.Meadow Spawn <PropId> [앞 cm] [Host|Client|All]` · `Grim.Meadow SpawnCell <Kind> …` · `Grim.Meadow Hit <Element> <세기> [반경] [PropId]` · `Grim.Meadow Dawn` · `Grim.Meadow Interact` · `Grim.Meadow Go <PropId|Cell> [번째] [거리]` · `Grim.Meadow Reload` · `Grim.Journal` — [테스트 치트](../dev/cheats).
- 자동화: `Grimoire.Dressing.Tables`(표 · 에셋 · 보상 문법 · EFT) · `.Rewards` · `.ReactionRules`(규칙표의 다섯 행 + 마른 풀 · 등잔) · `.PropReact`(소품마다 반응 · 보상 · 아침 되돌리기 · 날려 감) · `.HarvestRespawn`(공유 · 사람별 · 두 번 · 시간 · 아침 · 세이브 왕복) · `.Replication`(복제 · 휴면 설정, 서버 → 클라이언트 콜백으로 셀 모습). 테스트는 `Source/Grimoire/Dressing/Tests/GrimDressingTests.cpp`.
- 2인: `Scripts/Dressing/net_check_meadow.ps1`(리슨 + 클라이언트 — 상태 복제 · 클라이언트 F 채집 · 사람별 책장 · 아침 재생).
- QA 항목: [QA — 초원 꾸밈](../qa/meadow).

## 결과

결과 표 · 스크린숏 · 성능은 [QA — 초원 꾸밈](../qa/meadow).

## 남은 것 · 함정

- 남은 것: 등잔을 켜 두면 검은 개가 피하는 것(D04 의 밤 의미 — 생물 쪽), 2차 묶음(D11 · D12 · D13 · D15 · D17~D19 · D22~D24 · D32)과 장면 S1 · S2 · S5 조립, 나무 · 풀 채집(Q125 — `HarvestNodes.csv` 에 행을 더하고 배치 스크립트가 폴리지 자리에서 점을 만든다. 여러 번 쳐야 하는 것은 `HitsToHarvest`).
- 함정: 팩 풀 · 꽃 메시는 충돌이 없다 — 소품은 보이는 메시에 충돌을 두지 않고 `HitBox`(WorldDynamic, 폰은 겹치기)로 마법 · 비 · 불 번짐을 받는다. 레벨에 놓인 액터는 게임에서 `OnConstruction` 이 다시 돌지 않는다 — 표 값은 `PostInitializeComponents` 에서도 입힌다. `UMaterialEditingLibrary.set_material_instance_*_parameter_value` 가 돌려주는 값은 5.8 에서 믿을 수 없다(늘 False) — 이름이 부모에 있는지로 본다.
