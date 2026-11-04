# 그리모리 (Grimoire)

마법세계를 무대로 한 오픈월드 생존 게임. **Unreal Engine 5.8 · C++ + 블루프린트**.

- **문서 사이트** — 스펙(기획) · 시스템(기술 설계) · 개발 가이드. `docs/` 의 Markdown 을 VitePress 로 만들고 Netlify 가 배포한다. 로컬에서 보려면 `npm install` 뒤 `npm run docs:dev` → http://localhost:5173
- **저장소** — https://github.com/navermir-tech/Grimoire

## 빠른 시작

1. 요구 사항: Unreal Engine 5.8, Visual Studio 2022(C++ 게임 개발 워크로드), Git + Git LFS.
2. `git clone https://github.com/navermir-tech/Grimoire.git`
3. `Grimoire.uproject` 를 열고 모듈 빌드에 **예**.

자세한 절차 · 명령줄 빌드 · 문제 해결은 [docs/dev/setup.md](docs/dev/setup.md).

## 폴더

| 경로 | 내용 |
|---|---|
| `Grimoire.uproject`, `Source/`, `Config/`, `Content/` | 언리얼 프로젝트 |
| `docs/` | 문서 사이트 원본 — `docs/spec` 스펙 · `docs/systems` 시스템 · `docs/dev` 개발 |
| `package.json`, `netlify.toml` | 문서 사이트 빌드 · 배포 설정(게임 빌드와 무관) |
| `CLAUDE.md` | AI 세션(Claude Code) 작업 규칙 |

## 저장소 규칙 (요약)

- 바이너리 에셋은 Git LFS. 생성물(`Binaries` `Intermediate` `Saved` `DerivedDataCache`)은 커밋하지 않는다.
- `main` 은 늘 열리는 상태로 두고, 작업은 `feature/*` 에서. 커밋 메시지는 한국어 한 줄.
- 전체 규칙: [docs/dev/conventions.md](docs/dev/conventions.md)
