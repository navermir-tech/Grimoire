# 규칙 · 명명

## 언어

- 식별자(클래스 · 함수 · 변수 · 에셋 이름)는 **영어**. 주석 · 문서 · 커밋 메시지 · 게임 문구는 **한국어**.
- C++ 파일은 UTF-8. UBT 가 컴파일러에 `/utf-8` 을 주므로 BOM 은 없어도 된다.

## C++

- 언리얼 코딩 표준을 따른다: 클래스 접두 `A`(액터) `U`(오브젝트) `F`(구조체) `E`(열거) `I`(인터페이스) `T`(템플릿). 프로젝트 접두는 **`Grim`**(`AGrimWand`, `UGrimSpellDefinition`, `FGrimGenome`).
- 탭 들여쓰기, 중괄호는 새 줄(엔진 스타일). `.editorconfig` 가 맞춰 준다.
- 상태를 바꾸는 함수는 서버에서만(`check(HasAuthority())` 또는 이른 return). 클라이언트 요청은 `Server_` 접두 RPC.
- 리플리케이션 변수는 `OnRep_` 로 연출을 분리한다. 틱 대신 타이머 · 델리게이트.
- 수치는 데이터(DataAsset · DataTable · ini)에 두고 코드에 하드코딩하지 않는다.
- 로그는 `LogGrimoire`(공용) 또는 시스템별 카테고리(`LogGrimMagic` 등).

## 에셋 명명 (Content/Grimoire 아래)

| 접두 | 종류 | 예 |
|---|---|---|
| `BP_` | 블루프린트 액터 · 컴포넌트 | `BP_Candle`, `BP_Creature_FlameLizard` |
| `WBP_` | UMG 위젯 | `WBP_Grimoire`, `WBP_HUD` |
| `GA_` / `GE_` / `GC_` | 어빌리티 · 이펙트 · 게임플레이 큐 | `GA_Spell_Fireball`, `GE_Status_Wet` |
| `DA_` | 데이터 에셋 | `DA_Spell_Fireball`, `DA_Species_FlameLizard`, `DA_Item_Skillbook_Fireball` |
| `DT_` | 데이터 테이블 | `DT_ElementalReactions`, `DT_Recipes` |
| `IA_` / `IMC_` | 입력 액션 · 매핑 컨텍스트 | `IA_Cast`, `IMC_Default` |
| `ST_` | StateTree | `ST_Creature_Base` |
| `L_` | 레벨 | `L_World`, `L_Dungeon_Crypt` |
| `SM_` / `SK_` / `ABP_` / `AM_` / `AS_` | 스태틱 · 스켈레탈 메시 · 애님 BP · 몽타주 · 시퀀스 | |
| `M_` / `MI_` / `MF_` / `T_` | 머티리얼 · 인스턴스 · 함수 · 텍스처 | |
| `NS_` / `NE_` | Niagara 시스템 · 이미터 | `NS_Flame_Small` |
| `S_` / `SC_` / `SW_` | 사운드 | |
| `DL_` | 데이터 레이어 | |

폴더: `Content/Grimoire/<시스템>/<종류>`(예: `Magic/Spells/DA_Spell_Fireball`). 마켓 · 샘플 에셋은 `Content/<팩 이름>/` 에 그대로 두고 참조만 한다.

## GameplayTag 명명

`Element.Fire` · `Circle.3` · `State.Burning` · `Status.Hungry` · `Material.Flammable` · `Spell.Fireball` · `Creature.Size.Large` · `Work.Water` · `Biome.Snow` · `Cue.Spell.Fireball.Impact`. 태그는 `Config/Tags/<영역>.ini` 에 영역별로 둔다.

## Git

- 브랜치: `main` 은 늘 열리고 빌드되는 상태. 작업은 `feature/<이름>` → 합치기. 문서만 고칠 땐 `main` 직접 커밋 가능.
- 커밋 메시지: 한국어 한 줄로 "무엇을 했는지". 필요하면 빈 줄 뒤 설명.
- `.uasset` · `.umap` · 그림 · 소리 · 모델은 LFS(`.gitattributes`). 새 바이너리 확장자는 거기에 한 줄 더한 뒤 커밋.
- 생성물(`Binaries` `Intermediate` `Saved` `DerivedDataCache` `*.sln`)은 커밋하지 않는다.
- 바이너리 에셋은 둘이 동시에 고치면 합칠 수 없다. 같은 에셋을 만지기 전에 서로 말한다(혼자 개발 중에도 두 세션이 같은 블루프린트를 열지 않도록).

## 문서

- 스펙(무엇)은 `docs/spec/`, 시스템(어떻게)은 `docs/systems/`. 결정이 바뀌면 [결정 기록](../spec/decisions)에 한 줄.
- 코드가 문서와 달라지면 **문서를 코드에 맞춘다**(마일스톤 끝마다 점검).
