# 규칙 · 명명

## 언어

- 식별자(클래스 · 함수 · 변수 · 에셋 이름)는 **영어**. 주석 · 문서 · 커밋 메시지 · 게임 문구는 **한국어**.
- C++ 파일은 UTF-8. UBT 가 컴파일러에 `/utf-8` 을 주므로 BOM 은 없어도 된다.

## C++

- 언리얼 코딩 표준을 따른다: 클래스 접두 `A`(액터) `U`(오브젝트) `F`(구조체) `E`(열거) `I`(인터페이스) `T`(템플릿). 프로젝트 접두는 **`Grim`**(`AGrimWand`, `UGrimSpellDefinition`, `FGrimCreatureInstance`).
- 탭 들여쓰기, 중괄호는 새 줄(엔진 스타일). `.editorconfig` 가 맞춰 준다.
- 상태를 바꾸는 함수는 서버에서만(`check(HasAuthority())` 또는 이른 return). 클라이언트 요청은 `Server_` 접두 RPC.
- 리플리케이션 변수는 `OnRep_` 로 연출을 분리한다. 틱 대신 타이머 · 델리게이트.
- 수치는 데이터에 두고 코드에 하드코딩하지 않는다. 지금 쓰는 곳: 시스템별 설정 클래스 `UGrim*Settings`(`UDeveloperSettings`, `Config/DefaultGame.ini` 의 `[/Script/Grimoire.Grim*Settings]`), 표는 `Content/Grimoire/Data/*.csv`(런타임에 읽음 — `Grim.*.Reload`), 마법 · 종은 DataAsset(`Scripts/` 의 파이썬이 정본). C++ 기본값은 ini 에 없을 때만 쓰인다.
- 순수 계산은 `F<이름>Math`(엔진 없이 테스트), 자동화 테스트는 `<시스템>/Tests/` 에 `Grimoire.<시스템>.*`. 유니티 빌드라 다른 .cpp 의 익명 네임스페이스 · 전역 이름이 부딪친다 — 도우미 · 상수엔 파일별 고유 이름(이름 붙인 네임스페이스).
- 로그는 `LogGrimoire`(공용) 또는 시스템별 카테고리(`LogGrimMagic` · `LogGrimSurvival` · `LogGrimCreatures` · `LogGrimSave` · `LogGrimOnline` 등).
- 개발 콘솔 명령은 `Grim.<시스템>.<동작>`(`FAutoConsoleCommand`, `*Debug.cpp`, `#if !UE_BUILD_SHIPPING`), 치트 매니저 Exec 는 `Grim<이름>` — 목록은 [테스트 치트](./cheats).

## 에셋 명명 (Content/Grimoire 아래)

| 접두 | 종류 | 예 |
|---|---|---|
| `BP_` | 블루프린트 액터 · 컴포넌트 | `BP_Creature_Jackalope`(지금은 없음 — 양초 · 생물 · NPC 모두 C++ 클래스를 그대로 놓는다) |
| `WBP_` | UMG 위젯 | `WBP_Grimoire`, `WBP_HUD`(지금은 없음 — 위젯은 C++ 로 짠 `UGrim*Widget`) |
| `GA_` / `GE_` / `GC_` | 어빌리티 · 이펙트 · 게임플레이 큐 | `GA_Spell_Fireball`, `GE_Status_Wet`(지금은 에셋 없이 C++ 클래스 — `UGrimSpellAbility` · `UGrimGE_SpellCost` · `UGrimCueNotify_SpellImpact`) |
| `DA_` | 데이터 에셋 | `DA_Spell_Ember`, `DA_Creature_Jackalope`(종). 아이템은 에셋이 아니라 `Items.csv` 의 행 |
| `DT_` | 데이터 테이블 | (지금은 안 씀 — 표는 CSV 를 런타임에 읽는다. 에디터 임포트로 바꾸면 `DT_ElementalReactions` 식) |
| `VS_` | 마법 연출 묶음(`UGrimMagicFXVisualSet`) | `VS_Ember`, `VS_WaterJet`, `VS_Breeze` |
| `IA_` / `IMC_` | 입력 액션 · 매핑 컨텍스트 | `IA_SpellSlot1`, `IA_LockOn`, `IMC_Spell`, `IMC_Targeting` |
| `ST_` | StateTree | `ST_Creature_Base`(안 씀 — 생물 AI 는 C++ 컨트롤러) |
| `L_` | 레벨 | `L_OpenBeta`, `L_MainMenu`, `L_Tower`, (계획) `L_Dungeon_Crypt` |
| `SM_` / `SK_` / `ABP_` / `AM_` / `AS_` | 스태틱 · 스켈레탈 메시 · 애님 BP · 몽타주 · 시퀀스 | |
| `M_` / `MI_` / `MF_` / `T_` | 머티리얼 · 인스턴스 · 함수 · 텍스처 | |
| `NS_` / `NE_` | Niagara 시스템 · 이미터 | `NS_Flame_Small` |
| `S_` / `SC_` / `SW_` | 사운드 | |
| `DL_` | 데이터 레이어 | |

폴더: `Content/Grimoire/<시스템>/<종류>`(예: `Magic/Spells/DA_Spell_Fireball`). 마켓 · 샘플 에셋은 `Content/<팩 이름>/` 에 그대로 두고 참조만 한다.

## GameplayTag 명명

`Element.Fire` · `State.Burning` · `Status.Hungry` · `Material.Flammable` · `Spell.<이름>` · `Cooldown.<이름>` · `Input.Spell.Cast` · `Creature.*` · `Progression.Node.*` · `GameplayCue.Spell.Impact` · `Landmark.*` · `Poi.*` · `Habitat.*`(계획: `Work.Water` · `Biome.Snow`). 태그는 **네이티브 태그**로 자기 폴더의 `Grim<시스템>Tags.h/.cpp` 에 `UE_DEFINE_GAMEPLAY_TAG(_COMMENT)` 로 선언한다(공용 `Element.*` · `State.Burning/Wet` · `Input.Spell.*` 은 `Source/Grimoire/Core/GrimGameplayTags.h`, 큐는 `Source/Grimoire/Core/GrimSpellFXTypes.h`). ini 태그 파일(Config/Tags)은 쓰지 않는다.

## Git

- 브랜치: `main` 은 늘 열리고 빌드되는 상태. 작업은 세션마다 git 워크트리 · 브랜치(`claude/<이름>-<해시>`) → 메인 세션이 `main` 에 「합치기: <갈래>」 커밋으로 합친다([개발 기록 — 일하는 방식](./history#일하는-방식)). 문서만 고칠 땐 `main` 직접 커밋 가능.
- 커밋 메시지: 한국어 한 줄로 "무엇을 했는지". 필요하면 빈 줄 뒤 설명.
- `.uasset` · `.umap` · 그림 · 소리 · 모델 · 글꼴은 LFS(`.gitattributes`) — 단 `docs/` 아래 그림은 LFS 가 아니다(문서 사이트 빌드가 LFS 를 받지 않는다). 새 바이너리 확장자는 거기에 한 줄 더한 뒤 커밋.
- 생성물(`Binaries` `Intermediate` `Saved` `DerivedDataCache` `*.sln`)은 커밋하지 않는다.
- 바이너리 에셋은 둘이 동시에 고치면 합칠 수 없다. 같은 에셋을 만지기 전에 서로 말한다(혼자 개발 중에도 두 세션이 같은 블루프린트를 열지 않도록).

## 문서

- 스펙(무엇)은 `docs/spec/`, 시스템(어떻게)은 `docs/systems/`. 결정이 바뀌면 [결정 기록](../spec/decisions)에 한 줄.
- 코드가 문서와 달라지면 **문서를 코드에 맞춘다**(마일스톤 끝마다 점검).
