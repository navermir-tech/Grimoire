# 레벨 · 속성 성장 트리

> 스펙: [성장](../spec/progression) (R27, Q30 · Q31). 서클 승급 · 숙련도 · 스킬북은 [마법](./magic#서클). M2 후반 갈래 L([작업 분할](./m2b-plan)).

레벨과 서클은 **무관하다**(Q30). 레벨은 최대 HP · 최대 마나 · 성장 트리 포인트만, 서클은 마법탑 시험만 정한다. 둘 다 `Source/Grimoire/Progression/` 에 있고, `AGrimPlayerState` 의 하위 객체다.

| 클래스 | 하는 일 |
|---|---|
| `UGrimLevelComponent` | 누적 경험 · 레벨 · 받은 포인트, 레벨 보너스 GE, 경험 출처(스킬북 · 승급 · 지역 · 규칙 발견), 세이브 |
| `UGrimSkillTreeComponent` | 찍은 노드 · 남은 포인트, 효과 합 GE · 노드 태그, 찍기 RPC, 정화 의식(`ResetTree`), 세이브 |
| `UGrimGE_ProgressionBonus` | 무한 GE — 어트리뷰트마다 SetByCaller 수정자 하나(아래) |
| `FGrimLevelMath` · `FGrimSkillTreeMath` | 순수 계산(곡선 · 포인트 · 선행 · 효과 합 · 예산) — 테스트 대상 |
| `UGrimLevelSettings` · `UGrimSkillTreeSettings` | ini 설정 · 표 읽기(에셋 → CSV, 한 번 읽어 둔다) |
| `UGrimLevelWidget` · `UGrimSkillTreeWidget` | 레벨 · 경험 막대, 그리모어 「성장 트리」 탭 |

## 레벨

### 레벨 곡선 표 `Content/Grimoire/Data/LevelCurve.csv`

행 이름 = 레벨(1 부터 빈틈없이). 값은 그 레벨에 **올라설 때** 받는 것.

| 열 | 뜻 | 지금 값 |
|---|---|---|
| `TotalXP` | 이 레벨이 되는 누적 경험(레벨 1 = 0, 늘기만) | 다음 레벨까지 = 100 × 지금 레벨^1.5(10 단위 반올림). 레벨 30 = 189,040 |
| `MaxHealth` · `MaxMana` | 더해지는 최대 HP · 마나 | 레벨마다 +5 · +4, 5 의 배수 레벨은 +10 · +8 → 30 까지 HP +175 · 마나 +140 |
| `SkillPoints` | 받는 성장 트리 포인트 | 레벨마다 1, 10 · 20 · 30 은 2 → 30 까지 **32** |

- 상한은 `LevelCap`(30 = 오픈 베타, 정식 50 은 표에 줄을 더하고 올린다). 상한 문턱에서 경험이 멈춘다 — 상한을 올리면 이어서 쌓인다.
- 정본은 누적 경험(`TotalXP`, 소유자에게만 복제). 레벨은 곡선으로 계산하고 모두에게 복제(`OnRep_Level` → `OnLevelChanged`).
- **최대 HP · 마나**: 레벨 2 ~ 지금 레벨의 합을 `UGrimGE_ProgressionBonus` 하나로 `MaxHealth` · `MaxMana` 에 더한다. 레벨이 바뀌면 새 GE 를 먼저 걸고 옛 것을 지운다(최대값이 잠깐이라도 줄어 현재 HP · 마나가 깎이지 않게). 어트리뷰트 셋 파일은 고치지 않는다. 서클 승급은 최대 마나를 건드리지 않는다.

### 경험 출처

공개 함수 `UGrimLevelComponent::AwardExperience(float Amount, FGameplayTag Source)` (서버, 실제로 준 양을 돌려준다) — 다른 갈래(튜토리얼 퀘스트 보상 · 사냥 · 제작)가 부른다. 출처 태그는 `Progression.XPSource.*`(`GrimProgressionTags`).

| 출처 | 어디서 | 기본 경험(ini) | 반복 감쇠 |
|---|---|---|---|
| `Skillbook` | 스킬북으로 **새** 마법을 배움(`UGrimSpellbookComponent::OnLearnResult` = `Learned` 구독) | 150 | 없음 |
| `CircleUp` | 서클 승급 성공(`UGrimProgressionComponent::OnCircleUpResult` 구독) — 서클마다 한 번 | 400 | (첫 발견) |
| `RegionDiscovery` | 새 기후 지역 첫 방문 — 서버가 `RegionCheckInterval`(2초)마다 폰 위치의 `UGrimWeatherSubsystem::GetRegionAt` | 250 | (첫 발견) |
| `RuleDiscovery` | 상호작용 규칙 첫 발견(도감) — `NotifyRuleDiscovered(RuleId)` | 60 | (첫 발견) |
| `Quest` | 퀘스트 보상(갈래 T 가 부른다) | 100 | 없음 |
| `Hunt` · `Craft` | 사냥 · 제작(M3 · M4 에서 잇는다) | 30 · 5 | 0.8 / 120초 · 0.7 / 60초 |
| `Debug` | 콘솔 | 100 | 없음 |

- **같은 출처 반복 감쇠**: `RepeatWindowSeconds` 안에 같은 출처가 또 오면 `RepeatDecay^반복 횟수` 를 곱한다(`MinMultiplier` 아래로는 안 감). 규칙이 없는 출처는 `Default*`(0.7 · 60초 · 0.1). 전체에 `GlobalXPRate` 를 곱한다.
- **처음 한 번**: `AwardDiscovery(Source, Key)` 는 같은 (출처, 키)에 한 번만 출처의 `DefaultXP` 를 준다. 기록은 세이브에 남는다. 처음 서 있던 지역은 `bAwardStartingRegion=False` 면 조용히 「발견」만 한다.
- TODO: 속성 상태 컴포넌트가 「규칙 발동 + 일으킨 플레이어」 이벤트를 주지 않아 규칙 발견은 아직 아무도 부르지 않는다 — 생기면 `NotifyRuleDiscovered` 로 잇는다.
- 포인트 보너스: `AddBonusSkillPoints`(퀘스트 보상 등, 세이브됨).

## 속성 성장 트리

### 트리 표 `Content/Grimoire/Data/SkillTreeNodes.csv`

행 이름 = 노드 id(`<갈래>.<종류>`, 예 `Fire.Power`).

| 열 | 뜻 |
|---|---|
| `Tree` · `Branch` | 트리 id(`Normal` · `Elemental` · `Light` · `Dark`) · 세부 갈래 id. 표시 이름 · 색은 ini 의 `Trees` · `Branches` |
| `Kind` | `Power` · `Efficiency` · `Status` · `Interaction` · `Survival` · `Trait`(큰 노드) — UI 모양 · 문서용 |
| `GridX` · `GridY` | UI 격자 칸(0.5 단위 가능). Y = 0 이 안쪽(뿌리) |
| `Prerequisites` | 선행 노드 id 를 `\|` 로(모두 찍어야 열린다, 같은 트리 안만). 없는 노드 · 다른 트리 · 순환은 읽을 때 경고 |
| `Cost` · `RequiredCircle` | 포인트 비용 · 서클 조건 |
| `Effects` | `키=값` 을 `;` 로. 값이 `*` 로 시작하면 곱하기, 아니면 더하기(`MaxMana=+10;ManaCostScale=*0.97`) |

### 효과가 닿는 곳

찍은 노드의 효과를 키마다 합해(더하기는 합, 곱하기는 곱) **GE 하나**(`UGrimGE_ProgressionBonus`, 무한)로 건다. 바뀔 때마다 새 GE 를 걸고 옛 것을 지운다.

| 키 | GE 수정자 | 지금 쓰는 노드 |
|---|---|---|
| `MaxHealth` · `MaxMana` | 더하기(AddBase) | 단련된 몸 · 단단한 몸 · 축복받은 몸 / 마나 그릇 · 깊은 그릇 · 어둠의 그릇 |
| `HeatResist` · `ColdResist` · `DiseaseResist` | 더하기 | 물의 몸 · 서늘한 피 / 불씨 품기 · 햇볕 품기 / 마력 피막 · 약초 지식 |
| `MoveSpeedScale` · `CastSpeedScale` · `ManaCostScale` | 곱하기(MultiplyCompound, 생존 패널티와 같은 방식) — 값 = Mul × (1 + Add) | 가벼운 걸음 · 바람 걸음 · 그림자 걸음 / 완드 다루기 · 빠른 손 / 마나 절약 |
| 그 밖(`Fire.Power` · `Fire.IgniteStrength` · `Water.Trait.ForkedJet` …) | 없음 — 태그 `Progression.Node.<키>` 를 `DynamicGrantedTags` 로 붙이고, 합계는 `UGrimSkillTreeComponent::GetEffectValue(키)` | 속성별 위력 · 마나 효율 · 상태 · 상호작용 세기 · 특성 |

- 마법 쪽에 속성별 위력 · 마나 효율 어트리뷰트가 없어서 그 노드들은 아직 **태그 · 합계만** 있다. TODO(각 시스템): 피해 계산이 `<속성>.Power`, 비용 계산이 `<속성>.ManaCost`, 상호작용 자극이 `Fire.IgniteStrength` 같은 세기, 특성 노드는 태그를 읽는다.
- 새 효과 키를 표에 더하면 `GrimProgressionTags.cpp` 에도 태그를 더한다 — 효과를 쓰는 코드가 어차피 필요하므로 태그도 코드에 둔다. 빠지면 테스트 `Grimoire.Progression.TreeTable` 이 잡는다.

### 초기 트리 — 4 트리 · 14 갈래 · 84 노드

갈래마다 노드 6 개, 같은 모양:

| 칸 | 종류 | 비용 | 서클 | 선행 |
|---|---|---|---|---|
| `Power`(위) | 위력 | 1 | 1 | — |
| `Efficiency` · `Status` | 효율 · 상태 | 1 · 1 | 1 | Power |
| `Interaction` · `Survival` | 상호작용 · 생존 | 1 · 1 | 2 | Efficiency · Status |
| `Trait`(아래) | 특성(큰 노드) | **2** | **4** | Interaction **와** Survival |

| 트리 | 세부 갈래 | 노드 · 비용 |
|---|---|---|
| 노말 | 기본 공격 · 순수 마력 · 마나 · 완드 다루기 | 24 · 28 |
| 4원소 | 물 · 얼음 · 불 · 바람 · 땅 · 풀 | 36 · 42 |
| 빛 | 신성 · 빛 | 12 · 14 |
| 어둠 | 흑마법 · 그림자술 | 12 · 14 |
| **합** | 14 갈래 | **84 · 98** |

노말 트리의 갈래는 원소가 아니라 칸의 내용이 다르다(예: 마나 갈래 = 마나 그릇 +10 · 마나 절약 ×0.97 · 마나 회복 +10% · 깊은 그릇 +15 · 숙련도 경험 +10% · 특성 「피의 대가」). 원소 갈래는 위력 +5% · 마나 소모 −5% · 갈래별 상태 · 상호작용 · 생존 · 특성.

### 포인트는 모자라게 — 계산 근거

레벨 상한 30 의 포인트 32 로:

| 잣대 | 계산 | 비율 |
|---|---|---|
| 비용 | 32 / 98 | **32.7%** |
| 노드 수(가장 싼 것부터 찍은 상한) | 32 / 84 | **38.1%** |
| 갈래 완성 | 갈래 하나 = 7 포인트 → 4.6 갈래 / 14 | 33% |

- 비용 1 노드가 70 개(갈래마다 5)라 노드 수 비율도 40% 를 넘지 않는다. 특성을 노리면 갈래 4 개 남짓을 끝까지 가거나, 여러 갈래의 위쪽만 넓게 찍는다 — 「불 · 바람 마법사」, 「신성 치유사」처럼 길이 갈린다.
- 정식 상한 50 으로 올리면 표에 줄을 더하면서 포인트와 노드를 같이 늘려 같은 비율을 지킨다. 테스트 `Grimoire.Progression.TreeBudget` 이 ini 상한 · 실제 표로 30~40% 를 검사한다(비용 · 노드 수 둘 다).

### 찍기 · 다시 찍기

- 남은 포인트 = `UGrimLevelComponent::GetTotalSkillPoints()`(표 합 + 보너스) − 찍은 노드 비용 합. 포인트는 따로 저장하지 않는다.
- `UGrimSkillTreeComponent::TryUnlock(NodeId)`(서버) — 순서대로 판정: 없는 노드 · 이미 찍음 · 선행 · 서클 · 포인트. 클라이언트는 `Server_TryUnlock` / `RequestUnlock`. 결과는 `OnUnlockResult`(서버 · 소유 클라이언트), 바뀐 트리는 `OnTreeChanged`(복제 콜백 포함).
- **정화 의식**(Q31): `ResetTree(FName TreeId, const FGrimTreeResetCostCheck& CostCheck)`(서버) — 트리 하나의 노드를 모두 지우고 포인트를 돌려준다. `CostCheck` 가 묶여 있으면 (트리, 돌려받을 포인트)로 먼저 묻고 `false` 면 그만둔다. 재화가 아직 없어 비워 두면 공짜 — TODO: 인벤토리 · 재화가 생기면 정화 의식(갈래 T 의 탑 NPC)이 채운다. 비울 노드가 없으면 `false`.
- 레벨을 내려(콘솔) 포인트가 모자라도 찍은 노드는 그대로 두고 남은 포인트만 0 이 된다.

## UI

- **그리모어 「성장 트리」 탭** `UGrimSkillTreeWidget`: 위에 트리 탭 4 개(ini `Trees` 순서) · 「Lv · 남은 포인트 · 이 트리에 쓴 포인트」, 가운데 노드 격자(칸 `GridCellWidth` × `GridCellHeight` = 60 × 70, 노드 56 × 44, 큰 노드는 1.15 배)와 선행을 잇는 선(찍은 쪽에서 나오면 갈래색), 갈래 이름. 노드 색: 찍음 = 갈래색 + 금테, 찍을 수 있음 = 어두운 갈래색 + 갈래색 테, 포인트 모자람 = 흐린 테, 잠김(선행 · 서클) = 회색. 고른 노드는 흰 테 — 아래에 이름 · 갈래 · 종류 · 비용 · 서클 · 선행 · 설명 · 상태와 **찍기** 버튼(찍을 수 있을 때만).
- **레벨 · 경험 막대** `UGrimLevelWidget`: 「Lv 5」 · 경험 막대 · 「경험 1,230 / 2,820」 · 남은 포인트, 레벨이 오르면 「레벨 업! — Lv N」(UI 설정 `ToastSeconds`). HUD 에는 통합 때 끼운다 — 그전에는 콘솔 `Grim.UI.Level [0|1]` 로 화면 아래 가운데에 단독으로 띄운다.

## 세이브

둘 다 `IGrimSaveParticipant`(Scope = Player, 버전 1):

| Id | 쓰는 것 |
|---|---|
| `Progression.Level` | 누적 경험(float) · 보너스 포인트(int32) · 발견 기록(「출처 태그 \| 키」 문자열 배열) |
| `Progression.SkillTree` | 찍은 노드 id(문자열 배열) |

읽을 때: 경험은 상한에서 자르고 레벨 · GE 를 다시 맞춘다. 표에서 빠진 노드는 버린다(포인트는 저절로 돌아온다). 레벨과 트리 중 무엇이 먼저 읽힐지 몰라 선행 · 포인트는 다시 따지지 않는다. 발견 기록은 지금 세션의 것과 합친다.

## 설정

`DefaultGame.ini`:

- `[/Script/Grimoire.GrimLevelSettings]` — `LevelCap`(30) · `GlobalXPRate` · `Sources`(출처마다 `DefaultXP` · `RepeatDecay` · `RepeatWindowSeconds` · `MinMultiplier`) · `Default*` · `RegionCheckInterval` · `bAwardStartingRegion` · `LevelTableCSV`(또는 `LevelTableAsset`).
- `[/Script/Grimoire.GrimSkillTreeSettings]` — `NodeTableCSV`(또는 `NodeTableAsset`) · `Trees` · `Branches`(이름 · 색) · `GridCellWidth` · `GridCellHeight` · `NodeSize`.

## 콘솔 (개발 빌드, 끝의 [지연초]는 헤드리스용)

| 명령 | 하는 일 |
|---|---|
| `Grim.Level.AddXP <양> [출처] [지연초]` | 경험을 준다(출처 = `Progression.XPSource.` 뒤 이름, 없으면 Debug) |
| `Grim.Level.Set <레벨> [지연초]` | 레벨을 바로 정한다 |
| `Grim.Level.Dump [지연초]` | 레벨 · 경험 · 포인트 · 트리별 찍은 노드 · 효과 합 · 성장 어트리뷰트 |
| `Grim.Tree.Unlock <노드 id> [지연초]` | 노드를 찍는다(예 `Fire.Power`) |
| `Grim.Tree.Reset <트리 id> [지연초]` | 정화 의식 — 비용 없이 트리 하나 비우기 |
| `Grim.Level.Reload` · `Grim.Tree.Reload` | 표(CSV)를 다시 읽는다 |
| `Grim.UI.Level [0\|1]` | 레벨 · 경험 막대 단독 표시 |

## 테스트 `Grimoire.Progression.*`

| 테스트 | 보는 것 |
|---|---|
| `LevelCurve` | 작은 곡선: 경계 · 상한 · 경험 멈춤 · 포인트 · HP · 마나 합 · 막대, 빈틈 · 줄어드는 표는 문제 |
| `RepeatDecay` | 감쇠 · 최소 배율 · 창 · 출처별 기록, ini 규칙(상한 30, 퀘스트 감쇠 없음, 사냥 감쇠) |
| `TreeMath` | 효과 읽기 · 선행 · 서클 · 포인트 · 비용 합 · 트리 비우기 · 효과 합 → GE 값 · 태그, 없는 선행 · 다른 트리 · 순환 |
| `TreeTable` | 실제 표: 트리 4 개 · 4원소 6 갈래, 표시 설정, 효과 키가 어트리뷰트이거나 등록된 태그, 큰 노드는 서클 조건 |
| `TreeBudget` | **상한까지 올려도 30~40% 만** — 비용 · 노드 수 둘 다, 레벨마다 포인트 · HP · 마나가 는다 |
| `Component` | PlayerState 에서 경험 → 레벨 → 최대 HP · 마나 GE, 반복 감쇠, 첫 발견, 찍기 → 추위저항 · 태그, 정화 의식(비용 거절 · 통과), 세이브 왕복, 상한 |
