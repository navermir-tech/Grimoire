# 시스템 문서 — 아키텍처 개요

> 이 문서들은 **어떻게 만들지**를 정한다. 무엇을 만들지는 [스펙 문서](../spec/)가 정하고, 각 시스템 문서는 관련 R-번호를 머리에 단다.
>
> 상태: v0.1 (2026-11-04). 클래스 · 데이터 이름은 설계안이며, 구현하면서 이 문서를 코드에 맞춘다.

## 전체 그림

```
[Client]  입력 → 예측 연출 → UI(그리모어 · HUD)
    │  RPC / 리플리케이션 (서버 권위)
[Server (리슨 또는 전용)]
    ├─ GameMode / GameState        세계 규칙, 시간 · 날씨 상태 복제
    ├─ PlayerState + ASC           서클 · 마법 · 생존 속성 (죽어도 유지)
    ├─ Character + Wand            이동 · 시전 · 장비
    ├─ Elemental Interaction       사물 · 생물의 속성 상태와 반응 규칙
    ├─ Environment Subsystems      TimeOfDay · Weather · Climate
    ├─ Creature Systems            종 · 개체 · 유전 · 생태계 AI · 길들이기 · 노동
    ├─ Base Systems                거점 · 건축 · 농사 · 생산 · 인벤토리
    ├─ Faction / Quest             마법탑 · 사냥꾼 · 목표
    ├─ World                       World Partition + 인스턴스 던전
    └─ Save Subsystem              세계 · 플레이어 저장
```

## 설계 원칙

1. **서버 권위, 처음부터.** 멀티(R13)가 스펙이므로 싱글도 "혼자 있는 리슨 서버"다. 모든 게임플레이 상태는 서버가 바꾸고 복제한다. 나중에 멀티를 붙이는 일은 없다.
2. **GAS(Gameplay Ability System) 위에 세운다.** 마법 · 생존 스탯 · 상태 · 버프 · 생물 능력까지 GAS 의 Ability / Effect / Attribute / Tag 로 통일한다. 예측 · 복제 · 쿨다운 · 비용을 공짜로 얻는다.
3. **데이터 주도.** 마법 · 종 · 건축 조각 · 레시피 · 반응 규칙은 전부 DataAsset / DataTable 이다. 새 콘텐츠는 코드 없이 데이터만 추가한다.
4. **속성 상태는 하나의 시스템.** 양초도 플레이어도 생물도 같은 `ElementalState` 컴포넌트와 같은 반응 규칙표를 쓴다(R16). 특수 케이스 코드를 만들지 않는다.
5. **C++ 가 뼈대, 블루프린트가 살.** 시스템 · 네트워크 · 성능이 걸린 코드는 C++. 콘텐츠(특정 마법의 연출, 특정 생물의 행동 세부)는 C++ 기반 클래스를 상속한 블루프린트.
6. **작게 시작.** 각 시스템의 "M1 범위"를 지키고 확장은 [로드맵](./roadmap)을 따른다.

## 모듈 · 폴더 구성

처음엔 **단일 게임 모듈 `Grimoire`** 로 간다. 컴파일 시간이 문제가 되면 아래로 나눈다(이름은 지금부터 폴더로 쓴다).

| 폴더 (→ 장차 모듈) | 내용 |
|---|---|
| `Source/Grimoire/Core` | 게임 모드 · 상태 · 플레이어 상태 · ASC · 공용 태그 · 속성 자극 인터페이스 |
| `Source/Grimoire/Character` | 플레이어 캐릭터 · 컨트롤러 · 입력 |
| `Source/Grimoire/Appearance` | 외형(커스터마이징) — 카탈로그 · 조립 · NPC 생성 · 생성 화면 ([커스터마이징](./customization)) |
| `Source/Grimoire/Magic` | 마법 어트리뷰트 · 마법 정의 · 완드 · 시전 어빌리티 · 상성 |
| `Source/Grimoire/Interaction` | 속성 상태 컴포넌트 · 반응 규칙 · 상호작용 사물 |
| `Source/Grimoire/Survival` | 생존 어트리뷰트 · 주기 효과 · 체온 |
| `Source/Grimoire/Environment` | 시간 · 날씨 · 기후 서브시스템, 하늘 컨트롤러 |
| `Source/Grimoire/Creatures` | 생물 · 종 · 유전 · 생태계 스폰 · AI 브리지 · 길들이기 · 탑승 · 노동 |
| `Source/Grimoire/Base` | 거점 · 건축 · 농사 · 생산 · 인벤토리 · 아이템 |
| `Source/Grimoire/Factions` | 사냥꾼 AI · 거점 · 이벤트 · 퀘스트 |
| `Source/Grimoire/World` | 던전 인스턴스 · 월드 파티션 보조 |
| `Source/Grimoire/Save` | 세이브 서브시스템 · 직렬화 |
| `Source/Grimoire/UI` | HUD · 그리모어 · 건축 UI (C++ 베이스, UMG 블루프린트) |

클래스 접두는 `Grim` (예: `AGrimWand`, `UGrimSpellDefinition`).

플러그인: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `StateTree`, `GameplayStateTree`, `EnhancedInput`, `Niagara`, `CommonUI`(검토), `OnlineSubsystemNull`(→ 나중에 Steam).

## 핵심 클래스 (설계안)

| 클래스 | 역할 | 복제 |
|---|---|---|
| `AGrimGameMode` | 서버 규칙, 스폰 · 부활, 서버 옵션 | 서버 전용 |
| `AGrimGameState` | 시간 · 날씨 상태, 서버 옵션 스냅샷 | 모두에게 |
| `AGrimPlayerState` | **ASC 소유**, 서클, 배운 마법(`UGrimSpellbookComponent`), 계정 ID | 소유자 + 일부 공개 |
| `AGrimPlayerCharacter` | 이동 · 입력 · 완드 장착 · 카메라, `UGrimElementalStateComponent` | 모두에게 |
| `AGrimWand` | 완드 장비 액터, 시전 소켓, 보정치 | 모두에게 |
| `UGrimAbilitySystemComponent` | GAS ASC 확장(속성 태그 조회 · 상성 계산 훅) | GAS 규칙 |
| `UGrimElementalStateComponent` | 사물 · 생물의 속성 상태 + 반응 처리 | 상태 태그 · 세기 복제 |
| `UGrimTimeOfDaySubsystem` / `UGrimWeatherSubsystem` | 환경 | GameState 를 통해 |
| `AGrimCreature` | 생물 폰(ASC 자체 보유), 유전 · 길들임 데이터 | 모두에게 |
| `AGrimBaseCore` | 거점 핵, 반경 · 권한 · 일꾼 배정 | 모두에게 |
| `UGrimSaveSubsystem` | 세이브 · 로드 | 서버 전용 |

## 공통 데이터

| 데이터 | 형식 | 내용 |
|---|---|---|
| `DA_Spell_*` (`UGrimSpellDefinition`) | PrimaryDataAsset | 마법 하나 |
| `DA_Species_*` (`UGrimSpeciesDefinition`) | PrimaryDataAsset | 생물 종 하나 |
| `DA_Item_*` (`UGrimItemDefinition`) | PrimaryDataAsset | 아이템(스킬북 · 재료 · 음식 · 완드) |
| `DA_BuildPiece_*` | PrimaryDataAsset | 건축 조각 |
| `DT_ElementalReactions` | DataTable | 속성 반응 규칙 |
| `DT_Recipes` | DataTable | 생산 레시피 |
| `DT_Climate_*` | DataTable / Curve | 지역 기온 곡선 · 날씨 확률 |
| `DT_Ecology` | DataTable | 먹이 관계 · 영역 다툼 |
| GameplayTags (`Config/Tags/*.ini`) | 태그 | `Element.*`, `Circle.*`, `State.*`, `Status.*`, `Spell.*`, `Creature.*`, `Work.*` |

Asset Manager 로 PrimaryDataAsset 을 ID 로 찾는다(`Spell:Fireball`, `Species:FlameLizard`). 세이브는 이 ID 만 저장한다.

## 네트워크 규칙 (요약)

- 상태 변경은 서버 함수에서만. 클라이언트는 `Server_*` RPC 로 요청하거나 GAS 어빌리티 활성화(예측)로 한다.
- 복제는 **상태**를 복제하고(`OnRep_`), 연출은 클라이언트가 상태 변화를 보고 만든다. 연출 자체를 RPC 로 뿌리지 않는다(예외: 일회성 임팩트는 `NetMulticast` Unreliable 또는 GameplayCue).
- 관련성(Net Relevancy)과 Net Dormancy 를 처음부터 쓴다. 양초 수천 개가 모두 복제되면 안 된다 → 상태가 바뀔 때만 깨어난다.
- 상세: [월드 · 멀티플레이 · 세이브](./world-multiplayer).

## 문서 목록

- [마법 · 상호작용](./magic) — GAS 설계, 완드, 스킬북, 서클, 속성 태그, 반응 엔진
- [생존](./survival) — 어트리뷰트, 주기 효과, 체온 모델
- [환경 · 날씨](./environment) — 시간 · 날씨 · 기후 서브시스템, 렌더링 연동
- [마법 생물 · 생태계](./creatures) — 종 · 개체 · 유전 · 스폰 · AI · 길들이기 · 탑승 · 노동
- [건축 · 농사 · 생산](./building) — 거점 · 조각 건축 · 작물 · 생산 대기열 · 인벤토리
- [월드 · 멀티플레이 · 세이브](./world-multiplayer) — World Partition, 인스턴스 던전, 리슨/전용 서버, 세이브
- [로드맵 · 리스크](./roadmap)
