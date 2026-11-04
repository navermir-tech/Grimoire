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
- **폰이 바뀔 때마다(서버)**: 완드가 없으면 스폰하고 `Owner = 폰`, 폰 스켈레탈 메시의 `WandSocketName`(본 · 소켓 모두)에 `KeepRelativeTransform` 으로 부착. 메시 · 소켓이 없으면 폰 루트. 부착은 완드의 움직임 복제로 클라이언트에 간다.
- `LearnSpell(Spell)` — 서클 확인 → `GiveAbility(SourceObject = Spell)` → `KnownSpells` 에 추가(복제). 첫 마법이면 선택.
- `CycleSelection(±1)` · `SelectSpell(i)` — 선택 Spec 에만 `Input.Spell.Cast` 를 넣고 바뀐 Spec 을 `MarkAbilitySpecDirty`. `SelectedIndex` 복제, `OnSelectedSpellChanged`(서버 · 클라이언트) 로 HUD 갱신.
- `UGrimSpellSelectAbility` 는 ServerOnly — 입력은 클라이언트, 선택 변경은 서버, 결과는 복제.

## 마법 어트리뷰트 `UGrimMagicAttributeSet`

`Mana` · `MaxMana` · `Circle`(복제). `Mana` 는 현재값 · 기본값 모두 `[0, MaxMana]` 로 자르고, `MaxMana` 가 줄면 `Mana` 도 줄인다. `CastSpeed` · `SpellPower` · 속성별 위력 어트리뷰트는 아직 없다 — 완드 보정은 완드 액터 값을 직접 읽는다.

## 완드 `AGrimWand`

- 장비 액터(복제 · 움직임 복제). M1 모양은 엔진 기본 실린더(가늘게), 끝에 `Muzzle` 컴포넌트.
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

### 구성

- `UGrimElementalStateComponent` — 사물 · 생물 · 플레이어가 가진다.
  - `TArray<FGrimElementalState> States` : `{ FGameplayTag StateTag; float Intensity; float RemainingTime; }` — 예: `State.Burning(2.0)`, `State.Wet(1.0)`
  - `FGameplayTagContainer Material` : `Material.Flammable`, `Material.Conductive`, `Material.Absorbent` …(반응 조건)
  - `ApplyStimulus(const FGrimElementalStimulus&, AActor* Instigator)` — 서버. 규칙표를 조회해 상태를 바꾸고 `OnStateChanged` 브로드캐스트.
  - 복제: `States` 배열(FastArraySerializer). 클라이언트는 `OnRep` 에서 연출(파티클 · 머티리얼 파라미터).
- `FGrimElementalStimulus` : `{ FGameplayTag Element; float Intensity; EGrimStimulusSource Source; }` — Source 는 Spell · Weather · Propagation · Creature.
- `UGrimElementalReactionLibrary` : 규칙 조회 · 적용 · 전파(타이머로 반경 오버랩 → 이웃에 `ApplyStimulus(Source = Propagation)`).

### 반응 규칙표 `DT_ElementalReactions`

| 열 | 뜻 |
|---|---|
| `RequiredState` | 현재 상태 태그(없음 = 어떤 상태든) |
| `RequiredMaterial` | 재질 조건 |
| `Element` | 들어온 자극 속성 |
| `MinIntensity` | 자극 세기 하한 |
| `ResultState` / `IntensityDelta` / `ClearStates` | 결과 |
| `Propagate` | 주변 반경 · 전파 자극(불이 옆으로 번짐) |
| `Cue` | GameplayCue 태그(연출) |

### 양초 예시 데이터

| RequiredState | Material | Element | Min | 결과 |
|---|---|---|---|---|
| — | Flammable | Fire | 0.5 | `State.Burning` +1 |
| Burning | — | Wind | 0.3 | Intensity +1 (최대 3) |
| Burning | — | Wind | 2.5 | Clear Burning (돌풍에 꺼짐) |
| Burning | — | Water | 0.3 | Clear Burning, `State.Wet` +1 |
| Wet | — | Fire | 1.0 | Wet −1 (마름) |
| Burning | — | Dark | 0.5 | Intensity −1 |

양초 블루프린트 `BP_Candle` 은 **코드가 없다**: 메시 + `GrimElementalState`(Material = Flammable) + 상태 → 연출 바인딩(불꽃 Niagara 크기 = Burning Intensity).

### 날씨 · 생물과의 연결

- `UGrimWeatherSubsystem` 은 비가 올 때 **야외**(지붕 없음: 위로 트레이스) 사물에 주기적으로 `Water` 자극을 준다 → 불이 꺼진다. 번개는 낙뢰 지점에 `Wind + Light` 고강도 자극.
- 생물 · 플레이어도 같은 컴포넌트: `State.Wet` 이면 `UGrimDamageExecution` 이 번개 · 얼음 피해를 올린다. `State.Burning` 은 주기 피해 GE 를 ASC 에 적용한다(컴포넌트가 ASC 를 찾아 브리지).

### 성능 규칙

- 사물 컴포넌트는 **상태가 없을 땐 틱하지 않는다**. 상태가 생길 때만 타이머 등록.
- 전파 오버랩은 전용 채널 `ECC_Elemental`, 반경 상한 · 틱당 처리 상한(예산 200/프레임 — 측정 후 조정).
- Net Dormancy `DORM_DormantAll` 기본, 상태 변화 시 `FlushNetDormancy`.

## UI

- HUD: 단축 슬롯(마법 아이콘 · 쿨다운 · 서클 색), 마나, 조준점(대상의 속성 상태 아이콘 표시 — "젖음" 보이기).
- 그리모어(`WBP_Grimoire`): 배운 마법 · 필터 · 슬롯 배치 · 완드 정보 · 발견한 반응 규칙 도감.

## M1 범위

불씨 · 물줄기 · 미풍 3개 마법, 완드 1종, 양초 · 모닥불 · 마른 풀 3개 사물, 반응 규칙표 10행, 상성 표. 서클 UI 는 숫자만.

아직 없는 것: 시전 몽타주 · 머즐 이펙트 · 광선 연출, 그리모어 UI, 스킬북 아이템, 완드 정의 데이터 · 장착 해제, 서클 승급.
