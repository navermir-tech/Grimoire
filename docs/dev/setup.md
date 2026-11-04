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

Git LFS 가 설치돼 있으면 `.uasset` 등 바이너리 에셋도 함께 내려온다.

## 에디터 열기

1. `Grimoire.uproject` 더블클릭. 처음엔 "모듈이 없거나 오래됐습니다. 다시 빌드할까요?" → **예**. C++ 모듈을 빌드하고 열린다.
2. 더블클릭이 엔진을 못 찾으면 Epic Games Launcher → 라이브러리 → UE 5.8 **실행** → "프로젝트 찾아보기"로 `Grimoire.uproject` 선택.

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
Grimoire.uproject        프로젝트 파일 (엔진 5.8 연결)
Source/Grimoire/         C++ 게임 모듈
Content/Grimoire/        우리 에셋 (Maps · Blueprints · Characters · Magic · UI · Art · Audio · VFX)
Config/                  프로젝트 설정 ini
docs/                    이 문서 사이트 (VitePress)
Binaries/ Intermediate/  생성물 — git 에 넣지 않는다
Saved/ DerivedDataCache/
```

## 자주 겪는 문제

- **이상한 컴파일 · 링크 오류** → `Binaries/`, `Intermediate/` 삭제 후 다시 빌드.
- **LFS 파일이 포인터 텍스트로 보인다** → `git lfs pull`.
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
