# 마법 · 상호작용 시스템

> 스펙: [마법](../spec/magic) (R01 · R05 · R06 · R07 · R16)

## GAS 위에 올리는 방식

| 스펙 개념 | GAS 구현 |
|---|---|
| 마법 하나 | `UGameplayAbility` 서브클래스(`UGrimSpellAbility`) + 정의 데이터 `UGrimSpellDefinition` |
| 마나 소모 · 쿨다운 | Ability 의 Cost GE · Cooldown GE(서클 · 완드 보정은 GE 의 SetByCaller) |
| 피해 · 화상 · 젖음 등 | `UGameplayEffect`(즉시 / 지속 / 주기) + 부여 태그 `State.*` |
| 속성 | GameplayTag `Element.Fire` 등. 마법 · 완드 · 생물 · GE 모두 이 태그를 단다 |
| 서클 | `Circle` 어트리뷰트 + 태그 `Circle.N`. 마법 정의의 `RequiredCircle` 과 비교 |
| 배운 마법 | ASC 의 `GiveAbility` + `UGrimSpellbookComponent` 의 목록(저장용) |
| 상성 | 피해 계산 `UGrimDamageExecution`(GameplayEffectExecutionCalculation)에서 공격 속성 × 대상 속성 → 배율 |

ASC 는 **플레이어는 `AGrimPlayerState`**, **생물은 `AGrimCreature` 자신**이 소유한다. 플레이어 캐릭터가 죽어도 서클 · 마법은 남는다.

## 마법 정의 `UGrimSpellDefinition`

`Source/Grimoire/Magic/GrimSpellDefinition.h` — `UPrimaryDataAsset`, Asset Manager 타입 `Spell`.

| 속성 | 뜻 |
|---|---|
| `DisplayName` · `Description` · `Icon` | 표시용 |
| `SpellTag` | `Spell.*` 이름 태그. 피해 GE 의 에셋 태그로 붙는다 |
| `Element` | 공격 속성 `Element.*` — 상성 계산 |
| `Circle` | 필요 서클 1~10. 배울 때 · 시전할 때 확인 |
| `AbilityClass` | 비우면 공용 `UGrimSpellAbility` |
| `ManaCost` · `Cooldown` · `CooldownTag` · `CastTime` | 비용 · 쿨다운(`Cooldown.Spell.*`, 마법마다 달라야 서로 막지 않음) · 시전 지연 |
| `Delivery` | `Projectile` · `Beam`(M1). Area · Self · Touch 는 나중에 |
| `ProjectileClass` · `ProjectileSpeed` · `Radius` · `Range` | 비우면 `AGrimSpellProjectile`. 광선은 `Radius` 구 스윕 |
| `Damage` · `ImpactEffect` | 기본 피해(0 이면 피해 없음). 비우면 `UGrimGE_SpellDamage` |
| `Stimulus` | 사물에 주는 속성 자극 `FGrimElementalStimulus`(속성 · 세기 · 출처) |
| `Color` | 투사체 색(엔진 기본 구 + `BasicShapeMaterial` 의 `Color`) |

- 에셋 `Content/Grimoire/Magic/Spells/DA_Spell_*` 은 `Scripts/Magic/make_spells.py` 가 만든다 — **스크립트가 수치의 정본**이다.
- 하나의 `UGrimSpellAbility` 클래스가 여러 마법을 낸다: 마법서가 `GiveAbility` 할 때 Spec 의 `SourceObject` 에 정의를 넣고, 어빌리티는 Spec 에서 정의를 꺼낸다.

### M1 마법 (스펙 「제안」값)

| 에셋 | 마법 | 속성 | 전달 | 마나 | 쿨다운 | 피해 | 자극 세기 | 사거리 |
|---|---|---|---|---|---|---|---|---|
| `DA_Spell_Ember` | 불씨 | 불 | 투사체 1800cm/s, 반지름 12 | 10 | 1.0s | 10 | 1.0 | 25m |
| `DA_Spell_WaterJet` | 물줄기 | 물 | 투사체 2400cm/s, 반지름 15 | 8 | 0.6s | 6 | 1.0 | 18m |
| `DA_Spell_Breeze` | 미풍 | 바람 | 광선(구 스윕 반지름 40) | 6 | 0.8s | 3 | 1.0 | 12m |

미풍 세기 1.0 은 양초 규칙(바람 0.3 이상 키움, 2.5 이상 꺼짐)의 "키움" 구간이다.

## 수치 설정 `UGrimMagicSettings`

`Config/DefaultGame.ini` 의 `[/Script/Grimoire.GrimMagicSettings]`(프로젝트 설정 > Game > Grimoire Magic).

| 키 | M1 값 |
|---|---|
| `StartingSpells` | 불씨 · 물줄기 · 미풍 |
| `StartingWandClass` · `WandSocketName` | `AGrimWand` · `hand_r` |
| `StartingCircle` · `MaxManaByCircle` | 1 · 100 / 130 / 160 / 200 / 250 / 300 / 360 / 430 / 510 / 600 |
| `ManaRegenPerSecond` | 2 |
| `bFriendlyFire` | false — 플레이어끼리는 피해 없음(속성 자극은 준다) |
| `DefaultAffinity` · `ElementAffinities` | 1.0 · 상성 표(아래) |

## 시전 흐름

1. 입력: 캐릭터가 `UGrimAbilitySystemComponent::AbilityInputTagPressed(Input.Spell.Cast)`. **선택된 마법 Spec 만** `Input.Spell.Cast` 를 동적 태그로 가지므로 그 마법만 발동한다.
2. `UGrimSpellAbility`(InstancedPerActor · LocalPredicted):
   - `CanActivateAbility` — 정의 있음 · 완드 있음(맨손 시전 없음, Q4) · `Circle` ≥ 정의의 서클 · 쿨다운 · 비용.
   - `CommitAbility` — 비용 `UGrimGE_SpellCost`(즉시, `Mana += SetByCaller.Magic.ManaCost`), 쿨다운 `UGrimGE_SpellCooldown`(지속 `SetByCaller.Magic.Cooldown` 초, `CooldownTag` 를 Spec 의 `DynamicGrantedTags` 로). 클라이언트도 예측 적용한다.
   - 완드 보정: 비용 = `ManaCost × (1 − ManaEfficiency)`, 쿨다운 = `Cooldown ÷ CastSpeed`(`GrimMagicMath`).
   - `CastTime` > 0 이면 `WaitDelay` 뒤, 아니면 바로 `Deliver()`.
3. `Deliver()` — **서버만**. 조준점 = 플레이어 시점(`GetPlayerViewPoint`)에서 `Range` 앞으로 Visibility 트레이스한 점. 완드 `Muzzle` 에서 그쪽으로:
   - 투사체: `AGrimSpellProjectile` 스폰(복제 · 위치 복제, 클라이언트 예측 스폰 없음). 지형 · 사물(WorldStatic/Dynamic)에 막히고 폰 · 물리체는 겹쳐 맞는다. 시전자는 무시. 수명 = `Range ÷ ProjectileSpeed`.
   - 광선: 구 스윕(`SweepMultiByObjectType`) — 폰 · 물리체는 지나가며 맞히고, 첫 지형 · 사물에서 멈춘다. 연출 없음(바람이라 보이지 않는다 — M2 에서 Niagara).
4. 맞은 대상마다 `UGrimSpellAbility::ApplySpellHit`:
   - 대상 ASC(액터 → 폰이면 PlayerState 순으로 찾음)가 있으면 피해 GE 적용. 피해 Spec 은 `SetByCaller.Magic.Damage = Damage × 완드 속성 위력` 과 에셋 태그 `Element.*` · `Spell.*` 를 가진다.
   - `UGrimElementalStatics::ApplyStimulusToActor(대상, Stimulus, 시전 폰)` — 받는 쪽(`IGrimElementalReceiver`)이 있으면 반응 규칙. Magic 은 구현체를 모른다.

## 마법서 `UGrimSpellbookComponent`

`AGrimPlayerState` 의 하위 객체. `BeginPlay` 에서 `OnAbilityActorInfoReady` 를 구독한다(이미 초기화돼 있으면 바로 처리).

- **처음 한 번(서버)**: `Circle = StartingCircle`, `MaxMana = Mana = MaxManaByCircle[서클]`, 마나 회복 `UGrimGE_ManaRegen`(무한 · 1초 주기 `Mana += ManaRegenPerSecond`), 선택 어빌리티 `UGrimSpellSelectAbility` 를 `Input.Spell.Next` · `Input.Spell.Prev` 태그로 두 번, `StartingSpells` 를 `LearnSpell`.
- **폰이 바뀔 때마다(서버)**: 완드가 없으면 스폰하고 `Owner = 폰`, 폰 스켈레탈 메시의 `WandSocketName`(본 · 소켓 모두)에 `SnapToTargetNotIncludingScale` 로 부착(완드 루트 = 소켓, 상대 변환 없음). 메시 · 소켓이 없으면 폰 루트. 부착은 완드의 움직임 복제로 클라이언트에 간다.
  - `KeepRelativeTransform` 을 쓰면 안 된다 — 스폰한 월드 좌표가 그대로 손 기준 오프셋이 되어 완드가 몸에서 떨어진다(M1 통합 때 실제로 난 버그).
- `LearnSpell(Spell)` — 서클 확인 → `GiveAbility(SourceObject = Spell)` → `KnownSpells` 에 추가(복제). 첫 마법이면 선택.
- `CycleSelection(±1)` · `SelectSpell(i)` — 선택 Spec 에만 `Input.Spell.Cast` 를 넣고 바뀐 Spec 을 `MarkAbilitySpecDirty`. `SelectedIndex` 복제, `OnSelectedSpellChanged`(서버 · 클라이언트) 로 HUD 갱신.
- `UGrimSpellSelectAbility` 는 ServerOnly — 입력은 클라이언트, 선택 변경은 서버, 결과는 복제.

## 마법 어트리뷰트 `UGrimMagicAttributeSet`

`Mana` · `MaxMana` · `Circle`(복제). `Mana` 는 현재값 · 기본값 모두 `[0, MaxMana]` 로 자르고, `MaxMana` 가 줄면 `Mana` 도 줄인다. `CastSpeed` · `SpellPower` · 속성별 위력 어트리뷰트는 아직 없다 — 완드 보정은 완드 액터 값을 직접 읽는다.

## 완드 `AGrimWand`

- 장비 액터(복제 · 움직임 복제). M1 모양은 엔진 기본 실린더(가늘게, 길이 35cm).
- 컴포넌트: `Root`(Scene, 변환 없음 — 소켓에 그대로 붙는다) → `Grip`(손 안에서 쥐는 자세, +X = 완드 끝) → `Mesh`(회전 · 비균일 스케일은 여기만) · `Muzzle`(Grip 아래 +X 27.5cm). 루트나 Muzzle 의 부모에 스케일을 걸지 않는다 — 자식이 찌그러진다.
- `Grip` 기본값은 마네킹 `hand_r` 축을 재서 정했다: 손가락 −X, 손바닥 +Y, 엄지 쪽 +Z → 위치 (−9, 3.5, 0), 회전 Pitch 90(완드가 주먹에서 엄지 쪽으로 나옴). 캐릭터 뼈대 · 완드 모양이 바뀌면 블루프린트 자식에서 `Grip` 변환을 고친다. 콘솔 `Grim.Magic.DumpWands [지연초]` 가 부착 부모 · 손에서 떨어진 거리 · 손 공간 축을 찍는다.
- 보정은 클래스 기본값: `ElementPower`(`TMap<Element, 배율>`, 없으면 1) · `ManaEfficiency`(0~1) · `CastSpeed`(쿨다운을 나눔). 완드 종류는 블루프린트 자식으로 만든다.
- 나중(▢): `UGrimWandDefinition`(아이템 정의의 하위) · 등급 · 내구도, 장착 시 `GE_WandBonus` 로 어트리뷰트에 싣기.

## 스킬북 · 배우기

- 스킬북은 아이템(`UGrimItemDefinition`, `ItemType = Skillbook`, `TeachesSpell`).
- 사용 → `Server_UseItem` → `UGrimSpellbookComponent::LearnSpell(SpellId)`: 서클 확인 → `GiveAbility` → 목록에 추가(복제 · 저장) → 책 소모.
- 필사(Q5): `AGrimScribeStation` 에서 아는 마법 선택 + 잉크 → 스킬북 아이템 생성. 레시피는 `DT_Recipes` 의 특수 행.

## 서클

- `UGrimMagicAttributeSet`: `Circle`, `MaxMana`(서클 함수), `CastSpeed`, `SpellPower`, 속성별 `Power_Fire` …(완드 · 버프가 더함).
- 승급: `UGrimProgressionComponent`(PlayerState)가 조건(시험 퀘스트 완료 · 소재)을 확인하고 `Circle + 1` → `Circle.N` 태그 갱신 → 그리모어 UI 갱신.
- 사냥꾼의 "금지된 승급"은 같은 컴포넌트의 다른 경로(`AbsorbMana`) — NPC 전용이지만 같은 데이터 구조.

## 속성 태그 · 상성 표

- 속성 태그 `Element.Water / Fire / Wind / Earth / Light / Dark` 는 `Core/GrimGameplayTags`(네이티브). Magic 전용 태그(`SetByCaller.Magic.*` · `Cooldown.Spell.*` · `Spell.*`)는 `Magic/GrimMagicTags`.
- 피해 계산 `UGrimDamageExecution`(`UGrimGE_SpellDamage` 의 실행): `Health −= SetByCaller.Magic.Damage × 상성(공격 속성, 대상 속성)`. 공격 속성 = Spec 에셋 태그의 `Element.*`, 대상 속성 = 대상 ASC 태그의 `Element.*`(없으면 무속성 ×1).
- 상성 표는 `UGrimMagicSettings::ElementAffinities`(ini). 표만 바꾸면 밸런스가 바뀐다(Q2 · Q3). 현재 값(스펙 제안):

| 공격 → 대상 | 배율 |
|---|---|
| 물 → 불 · 불 → 바람 · 바람 → 땅 · 땅 → 물 | ×1.5 |
| 불 → 물 · 바람 → 불 · 땅 → 바람 · 물 → 땅 | ×0.75 |
| 빛 → 어둠 · 어둠 → 빛 | ×1.5 |
| 그 외(같은 속성 포함) | ×1.0 (`DefaultAffinity`) |

- 테스트: `Grimoire.Magic.Affinity` · `Grimoire.Magic.Cost` · `Grimoire.Magic.Effects`(GE 실제 적용) · `Grimoire.Magic.StartingSpells`(에셋 값) — `Source/Grimoire/Magic/Tests/`.

## 속성 상호작용 엔진 (R16)

> 코드: `Source/Grimoire/Interaction/` · 규칙표 원본: `Content/Grimoire/Data/ElementalReactions.csv` · 테스트: `Grimoire.Interaction.*`

### 구성

| 클래스 | 하는 일 |
|---|---|
| `UGrimElementalStateComponent` | 기반 `IGrimElementalReceiver` 구현. 사물 · 생물 · 플레이어가 가진다. 서버에서 자극 → 규칙 → 상태 변화, 복제, 델리게이트 |
| `FGrimElementalState` / `FGrimElementalStateArray` | 상태 하나 `{ StateTag, Intensity, RemainingTime }`(음수 = 무한) · FastArraySerializer 배열 |
| `FGrimElementalReactionRow` | 규칙표 행(`FTableRowBase`) |
| `FGrimElementalReactionLibrary` | 규칙 조회 · 적용 · 시간 진행. 액터 · 월드를 모르는 순수 로직(테스트가 직접 부른다) |
| `UGrimElementalPropagationSubsystem` | 전파 스케줄러(TickableWorldSubsystem, 게임 · PIE 월드). 출처가 없으면 틱하지 않는다 |
| `UGrimInteractionSettings` | 전역 예산 · 상한(`DefaultGame.ini` 의 `[/Script/Grimoire.GrimInteractionSettings]`) |
| `AGrimElementalObject` → `AGrimCandle` · `AGrimCampfire` · `AGrimDryGrass` | 엔진 기본 도형 + 상태 컴포넌트 + 불꽃(원뿔) + 포인트 라이트 |
| `GrimInteractionTags` | `Material.Flammable / Consumable / Conductive / Absorbent`, `State.Charred` |

- 자극은 기반의 `FGrimElementalStimulus { Element, Intensity, Source }` 하나. 마법 · 날씨는 `UGrimElementalStatics::ApplyStimulusToActor` 로만 보낸다(구현체를 몰라도 된다).
- 컴포넌트 `Material`(태그 컨테이너)이 재질 조건, `InitialStates` 는 BeginPlay 때 서버가 넣는 상태(처음부터 켜진 모닥불 등), `ReactionTable` 을 비우면 기본 규칙표.
- 상태 변화 델리게이트 `OnStateChanged`(BP) · `OnStateChangedNative`(C++): `(Component, StateTag, Old, New)`, New 0 = 사라짐. **서버는 계산 직후, 클라이언트는 FastArray 복제 콜백**(`PostReplicatedAdd / Change`, `PreReplicatedRemove`)에서 불린다 → 연출은 이 델리게이트 하나에 묶는다.
- `RemainingTime` 은 서버에서 계속 줄지만 항목을 더럽히지 않는다. 복제되는 값은 세기가 바뀔 때의 스냅숏(연출 참고용)이다.

### 반응 규칙표

**원본은 CSV, 런타임에 DataTable 로 읽는다**(`UDataTable::CreateTableFromCSVString`). 파이썬 헤드리스 임포트 대신 이 길을 고른 이유:

- 원본이 하나다 — `.uasset` 결과물이 없어 CSV 와 에셋이 어긋날 일이 없고, 바이너리 충돌도 없다(갈래 병렬 작업 · 리뷰가 텍스트 diff).
- 테스트가 게임과 똑같은 파일을 읽는다. 에디터 · 헤드리스 단계 없이 빌드만 하면 규칙이 바뀐다. 개발 중에는 콘솔 `Grim.Interaction.ReloadReactions` 로 다시 읽는다.
- 나중에 에디터에서 다루고 싶으면 CSV 를 `DT_ElementalReactions` 로 임포트하고 설정 `ReactionTableAsset` 에 지정하면 그쪽을 쓴다(행 구조체가 같다).
- 대가: 패키징 시 CSV 를 함께 넣어야 한다 — `DefaultGame.ini` `[/Script/UnrealEd.ProjectPackagingSettings]` 에 `+DirectoriesToAlwaysStageAsUFS=(Path="Grimoire/Data")`(통합 때).

| 열 | 뜻 |
|---|---|
| `Name` · `Note` | 행 이름 · 사람용 설명 |
| `Trigger` | `Stimulus`(자극이 들어올 때) · `Expire`(시간이 다 된 상태가 사라질 때 — `RequiredState` 가 그 상태) |
| `RequiredState` | 이 상태가 있어야 한다(`None` = 상관없음) |
| `BlockedByStates` | 이 중 하나라도 있으면 안 맞는다 — 예: 젖으면 불이 안 붙는다. `(State.Wet,State.Charred)` |
| `RequiredMaterial` | 재질 조건(`None` = 상관없음) |
| `Element` · `MinIntensity` | 들어온 자극 속성(하위 태그도 맞음) · 세기 하한 |
| `ClearStates` | 먼저 지울 상태들 |
| `ResultState` · `IntensityDelta` · `MaxIntensity` | 결과 상태 세기 += Delta(상한 0 = 없음). 0 이하면 사라진다 |
| `Duration` | 결과 상태의 남은 시간(초). 0 = 새 상태는 무한, 있던 상태는 그대로 |
| `Propagate` | `(Element, TargetMaterial, Radius, IntensityScale)` — 결과 상태가 살아 있는 동안 이웃에 줄 자극 |
| `Cue` | GameplayCue 태그(M1 은 비움) |

**위에서부터 검사해 처음 맞는 행 하나만 적용한다.** 그래서 강한 조건(돌풍)을 약한 조건(바람) 위에 두고, "젖은 것에 센 불"은 마르기만 하고 같은 자극으로 불이 붙지 않는다.

M1 규칙(11행 — 원본은 CSV):

| 행 | 조건 | 결과 |
|---|---|---|
| `Gust_Extinguish` | Burning + Wind ≥ 2.5 | Burning 지움(돌풍에 꺼짐) |
| `Wind_Feed` | Burning + Wind ≥ 0.3 | Burning +1, 최대 3 |
| `Water_Douse` | Burning + Water ≥ 0.3 | Burning 지움, Wet +1(최대 3, 20초) |
| `Earth_Smother` | Burning + Earth ≥ 0.5 | Burning 지움 |
| `Dark_Dim` | Burning + Dark ≥ 0.5 | Burning −1 |
| `Fire_Dry` | Wet + Fire ≥ 1.0 | Wet −1(마름) |
| `Wind_Dry` | Wet + Wind ≥ 0.5 | Wet −1 |
| `Water_Soak` | Water ≥ 0.3 | Wet +1(최대 3, 20초) |
| `Grass_Ignite` | Consumable, Wet · Burning · Charred 아님, Fire ≥ 0.5 | Burning +1(8초), 2.5m 전파 ×1.0 |
| `Flammable_Ignite` | Flammable, Wet · Burning · Charred 아님, Fire ≥ 0.5 | Burning +1(무한), 1.2m 전파 ×0.5 |
| `Grass_BurnOut` | Expire Burning, Consumable | Charred 1(다시 안 탄다) |

스펙의 「양초 예시」는 모두 이 표로 나온다: 켜짐 · 바람으로 커짐(최대 3) · 돌풍에 꺼짐 · 물로 꺼지고 젖음 · 젖으면 불 안 붙음 · 불 · 바람 · 시간(20초)으로 마름 · 어둠에 작아짐. `Grimoire.Interaction.CandleExample` 이 한 줄씩 확인한다.

### 전파

- 규칙의 `Propagate` 가 있는 결과 상태를 가진 컴포넌트가 `UGrimElementalPropagationSubsystem` 에 출처로 등록된다(상태가 사라지면 빠진다).
- 출처마다 `PropagationInterval`(1초)마다 한 번, 몸 위치에서 구 오버랩(모든 오브젝트 타입) → `TargetMaterial` 을 가진 이웃에 `{ Element, 세기 × IntensityScale, Source = Propagation }` 자극.
- 예산: 프레임당 쿼리 `MaxPropagationQueriesPerFrame`(200), 쿼리당 이웃 `MaxNeighborsPerQuery`(16), 반경 상한 `MaxPropagationRadius`(10m). 예산이 모자라면 다음 프레임에 이어 간다(라운드 로빈).
- 처음 번지는 건 불이 붙고 한 간격 뒤. 이미 타는 이웃은 `BlockedByStates` 로 막혀 되먹임이 없다 → 마른 풀 불은 8초 뒤 꺼지고 탄다.
- 전용 콜리전 채널(`ECC_Elemental`)은 M1 에서 쓰지 않는다(`DefaultEngine.ini` 를 건드리지 않으려고). 사물이 많아지면 채널을 만들어 쿼리를 좁힌다.

### 사물 액터

| 액터 | 모양 | 재질 | 특징 |
|---|---|---|---|
| `AGrimCandle` | 원기둥 지름 6cm · 높이 20cm | Flammable | 빛 15cd × 불꽃 세기 |
| `AGrimCampfire` | 원기둥 지름 80cm · 높이 15cm | Flammable | 빛 120cd × 세기. 생존 쪽 열원(`GetStateIntensity(State.Burning)`) |
| `AGrimDryGrass` | 1m × 1m × 10cm 판 | Flammable + Consumable | 폰은 겹쳐 지나가고 그 밖은 막는다(마법 판정 · 전파 쿼리를 받음). 8초 타고 Charred |

- 공통(`AGrimElementalObject`): 루트 아래 `BodyMesh` · `FlameMesh`(원뿔, 충돌 없음, 크기 = 기준 × √세기) · `FlameLight`(포인트 라이트, 세기 = Burning × cd) · `ElementalState`. `OnStateChanged` → `RefreshElementalVisuals()`(서버 · 클라이언트 공통, 데디케이티드 서버는 건너뜀).
- 젖음: 몸체 `BasicShapeMaterial` 의 `Color` 파라미터를 `WetColor` 쪽으로(젖음 2 에서 끝까지). 탐 = `CharredColor`.
- 수치(크기 · 빛 · 색)는 액터 기본값(UPROPERTY)이라 블루프린트 자식이나 배치 인스턴스에서 바꾼다. 나중에 불꽃은 Niagara(`NS_Flame_Small`)로 갈아 끼운다.

### 성능 · 네트워크 규칙

- 컴포넌트는 **틱하지 않는다**. 시간 제한 상태(젖음 · 풀 불)가 있을 때만 `StateUpdateInterval`(0.25초) 루핑 타이머, 없어지면 끈다. 무한 상태(켜진 양초)만 있으면 타이머도 없다.
- 사물 액터는 `DORM_DormantAll` 로 시작하고, 상태가 바뀔 때만 컴포넌트가 `FlushNetDormancy` 로 깨워 보낸다. 플레이어 · 생물(휴면 안 함)에 달아도 같은 코드다.
- **깨우기는 값을 바꾸기 "전"에 한다.** 엔진은 깨울 때 그 순간의 값을 "클라이언트가 이미 가진 값"으로 삼는다(`UNetConnection::FlushDormancyForObject` 가 현재 상태로 새 복제자를 만든다). 바꾼 뒤에 깨우면 변화가 기준에 묻혀 원격 클라이언트에 영영 가지 않는다 — M1 2인 PIE 에서 "호스트에는 불빛이 보이는데 원격 클라이언트에는 하나도 안 보이던" 원인. 그래서 상태 배열의 `SetIntensity` · `Remove` 가 실제로 바꾸기 직전에 `NotifyPreChange` → 컴포넌트 `HandlePreStateChange` → `FlushNetDormancy` 를 부른다. 접속 전에 바뀐 상태(처음부터 켠 모닥불)는 첫 복제에 실려 가서 이 버그가 가려졌다.
- 클라이언트 제거 콜백 `PreReplicatedRemove` 는 항목이 배열에서 빠지기 전에 불린다 → 세기를 먼저 0 으로 두고 델리게이트를 부른다(`Has` 도 세기 0 을 없는 것으로 본다). 안 그러면 원격 클라이언트에서 꺼진 불이 계속 보인다.
- 확인: `Scripts/Integration/net_check.ps1` — 헤드리스 리슨 서버 + 클라이언트를 띄워 서버에서 `Grim.Interaction.Stimulus <속성> <세기> [지연초] [클래스]` 로 점화 · 바람 · 물을 주고, 양쪽 `Grim.Interaction.Dump [지연초]`(상태 · 불꽃 · 빛) 줄이 같은지 본다. 결과 로그는 `Saved/NetCheck/`.
- 상태를 바꾸는 건 서버뿐(`HasAuthority` 아니면 무시).

### 날씨 · 생물과의 연결 (통합 이후)

- `UGrimWeatherSubsystem` 은 비가 올 때 **야외**(지붕 없음: 위로 트레이스) 사물에 주기적으로 `Water` 자극을 준다 → 불이 꺼진다. 번개는 낙뢰 지점에 `Wind + Light` 고강도 자극.
- 생물 · 플레이어도 같은 컴포넌트: `State.Wet` 이면 `UGrimDamageExecution` 이 번개 · 얼음 피해를 올린다. `State.Burning` 은 주기 피해 GE 를 ASC 에 적용한다(컴포넌트가 ASC 를 찾아 브리지 — 아직 없음).

## UI

- HUD: 단축 슬롯(마법 아이콘 · 쿨다운 · 서클 색), 마나, 조준점(대상의 속성 상태 아이콘 표시 — "젖음" 보이기).
- 그리모어(`WBP_Grimoire`): 배운 마법 · 필터 · 슬롯 배치 · 완드 정보 · 발견한 반응 규칙 도감.

## M1 범위

불씨 · 물줄기 · 미풍 3개 마법, 완드 1종, 양초 · 모닥불 · 마른 풀 3개 사물, 반응 규칙표 10행, 상성 표. 서클 UI 는 숫자만.

아직 없는 것: 시전 몽타주 · 머즐 이펙트 · 광선 연출, 그리모어 UI, 스킬북 아이템, 완드 정의 데이터 · 장착 해제, 서클 승급.
