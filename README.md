# 그리모리 (Grimoire)

마법세계를 무대로 한 게임. **Unreal Engine 5.8 · C++ + 블루프린트**로 만든다.

- 저장소: https://github.com/navermir-tech/Grimoire
- 기획서: [docs/GDD.md](docs/GDD.md)
- AI 세션(Claude Code) 작업 규칙: [CLAUDE.md](CLAUDE.md)

## 요구 사항

| 항목 | 버전 · 메모 |
|---|---|
| Unreal Engine | **5.8** — Epic Games Launcher 로 설치 |
| Visual Studio 2022 | 「C++를 사용한 게임 개발」 워크로드 + 「Unreal Engine 설치 관리자」 구성 요소 |
| Git · Git LFS | Git 2.4x 이상, Git LFS 3.x (`git lfs install` 을 한 번 해 둔다) |

## 시작하기

1. **저장소 받기**
   ```bash
   git clone https://github.com/navermir-tech/Grimoire.git
   ```
   Git LFS 가 설치돼 있으면 `.uasset` 같은 바이너리 에셋도 함께 내려온다.
2. **에디터 열기** — `Grimoire.uproject` 를 더블클릭한다. 처음에는 "모듈이 없거나 오래됐습니다. 다시 빌드할까요?" 라고 묻는데 **예**를 누르면 C++ 모듈을 빌드하고 열린다.
3. **Visual Studio 로 작업하기(선택)** — `Grimoire.uproject` 우클릭 → **Generate Visual Studio project files** → `Grimoire.sln` 을 연다. 구성은 `Development Editor` / `Win64`, 시작 프로젝트는 `Grimoire`.
4. **명령줄 빌드(선택)** — 프로젝트 폴더에서:
   ```bat
   set UE=C:\Program Files\Epic Games\UE_5.8
   "%UE%\Engine\Build\BatchFiles\Build.bat" GrimoireEditor Win64 Development -Project="%CD%\Grimoire.uproject" -WaitMutex
   "%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\Grimoire.uproject"
   ```

## 폴더 구조

```
Grimoire.uproject            프로젝트 파일 (엔진 5.8 연결)
Source/Grimoire/             C++ 게임 모듈 — 게임 모드 · 시스템 · 로직
Content/Grimoire/            우리가 만드는 에셋 (Maps · Blueprints · Characters · Magic · UI · Art · Audio · VFX)
Config/                      프로젝트 설정 ini (Engine · Game · Editor · Input)
docs/                        기획서 · 결정 기록 · 레퍼런스
Binaries/ Intermediate/      생성물 — git 에 넣지 않는다. 지워도 다시 만들어진다.
Saved/ DerivedDataCache/
```

## 저장소 규칙

- `.uasset` · `.umap` 과 그림 · 소리 · 모델 원본은 **Git LFS** 로 올라간다(`.gitattributes`). 새 확장자를 쓰게 되면 거기에 한 줄 더한다.
- 생성물 폴더(`Binaries` `Intermediate` `Saved` `DerivedDataCache`)와 `.sln` 은 커밋하지 않는다(`.gitignore`).
- 브랜치: `main` 은 늘 열리고 빌드되는 상태로 두고, 작업은 `feature/<이름>` 에서 한 뒤 합친다.
- 커밋 메시지는 한국어 한 줄로 "무엇을 했는지" 를 적는다.
- 바이너리 에셋(블루프린트 · 맵)은 둘이 동시에 고치면 합칠 수 없다. 같은 에셋을 만지기 전에 서로 말한다.
