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
| `ManaCost` · `ChannelManaPerSecond` · `Cooldown` · `CooldownTag` | 시전 비용 · 누르는 동안 초당 비용(Stream 만, Q19) · 쿨다운(`Cooldown.Spell.*`, 마법마다 달라야 서로 막지 않음) |
| `CastMontage` · `CastWindup` · `bChanneled` | 시전 동작(연출 쪽이 Cast 큐에서 재생) · 입력부터 실제 발사까지 초 · 누르고 있는 동안 지속 |
| `Delivery` | `Bolt` · `Stream` · `Cone` · `Beam` · `Lob`(열거형만, Bolt 로 처리) — [투사체 · 시전 감각](./spell-feel#전달-방식-delivery-다시-정의) |
| `Radius` · `Range` | 판정 반지름(Bolt 구 · Stream 구간 스윕 · Beam 스윕) · 사거리(조준 트레이스 · 락온 거리 · Bolt 수명) |
| `ProjectileClass` · `BoltSpeed` · `HomingStrength` | Bolt: 비우면 `AGrimSpellProjectile` · 속도 · 락온 대상 유도(초당 최대 회전 각, 0 = 없음) |
| `StreamLaunchSpeed` · `StreamGravityScale` · `StreamTickInterval` · `StreamSegments` · `StreamSplashRadius` · `StreamStimulusInterval` | Stream: 뿜는 속도 · 중력 배율 · 서버 판정 주기 · 포물선 구간 수 · 떨어진 곳 물보라 반지름 · 한 대상에 자극을 주는 간격 |
| `ConeHalfAngle` · `ConeRange` · `ConeFalloff` | Cone: 반각 · 거리 · 거리별 자극 세기 `FGrimConeFalloff { NearDistance, NearIntensity, FarIntensity, Exponent }` |
| `Damage` · `ImpactEffect` | 기본 피해(0 이면 피해 없음). 비우면 `UGrimGE_SpellDamage` |
| `Stimulus` | 사물에 주는 속성 자극 `FGrimElementalStimulus`(속성 · 세기 · 출처). Cone 은 세기를 `ConeFalloff` 로 바꾼다 |
| `VisualSet` | 연출 묶음 `UGrimSpellVisualSet`(Core/GrimSpellFXTypes). 연출 쪽(MagicFX) 큐 처리기가 읽는다. 비어도 게임은 돈다 |
| `Color` | VisualSet 이 없을 때 투사체 임시 구 색(엔진 기본 구 + `BasicShapeMaterial` 의 `Color`) |

- 에셋 `Content/Grimoire/Magic/Spells/DA_Spell_*` 은 `Scripts/Magic/make_spells.py` 가 만든다 — **스크립트가 수치의 정본**이다.
- 하나의 `UGrimSpellAbility` 클래스가 여러 마법을 낸다: 마법서가 `GiveAbility` 할 때 Spec 의 `SourceObject` 에 정의를 넣고, 어빌리티는 Spec 에서 정의를 꺼낸다.

### 시작 마법 (M1.5 제안값)

| 에셋 | 마법 | 속성 | 전달 | 마나 | 쿨다운 | Windup | 피해 | 자극 세기 | 사거리 |
|---|---|---|---|---|---|---|---|---|---|
| `DA_Spell_Ember` | 불씨 | 불 | Bolt 7500cm/s, 반지름 10, 유도 60°/s | 10 | 0.6s | 0.1s | 10 | 1.0 | 25m |
| `DA_Spell_WaterJet` | 물줄기 | 물 | Stream(누르는 동안) 1400cm/s, 중력 ×1, 0.1초 · 8구간, 반지름 15, 물보라 40 | 2 + 초당 10 | 0.3s | 0.05s | 자극마다 4 | 1.0, 대상마다 1초에 한 번 | 12m |
| `DA_Spell_Breeze` | 미풍 | 바람 | Cone 반각 30° · 10m | 6 | 0.8s | 0.1s | 3 | 1.5m 안 2.8 → 10m 0.6 | 10m |

미풍 세기는 양초 규칙(바람 0.3~2.5 키움, 2.5 이상 꺼짐)에 맞춘 거리 곡선이다: 시작점(완드 끝)에서 약 2.7m 안이면 돌풍(끔), 그 밖은 키움. 물줄기는 처음 닿는 순간 자극을 주므로 양초는 한 번 스치면 꺼진다.

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
| `MaxLagCompensation` | 0.15 — 서버 Bolt 를 시전자 지연만큼 앞으로 당기는 상한(초) |
| `CloseAimDistance` · `ChestHeight` | 200 · 40 — 조준점이 2m 안이면 가슴(폰 위치 + 40cm)에서 쏜다 |
| `HomingMaxAngle` | 60 — Bolt 유도: 대상이 진행 방향에서 이보다 벗어나면 휘지 않는다 |
| `MaxConeTargets` · `StreamKeepTargetAngle` | 16 · 45 — Cone 대상 상한 · Stream 이 시작 때 잡은 대상을 계속 따라가는 각 |
| `MaxAimOriginError` · `MaxAimAngleError` · `AimRangeSlack` | 800 · 60 · 300 — 서버의 TargetData 검증 허용치 |

## 시전 흐름

1. 입력: 캐릭터가 `UGrimAbilitySystemComponent::AbilityInputTagPressed(Input.Spell.Cast)`. **선택된 마법 Spec 만** `Input.Spell.Cast` 를 동적 태그로 가지므로 그 마법만 발동한다.
2. `UGrimSpellAbility`(InstancedPerActor · LocalPredicted):
   - `CanActivateAbility` — 정의 있음 · 완드 있음(완드 없으면 주먹 기본 공격만, Q4) · `Circle` ≥ 정의의 서클 · 쿨다운 · 비용.
   - `CommitAbility` — 비용 `UGrimGE_SpellCost`(즉시, `Mana += SetByCaller.Magic.ManaCost`), 쿨다운 `UGrimGE_SpellCooldown`(지속 `SetByCaller.Magic.Cooldown` 초, `CooldownTag` 를 Spec 의 `DynamicGrantedTags` 로). 클라이언트도 예측 적용한다.
   - 완드 보정: 비용 = `ManaCost × (1 − ManaEfficiency)`, 쿨다운 = `Cooldown ÷ CastSpeed`(`GrimMagicMath`). 누르는 마법은 시작할 때 `ManaCost` + 첫 판정 몫이 있어야 한다.
3. 조준 · 네트워크 — 자세한 표는 [투사체 · 시전 감각 — 네트워크](./spell-feel#네트워크-—-내-화면에서는-즉시):
   - **시전자(로컬)**: `UGrimTargetingStatics::FindAimProvider(폰)->GetCurrentAim(사거리)`(Cone 은 `ConeRange`), 제공자가 없으면 카메라 정면 Visibility 트레이스 → `FGrimAimResult`. 원격 클라이언트는 이것을 `FGameplayAbilityTargetData_SingleTargetHit`(TraceStart = 시선 시작, Location = 조준점, Actor = 대상, Item = 모드)로 서버에 보내고, 바로 **Cast 큐(예측)** · Stream 이면 **Channel 큐(예측, 끝나면 거둠)** 를 띄우고, Bolt 면 `CastWindup` 뒤 **가짜 볼트**를 쏜다.
   - **서버**: TargetData 를 받으면 검증(`IsClientAimPlausible` — 시선 시작이 폰 근처, 조준점이 사거리 안, 방향이 폰 조준과 60° 안. 대상은 `CanBeTargetedBy` · 거리를 다시 보고 조준점을 서버에서 다시 구함). 어긋나면 서버 조준(카메라 캐시 · 컨트롤 회전). 그 콜백 안(클라이언트 예측 키 창)에서 같은 Cast · Channel 큐를 실행 → 시전자는 두 번 보지 않는다. 지연 = 핑 ÷ 2 로 `SplitLag` → 남은 Windup 만큼 기다렸다가 판정, 이미 지난 시간은 Bolt 를 앞으로 당김(`CatchUp`, 상한 0.15초).
   - 리슨 서버 호스트 · 단독은 TargetData 없이 바로 서버 경로.
   - Bolt · Cone · Beam 은 **서버가 판정 뒤 어빌리티를 끝낸다**(복제). 시전자가 먼저 끝내면 서버가 판정 전에 끝날 수 있어서다. 끝 복제가 안 오면 2초 뒤 스스로 끝낸다. Stream 은 시전자가 떼면(`InputReleased`) 끝낸다 — 서버는 판정을 한 번도 못 했으면 끝나기 전에 한 번 한다.
4. 판정(서버, 시작점 · 방향은 시차 보정 `GrimSpellDelivery::ComputeLaunch` — 완드 끝 → 조준점, 2m 안은 가슴에서):
   - **Bolt**: `AGrimSpellProjectile` 스폰(복제 · 위치 복제). 지형 · 사물(WorldStatic/Dynamic)에 막히고 폰 · 물리체는 겹쳐 맞는다. 시전자는 무시. 수명 = `Range ÷ BoltSpeed`. 대상이 있으면 틱마다 `SteerTowards`(초당 `HomingStrength`°, 60° 넘게 빗나가면 안 휨). 맞으면 Impact 큐.
   - **Stream**: `StreamTickInterval` 마다 `TraceStreamArc` — 조준점을 지나는 낮은 호(`SolveLaunchDirection`)를 `StreamSegments` 구간으로 구 스윕, 첫 지형 · 사물에서 끝나고 그 자리 `StreamSplashRadius`(40cm) 안의 것도 맞는다(바닥의 양초를 낮게 스칠 때 바닥이 먼저 닿으므로). 걸린 대상마다 `StreamStimulusInterval` 에 한 번 자극 · 피해 · Impact 큐(처음 닿으면 바로). 틱마다 `ChannelManaPerSecond × 주기` 마나, 모자라면 끝. 시작 때 잡은 대상이 폰 조준에서 45° 안이면 계속 따라가고, 아니면 지금 시선으로.
   - **Cone**: 시작점에서 `ConeRange` 구 오버랩 → 받는 쪽(속성 사물) · ASC 가 있는 것 중 `IsInCone`(반각) · 시야(벽 너머 제외) → 가까운 순 16개까지, 거리별 세기 `ConeIntensityAt` 로 자극 · Impact 큐.
   - **Beam**: 구 스윕(`SweepMultiByObjectType`) — 폰 · 물리체는 지나가며 맞히고, 첫 지형 · 사물에서 멈춘다. 맞은 곳마다 Impact 큐.
5. 맞은 대상마다 `UGrimSpellAbility::ApplySpellHit`:
   - 대상 ASC(액터 → 폰이면 PlayerState 순으로 찾음)가 있으면 피해 GE 적용. 피해 Spec 은 `SetByCaller.Magic.Damage = Damage × 완드 속성 위력` 과 에셋 태그 `Element.*` · `Spell.*` 를 가진다.
   - `UGrimElementalStatics::ApplyStimulusToActor(대상, Stimulus, 시전 폰)` — 받는 쪽(`IGrimElementalReceiver`)이 있으면 반응 규칙. Magic 은 구현체를 모른다. Cone 은 세기를 거리별 값으로 바꿔 준다.

### 연출 큐 (GameplayCue)

Magic 은 그리지 않고 `GrimCueTags`(Core/GrimSpellFXTypes) 큐를 **실행만** 한다. 처리기(MagicFX)가 없어도 게임은 돈다. 파라미터는 `UGrimSpellAbility::MakeSpellCueParameters`: `SourceObject` = 정의, `Location` · `Normal`, `AggregatedSourceTags` 에 `Element.*`, `Instigator` · `EffectCauser` = 시전 폰. ASC 는 시전자의 것(PlayerState — 늘 관련성 있음).

| 큐 | 어디서 | Location · Normal |
|---|---|---|
| `GameplayCue.Spell.Cast` (Execute) | 시전자: `ActivateAbility`(예측) / 서버: TargetData 콜백 · 호스트는 `BeginAuthorityCast`. **누르는 순간** 한 번 — 실제 발사는 `CastWindup` 뒤라 섬광은 처리기가 그만큼 늦춰도 된다 | 시작점(완드 끝 또는 가슴) · 발사 방향 |
| `GameplayCue.Spell.Channel` (Add/Remove) | Stream(`bChanneled`)만. Cast 와 같은 곳에서 `K2_AddGameplayCueWithParams(.., bRemoveOnAbilityEnd)` → 어빌리티가 끝나면 거둔다 | 시작 때의 시작점 · 방향. 줄기 모양은 처리기가 `UGrimSpellAbility::TraceStreamArc` 로 매 프레임 같은 식을 그린다 |
| `GameplayCue.Spell.Impact` (Execute) | 서버 확정 뒤(예측 키 없음 → 시전자 포함 모두): Bolt 투사체 명중, Stream 자극 때, Cone 대상마다, Beam 맞은 곳마다 | 맞은 곳 · 표면 법선(Cone 은 −발사 방향) |

- Bolt 의 **꼬리**는 큐가 아니라 투사체(진짜 · 가짜 모두)가 `VisualSet.Trail` 을 직접 붙인다(움직이는 액터라서). Niagara 면 사용자 변수 `Color`, Cascade 면 색 파라미터 `Color` 에 `ElementColor`. VisualSet 이 없으면 속성색 구만.
- 시전자 클라이언트에서는 진짜 투사체 복제본을 숨기고(가짜가 대신 보인다), 진짜가 사라지면 같은 마법의 가장 오래된 가짜도 지운다. 가짜는 판정 없이 지형 · 사물에서 멈춰 사라지고, 터짐은 서버 Impact 큐가 그린다.

### 디버그 · 2인 확인

- 콘솔 `Grim.Magic.Cast <마법 이름 일부> [대상 클래스|None] [누르는 초] [지연초]` — 로컬 플레이어가 원점에 가장 가까운 그 클래스 액터를 고정 조준해 시전(0.5초 뒤 누름). `Grim.Magic.Place <대상 클래스> <거리cm> [지연초]` — 서버가 원격 클라이언트 폰을 그 대상 −X 쪽에 두고 대상을 보게.
- `Scripts/Integration/net_check_delivery.ps1` — 헤드리스 리슨 서버 + 클라이언트에서 원격 클라이언트가 불씨 → 먼 미풍 → 가까운 미풍 → 불씨 → 물줄기(1.5초)를 양초에 쏘고 양쪽 `Grim.Interaction.Dump` 를 비교한다. 결과 `Saved/NetCheck/delivery_*.log`.

## 마법서 `UGrimSpellbookComponent`

`AGrimPlayerState` 의 하위 객체. `BeginPlay` 에서 `OnAbilityActorInfoReady` 를 구독한다(이미 초기화돼 있으면 바로 처리).

- **처음 한 번(서버)**: `Circle = StartingCircle`, `MaxMana = Mana = MaxManaByCircle[서클]`, 마나 회복 `UGrimGE_ManaRegen`(무한 · 1초 주기 `Mana += ManaRegenPerSecond`), 선택 어빌리티 `UGrimSpellSelectAbility` 를 `Input.Spell.Next` · `Input.Spell.Prev` 태그로 두 번, `StartingSpells` 를 `LearnSpell`.
- **폰이 바뀔 때마다(서버)**: 완드가 없으면 스폰하고 `Owner = 폰`, 폰 스켈레탈 메시의 `WandSocketName`(본 · 소켓 모두)에 `SnapToTargetNotIncludingScale` 로 부착(완드 루트 = 소켓, 상대 변환 없음). 메시 · 소켓이 없으면 폰 루트. 부착은 완드의 움직임 복제로 클라이언트에 간다.
  - `KeepRelativeTransform` 을 쓰면 안 된다 — 스폰한 월드 좌표가 그대로 손 기준 오프셋이 되어 완드가 몸에서 떨어진다(M1 통합 때 실제로 난 버그).
- `LearnSpell(Spell)` — 서클 확인 → `GiveAbility(SourceObject = Spell)` → `KnownSpells` 에 추가(복제). 첫 마법이면 선택.
- `CycleSelection(±1)` · `SelectSpell(i)` — 선택 Spec 에만 `Input.Spell.Cast` 를 넣고 바뀐 Spec 을 `MarkAbilitySpecDirty`. `SelectedIndex` 복제, `OnSelectedSpellChanged`(서버 · 클라이언트) 로 HUD 갱신.
- `UGrimSpellSelectAbility` 는 ServerOnly — 입력은 클라이언트, 선택 변경은 서버, 결과는 복제.

## 숙련도 (Q26)

> 스펙: [마법 — 숙련도](../spec/magic#숙련도-q26-확정). 구현 예정 **M2**(스킬북 · 서클 1→3 과 함께).

**밸런스는 표 두 개와 설정 하나로만 바꾼다.** 코드에는 등급 수 · 수치를 적지 않는다.

### 데이터

| 무엇 | 형식 | 내용 |
|---|---|---|
| `DT_ProficiencyGrades` | DataTable `FGrimProficiencyGradeRow` | 행 = 등급 하나. `Grade`(D · C · B · A · S), `DisplayName`(입문 … 마스터), `XPToReach`(누적), `ManaCostMultiplier`, `CooldownMultiplier`, `ExtraEffectScale`(계약 확률 같은 마법별 추가 효과 배율). 행 순서가 등급 순서 — 등급을 늘리거나 줄여도 코드는 그대로 |
| `UGrimProficiencyProfile` | DataAsset | 마법 묶음별 경험 곡선 바꾸기: `GradeTable`(위 표 참조, 기본은 공용), `XPScale`(필요 경험 배율 — 높은 서클 마법은 크게), `XPPerCast` · `XPPerMeaningfulHit` |
| `UGrimSpellDefinition` 추가 필드 | | `ProficiencyProfile`(비우면 서클별 기본 프로필), `ProficiencyExtraEffect`(선택: 예 `Taming.CaptureBonus` 와 그 최대값) |
| `UGrimProficiencySettings` | DeveloperSettings (`DefaultGame.ini`) | `GlobalXPRate`(서버 옵션으로도 노출), `RepeatTargetDecay`(같은 대상 반복 감쇠, 예 0.5 → 두 번째부터 절반씩), `RepeatWindowSeconds`, `DefaultProfileByCircle[1..10]` |

예시 `DT_ProficiencyGrades` (CSV 로도 관리 — `Content/Grimoire/Data/ProficiencyGrades.csv`):

```
Grade,DisplayName,XPToReach,ManaCostMultiplier,CooldownMultiplier,ExtraEffectScale
D,입문,0,1.00,1.00,0.0
C,익숙,100,0.95,0.95,0.25
B,숙달,300,0.90,0.90,0.5
A,능숙,700,0.85,0.85,0.75
S,마스터,1500,0.75,0.80,1.0
```

### 계산 · 적용

- 저장: `UGrimSpellbookComponent` 에 `TArray<FGrimSpellProficiency { SpellId, XP }>`(FastArray, 소유자에게만 복제, 플레이어 세이브). 등급은 저장하지 않고 XP 와 표로 **매번 계산**한다 → 표를 바꾸면 기존 세이브도 새 기준을 따른다.
- 순수 계산 `FGrimProficiencyMath`(액터 모름, 테스트가 직접 부름): `GetGrade(XP, Profile)` · `GetMultipliers(Grade)` · `ComputeXPGain(Def, bMeaningfulHit, RepeatCount, Settings)`.
- 경험 지급: 서버에서 시전 확정 때 `XPPerCast`, 명중 처리(피해 · 자극 · 계약 결과) 때 `XPPerMeaningfulHit`. 같은 (마법, 대상) 쌍의 최근 기록으로 `RepeatTargetDecay` 를 곱한다. 마지막에 `× XPScale × GlobalXPRate`.
- 마나: `UGrimSpellAbility` 의 비용 GE 를 `SetByCaller.Magic.ManaCost = 기본 마나 × ManaCostMultiplier` 로(완드 마나 효율 · 인챈트와 곱셈).
- 쿨타임: 쿨다운 GE 지속시간 `SetByCaller.Magic.Cooldown = 기본 쿨타임 × CooldownMultiplier / CastSpeed`.
- 계약 확률 같은 추가 효과: `ProficiencyExtraEffect` 최대값 × `ExtraEffectScale`.
- 등급이 오르면 소유 클라이언트에 `OnProficiencyGradeUp(SpellId, Grade)` → UI 알림 · 그리모어 쪽 갱신.

### 테스트

`Grimoire.Magic.Proficiency.Grade`(경계값) · `.Multipliers` · `.RepeatDecay` · `.TableEdit`(행을 바꾸면 결과가 따라 바뀌는지).

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
- 필사는 없다(Q5 미채택). 스킬북은 탑 · 던전 · 유적 · 전리품으로만 얻는다.

## 서클

- `UGrimMagicAttributeSet`: `Circle`, `MaxMana`(서클 함수), `CastSpeed`, `SpellPower`, 속성별 `Power_Fire` …(완드 · 버프가 더함).
- 승급: `UGrimProgressionComponent`(PlayerState)가 조건(시험 퀘스트 완료 · 소재)을 확인하고 `Circle + 1` → `Circle.N` 태그 갱신 → 그리모어 UI 갱신.
- 사냥꾼의 "금지된 승급"은 같은 컴포넌트의 다른 경로(`AbsorbMana`) — NPC 전용이지만 같은 데이터 구조.

## 속성 태그 · 상성 표

- 속성 태그 `Element.Water / Fire / Wind / Earth / Light / Dark` 는 `Core/GrimGameplayTags`(네이티브). Magic 전용 태그(`SetByCaller.Magic.*` · `Cooldown.Spell.*` · `Spell.*`)는 `Magic/GrimMagicTags`.
- 피해 계산 `UGrimDamageExecution`(`UGrimGE_SpellDamage` 의 실행): `Health −= SetByCaller.Magic.Damage × 상성(공격 속성, 대상 속성)`. 공격 속성 = Spec 에셋 태그의 `Element.*`, 대상 속성 = 대상 ASC 태그의 `Element.*`(없으면 무속성 ×1).
- 상성 표는 `UGrimMagicSettings::ElementAffinities`(ini). 표만 바꾸면 밸런스가 바뀐다(Q2 · Q3).
- **10속성 확정(Q2 · Q3, 코드 반영 전)**: 스펙이 무속성 + 6계열 10속성과 상성표(극성 ×1.5)를 정했다([마법 — 속성과 상성](../spec/magic#속성과-상성-r07)). 태그를 계열 아래로 나눈다 — `Element.None` · `Element.Water.Liquid` · `Element.Water.Ice` · `Element.Fire` · `Element.Wind` · `Element.Earth.Stone` · `Element.Earth.Plant` · `Element.Light.Holy` · `Element.Light.Radiance`(빛 — 번개류) · `Element.Dark.Hex`(흑마법) · `Element.Dark.Shadow`(그림자술). 태그 이름은 코드에 넣을 때 정한다. 극성은 갈래 짝 `Holy ↔ Hex` · `Radiance ↔ Shadow` 만 표에 넣고, 계열 태그(`Element.Light` ↔ `Element.Dark`) 행은 없앤다. `Element.None` 은 표에 행이 없어 늘 `DefaultAffinity`(×1.0). 반응 규칙표는 하위 태그도 맞으므로 `Element.Water` 규칙은 물 · 얼음 둘 다에 걸린다. 번개 + 젖음 = 감전은 상성이 아니라 반응 규칙 한 줄이다.
- 지금 코드 값(6속성, 10속성 확정 전):

| 공격 → 대상 | 배율 |
|---|---|
| 물 → 불 · 불 → 바람 · 바람 → 땅 · 땅 → 물 | ×1.5 |
| 불 → 물 · 바람 → 불 · 땅 → 바람 · 물 → 땅 | ×0.75 |
| 빛 → 어둠 · 어둠 → 빛 | ×1.5 |
| 그 외(같은 속성 포함) | ×1.0 (`DefaultAffinity`) |

- 테스트: `Grimoire.Magic.Affinity` · `Grimoire.Magic.Cost` · `Grimoire.Magic.Effects`(GE 실제 적용) · `Grimoire.Magic.StartingSpells`(에셋 값) · `Grimoire.Magic.Delivery.*`(시차 보정 · 유도 · 포물선 · 원뿔 · 거리별 세기 · 지연 보정 · TargetData 검증 · 시작 마법 전달 방식) — `Source/Grimoire/Magic/Tests/`.

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

## 소프트 락온 (R17)

> 스펙: [조준 · 소프트 락온](../spec/magic#조준-소프트-락온-r17). 레퍼런스: 호그와트 레거시. **M1.5** 갈래 B 에서 구현 — 투사체 재설계와 한 묶음([투사체 · 시전 감각](./spell-feel)). 코드 `Source/Grimoire/Targeting/`.

### 구성

| 부품 | 위치 | 역할 |
|---|---|---|
| `UGrimTargetingComponent` | `AGrimPlayerController` 의 기본 하위 객체(`Targeting`). 로컬 컨트롤러에서만 고른다(틱에서 `IsLocalController` 확인, 전용 서버는 틱 끔) | `IGrimAimProvider` 구현. **0.05초 간격**(`SelectionInterval`)으로 후보를 모아 점수를 매기고 현재 대상을 정한다. 고정 락온 · 카메라 추적 · 옆 대상 넘기기 · 대상 표식 |
| `FGrimTargetingMath` | `Targeting/GrimTargetingMath` | 순수 계산 — 원뿔 각도 · 점수 · 분류 · 정렬/상한 · 화면상 넘김 대상, 가림 타이머(`FGrimOcclusionTimer`), 튕김 감지(`FGrimLookFlickDetector`) |
| `IGrimTargetable` | 적 · 생물 · 상호작용 사물 | 조준점(`GetTargetPoint`), 종류(`EGrimTargetKind`), 대상이 될 수 있는지(`CanBeTargetedBy` — 죽음 · 숨음이면 false). 사물은 `AGrimElementalObject`(종류 Object, 조준점 = 몸체 바운드 가운데에서 반높이 × `TargetPointHeightRatio` 0.6 위) |
| `AGrimTargetDummy` | `Targeting/GrimTargetDummy` | 생물 · 적이 생기기 전 시험용 대상(구체, 기본 종류 Enemy, 왕복 이동 옵션) |
| `UGrimTargetingSettings` | `DefaultGame.ini` `[/Script/Grimoire.GrimTargetingSettings]` | 아래 「수치」 |
| 대상 표식 | `UGrimTargetingComponent::DrawTargetMarker` | **임시**: 대상 조준점에 카메라를 보는 속성색 링(디버그 선, 출시 빌드엔 안 그려짐). 고정이면 바깥 링 하나 더. 정식 위젯은 HUD 작업 때 — `OnTargetChanged`(대상 · 모드) 를 듣는다 |

### 선정 (로컬)

1. 후보 수집: 캐릭터 위치에서 **사거리 구 오버랩**(오브젝트 종류 `CandidateObjectChannels` = WorldDynamic · Pawn · PhysicsBody — 전용 충돌 채널은 만들지 않았다) → 액터 또는 그 컴포넌트의 `IGrimTargetable` 만. 자기 폰과 폰에 붙은 것(완드)은 뺀다. 사거리 = 선택된 마법의 `Range`(없으면 `DefaultSelectionRange` 2500).
2. 거르기: `CanBeTargetedBy`, **아군(`Ally`) · 종류 없음 제외**, 원뿔(반각 25°, 지금 대상은 +5° 여유), 사거리, 캐릭터 뒤(카메라와 캐릭터 사이) 제외.
3. 점수: `Center × (1 − 각도 / 반각) + Distance × (1 − 거리 / 사거리) + Kind × 종류 가중 + Keep × (지금 대상이면 1)`, 각 항은 0~1 로 자른다. 종류 가중은 적 1.0 · 반응 사물 0.5 · 생물 0.4 · 그 밖의 사물 0.1. **반응 사물** = 선택된 마법의 자극(`Stimulus`, 속성이 비면 `Element`)으로 반응 규칙표에 맞는 행이 있는 사물(`FGrimElementalReactionLibrary::FindStimulusRule` 조회만 — 휴면을 깨우지 않는다).
4. 정렬 · 상한: `bEnemiesBeforeObjects`(기본 켬)면 **보이는 적이 있으면 점수와 상관없이 적이 먼저**(스펙 「전투 중에는 적이 사물보다 먼저」), 그 안에서 점수 순. 후보 상한 32.
5. 시야: 점수 상위 **8개만** 원뿔 꼭짓점 → 조준점 라인 트레이스(`VisibilityChannel` = Visibility). 처음 보이는 것이 대상. 지금 대상은 **0.3초까지** 가려져도 유지하고, 넘으면 풀린다.
6. 대상은 **로컬에서만** 고른다(입력 반응성). 시전할 때 어빌리티가 `TargetData`(대상 액터 + 조준점)를 서버로 보낸다 — `FGameplayAbilityTargetData_SingleTargetHit` 하나에 시선 시작 · 조준점 · 대상 · 모드를 싣고, 서버는 시선 · 사거리 · 대상 가능 여부를 다시 검사해 어긋나면 서버 조준으로 바꾼다(치트 방지 최소한, [시전 흐름](#시전-흐름) 3). 서버는 원격 시전자에 대해 조준 제공자를 부르지 않는다.

**선정 기준 옵션** (호그와트 레거시의 Camera Relative Targeting): `Camera`(기본) = 카메라 위치 · 정면이 원뿔. `Movement` = 캐릭터 눈높이에서 **이동 입력 방향**(입력이 없으면 캐릭터 정면)이 축, 요 각도만 본다. `SetTargetingReference` · 콘솔 `Grim.Targeting.Reference Camera|Movement`.

**소프트 락온 끄기 (Q18)**: `SetSoftLockEnabled(false)` · 콘솔 `Grim.Targeting.SoftLock 0`. 꺼지면 자동 대상이 없고 마법은 화면 중앙으로. 고정 락온은 여전히 쓸 수 있다(토글하면 그 순간 한 번 골라 고정). 기본값은 `bSoftLockEnabledByDefault` — 옵션 화면이 생기면 거기서 덮어쓴다.

### 조준 결과 (`GetCurrentAim(MaxRange)`)

| 상태 | `Mode` | `Target` · `AimPoint` |
|---|---|---|
| 고정 락온 | `Locked` | 고정 대상 · 조준점. **사거리와 상관없이** 돌려준다(마법 쪽이 사거리에서 자른다) |
| 소프트 대상이 있고 캐릭터에서 `MaxRange` 안 | `Soft` | 대상 · 조준점 |
| 그 밖 | `Free` | 없음 · 카메라 정면 트레이스가 닿은 곳(없으면 사거리 끝). 트레이스는 캐릭터 깊이에서 시작해 사거리만큼(카메라와 캐릭터 사이 것을 맞히지 않게) |
| 줌인 조준 | `Zoom` | **아직 없음** — R24 조사 중. 입력 `IA_AimZoom` 과 `SetZoomAimHeld` 자리만 있다(TODO) |

`ViewOrigin` 은 언제나 카메라 위치.

### 시전 연결 (갈래 A)

- `UGrimSpellAbility` 가 활성화될 때 `IGrimAimProvider::GetCurrentAim` 결과를 TargetData 로 담는다. 제공자가 없거나 Free 면 카메라 정면. Locked 대상이 마법 사거리 밖이면 대상을 버리고 카메라 정면으로.
- 캐릭터 회전: 시전 순간 대상 방향으로 몸을 돌린다(서버 확정, 0.15초 보간) — 캐릭터 쪽 몫(M1.5 갈래 C), 어빌리티는 하지 않는다.
- 투사체 유도: `AGrimSpellProjectile` 이 틱마다 속도 방향을 대상 조준점 쪽으로 초당 `HomingStrength`° 까지 돌린다(`GrimSpellDelivery::SteerTowards`, 속력 그대로, `HomingMaxAngle` 넘게 빗나가면 안 휨 — 유턴 없음). 0 = 유도 없음. 스펙의 "살짝 휨"(Q16 확정). 엔진 `HomingAccelerationMagnitude` 대신 각속도로 한 건 속력이 바뀌지 않고 순수 함수로 테스트하려고.
- 미풍은 Cone 이 되어 대상 쪽 원뿔로 판정한다. 광선(Beam)은 후속 빛 속성용으로 남겼다.

### 고정 락온 (Q17)

- 입력 `IA_LockOn`(PC **Caps Lock** · 패드 **R3** = `Gamepad_RightThumbstick`, 토글 — 호그와트 레거시와 같음) → 컨트롤러 → `ToggleLock`. 지금 소프트 대상을 고정한다. 대상이 없으면 그 순간 한 번 골라(소프트 락온이 꺼져 있어도) 고정, 그래도 없으면 아무것도 안 함. 다시 누르면 풀리고 대상은 소프트 대상으로 남는다.
- 고정 중에는 선정을 건너뛰고 대상만 검사한다: 죽음 · 사라짐 · `CanBeTargetedBy` false, 캐릭터에서 `max(LockedMaxRange 3000, 마법 사거리)` 밖, `LockedOcclusionGraceSeconds`(1초) 넘게 가림 → **자동 해제**.
- 카메라: 매 프레임 스프링암 피벗(`AGrimPlayerCharacter::GetCameraBoom()` 위치)에서 대상을 보는 회전으로 **컨트롤 회전을 보간**(`RInterpTo`, 속도 `LockCameraInterpSpeed` 8, 피치 +`LockCameraPitchOffset` −5°, −60°~40° 제한). 캐릭터 파일은 고치지 않는다 — 스프링암이 컨트롤 회전을 따른다.
- 옆 대상 넘기기: 컨트롤러가 `AddYawInput` · `AddPitchInput` 을 가로채 고정 중이면 카메라를 돌리지 않고 `HandleLookInput` 으로 보낸다(마우스 · 오른쪽 스틱이 같은 길). 입력을 모아 `LockSwitchInputThreshold`(6)를 넘으면 그 방향(화면 좌표)으로 `LockSwitchMaxAngleDegrees`(60°) 안의 화면상 후보(카메라 평면 투영 `ProjectToViewPlane` — 뷰포트 없이 계산해 헤드리스에서도 된다, 시야각 밖은 뺌) 중 `거리 × (1 + 각/허용각)` 이 가장 작은 것으로 넘긴다(가려졌으면 다음 것). 입력이 0.2초 없으면 모은 값을 버리고(천천히 흘린 입력은 넘김이 아님), 넘긴 뒤 0.35초 쿨다운.

### 수치 (`UGrimTargetingSettings`)

| 항목 | 기본 | 뜻 |
|---|---|---|
| `SelectionInterval` | 0.05초 | 선정 간격 |
| `ConeHalfAngleDegrees` · `CurrentTargetExtraHalfAngleDegrees` | 25° · 5° | 원뿔 반각, 지금 대상 여유 |
| `DefaultSelectionRange` | 2500cm | 마법이 없을 때 거리 |
| `MaxCandidates` · `MaxVisibilityTraces` | 32 · 8 | 후보 상한, 시야 트레이스 상한 |
| `OcclusionGraceSeconds` | 0.3초 | 소프트 대상 가림 허용 |
| `Weights` | Center 1 · Distance 0.5 · Kind 1 · Keep 0.3 · 적 1 · 생물 0.4 · 반응 사물 0.5 · 사물 0.1 | 점수 |
| `bEnemiesBeforeObjects` | 켬 | 보이는 적 우선 |
| `LockedMaxRange` · `LockedOcclusionGraceSeconds` | 3000cm · 1초 | 고정 해제 조건 |
| `LockCamera*` | 속도 8 · 피치 −5° · −60°~40° | 고정 카메라 |
| `LockSwitch*` | 문턱 6 · 쉼 0.2초 · 쿨다운 0.35초 · 60° | 옆 대상 넘기기 |
| `Marker*` · `ElementColors` | 반지름 20cm + 거리 × 0.012, 고정 × 1.4. 물 `#3A7BD5` · 불 `#E0582B` · 바람 `#7CC9A6` | 임시 표식 |

### 콘솔 (개발 빌드)

`Grim.Targeting.SoftLock <0|1>` · `Grim.Targeting.Reference <Camera|Movement>` · `Grim.Targeting.Lock`(토글) · `Grim.Targeting.Look <요> <피치>`(시점 입력 한 번 — 넘기기 확인) · `Grim.Targeting.Dump`(상태 + `GetCurrentAim` 결과) · `Grim.Targeting.SpawnDummy [앞cm] [오른쪽cm] [종류]` · 변수 `Grim.Targeting.Debug 1`(후보 · 점수 그리기). 끝에 지연(초)을 붙이면 헤드리스 `-ExecCmds` 에서도 쓸 수 있다.

### 멀티 · 성능

- 선정은 각 클라이언트가 자기 화면 기준으로 한다. 서버는 시전 때 검증만 한다. 조준 컴포넌트는 복제하지 않는다.
- 후보 상한 32, 시야 트레이스는 점수 상위 8개만, 선정은 0.05초마다(카메라 추적 · 표식만 매 프레임).
- 사물은 `IGrimTargetable` 을 구현하되 휴면 상태를 깨우지 않는다(바운드 · 상태 조회만).

### 아직 없는 것

- 줌인 조준(R24, `Mode = Zoom`) — 스펙 조사 뒤 이어 붙인다.
- 「나를 노리는 적 가산」 — 적 AI 가 생기면 종류 가중에 더한다.
- 정식 대상 표식 위젯 · 옵션 화면(Q18 · 선정 기준 저장).

### 테스트

- 순수 계산은 자동화 테스트 `Grimoire.Targeting.*` — `Cone` · `Score` · `Priority.Category` · `Priority.SortAndCap` · `Occlusion` · `Lock.Switch` · `Lock.Flick`.

## UI

- HUD: 단축 슬롯(마법 아이콘 · 쿨다운 · 서클 색), 마나, 조준점(대상의 속성 상태 아이콘 표시 — "젖음" 보이기).
- 그리모어(`WBP_Grimoire`): 배운 마법 · 필터 · 슬롯 배치 · 완드 정보 · 발견한 반응 규칙 도감.

## M1 범위

불씨 · 물줄기 · 미풍 3개 마법, 완드 1종, 양초 · 모닥불 · 마른 풀 3개 사물, 반응 규칙표 10행, 상성 표. 서클 UI 는 숫자만.

아직 없는 것: 그리모어 UI, 스킬북 아이템, 완드 정의 데이터 · 장착 해제, 서클 승급. 시전 동작 · 섬광 · 터짐 연출은 M1.5 갈래 C(MagicFX)가 큐 처리기로 붙인다.
