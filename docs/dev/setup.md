# 시작하기

## 요구 사항

| 항목 | 버전 · 메모 |
|---|---|
| Unreal Engine | **5.8** — Epic Games Launcher 로 설치(`C:\Program Files\Epic Games\UE_5.8`) |
| Visual Studio 2022 | 「C++를 사용한 게임 개발」 워크로드 + 「Unreal Engine 설치 관리자」 구성 요소 |
| Git · Git LFS | `git lfs install` 을 한 번 해 둔다 |
| Node.js 20+ | 문서 사이트(VitePress) 전용. 게임 빌드에는 필요 없다 |

## 저장소 받기

```bash
git clone https://github.com/navermir-tech/Grimoire.git
cd Grimoire
```

Git LFS 가 설치돼 있으면 `.uasset` 등 바이너리 에셋도 함께 내려온다(LFS 대상은 `.gitattributes` — 언리얼 에셋 · 3D · 그림 · 소리 · 글꼴. `docs/` 아래 그림은 LFS 가 아니다).

에디터는 `L_OpenBeta`(오픈 베타 지형)로 열리고, 게임으로 실행하면 메인 메뉴 `L_MainMenu` 부터 뜬다(`Config/DefaultEngine.ini` 의 `EditorStartupMap` · `GameDefaultMap`).

## 에디터 열기

1. `Grimoire.uproject` 더블클릭. 처음엔 "모듈이 없거나 오래됐습니다. 다시 빌드할까요?" → **예**. C++ 모듈을 빌드하고 열린다.
2. 더블클릭이 안 되거나 엔진을 못 찾으면 아래 「.uproject 연결」을 한 번 한다.

### .uproject 연결 (이 PC 에서 한 번)

이 PC 는 런처의 설치 목록(`C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat`)이 비어 있어서, 언리얼 버전 선택기가 `"EngineAssociation": "5.8"` 을 찾지 못했다. 그래서 두 가지를 **내 계정(HKCU)** 에 직접 등록했다(관리자 권한 불필요, 2026-11-05).

- 엔진 위치: `HKCU\Software\Epic Games\Unreal Engine\Builds` 의 값 `5.8` = `C:/Program Files/Epic Games/UE_5.8`
- 파일 연결: `HKCU\Software\Classes\.uproject` → `Unreal.ProjectFile`. 우클릭 메뉴(열기 · 게임 실행 · Visual Studio 프로젝트 생성 · 엔진 버전 바꾸기)는 `C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe` 를 부른다.

> **함정 (2026-11-08)**: 11-05 에 이 등록을 **Claude 앱 안에서** 했더니, 값이 앱의 가상 레지스트리에만 들어갔다. 앱 안에서는 보이고 앱에서 띄운 에디터도 열리지만, 탐색기에서 더블클릭하면 「No Unreal Engine installations found」가 뜬다. 엔진 위치는 **앱 밖의 보통 PowerShell** 에서 등록한다:
>
> ```powershell
> New-Item -Path "HKCU:\Software\Epic Games\Unreal Engine\Builds" -Force | Out-Null; Set-ItemProperty -Path "HKCU:\Software\Epic Games\Unreal Engine\Builds" -Name "5.8" -Value "C:/Program Files/Epic Games/UE_5.8"
> ```
>
> 엔진을 못 찾는다는 창이 뜨면 「Yes」로 엔진 폴더를 고르지 않는다. 그러면 `Grimoire.uproject` 의 `EngineAssociation` 이 이 PC 전용 GUID 로 바뀌어 다른 PC 에서 안 열린다.

선택기의 `/fileassociations` 는 관리자 권한이 없으면 조용히 실패한다(종료 코드 1). Git Bash 에서 `reg add /v …` 를 부르면 `/v` 가 경로로 바뀌어 실패하므로 PowerShell 로 한다.

## 플레이(PIE) 단축키

- **플레이 종료: Shift+Esc** (보조 **Ctrl+Esc**). Esc 단독으로는 꺼지지 않는다 — 게임 안에서 Esc 는 메뉴가 쓴다. 에디터가 켜질 때 `Source/Grimoire/Core/GrimEditorKeyBindings.cpp` 가 걸어서 저장소를 받은 누구에게나 같다(ini 의 `ProjectDefinedChords` 는 JSON 값이 깨져 무시돼 쓰지 않는다). 에디터 환경설정에서 바꿔도 다음 실행 때 다시 이 키가 된다.
- 에디터 환경설정 → 키보드 단축키에서 직접 바꾼 값이 있으면 그쪽이 이긴다.

## Visual Studio 로 작업하기

`Grimoire.uproject` 우클릭 → **Generate Visual Studio project files** → `Grimoire.sln`. 구성 `Development Editor` / `Win64`, 시작 프로젝트 `Grimoire`, F5.

## 명령줄 빌드

PowerShell(프로젝트 폴더에서):

```powershell
$UE = "C:\Program Files\Epic Games\UE_5.8"
& "$UE\Engine\Build\BatchFiles\Build.bat" GrimoireEditor Win64 Development -Project="$PWD\Grimoire.uproject" -WaitMutex
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\Grimoire.uproject"
```

- `Build.bat` 을 거쳐야 한다. `UnrealBuildTool.exe` 를 직접 부르면 시스템 .NET(6.0)을 찾다가 실패한다(UBT 는 .NET 10 이 필요하고, `Build.bat` 이 엔진에 내장된 .NET 을 쓰도록 잡아 준다).
- Git Bash 에서 `Build.bat` 을 직접 부르면 공백 있는 경로 때문에 `'C:\Program' is not recognized` 가 난다. Git Bash 에서는 PowerShell 을 경유한다:

```bash
powershell -NoProfile -Command "& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' GrimoireEditor Win64 Development -Project='$(cygpath -w "$PWD")\Grimoire.uproject' -WaitMutex"
```

## 폴더 구조

```
Grimoire.uproject        프로젝트 파일 (엔진 5.8 연결, 켠 플러그인)
Source/Grimoire/         C++ 게임 모듈 — 시스템별 폴더 (docs/systems/index 「모듈 · 폴더 구성」)
Source/*.Target.cs       빌드 타깃 Grimoire(게임) · GrimoireEditor
Content/Grimoire/        우리 에셋 (Maps · Magic · MagicFX · Creatures · Environment · Terrain · Characters · Input · UI · Data · Art · Audio · VFX · Blueprints)
Content/Grimoire/Data/   런타임에 읽는 CSV 표 (아이템 · 반응 규칙 · 기후 · 레벨 · 퀘스트 · 외형 …)
Content/<팩>/            마켓 · 샘플 · 템플릿 에셋 (NiagaraExamples · Characters · Input)
Config/                  프로젝트 설정 ini (DefaultGame.ini 의 [/Script/Grimoire.Grim*Settings] 가 수치의 정본)
Scripts/<시스템>/         에셋 만드는 에디터 파이썬 · 헤드리스 확인 PowerShell (net_check*.ps1 등)
art/                     그림 · 3D 원본 작업 폴더와 도구(art/tools) — art/README.md
docs/                    이 문서 사이트 (VitePress)
Binaries/ Intermediate/  생성물 — git 에 넣지 않는다
Saved/ DerivedDataCache/
```

## 자주 겪는 문제

- **이상한 컴파일 · 링크 오류** → `Binaries/`, `Intermediate/` 삭제 후 다시 빌드.
- **LFS 파일이 포인터 텍스트로 보인다** → `git lfs pull`. 새 git 워크트리도 `.uasset` 이 포인터라 에셋을 읽는 테스트가 실패한다 → 그 워크트리에서 `git lfs checkout`.
- **빌드가 「Live Coding is active」 로 실패** → 에디터가 켜져 있다(다른 워크트리의 에디터도 뮤텍스를 같이 쓴다). 에디터를 닫거나 `Build.bat … -NoHotReloadFromIDE`.
- **헤드리스 실행 뒤 `Config/DefaultEngine.ini` 의 AndroidFileServer 섹션이 바뀜** → 에디터가 자동으로 쓴 것. 내 변경이 아니면 커밋 전에 되돌린다.
- **OneDrive 안에 두지 않는다** → 생성물 수만 개가 동기화되고 빌드 중 파일이 잠긴다. 지금 위치 `C:\Dev\Grimoire` 는 OneDrive 밖이다(아래 「프로젝트 경로」).

## 프로젝트 경로 — 영문 경로에 둔다

프로젝트는 **`C:\Dev\Grimoire`** 에 있다(2026-11-05 이동, 그 자리에서 `GrimoireEditor` 빌드 성공 확인). 처음 있던 `C:\Users\hej29\OneDrive\바탕 화면\01.게임개발\02.Grimoire` 같은 **한글 · 공백 · OneDrive 경로에서는 C++ 모듈 빌드가 실패한다**. 다른 PC 에서 받을 때도 영문 경로에 받는다.

- 원인: UBT 가 컴파일러에 넘기는 응답 파일(`Intermediate/**/*.rsp`)이 BOM 없는 UTF-8 인데, `cl.exe` 는 이를 시스템 코드페이지(CP949)로 읽는다. 그래서 `바탕 화면/01.게임개발` 이 `諛뷀깢 ?붾㈃/01.寃뚯엫媛쒕컻` 로 깨지고 `fatal error C1083: 컴파일러 중간 파일을 열 수 없습니다` 가 난다. 에디터에서 "모듈 다시 빌드"를 눌러도 같은 실패가 난다.
- 안 되는 우회: `C:\Dev\Grimoire` 정션(junction)으로 영문 경로를 만들어 빌드해도 UBT 가 정션을 실제 경로로 풀어 버려 같은 오류가 난다. UBA 를 끄는 것(`-NoUBA`)도 무관하다.

한글 경로에 둘 수밖에 없다면 선택지는 둘이다.

| 방법 | 내용 | 권장 |
|---|---|---|
| **A. 영문 경로로 옮기기** | 예: `C:\Dev\Grimoire`. 저장소가 GitHub 에 있으니 새 위치에 `git clone` 하거나 폴더째 옮기면 된다. OneDrive 동기화 문제도 함께 사라진다 | ✅ |
| B. Windows 를 UTF-8 로 | 설정 → 시간 및 언어 → 언어 및 지역 → 관리자용 언어 설정 → 시스템 로캘 변경 → 「Beta: 세계 언어 지원을 위해 Unicode UTF-8 사용」 체크 → 재부팅. 한글 경로를 유지할 수 있지만 CP949 를 가정하는 오래된 프로그램이 깨질 수 있다 | 차선 |

옮긴 뒤에는 `Binaries/` `Intermediate/` 를 지우고 다시 빌드한다(옛 경로가 캐시에 남아 있다).
