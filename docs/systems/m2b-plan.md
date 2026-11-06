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
| **V. 세이브 v1** | 새 `Source/Grimoire/Save/**`, 그리고 **이미 있는** 저장 대상에 `IGrimSaveParticipant` 구현을 더하는 줄: 마법서 · 서클(`Magic/GrimSpellbookComponent` · `GrimProgressionComponent`), 생존(`Survival/GrimSurvivalComponent`), 시간 · 날씨(`Environment/`), 속성 상태(`Interaction/GrimElementalStateComponent`), 침대 · 스킬북(놓인 액터) | [멀티플레이 — 세이브](../spec/multiplayer), [월드 · 멀티 · 세이브](./world-multiplayer#세이브-ugrimsavesubsystem-서버-전용) | 다른 갈래가 새로 만드는 것(레벨 · 트리 · 퀘스트)은 그 갈래가 인터페이스를 구현 — V 는 모으기만 |
| **T. 마법탑 튜토리얼** | 새 맵 `Content/Grimoire/Maps/L_Tower.umap`(그레이박스 — 헤드리스 파이썬), 새 `Source/Grimoire/Quest/**`, 새 `Source/Grimoire/Factions/**`(탑 NPC), `Config/DefaultEngine.ini` 의 기본 맵 줄 | [세력 · 서사 — 마법탑](../spec/factions), [세계](../spec/world), [마법](../spec/magic) | 서클 시험 → `UGrimProgressionComponent` 승급 함수(있음). 퀘스트 경험 → L 의 `AwardExperience`, 정화 의식 → L 의 `ResetTree`(브랜치에 없으니 TODO 로, 통합 때 잇는다). 대리 수리 · 지우기는 인벤토리 · 내구도(M4) 전이라 대화 자리만. 자기 퀘스트 진행에 `IGrimSaveParticipant` 구현 |

- 규칙은 앞 묶음과 같다: `Grimoire.Build.cs` 는 끝에 한 줄, 새 태그는 자기 폴더, `Config/DefaultGame.ini` 는 **파일 끝에** 자기 섹션만, `docs/spec/decisions.md` 에 줄을 더하지 않는다(보고에 적으면 통합 때 옮긴다), 맵 배치는 T 의 새 맵에만(다른 갈래는 무엇을 놓을지 보고에).
- 빌드 · 확인 규칙: [M1.5 작업 분할](./m15-plan#빌드-·-확인-규칙). 새 테스트는 `Grimoire.Input.*` · `Grimoire.Magic.BasicAttack.*` · `Grimoire.Progression.*` · `Grimoire.Save.*` · `Grimoire.Quest.*` 아래.

## 결과 (2026-11-05)

L(`5b63272`) → V(`76ae11d`) → K(`9c27b38`) → T(`5ee83ed`, 중간에 main 을 T 에 합침 `fd32c3a`) 순으로 합쳤다. 이어 실내 피해 막기 · 시작 마법 개발 옵션(`3b32cad`), 스킬북 판정(`777bac0`), 그리모어 화면 키 T(`9b34695`)를 main 에 바로 고쳤다. 개발 기록: [M2](../dev/history#m2-·-살아남기-2026-11-05).

| 갈래 | 만든 것 | 주요 코드 · 스크립트 |
|---|---|---|
| L | 레벨 곡선 · 경험 출처(`LevelCurve.csv`), 속성 성장 트리 4개 84노드(`SkillTreeNodes.csv`), 최대 HP · 마나는 GE 보너스, 그리모어 성장 트리 탭 · 레벨 막대 | `Source/Grimoire/Progression/GrimLevelComponent.h` 의 `UGrimLevelComponent`(`AwardExperience`), `Source/Grimoire/Progression/GrimSkillTreeComponent.h`(`ResetTree`), `Source/Grimoire/UI/GrimSkillTreeWidget.h`, `Source/Grimoire/UI/GrimLevelWidget.h` |
| V | 세계 · 플레이어 세이브, 참여자 모으기 · 모르는 레코드 보존 · 버전, 봉투 + CRC · `.bak` 백업, PC 별 PlayerId | `Source/Grimoire/Save/GrimSaveSubsystem.h` 의 `UGrimSaveSubsystem`, `Source/Grimoire/Save/GrimSaveRecords.h`, `Source/Grimoire/Save/GrimPlayerIdentity.h`, `Scripts/Save/check_save.ps1` — [세이브](./world-multiplayer#세이브-ugrimsavesubsystem-서버-전용) |
| K | 호그와트 배치 — 슬롯 바로 시전 · 슬롯 세트, 기본 공격(4연타 강타 · `DA_Spell_BasicAttack`, 스킬북으로 배움) · 주먹질, 책 속성 문양 11개, HUD 키 글자 | `Source/Grimoire/Magic/GrimBasicAttackAbility.h`, `Source/Grimoire/Magic/GrimSpellSlotInput.h`, `Source/Grimoire/Magic/GrimFistAbility.h`, `Source/Grimoire/MagicFX/GrimBookSigils.h`, `Scripts/Magic/make_basic_attack.py`, `Scripts/Integration/net_check_controls.ps1` |
| T | 퀘스트 시스템(표 · 목표 · 진행 · 세이브 · 이벤트 태그), 탑 NPC 대화 · 서비스 · 승급 시험, 전투 금지 볼륨, `L_Tower` 그레이박스 | `Source/Grimoire/Quest/GrimQuestComponent.h`(정적 `UGrimQuestComponent::NotifyQuestEvent`), `Source/Grimoire/Quest/GrimQuestSubsystem.h`, `Source/Grimoire/Factions/GrimTowerNPC.h`, `Source/Grimoire/Factions/GrimNoCombatVolume.h`, `Scripts/Tutorial/build_tower_map.py`, `Scripts/Tutorial/check_tutorial.ps1` — [튜토리얼](./tutorial) |

- 설계 선택: 서클은 최대 마나와 무관 — 레벨이 정한다(`3622f43`). 시작 마법은 개발 옵션 `-GrimStartingSpells` 로만(R31) — 확인 스크립트가 이 옵션을 붙인다. 탑 실내는 `Zone.NoCombat` 로 피해만 막고 속성 자극은 그대로.
- 대리 수리 · 지우기는 인벤토리 · 내구도(M4) 전이라 대화 자리만 있다. 게임 맵은 M3 통합 때 `L_Tower` → `L_OpenBeta` 로 바뀌었다(탑은 `Scripts/Tutorial/tower_greybox.py` 로 지형 위에 다시 짓는다).

## 통합 뒤 생존 조정 (R29 — 2026-11-05 스펙 변경)

> 상태: **아직 안 함**(2026-11-06 코드 기준). `Source/Grimoire/Survival/` 에는 옛 동작이 남아 있다 — 체온 수렴(`BodyTemperatureTimeConstantSeconds`), 피곤함 바닥 기절(`Status.Collapsed` 강제 수면), 밤 건너뛰기 비율 `SleepSkipRatio`(기본 1 = 전원이라 동작은 스펙과 같다), 열병 없음.

갈래 V 가 생존 컴포넌트에 세이브 줄을 더하는 중이라, 이 조정은 네 갈래를 합친 뒤 한 번에 한다.

- 체온: 섭씨 게이지 수렴을 없애고 등급을 **즉시** 적용. 모자람 3 이상 5분 → 감기(추위) · 열병(더위, 새 질병).
- 피곤함: 높은 구간 = 탈진(달리기 불가 · 지친 걸음 속도 · 시전 차단 태그), 그 아래 = 시전 속도 · 명중(투사체 퍼짐) 저하. 강제 기절은 없앤다.
- 질병: 「상태별 디버프 총칭」 구조로 — 치료 수단 태그(약초 · 치료 마법 · 질병별 물약 · 정화)를 질병 정의 데이터에.
- 잠: `SleepSkipRatio` 옵션 삭제(항상 전원).

