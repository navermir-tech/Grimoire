# 문서 사이트 · 배포

이 사이트는 저장소의 `docs/` 폴더에 있는 Markdown 을 [VitePress](https://vitepress.dev) 로 만든 것이다. 파일을 고치고 push 하면 Netlify 가 다시 빌드해 배포한다. 아티팩트 같은 외부 저장소에 의존하지 않고, **문서 원본은 늘 git 안에** 있다.

## 로컬에서 실시간으로 보기

프로젝트 루트에서(처음 한 번 `npm install`):

```bash
npm install
npm run docs:dev
```

`http://localhost:5173` 이 열리고, `docs/**/*.md` 를 저장하면 **즉시** 화면이 바뀐다(핫 리로드).

| 명령 | 뜻 |
|---|---|
| `npm run docs:dev` | 개발 서버(핫 리로드) |
| `npm run docs:build` | 정적 사이트 빌드 → `docs/.vitepress/dist/`. **깨진 링크가 있으면 실패한다** — 커밋 전에 한 번 돌린다 |
| `npm run docs:preview` | 빌드 결과 미리보기 |

## 문서 추가하기

1. `docs/spec/` 또는 `docs/systems/` 에 `.md` 파일을 만든다. 첫 줄은 `# 제목`.
2. `docs/.vitepress/config.mts` 의 `sidebar` 에 한 줄 더한다.
3. 다른 문서로의 링크는 상대 경로 + 확장자 없이: `[마법](../spec/magic)`.
4. 그림은 `docs/public/` 에 두고 `/그림.png` 로 참조한다. `docs/` 아래 그림은 LFS 가 아니라 일반 git 으로 올라간다(Netlify 빌드가 LFS 파일을 받지 않기 때문 — `.gitattributes` 참고). 큰 그림은 줄여서 넣는다.

## Netlify 배포

설정은 저장소 루트의 `netlify.toml` 에 있다(빌드 명령 · 배포 폴더 · Node 버전). 사이트를 처음 연결할 때만 아래를 한 번 한다:

1. [app.netlify.com](https://app.netlify.com) 에 GitHub 계정으로 로그인한다.
2. **Add new project → Import an existing project → GitHub** → `navermir-tech/Grimoire` 를 고른다.
3. 빌드 설정은 `netlify.toml` 에서 자동으로 읽힌다(Build command `npm run docs:build`, Publish directory `docs/.vitepress/dist`). 그대로 **Deploy** 를 누른다.
4. 1~2분 뒤 `https://<사이트이름>.netlify.app` 이 생긴다. Project configuration → Change project name 에서 `grimoire-docs` 같은 이름으로 바꿀 수 있다.

이후로는 `main` 에 push 할 때마다 자동 배포된다. `docs/` · `package.json` · `netlify.toml` 이 바뀌지 않은 커밋(언리얼 에셋만 바뀐 커밋)은 빌드를 건너뛴다(`netlify.toml` 의 `ignore`).

### 확인 · 문제

- Netlify 대시보드 → Deploys 에서 로그를 본다. 로컬에서 `npm run docs:build` 가 되면 Netlify 에서도 된다.
- 비공개로 보고 싶으면 Netlify 의 Site protection(비밀번호, 유료)을 켠다. 저장소를 비공개로 바꿔도 Netlify 는 계속 빌드한다.
