# 마법 생물 · 생태계 시스템

> 스펙: [마법 생물](../spec/creatures) (R02 · R14 · R15)

## 데이터

### 종 `UGrimSpeciesDefinition` (PrimaryDataAsset, 타입 `Species`)

| 필드 | 내용 |
|---|---|
| 외형 | SkeletalMesh, AnimBP, 크기 등급(Small · Medium · Large · Legendary), 색상 변이 머티리얼 파라미터 목록 |
| 속성 | `Element` 태그. 약점 · 내성은 상성 표로 충분하면 생략 |
| 기본 능력치 | `BaseStats { Health, Mana, Power, Speed, Intellect }` + 레벨 성장 |
| 생태 | `Diet`(Herbivore · Carnivore · Manavore), `Habitat` 바이옴 태그, `ActivityTime`(Diurnal · Nocturnal), `Territorial`, 무리 크기 |
| 쓰임 | `bEdible`(드롭표), `bTameable` + `TamingCircle`, `bMountable` + 이동 모드, `WorkAptitudes { Work.Gather: 2, Work.Water: 3 … }` |
| 번식 | `BreedingGroup`, `GestationMinutes`, 알 / 새끼 |
| AI | StateTree 에셋, 공격 어빌리티 목록(GAS), 감지 설정 |

### 개체 `FGrimCreatureInstance` (복제 · 저장)

`SpeciesId, Level, FGrimGenome Genome, FString Name, FGuid OwnerId(길들인 주인), ETameState, Loyalty, Hunger, Fatigue`

### 유전 `FGrimGenome`

`TMap<EStat, uint8> IV`(0~31), `int32 ColorVariant`(0 = 기본), `bool bMutant`, `uint8 MutantTier`

- 실제 능력치 = `Base × (1 + IV / 64) × 레벨 계수`. 돌연변이는 IV 상한 31 → 40.

## 생물 액터 `AGrimCreature : ACharacter`

- 자체 `UGrimAbilitySystemComponent` + `UGrimVitalAttributeSet` + `UGrimElementalStateComponent`.
- 컴포넌트: `UGrimTamingComponent`, `UGrimMountComponent`, `UGrimWorkerComponent`, `UGrimBreedingComponent`(길들인 뒤 활성).
- 종 블루프린트 `BP_Creature_<종>` 은 데이터 연결 + 특수 행동만.

## 생태계

### 스폰 `UGrimEcosystemSubsystem` (서버)

- 지역(기후 볼륨)마다 **개체군 레코드** `{ SpeciesId, Target, Current, RegrowRate }`.
- 플레이어 근처(World Partition 로드 범위)에서만 실제 액터로 **펼치고**(instantiate), 멀어지면 레코드로 **접는다**(fold) — 위치 · 레벨 · 유전만 남김. 세계 어디서나 생태계가 "존재"하되 비용은 근처만.
- 스폰 조건: 바이옴 · 시간대 · 날씨(비 → 불 생물 숨김) · 지형(물가 · 동굴).
- 상한: 지역 `Target` 을 넘지 않음. 사냥하면 `Current` 감소, `RegrowRate` 로 회복(멸종 없음).

### 먹이 관계 · 영역 `DT_Ecology`

- 행: `Predator, Prey, Priority`. StateTree 의 `Hunt` 가 이 표로 표적을 고른다.
- 영역: 대형 종은 `TerritoryRadius`. 다른 대형 종이 들어오면 `TerritoryFight` 상태(서로 공격, 체력 30% 이하면 후퇴). 플레이어는 어그로 유도 마법(바람으로 소리 · 어둠으로 유인)으로 싸움을 붙일 수 있다.

### AI (StateTree)

- 공용 트리 `ST_Creature_Base`: `Idle → Wander / Graze → (허기) Hunt / Forage → (위협) Flee / Fight → (밤 · 낮) Sleep → (영역) TerritoryFight`. 종별 파라미터 · 서브트리.
- 감지: `AIPerception`(시각 · 청각) — 안개에서 반경 감소, 자신이 `State.Burning` 이면 공황(Flee).
- 공격은 GAS 어빌리티(속성 태그 포함) → 플레이어 · 사물에도 자극 적용(불 생물의 브레스가 풀을 태운다).

## 개체 차이 (R23)

- 개체 레코드에 `Sex`(Female · Male), `ColorRegions[6]`(팔레트 색 번호 + `FLinearColor` 흔들림), `SizeScale`(0.9~1.1, 거대 1.3~1.5), `bGiant`, `Personality`(Q27) 를 더한다. 세이브 · 복제는 이 레코드 그대로.
- 색: 종 정의 `UGrimCreatureSpecies` 에 영역별 `BasePalette` · `MutantPalette` · 성별 팔레트 덮어쓰기. 머티리얼은 마스크 텍스처(영역 = RGBA + 둘째 마스크) 하나 + 다이내믹 머티리얼 인스턴스 파라미터 `RegionColor0..5`. 생성 · 부화 때 서버가 굴리고 클라이언트는 레코드로 칠한다.
- 암수 형태: 스켈레탈 메시 하나 + 성별 부위 메시(뿔 · 갈기)를 `SexParts` 로 켜고 끔. 크기는 액터 스케일 + 캡슐 · 이동 속도 보정.

## 길들이기 `UGrimTamingComponent`

- 조건: 대상 체력 ≤ 30%(또는 잠듦 · 지침), 플레이어 서클 ≥ `TamingCircle`, 계약 마법 명중 — 기본 `GA_Spell_Bind`(무속성) 또는 속성별 `GA_Spell_Bind_<속성>`(그 속성 생물이면 `ElementBindBonus`).
- 성공률 = 기본 + 약해짐 + 먹이 + 속성 일치(마법 · 완드) + 장신구 + 서클 차 (+ 숙련도, Q26) − 등급. 수치는 `UGrimTamingSettings`. 실패 시 저항(공격).
- 성공: `OwnerId` 설정, 레코드를 플레이어 소유 목록으로 이동(생태계 개체군에서 제외), 이름 짓기 UI.
- 충성도 `Loyalty`(0~100, 길들인 직후 50): 서버가 사건(먹이 · 치료 · 승리 · 굶김 · 혹사 · 기절 · 환경)마다 `FGrimLoyaltyEvent` 로 더하고 빼며, 성격 배율(Q27)을 곱한다. 최근 사건 3개를 레코드에 남겨 UI 가 「까닭」으로 보여 준다. 단계(충직 · 보통 · 불만 · 반항)는 태그 `Creature.Loyalty.*` 로 걸어 작업 속도 · 명령 순종 확률 GE 를 바꾼다. 0 → 야생 레코드로 되돌림. 번식장 배정 조건 `Loyalty ≥ 50`.

## 탑승 `UGrimMountComponent`

- 탑승 시 플레이어 컨트롤러가 생물 폰을 **Possess**, 플레이어 캐릭터는 안장 소켓에 부착(비활성 · 충돌 끔). 하차 시 역순. 가장 단순하고 네트워크에 안전한 방식.
- 이동 모드: Walk · Swim · Glide · Fly 는 `CharacterMovement` 모드 + 종별 속도.
- 탑승 중 시전(Q9): 플레이어 ASC 는 PlayerState 에 있으므로 폰이 바뀌어도 시전 가능. 투사체 발사 위치만 완드(부착된 캐릭터) 기준.

## 노동 `UGrimWorkerComponent` + `AGrimBaseCore`

- 거점 핵이 **작업 보드**를 가진다: 작업 요청 `{ WorkTag, TargetActor, Priority }` 를 시설 · 밭 · 자원 노드가 등록(물 부족한 밭 → `Work.Water`).
- 배정된 생물의 StateTree `Work` 상태가 적성 `WorkAptitudes[WorkTag] > 0` 인 요청을 우선순위로 고른다 → 이동 → 수행(해당 속성 자극 적용: 물 생물이 밭에 `Water` 자극 — 플레이어 마법과 **같은 경로**) → 다음.
- 일꾼도 `Hunger · Fatigue`: 먹이통 · 잠자리 시설이 거점에 필요.

## 번식 `UGrimBreedingComponent` + `AGrimBreedingPen`

- 번식장에 같은 `BreedingGroup` 의 암수 한 쌍(충성도 50 이상) 배정 → `GestationMinutes` 타이머(서버. 세이브에 남은 시간 저장).
- 자식 유전: 스탯마다 부모 중 하나의 IV(50 / 50) 또는 평균 ± 2. 색은 영역마다 부모 중 하나(+ 흔들림 새로 굴림), 성별 반반, 크기는 부모 평균 ±0.05(거대 유전 확률 `GiantInheritChance`).
- 돌연변이 판정: `p = 3% + 부모 돌연변이 1마리당 +3%`(Q8) → `bMutant = true`, 영역 하나 이상을 `MutantPalette` 에서, IV 상한 40 으로 재굴림(기존 값 + 0~9).
- 결과: 알(`AGrimEgg`, 부화 시간) 또는 새끼(소형 모델 스케일 0.5 → 성장).

## 식용 · 드롭

- 사망 시 `DropTable`(고기 · 가죽 · 마나 결정). 돌연변이는 희귀 소재 ↑.
- 길들인 생물의 부산물(알 · 젖)은 시설에서 주기 생산.

## 세이브

- 길들인 개체 전부(인스턴스 구조체). 야생은 지역 개체군 레코드 + 접힌 개체(위치 · 유전)만.

## M1~M3 범위

M1 없음. M3: 종 2(초식 소형 · 육식 중형) + 생태계 스폰 + 길들이기 + 탑승 1종. 노동 · 번식은 M4~M5.
