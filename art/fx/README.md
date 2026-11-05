# 마법 이펙트 그림 (볼트)

설계 · 에셋 목록은 [투사체 · 시전 감각 — 볼트 이펙트](../../docs/systems/spell-feel.md). 여기는 그림 작업 절차만.

- `refs/` — 사용자 참고 그림(2026-11-06). 왼쪽 검은 구 = 완드 끝, 오른쪽 구 = 표적, 줄마다 한 볼트의 여러 순간.
- `prompts/` — GPT(codex) 프롬프트. `sheet_<속성>` 속성별 시안(시전 · 비행 · 적중 세 줄), `layer_*` 레이어 분해(검은 바탕), `tex_*` 텍스처 한 장씩(검은 바탕 · 회색조).
- `concepts/` — 시안 · 레이어 분해를 줄여 담은 jpg(원본 `raw/` 는 커밋하지 않는다).
- `bolt/` — 게임 텍스처 원본(`T_Bolt_*`, 밝기 = 알파). `compare/` — 시안 대 게임 비교.

## 순서

1. 시안: `bash art/tools/run_codex_job.sh <절대경로>/art/fx sheet_fire <refs/user-ref-bolt-1.png,…>` (참고 그림 4장을 `-i` 로)
2. 텍스처: `tex_*` 를 같은 식으로(참고 2장) → `python Scripts/MagicFX/make_bolt_textures.py` → `bolt/T_Bolt_*.png`
   - 빛줄기 · 불티 · 실안개는 90° 돌려 저장한다 — Niagara 속도 정렬 스프라이트는 텍스처 「위」가 진행 방향.
   - 번쩍임 · 빛살은 GPT 그림이 없거나 깨지면 절차 생성 그림으로 대신한다.
3. 언리얼(에디터를 닫고): `make_bolt_fx.py` → `make_magicfx_assets.py`(`GRIM_FX_STEPS=visual_sets`) — 커맨드렛 명령은 각 스크립트 머리말.
4. 확인: `powershell -File Scripts/MagicFX/capture_bolt.ps1 -Spell Ember -Slomo 0.01 -Shots 26 -Start 20.6 -Interval 0.25 -Exposure -2.5 -Tag _dark`
   → `python Scripts/MagicFX/compare_bolt.py art/fx/raw/sheet_fire.png Saved/BoltCapture/Ember_side_dark 1 5 14 art/fx/compare/ember.jpg "제목"`

## 함정

- codex 가 결과 자리에 **폴더**나 빈 파일을 남길 때가 있다(2026-11-06 바람 시안 · 번쩍임). 로그의 `session id` 로 `~/.codex/generated_images/<세션>/exec-*.png` 를 찾아 눈으로 보고 옮긴다.
- 연출 묶음이 가리키는 Niagara 에셋을 `delete_asset` 하면 커맨드렛이 죽는다 — `make_bolt_fx.py` 는 지우지 않고 안을 비워 다시 채운다.
- `SpawnSystemAtLocation` 으로 만든 컴포넌트는 절대 위치라 붙여도 따라오지 않는다 — 투사체 꼬리는 `SpawnSystemAttached`.
- 실제 시전은 투사체를 7.5 m 앞당겨 만들어 L_Tower 표적(4 m)은 비행 없이 터진다 — 비행은 `Grim.FX.Bolt` 로 찍는다.
