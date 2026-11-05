# M2 후반 작업 분할 — 탑에서 첫 밤까지

> 목표: M2 완료 기준 **「탑에서 시작해 첫 밤을 넘기고 3서클」** 을 실제로 플레이할 수 있게 한다. ([로드맵](./roadmap) M2, 앞 묶음 [M2 작업 분할](./m2-plan))

## 공통 기반 (main 에 있음 — 고치지 않는다)

| 파일 | 내용 |
|---|---|
| M1 · M1.5 · M2 앞 묶음 전부 | 전달 방식 · 조준(줌 포함) · 연출 · 생존 · 날씨 · 숙련도 · 스킬북 · 서클 승급(`UGrimProgressionComponent`) · HUD · 그리모어 화면 |
| `Core/GrimSaveTypes` | `EGrimSaveScope`(Player · World), 인터페이스 `IGrimSaveParticipant`(`GetSaveScope` · `GetSaveId` · `GetSaveVersion` · `WriteSaveData(FArchive&)` · `ReadSaveData(FArchive&, Version)`) — **저장할 것이 있는 쪽이 스스로 구현**하고, 세이브 시스템은 이것만 모은다 |

## 갈래와 파일 소유

| 갈래 | 소유 | 스펙 | 다른 갈래와 만나는 곳 |
|---|---|---|---|
| **K. 조작 · 기본 공격** | `Character/GrimPlayerCharacter.*`(입력), `Content/Grimoire/Input/` 의 이동 · 시전 입력(조준 입력 IA_LockOn · IA_AimZoom · IMC_Targeting 은 그대로), `Magic/` 의 새 파일(기본 공격 어빌리티 · 슬롯 시전), `MagicFX/**`, `UI/GrimHUDWidget*` | [마법 — 완드 · 기본 공격 (R26) · 기본 자세](../spec/magic) | 슬롯은 마법서의 `SlotSpells` · `ActiveSlotSet`(앞 묶음 G). 기본 공격은 마법 정의 하나(무속성 · 비용 0 · 쿨다운 0 · `bTracksProficiency=false`) |
| **L. 레벨 · 성장 트리** | 새 `Source/Grimoire/Progression/**`, `Core/GrimPlayerState.*`(컴포넌트 하나 더하는 줄만), `Content/Grimoire/Data/` 의 레벨 · 트리 표, `UI/GrimGrimoireWidget*`(성장 트리 탭) · 새 `UI/GrimSkillTree*` · 새 `UI/GrimLevelWidget*`(HUD 에는 통합 때 끼운다) | [성장](../spec/progression) | 공개 함수 `AwardExperience(Amount, SourceTag)`(다른 갈래가 부른다 — 튜토리얼 퀘스트 보상 등), `ResetTree(TreeId)`(정화 의식). 최대 HP · 마나는 GE 로(어트리뷰트 셋 파일은 안 고침). 자기 컴포넌트에 `IGrimSaveParticipant` 구현 |
| **V. 세이브 v1** | 새 `Source/Grimoire/Save/**`, 그리고 **이미 있는** 저장 대상에 `IGrimSaveParticipant` 구현을 더하는 줄: 마법서 · 서클(`Magic/GrimSpellbookComponent` · `GrimProgressionComponent`), 생존(`Survival/GrimSurvivalComponent`), 시간 · 날씨(`Environment/`), 속성 상태(`Interaction/GrimElementalStateComponent`), 침대 · 스킬북(놓인 액터) | [멀티플레이 — 세이브](../spec/multiplayer), [월드 · 멀티 · 세이브](./world-multiplayer#세이브) | 다른 갈래가 새로 만드는 것(레벨 · 트리 · 퀘스트)은 그 갈래가 인터페이스를 구현 — V 는 모으기만 |
| **T. 마법탑 튜토리얼** | 새 맵 `Content/Grimoire/Maps/L_Tower`(그레이박스 — 헤드리스 파이썬), 새 `Source/Grimoire/Quest/**`, 새 `Source/Grimoire/Factions/**`(탑 NPC), `Config/DefaultEngine.ini` 의 기본 맵 줄 | [세력 · 서사 — 마법탑](../spec/factions), [세계](../spec/world), [마법](../spec/magic) | 서클 시험 → `UGrimProgressionComponent` 승급 함수(있음). 퀘스트 경험 → L 의 `AwardExperience`, 정화 의식 → L 의 `ResetTree`(브랜치에 없으니 TODO 로, 통합 때 잇는다). 대리 수리 · 지우기는 인벤토리 · 내구도(M4) 전이라 대화 자리만. 자기 퀘스트 진행에 `IGrimSaveParticipant` 구현 |

- 규칙은 앞 묶음과 같다: `Grimoire.Build.cs` 는 끝에 한 줄, 새 태그는 자기 폴더, `Config/DefaultGame.ini` 는 **파일 끝에** 자기 섹션만, `docs/spec/decisions.md` 에 줄을 더하지 않는다(보고에 적으면 통합 때 옮긴다), 맵 배치는 T 의 새 맵에만(다른 갈래는 무엇을 놓을지 보고에).
- 빌드 · 확인 규칙: [M1.5 작업 분할](./m15-plan#빌드-·-확인-규칙). 새 테스트는 `Grimoire.Input.*` · `Grimoire.Magic.BasicAttack.*` · `Grimoire.Progression.*` · `Grimoire.Save.*` · `Grimoire.Quest.*` 아래.

## 통합 뒤 생존 조정 (R29 — 2026-11-05 스펙 변경)

갈래 V 가 생존 컴포넌트에 세이브 줄을 더하는 중이라, 이 조정은 네 갈래를 합친 뒤 한 번에 한다.

- 체온: 섭씨 게이지 수렴을 없애고 등급을 **즉시** 적용. 모자람 3 이상 5분 → 감기(추위) · 열병(더위, 새 질병).
- 피곤함: 높은 구간 = 탈진(달리기 불가 · 지친 걸음 속도 · 시전 차단 태그), 그 아래 = 시전 속도 · 명중(투사체 퍼짐) 저하. 강제 기절은 없앤다.
- 질병: 「상태별 디버프 총칭」 구조로 — 치료 수단 태그(약초 · 치료 마법 · 질병별 물약 · 정화)를 질병 정의 데이터에.
- 잠: `SleepSkipRatio` 옵션 삭제(항상 전원).

