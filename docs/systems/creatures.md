# 마법 생물 · 생태계 시스템

> 스펙: [마법 생물](../spec/creatures) (R02 · R14 · R15 · R23 · R35) · 코드: `Source/Grimoire/Creatures/` · 테스트: `Grimoire.Creatures.*`
>
> M3 묶음 1(갈래 C)에서 **종 정의 · 생물 폰 · AI · 생태계 스폰 · 두 속성 상성 · 튜토리얼 이벤트**까지 만들었다. 길들이기 · 충성도 · 탑승은 묶음 2(갈래 T) — 아래 「다음 묶음이 쓸 자리」.

## 한눈에

| 클래스 · 파일 | 하는 일 |
|---|---|
| `UGrimCreatureSpecies` (PrimaryDataAsset, 타입 `CreatureSpecies`) | 종 정의 — 도감 · 능력치 · 공격 · 생태 · 전리품 · 색 영역 · 임시 모델 부위 · 길들이기 자리 |
| `FGrimCreatureInstance` | 개체 기록(성별 · 색 · 크기 · 거대 · 돌연변이 · 성격 · 길들이기 자리). 서버가 굴리고 복제 · 접힌 기록이 그대로 쓴다 |
| `AGrimCreature` (Character) | 생물 폰 — ASC · 체력 · 속성 태그 · 속성 상태 · 조준 · 피해 · 죽음 · 전리품 · 고유 능력 |
| `AGrimCreatureAIController` (AAIController) | 서버 AI — 감각을 모아 순수 판단 `GrimCreatureMath::Decide` → 상태 · 이동 |
| `UGrimCreatureAttackAbility` (GameplayAbility) | 생물 공격 하나 = 속성 하나. 마법 피해 GE + 자극 + 넉백 |
| `AGrimHabitatVolume` | 서식 구역 상자 — 종 · 상한 · 시간대 · 회복 |
| `UGrimEcosystemSubsystem` (WorldSubsystem, 서버) | 개체군 기록 · 회복 · 접기 · 펼치기 · 먹이 관계 표 |
| `GrimCreatureMath` | 순수 계산(개체 굴림 · 전리품 · 활동 시간 · AI 판단 · 개체군) — 테스트가 직접 부른다 |
| `UGrimCreatureSettings` | 수치 — `Config/DefaultGame.ini` `[/Script/Grimoire.GrimCreatureSettings]` |
| `GrimCreatureTags` | `Creature.Ability.*` · `Creature.State.*` · `Habitat.*` · `Element.Earth.Plant`(풀) · `Element.Dark.Shadow`(그림자술) |
| 에셋 | `Content/Grimoire/Creatures/Species/DA_Creature_{Jackalope, Kelpie, BlackDog}` — `Scripts/Creatures/make_creature_species.py` 가 정본 |
| 표 | `Content/Grimoire/Data/CreatureEcology.csv` — 먹이 관계(`Predator, Prey, Priority`) |

## 종 정의 `UGrimCreatureSpecies`

| 묶음 | 필드 |
|---|---|
| 도감 | `SpeciesId`(개체 기록 · 서식 구역 · 먹이 관계 표 · 콘솔이 쓰는 이름), `DisplayName`, `Origin`, `PrimaryElement` · `SecondaryElement`(R35 — 주 = 전투 역할, 부 = 생태 특징), `SizeClass`(Small · Medium · Large · Legendary), `AbilityTags`(고유 능력 `Creature.Ability.*` 하나) |
| 능력치 | `MaxHealth`, `Power`(공격 배율), `WalkSpeed` · `RunSpeed`, `Temperament`(Timid · Docile · Aggressive), `PerceptionRadius`, `AggroRadius`, `FleeHealthRatio`(0 = 설정값), `Attacks[]` |
| 공격 `FGrimCreatureAttack` | `AttackName`, `Element`(하나), `Damage`, `Range`(캡슐 겉면까지), `Cooldown`, `Knockback`, `StimulusIntensity`, `Weight` — 주속성 기술 · 부속성 기술이 따로 |
| 생태 | `Diet`(Herbivore · Carnivore · Manavore), `Activity`(Diurnal · Nocturnal · DayAndDusk · Always), `bHideOutsideActiveTime`(시간 밖이면 사라짐), `bAvoidsLight`, `HerdSizeMin/Max`, `HabitatTags`(`Habitat.*`), `WaterMovement`(Swim · RunOnSurface), `Loot[]` |
| 전리품 `FGrimCreatureLootEntry` | `ItemId`(FName — 갈래 I 의 아이템 id) · `MinCount` · `MaxCount` · `Chance` |
| 개체 차이 | `ColorRegions[]`(영역 2~6: `BasePalette` · `MalePalette` · `FemalePalette` · `bJitter`), `MutantPalette`(3~5색), `MaleRatio`, `GiantChance`(2%) |
| 임시 모델 | `CapsuleRadius` · `CapsuleHalfHeight`, `BodyParts[]`(`Shape` = 엔진 기본 도형 Cube · Sphere · Cylinder · Cone, 위치 · 회전 · 배율, `ColorRegion`, `Sex` = Any · FemaleOnly · MaleOnly, `bVisibleWhenHidden`) |
| 길들이기 자리 | `bTameable`, `TamingCircle`, `PreferredPactElement`(잘 듣는 파크툼 속성), `bMountable` |

- 에디터 검사(`IsDataValid`): 종 id, 주속성 있음(무속성 생물 없음), 부속성 ≠ 주속성, 색 영역 1~6, 부위의 색 영역 번호, 무리 최소 ≤ 최대, 공격마다 속성.
- 진짜 메시가 오면 `BodyParts` 를 비우고 스켈레탈 메시 + 마스크 머티리얼(`RegionColor0..5`)로 바꾼다 — 개체 기록은 그대로.

### M3 생물 3종 (`make_creature_species.py`)

| | 재칼로프 `Jackalope` | 켈피 `Kelpie` | 검은 개 `BlackDog` |
|---|---|---|---|
| 속성 | 바람 · 풀(`Element.Earth.Plant`) | 물 · 바람 | 그림자술(`Element.Dark.Shadow`) · 불 |
| 분류 · 기질 | Small · 초식 · 겁 많음 | Medium · 초식 · 온순 · 탈것 | Medium · 육식 · 공격적 |
| 체력 · 힘 · 걷기/달리기 | 30 · 1.0 · 220/650 | 90 · 1.2 · 250/900 | 70 · 1.4 · 300/800 |
| 공격 | 뿔 돌진(바람 4, 0.9m, 넉백 500) · 풀씨 뿌리기(풀 2, 1.5m) | 물 뿜기(물 8, 4m, 넉백 700) · 돌풍 걷어차기(바람 6, 1.2m) | 그림자 물기(그림자술 9, 0.8m) · 불씨 눈빛(불 6, 3.5m) |
| 고유 능력 | 씨앗 퍼뜨리기 | 물 위 달리기 | 그림자 걸음 |
| 활동 · 무리 | 낮 · 3~6 | 낮 + 해 질 녘 · 2~3 | 밤(낮엔 사라짐) · 1~2, 불빛을 피함 |
| 전리품(아이템 id) | `JackalopeMeat` 1~2 · `JackalopeFur` 70% · `JackalopeAntler` 35% | `KelpieMane` 1~2 · `KelpieWaterEssence` 50% (고기 없음) | `BlackDogHide` · `BlackDogFang` 1~2 60% |
| 색 영역 | 털 · 배 · 뿔(수컷 팔레트) · 눈 | 몸 · 갈기(수컷 팔레트) · 발굽 · 눈 | 털 · 불씨 눈 · 목털(수컷 팔레트) · 발 |
| 성별 부위 | 수컷 갈라진 큰 뿔 / 암컷 뿔 돌기 | 수컷 긴 갈기 / 암컷 짧은 갈기 | 수컷 큰 목털 / 암컷 작은 목털 |
| 길들이기 자리 | 1서클 · 벤투스(바람) | 2서클 · 아쿠아(물) | 3서클 · 그림자술 |

- 아이템 id 는 갈래 I 의 아이템 표와 이름을 맞춘다(통합 때 한쪽을 고친다).

## 개체 기록 `FGrimCreatureInstance` (R23)

`SpeciesId, Seed, Sex, Colors[] { PaletteIndex, bMutant, Color }, SizeScale, bGiant, bMutant, Personality, StatScale` + 길들이기 자리 `OwnerId(FGuid), Loyalty, Nickname`.

- **굴림**(`GrimCreatureMath::RollInstance(종, 씨앗, 설정 Roll)`): 서버가 생성 때 굴린다. 같은 종 + 같은 씨앗 = 같은 개체. 순서 고정: 성별(`MaleRatio`) → 거대(`GiantChance`) → 크기(0.9~1.1, 거대 1.3~1.5) → 돌연변이(`WildMutantChance` 3%, 영역 하나가 변이 팔레트) → 영역마다 팔레트 색 + 흔들림(색조 ±4° · 채도 · 밝기 ±8%, `bJitter` 끄면 그대로) → 성격(None 밖에서 하나).
- 능력치 배율 `StatScale` = 1 + (크기 − 1) × `StatPerSize`(0.5 → ±5%), 거대면 × `GiantStatMultiplier`(1.5). 체력 = 종 `MaxHealth` × `StatScale`, 공격 피해 = `Damage × Power × StatScale`. 거대 개체는 `Creature.Ability.GiantRoar` 태그(기술 하나 더 — 효과는 자리만).
- 성별 팔레트: 그 성별 팔레트가 비어 있지 않으면 그것, 아니면 기본. 성별 부위는 그 성별일 때만 만든다.
- 클라이언트는 복제된 기록으로 칠한다(굴리지 않는다). 최종 색까지 복제 — 서버 · 클라이언트 색이 비트까지 같다.
- 유전 IV · 번식(`FGrimGenome`)은 M4~M5.

## 생물 폰 `AGrimCreature`

- **컴포넌트**: `UGrimAbilitySystemComponent`(Minimal 복제) + `UGrimVitalAttributeSet`(Health · MaxHealth) + `UGrimElementalStateComponent`(재사용) + `BodyRoot`(기본 도형 부위의 부모). 스켈레탈 메시는 비어 있다.
- **복제**: `Species`(에셋 참조) · `Instance` · `AIState` · `bShadowHidden` · `Death { bDead, Drops[], KillerPlayer, KillerName }`. 이동은 Character 복제.
- **속성 태그**: 서버 · 클라이언트 각자 루즈 태그 `Element.<주>` · `Element.<부>` · `SubElement.<부>`(아래 두 속성) · 종 `AbilityTags`. 종에서 정해지므로 복제하지 않는다.
- **속성 상태**: 재질 = 설정 `ElementMaterials` 에서 **두 속성 몫을 합친 것**(풀 → `Material.Flammable` · `Material.Absorbent`, 물 → `Material.Conductive`). 자극은 `IGrimElementalReceiver` 를 구현한 생물이 받아 상태 컴포넌트에 넘기고(규칙표 그대로), 덧붙여 — 빛 · 불 자극이면 그림자 걸음을 풀고, 마법 자극이면 쏜 이에게 반응한다.
- **불붙음**: `State.Burning` 이면 초당 `BurnDamagePerSecond × 세기` 불 피해(마지막으로 때린 이의 ASC 로 — 타 죽어도 사냥한 플레이어 몫), `BurnSelfExtinguishSeconds`(5초) 뒤 흙에 뒹굴어 끈다(`Element.Earth` 자극 → 반응 규칙 `Earth_Smother`). AI 는 공황(도망).
- **조준**: `IGrimTargetable` — 종류 Creature(공격적 종이면 Enemy), 조준점 = 캡슐 가운데 위 30%, 죽음 · 그림자 걸음 중엔 조준 안 됨.
- **피해 · 죽음**(서버): Health 변화 델리게이트 → 때린 이 기억(`ProvokedMemorySeconds`) → 체력 ≤ `WeakenedHealthRatio`(30%) 처음이면 `Creature.State.Weakened` + 튜토리얼 이벤트 → 0 이면 죽음: 전리품 굴림 → `Death` 복제 → `OnCreatureDied` · `OnLootDropped`(아이템 id · 개수 · 위치) → 튜토리얼 이벤트 → 생태계에 알림 → `CorpseSeconds`(6초) 뒤 사라짐. 클라이언트는 `OnRep_Death` 로 눕히고 같은 전리품을 안다.
- **전리품 → 줍기 아이템**: `SpawnLootPickups` 에 `UGrimItemStatics::SpawnPickup(World, ItemId, Count, Location)` 을 부를 자리(TODO 갈래 I). 지금은 이벤트 + 로그.

### 튜토리얼 이벤트

`UGrimQuestComponent::NotifyQuestEvent(플레이어, 이벤트)` — 이름 = 설정 접두 + 크기 분류.

| 때 | 이벤트 | 누구에게 |
|---|---|---|
| 처음 체력 30% 이하 | `Creature.Weakened.<분류>` (소형 = `Creature.Weakened.Small`) | 마지막으로 때린 플레이어 |
| 죽음 | `Creature.Killed.<분류>` (소형 = `Creature.Killed.Small`) | 죽인 플레이어(GE 컨텍스트의 Instigator = PlayerState). 다른 생물 · 환경이 죽이면 없음 |

### 두 속성 상성 (Q35)

- 공격은 늘 속성 하나. 대상이 둘이면 배율 = 상성(공격, 주) × (1 + 0.5 × (상성(공격, 부) − 1)) — `UGrimDamageExecution`(마법 · 생물 공격 모두 같은 GE).
- **주 · 부 구분 방법**: 부속성에는 표지 태그 `SubElement.<같은 이름>` 을 함께 단다(`UGrimDamageExecution::GetSecondaryMarkerTag`). 피해 계산은 대상 태그에서 `Element.*`(Element.None 제외)를 모아 **표지가 붙은 것 = 부, 나머지 첫째 = 주**. 태그 묶음 순서(ASC 태그 개수 컨테이너는 순서를 지키지 않는다)에 기대지 않으려고 표지를 썼다. 표지 없이 둘이면 순서대로(정하지 않은 동작).
- 반영 비율 0.5 는 `UGrimDamageSettings::SecondaryElementWeight`(ini).
- 지금 상성 표는 6속성(10속성 표는 코드 반영 전)이라 풀 · 그림자술 행이 없다 → 그 둘은 ×1. 10속성 표가 들어오면 식은 그대로 맞는다.

## 고유 능력

| 능력 | 지금 하는 것 | 다음 |
|---|---|---|
| 씨앗 퍼뜨리기 (`Creature.Ability.SeedSpread`) | `SeedSpreadInterval`(2초)마다 발밑 `SeedSpreadRadius`(1.5m) 안의 `SeedSpreadMaterial`(`Material.Consumable` — 마른 풀) 사물에 `Element.Earth.Plant` 자극(출처 Creature, 세기 0.5). 잠 · 싸움 중엔 안 한다 | 반응 규칙표에 「탄 풀 + 풀 자극 → 다시 남」 행이 들어오면 눈에 보인다(Interaction 갈래 몫 — 지금은 행이 없어 아무 일 없다) |
| 물 위 달리기 (`Creature.Ability.WaterRun`, `WaterMovement = RunOnSurface`) | 물 볼륨 안에서 부력 `WaterRunBuoyancy`(1.6)로 수면에 떠 달리기 속도로 헤엄 | 탑승 · 진짜 수면 이동 모드(커스텀 이동 모드)는 갈래 T |
| 그림자 걸음 (`Creature.Ability.ShadowStep`) | 사냥 · 싸움 중, 밤 + 불빛 없음 + 표적이 `ShadowStepMinTargetDistance`(4m) 밖이면 `ShadowStepSeconds`(3초) 숨음(`Creature.State.ShadowHidden`, 눈만 보임, 조준 · 포식자 감지 안 됨). 덮치거나 · 맞거나 · 빛(`Element.Light`) · 불 자극이면 풀리고 `ShadowStepCooldown`(8초) | 빛 마법 · 신성이 10속성으로 나뉘면 `ShadowRevealElements` 만 고친다 |

## AI (서버)

- **StateTree · BT 를 쓰지 않고 C++ 상태 기계 + 순수 판단 함수**로 했다. 까닭: (1) StateTree · BT 는 바이너리 에셋이라 헤드리스 파이썬으로 만들기 · 리뷰하기가 어렵고 사람이 에디터에서 손봐야 한다(CLAUDE.md — 블루프린트 변경은 사용자 몫), (2) 판단을 `GrimCreatureMath::Decide(감각, 기질)` 순수 함수로 두면 자동화 테스트로 바로 검증된다, (3) M3 의 상태 7개는 우선순위 한 줄로 충분하다. 상태가 늘거나(영역 다툼 · 작업 · 탑승) 디자이너가 손봐야 하면 `Decide` 를 StateTree 조건 · 평가자로 옮긴다 — 감각 구조체 · 상태 이름이 그대로 StateTree 의 입력 · 상태가 된다.
- 주기 `ThinkInterval`(0.25초, 생물마다 시작을 흩음)마다 감각 `FGrimCreatureSenses` 를 모은다: 체력 비율, 불붙음, 활동 시간, 위협(플레이어 · 포식자가 `PerceptionRadius` 안 — 잠들면 × `SleepPerceptionScale`), 덤빌 거리(`AggroRadius`), 맞음(기억 안의 때린 이, `ChaseGiveUpDistance` 안), 먹이(먹이 관계 표 · `HuntRadius`), 불빛(타는 것 `LightAvoidRadius` 안 — 빛 피함 · 그림자 걸음 종만).
- **판단 우선순위**: 불붙음 → Flee / 빛 피함 + 불빛 → Flee / 위협 + 체력 ≤ 도망 비율 → Flee / 겁 많음 + 위협 · 맞음 → Flee / 맞음 → Fight(공격 없으면 Flee) / 공격적 + 활동 시간 + 덤빌 거리 → Fight / 활동 시간 밖 → Sleep / 육식 + 먹이 → Hunt / 그 밖 → 한가함(Idle · Wander · Graze 를 `CalmWeights` · `CalmActivitySeconds` 로 번갈아 — 풀 뜯기는 초식만).
- **행동**: Flee = 위협 반대쪽 `MoveStepDistance` 앞(±30° 흔듦), `FleeSafeDistance` 밖이면 멈춤. Fight · Hunt = 달려가 사거리 × 0.9 안이면 멈춰 `TryAttack`(준비된 공격 중 가중치로 하나). Wander = 집(처음 자리) 둘레 `WanderRadius`. Sleep = 멈춤(웅크림 연출).
- **이동**: 매 프레임 목표로 `AddMovementInput`(내비 메시 없이 곧게). 지형 통합(갈래 W) 때 내비 메시가 생기면 `MoveToLocation` 으로 바꾼다.
- 먹이 관계 `CreatureEcology.csv`: `BlackDog → Jackalope`(Priority 1). 포식자는 피식자에게 위협이 된다(재칼로프는 검은 개를 보면 도망 — 숨은 개는 못 본다).
- 날씨 반응(비 → 불 생물 숨음) · 영역 다툼은 아직 없다.

## 생태계 (R14)

### 서식 구역 `AGrimHabitatVolume`

- 상자(`Bounds`, 충돌 · 복제 없음) + `Species[] { Species, MaxCount, bOverrideActivity · Activity, RegrowSeconds }` + `HabitatTags` + `bStartFull` + `RandomSeed`. 서버 BeginPlay 에 생태계에 등록.
- 종의 `HabitatTags` 와 구역 태그가 안 맞으면 경고만 하고 놓는다.

### 개체군 · 접기 · 펼치기 `UGrimEcosystemSubsystem`

- 구역 · 종마다 개체군 `{ 상한 Cap, 접힌 기록 Folded[], 펼친 액터 Active[], 회복 타이머 }`, 현재 = 접힘 + 펼침.
- 등록 때(`bStartFull`) 상한만큼 **무리 단위로** 기록을 굴린다: 무리 크기 = 종 `HerdSizeMin~Max`, 가운데 한 점 + `HerdRadius` 둘레. 무리에 거대 개체가 있으면 첫째 자리(우두머리).
- `UpdateInterval`(1초)마다:
  1. **회복** `StepRegrow`: 현재 < 상한이면 `RegrowSeconds`(240초)마다 하나(남은 무리 곁에). 멸종 없음.
  2. **접기**: 펼친 액터가 모든 플레이어에게서 `DespawnRadius`(80m) 밖이거나, 숨는 종(`bHideOutsideActiveTime`)이 활동 시간 밖이면 기록(개체 · 위치 · 체력 비율 · 무리)으로 접고 액터를 지운다.
  3. **펼치기**: 기록이 플레이어 `SpawnRadius`(60m) 안이면 바닥(WorldStatic 아래로 트레이스)을 찾아 액터로. **서버 성능 상한**: 세계 전체 `MaxActiveCreatures`(40), 갱신당 `MaxSpawnsPerUpdate`(6). 펼침 < 접음 반경으로 되풀이를 막는다.
- 죽으면 개체군에서 빠진다(현재 −1 → 회복 시작). 콘솔로 놓은 생물(`Grim.Creature.Spawn`)은 구역이 없어 접히지 않는다.
- 세이브: 아직 없음 — 다음 묶음에서 길들인 개체 + 구역 개체군(접힌 기록)을 `IGrimSaveParticipant` 로 남긴다(생물 액터는 레벨에 놓인 것이 아니라 세이브 수집에서 빠진다).

## 다음 묶음(길들이기 · 충성도 · 탑승 — 갈래 T)이 쓸 자리

- 종: `bTameable` · `TamingCircle` · `PreferredPactElement` · `bMountable` · `WaterMovement`.
- 개체 기록: `OwnerId`(무효 = 야생) · `Loyalty` · `Nickname` · `Personality`(Q27, 지금 굴리기만) — 로직 없음. 길들이면 개체군 `Active` 에서 빼고 소유 목록으로(`UGrimEcosystemSubsystem::HandleCreatureDied` 처럼 빼는 함수를 하나 더).
- 약해짐: `Creature.State.Weakened` 태그(체력 30% 이하 — 포획 확률 보정), 잠 = `AIState Sleep`.
- 탑승: 생물은 Character 라 Possess 그대로. 물 위 달리기는 부력 자리만 — 수면 이동 모드를 붙인다.
- AI: 길들인 생물의 `Follow` · 명령 상태는 `Decide` 앞에 「주인이 있으면」 갈래를 더한다.

## 설정 `[/Script/Grimoire.GrimCreatureSettings]`

| 묶음 | 값(기본) |
|---|---|
| 종 | `Species[]`(3종 에셋), `EcologyCSV`, `ElementMaterials[]` |
| `Roll` | 크기 0.9~1.1 · 거대 1.3~1.5 · `StatPerSize` 0.5 · `GiantStatMultiplier` 1.5 · `WildMutantChance` 0.03 · 흔들림 4° / 0.08 / 0.08 · `GiantLootMultiplier` 2 |
| `AI` | `ThinkInterval` 0.25 · `FleeHealthRatio` 0.25 · `ProvokedMemorySeconds` 12 · `ChaseGiveUpDistance` 30m · `HuntRadius` 25m · `WanderRadius` 8m · `CalmActivitySeconds` 3~8 · `CalmWeights` 1/2/2 · `SleepPerceptionScale` 0.5 · `MoveStepDistance` 6m · `FleeSafeDistance` 20m · `LightAvoidRadius` 9m · 해 질 녘 17~21시 |
| `Abilities` | 씨앗 2초 · 1.5m · 0.5 · `Material.Consumable` / 그림자 걸음 3초 · 8초 · 4m · 드러내는 속성 `Element.Light` · `Element.Fire` / 물 위 부력 1.6 · 속도 1 / 불 피해 2 · 5초 뒤 끔 |
| `Ecosystem` | 갱신 1초 · 펼침 60m · 접음 80m · **상한 40** · 갱신당 6 · 회복 240초 · 무리 반경 4m · 시체 6초 · 바닥 트레이스 위 10m / 아래 50m |
| 퀘스트 | `WeakenedHealthRatio` 0.3 · `KilledEventPrefix` `Creature.Killed.` · `WeakenedEventPrefix` `Creature.Weakened.` |

두 속성 반영 비율은 `[/Script/Grimoire.GrimDamageSettings] SecondaryElementWeight=0.5`.

## 콘솔 (개발 빌드)

| 명령 | 하는 일 |
|---|---|
| `Grim.Creature.Spawn <종 id> [수] [지연초] [거리cm]` | 서버: 첫 플레이어 앞에 개체를 굴려 놓는다 |
| `Grim.Creature.Habitat <종 id> [상한] [지연초] [거리cm] [반크기cm]` | 서버: 첫 플레이어 앞에 서식 구역 |
| `Grim.Creature.Damage <양> [지연초] [종 id\|*] [Element.*]` | 서버: 첫 플레이어가 쏜 것처럼 마법 피해 |
| `Grim.Creature.Stimulus <Element.*> <세기> [지연초] [종 id\|*]` | 서버: 마법 출처 속성 자극 |
| `Grim.Creature.Kill [지연초] [종 id\|*]` | 서버: 첫 플레이어가 죽인 것으로 |
| `Grim.Creature.Time <0~1> [지연초]` | 서버: 시각(0 자정 · 0.5 정오) |
| `Grim.Creature.Dump [지연초]` | 이 프로세스에서 본 생물(seed · 성별 · 크기 · 색 · 체력 · 죽음 · 전리품 + AI 상태) |
| `Grim.Creature.Eco [지연초]` | 서버: 구역 개체군(현재 · 펼침 · 접힘 · 죽음 · 회복) |

## 스크립트 · 확인

- `Scripts/Creatures/make_creature_species.py` — 3종 에셋을 만든다 / 고친다(값의 정본).
- `Scripts/Creatures/place_creature_test.py` — `L_M1_Sandbox` 를 사본 `/Game/Grimoire/Creatures/Test/L_CreatureTest` 로 떠서 서식 구역 3곳(재칼로프 초원 25m · 켈피 물가 · 검은 개 밤길 30m) + 마른 풀 6 + 켜진 모닥불을 놓는다. 원본 맵은 저장하지 않는다. 사본은 커밋하지 않는다. 통합 때는 `GRIM_CREATURE_TEST_MAP` 을 대상 맵으로 주고 돌린다(지형 맵에는 갈래 W 의 서식 구역 자리에).
- `Scripts/Creatures/net_check_creatures.ps1` — 리슨 서버 + 클라이언트 헤드리스. 스폰 · 서식 구역 펼치기 · 약해짐 · 죽음 · 전리품 · 밤 사냥 · 개체군을 돌리고 덤프 다섯 번을 서버 · 클라이언트끼리 비교한다(`NETCHECK_RESULT fail=0`).
- 테스트 `Grimoire.Creatures.*`: `Roll`(개체 굴림) · `DualAffinity`(Q35 식 · 주 · 부 가리기 · 실제 GE) · `Loot` · `AIDecide`(활동 시간 · 판단) · `Population`(회복 · 접기 · 펼치기 · 무리) · `SpeciesAssets`(3종 에셋 · 먹이 관계 표 · 설정) · `Actor`(스폰 → 태그 · 체력 · 재질 → 약해짐 → 죽음 · 전리품 이벤트) · `Ecosystem`(구역 등록 · 접힌 기록).

## M3 범위

- 묶음 1(이번): 위 전부 — 3종 · 생태계 스폰 · AI · 두 속성 상성 · 튜토리얼 이벤트.
- 묶음 2(갈래 T): 길들이기 · 충성도 · 성격 · 탑승 · 세이브 참여 · 튜토리얼 `Creature.Tamed.Small`.
- 노동 · 번식 · 영역 다툼 · 날씨 반응은 M4~M5. 그때의 설계(작업 보드 · 번식장 · 유전 IV)는 아래에 남겨 둔다.

## 나중 설계(M4~M5)

- **노동** `UGrimWorkerComponent` + `AGrimBaseCore`: 거점 핵의 작업 보드 `{ WorkTag, TargetActor, Priority }` → 적성 `WorkAptitudes[WorkTag] > 0` 인 요청을 우선순위로 → 해당 속성 자극(물 생물이 밭에 `Water` — 플레이어 마법과 같은 길). 일꾼도 배고픔 · 피곤함.
- **번식** `UGrimBreedingComponent` + `AGrimBreedingPen`: 같은 `BreedingGroup` 암수(충성도 50 이상) → `GestationMinutes` 타이머(서버, 세이브). 자식: 스탯마다 부모 IV 하나 또는 평균 ±2, 색은 영역마다 부모 중 하나(+ 흔들림 새로), 성별 반반, 크기 부모 평균 ±0.05(거대 유전 `GiantInheritChance`). 돌연변이 `p = 3% + 부모 돌연변이 1마리당 +3%`(Q8) → 영역 하나 이상 변이 팔레트, IV 상한 40.
- **충성도**: 0~100, 길들인 직후 50, 사건(먹이 · 치료 · 승리 · 굶김 · 혹사 · 기절 · 환경)마다 `FGrimLoyaltyEvent` × 성격 배율(Q27), 최근 3개 = 「까닭」, 단계 태그 `Creature.Loyalty.*`, 0 → 야생으로.
- **탑승** `UGrimMountComponent`: 플레이어 컨트롤러가 생물 폰을 Possess, 캐릭터는 안장 소켓에 부착. 탑승 중 시전(Q9) — ASC 가 PlayerState 에 있어 그대로.
- **영역 다툼**: 대형 종 `TerritoryRadius` — 다른 대형 종이 들어오면 서로 공격, 체력 30% 이하면 후퇴.
