# 문서 사이트 · 배포

이 사이트는 저장소의 `docs/` 폴더에 있는 Markdown 을 [VitePress](https://vitepress.dev) 로 만든 것이다. 파일을 고치고 push 하면 GitHub Actions 가 다시 빌드해 GitHub Pages(https://navermir-tech.github.io/Grimoire/)에 배포한다. 아티팩트 같은 외부 저장소에 의존하지 않고, **문서 원본은 늘 git 안에** 있다.

## 로컬에서 실시간으로 보기

프로젝트 루트에서(처음 한 번 `npm install`):

```bash
npm install
npm run docs:dev
```

`http://localhost:5173/Grimoire/` 가 열리고, `docs/**/*.md` 를 저장하면 **즉시** 화면이 바뀐다(핫 리로드).

| 명령 | 뜻 |
|---|---|
| `npm run docs:dev` | 개발 서버(핫 리로드) |
| `npm run docs:build` | 정적 사이트 빌드 → `docs/.vitepress/dist/`. **깨진 링크가 있으면 실패한다** — 커밋 전에 한 번 돌린다 |
| `npm run docs:preview` | 빌드 결과 미리보기 |

## 문서 추가하기

1. `docs/spec/` 또는 `docs/systems/` 에 `.md` 파일을 만든다. 첫 줄은 `# 제목`.
2. `docs/.vitepress/config.mts` 의 `sidebar` 에 한 줄 더한다.
3. 다른 문서로의 링크는 상대 경로 + 확장자 없이: `[마법](../spec/magic)`.
4. 그림은 `docs/public/` 에 두고 `/그림.png` 로 참조한다(마크다운 `![](/art/...)` · 링크에는 VitePress 가 사이트 base `/Grimoire/` 를 붙여 준다. 마크다운 안에 raw HTML `<img src="/...">` 를 직접 쓰면 base 가 안 붙으니 마크다운 문법이나 상대 경로를 쓴다). `docs/` 아래 그림은 LFS 가 아니라 일반 git 으로 올라간다(배포 워크플로가 LFS 파일을 받지 않기 때문 — `.gitattributes` 참고). 큰 그림은 줄여서 넣는다.

## GitHub Pages 배포

설정은 `.github/workflows/docs.yml` 에 있다. `main` 에 push 하면 `docs/**` · `package.json` · `package-lock.json` · 워크플로 파일이 바뀐 커밋만 빌드한다(언리얼 에셋만 바뀐 커밋은 건너뜀). Node 22 · `npm ci` · `npm run docs:build` → `docs/.vitepress/dist` 를 Pages 에 올린다. 체크아웃은 LFS 를 받지 않는다.

- 주소: https://navermir-tech.github.io/Grimoire/ — 프로젝트 사이트라 `config.mts` 에 `base: '/Grimoire/'` 가 있다. 로컬 `docs:dev` 도 `http://localhost:5173/Grimoire/` 로 열린다.
- 저장소 설정 Settings → Pages → Build and deployment → Source 가 **GitHub Actions** 여야 한다(처음 한 번).
- 손으로 다시 돌리기: Actions → docs → Run workflow, 또는 `gh workflow run docs.yml`.

### 확인 · 문제

- Actions 탭의 docs 실행 로그를 본다. 로컬에서 `npm run docs:build` 가 되면 워크플로에서도 된다.
- **파일 300개가 넘게 바뀐 푸시**(맵 다시 만들기 · 에셋 합치기)는 GitHub 이 경로 필터를 처음 300개로만 보므로 docs 가 안 돌 수 있다. 그때는 `gh workflow run docs.yml --ref main` 으로 손으로 돌린다.
- 저장소가 공개라 무료 · 횟수 제한 없음. 저장소를 비공개로 바꾸면 무료 플랜에서는 Pages 가 꺼진다.
- 예전 Netlify(grimoire2026.netlify.app)는 무료 크레딧 소진으로 2026-11-05 18:10 이후 배포가 멈춰 그만 썼다([결정 기록](../spec/decisions#결정-기록)).
