# 시스템 문서 — 아키텍처 개요

> 이 문서들은 **어떻게 만들지**를 정한다. 무엇을 만들지는 [스펙 문서](../spec/)가 정하고, 각 시스템 문서는 관련 R-번호를 머리에 단다.
>
> 상태: v0.2 (2026-11-06). M3 까지 합친 코드에 맞췄다. 표의 「구현」은 `Source/Grimoire/` 에 있는 것, 「계획」은 아직 코드가 없는 설계안이다.

## 전체 그림

```
[Client]  입력 → 예측 연출 → UI(그리모어 · HUD · 메뉴)
    │  RPC / 리플리케이션 (서버 권위)
[Server (리슨 또는 단독 — 전용은 계획)]
    ├─ GameMode / GameState        세계 규칙. 시간 · 날씨는 각자 복제 액터(Replicator)로
    ├─ PlayerState + ASC           서클 · 마법 · 생존 · 레벨 · 성장 트리 · 외형 (죽어도 유지)
    ├─ Character + Wand + Book     이동 · 시전 · 조준 · 그리모어 책 · 가방 · 길들인 생물
    ├─ Elemental Interaction       사물 · 생물의 속성 상태와 반응 규칙
    ├─ Environment Subsystems      TimeOfDay · Weather(기후 지역) · Sleep
    ├─ Creature Systems            종 · 개체 · 생태계 스폰 · AI · 길들이기 · 탑승 · 보관함 (번식 · 노동은 계획)
    ├─ Items                       아이템 표 · 가방 · 픽업 · 요리
    ├─ Base Systems (계획, M4)      거점 · 건축 · 농사 · 생산
    ├─ Faction / Quest             마법탑 NPC · 퀘스트 (사냥꾼은 계획)
    ├─ World                       World Partition 지형(L_OpenBeta) · 인스턴스 던전(계획)
    ├─ Online                      세션 · 접속 · 메뉴 (OSS Null)
    └─ Save Subsystem              세계 · 플레이어 저장
```

## 설계 원칙

1. **서버 권위, 처음부터.** 멀티(R13)가 스펙이므로 싱글도 "혼자 있는 리슨 서버"다. 모든 게임플레이 상태는 서버가 바꾸고 복제한다. 나중에 멀티를 붙이는 일은 없다.
2. **GAS(Gameplay Ability System) 위에 세운다.** 마법 · 생존 스탯 · 상태 · 버프 · 생물 능력까지 GAS 의 Ability / Effect / Attribute / Tag 로 통일한다. 예측 · 복제 · 쿨다운 · 비용을 공짜로 얻는다.
3. **데이터 주도.** 마법 · 종 · 건축 조각 · 레시피 · 반응 규칙은 전부 데이터다. 지금은 마법 · 종이 DataAsset, 나머지 표는 `Content/Grimoire/Data/*.csv` 를 런타임에 읽는다(설정 클래스가 경로를 든다). 새 콘텐츠는 코드 없이 데이터만 추가한다.
4. **속성 상태는 하나의 시스템.** 양초도 플레이어도 생물도 같은 `ElementalState` 컴포넌트와 같은 반응 규칙표를 쓴다(R16). 특수 케이스 코드를 만들지 않는다.
5. **C++ 가 뼈대, 블루프린트가 살.** 시스템 · 네트워크 · 성능이 걸린 코드는 C++. 콘텐츠(특정 마법의 연출, 특정 생물의 행동 세부)는 C++ 기반 클래스를 상속한 블루프린트.
6. **작게 시작.** 각 시스템의 "M1 범위"를 지키고 확장은 [로드맵](./roadmap)을 따른다.

## 모듈 · 폴더 구성

**단일 게임 모듈 `Grimoire`** (`Source/Grimoire/Grimoire.Build.cs`). 컴파일 시간이 문제가 되면 폴더 단위로 모듈을 나눈다. 순수 계산은 폴더마다 `*Math` 로 떼고, 자동화 테스트는 각 폴더의 `Tests/`(이름 `Grimoire.<시스템>.*`).

| 폴더 | 내용 |
|---|---|
| `Source/Grimoire/Core/` | 게임 모드 · 게임 상태 · 플레이어 상태(ASC 소유) · ASC · 공용 태그, 시스템 사이 약속(속성 자극 `IGrimElementalReceiver` · 세이브 `IGrimSaveParticipant` · 조준 `IGrimAimProvider` · `IGrimTargetable` · 연출 묶음 `UGrimSpellVisualSet`), 에디터 단축키 |
| `Source/Grimoire/Core/Cheats/` | 치트 매니저(`GrimSuperCheat` 등) · 지연 명령 `Grim.At` · 고정 카메라 `Grim.Cam` ([테스트 치트](../dev/cheats)) |
| `Source/Grimoire/Core/Perf/` | 성능 측정 콘솔 `Grim.Perf.*` ([성능 측정 방법](../dev/perf-measure)) |
| `Source/Grimoire/Character/` | 플레이어 캐릭터 · 컨트롤러 · 입력(Enhanced Input) |
| `Source/Grimoire/Appearance/` | 외형(커스터마이징) — 카탈로그 · 조립 · NPC 생성 · 생성 화면 ([커스터마이징](./customization)) |
| `Source/Grimoire/Magic/` | 마법 어트리뷰트 · 마법 정의 · 완드 · 시전 어빌리티 · 전달 방식 · 상성 피해 · 숙련도 · 서클 · 스킬북 · 기본 공격 · 슬롯 시전 · 파크툼 투사체 ([마법](./magic)) |
| `Source/Grimoire/MagicFX/` | 시전 연출 큐 · 그리모어 책 · 시전 자세 애님 · 카메라 흔들림 · 이펙트 에디터 도우미 ([투사체 · 시전 감각](./spell-feel)) |
| `Source/Grimoire/Targeting/` | 소프트 · 고정 락온, 줌인 조준, 조준 더미 |
| `Source/Grimoire/Interaction/` | 속성 상태 컴포넌트 · 반응 규칙 · 전파 · 상호작용 사물(양초 · 모닥불 · 마른 풀) |
| `Source/Grimoire/Survival/` | 생존 어트리뷰트 · 주기 효과 · 체온 · 피곤함 · 질병 · 침대 · 잠 ([생존](./survival)) |
| `Source/Grimoire/Environment/` | 시간 · 날씨 서브시스템, 기후 지역 볼륨, 하늘 컨트롤러 ([환경](./environment)) |
| `Source/Grimoire/Creatures/` | 생물 · 종 · AI · 생태계 스폰 · 서식 구역 · 길들이기 · 충성도 · 탑승 · 데리고 다니기 · 보관함 · 거점 생물 상자 ([생물](./creatures)). 번식 · 노동은 계획 |
| `Source/Grimoire/Items/` | 아이템 · 가방 · 픽업 · 먹기 · 요리 냄비 |
| `Source/Grimoire/Progression/` | 레벨 · 경험치 · 속성 성장 트리 ([레벨 · 성장 트리](./progression)) |
| `Source/Grimoire/Quest/` | 퀘스트 표 · 진행 · 추적 위젯 · 튜토리얼 소품(작물 밭 · 트리거) ([튜토리얼](./tutorial)) |
| `Source/Grimoire/Factions/` | 마법탑 NPC · 대화 · 상점 · 전투 금지 볼륨 · 시험 표적. 사냥꾼은 계획 |
| `Source/Grimoire/Online/` | 세션 · 접속 · 메뉴 게임 모드 · 이 PC 의 옵션 ([월드 · 멀티](./world-multiplayer#세션-·-접속-·-메뉴-m3-갈래-j)) |
| `Source/Grimoire/World/` | 지형 생성 · 분석 · 표식 · 개발 글씨 · 이동 치트 `Grim.Go` ([지형](./terrain)). 던전 인스턴스는 계획 |
| `Source/Grimoire/Save/` | 세이브 서브시스템 · 파일 · 레코드 · 플레이어 id |
| `Source/Grimoire/UI/` | HUD · 그리모어 화면 · 레벨 · 성장 트리 · 메뉴 위젯 (C++ UMG — 위젯 블루프린트 없음) |
| (계획) `Base` | 거점 · 건축 · 농사 · 생산 — M4 에 새 폴더로 ([건축](./building)). 인벤토리는 `Items/` 에 먼저 생겼다 |

클래스 접두는 `Grim` (예: `AGrimWand`, `UGrimSpellDefinition`).

플러그인 · 모듈:

- `Grimoire.uproject` 에서 켠 것: `GameplayAbilities`, 에디터 전용 `PythonScriptPlugin` · `EditorScriptingUtilities` · `ModelingToolsEditorMode` · `ToolsetRegistry` · `NiagaraToolsets`(이펙트 헤드리스 조립). Enhanced Input · Niagara · OnlineSubsystemNull 은 엔진 기본으로 켜져 있다.
- `Grimoire.Build.cs` 의존: `GameplayAbilities` · `GameplayTags` · `GameplayTasks` · `EnhancedInput` · `UMG` · `Niagara` · `AIModule`(생물 AI) · `OnlineSubsystem` · `OnlineSubsystemUtils` · `Sockets`(접속) · `Landscape` · `Foliage` · `Json`(지형) · `RHI` · `RenderCore`(성능 측정), 에디터 빌드만 `UnrealEd`.
- 검토만 하고 안 쓴 것: `StateTree`(생물 AI 는 `AAIController` + 타이머 판단), `CommonUI`. Steam 은 출시 때(R32).

## 핵심 클래스

| 클래스 | 역할 | 복제 | 상태 |
|---|---|---|---|
| `AGrimGameMode` | 서버 규칙, 폰 · 컨트롤러 지정 | 서버 전용 | 구현: `Source/Grimoire/Core/GrimGameMode.h` |
| `AGrimGameState` | 빈 껍데기 — 시간 · 날씨는 각 시스템의 복제 액터가 든다 | 모두에게 | 구현: `Source/Grimoire/Core/GrimGameState.h` |
| `AGrimPlayerState` | **ASC 소유**, 어트리뷰트(`VitalSet` · `MagicSet`), 마법서 · 서클 · 생존 · 레벨 · 성장 트리 · 외형 컴포넌트 | 소유자 + 일부 공개 | 구현: `Source/Grimoire/Core/GrimPlayerState.h` |
| `AGrimPlayerCharacter` | 이동 · 입력 · 완드 · 그리모어 책 · 카메라 | 모두에게 | 구현: `Source/Grimoire/Character/GrimPlayerCharacter.h` |
| `AGrimPlayerController` | 조준(`UGrimTargetingComponent`), 치트 서버 RPC, 개발 빌드 CheatManager | 소유자 | 구현: `Source/Grimoire/Character/GrimPlayerController.h` |
| `AGrimWand` | 완드 장비 액터, 시전 소켓, 속성 친화 · 마나 절약 | 모두에게 | 구현: `Source/Grimoire/Magic/GrimWand.h` |
| `UGrimAbilitySystemComponent` | GAS ASC 확장 — 입력 태그(`Input.*`)로 어빌리티 발동 | GAS 규칙 | 구현: `Source/Grimoire/Core/GrimAbilitySystemComponent.h` |
| `UGrimSpellbookComponent` | 배운 마법 · 숙련도(FastArray) · 단축 슬롯 | 소유자 | 구현: `Source/Grimoire/Magic/GrimSpellbookComponent.h` |
| `UGrimElementalStateComponent` | 사물 · 생물의 속성 상태 + 반응 처리 | 상태 태그 · 세기(FastArray, 휴면) | 구현: `Source/Grimoire/Interaction/GrimElementalStateComponent.h` |
| `UGrimTimeOfDaySubsystem` / `UGrimWeatherSubsystem` | 시간 · 지역 날씨(월드 서브시스템) | `AGrimTimeOfDayReplicator` · `AGrimWeatherReplicator` 로 | 구현: `Source/Grimoire/Environment/GrimTimeOfDaySubsystem.h` · `Source/Grimoire/Environment/GrimWeatherSubsystem.h` |
| `AGrimCreature` | 생물 폰(ASC 자체 보유), 개체 굴림 · 길들임 | 모두에게 | 구현: `Source/Grimoire/Creatures/GrimCreature.h` |
| `UGrimEcosystemSubsystem` | 서식 구역 개체군, 플레이어 근처만 펼침(활성 상한) | 서버 전용 | 구현: `Source/Grimoire/Creatures/GrimEcosystemSubsystem.h` |
| `UGrimInventoryComponent` | 플레이어 가방 | 소유자(FastArray) | 구현: `Source/Grimoire/Items/GrimInventoryComponent.h` |
| `UGrimSessionSubsystem` | 서버 열기 · 참가 · 끊김 메시지 · 슬롯 목록 | 로컬(게임 인스턴스) | 구현: `Source/Grimoire/Online/GrimSessionSubsystem.h` |
| `UGrimSaveSubsystem` | 세이브 · 로드 | 서버 전용 | 구현: `Source/Grimoire/Save/GrimSaveSubsystem.h` |
| `AGrimBaseCore` | 거점 핵, 반경 · 권한 · 일꾼 배정 | 모두에게 | 계획(M4) |
| `UGrimDungeonSubsystem` | 던전 인스턴스 슬롯 | 서버 전용 | 계획(M6) — [월드 · 멀티](./world-multiplayer#인스턴스-던전) |

## 공통 데이터

| 데이터 | 형식 | 내용 | 상태 |
|---|---|---|---|
| `DA_Spell_*` (`UGrimSpellDefinition`) | PrimaryDataAsset | 마법 하나 | 구현: `Source/Grimoire/Magic/GrimSpellDefinition.h`, `Scripts/Magic/make_spells.py` |
| `DA_Creature_*` (`UGrimCreatureSpecies`) | PrimaryDataAsset | 생물 종 하나 | 구현: `Source/Grimoire/Creatures/GrimCreatureSpecies.h`, `Scripts/Creatures/make_creature_species.py` |
| `VS_*` (`UGrimMagicFXVisualSet`) | DataAsset | 마법 연출 묶음 | 구현: `Source/Grimoire/MagicFX/GrimMagicFXVisualSet.h` |
| `Items.csv` · `Recipes.csv` | CSV(런타임 읽기) | 아이템(재료 · 음식 · 스킬북 · 화폐) · 요리 | 구현: `Source/Grimoire/Items/GrimItemSettings.h` — 아이템은 DataAsset 이 아니라 표의 행(id) |
| `ElementalReactions.csv` | CSV | 속성 반응 규칙 | 구현: `Source/Grimoire/Interaction/GrimInteractionSettings.h` |
| `ClimateRegions.csv` | CSV | 지역 기온 곡선 · 등급 · 날씨 전이표 | 구현: `Source/Grimoire/Environment/GrimWeatherSettings.h` |
| `CreatureEcology.csv` | CSV | 먹이 관계 | 구현: `Source/Grimoire/Creatures/GrimCreatureSettings.h` |
| `ProficiencyGrades.csv` | CSV | 숙련도 등급 D~S | 구현: `Source/Grimoire/Magic/GrimProficiencySettings.h` |
| `LevelCurve.csv` · `SkillTreeNodes.csv` | CSV | 레벨 곡선 · 성장 트리 노드 | 구현: `Source/Grimoire/Progression/GrimProgressionSettings.h` |
| `Quests.csv` · `QuestSteps.csv` · `QuestObjectives.csv` · `TowerNpcs.csv` · `TowerDialogue.csv` | CSV | 퀘스트 · 탑 NPC · 대사 | 구현: `Source/Grimoire/Quest/GrimQuestSettings.h` |
| `Appearance/*.csv` | CSV | 외형 조각 · 팔레트 · 얼굴 · NPC 규칙 | 구현: `Source/Grimoire/Appearance/GrimAppearanceSettings.h` |
| `Enchants.csv` · `EnchantLevels.csv` · `Skillbooks.csv` · `PactumSpells.csv` | CSV | 인챈트 · 스킬북 목록 · 파크툼 정의 원본 | 문서 · 스크립트용(코드는 아직 안 읽음) |
| `DA_BuildPiece_*` · 생산 레시피 | PrimaryDataAsset · 표 | 건축 조각 · 작업대 생산 | 계획(M4) |
| GameplayTags | 네이티브 태그 | `Element.*` · `SubElement.*` · `State.*` · `Status.*` · `Spell.*` · `Cooldown.*` · `Input.*` · `Creature.*` · `Progression.*` · `GameplayCue.*` · `Landmark.*` · `Poi.*` · `Habitat.*` … | 구현: 폴더마다 `Grim<시스템>Tags.h/.cpp` 의 `UE_DEFINE_GAMEPLAY_TAG`(공용은 `Source/Grimoire/Core/GrimGameplayTags.h`). ini 태그 파일은 쓰지 않는다 |

표 경로는 `Content/Grimoire/Data/` 기준, 설정은 `Config/DefaultGame.ini` 의 `[/Script/Grimoire.Grim*Settings]` 섹션(키 `*CSV`). 대부분 `Grim.<시스템>.Reload` 로 게임 중 다시 읽는다.

마법 · 종은 `GetPrimaryAssetId`(`Spell:<에셋 이름>`, `CreatureSpecies:<종 id>`)를 주지만, 지금 세이브는 에셋 **경로 문자열**을 저장한다. Asset Manager ID 로 바꾸는 것은 남은 일([세이브](./world-multiplayer#세이브-ugrimsavesubsystem-서버-전용)).

## 네트워크 규칙 (요약)

- 상태 변경은 서버 함수에서만. 클라이언트는 `Server_*` RPC 로 요청하거나 GAS 어빌리티 활성화(예측)로 한다.
- 복제는 **상태**를 복제하고(`OnRep_`), 연출은 클라이언트가 상태 변화를 보고 만든다. 연출 자체를 RPC 로 뿌리지 않는다(예외: 일회성 임팩트는 `NetMulticast` Unreliable 또는 GameplayCue).
- 관련성(Net Relevancy)과 Net Dormancy 를 처음부터 쓴다. 양초 수천 개가 모두 복제되면 안 된다 → 상태가 바뀔 때만 깨어난다.
- 상세: [월드 · 멀티플레이 · 세이브](./world-multiplayer), 예산: [성능](./performance).

## 문서 목록

- [마법 · 상호작용](./magic) — GAS 설계, 완드, 스킬북, 서클, 속성 태그, 반응 엔진
- [투사체 · 시전 감각](./spell-feel) · [이펙트 자체 제작](./magic-fx-authoring)
- [생존](./survival) — 어트리뷰트, 주기 효과, 체온 모델
- [환경 · 날씨](./environment) — 시간 · 날씨 · 기후 서브시스템, 렌더링 연동
- [마법 생물 · 생태계](./creatures) — 종 · 개체 · 유전 · 스폰 · AI · 길들이기 · 탑승 · 노동
- [레벨 · 성장 트리](./progression) · [마법탑 튜토리얼](./tutorial) · [커스터마이징](./customization) · [지형](./terrain)
- [건축 · 농사 · 생산](./building) — 거점 · 조각 건축 · 작물 · 생산 대기열 · 인벤토리
- [월드 · 멀티플레이 · 세이브](./world-multiplayer) — World Partition, 인스턴스 던전, 리슨/전용 서버, 세이브
- [성능 예산](./performance) · [로드맵 · 리스크](./roadmap)
