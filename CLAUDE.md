# CLAUDE.md

모든 세션에 필요한 것만 둔다. 기획(무엇)은 `docs/spec/`, 기술 설계(어떻게)는 `docs/systems/`, 사람용 절차는 `docs/dev/` 다. 문서 사이트는 VitePress(`npm run docs:dev`)로 보고 Netlify 가 배포한다.

## 프로젝트

- 「그리모리 (Grimoire)」: 마법세계 오픈월드 생존 게임. **Unreal Engine 5.8** · C++ 모듈 `Grimoire` + 블루프린트. 저장소 `github.com/navermir-tech/Grimoire`, 기본 브랜치 `main`.
- 엔진 `C:\Program Files\Epic Games\UE_5.8`. 요구사항 원문은 `docs/spec/index.md` 의 R01~R16, 미결은 `docs/spec/decisions.md` 의 Q 번호로 가리킨다.
- **경로 함정**: 현재 폴더(OneDrive · 한글 · 공백)에서는 C++ 빌드가 실패한다(cl.exe 가 응답 파일을 CP949 로 읽음 — `docs/dev/setup.md` 「프로젝트 경로」). 영문 경로로 옮기기 전에는 C++ 변경을 빌드로 검증할 수 없다. 정션 우회는 안 된다.

## 자주 쓰는 명령

```bash
npm run docs:dev        # 문서 사이트 핫 리로드(localhost:5173). preview 는 .claude/launch.json 의 "docs"
npm run docs:build      # 깨진 링크 검사 포함 — 문서 커밋 전에 돌린다
```

```powershell
# 언리얼 빌드 · 실행 (PowerShell. Git Bash 에서 Build.bat 을 직접 부르면 공백 경로에서 깨진다)
$UE = "C:\Program Files\Epic Games\UE_5.8"
& "$UE\Engine\Build\BatchFiles\Build.bat" GrimoireEditor Win64 Development -Project="$PWD\Grimoire.uproject" -WaitMutex
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\Grimoire.uproject"
```

- 빌드는 2분을 넘기므로 백그라운드로 돌리고 `Result:` 와 `exit=` 줄을 본다. 파이프 뒤의 종료 코드에 속지 않는다(`$LASTEXITCODE` · `PIPESTATUS`).
- `UnrealBuildTool.exe` 직접 호출 금지(.NET 10 필요 — `Build.bat` 이 엔진 내장 .NET 을 잡아 준다).
- `npx vitepress --version` 은 버전 대신 개발 서버를 띄운다. 쓰지 않는다.

## 구조

- `Source/Grimoire/` 단일 게임 모듈. 시스템별 하위 폴더(Core · Magic · Survival · Environment · Creatures · Base · Factions · World · Save · UI)는 `docs/systems/index.md` 대로. 클래스 접두 `Grim`.
- `Content/Grimoire/<시스템>/` 에만 우리 에셋. 마켓 에셋은 `Content/<팩>/`.
- `Config/Default*.ini` 가 설정의 정본. 수치는 데이터(DataAsset · DataTable · ini)에 두고 코드에 적지 않는다.
- 문서: 새 페이지는 `docs/.vitepress/config.mts` 의 sidebar 에 등록한다. 링크는 확장자 없는 상대 경로. `docs/` 아래 그림은 LFS 가 아니다.

## 규칙과 함정

- 식별자는 영어, 주석 · 문서 · 커밋 · 게임 문구는 한국어. C++ 는 UTF-8(UBT 가 `/utf-8` 을 준다).
- 서버 권위 · GAS · 데이터 주도 · 속성 상태 단일 시스템 — `docs/systems/index.md` 「설계 원칙」을 코드보다 먼저 읽는다.
- 명명 규칙(에셋 접두 · GameplayTag)은 `docs/dev/conventions.md`.
- `.uasset` · `.umap` 은 바이너리 — 텍스트로 읽거나 고치지 않는다. 블루프린트 변경은 사용자가 에디터에서 하고, 로직은 가능하면 C++ 로 짠다.
- 헤더(UPROPERTY · UFUNCTION · 새 클래스) 변경은 에디터를 닫고 빌드한다. 라이브 코딩은 .cpp 본문만.
- 생성물(Binaries · Intermediate · Saved · DerivedDataCache)은 지워도 된다. 커밋 전 `git status` 로 섞이지 않았는지 본다.
- 스펙 · 설계가 바뀌면 문서를 먼저 고치고 `docs/spec/decisions.md` 에 한 줄 남긴다. 코드가 문서와 다르면 문서를 코드에 맞춘다.
- 이 머신의 Bash 도구는 히어독 여러 개가 든 250줄 넘는 명령에서 따옴표 파싱 오류를 낸다 → 큰 파일은 Write 도구나 여러 호출로 만든다.
- 커밋은 사용자가 시키면 한다. 메시지는 한국어 한 줄 + 필요하면 설명.

## 문서 갱신 규칙

1. CLAUDE.md 에는 모든 세션에 필요한 것(명령 · 구조 · 전역 규칙 · 함정)만 둔다. 기능 설명 · 수치 · 날짜는 `docs/` 로.
2. 주제별 함정이 늘면 `.claude/rules/<주제>.md` 로 나누고 여기엔 이름만 적는다.
3. 80줄 이하로 유지한다.
