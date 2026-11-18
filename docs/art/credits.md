# 크레딧 — 출처 표기

게임에 실제로 들어가는 **CC BY 4.0** 에셋의 목록이다. CC BY 는 저작자 · 제목 · 라이선스 링크를 적고, 원본을 고쳤으면 고쳤다고 밝혀야 한다.

- **정본은 데이터 파일 `Content/Grimoire/Data/Credits.json`** 이다([Q137](../spec/decisions#미결-질문) 답 2026-11-18). 같은 파일을 세 곳이 쓴다.
  - 게임 안: 메인 메뉴 · Esc 메뉴의 「설정」 → 「크레딧 확인」 탭([UI — 크레딧 탭](../systems/world-zones#크레딧-확인-탭-q137)).
  - 이 페이지 아래 표와 표기 문구 — `node Scripts/Docs/credits.mjs --write` 가 만든다. 손으로 고치지 않는다(`npm run docs:check` 가 어긋나면 실패).
  - 출시본 폴더의 `CREDITS.txt` — `node Scripts/Docs/credits.mjs --txt <출시본 폴더>/CREDITS.txt`.
- **쓰기 시작할 때 한 줄 더한다.** 받아만 두고 게임에 넣지 않은 팩은 적지 않는다(`docs/dev/market-assets.md` 「받은 팩」 표에 「—」로 남는다).
- 작가 · 라이선스는 **Fab 목록 페이지에서 직접 본 것**을 적고, 언제 봤는지 남긴다(`Checked`). 후보를 고르던 때의 표([Fab 무료 에셋 후보](./fab-free-assets))는 정본이 아니다.
- Fab Standard License · 옛 UE 마켓플레이스 라이선스 에셋은 표기 의무가 없어 여기에 적지 않는다.

## 한 줄 더하는 법

1. `Content/Grimoire/Data/Credits.json` 의 `Attributions` 에 객체 하나를 더한다 — `Author` · `Asset` · `Store` · `AssetUrl` · `License` · `LicenseUrl` 과 플레이어에게 보이는 「고친 점」 `ModifiedKo` · `ModifiedEn`, 문서 표에만 쓰는 `DocUsedFor` · `DocChanges`(마크다운), 확인 기록 `Checked`.
2. `node Scripts/Docs/credits.mjs --write` 로 아래 표를 다시 쓴다.
3. 자동화 테스트 `Grimoire.UI.Credits.*` 와 `npm run docs:check` 로 확인한다.

<!-- credits:begin — 생성물: Content/Grimoire/Data/Credits.json 을 고치고 node Scripts/Docs/credits.mjs --write -->

## 쓰고 있는 CC BY 에셋

| 저작자 | 에셋 | 라이선스 | 어디에 쓰나 | 고친 점 | 확인 |
|---|---|---|---|---|---|
| tharlevfx | [Water Materials](https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861) (Fab) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | 마탑 구간 개울 2 줄(`Creek_TowerBrook` · `Creek_Pasture`)과 합류 연못의 물 표면 — 재질 `M_River` 와 그것이 쓰는 텍스처 · 재질 함수 | 원본 파일은 그대로 두고, 파생 인스턴스 `MI_Dress_Water`(`Content/Grimoire/Terrain/Dress/`)에서 물 색(회색 → 숲 개울 청록) · 물결 크기와 속도와 세기 · 물가 투명도를 바꾸고, 파도(정점 이동)를 끄고, 양면으로 그린다. 값은 `Scripts/Terrain/dress_assets.py` 의 `WATER_SCALARS` · `WATER_COLORS` | 2026-11-09 — Fab 목록에서 저작자 tharlevfx · 라이선스 「Creative Commons Attribution(CC BY 4.0)」 확인 |

## 표기 문구

게임 안 크레딧 탭 · 출시본 `CREDITS.txt` 에 그대로 들어가는 문구:

> Water Materials by tharlevfx (https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861), licensed under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Colors, wave scale, speed and strength and opacity were modified; vertex waves were disabled.
>
> tharlevfx 의 Water Materials — CC BY 4.0. 색 · 물결 크기와 속도와 세기 · 투명도를 고치고 정점 파도를 껐다.

## 엔진 표기

언리얼 엔진 EULA 가 크레딧에 넣으라는 문구(게임 안 탭 · `CREDITS.txt` 끝에 들어간다):

- Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
- Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.

<!-- credits:end -->
