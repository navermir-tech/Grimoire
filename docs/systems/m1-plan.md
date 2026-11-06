# M1 작업 분할 — 완드와 양초

> 목표: **양초를 켜고 바람으로 키우고 물로 끈다 — 둘이 함께.** ([로드맵](./roadmap) M1)
>
> M1 은 네 갈래를 병렬 세션으로 나눠 만들고, 마지막에 하나로 합쳐 샌드박스 맵에서 2인 복제를 검증한다.

## 공통 기반 (main 에 있음)

모든 갈래가 이 위에서 시작한다. 기반 파일은 **고치지 않는다**(고쳐야 하면 이유를 커밋 메시지와 이 문서에 남긴다).

| 파일 | 내용 |
|---|---|
| `Grimoire.uproject` · `Grimoire.Build.cs` | GAS · Niagara · Python 에디터 스크립팅 플러그인, 모듈 의존성. 모듈 폴더가 include 루트(`#include "Core/GrimGameMode.h"`) |
| `Core/GrimGameplayTags` | `Element.*` 6개, `State.Burning` · `State.Wet`, `Input.Spell.Cast/Next/Prev` |
| `Core/GrimAbilitySystemComponent` | 입력 태그 방식: 어빌리티 Spec 의 `GetDynamicSpecSourceTags()` 에 `Input.*` 를 넣으면 그 입력에 발동 |
| `Core/GrimPlayerState` | ASC(Mixed) · `VitalSet` · `MagicSet` · `Spellbook` · `Survival` 소유. `InitAbilityActorInfoForPawn(Pawn)` · `OnAbilityActorInfoReady` 델리게이트 |
| `Core/GrimElementalTypes` | `FGrimElementalStimulus`, 인터페이스 `IGrimElementalReceiver`, `UGrimElementalStatics::ApplyStimulusToActor / GetStateIntensity` |
| `Core/GrimGameMode` · `GrimGameState` | GameState · PlayerState 클래스 지정 |
| `Magic/GrimMagicAttributeSet` · `Magic/GrimSpellbookComponent` | 빈 껍데기 — 갈래 B 가 채운다 |
| `Survival/GrimVitalAttributeSet` · `Survival/GrimSurvivalComponent` | 빈 껍데기 — 갈래 D 가 채운다 |

## 갈래와 파일 소유

같은 파일을 두 갈래가 고치지 않도록 소유를 나눈다. 다른 갈래 기능이 필요하면 **기반 인터페이스만** 쓴다.

| 갈래 | 소유 (만들고 고치는 곳) | 다른 갈래와 만나는 곳 |
|---|---|---|
| **A. 캐릭터 · 입력 · 맵** | `Source/Grimoire/Character/**`, `Core/GrimGameMode.cpp`(기본 폰 · 컨트롤러 지정만), `Content/Characters/`, `Content/Input/`(템플릿에서 가져온 것), `Content/Grimoire/Input/`, `Content/Grimoire/Maps/L_M1_Sandbox.umap`, `Config/DefaultEngine.ini` 의 맵 항목, `Config/DefaultInput.ini` | 폰이 `PossessedBy` · `OnRep_PlayerState` 에서 `InitAbilityActorInfoForPawn` 호출. 시전 · 선택 입력은 `UGrimAbilitySystemComponent::AbilityInputTagPressed(Input.Spell.*)` 로만 보냄. 손 소켓 이름 `hand_r` |
| **B. 마법 · 완드** | `Source/Grimoire/Magic/**`(어트리뷰트 · 마법서 껍데기 포함), `Content/Grimoire/Magic/` | `OnAbilityActorInfoReady` 에서 시작 마법 지급 · 완드 생성 후 폰 메시 `hand_r` 에 부착. 맞은 대상에 `UGrimElementalStatics::ApplyStimulusToActor`. 선택 마법만 `Input.Spell.Cast` 태그를 가진다 |
| **C. 속성 상호작용** | `Source/Grimoire/Interaction/**`, `Content/Grimoire/Interaction/`(안 생김), `Content/Grimoire/Data/ElementalReactions.csv` 류 | `UGrimElementalStateComponent` 가 `IGrimElementalReceiver` 구현. 양초 · 모닥불 · 마른 풀 액터 |
| **D. 환경 · 생존** | `Source/Grimoire/Environment/**`, `Source/Grimoire/Survival/**`(껍데기 포함), `Content/Grimoire/Environment/`, `Content/Grimoire/Survival/`(안 생김) | `OnAbilityActorInfoReady` 에서 초기 어트리뷰트 · 주기 효과. 열원은 `GetStateIntensity(State.Burning)` 로 조회. 하늘은 스스로 조명 컴포넌트를 가진 액터 하나 |

공용으로 손댈 수밖에 없는 것:

- `Grimoire.Build.cs` 에 모듈을 더해야 하면 **목록 끝에 한 줄**만 더한다(합칠 때 충돌을 쉽게 풀기 위해).
- 새 GameplayTag 는 자기 폴더의 `<시스템>Tags.h/.cpp` 에 `UE_DEFINE_GAMEPLAY_TAG` 로 선언한다. `Core/GrimGameplayTags` 는 고치지 않는다.
- 맵(`L_M1_Sandbox`)에 액터를 놓는 일은 갈래 A 만 한다. 다른 갈래는 "무엇을 어디에 놓을지"를 자기 결과 보고에 적고, 통합 단계에서 배치한다.

## 에셋 만드는 방법

`.uasset` 은 텍스트로 만들 수 없다. 갈래는 둘 중 하나로 만든다.

1. **C++ 만으로**: 기본값을 C++ 클래스에 두고, 메시는 엔진 기본 도형(`/Engine/BasicShapes/...`) 참조.
2. **에디터 파이썬 스크립트**: `Scripts/<시스템>/*.py` 를 커밋하고 헤드리스로 돌려 에셋을 만든다. 스크립트가 원본이고 에셋은 결과다.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Magic\make_spells.py" -unattended -nop4 -nosplash -nullrhi
```

## 검증

- 각 갈래: `GrimoireEditor` 빌드 성공 + 순수 로직은 자동화 테스트(`Source/Grimoire/<시스템>/Tests/`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, 이름 `Grimoire.<시스템>.*`).

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire.<시스템>; Quit" -unattended -nop4 -nosplash -nullrhi -log
```

- 통합: 네 브랜치를 합친 뒤 `L_M1_Sandbox` 에 양초 · 모닥불 · 마른 풀 · 하늘을 놓고, PIE 2 클라이언트(리슨 서버)로 완료 기준을 확인한다.

## 갈래별 완료 기준

| 갈래 | 완료 기준 |
|---|---|
| A | 3인칭 이동 · 카메라 · 점프, 입력 태그 전달, 샌드박스 맵이 기본 맵으로 열림, 클라이언트 2명이 서로 보임 |
| B | 불씨 · 물줄기 · 미풍 3개 마법(투사체 또는 광선), 마나 소모 · 쿨다운, 완드 1종, 다음/이전 선택, 상성 표, 상성 · 비용 테스트 |
| C | 상태 컴포넌트(복제 · 휴면) + 반응 규칙표(10행 이상) + 전파, 양초 · 모닥불 · 마른 풀, 「양초 예시」 규칙이 테스트로 통과 |
| D | 밤 · 낮(서버 시간 복제 + 클라이언트 외삽) · 하늘 액터, 전역 기온, HP · 마나 회복 · 배고픔 감소 · 체온 수렴(열원 반영), 체온 · 배고픔 테스트 |

## 결과 (2026-11-05)

A(`ff12906`) → D(`3944650`) → C(`4b51551`) → B(`7772cf9`) 순으로 합치고, 통합 커밋 `ba78ea6` 에서 오류 수정 · 배치를 했다. 이어 치트(`3a902fd`)와 2인 화면 버그 수정(`7d35f92`)을 합쳤다. 개발 기록: [M1](../dev/history#m1-·-완드와-양초-2026-11-05).

| 갈래 | 만든 것 | 주요 코드 · 스크립트 |
|---|---|---|
| A | 3인칭 캐릭터 · 컨트롤러, 입력 액션 → `Input.*` 태그로 어빌리티 발동, 템플릿 마네킹 · 입력, 샌드박스 맵 | `Source/Grimoire/Character/GrimPlayerCharacter.h` 의 `AGrimPlayerCharacter`, `Source/Grimoire/Character/GrimPlayerController.h`, `Scripts/Character/make_sandbox_map.py` · `make_input.py` |
| B | 불씨 · 물줄기 · 미풍(`DA_Spell_Ember` · `DA_Spell_WaterJet` · `DA_Spell_Breeze`), 비용 · 쿨다운 GE(SetByCaller), 상성 피해, 마법서 선택, 완드 부착 | `Source/Grimoire/Magic/GrimSpellAbility.h` 의 `UGrimSpellAbility`, `Source/Grimoire/Magic/GrimSpellDefinition.h`, `Source/Grimoire/Magic/GrimDamageExecution.h`, `Source/Grimoire/Magic/GrimWand.h`, `Scripts/Magic/make_spells.py` |
| C | 상태 컴포넌트(FastArray 복제 · 휴면), 반응 규칙표 11행(**CSV 를 런타임에 읽음** — DataTable 아님), 전파 서브시스템, 양초 · 모닥불 · 마른 풀(C++ `AGrimCandle` · `AGrimCampfire` · `AGrimDryGrass`) | `Source/Grimoire/Interaction/GrimElementalStateComponent.h`, `Source/Grimoire/Interaction/GrimElementalObject.h`, `Content/Grimoire/Data/ElementalReactions.csv` |
| D | 낮밤(서버 스냅숏 복제 + 클라이언트 외삽), 하늘 액터, 전역 기온, 생존 1초 주기 GE · 체온 수렴 · 상태 태그 | `Source/Grimoire/Environment/GrimTimeOfDaySubsystem.h`, `Source/Grimoire/Environment/GrimSkyController.h`, `Source/Grimoire/Survival/GrimSurvivalComponent.h`, `Source/Grimoire/Survival/GrimSurvivalModel.h` |
| 통합 | 플레이어에도 속성 상태(물 맞으면 젖고 체온 하락), 배고프면 마나 회복 멈춤, 패키징에 CSV 포함, 샌드박스 배치 | `Scripts/Integration/place_m1_props.py` |
| 치트 · 버그 | `GrimSuperCheat`, 휴면 깨우기를 바꾸기 **전**으로(원격 불빛), 완드 Snap 부착 | `Source/Grimoire/Core/Cheats/`, `Scripts/Integration/net_check.ps1` |

- 확인: 빌드 성공, 자동화 테스트 16개 통과, `net_check.ps1` 헤드리스 2인.
- 계획과 달라진 것: 반응 규칙표는 `DT_ElementalReactions` 대신 CSV(설정 `UGrimInteractionSettings::ReactionTableCSV`), 사물은 블루프린트 없이 C++ 클래스를 그대로 놓았다. 기반 파일 `Core/GrimGameplayTags` 는 그 뒤 갈래에서도 고치지 않았다(새 태그는 폴더별 `Grim<시스템>Tags`).
- 지금 게임 맵은 `L_OpenBeta`(M3), `L_M1_Sandbox` 는 시험용으로 남았다.
