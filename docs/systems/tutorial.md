# 마법탑 튜토리얼 · 퀘스트

> 기획: [세력 · 서사 — 마법탑](../spec/factions#마법탑-r08)(튜토리얼 흐름 R31). 작업 분할: [M2 후반](./m2b-plan) 갈래 T.
> 코드: `Source/Grimoire/Quest/`(퀘스트) · `Source/Grimoire/Factions/`(마법탑 NPC · 시험 표적 · 전투 금지 볼륨 · 상점). 표: `Content/Grimoire/Data/`. 맵: `Content/Grimoire/Maps/L_Tower`(튜토리얼 맵 — 게임 기본 맵은 `Content/Grimoire/Maps/L_MainMenu`, 메뉴에서 시작하는 게임 맵은 `[/Script/Grimoire.GrimOnlineSettings] GameMap` = L_OpenBeta — 그 안의 탑에서도 같은 튜토리얼이 돈다). 튜토리얼은 설정 `AutoStartMaps`(L_Tower · L_OpenBeta)에서 저절로 시작한다.
>
> 상태: 구현됨. 거점(`Base.Founded`) 단계만 판정할 시스템이 없다(M4).

## 한눈에

| 조각 | 클래스 · 파일 | 하는 일 |
|---|---|---|
| 표 | `Content/Grimoire/Data/Quests.csv` · `QuestSteps.csv` · `QuestObjectives.csv` · `TowerNpcs.csv` · `TowerDialogue.csv`(설정 `[/Script/Grimoire.GrimQuestSettings]`) | 퀘스트 · 단계 · 목표 · NPC · 대사 전부. 런타임에 읽는다(숙련도 등급표와 같은 방식), 콘솔 `Grim.Quest.Reload` |
| 순수 계산 | `FGrimQuestDatabase`(표 읽기 · 검사) · `FGrimQuestModel`(시작 · 신호 · 조회 · 단계 넘김 · 시간 초과) — `Source/Grimoire/Quest/GrimQuestTypes.h`, `FGrimDialogueDatabase`(대사 고르기) — `Source/Grimoire/Factions/GrimFactionTypes.h` | 액터를 모른다 — 자동화 테스트 `Grimoire.Quest.*` |
| 진행 | `UGrimQuestComponent`(`Source/Grimoire/Quest/GrimQuestComponent.h`) | 플레이어 한 명의 진행(`FGrimQuestLog`). **PlayerState 에 런타임으로 붙는다**(Core 를 고치지 않으려고, 이름 고정 `GrimQuest`). 소유 클라이언트에만 복제. 세이브 `IGrimSaveParticipant`(Player, `Quest.Log`) |
| 세계 | `UGrimQuestSubsystem`(`Source/Grimoire/Quest/GrimQuestSubsystem.h`, 월드 서브시스템, 서버) | PlayerState 가 생기면 컴포넌트를 붙인다(세이브가 읽기 전에), 0.25초마다 조회형 목표, 밤 건너뛰기 → 신호, 단계 시작의 시각 · 소품 · 줄 것 |
| 화면 | `UGrimQuestTrackerWidget`(오른쪽 위 추적 창 · 알림) · `UGrimDialogueWidget`(대화 창 · 서비스 메뉴) | C++ 위젯 트리(WBP 없음), 소유 클라이언트 |
| NPC | `AGrimTowerNPC`(탑의 스승 「마렌」) · `AGrimQuestBoard`(퀘스트 게시판) — 둘 다 `Source/Grimoire/Factions/GrimTowerNPC.h`. 마렌의 외형은 [커스터마이징](./customization) 규칙 `Tower_Instructor` | 다가가 **F**(설정 `TalkKey`) — 침대와 같은 방식(`WasInputKeyJustPressed`). 정식 상호작용 키는 조작 갈래 몫 |
| 소품 | `AGrimTrialTarget`(과녁 · 시험 표적, 더미 + 명중 세기 · `HitsToBreak` 번 맞으면 부서짐) · `AGrimCropPlot`(작물 자리, 농사 M4 전) · `AGrimQuestTriggerVolume`(들어오면 퀘스트 시작) — `Source/Grimoire/Quest/GrimTutorialProps.h` · `AGrimNoCombatVolume`(탑 실내 = `Zone.NoCombat`). 냄비는 아이템 쪽 `AGrimCookingPot` | 맵에 놓는다 |

## 퀘스트 표

- **Quests.csv**: 행 이름 = 퀘스트 ID. `Kind`(Main · Trial · Side), `AutoStart`(설정 `AutoStartMaps` 의 맵에서만), `TrialCircle`(시험이면 통과 때 오르는 서클), `RequireCircleConditions`(시험 시작에 승급 조건 — 배운 마법 수 · 등급 합 — 도 볼까), `RequiresQuest` · `RequiresStep`(그 퀘스트를 끝냈거나 그 단계 이상 진행 중), `RewardXP`(`UGrimLevelComponent::AwardExperience`, 출처 `XPSource.Quest`), `RewardSpell`(스킬북 하나를 쓴 것처럼), `NextQuest`.
- **QuestSteps.csv**: `Quest` · `Step`(0 부터 빈틈 없이) · `Text`(추적 창) · `TimeLimitSeconds`(넘기면 퀘스트 실패 — 다시 청할 수 있다) · `TimeOfDayOnStart`(`Dusk+0.2` = 해넘이 + 0.2시간, `Dawn` · `Noon` · `Midnight` · 숫자 시) · `PropsTag`(이 태그의 소품을 단계 시작에 처음 상태로 되돌리고 켠다, 끝나면 끈다) · `Give`(아래).
- **QuestObjectives.csv**: 한 단계의 목표를 **모두** 채우면 다음 단계. 한 번 채운 목표는 그 단계 동안 남는다(켰던 양초가 꺼져도 「켰다」).

| 목표 종류 | 판정 | Target · State · Value · Count |
|---|---|---|
| `ElementalState` | 조회 — 태그 Target 사물의 상태 State 세기가 Compare(AtLeast · AtMost) Value 인 것이 Count 개 | 액터 태그 · `State.Burning` … |
| `KnowSpell` | 조회 — 마법을 배웠다 | 마법 ID(에셋 `DA_Spell_<ID>`) |
| `ProficiencyGrade` | 조회 — 숙련도 등급 번호 ≥ Value(D=0 …) | 마법 ID |
| `ReachCircle` | 조회 — 서클 ≥ Value | — |
| `ReachLocation` | 조회 — 태그 Target 표지 액터에서 Value cm 안 | 표지 태그 |
| `Attribute` | 조회 — 어트리뷰트가 Compare Value | 이름(`Fatigue` · `Hunger` …) |
| `CompleteQuest` | 조회 — 그 퀘스트를 끝냈다 | 퀘스트 ID |
| `TalkToNpc` | 신호 — 대화를 끝까지 읽었다(서버가 거리 검사) | NPC ID |
| `SleepThroughNight` | 신호 — 잠든 채 밤 건너뛰기를 맞았다(페이드 시작 때 잠들어 있던 사람) | — |
| `HitTarget` · `BreakTarget` | 신호 — 시험 표적이 마법에 맞았다 · 부서졌다 | 표적 태그 |
| `Event` | 신호 — 다른 시스템이 `UGrimQuestComponent::NotifyQuestEvent(플레이어, 이벤트 ID)` | 이벤트 ID |

- 새 단계는 **다음 주기부터** 조회한다 — 단계 시작의 세계 쪽 일(소품 되돌리기)이 끝난 뒤에 보려고(되돌리기 전의 켜진 양초로 시험이 바로 끝나지 않게).
- **Give**(단계 시작에 주는 것, `|` 로 여럿): `Skillbook:<마법 ID>`(**가방에 스킬북 아이템 `Skillbook_<마법 ID>` 를 넣는다** — 아이템 표에 그 행이 없을 때만 예전처럼 월드에 책을 놓는다, 태그 `Given_Skillbook_<ID>`), `Bedroll`(침낭, 태그 `Given_Bedroll`), `Item:<ID>`(플레이어 가방에 하나 — `UGrimItemStatics::GiveItem`, 표에 없는 아이템(거점 핵 M4)은 기록만). 뒤에 `@<태그>` 를 붙이면 그 액터 곁, 없으면 플레이어 앞 1.5m. 마법 정의(`DA_Spell_<ID>`, 설정 `SpellAssetDirectory`)가 없는 마법은 경고만 하고 넘어간다 — 지금은 튜토리얼 마법 모두(`BasicAttack` · `Ember` · `WaterJet` · `Breeze` · `Pactum`) 정의가 있다.

### 이벤트(다른 시스템이 부르는 신호)

| 이벤트 ID | 부르는 곳 | 상태 |
|---|---|---|
| `UI.Inventory.Opened` | 가방 화면을 열었을 때 — `UGrimInventoryComponent` | 있음 |
| `Creature.Killed.Small` | 생물을 잡은 플레이어 — `AGrimCreature`(`Creature.Killed.<크기 분류>`, 접두는 생물 설정 `KilledEventPrefix`) | 있음(M3 갈래 C) |
| `Item.Obtained.Meat` | 고기 묶음(`Meat`) 아이템을 가방에 넣었을 때 — 줍기 · 지급 모두(`Item.Obtained.<ID>` 도 같이) | 있음(갈래 I) |
| `Creature.Weakened.Small` | 마지막으로 때린 플레이어, 생물 체력 30% 이하 — `AGrimCreature`(`WeakenedEventPrefix`) | 있음(M3 갈래 C) |
| `Creature.Tamed.Small` | 파크툼(길들이기) 성공 — `UGrimTamingSubsystem`(`TamedEventPrefix`) | 있음(M3 갈래 C) |
| `Cooking.IngredientsAdded` · `Cooking.Completed` · `Food.Eaten` | 아궁이 위 냄비(`Tut_CookingPot`, `AGrimCookingPot`)에 재료를 넣음 · 요리 완성(넣은 사람들) · 음식을 먹음 | 있음(갈래 I) |
| `Base.Founded` | 거점 핵으로 첫 거점을 세움 | 미구현(M4) |

없는 신호는 콘솔 `Grim.Quest.At <초> Event <ID>` 로 채운다.

## 튜토리얼 흐름 (Tutorial)

시작할 때 아는 마법이 없다 — 모두 스킬북(`Give`)으로 받는다. 완드는 처음부터 **기본 완드**(`Wand_Basic` — 실제 장비, Q71)를 쥐고 있다(시작 장비 `UGrimBaseSettings::StartingEquipment`, [건축 · 장비](./building#내구도-·-수리-·-인챈트-r18-·-r19)) — 그래서 1 · 2단계 기본 공격 흐름이 그대로다. 대사는 `TowerDialogue.csv`(퀘스트 · 단계마다 한 행).

| 단계 | 할 일 | 목표 | 판정 |
|---|---|---|---|
| 0 | 스승에게 말 걸기(완드) | TalkToNpc Master | 진짜 |
| 1 | 가방 열기 · 기본 공격 스킬북 사용 | Event `UI.Inventory.Opened`(가방 화면을 열면 소유 클라이언트가 서버로 알림) + KnowSpell BasicAttack (Give → 가방) | 진짜 |
| 2 | 수련실 과녁을 기본 공격으로 부수기 | BreakTarget Tut_TrainingTarget(과녁 `HitsToBreak` = 3) | 진짜 |
| 3 | 스승에게 | TalkToNpc | 진짜 |
| 4 | 플라미스 · 아쿠엘 · 벤투스 스킬북 읽기 | KnowSpell × 3 (Give) | 진짜 |
| 5~7 | 양초 켜기 → 벤투스로 키우기(세기 2) → 아쿠엘로 끄기(꺼짐 + 젖음) | ElementalState Tut_Candle | 진짜 |
| 8 | 홀 화단 작물 셋에 물 | ElementalState Tut_Crop State.Wet × 3 | 진짜(작물 자리 — 작물 M4) |
| 9 | 앞마당으로 | ReachLocation Tut_Yard | 진짜 |
| 10 · 11 | 소형 생물 사냥 · 고기 줍기 | Event | 진짜 — 사냥은 갈래 C, 줍기(`Item.Obtained.Meat`)는 갈래 I. 앞마당에 생물을 놓는 것은 맵 스크립트에 없다(표지 `Tut_CreatureSpot` 만) |
| 12 | 파크툼 스킬북 읽기 | KnowSpell Pactum (Give) | 진짜 |
| 13 · 14 | 체력 30% 이하 · 파크툼으로 길들이기 | Event | 진짜(갈래 C) |
| 15 · 16 | 주방으로 · 플라미스로 아궁이 | ReachLocation · ElementalState Tut_Stove | 진짜 |
| 17 · 18 | 재료 받기(Give `Item:Herb · Salt · Grain`) · 냄비에 넣고 완성 · 스튜 먹기 | Event | 진짜(갈래 I) |
| 19 | 스승에게(체온 · 피로 소개) | TalkToNpc | 진짜 |
| 20 | (해넘이로) 앞마당 모닥불 피우기 — 곁에 침낭(Give `Bedroll@Tut_CampSite`) | ElementalState Tut_Campfire | 진짜 |
| 21 | 침낭에서 밤 넘기기 · 피로 풀기 | SleepThroughNight + Attribute Fatigue ≤ 10 | 진짜 |
| 22 | 아침 — 스승에게(거점 핵, 이제 승급 시험을 청할 수 있다) | TalkToNpc | 진짜 |
| 23 · 24 | 거점 세울 곳으로(Item BaseCore) · 첫 거점 | ReachLocation Tut_BaseSite · Event Base.Founded | 진짜 · M4 |

- **승급 시험**(튜토리얼 밖, 스승의 서비스 「서클 승급 시험」 — 첫 밤 뒤): `Trial_Circle2` = 시험의 방으로 → 40초 안에 양초 셋(소품 `Trial2_Prop` 을 처음 상태로), 보상 이그니스 스킬북. `Trial_Circle3` = 60초 안에 움직이는 표적 세 번 + 젖은 양초(젖음 2) 말려 켜기. 통과 = `UGrimProgressionComponent::TryCircleUp(강제)`. 지금 표는 `RequireCircleConditions = False`(시험이 승급 조건을 대신) — 결정 필요.
- **사냥꾼 첫 등장**(`Hunter_FirstEncounter`): 튜토리얼을 끝낸 뒤 사냥꾼 거점의 발동 볼륨에 들어가면 시작 → 거점 살펴보기 → 스승에게 알리기. 이야기는 다음 묶음에서.

## 마법탑 NPC · 서비스

- 말을 걸면 진행에 맞는 대사(행 고르기: 그 단계 > 진행 중 > 끝냄 > 기본, 같은 조건은 `Priority`) → 끝까지 읽으면 서버에 대화 신호 → 서비스 메뉴.
- 대화 창이 열리면 입력은 UI 전용(이동 · 시전 멈춤) — 키 F · 스페이스 · Enter 다음, 1~9 고르기, Esc · Tab 닫기, 마우스로도 누른다.
- 서비스(`TowerNpcs.csv` 의 `Services`, 서버가 처리 → 대사 맥락으로 답):

| 서비스 | 하는 일 | 답 맥락 |
|---|---|---|
| `Trial` | 지금 서클 + 1 의 시험을 시작 | `Trial.Started` · `Trial.Busy` · `Trial.Locked` · `Trial.NotReady` · `Trial.None` |
| `Purify:<트리>` | 정화 의식 — `UGrimSkillTreeComponent::ResetTree(트리)`, 트리마다 메뉴 하나(Q31). 비용은 아직 무료(TODO 재화) | `Purify` · `Purify.Empty` |
| `Repair` · `Erase` | 대리 수리 · 지우기 — 내구도 · 인챈트(M4) 전이라 「아직 할 수 없다」 대사만 | `Repair` · `Erase` |
| `Buy:<아이템>` | 상점 — 가방의 솔리두스(`Solidus`)로 아이템 하나(`GrimTowerShop::TryBuy`, 값 `[/Script/Grimoire.GrimShopSettings] Prices`). 마렌: `Buy:Skillbook_CreatureStorage`(쿠스토디아) 120 솔(R38 — [생물 보관함](./creatures#데리고-다니기-·-보관함-·-거점-상자-r38)) · `Buy:Wand_Basic`(기본 완드) 10 솔 — 늘 판다(Q71 — [완드](./magic#완드-agrimwand)). 상점이 있는 NPC 는 표 검사가 아래 세 맥락을 요구 | `Buy` · `Buy.NoMoney` · `Buy.BagFull` |

## 맵 L_Tower

`Scripts/Tutorial/build_tower_map.py`(헤드리스 파이썬, 다시 돌려도 같은 결과). 배치(좌표 · 방 · 태그)는 `Scripts/Tutorial/tower_greybox.py` 에 있고, L_OpenBeta(`Scripts/Terrain/build_openbeta.py`)도 같은 함수로 탑을 짓는다. +X = 동쪽.

- **탑 1층**(32 × 24m, 벽 5m · 지붕): 시작 방(북, 탁자 위 양초 `Tut_Candle` · PlayerStart), 숙소(북서, 침대 둘), 주방(북동, 아궁이 `Tut_Stove` · 냄비 `Tut_CookingPot`), 홀(스승 `Npc_Master` · 게시판 `QuestBoard` · 화단 작물 `Tut_Crop` × 3), 서고(서), 수련실 · 시험의 방(남 — 과녁 `Tut_TrainingTarget`, 시험 양초 `Trial2_Candle` × 3, 움직이는 표적 `Trial3_Target`, 젖은 양초 `Trial3_WetCandle`). 위에 첨탑(원통 · 원뿔).
- **전투 금지**: 북쪽 방들 · 홀 · 서고에 `AGrimNoCombatVolume` 두 개 — 안에 있으면 폰 ASC 에 `Zone.NoCombat`(복제 루스 태그). 수련실과 탑 밖은 뺐다. 실내에서도 양초 · 아궁이 · 작물에는 마법을 써야 하니 **피해만** 막는다 — 구현: `Source/Grimoire/Magic/GrimSpellAbility.h` 의 `UGrimSpellAbility::IsCombatBlocked`(시전자나 대상이 이 태그를 가지면 피해 없음). 시전 자체는 막지 않는다.
- **탑 밖**: 앞마당(`Tut_Yard`, 생물 자리 `Tut_CreatureSpot`, 모닥불 `Tut_Campfire`, 침낭 자리 `Tut_CampSite`), 초원(양초 둘 · 마른 풀 · 지붕 쉼터 · 나무), 거점 세울 곳(북동 돌판 `Tut_BaseSite`), 사냥꾼 거점(먼 남동 `Hunter_Camp` — 천막 · 상자 · 발동 볼륨). 하늘 `GrimSkyController`, 기후 볼륨 Meadow.

## 콘솔 · 확인

- `Grim.Quest.Dump` · `Grim.Quest.Run <명령> …` · `Grim.Quest.At <초> <명령> …` — 명령: `Dump`, `Talk <NPC>`, `Service <NPC> <서비스>`, `Teleport <태그|NPC> [거리]`, `Stimulus <태그> <Element.X> [세기]`(마법 자극, 쏜 쪽 = 첫 플레이어), `Start <퀘스트>`, `Skip [퀘스트]`(지금 단계를 채움), `Fail <퀘스트>`, `Event <ID> [수]`. `Grim.Quest.Reload` — 표 다시 읽기.
- `Scripts/Tutorial/check_tutorial.ps1` — 헤드리스 단독 게임으로 튜토리얼 · 시험 둘 · 사냥꾼 퀘스트를 끝까지 밟고 PASS/FAIL. 기본 공격 · 파크툼 스킬북 단계는 `Skip`, 생물 · 요리 · 거점 단계는 `Event` 로 넘긴다(정의 · 시스템이 생기기 전에 짠 그대로 — 스크립트 머리 주석도 옛 사정).
- 자동화 테스트: `Grimoire.Quest.Table`(저장소 표 검사) · `TableProblems` · `TimeOfDay` · `Model` · `Timeout` · `Dialogue` · `Save` · `TutorialFlow`(저장소 표로 처음부터 끝까지).
