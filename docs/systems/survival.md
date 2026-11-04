# 생존 시스템

> 스펙: [생존](../spec/survival) (R03), [세계 · 환경](../spec/world) (R04)

## 어트리뷰트

`UGrimVitalAttributeSet` — ASC 에 부착, 플레이어 · 생물 공용.

| 어트리뷰트 | 범위 | 비고 |
|---|---|---|
| `Health` / `MaxHealth` | 0~Max | 0 → 쓰러짐(`Status.Downed`) → 사망 |
| `Mana` / `MaxMana` / `ManaRegen` | | MaxMana 는 서클 함수 + 버프 |
| `Hunger` | 0~100 | 100 = 배부름. 시간당 감소 |
| `Fatigue` | 0~100 | 0 = 쌩쌩. 깨어 있으면 증가, 밤 ×1.5 |
| `BodyTemp` | 섭씨 | 안전 35~39(제안). 주변 기온 쪽으로 수렴 |
| `DiseaseResist` | 0~100 | 질병 GE 적용 확률에 영향 |

생물은 `Hunger`, `Fatigue` 만 쓴다(일꾼 관리용). 생물의 체온은 종의 적응 기후로 단순화(▢).

## 주기 효과 (서버 전용)

- `GE_Survival_Tick`(무한 지속, 주기 1초)이 `Hunger`, `Fatigue` 를 깎는다. 배율은 `SetByCaller`(활동 · 시간대 · 날씨).
- 임계값 → 태그: `Hunger < 20 → Status.Hungry`, `Hunger == 0 → Status.Starving`(HP 감소, ManaRegen 0), `Fatigue > 80 → Status.Exhausted`(CastSpeed −30%), `Fatigue == 100 → Status.Collapsed`(강제 수면 10초).
- 태그가 붙으면 그에 묶인 GE(`GE_Status_Starving` 등)가 적용 · 해제된다. 이 매핑은 `UGrimSurvivalComponent`(PlayerState)가 어트리뷰트 변경 델리게이트로 처리.

## 체온 모델

```
TargetTemp = AmbientTemp(지역 기후 곡선[시간] + 날씨 보정)
           + Σ 열원(반경 내 State.Burning 세기 / 거리)     // 모닥불 · 불붙은 것 · 불 마법 잔향
           + 의복 단열(ClothingInsulation) × (BodyTemp 방향)
           − 젖음(State.Wet 세기 × 4℃)
           − 바람(풍속 × 0.3℃, 실내면 0)
BodyTemp += (TargetTemp − BodyTemp) × Rate × dt           // Rate 는 느리게(분 단위)
```

- `AmbientTemp` 는 `UGrimWeatherSubsystem::GetAmbientTemperature(Location)`.
- 실내 판정: 위로 트레이스해 지붕(건축 조각)이 있으면 비 · 바람 영향 0.
- 범위 이탈 → `Status.Hypothermia` / `Status.Hyperthermia` 태그 → 해당 GE(이동 · 시전 패널티, 지속되면 HP 감소).
- 불 마법으로 몸 데우기: 불씨를 자기에게 쓰면 `State.Warmed`(열원 취급, 2분) — R16 의 "마법으로 해결".

## 질병

- `GE_Disease_*`(지속형, 태그 `Status.Sick.Cough` …). 원인별 적용 확률: 날것 섭취, `State.Wet` + `Hypothermia` 지속, 특정 생물 공격, 오염 지역 볼륨.
- 치료: 약초 아이템(GE 제거), 빛 마법 `GA_Spell_Purify`(태그 `Status.Sick.*` 제거), 시간 경과.

## 먹기 · 요리 · 잠

- 음식 아이템 정의에 `HungerRestore`, `bRaw`, `DiseaseChance`, `Buffs`.
- 요리는 사물 상태로 한다: 날고기를 땅이나 모닥불 위에 두고 불 자극 → `State.Cooked` → 요리 아이템으로 바뀐다(R16).
- 잠: 침대는 직접 상호작용을 허용하는 예외(▢ 확인 필요). `Fatigue` 빠르게 감소. 밤이면 시간 가속 투표(멀티: 전원 동의 시 — 제안).

## 죽음 · 부활

- `Health == 0` → `Status.Downed`(60초, 동료가 빛 마법으로 회복 가능) → 사망.
- 사망: 가방 일부 드롭(`AGrimLootBag`), `AGrimGameMode::RespawnPlayer`(거점 핵 또는 마법탑). PlayerState 유지(서클 · 마법).

## M1 범위

HP · 마나 · 배고픔 · 체온만. 질병 · 피로는 M2.
