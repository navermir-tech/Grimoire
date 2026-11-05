# 그림 작업 폴더

- `raw/` 는 codex 원본(커밋하지 않음), `logs/` 도 커밋하지 않는다.
- 다듬은 결과만 `docs/public/art/` 로 옮겨 문서에 쓴다.

## 함정

- codex 가 그림은 만들고 `raw/<작업>.png` 로 옮기지 못할 때가 있다(윈도 명령 길이 제한 — 로그에 「Windows error 206」). 그러면 결과가 `MISSING` 이다. 그림은 `~/.codex/generated_images/<세션>/exec-*.png` 에 남아 있으니 그 작업 시각의 마지막 파일을 복사한다.
- **여러 작업을 동시에 돌리면 결과가 섞일 수 있다**(2026-11-05 분탑 4장: G 자리에 F 와 같은 숲 서원 그림이 들어옴 — 각 세션이 「최근 생성 그림」을 복사하다 다른 세션 것을 집은 것으로 보임). 동시에 돌렸으면 결과를 꼭 눈으로 보고, 틀린 것만 혼자 다시 돌린다.
- 작업 폴더는 절대 경로로 넘긴다. 셸 작업 폴더가 바뀐 상태에서 상대 경로로 부르면 스크립트를 못 찾는다.

## 환경 에셋 만들기 (GPT → Tripo → 블렌더 → 언리얼)

방향은 [아트 방향](../docs/spec/art-direction.md). 에셋 하나는 `art/env/<에셋>/` 에 모은다. 첫 결과는 초원 바위 A(2026-11-06).

1. **GPT 그림** — `art/env/prompts/<에셋>.txt`(물체 하나 · 연회색 단색 배경 · 고른 빛 · 그림자 없음) → `bash art/tools/run_codex_job.sh <절대경로>/art/env <에셋>`
2. **4면 시트** — 1의 그림을 참고(`-i`)로 2×2 시트(왼쪽 위 정면 · 오른쪽 위 왼쪽 · 왼쪽 아래 뒤 · 오른쪽 아래 오른쪽, 눈높이 · 거의 정투영). 따로 그린 4장은 디테일이 어긋난다. Tripo 멀티뷰는 4면까지(위 · 아래는 받지 않는다).
3. **자르기** — `python art/tools/tripo.py sheet …` 가 자르고 바로 생성까지 한다(API). 웹으로 할 때는 자르기만: `python -c "import sys; sys.path.insert(0,'art/tools'); import tripo; tripo.cut_sheet('<시트>', 'art/env/<에셋>/views')"`. 칸 경계가 아니라 줄 전체에서 물체를 찾아 잘라 이웃 물체가 끼지 않고, 네 장 크기 · 바닥선을 맞춘다.
4. **Tripo** — API: `python art/tools/tripo.py sheet <시트> art/env/<에셋>/tripo`(키는 `~/.tripo/api_key`, API 크레딧은 웹과 따로). 웹(Tripo Studio): 3D 작업 공간 → 멀티뷰 → 앞 · 왼쪽 · 오른쪽 · 뒷면에 넣고, 지오메트리 및 텍스처 = 울트라 메시 · 4K · 조명 제거 · PBR · 삼각형, 폴리곤 수는 크기에 맞게(바위 50만) → 생성(55 크레딧) → 내보내기 GLB 4K(5 크레딧) → `art/env/<에셋>/tripo/` 로 옮기고 `task.json` 에 작업 번호 · 설정을 적는다.
5. **블렌더 정리** — `"C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --factory-startup -P art/tools/blender_cleanup.py -- <glb> art/env/<에셋> SM_<이름> <높이cm> [--sink 0.08]` → `.glb` · `.blend` · `_preview.png`
6. **언리얼** — `$env:GRIM_ENV_GLB=<정리된 glb>; $env:GRIM_ENV_DEST="<종류>/SM_<이름>"` 후 `Scripts/Environment/import_env_asset.py` 를 커맨드렛으로 → `/Game/Grimoire/Environment/<종류>/SM_<이름>/`(Nanite 켬)

- 블렌더 스크립트에 상대 경로를 주면 출력이 .blend 기준으로 바뀐다 — 스크립트가 절대 경로로 바꾼다.
- `.blend1`(블렌더 백업)은 커밋하지 않는다.

## 마법 이펙트 (GPT 시안 → 스크립트 조립)

`art/fx/` — 시안 프롬프트 `prompts/<마법>.txt`(4칸: 시전 · 몸통 · 꼬리 · 터짐, 어두운 배경), 절차적 텍스처 `textures/`(`Scripts/MagicFX/make_fx_textures.py`), 게임 캡처 `ingame/`(커밋하지 않음). 시안 · 비교 그림은 `docs/public/art/fx/`. 절차는 [마법 이펙트 자체 제작](../docs/systems/magic-fx-authoring.md).
