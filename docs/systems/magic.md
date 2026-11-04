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
