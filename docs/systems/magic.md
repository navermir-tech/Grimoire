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

```cpp
UCLASS()
class UGrimSpellDefinition : public UPrimaryDataAsset
{
	FText Name, Description;
	UTexture2D* Icon;
	FGameplayTag Element;                    // Element.Fire …
	int32 Circle;                            // 1~10
	TSubclassOf<UGrimSpellAbility> AbilityClass;
	float ManaCost, Cooldown, CastTime;
	EGrimSpellDelivery Delivery;             // Projectile · Beam · Area · Self · Touch
	TSubclassOf<AActor> ProjectileClass;
	TSubclassOf<UGameplayEffect> ImpactEffect;
	FGrimElementalStimulus Stimulus;         // 사물에 주는 속성 자극(종류 · 세기)
	FPrimaryAssetId SkillbookItem;           // 이 마법을 가르치는 스킬북
};
```

- Asset Manager 타입 `Spell`. 콘텐츠를 넣는 쪽은 `DA_Spell_Fireball` 같은 에셋만 추가하면 마법이 생긴다.
- `AbilityClass` 는 대부분 공용(`GA_Spell_Projectile`, `GA_Spell_Beam` …)이고, 특수 마법만 전용 블루프린트.

## 시전 흐름

1. 입력(Enhanced Input `IA_Cast`) → `AGrimPlayerCharacter` 가 현재 슬롯의 마법 태그로 `ASC->TryActivateAbilitiesByTag`.
2. `UGrimSpellAbility::ActivateAbility`(예측 가능): 완드 확인(`AGrimWand` 장착 없으면 실패 — Q4), 비용 · 쿨다운 체크, 시전 몽타주 재생, `CastTime` 뒤 `Deliver()`.
3. `Deliver()` — 서버: 투사체 스폰 / 라인 트레이스 / 범위 쿼리 → 맞은 대상마다 **두 가지**를 한다:
   - ASC 가 있으면 `ImpactEffect` 적용(피해 · 상태). `UGrimDamageExecution` 이 상성 · 완드 · 서클 보정을 계산.
   - `UGrimElementalStateComponent` 가 있으면 `ApplyStimulus(Stimulus, Instigator)` 호출 → 반응 규칙.
4. 클라이언트: 투사체는 예측 스폰하지 않고 서버 스폰 복제 + 로컬 머즐 이펙트만 즉시(체감 지연 줄이기).

## 완드 `AGrimWand`

- 장비 액터. 손 소켓에 부착, `MuzzleSocket` 에서 투사체가 나간다.
- 데이터 `UGrimWandDefinition`(아이템 정의의 하위): 속성 친화(`TMap<FGameplayTag, float>` 위력 보정), 시전 속도, 마나 효율, 등급, 내구도(▢).
- 장착 시 ASC 에 `GE_WandBonus`(무한 지속, 장착 해제 시 제거)로 보정을 어트리뷰트에 싣는다 → 피해 계산이 자연스럽게 반영.

## 스킬북 · 배우기

- 스킬북은 아이템(`UGrimItemDefinition`, `ItemType = Skillbook`, `TeachesSpell`).
- 사용 → `Server_UseItem` → `UGrimSpellbookComponent::LearnSpell(SpellId)`: 서클 확인 → `GiveAbility` → 목록에 추가(복제 · 저장) → 책 소모.
- 필사(Q5): `AGrimScribeStation` 에서 아는 마법 선택 + 잉크 → 스킬북 아이템 생성. 레시피는 `DT_Recipes` 의 특수 행.

## 서클

- `UGrimMagicAttributeSet`: `Circle`, `MaxMana`(서클 함수), `CastSpeed`, `SpellPower`, 속성별 `Power_Fire` …(완드 · 버프가 더함).
- 승급: `UGrimProgressionComponent`(PlayerState)가 조건(시험 퀘스트 완료 · 소재)을 확인하고 `Circle + 1` → `Circle.N` 태그 갱신 → 그리모어 UI 갱신.
- 사냥꾼의 "금지된 승급"은 같은 컴포넌트의 다른 경로(`AbsorbMana`) — NPC 전용이지만 같은 데이터 구조.

## 속성 태그 · 상성 표

- `Config/Tags/Element.ini` 에 `Element.Water / Fire / Wind / Earth / Light / Dark`.
- 상성은 `DT_ElementAffinity`(행: 공격 속성, 열: 대상 속성, 값: 배율). 기본값은 스펙의 제안(순환 ×1.5 / ×0.75, 빛 ↔ 어둠 ×1.5, 그 외 ×1.0). 표만 바꾸면 밸런스가 바뀐다(Q2 · Q3).

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
- 사물 액터는 `DORM_DormantAll` 로 시작하고, 상태가 바뀐 프레임에만 컴포넌트가 `FlushNetDormancy` 로 깨워 보낸다. 플레이어 · 생물(휴면 안 함)에 달아도 같은 코드다.
- 상태를 바꾸는 건 서버뿐(`HasAuthority` 아니면 무시).

### 날씨 · 생물과의 연결 (통합 이후)

- `UGrimWeatherSubsystem` 은 비가 올 때 **야외**(지붕 없음: 위로 트레이스) 사물에 주기적으로 `Water` 자극을 준다 → 불이 꺼진다. 번개는 낙뢰 지점에 `Wind + Light` 고강도 자극.
- 생물 · 플레이어도 같은 컴포넌트: `State.Wet` 이면 `UGrimDamageExecution` 이 번개 · 얼음 피해를 올린다. `State.Burning` 은 주기 피해 GE 를 ASC 에 적용한다(컴포넌트가 ASC 를 찾아 브리지 — 아직 없음).

## UI

- HUD: 단축 슬롯(마법 아이콘 · 쿨다운 · 서클 색), 마나, 조준점(대상의 속성 상태 아이콘 표시 — "젖음" 보이기).
- 그리모어(`WBP_Grimoire`): 배운 마법 · 필터 · 슬롯 배치 · 완드 정보 · 발견한 반응 규칙 도감.

## M1 범위

불씨 · 물줄기 · 미풍 3개 마법, 완드 1종, 양초 · 모닥불 · 마른 풀 3개 사물, 반응 규칙표 10행, 상성 표. 서클 UI 는 숫자만.
