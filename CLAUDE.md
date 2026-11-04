# CLAUDE.md

모든 세션에 필요한 것만 둔다. 기획은 `docs/GDD.md`, 사람용 실행 절차는 `README.md` 다.

## 프로젝트

- 「그리모리 (Grimoire)」는 마법세계 게임이다. **Unreal Engine 5.8** · C++ 모듈 `Grimoire` + 블루프린트. 저장소 `github.com/navermir-tech/Grimoire`, 기본 브랜치 `main`.
- 엔진은 `C:\Program Files\Epic Games\UE_5.8`. 프로젝트 경로에 한글 · 공백이 있으므로(OneDrive 바탕 화면) 경로는 늘 따옴표로 감싼다.

## 자주 쓰는 명령 (Git Bash)

```bash
UE="/c/Program Files/Epic Games/UE_5.8"
P="$(cygpath -w "$PWD")\Grimoire.uproject"
"$UE/Engine/Build/BatchFiles/Build.bat" GrimoireEditor Win64 Development -Project="$P" -WaitMutex   # C++ 바꾼 뒤 에디터 모듈 빌드
"$UE/Engine/Build/BatchFiles/Build.bat" -projectfiles -project="$P" -game -engine                  # Grimoire.sln 다시 만들기
"$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$P"                                                  # 에디터 열기
"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$P" -run=CompileAllBlueprints -nop4 -nosplash -unattended   # 블루프린트 일괄 컴파일 점검
```

- 빌드는 2분을 넘기므로 백그라운드로 돌리고 끝나면 로그 끝을 본다. 한글 경로가 깨지면 PowerShell 도구로 같은 명령을 돌린다.

## 구조

- `Source/Grimoire/` 가 유일한 게임 모듈이다. 모듈 진입은 `Grimoire.cpp`, 공용 로그 카테고리는 `LogGrimoire`.
- `Content/Grimoire/` 아래에만 우리 에셋을 둔다(Maps · Blueprints · Characters · Magic · UI · Art · Audio · VFX). 마켓 · 샘플 에셋은 `Content/<팩 이름>/` 으로 따로 둔다.
- `Config/Default*.ini` 가 프로젝트 설정의 정본이다. 에디터의 Project Settings 가 이 파일을 고친다. 수치는 ini 에 적고 코드에 하드코딩하지 않는다.

## 규칙과 함정

- 식별자는 영어, 주석 · 문서 · 커밋 메시지 · 게임 문구는 한국어. C++ 파일은 UTF-8 로 저장한다. UBT 가 컴파일러에 /utf-8 을 주므로 BOM 은 없어도 된다.
- 언리얼 명명 규칙: 클래스 접두 `A`(액터) `U`(오브젝트) `F`(구조체) `E`(열거) `I`(인터페이스). 에셋 접두 `BP_` `WBP_` `M_` `MI_` `T_` `SM_` `SK_` `ABP_` `AM_` `NS_` `DA_` `DT_` `IA_` `IMC_` `L_`.
- `.uasset` · `.umap` 은 바이너리라 텍스트로 읽거나 고칠 수 없다. 로직은 가능하면 C++ 로 짜고, 블루프린트 작업은 사용자가 에디터에서 한다. 블루프린트 변경이 필요하면 무엇을 어디서 바꿀지 순서로 적어 준다.
- C++ 헤더를 바꾸면(UPROPERTY · UFUNCTION · 새 클래스) 라이브 코딩이 아니라 **에디터를 닫고** 빌드한다. 라이브 코딩은 .cpp 본문 수정에만 쓴다.
- 생성물(`Binaries` `Intermediate` `Saved` `DerivedDataCache`)은 지워도 된다. 이상한 빌드 오류는 `Intermediate` 와 `Binaries` 를 지우고 다시 빌드하면 대개 사라진다.
- 바이너리 에셋은 `.gitattributes` 가 LFS 로 보낸다. 새 바이너리 확장자를 쓰면 거기에 한 줄 더하고 나서 커밋한다. `git add -A` 전에 `git status` 로 생성물이 섞이지 않았는지 본다.
- 커밋은 사용자가 시키면 한다. 메시지는 한국어 한 줄 + 빈 줄 + 필요하면 설명.

## 문서 갱신 규칙

1. CLAUDE.md 에는 모든 세션에 필요한 것(명령 · 구조 · 전역 규칙)만 둔다. 기능 설명 · 수치 · 날짜는 넣지 않는다.
2. 설계 결정은 `docs/GDD.md` 의 해당 절을 고치고 「결정 기록」에 날짜 · 결정 · 까닭을 한 줄 더한다.
3. 기능 · 모듈별 설명과 함정이 늘어나면 `.claude/rules/<주제>.md` 로 나누고 여기에는 이름만 적는다.
4. CLAUDE.md 는 80줄 이하로 유지한다.
