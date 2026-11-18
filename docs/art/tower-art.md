# 마법탑 구역 아트 시범 — 자체 제작 화풍

> 상태: **시범** · 2026-11-18 · [Q96](../spec/decisions#미결-질문) 사용자 요청: 「지금 레벨 들어간 에셋 아트풍이 맘에 안 든다 — 내가 예전에 하고 싶었던 아트풍으로 마법탑 실외 / 실내 / 앞마당까지만 만들어 배치해 볼까?」
>
> 목표는 사용자가 에디터에서 보고 **「이 화풍으로 계속 갈지」** 판단하는 것. 화풍은 [아트 방향](../spec/art-direction)(Painterly High-Fidelity Stylized Realism), 정본 콘셉트는 [1번 지역 콘셉트](./region1-concepts)의 `tower-exterior`(Q95) · `tower-dusk` · `tower-yard` · `tower-in-*`, 모델 목록은 [필요한 3D 모델](./region1-assets) 「마법탑」 · 「탑 실내 키트」.

## 무엇을 바꿨나

L_OpenBeta 의 **탑 구역**(탑 1층 · 둥근 기단 · 탑 몸 · 현관 · 앞마당 평지 반경 약 45 m · 앞마당에서 큰길로 이어지는 석등 줄 · 앞마당 POI 「꺼진 화로 제단」 · 「길손의 돌무더기」)만 바꿨다. 다른 곳은 그대로다.

| 전(그레이박스 · 팩) | 후(자체 제작) |
|---|---|
| 탑: 엔진 기본 도형(네모 방 + 원통 + 원뿔) | 블렌더 모듈 키트 — 둥근 마름돌 기단 · 가는 탑 몸(층 띠 · 아치 창 · 둥근 창 · 발코니) · 놋쇠 관측실 · 남색 슬레이트 원뿔 지붕 · 놋쇠 혼천의 · 동쪽 현관(뾰족 아치 · 열린 쌍여닫이 문 · 박공 지붕) · 배너 |
| 1층 방: 회색 상자 벽 · 바닥 = 초원(풀이 방 안에 남, BUG-010) | 마름돌 벽(굽돌 띠 · 나무 띠) · 문설주 · 상인방 · 판석 / 나무 마루 · 나무 천장 들보 · 책장 · 아궁이 · 조리대 · 책상 · 깔개 · 화단 · 시험 받침돌 · 바닥 놋쇠 마법진 · 배너. 방 안 풀은 **풀 제외 볼륨**으로 막았다 |
| 앞마당: 상자 모닥불 돌 · 원뿔 나무 · 상자 쉼터 | 둥근 판석 마당 + 놋쇠 상감 나침반 · 굽은 돌 화단 넷(꽃) · 놋쇠 구 등 기둥 넷 · 장작 헛간 · 통 · 우물 · 쉼터 · 모닥불 돌(초원 바위 A) |
| 큰길 등불 줄 · 화로 제단 · 돌무더기: 상자 블록 | 석등 여섯 · 팔각 화로 제단 · 돌무더기 |
| 구역 안 팩 폴리지(StyleHex 나무 · 덤불 · 꽃 · 풀포기) | 이 구역 안의 인스턴스 23 만 뺐다(목록은 아래 「되돌리기」). 길가 돌 · 돌담 돌처럼 한 메시를 폴리지 타입 여럿이 쓰는 것은 타입을 가를 수 없어 두었다 |

**그대로 둔 것**: 마당으로 드는 큰길 돌담(`Wall_TowerRoad_W/E`, StyleHex 돌), 게임 액터 전부(스승 · 퀘스트 게시판 · 양초 · 작물 자리 · 과녁 · 침대 · 아궁이 불 · 냄비 · 모닥불 · 마른 풀 · 전투 금지 볼륨 · 퀘스트 발동 볼륨 · 표식 · PlayerStart), 그레이박스의 **충돌**(숨기기만 했다 — 방 크기 · 문 · 길이 전과 같다), 랜드스케이프 재질과 풀(맵 전체 하나라 이 구역만 바꿀 수 없다 — StyleHex 텍스처), 하늘 · 밤 노출 · 달빛(다른 세션 몫), 마나 빛줄기, 개발 글씨, 거점 자리 판.

## 만든 에셋

모두 `Content/Grimoire/Environment/` 아래. 원본(다시 만들 수 있는 것)은 `art/env/` 와 스크립트다.

| 묶음 | 에셋 | 만드는 길 |
|---|---|---|
| 텍스처 21 | `Materials/Textures/T_GrimEnv_*` — 돌 · 슬레이트 · 나무 · 금속 · 천 · 잎 각 색 / 노멀 / ORM, 큰 무늬 마스크, 배너 · 깔개 그림 | `art/tools/make_env_textures.py`(파이썬 절차 노이즈 — 사 온 텍스처 없음) → `art/env/materials/` |
| 마스터 재질 1 | `Materials/M_GrimEnv_Master` | `Scripts/Environment/tower_art_import.py` 가 노드를 짠다 |
| 재질 인스턴스 15 | `Materials/MI_GrimEnv_<슬롯>` — Stone · StoneDark · Flagstone · Slate · Wood · WoodInk · Brass · Iron · Glass · Cloth · Banner · Rug · Leaf · Soil · Rune | 같은 스크립트의 `MATERIALS` 표(색조 · 변이 · 이끼 · 발광) |
| 블렌더 키트 43 | `Tower/SM_Tower_*`(외관 · 앞마당) · `Tower/SM_TowerIn_*`(실내) | `art/tools/blender_tower_kit.py` + `art/tools/tower_kit/` → `art/env/tower-kit/*.glb` · `.blend` |
| Tripo 입력 그림 4 | 석등 · 화로 제단 · 우물 · 돌무더기 4면 시트(`art/env/tower-yard-*/`) | GPT(Codex) `art/env/prompts/tower-yard-*-sheet.txt` — 3D 생성은 아래 「남은 것」 |

### 마스터 재질 `M_GrimEnv_Master`

아트 방향의 「언리얼 — 마스터 재질로 모든 에셋 색을 한데 묶기」. 텍스처는 일부러 **중립 회색**이고 색은 인스턴스가 정한다 — 같은 돌 텍스처가 양피지색 벽 · 어두운 굽돌 · 회색 판석 · 흙이 된다.

| 손잡이 | 하는 일 |
|---|---|
| `Tint` · `Desaturate` | 색조 · 채도(콘셉트 색: 양피지색 돌 · 남색 슬레이트 · 놋쇠 · 잉크빛 창틀) |
| `VariationAmount` | 정점 색 R(조각마다 무작위) — 마름돌 · 판석 · 널이 하나하나 조금씩 다른 밝기 |
| `MacroTex` · `MacroScale` · `MacroStrength` | 월드 좌표로 깐 큰 얼룩(18 m) — 넓은 벽 반복 숨기기 |
| `MossAmount` · `MossColor` | 위를 보는 면 + 큰 얼룩 B 에 이끼(「Detailed Environment Art」) |
| `AOTint` | 오목한 곳을 검정 대신 **푸른 그늘**로(「Painterly — 따뜻한 빛 · 푸른 그림자」) |
| `UseVertexColor` | 잎 · 꽃 · 책처럼 정점 색이 곧 색인 것 |
| `RoughnessScale` · `MetallicScale` · `NormalStrength` · `UVScale` | PBR(손그림 빛 없음 — 「Realistic PBR」) |
| `EmissiveColor` · `EmissiveStrength` | 창 · 유리 구 · 놋쇠 상감의 따뜻한 빛(해 질 녘 창빛, 콘셉트 tower-dusk) |

텍셀 밀도: 키트 UV 는 **1 UV = 2 m**(텍스처 1024 → 512 px/m), 모든 조각이 같다. 배너 · 깔개만 그림 한 장(UV 0~1).

### 블렌더 키트

`blender.exe -b --factory-startup -P art/tools/blender_tower_kit.py -- art/env/tower-kit exterior,interior,yard` → GLB 43 + `TowerKit_*.blend`. 같은 시드라 다시 돌려도 같은 모양이다.

- 돌은 **하나하나 모델**이다(모서리 깎임 · 1.2 cm 이음매 · 면이 살짝 울퉁불퉁) — 아트 방향 「High-Fidelity — 돌 모서리 깎임 · 이음매까지 모델에」. 창 · 문 구멍 둘레에서는 돌 줄을 끊고 구멍 모양(뾰족 아치 · 원)대로 돌테를 두른다(`Opening` · `ashlar_ring` · `ashlar_wall`). Nanite 라 면 수(탑 몸 14만)가 문제 되지 않는다.
- 블렌더 +Y 는 언리얼 −Y 로 들어온다(glTF 축 변환, 가져오기 로그 「축 검사」). 좌우 대칭 조각은 그대로, 비대칭(아궁이 · 조리대)은 놓을 때 yaw 180.
- 둥근 덩어리(석등 · 화로 제단 · 돌무더기 · 우물)는 Tripo 몫이지만 Tripo 가 막혀 같은 크기 · 같은 원점의 블렌더 대체 모델(`*_Blender`)로 놓았다. Tripo 결과(`art/env/tower-yard-<이름>/SM_Tower_<이름>.glb`)가 생기면 가져오기 · 배치 스크립트가 그것을 먼저 쓴다.

## 레벨에 놓기

```powershell
$Cmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
# 1) 텍스처 · 재질 · 메시(GRIM_TOWER_ONLY=<이름,이름> 이면 그 메시만)
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Environment\tower_art_import.py" -unattended -nop4 -nosplash -nullrhi
# 2) 탑 구역 입히기(다시 돌려도 같은 결과)
& $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Tutorial\tower_art_dress.py" -unattended -nop4 -nosplash -nullrhi
```

- `build_openbeta.py` · `dress_openbeta.py` 로 맵을 다시 만들면 2) 를 다시 돌린다.
- 새 액터는 모두 태그 `TowerArt` · 폴더 `TowerArt/…` · 이름 `TA_*`, 늘 로드(먼 데서 보이는 랜드마크 · 시작 자리 — 그레이박스와 같다).
- 그레이박스 · 팩 소품은 **지우지 않고 보이지만 않게** 했다(태그 `TowerArtHidden`, 충돌은 남음). 새 메시는 충돌이 없고, 새로 생긴 막는 곳(둥근 기단 · 현관 벽 · 통로 벽 · 더 놓은 책장 · 헛간 · 우물 · 석등 · 등 기둥 · 통)만 숨긴 상자 충돌(`TA_Block_*`)을 놓았다.
- 탑 동문(X 16 m) → 통로(기단 벽 두께 안) → 현관 → 앞마당으로 길이 이어진다. 1층 방 배치 · 문 · 게임 액터 자리는 그대로다.
- 방 안 · 현관 · 마당 풀은 `AGrimGrassExclusionVolume`(`Source/Grimoire/Environment/GrimGrassExclusionVolume.h`, `TA_NoGrass_*`)이 엔진 `ALandscapeProxy::AddExclusionBox` 로 막는다 — BUG-010 고침.

### 되돌리기

```powershell
$env:GRIM_TOWER_ART_REVERT = "1"; & $Cmd "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\Tutorial\tower_art_dress.py" -unattended -nop4 -nosplash -nullrhi
```

`TowerArt` 액터를 지우고, `TowerArtHidden` 을 다시 보이게, 뺀 팩 폴리지 인스턴스를 되돌린다. 숨긴 액터 이름 · 뺀 인스턴스 변환은 `Scripts/Tutorial/tower_art_removed.json`.

## 콘셉트와 다른 점

| 콘셉트 | 이번 시범 | 까닭 |
|---|---|---|
| 가늘고 높은 원통 탑 하나(지름 약 12 m) | **지름 41.6 m 둥근 기단(1층)** 위에 가는 탑 몸(지름 14.8 → 13.2 m) | 그레이박스 1층 방 배치(32 × 24 m — 시작 방 · 숙소 · 서고 · 주방 · 홀 · 시험의 방)와 게임 액터 자리를 그대로 두려고 둥근 기단으로 감쌌다. 콘셉트 실루엣으로 가려면 1층 평면을 줄여야 한다 → [Q180](../spec/decisions#미결-질문) |
| 1층 방이 탑 원통 안쪽 곡면 벽(T01) | 네모 방(그레이박스 벽 자리 그대로), 기단과 방 사이는 막힌 빈 곳 | 위와 같다 |
| 정문 앞 마당 가운데 화로 제단 | 마당 가운데는 튜토리얼 모닥불(`Tut_Campfire`), 화로 제단은 레이아웃의 POI 자리(탑에서 약 108 m) | 게임 자리 그대로 |
| 덩굴 · 담쟁이 · 나무 · 풀 덩어리 | 없음(원뿔 그레이박스 나무는 숨김) | 잎 · 덩굴은 카드 방식(C07 · C16) — 이번 범위 밖 |
| Tripo 석등 · 화로 제단 · 우물 · 돌무더기 | 블렌더 대체 모델 | Tripo 크레딧 · 로그인 막힘(아래) |
| 위층 · 나선 계단 | 계단 모델만 만들고 놓지 않음 | 위층이 없고, 홀 문 앞을 막는다 |
| 바다 · 절벽 곶 | 지형 그대로 | 지형 몫 |

## 결과 — 콘셉트 · 전 · 후

같은 자리 · 같은 카메라(`shoot_openbeta.ps1 -Set tower`, 1600 × 900). 왼쪽 콘셉트, 가운데 전(그레이박스 · 팩), 오른쪽 후. 비교 그림은 `Scripts/QA/compare_concept.py`.

**외관**(콘셉트 `tower-exterior`)

![](/qa/tower-compare-exterior.jpg)

**큰길에서**(콘셉트 `tower-dusk` 구도 — 해 질 녘은 아래)

![](/qa/tower-compare-road.jpg)

**앞마당**(콘셉트 `tower-yard`)

![](/qa/tower-compare-yard.jpg)

**1층 홀 · 시작 방 · 서고 · 주방**(콘셉트 `tower-in-*`)

![](/qa/tower-compare-hall.jpg)

![](/qa/tower-compare-start.jpg)

![](/qa/tower-compare-library.jpg)

![](/qa/tower-compare-kitchen.jpg)

| 현관 앞 | 석등 줄 | 꺼진 화로 제단(POI) |
|---|---|---|
| ![](/qa/tower-porch-after.jpg) | ![](/qa/tower-lanternrow-after.jpg) | ![](/qa/tower-brazier-after.jpg) |

**해 질 녘**: `-TimeOfDay 0.74` 로 찍은 그림(`tower-*-after-dusk.jpg`)이 낮과 같은 하늘 · 밝기로 나왔다(하늘 · 노출 · 달빛은 밤 밝기 세션 몫, BUG-008) — 그래서 해 질 녘 색은 판정하지 않았다. 창 · 관측실 유리 · 놋쇠 구 등은 낮에도 따뜻한 빛(`EmissiveStrength`)으로 보이고, 꼭대기 혼천의가 마나 빛줄기를 두른다(`tower-road-after-dusk.jpg`).

### 성능

`measure.ps1 -TowerOnly`(문 앞 · 1층 홀 · 앞마당 — 전 `tower-before-t`, 후 `tower-after-t2`(폴리지 고친 마지막 판), 같은 조건: 다른 세션 언리얼 1개가 같이 돎). 개발 PC RX 9070, 1080p, 에디터 바이너리 -game.

| 자리 | GPU ms 전 → 후 | 렌더 스레드 ms | 드로 콜 | 프리미티브 | 메모리 MB |
|---|---|---|---|---|---|
| 탑 문 앞(`door`) | 4.34 → **4.95** | 5.48 → 6.20 | 1,086 → 1,201 | 5.05 M → 4.22 M | 3,036 → 2,935 |
| 1층 홀(`towerhall`) | 4.67 → **4.88** | 5.97 → 5.99 | 515 → 1,512 | 1.50 M → 2.17 M | 3,012 → 2,924 |
| 앞마당(`toweryard`) | 4.42 → **5.19** | 6.19 → 6.20 | 601 → 1,490 | 2.65 M → 2.16 M | 3,028 → 2,933 |

- **예산 안**([성능 예산](../systems/performance)): GPU 7 ms(개발 PC 환산) · 렌더 스레드 10 ms · 비-Nanite 드로 콜 2,000 아래. 전 자리(`tower-after`, 9 자리)도 GPU 4.4 ~ 5.0 ms.
- 가장 크게 는 것은 **드로 콜**(홀 · 마당 +900 ~ 1,000) — 1층 벽 · 바닥 · 천장 조각 148 개 × 재질 슬롯 2~3 과 움직이는 점광 6 개(현관 · 통로 · 등 기둥)다. 다음에 줄일 곳: 1층 조각을 방마다 한 메시로 묶기, 등 기둥 점광 그림자 끄기.
- 메모리는 측정마다 ±150 MB 흔들려 차이를 가를 수 없다(새 텍스처 21 장 1K · 메시 43, 디스크 약 54 MB).
- 전 9 자리 측정(`tower-before` · `tower-after`)은 다른 세션의 언리얼이 둘 돌던 때라 렌더 스레드가 모든 자리(먼 계곡까지)에서 10 ms 로 같이 늘었다 — CPU 숫자는 `-TowerOnly` 표를 본다.

## 남은 것

- **Tripo 3D**: API 크레딧 0, Tripo Studio 웹은 이 PC 의 크롬 · 내장 브라우저 모두 로그인 전이라 생성하지 못했다(로그인 · 결제는 하지 않는다). 입력 시트는 준비됨: `art/env/tower-yard-{brazier,lantern,well,cairn}/views/`. 로그인하면 웹 멀티뷰(울트라 메시 · 4K · 조명 제거 · PBR) → GLB 를 `art/env/tower-yard-<이름>/tripo/` → `blender_cleanup.py` 로 `SM_Tower_<이름>.glb` → 1) · 2) 다시.
- 나무 · 덩굴 · 풀 덩어리(카드), 탑 위층, 1층을 원통 안 곡면 방으로(Q180).
