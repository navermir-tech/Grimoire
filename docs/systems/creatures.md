# 마법 생물 · 생태계 시스템

> 스펙: [마법 생물](../spec/creatures) (R02 · R14 · R15 · R23 · R35) · 코드: `Source/Grimoire/Creatures/` · 테스트: `Grimoire.Creatures.*`
>
> M3 묶음 1(갈래 C)에서 **종 정의 · 생물 폰 · AI · 생태계 스폰 · 두 속성 상성 · 튜토리얼 이벤트**까지 만들었다. 묶음 2(갈래 T)에서 **길들이기(파크툼) · 충성도 · 성격 · 명령 · 탑승 · 세이브**를 더했다 — 아래 「길들이기」 · 「충성도」 · 「탑승」. 테스트: `Grimoire.Taming.*`.
> R38(2026-11-06)에서 **데리고 다니는 수 · 생물 보관함(노말 스킬) · 거점 생물 상자 · 소지품 「생물」 탭**을 더했다 — 아래 「데리고 다니기 · 보관함 · 거점 상자」. 테스트: `Grimoire.Creatures.Storage.*`.

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
| `UGrimCreatureTamingComponent` (생물의 기본 하위 객체) | 길들이기 · 충성도 상태 — 명령 · 최근 까닭 · 기절 복제, 서버 주기(지침 · 굶김 · 쉼 …), 단계 태그 |
| `UGrimMountComponent` (생물의 기본 하위 객체) | 탈것 — 탄 사람 복제 · 붙이기 · 입력 중계 · 물 위 달리기 · 떨어뜨림 |
| `UGrimTamerComponent` (PlayerState, 런타임 "GrimTamer") | 플레이어의 가진 개체 기록 · 명령 RPC · 세이브 `Creatures.Owned` · F 안내 · 탑승 입력 보내기 |
| `UGrimTamingSubsystem` (WorldSubsystem, 서버) | 길들이기 컴포넌트 붙이기 · 포획 판정 `TryTame` · 함께 이김 · 죽음 · 놓아줌 기록 |
| `UGrimCreatureMenuWidget` | 생물 메뉴(C++ 위젯 트리) — 충성도 막대 · 까닭 세 줄 · 따라오기 · 기다리기 · 타기 · 먹이 · 이름 · 놓아주기 |
| `AGrimPactumProjectile` (`Magic/`) | 파크툼 볼트 — 생물에 맞으면 피해 대신 포획 판정 |
| `GrimTamingMath` | 순수 계산(포획 확률 · 충성도 · 단계 · 성격 · 순종 · 길들인 AI 판단 · 세이브 기록) |
| `UGrimTamingSettings` | 수치 — `[/Script/Grimoire.GrimTamingSettings]` |
| `GrimTamingTags` | `Creature.Loyalty.*` · `Creature.State.{Tamed, Exhausted, Enraged, KnockedOut, Ridden}` · `Taming.CaptureBonus` · `Spell.Pactum.*` · `Cooldown.Spell.Pactum.*` |
| 파크툼 | `Content/Grimoire/Magic/Spells/DA_Spell_Pactum{, _Ignis, _Aqua, _Ventus, _Terra, _Umbra}` — `Scripts/Magic/make_pactum_spells.py`, 목록 표 `Content/Grimoire/Data/PactumSpells.csv` |
| `UGrimCreatureStorageComponent` (PlayerState, 런타임 "GrimCreatureStorage") | 데리고 다니는 수 · 생물 보관함(노말 스킬) · 옮기기 RPC · 세이브 `Creatures.Storage` · 거점 상자 화면(R38, 아래 「데리고 다니기 · 보관함 · 거점 상자」) |
| `AGrimCreatureBox` | 거점 생물 상자(팰 상자 자리) — 칸 복제 · 풀어놓기 · 세계 세이브 `CreatureBox.<이름>` |
| `UGrimCreatureStorageWidget` | 생물 목록 화면 — 소지품 「생물」 탭 · 상자 관리 화면 둘 다 |
| `UGrimCreatureStorageTab` | 소지품 화면의 「생물」 탭(M4 탭 틀 `UGrimInventoryTab`, ini `+InventoryTabClasses=`, 순서 15) — 위 위젯을 끼우고 보일 때 · 숨을 때 알린다 |
| `GrimCreatureStorageMath` · `UGrimCreatureStorageSettings` | 순수 계산(상한 · 옮기기 판정 · 갈 곳 · 보관 중 회복 · 상자 세이브) · 수치 `[/Script/Grimoire.GrimCreatureStorageSettings]` |

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

`SpeciesId, Seed, Sex, Colors[] { PaletteIndex, bMutant, Color }, SizeScale, bGiant, bMutant, Personality, StatScale` + 길들이기 `OwnerId(FString — 세이브 PlayerId, 빈 문자열 = 야생), Loyalty(0~100), Nickname`.

- **굴림**(`GrimCreatureMath::RollInstance(종, 씨앗, 설정 Roll)`): 서버가 생성 때 굴린다. 같은 종 + 같은 씨앗 = 같은 개체. 순서 고정: 성별(`MaleRatio`) → 거대(`GiantChance`) → 크기(0.9~1.1, 거대 1.3~1.5) → 돌연변이(`WildMutantChance` 3%, 영역 하나가 변이 팔레트) → 영역마다 팔레트 색 + 흔들림(색조 ±4° · 채도 · 밝기 ±8%, `bJitter` 끄면 그대로) → 성격(None 밖에서 하나).
- 능력치 배율 `StatScale` = 1 + (크기 − 1) × `StatPerSize`(0.5 → ±5%), 거대면 × `GiantStatMultiplier`(1.5). 체력 = 종 `MaxHealth` × `StatScale`, 공격 피해 = `Damage × Power × StatScale`. 거대 개체는 `Creature.Ability.GiantRoar` 태그(기술 하나 더 — 효과는 자리만).
- 성별 팔레트: 그 성별 팔레트가 비어 있지 않으면 그것, 아니면 기본. 성별 부위는 그 성별일 때만 만든다.
- 클라이언트는 복제된 기록으로 칠한다(굴리지 않는다). 최종 색까지 복제 — 서버 · 클라이언트 색이 비트까지 같다.
- 유전 IV · 번식(`FGrimGenome`)은 M5 계획.

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
| 물 위 달리기 (`Creature.Ability.WaterRun`, `WaterMovement = RunOnSurface`) | 안 탔을 때: 물 볼륨 안에서 부력 `WaterRunBuoyancy`(1.6)로 수면에 떠 달리기 속도로 헤엄. 탄 채: 수면에 붙여 `MOVE_Flying` 으로 달림(아래 「탑승」) | 진짜 물 · 지형이 오면(갈래 W) 볼륨 · `Grim.Water` 표면을 놓고 사람이 본다 |
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
- 길들이면 `HandleCreatureTamed` 로 개체군에서 빠진다(현재 −1 → 회복 시작, 접히지 않는다). 놓아준 생물은 개체군에 돌아가지 않는 떠돌이다.
- 세이브: 길들인 개체는 주인 플레이어 세이브(`Creatures.Owned`, 아래 「길들이기」). 구역 개체군(접힌 기록)의 세계 세이브는 아직 없다(생물 액터는 레벨에 놓인 것이 아니라 세이브 수집에서 빠진다).

## 길들이기 (갈래 T)

### 파크툼 주문

| 정의(DA_Spell_…) | 이름 | 속성 | 서클 | 스킬북 아이템 |
|---|---|---|---|---|
| `Pactum` | 파크툼 | 무속성(`Element.None`) | 1 | `Skillbook_Pactum`(튜토리얼 12단계 `Skillbook:Pactum`) |
| `Pactum_Ignis` · `Pactum_Aqua` · `Pactum_Ventus` · `Pactum_Terra` | 파크툼 이그니스 · 아쿠아 · 벤투스 · 테라 | 불 · 물 · 바람 · 땅 | 2 | `Skillbook_Pactum_<속성>` |
| `Pactum_Umbra` | 파크툼 움브라 | 그림자술(`Element.Dark.Shadow`) | 3 | `Skillbook_Pactum_Umbra` |
| `Pactum_Glacies` · `Flora` · `Sanctus` · `Lux` · `Maledicta` | (얼음 · 풀 · 신성 · 빛 · 흑마법) | — | — | 표(`PactumSpells.csv`)에 행만 — `bImplemented=False` |

- 모두 **피해 없는 볼트**(마나 12 · 쿨다운 2.5초 · 4500cm/s · 유도 90°/s · 사거리 22m, 자극 세기 0). 투사체 `AGrimPactumProjectile` 이 생물에 맞으면 `UGrimTamingSubsystem::TryTame`(서버)을 부르고 숙련도 명중 경험을 준다. 다른 것에 맞으면 터지기만 한다.
- 숙련도 추가 효과(Q26): `ProficiencyExtraEffect = { Taming.CaptureBonus, MaxValue 0.1 }` — S 등급에서 포획 확률 +10%p(등급 배율 `ExtraEffectScale` 이 곱해진다).
- 수치의 정본: 목록 · 이름 · 속성 · 서클 · 색 = `PactumSpells.csv`, 공통 수치 = `Scripts/Magic/make_pactum_spells.py` 의 `COMMON`.

### 포획 확률 `GrimTamingMath::CalcCaptureChance`

```
확률 = 기본 0.30
     + 약해짐(체력 ≤ 30%) 0.20 + 약한 정도 0.15 × (1 − 체력 ÷ 0.3)
     + 잠듦(AI Sleep) 0.15 + 지침(Creature.State.Exhausted) 0.10
     + 먹이(FedMemorySeconds 120초 안에 이 플레이어가 줌) 0.10 — 좋아하는 먹이면 0.20
     + 속성 일치(파크툼 속성이 잘 듣는 파크툼 · 주 · 부속성과 맞음) 0.25
     + 완드 친화 min((완드의 그 생물 주속성 위력 − 1) × 0.5, 0.15)
     + 서클 차 min((내 서클 − 요구 서클) × 0.05, 0.15)
     + 숙련도 추가 효과 + 장신구(M4 자리)
     − 등급(소형 0 · 중형 0.10 · 대형 0.25 · 전설 0.45, 거대 개체 +0.10)
→ 0.02 ~ 0.95 로 자른다. 요구 서클 미만(MaxCircleDeficit 0)이면 시도할 수 없다(CircleTooLow).
```

- 예: 1서클 플레이어가 체력 20% 재칼로프에 파크툼 → 0.30 + 0.20 + 0.05 = **55%**, 파크툼 벤투스면 80%, 좋아하는 먹이(허브 · 곡물)까지 주면 95%.
- **성공**: 개체 기록 `OwnerId` = 그 플레이어 PlayerId(`UGrimSaveSubsystem::GetPlayerId` — 세이브 파일 이름과 같은 값), 충성도 50, 까닭 「계약」, 체력 50% 까지, 개체군에서 빼기(`UGrimEcosystemSubsystem::HandleCreatureTamed` — 회복이 시작된다), 주인 기록에 넣기, 퀘스트 이벤트 `Creature.Tamed.<분류>`(소형 = 튜토리얼 14단계 `Creature.Tamed.Small`).
- **실패 = 저항**: `EnrageSeconds`(6초) 동안 `Creature.State.Enraged` — AI 판단 1-1 순위로 **겁 많은 종도 시전자에게 덤빈다**(공격이 없으면 도망).
- 먹이 주기: 야생에게도 F(먹일 것이 가방에 있을 때만 안내) — 좋아하는 먹이(`FoodRules.FavoriteItems`) 먼저, 없으면 먹는 묶음(`AcceptedGroups` → 없으면 먹이 습성 기본 `DietGroups`) 중 하나를 가방에서 하나 빼고 기록한다. 재칼로프 = 허브 · 곡물(풀 · 요리 재료), 켈피 = 수초(풀), 검은 개 = 재칼로프 고기(고기).
- 지침: 모든 생물이 도망 · 싸움 · 사냥 · 탑승 중 초당 1씩 고단함이 쌓이고(쉬면 0.5씩 풀림) 90 이면 `Creature.State.Exhausted`.
- 개발: `Grim.Taming.ForceRoll <0~1>` 로 굴림 고정(음수 = 끔).

### 길들인 생물

- 주인 곁 AI(`AGrimCreatureAIController::ThinkTamed` → 순수 판단 `GrimTamingMath::DecideTamed`): 탔음 → `Ridden`(입력은 탈것 컴포넌트) / 기절 → `Sleep` / 불붙음 → `Flee` / 다른 생물에게 맞음 → 겁 많거나 체력 낮으면 주인 쪽으로 `Flee`, 아니면 `Fight` / 기다리기 → `Stay` / 거점에 풀어놓음(`Roam`, R38) → `Wander`(집 둘레 배회) / 따라오기 → 3.5m 밖이면 `Follow`(9m 밖이면 달림, 50m 밖이면 주인 뒤로 옮김) · 안이면 `Idle`. 플레이어(주인 · 친구)에게 맞은 것은 화내지 않는다.
- 플레이어는 길들인 생물을 겨누지 않는다(`CanBeTargetedBy` — 요청자가 플레이어 폰이면 false).
- 체력 0 → 죽지 않고 **쓰러진다**(`Creature.State.KnockedOut` · 충성도 「기절」), 20초 뒤 체력 20% 로 일어난다. 디버그 처치처럼 정말 죽으면 주인 기록에서 지운다.
- 명령(F 메뉴 · RPC `UGrimTamerComponent::Server_Action`): 따라오기 · 기다리기는 단계의 **순종 확률**로 굴려 실패하면 「명령을 무시했다」. 놓아주기는 늘 된다(메뉴에서 두 번 눌러 확인) — 주인 · 충성도 · 이름을 지우고 그 자리를 집으로 야생이 된다(다시 잡을 수 있다, 개체군에는 돌아가지 않는 「떠돌이」).
- 이름: 메뉴 「이름 짓기」(글 상자 + Enter) 또는 콘솔 `Grim.Taming.Act Name <종> <이름>` — 앞뒤 공백 · 줄바꿈을 지우고 16자.

### 세이브 · 다시 부르기

- `UGrimTamerComponent` 가 Player 참여자 `Creatures.Owned` v1: `int32 수` + 항목마다 개체 기록 전체(종 · 씨앗 · 성별 · 색 · 크기 · 거대 · 돌연변이 · 성격 · 능력치 배율 · 주인 · 충성도 · 이름) · 체력 비율 · 명령 · 최근 까닭 · 저장 UTC 시각(`GrimTamingMath::SerializeOwnedRecord`).
- 저장할 때 세계의 내 생물로 기록을 고쳐 쓴다. 불러오면(접속) 기록 중 세계에 없는 것을 **주인 뒤 반원 3m** 에 스폰하고, 접속하지 않은 시간 한 시간마다 「주인 부재」(최대 5). 이미 있으면 상태만 맞춘다(`Grim.Save.Load` 다시 불러오기).
- 주인이 나가면(PlayerState 파괴) 세이브가 먼저 쓰인 뒤 세계의 그 생물을 거둔다. 위치는 저장하지 않는다(늘 주인 곁으로 — 거점 배치는 M4).
- `Creatures.Owned` 는 **데리고 다니는 생물만** 담는다. 보관함은 `Creatures.Storage`(플레이어), 거점 상자 · 풀어놓은 생물은 상자의 세계 세이브(아래 R38). 다시 부를 때 데리고 다니는 수를 넘는 기록은 보관함(자리가 있으면 — 쿠스토디아를 배우지 않아도 기본 10칸)으로 들어간다 — 둘 다 안 되면 잃지 않게 그대로 부른다.

## 데리고 다니기 · 보관함 · 거점 상자 (R38)

> 스펙: [마법 생물 — 데리고 다니기 · 보관함 · 거점 생물 상자](../spec/creatures#데리고-다니기-·-보관함-·-거점-생물-상자-r38). 테스트: `Grimoire.Creatures.Storage.*`. 수치는 제안(Q60~Q64) — 단 **Q42 답(2026-11-07)**: 보관함은 처음부터(쿠스토디아 없이) 10마리, 인벤토리처럼 쓴다. 10마리를 넘기려면 노말 스킬 「쿠스토디아」를 배운다(늘어나는 칸은 기존 설계 수치).

### 네 자리 `EGrimCreaturePlace`

| 자리 | 어디에 있나 | 기록을 누가 들고 있나 |
|---|---|---|
| `Party` 데리고 다님 | 세계 — 명령 따라오기 · 기다리기 | 생물 액터 + 주인의 `UGrimTamerComponent::Records`(세이브 `Creatures.Owned`) |
| `Storage` 보관함 | 세계에 없음 | `UGrimCreatureStorageComponent::Stored`(가진 개체 기록 `FGrimOwnedCreatureRecord` 그대로, 소유 클라이언트에 복제, 세이브 `Creatures.Storage`) |
| `Box` 상자 안 | 세계에 없음 | `AGrimCreatureBox::Entries`(`FGrimCreatureBoxEntry` = 기록 + 풀어놓음, 모두에게 복제, 세이브 `CreatureBox.<이름>`) |
| `Released` 풀어놓음 | 세계 — 명령 `Roam`, 길들이기 컴포넌트 `HomeBox` = 상자(복제) | 상자 칸(`bReleased`) + 생물 액터. 저장 · 거둘 때 액터에서 기록을 고쳐 쓴다 |

- 개체 기록(성별 · 색 · 크기 · 거대 · 돌연변이 · 성격 · 능력치 배율 · 주인 · 충성도 · 이름) · 체력 비율 · 최근 까닭은 어느 자리로 옮겨도 그대로다(생태계 접힌 기록과 같은 `FGrimCreatureInstance`). 보관함 · 상자에 넣은 때(`SavedUtcSeconds`, 복제)를 적어 두고 세계로 나올 때 `GrimCreatureStorageMath::GetHealedRatio`(분당 `StoredHealPerMinute` 20%)로 체력을 채운다 — 넣어 둔 동안은 틱이 없다(굶김 · 충성도 사건 없음).
- **데리고 다니는 수** = `GetPartyCreatures()` — 세계에서 `OwnerId` 가 내 PlayerId 이고 `HomeBox` 가 없는 살아 있는 생물. 풀어놓은 생물 · 죽은 생물은 빠진다.

### 상한 — 진행이 올리는 자리

- 서버가 `LimitRefreshSeconds`(1초)마다 셈해 소유 클라이언트에 복제: `PartyLimit = ComputeLimit(PartyLimitByCircle, 서클, PartyLimitTagBonuses, ASC 태그, PartyLimitMax)`.
- 보관함 칸(Q42): `StorageCapacity = ComputeStorageCapacity(StorageBaseCapacity, 쿠스토디아 배움, StorageCapacityByCircle, 서클, StorageCapacityTagBonuses, ASC 태그, StorageCapacityMax)` = 기본 10 + (배웠으면 서클 표 8 · 12 · 16 + 노드 +4), 기본 칸 ~ 40 으로 자른다. 안 배우면 서클 · 노드와 상관없이 10. 배우는 순간 다시 셈한다.
- 서클 표: 맞는 구간 중 `MinCircle` 이 가장 큰 것. 태그 보너스: 플레이어 ASC 에 그 태그가 있으면 더한다 — 성장 트리 노드 효과 키 `CreatureParty` · `CreatureStorage`(태그 `Progression.Node.CreatureParty` · `.CreatureStorage`, `GrimTamingTags` 에 등록)를 트리 표(`SkillTreeNodes.csv`)에 넣으면 노드가 상한을 올린다. 키 하나 = 태그 하나라 겹쳐 쌓이지 않는다 — 더 올리려면 태그(`CreatureParty2` …)를 ini 표에 더한다.

### 옮기기 `UGrimCreatureStorageComponent::Transfer(상자, 종, 씨앗, From, To, bCheckReach)` (서버)

1. 상자가 끼면(From · To 가 Box · Released) 상자가 있고 `BoxManageRadius`(3m) + `BoxServerReachSlack` 안이어야 한다(`NoBox`).
2. From 자리에 그 개체(종 + 씨앗)가 있나 — 상자 칸은 주인이 나여야 한다(`NotFound`).
3. 순수 판정 `GrimCreatureStorageMath::CheckTransfer`: 같은 곳 `Invalid` · 데리고 다님 가득 `PartyFull` · 보관함 가득 `StorageFull`(쿠스토디아를 안 배웠으면 `StorageFullUnlearned` — 「배우면 칸이 늘어난다」 안내) · 상자 가득 `BoxFull`(풀어놓은 것을 거두는 건 칸이 그대로) · 풀어놓기 상한 `ReleasedFull`.
4. 상자 안 ↔ 풀어놓음은 상자 안에서(`ReleaseEntry` · `RecallEntry`). 그 밖은 빼기(데리고 다님이면 내리기 → 기록 → 주인 기록에서 빼기 → 액터 지우기) → 넣기(데리고 다님이면 주인 뒤 3m 에 스폰 · `RestoreOwned` · 따라오기 · 주인 기록에 넣기, 풀어놓기면 상자 둘레에 스폰). 넣기가 실패하면 잃지 않게 보관함에 둔다.
5. 알림(「이름 → 보관함」 · 거절 까닭 — `UGrimTamerComponent::SendNotice`) · 로그 `[Server] 생물 옮김 <종> seed=<씨앗> <From> → <To> (데리고 다님 n/m · 보관함 n/m · 상자 n/m)`.

- 클라이언트는 `RequestTransfer` → `Server_Transfer`(Reliable). 생물 메뉴(F) 「보관함에 넣기」(배웠을 때) · 소지품 「생물」 탭 · 상자 화면 · 콘솔이 같은 RPC 를 부른다.

### 새로 길들일 때 (`UGrimTamingSubsystem::TryTame`)

- 굴림 **전에** `DecideNewTame()`: 데리고 다님 자리 → `Party` / 보관함(자리 — 안 배워도 기본 10칸) → `Storage` / 세계의 자리 있는 상자(`bSendOverflowToBox`, 주인에게 가장 가까운 것) → `Box` / 없으면 `Refused` → 결과 `EGrimTamingResult::NoRoom`, 굴리지 않아 생물이 화내지 않는다(Q64 순서 그대로).
- 계약이 걸리지 않으면 **왜 안 됐는지** 알린다(Q64 답) — 서버가 [마법 실패 알림](./magic#마법-실패-알림-q64)으로: 갈 곳 없음 `TameNoRoom` 「데리고 다님 · 보관함 · 생물 상자가 모두 가득 — 계약이 걸리지 않는다」, 계약 서클 모자람 `TameCircleTooLow`, 받지 않는 생물 `TameRefused`. 소유 클라이언트 HUD 위쪽 한 줄 + 생물 자리에서 `GameplayCue.Magic.Fizzle`(모두에게).
- 성공 뒤 `Party` 가 아니면 `PlaceNewlyTamed` 가 **다음 틱**(파크툼 명중 처리 도중에 액터를 지우지 않게) 보관함 · 상자로 보낸다. 퀘스트 이벤트 `Creature.Tamed.<분류>` 는 그대로 나간다.

### 생물 보관함 = 노말 스킬

- 보관함 자체는 처음부터 있다(기본 10칸, Q42). 쿠스토디아는 칸을 늘리는 노말 스킬이다. 생물 메뉴(F) 「보관함에 넣기」 · 가방 「생물」 탭은 배우지 않아도 쓴다 — 안 배웠으면 「스킬북: 쿠스토디아를 읽으면 보관함 칸이 늘어난다」 안내 한 줄.
- 마법이 아닌 「배우면 쓸 수 있는 능력」. 스킬북 아이템의 `TeachesSpell` 에 `Skill:<ID>` 를 쓰면 노말 스킬 책이다(`GrimCreatureStorageMath::ParseNormalSkill`) — 열 추가 없이 아이템 표를 그대로 쓰려고. 가방 「사용」(`UGrimInventoryComponent::ReadSkillbook`) → `UGrimCreatureStorageComponent::LearnNormalSkill` → `StorageSkillId`(`CreatureStorage`)면 `LearnStorageSkill`(서클 `StorageRequiredCircle` 확인 · 이미 배움이면 못 씀 = 책 남음). 앞으로 「수리」 · 「지우기」가 노말 스킬이면 같은 갈래(`LearnNormalSkill`)에 ID 를 더한다.
- 아이템 `Skillbook_CreatureStorage`(「스킬북: 쿠스토디아」, `Skill:CreatureStorage`) · 화폐 아이템 `Solidus`(솔, Q38 — 화폐는 아직 가방 아이템).
- **상점**: 마법탑 NPC 서비스 `Buy:<아이템 ID>`(`TowerNpcs.csv` — 마렌 「스킬북: 쿠스토디아 사기 (120 솔)」 — Q42 는 탑 · 분탑 서고에서 판다고 했다: 서고 NPC 가 생기면 이 서비스 한 줄을 그 NPC 로 옮긴다). 값은 `[/Script/Grimoire.GrimShopSettings] Prices`, 화폐 `CurrencyItem`. `GrimTowerShop::TryBuy` 가 솔 확인 → 가방 자리 → 솔 빼기 → 아이템 넣기, 대사 맥락 `Buy` · `Buy.NoMoney` · `Buy.BagFull`(`TowerDialogue.csv`, 상점이 있는 NPC 면 표 검사가 요구).

### 거점 생물 상자 `AGrimCreatureBox`

- 놓인 곳 어디서나 쓰는 독립 액터(기본 도형 상자 + 뚜껑 + 초록 문장). 칸 `BoxCapacity`(60, 액터 `CapacityOverride`)는 모든 플레이어 합, 화면은 내 칸만(`OwnerPlayerId` 로 고름). 상자 둘레 3m 에서 F → 관리 화면(생물 안내보다 먼저).
- **풀어놓기**: 상자 둘레 `BoxRoamRadius`(20m)의 25~60% 원 위에 스폰, 집 = 그 자리, 명령 `Roam` → 길들인 AI 판단 `Wander`(집 둘레 `RoamWanderRadius` 6m 를 3~8초마다 걷거나 멈춤), 상자에서 반경 × `RoamLeashScale`(1.5) 밖이면 집으로 옮긴다. 상자마다 `BoxMaxReleased`(10).
- 풀어놓은 생물: 굶김 · 주인 부재를 세지 않는다(M5 먹이 상자 때 다시), 생물 메뉴는 먹이 · 이름 · 놓아주기만(따라오기 · 기다리기 · 타기는 「상자에서 데리고 다니기」 안내). 주인이 나가도 남고(`DespawnOwned` 가 건너뜀), 주인이 접속하면 상자 · 보관함 컴포넌트가 주인 PlayerState 를 다시 잇는다(2초 · 1초 주기). 놓아줌 · 떠남 · 죽음이면 상자 칸에서도 지운다(`ForgetCreature`).
- 세이브 `CreatureBox.<액터 이름>` v1 = `GrimCreatureStorageMath::SerializeBoxEntries`(수 + 항목마다 풀어놓음 + 가진 개체 기록). 쓸 때 풀어놓은 생물 액터로 기록을 고치고, 읽으면 `SyncReleased`(없는 액터를 다시 내놓고 칸에 없는 액터는 거둔다). 게임 중 놓은 상자는 세이브 시스템의 「스폰 액터」로 다시 만들어진다.
- **건축으로 놓기(M4)**: 조각 표 `BuildPieces.csv` 의 시설 `CreatureBox`(「생물 상자」, 나무 10 · 돌 4, 체력 200) → `AGrimBaseCreatureBox`(`Base/GrimBaseStations`) = 이 상자 + `UGrimStructureComponent`(노후 · 불 · 「수리」 · 철거). 시설이라 거점 밖에도 놓이지만 밖은 빨리 닳는다(Q66 · Q74).
- 지은 상자가 부서지거나 헐리면 칸마다 꺼내 **가까운 다른 상자(자리 있는) → 주인의 보관함**(접속 중 · 자리 — 안 배워도 기본 10칸) 순으로 옮긴다. 둘 다 안 되면 그 생물은 사라진다(경고 로그). 레벨에 놓은 상자 · 콘솔 상자(`AGrimCreatureBox`)는 부서지지 않는다 — 게임 중 지워지면 예전처럼 기록이 함께 사라진다.
- **남은 것**: `GetRoamRadius()` 를 거점 반경으로, `FindNearestWithRoom` 을 「내 거점의 상자」로, 생물을 잃는 경우(상자 · 보관함 모두 꽉 참 · 주인 접속 안 함)의 규칙(「칸이 비어야 철거」 · 그 자리에 풀어놓기 등). 상자 액터 · 칸 · 세이브 형식은 그대로 둔다. M5 일꾼은 `Released` 생물에서 고른다(작업 보드).

### 화면 `UGrimCreatureStorageWidget`

- 한 클래스, 두 쓰임: 소지품(L)의 탭 「생물」(`UGrimCreatureStorageTab` 이 `BuildContent` 에서 만들어 끼우고 `OnShown` · `OnHidden` 에 `SetTabActive`), 상자 관리 화면(`SetStandaloneScreen`, F · Esc 로 닫음, 멀어지면 닫음).
- 칸: 데리고 다님 · 생물 보관함 · (상자 화면이면) 거점 생물 상자. 줄 = 색 칸(영역마다) + 「이름 (종) · 암/수 · 돌연변이 / 크기 ×1.05 거대 · 충성도 72 충직 · 체력 80%」 + 단추(보관함에 넣기 · 데리고 다니기 · 상자로 · 풀어놓기 · 거두기). 머리 줄 「데리고 다님 n/m · 보관함 n/m」(안 배워도 보관함 n/10). 쿠스토디아를 안 배웠으면 「읽으면 보관함 칸이 늘어난다」 안내 한 줄(마렌에게서 산다).
- 0.5초마다 다시 읽고, 줄(자리 · 종 · 씨앗)이 바뀌었을 때만 줄을 다시 만든다(누르는 도중에 단추가 사라지지 않게). 단추는 포커스를 받지 않는다(가방 키 · F 가 계속 게임으로).

### 설정 `[/Script/Grimoire.GrimCreatureStorageSettings]` (제안)

| 값 | 기본 |
|---|---|
| `PartyLimitByCircle` · `PartyLimitTagBonuses` · `PartyLimitMax` | 1서클 3 · 3서클 4 · 5서클 5 / `Progression.Node.CreatureParty` +1 / 8 |
| `StorageSkillId` · `StorageRequiredCircle` | `CreatureStorage` · 1 |
| `StorageBaseCapacity` | 10 — 쿠스토디아 없이 처음부터(Q42 답) |
| `StorageCapacityByCircle` · `StorageCapacityTagBonuses` · `StorageCapacityMax` | 배우면 더하는 칸 8 · 12 · 16 / `Progression.Node.CreatureStorage` +4(배웠을 때만) / 전체 상한 40 |
| `StoredHealPerMinute` | 0.2(분당 최대 체력 20%) |
| `BoxCapacity` · `BoxManageRadius` · `BoxServerReachSlack` | 60 · 300cm · 250cm |
| `BoxRoamRadius` · `BoxMaxReleased` · `RoamWanderRadius` · `RoamLeashScale` | 2000cm · 10 · 600cm · 1.5 |
| `bSendOverflowToBox` · `LimitRefreshSeconds` | True · 1초 |

`[/Script/Grimoire.GrimShopSettings]`: `CurrencyItem=Solidus`, `Prices=(Skillbook_CreatureStorage, 120)` · `(Wand_Basic, 10)`(기본 완드 — [완드](./magic#완드-agrimwand), Q71).

### 콘솔 · 확인

| 명령 | 하는 일 |
|---|---|
| `Grim.Taming.Storage <From> <To> [종 id\|*] [지연초]` | 로컬 플레이어: 그 자리(Party · Storage · Box · Released)의 첫 개체를 옮긴다(곁의 상자) — RPC 그대로 |
| `Grim.Taming.SpawnBox [거리cm] [지연초] [Host\|Client]` | 서버: 그 플레이어 앞에 거점 생물 상자 |
| `Grim.Taming.StorageLearn [1\|0] [지연초]` | 서버: 모든 플레이어의 보관함 배움(스킬북 없이) |
| `Grim.Taming.Box <Open\|Close> [지연초]` | 로컬 플레이어: 곁의 상자 관리 화면 |
| `Grim.Taming.StorageDump [지연초] [Local\|Remote]` | 이 프로세스가 본 그 플레이어의 상한 · 목록(`storage` · `storagerow` 줄 — 비교용, `storagetext` 화면 글) |

- `Scripts/Creatures/net_check_storage.ps1` — 리슨 서버 + 클라이언트 헤드리스: 호스트 · 클라이언트가 각자 파크툼으로 길들임 → 스킬북 아이템 읽기 → 넣기 → 목록 → 꺼내기 → 상자 → 풀어놓기 → 데리고 다니기. 서버가 본 클라이언트 목록(Remote)과 클라이언트가 본 목록(Local)을 다섯 번 비교하고, 양쪽 단계별 기대 줄 · 서버 로그(두 플레이어 몫)를 본다(`NETCHECK_RESULT fail=0`).
- 테스트 `Grimoire.Creatures.Storage.*`: `Limits`(서클 표 · 태그 보너스 · ini) · `Rules`(옮기기 판정 · 갈 곳 · 회복 · 노말 스킬 · 상점 이름 · 대사 표 검사) · `Serialize`(상자 세이브 왕복 · 깨진 데이터) · `Actor`(안 배움 10칸 → 스킬북으로 배움 18칸 → 상한 → 넘친 계약은 보관함 → 넣기 · 꺼내기 개체 기록 그대로 → 보관함 · 데리고 다님 가득 → 상자 · 풀어놓기 · 거두기 · 남의 칸 → 상자 가득 → 보관함 · 상자 세이브 왕복) · `ReleasedSave`(풀어놓은 생물 다시 불러오기 · 놓아주면 칸에서 지움) · `Shop`(솔 모자람 · 산다 · 팔지 않음 · 기본 완드 10 솔 두 번) · `BaseCapacity`(Q42 — 안 배워도 넣고 꺼냄 → 10마리 → `StorageFullUnlearned` → 상자 없으면 계약 거절 · 상자 있으면 상자 → 배우면 18칸).

## 충성도 (갈래 T)

- 0~100, 길들인 직후 50. 사건 하나 = 기본 변화량(`Loyalty.EventDeltas`) × 성격 배율 × 세기, 0~100 으로 자른다. 실제 바뀐 값을 까닭 맨 앞에 넣고 세 줄만 남긴다(복제 — 메뉴 「최근에 오르내린 까닭」).
- 단계 태그 `Creature.Loyalty.{Devoted, Normal, Discontent, Rebellious, Gone}` + `Creature.State.Tamed` 는 각 기기가 복제된 충성도로 붙인다. 0 이 되면 **떠난다**(야생으로, 주인 기록에서 지움).

| 사건 | 변화 | 언제(서버, `TickSeconds` 5초마다 셈) |
|---|---|---|
| 먹이 · 좋아하는 먹이 | +3 · +6 | F 먹이 주기 |
| 쉼 | +2 | 기다리기 · 잠이 120초 이어짐 |
| 치료 | +4 | 플레이어가 준 회복이 최대 체력 10% 이상(신성 치유 마법 자리) |
| 함께 싸워 이김 | +3 | 주인이나 그 생물이 쓰러뜨린 생물이 25m 안 |
| 함께 탐험 | +2 | 주인 곁(40m 안)에서 지난 지점보다 60m 옮김 |
| 맞는 환경 | +1 | 서식지 태그가 맞는 서식 구역 안 240초 |
| 함께 지낸 날 | +1 | 1440초마다 |
| 굶김 | −5 | 먹이 없이 900초, 그 뒤 180초마다 |
| 혹사 | −4 | 지쳤는데(고단함 90) 싸움 · 탑승 30초 |
| 다친 채 | −3 | 체력 50% 아래 180초 |
| 기절 | −8 | 체력 0 |
| 맞지 않는 환경 | −2 | 불 주속성이 젖음 · 물 주속성이 불붙음 60초 |
| 다툼 | −2 | 다른 생물과 싸움(60초에 한 번) |
| 주인 부재 | −3 | 주인 80m 밖 300초, 그 뒤 300초마다 · 접속하지 않은 한 시간마다 |

| 단계 | 충성도 | 순종 | 탑승 속도 | 작업 속도(M5 자리) | 타기 거부 | 떨어뜨림(8초마다) |
|---|---|---|---|---|---|---|
| 충직 | 80~100 | 100% | ×1.15 | ×1.2 | 0 | 0 |
| 보통 | 50~79 | 100% | ×1 | ×1 | 0 | 0 |
| 불만 | 20~49 | 75% | ×0.9 | ×0.8 | 10% | 0 |
| 반항 | 1~19 | 40% | ×0.8 | ×0(일 거부) | 60% | 25% |
| 떠남 | 0 | — | — | — | — | — |

- **성격(Q27)** `Personalities[]` — 사건별 배율(없으면 오름 · 내림 배율): 먹보(먹이 ×2 · 굶김 ×2) · 일벌레(혹사 ×0.5 · 쉼 ×0.5 · 작업 ×1.2) · 겁쟁이(기절 ×2 · 다툼 ×1.5 · 함께 이김 ×0.5) · 싸움꾼(함께 이김 ×2 · 다툼 ×0.5 · 쉼 ×0.75) · 잠꾸러기(쉼 ×2 · 혹사 ×1.5 · 작업 ×0.8). 개체 기록의 `Personality` 는 갈래 C 가 굴린다.
- 「충직 — 주인이 쓰러지면 지킨다」 · 번식 조건(50 이상)은 M4~M5 자리.

## 탑승 (갈래 T)

- **Possess 대신 탄 사람 입력 중계**. 처음 설계(생물 폰 Possess)를 바꿨다 — 까닭: ASC 는 PlayerState 에 있지만 아바타 · 조준 제공자(컨트롤러의 조준 컴포넌트) · 카메라 · 가방 · 세이브(`PlayerState->GetPawn()` 으로 위치 · 생존) · HUD 가 모두 「컨트롤러의 폰 = 플레이어 캐릭터」에 기대고 있어, 폰을 바꾸면 탄 채 시전 · 저장이 깨진다. 그래서:
  1. F 메뉴 「타기」 → 서버 `UGrimMountComponent::TryMount`(탈것 종 · 주인 · 기절 아님 · 단계의 거부 굴림).
  2. 복제되는 `Rider` 를 각 기기가 받아 캐릭터를 생물 캡슐(메시에 `Saddle` 소켓이 생기면 그 소켓)에 `MountProfiles.SaddleOffset`(켈피 0,0,125 × 크기)으로 붙이고, 캐릭터 이동 컴포넌트 틱 · 충돌을 끈다. 생물 캡슐은 `WorldDynamic`(마법 볼트)을 무시한다 — 탄 사람의 볼트가 자기 탈것을 맞히지 않게.
  3. 탄 사람 화면: 이동 입력은 그대로 캐릭터에 쌓이고(`AddMovementInput`), `UGrimTamerComponent` 가 매 틱 `ConsumeMovementInputVector` 해서 서버로(`Server_MountInput`, 비신뢰 · 33ms 간격 또는 바뀔 때). 서버가 생물에 `AddMovementInput` — 0.3초 안 오면 멈춘다. 리슨 호스트는 바로.
  4. 탑승 속도 = 종 달리기 속도 × 프로필 `SpeedScale` × 단계 배율. 카메라 붐 +180cm(탄 사람 화면만).
- **탄 채 시전(Q9)**: 컨트롤러 · 캐릭터 · ASC 가 그대로라 시전 입력 · 조준 · 판정이 걷는 때와 같은 길이다.
- **그리모어(Q25)**: 탄 사람 ASC 에 `State.Mounted`(루즈 — 각 기기가 붙임) → MagicFX 의 거두기 판정이 「그냥 달릴 때 허리춤, 조준 · 시전하면 띄우고 `MountedCombatHoldSeconds` 4초 뒤 거둠」을 한다.
- **물 위 달리기**: 켈피(`WaterMovement = RunOnSurface`)가 탄 채 물 볼륨(PhysicsVolume `bWaterVolume`)에 들어가 헤엄 상태가 되면 `MOVE_Flying` 으로 바꿔 볼륨 윗면 + 2cm 에 붙여 달린다. 발아래가 물이 아니면 내려선다. 물 볼륨이 없는 맵은 `WaterSurfaceTag`(`Grim.Water`) 태그 표면을 밟고 달린다(충돌 있는 표면 — 자리, 로그만). 안 탔을 때는 갈래 C 의 부력 헤엄 그대로.
- 내리기: F(탄 동안 안내 「[F] 내리기」) → 생물 오른쪽 160cm 에 선다. 반항 단계면 8초마다 25% 로 떨어뜨린다. 탈것이 쓰러지거나 놓아지면 내려진다.

## 길들이기 화면 · 입력

- F(`Behavior.MenuKey`): 4m 안 내 생물 → 「[F] <이름> 돌보기」 → 메뉴. 야생이고 먹일 것이 있으면 「[F] <종> 에게 먹이 주기」. 줍기 · 냄비(같은 F)가 가까우면 그쪽이 먼저(생물 안내를 내지 않는다).
- 메뉴(`UGrimCreatureMenuWidget`, UI 전용 입력): 이름 · 종 · 성별 · 거대 · 돌연변이 · 성격 · 체력 · 명령, 충성도 막대 + 단계, 최근 까닭 세 줄, 1 따라오기 · 2 기다리기 · (3 타기) · 먹이 주기 · 이름 짓기 · 놓아주기 · 닫기. 멀어지거나 놓아지면 닫힌다.
- 알림 한 줄(서버 → 소유 클라이언트 `Client_Notice`): 길들였다 · 저항 · 서클 모자람 · 명령 무시 · 먹이 · 이름 · 탔다 · 거부 · 떠남.

## 설정 `[/Script/Grimoire.GrimTamingSettings]`

| 묶음 | 값(기본) |
|---|---|
| `Capture` | 위 「포획 확률」 + `FedMemorySeconds` 120 · `EnrageSeconds` 6 · `TamedHealToRatio` 0.5 |
| `Loyalty` | `InitialLoyalty` 50 · `EventDeltas[]` · `Stages[]` · `TickSeconds` 5 · 굶김 900/180 · 쉼 120 · 다친 채 0.5/180 · `HealMinRatio` 0.1 · 고단함 1/0.5/90 · 혹사 30 · 함께 이김 25m · 탐험 60m · 환경 240/60 · 날 1440 · 다툼 60 · 부재 80m/300/300 · 접속 안 함 최대 5 · 기절 20초/0.2 · 떨어뜨림 8초 · 까닭 3줄 |
| `Personalities[]` | 성격 6(None 포함) |
| `Behavior` | 따라오기 3.5m · 달림 9m · 옮김 50m · 부르기 3m · F 거리 4m · 서버 여유 2.5m · `MenuKey` F · 이름 16자 |
| `Mount` | `Profiles[]`(켈피) · 입력 시간 0.3초 · 보내기 33ms · 수면 +2cm · `WaterSurfaceTag` |
| `FoodRules[]` · `DietGroups[]` | 위 「먹이 주기」 |
| `CaptureBonusEffectTag` · `TamedEventPrefix` | `Taming.CaptureBonus` · `Creature.Tamed.` |

## 콘솔 — 길들이기 (개발 빌드)

| 명령 | 하는 일 |
|---|---|
| `Grim.Taming.Grant [서클=3] [지연초]` | 서버: 모든 플레이어 서클을 올리고 파크툼 6개를 가르친다 |
| `Grim.Taming.Cast <마법 에셋> <종 id\|*> [누르는 초] [지연초]` | 로컬 플레이어: 그 종의 가장 가까운 야생 생물을 겨눠 시전(탄 채도) |
| `Grim.Taming.Act <Follow\|Wait\|Mount\|Feed\|Release\|Dismount\|Name> [종 id\|*] [이름] [지연초]` | 로컬 플레이어: 가장 가까운 내 생물에게(먹이는 야생도) — RPC 그대로 |
| `Grim.Taming.Move <앞> <옆> <초> [지연초]` | 로컬 플레이어: 카메라 기준 이동 입력(헤드리스 탑승 이동) |
| `Grim.Taming.Loyalty <사건\|Set> [값] [종 id\|*] [지연초]` | 서버: 길들인 생물에 충성도 사건(세기) 또는 값 |
| `Grim.Taming.Dump [지연초]` | 이 프로세스에서 본 생물의 주인 · 충성도 · 이름 · 명령 · 탄 사람 · `State.Mounted` · 붙음 · 까닭 · 단계 태그 + 위치 줄 |
| `Grim.Taming.ForceRoll <0~1>` (콘솔 변수) | 포획 굴림 고정, 음수 = 끔 |

- `Scripts/Creatures/net_check_taming.ps1` — 리슨 서버 + 클라이언트 헤드리스: 클라이언트가 물줄기 · 플라미스로 재칼로프를 약하게 → 파크툼 → 따라오기 → 켈피 파크툼 아쿠아 → 이름 → 타기 → 타고 달리기 → 탄 채 플라미스 → 내리기 → 기다리기. 덤프 여섯 번을 서버 · 클라이언트끼리 비교하고 꼭 있어야 할 줄(길들임 · 퀘스트 이벤트 · 탔다 · 내렸다 · 탄 채 발동)을 본다(`NETCHECK_RESULT fail=0`).
- 테스트 `Grimoire.Taming.*`: `Chance`(몫 · 자르기 · 서클) · `Loyalty`(사건 · 자르기 · 단계 · 까닭 · ini 표) · `Personality`(배율 · ini 성격) · `Commands`(길들인 AI 판단 · 화난 야생 · 순종 · 거부 · 떨어뜨림 · 이름) · `SaveRoundTrip`(기록 왕복 · 깨진 데이터) · `Assets`(파크툼 6 · 스킬북 · 켈피 · 먹이 표) · `Actor`(길들임 → 태그 → 기절 → 떠남).

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
- 묶음 2(갈래 T, 이번): 길들이기(파크툼 6) · 충성도 · 성격 · 명령 · 탑승(켈피) · 세이브 참여 · 튜토리얼 `Creature.Tamed.Small` — 위 「길들이기」 · 「충성도」 · 「탑승」.
- 노동 · 번식 · 영역 다툼 · 날씨 반응은 M4~M5. 그때의 설계(작업 보드 · 번식장 · 유전 IV)는 아래에 남겨 둔다.

## 나중 설계(M4~M5)

- **노동** `UGrimWorkerComponent` + `AGrimBaseCore`: 거점 핵의 작업 보드 `{ WorkTag, TargetActor, Priority }` → 적성 `WorkAptitudes[WorkTag] > 0` 인 요청을 우선순위로 → 해당 속성 자극(물 생물이 밭에 `Water` — 플레이어 마법과 같은 길). 일꾼도 배고픔 · 피곤함.
- **번식** `UGrimBreedingComponent` + `AGrimBreedingPen`: 같은 `BreedingGroup` 암수(충성도 50 이상) → `GestationMinutes` 타이머(서버, 세이브). 자식: 스탯마다 부모 IV 하나 또는 평균 ±2, 색은 영역마다 부모 중 하나(+ 흔들림 새로), 성별 반반, 크기 부모 평균 ±0.05(거대 유전 `GiantInheritChance`). 돌연변이 `p = 3% + 부모 돌연변이 1마리당 +3%`(Q8) → 영역 하나 이상 변이 팔레트, IV 상한 40.
- 충성도 · 탑승은 M3 갈래 T 에서 만들었다(위). 일꾼의 작업 속도는 `GrimTamingMath::GetWorkSpeedScale`(단계 × 성격) 자리만 있다.
- **영역 다툼**: 대형 종 `TerritoryRadius` — 다른 대형 종이 들어오면 서로 공격, 체력 30% 이하면 후퇴.
