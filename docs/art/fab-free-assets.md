# Fab 무료 에셋 후보

> 상태: **검토용 v2** · 2026-11-07 · v2 에서 [필요 목록 대조](#필요-목록-대조)와 [실내](#실내)를 더했다. Fab(fab.com) 상세 페이지와 Fab 이 페이지에 쓰는 목록 데이터(가격 · 라이선스 · UE 버전 · Nanite 표시)를 하나씩 직접 확인했다. 받기(claim) · 구매 · 로그인은 하지 않았다 — **받기는 사용자가 한다.**
>
> 필요 항목 번호(C01 …)는 [1번 지역 — 필요한 3D 모델](./region1-assets), 스타일 기준은 [아트 방향](../spec/art-direction)(Painterly High-Fidelity Stylized Realism)이다.

## 요약

- **찾은 것 175개**(아래 표): 자연물 57 · 돌 건축 · 폐허 18 · 소품 · 캠프 · 마법 소품 22 · 에픽 공식 샘플 5 · 기간 한정 1 · **실내 65**(v2) · 대조표 보충 7(v2). 모두 2026-11-07 에 0원이었다(「개인만」은 개인 등급만 0원).
- **필요 목록 대조**([표](#필요-목록-대조)): 공통 모델 C01~C43 · 탑 실내 키트 T01~T13 = 56개 번호 중 **있음 30 · 일부 17 · 없음 9**.
- **채우는 항목**
  - 자연물은 거의 다 채운다: 나무(C07 · C08 · C09) · 관목 · 풀(C12) · 들꽃(C13) · 고사리(C14) · 덩굴(C16) · 이끼 · 숲 바닥(C17) · 바위(C01 · C04) · 절벽(C02) · 쓰러진 나무 · 그루터기(C11) · 지형 재질 · 물 재질 · 하늘.
  - **Megascans 「Historic Medieval」 연작**(서고 · 성 · 교회 폐허 · 농가, 검색 표본 392개 중 288개가 0원)이 v2 의 가장 큰 발견이다 — 폐허 석벽 · 아치 · 창(C21 · T04), 성 문 · 문틀 · 기둥(C24 · T06), 벽난로 · 화덕 · 솥, 샹들리에 · 촛대 · 횃불(T09 · C41), 의자 · 탁자 · 걸상(T11), 두루마리 · 책(C40 · C43), 상자 · 통 · 자루 · 바구니 · 장작 · 수레 · 맷돌(C27~C29 · C36 · C38 · 방아실).
  - 실내 꾸밈은 가구 · 부엌 · 조명 · 직물 · 계단 · 문까지 무료로 대부분 채운다. 모듈식 실내 키트는 [Medieval Room Kit](https://www.fab.com/listings/52610433-0210-40e5-884b-abc12cf8901e)(손그림 모듈 48, CC BY) 하나가 쓸 만하다.
- **빈 곳**(무료로 맞는 것을 못 찾음)
  - **마법탑 본체**(원통 석조 층 모듈 · 곡면 실내 벽 T01 · 관측 돔 · 놋쇠 혼천의 C42 · 둥근 창 T05 · 나선 계단) — 무료 중 원통 탑 키트는 없다. 키트(블렌더)로 만드는 계획 그대로.
  - **룬 선돌 · 선돌 고리(C33)** · 길가 돌 표지 · 돌 상자(C31) · 바닥 상감 원(T13) — 맞는 것이 없다. 계획대로 만든다.
  - **거대 고목(XL)** · 부유 바위섬 · 바위 문 기둥 · 생물 자리 굴(C25) — 무료 히어로 메시는 없다.
  - **풍차 외관 · 날개 · 큰 나무 톱니바퀴 · 축** — 무료는 로우 폴리 · 만화풍만. 맷돌만 Megascans 로 채운다.
  - 마나초(C18) · 빛나는 버섯 — 판타지 식물 무료는 만화풍뿐. 사실적 꽃 · 버섯에 발광 재질을 입히는 쪽이 빠르다.
  - 마법 기구 — 망원경 · 지구의 · 혼천의 · 레일 사다리 · 독서대 · 수련실 과녁 · 허수아비 · 퀘스트 게시판은 무료로 맞는 것이 없다([실내 빈 곳](#실내-빈-곳)).
- **스타일**: 우리 방향에 바로 맞는(상) 무료는 적다 — StyleHex 의 무료 세 개(숲 · 풀꽃 · 바위)가 가장 가깝다. 나머지 좋은 품질은 대부분 **사실적**(Megascans · Megaplants · Project Nature)이라 셰이더로 맞춰야 한다(아래 「Q 후보」).

### 표 읽는 법

- **무료 종류**: `영구` = 개인 · 기업 등급 모두 0원(마감일 없음) · `개인만` = 개인 등급만 0원, 기업 등급은 유료 · `기간 한정(~마감)` = 정가가 있고 마감일까지 0원 · `CC BY` = 크리에이티브 커먼즈 저작자 표시(크레딧 필요).
  - 판매자가 나중에 가격을 바꿀 수 있다. 0원일 때 받아 두면 내 라이브러리에 남는다(가격이 바뀐 뒤의 동작은 이 조사에서 직접 확인하지 않았다).
- **라이선스**: `Fab Std` = Fab Standard License(상업 게임 가능, 크레딧 불필요) · `CC BY 4.0` · `UE 전용` = 옛 UE 마켓플레이스 라이선스이거나 설명에 「언리얼 엔진 제품에만」이라고 적힌 것.
- **UE**: 상세 페이지의 지원 엔진 범위. 5.8 이 없으면 가장 높은 버전.
- **Nanite**: `예` = 판매자가 Nanite 를 켰다고 표시 · `아니오` = 끈다고 표시 · `—` = 표시 없음.
- **스타일 맞음**: 썸네일과 갤러리를 보고 매긴 것. `상` 그대로 섞어 써도 된다 · `중` 형태는 맞고 재질 · 색만 맞추면 된다(사실적 스캔 대부분) · `하` 로우 폴리 · 만화풍이라 우리 화면에 섞기 어렵다(그레이박스 대용).
- **AI 생성**: 판매자가 「AI 생성」으로 표시한 것은 메모에 적었다.

## 필요 목록 대조

[1번 지역 — 필요한 3D 모델](./region1-assets)의 공통 모델(C01~C43)과 탑 실내 키트(T01~T13) 번호마다 무료 후보가 있는지 본다. `있음` = 그대로 놓을 만한 후보가 있다(스타일은 대부분 「중」) · `일부` = 일부 변형만, 또는 고쳐 써야 한다 · `없음` = 계획대로 만든다. 후보 이름의 링크는 아래 각 표에 있다. 「MS」 = Megascans 낱개.

| 번호 | 모델 | 판정 | 무료 후보 |
|---|---|---|---|
| C01 | 초원 바위 | 있음 | StyleHex Stylized Rocks(상) · MS Mossy Rock Formation |
| C02 | 절벽 · 바위벽 | 있음 | MS Sharp Cliff · Limestone Rock Strata · Stone Walls Vol. 1 · Dark Ruins 샘플 |
| C03 | 물가 돌 · 징검돌 | 일부 | 바위 팩의 납작한 돌에 젖은 재질. 전용 징검돌은 없음 |
| C04 | 잔돌 · 자갈 | 있음 | StyleHex Rocks · Forest Sample |
| C05 | 판석 디딤돌 · 돌계단 | 일부 | MS 교회 폐허 바닥 판석(Church Ruin Floor Stone Slab). 계단 조각은 없음 |
| C06 | 광석 노두 | 일부 | 결정만 — [Stylized Crystal Pack](https://www.fab.com/listings/ac4247b8-e7b8-4826-92b2-1cf5ebfedeac)(CC BY). 바위에 박힌 광맥은 없음 |
| C07 | 활엽수 | 있음 | StyleHex Forest(상) · Megaplants English Oak · Hornbeam |
| C08 | 작은 나무 · 관목 | 있음 | Megaplants Silver Birch · Free Shrubs Pack · Cozy Nature |
| C09 | 침엽수 | 있음 | Project Nature Spruce Forest · Megaplants(Norway Spruce · Baltic Pine) |
| C10 | 속 빈 고목 둥치 | 일부 | Megaplants Dead Pine · Stump Scanned. 속이 빈 큰 둥치는 없음 |
| C11 | 쓰러진 통나무 · 그루터기 | 있음 | Black Cherry Broken Trunks · MS Large Fallen Tree · Mossy Tree Stump |
| C12 | 풀 | 있음 | StyleHex Foliage(상) · Project Nature Grass Library |
| C13 | 들꽃 | 있음 | StyleHex Foliage(상) · Meadow Flowers · Megaplants Heather · Wood Anemone |
| C14 | 고사리 · 넓은 잎 덤불 | 있음 | Fern Collection · Megaplants Bracken Fern · MS Lady Fern |
| C15 | 갈대 · 부들 · 수련 | 일부 | Megaplants Common Bulrush · Dry Common Reed. 수련은 없음 |
| C16 | 담쟁이 · 덩굴 | 있음 | [Dynamic Ivy Creation Pack](https://www.fab.com/listings/71697e6c-508c-420b-86e1-0ffb85827f27)(Project Nature, UE 4.19~5.4) · [MS English Ivy](https://www.fab.com/listings/3b14e8ce-d4fe-4efa-af61-16335108b36f) |
| C17 | 이끼 · 낙엽 · 흙 얼룩 | 일부 | MS Moss Scatter · Mossy Forest Floor(표면). 데칼 묶음은 없음 |
| C18 | 마나초 | 없음 | 만화풍뿐. 사실적 꽃 + 발광 재질로 |
| C19 | 버섯 | 일부 | MS Bolete Mushrooms. 선반 버섯 · 빛나는 버섯은 없음 |
| C20 | 마른 돌담 | 일부 | [MS Modular Stone Wall](https://www.fab.com/listings/16754e4c-74dd-4bc7-8833-025ef47df632) · Medieval Game Environment(UE 전용) |
| C21 | 폐허 석조 벽 키트 | 있음 | **MS 교회 폐허 연작**(벽 · 아치 · 창 · 버팀벽 · 바닥) · Maridalen Ruins |
| C22 | 무너진 목조 들보 | 있음 | [MS Old Wooden Beam](https://www.fab.com/listings/13d1a4c3-1f5b-4e85-85bd-a5a343c1620f) 여러 개 |
| C23 | 나무 울타리 · 말뚝 | 있음 | [MS Historic Medieval Farm Post Oak](https://www.fab.com/listings/28ac2daa-661f-4284-92f6-ab76e61fd475) · [Farm Pen Wall](https://www.fab.com/listings/40dac89b-63c1-46e9-8f78-37368642863a) |
| C24 | 나무 문 | 있음 | MS 성 문(Castle Door) · Modular Wooden Door · STYLIZED Door Dungeon |
| C25 | 생물 자리 — 굴 입구 | 없음 | — |
| C26 | 생물 자리 — 빈 둥지 | 일부 | [MS Grassland African Nest Bird](https://www.fab.com/listings/a28d5aa6-e749-4b65-b766-3321e96fd226) — 초원 새 둥지 하나 |
| C27 | 나무 상자 · 궤 | 있음 | MS 농가 상자 · Old Wooden Chest · Wood Chest-01 |
| C28 | 통 · 양동이 | 있음 | MS 농가 통 · Barrel: Stylized Wooden · Wooden Pottery Props |
| C29 | 자루 | 있음 | MS 농가 자루 · Medieval Burlap Sack Pack |
| C30 | 항아리 · 병 · 단지 | 있음 | MS 농가 질그릇 단지 · EnchantedPotionRoom · Fantasy Bottle |
| C31 | 돌 상자(룬) | 없음 | — |
| C32 | 나무 보물 상자 | 있음 | Free Treasure Chest – IronBound(CC BY) · Last Wizard 상자 |
| C33 | 룬 선돌 | 없음 | — |
| C34 | 돌무더기 | 없음 | 바위 팩의 돌을 쌓아 만든다 |
| C35 | 모닥불 고리 | 있음 | Stick Together FREE · Medieval Campfire |
| C36 | 짐수레 | 있음 | MS 농가 수레(Farm Cart Wood Worn) · 마차(Farm Wagon) |
| C37 | 건초 · 밀 다발 | 일부 | [MS Round Hay Bale](https://www.fab.com/listings/fc0388e8-5443-4e54-b618-65790804779b) · 건초 더미. 밀 다발 · 밀밭 카드는 없음 |
| C38 | 장작 더미 | 있음 | MS 농가 장작 더미 · Firewood Pieces |
| C39 | 낡은 천 · 깃발 | 일부 | Torn Banner(개인만) · Last Wizard 배너. 늘어진 휘장은 없음 |
| C40 | 종이 소품 | 일부 | MS 서고 두루마리. 지도 · 쪽지 · 별 지도는 없음 |
| C41 | 양초 · 촛대 · 랜턴 | 있음 | MS 농가 촛대 · 서고 횃불 · 양초 |
| C42 | 놋쇠 혼천의 | 없음 | — |
| C43 | 책 | 있음 | MS 서고 책 · 옛 책 · EnchantedPotionRoom 책 |
| T01 | 곡면 석벽 모듈 | 없음 | 곧은 벽만 — Medieval Room Kit · MS 교회 폐허 벽 |
| T02 | 바닥(판석 · 대리석 · 마루) | 일부 | Stone Tiles Material Pack · MS 교회 폐허 판석. 대리석 · 마루 재질은 따로 |
| T03 | 나무 천장 들보 · 늑골 천장 | 일부 | MS 나무 들보. 방사형 서까래 · 고딕 늑골 천장은 없음 |
| T04 | 고딕 아치 창 | 일부 | MS 교회 폐허 창(돌 틀). 납 유리 격자는 없음 |
| T05 | 둥근 창 | 없음 | — |
| T06 | 원형 기둥 | 일부 | MS 성 기둥(Castle Pillar). 놋쇠 띠 · 주두는 없음 |
| T07 | 남색 배너 | 일부 | Torn Banner · Last Wizard 배너(색 바꿈) |
| T08 | 러그 | 있음 | MS Historic Bazaar Rug 여러 장 |
| T09 | 촛대 샹들리에 | 있음 | MS 서고 샹들리에 · Pirate chandelier(CC BY) |
| T10 | 벽 선반 · 벽 촛대 | 있음 | MS Wooden Shelf · 서고 횃불 |
| T11 | 책상 · 의자 · 걸상 | 있음 | MS 서고 탁자 · 의자 · 걸상 · 5 Stylized Stools |
| T12 | 화분 | 있음 | MS 질그릇 화분(Clay Pot) |
| T13 | 바닥 상감 원 | 없음 | 데칼로 만든다 |

**장소별 핵심 모델**(그림별 표의 굵은 것) 중 무료로 채우는 것: 돌 우물 · 벽난로 · 돌 아궁이 · 무쇠 솥 · 맷돌 · 돌다리 · 성 문틀 · 감시탑 폐허 벽(교회 폐허 연작) · 무기(창 · 검). 못 채우는 것: 마법탑 본체 · 관측 돔 · 정문 · 석등 · 화로 제단 · 거대 고목 · 바위 기둥 · 선돌 · 부유석 · 풍차 · 날개 · 톱니바퀴 · 망원경 · 혼천의 · 봉화대 · 들벌 둥지 · 허수아비 · 과녁 · 퀘스트 게시판 · 속성 받침대 · 시험관 의자.

## 자연물

### 나무 — C07 활엽수 · C08 작은 나무 · C09 침엽수

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [FREE Stylized Forest Sample](https://www.fab.com/listings/3c1a31b6-e523-4a0f-97d0-26b0db69b6bc) (StyleHex Studio) | 영구 | Fab Std | 4.27~5.8 | **상** | 나무 · 덤불 · 풀 · 바위 18메시, LOD 4, Nanite 아니오. 맵 2. 반사실 스타일라이즈드 — 우리 방향에 가장 가깝다 |
| [Megaplants: English Oak](https://www.fab.com/listings/83642c38-7661-4df1-8629-0422e1898d26) (Quixel Megaplants) | 영구 | Fab Std | 5.7~5.8 | 중 | 참나무 = C07 형태 그대로. Nanite 폴리지 · 절차적 식생 편집기(PVE) 프리셋 · 다이내믹 윈드. **PVE 플러그인 필요** |
| [Megaplants: European Hornbeam](https://www.fab.com/listings/43679e75-6ed0-4b03-9288-9e2bc49376d2) | 영구 | Fab Std | 5.7~5.8 | 중 | 서어나무. 둥근 잎 덩어리(C07) |
| [Megaplants: Silver Birch](https://www.fab.com/listings/94262633-61ed-4f4b-993e-dbc24af3e6ba) | 영구 | Fab Std | 5.7~5.8 | 중 | 자작나무(C08) |
| [Megaplants: Dead Pine](https://www.fab.com/listings/bc9e4e89-fcf4-4be6-b0f0-8b37395a72de) | 영구 | Fab Std | 5.8 | 중 | 고사목 — 고목 둥치(C10) · 고개 분위기 |
| [Megascans Trees: European Beech](https://www.fab.com/listings/d11cc01d-9422-41b7-950f-416c9ce79caf) (Quixel Megascans) | 영구 | Fab Std | 5.1~5.6 | 중 | 옛 「Early Access」 나무. Megaplants 이전 방식 |
| [temperate Vegetation: Spruce Forest](https://www.fab.com/listings/f8044501-17a2-498f-b198-5f1bc71ee87a) (Project Nature) | 영구 | Fab Std | 4.19~**5.4** | 중 | 가문비 숲(C09). 평점 4.7(119). 5.4 프로젝트로 받아 옮겨야 한다 |
| [DZ Sample Trees](https://www.fab.com/listings/7c95ac5f-246e-42fb-9d2a-4807db87f69d) (DropZone Studios) | 영구 | Fab Std | 5.1~5.8 | 중 | 사실적 나무 몇 그루(Nanite 언급) |
| [Unreal Engine - Free Nanite Plants Tree Grass Shrub Sample](https://www.fab.com/listings/728a2a8b-9702-4525-afa5-b4a9b3fb636f) (3DGardenPlants) | 영구 | Fab Std | 5.5~5.8 | 중 | Nanite 예. 나무 · 풀 · 관목 샘플 |
| [Cozy Nature – Foliage Pack](https://www.fab.com/listings/d6f25a58-a0ed-4491-be48-e764cb26cef6) (ATRV Studio) | 영구 | Fab Std | 5.2~5.8 | 중 | 활엽수 · 침엽수 · 풀 · 꽃 · 바위 21메시. 밝은 반사실 |
| [Stylized Environment - Forest](https://www.fab.com/listings/7b06f300-f295-40fe-9972-b9de3a0abe03) (SilverSet Studios) | 개인만 | Fab Std | 5.0~5.7 | 하 | 28메시. 채도 높은 만화풍 |
| [Free Mega Tree](https://www.fab.com/listings/e5ac0773-7e11-4e78-8d59-526ecd661b93) (3D Nik) | 개인만 | Fab Std | 5.5~5.8 | 하 | 로우 폴리 나무 1 |
| [Stylized Nature Pack](https://www.fab.com/listings/b066de06-73b8-4fbe-b30c-468f5bcf7575) (Tiny Talisman Games) | 개인만 | Fab Std | 4.22~5.7 | 하 | 로우 폴리 숲. 그레이박스 대용 |

Megaplants 는 40종 전부가 0원이었다(갈대 · 고사리 · 히스 · 블루베리 · 엘더 · 묘목들 포함, 판매자 페이지 「Quixel Megaplants」).

### 관목 · 풀 · 들꽃 · 고사리 · 갈대 — C08 · C12 · C13 · C14 · C15

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [FREE Stylized Foliage — Grass, Flowers & Plants](https://www.fab.com/listings/b494d1c5-8d02-48c4-9038-54bb8970a26f) (StyleHex Studio) | 영구 | Fab Std | 4.27~5.8 | **상** | 풀 · 꽃 · 식물 29메시(삼각형 59~597), Nanite 아니오. C12 · C13 바로 |
| [temperate Vegetation: Foliage Collection](https://www.fab.com/listings/6a5ae8db-d80f-4b23-b276-87da390cfe56) (Project Nature) | 영구 | Fab Std | 4.19~**5.4** | 중 | 아래 셋(풀 라이브러리 · 바닥 식생 · 들꽃)을 묶은 것 + 지형 재질. 폴리지 묶음 277개. 평점 4.6(70) |
| [temperate Vegetation: Meadow Flowers](https://www.fab.com/listings/e648b1bd-11f6-4895-bd43-15e399c80297) (Project Nature) | 영구 | Fab Std | 4.19~5.4 | 중 | 꽃 20종 · 묶음 96. 피벗 페인터 바람. C13 데이지 · 노란 꽃 |
| [temperate Vegetation: optimized Grass Library](https://www.fab.com/listings/8b68642e-35f4-438e-82b4-799fc2228303) | 영구 | Fab Std | 4.19~5.4 | 중 | C12 |
| [temperate Vegetation: Ground Foliage](https://www.fab.com/listings/4c5226fa-0dd0-4ab1-b49a-523b72ef4107) | 영구 | Fab Std | 4.19~5.4 | 중 | 바닥 잡초 · 낙엽 덤불 |
| [Temperate Vegetation: Fern Collection](https://www.fab.com/listings/b778bd8f-524c-42b6-b60c-4caac59029c1) | 영구 | Fab Std | 4.19~5.4 | 중 | C14 고사리 |
| [Temperate Vegetation: Wild Berries](https://www.fab.com/listings/b6307eff-f99f-48ae-bc3e-4d15b56eda11) | 영구 | Fab Std | 4.19~5.4 | 중 | 산딸기 덤불 — 채집물 후보 |
| [interactive Open World Foliage](https://www.fab.com/listings/eba98f8c-d99a-4f60-8495-5db0e62666ff) | 영구 | Fab Std | 4.19~5.4 | 중 | 평점 4.7(188). 밟으면 눕는 풀 |
| [Megaplants: Bracken Fern](https://www.fab.com/listings/4d60e635-7a49-4f19-818c-d018046be39f) | 영구 | Fab Std | 5.8 | 중 | C14 |
| [Megaplants: Common Heather](https://www.fab.com/listings/5d88b60a-524c-4774-a9d7-7e1cd4510be6) | 영구 | Fab Std | 5.8 | 중 | 보라 꽃대(C13) 대용 |
| [Megaplants: Common Bulrush](https://www.fab.com/listings/764be2a2-72e0-4fb8-8108-532a4a3dc046) | 영구 | Fab Std | 5.8 | 중 | 부들(C15) |
| [Megaplants: Wood Anemone](https://www.fab.com/listings/636348cc-22c5-49ea-b93c-ff60af8384c7) | 영구 | Fab Std | 5.8 | 중 | 흰 들꽃(C13) |
| [Free Shrubs Pack (Ultra Realistic Wind)](https://www.fab.com/listings/7ca465ab-fb9c-4d6b-bddb-82c20f604657) (Greenleaf Vision) | 영구 | Fab Std | 5.4~5.8 | 중 | 관목(C08), Nanite 언급 |
| [Light Foliage](https://www.fab.com/listings/30e6c98c-6634-4b7e-b142-2c3cdfde6e1e) (MYTHRA TECH) | 개인만 | Fab Std | 4.23~5.6 | 중 | 풀밭 · 꽃 · 관목. 평점 4.9(49) |
| [Megascans: Lady Fern](https://www.fab.com/listings/3eead001-af7f-4b65-afcc-89c368f089bc) | 영구 | Fab Std | Fab 플러그인 | 중 | Megascans 낱개 예시 |

### 바위 · 절벽 — C01 · C02 · C03 · C04

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [FREE Stylized Rocks — Stones & Boulders](https://www.fab.com/listings/3edeea52-d490-4ffd-8b51-70376f5ec82d) (StyleHex Studio) | 영구 | Fab Std | 4.27~5.8 | **상** | 바위 12메시(삼각형 178~1,526). C01 · C04 |
| [Stylized Painterly Rock Pack](https://www.fab.com/listings/7dcf8d2d-7b03-46a0-acc0-497e6735b05a) (ShatterChron Interactive) | 영구 | Fab Std | 5.5 | 중 | 붓 터치 · 이끼 높이 블렌드를 마스터 재질에서 조절 — **우리 바위 재질 참고용으로도 좋다** |
| [Dark Ruins Megascans Sample](https://www.fab.com/listings/836ed2f8-e2d6-49be-98d3-59d104bd351e) (Quixel Megascans) | 영구 | Fab Std | 5.5~5.8 | 중 | 절벽 · 바위 · 폐허 샘플 프로젝트. 평점 4.4(219). 아래 「폐허」에도 |
| [Megascans: Mossy Rock Formation](https://www.fab.com/listings/3acf7891-dfe0-45da-b210-813fdc3d1f77) | 영구 | Fab Std | Fab 플러그인 | 중 | 이끼 낀 바위(C01) |
| [Megascans: Sharp Cliff](https://www.fab.com/listings/975dd55b-a53f-4ce7-84f6-94a096250631) | 영구 | Fab Std | Fab 플러그인 | 중 | 절벽(C02). **같은 이름의 유료 「Sharp Cliff」가 따로 있다** — 링크로 받는다 |
| [Dolomites — Free Rock/Boulder Sample (8K/Nanite)](https://www.fab.com/listings/38f5beea-da28-4bd2-af41-24b0d48b967d) (APEX Nature) | 영구 | Fab Std | 5.4~5.8 | 중 | 큰 바위 1개, Nanite 예(삼각형 200만) |
| [Free Sample - Realistic Rock Set (Limestone Rock Strata)](https://www.fab.com/listings/d2c74eb7-4278-40ab-861a-fdca41855f5a) (Ancraite) | 영구 | Fab Std | 5.4~5.7 | 중 | 층진 석회암 — C02 층진 암벽에 가깝다 |
| [[FREE] Mountain Tops for Unreal Engine 5+](https://www.fab.com/listings/8cdfadfb-5fa6-4b4e-99db-1a02b01295c2) (Arghanion's Puzzlebox) | 개인만 | Fab Std | 5.3~5.8 | 중 | 원경 산봉우리. 평점 4.9(35) |
| [Stone Walls Vol. 1](https://www.fab.com/listings/180a95c2-c24c-41a2-b171-243e7410b637) (Unimodels) | 영구 | Fab Std | 4.18~5.8 | 중 | 이름과 달리 스캔 암벽 조각 — 절벽 · 바위벽(C02) |
| [Environment - Rock Collection 04 (Free)](https://www.fab.com/listings/a51e61ac-98fa-4c54-ab23-fc533687afb7) (Shadowmire Studios) | 개인만 | Fab Std | 5.1~5.8 | 중 | 평점 5(21) |
| [Free Pack - Rocks Stylized](https://www.fab.com/listings/a0746c4b-428b-4556-b4c9-98f70c2a30d4) (PolyOne Studio) | 개인만 | Fab Std | 5.7~5.8 | 하 | 11메시, 매끈한 로우 폴리 |

### 쓰러진 나무 · 그루터기 · 숲 바닥 · 이끼 · 버섯 — C11 · C17 · C19

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Black Cherry Broken Trunks 01](https://www.fab.com/listings/2ee3d260-9ed0-434c-aff2-168ff07e7c90) (Baldinoboy 3D) | 영구 | Fab Std | 5.0~5.8 | 중 | 부러진 줄기 2, Nanite 예. C11 |
| [Black Cherry Firewood Pieces 01](https://www.fab.com/listings/63322d94-8503-4ce2-a1fb-4b135719c530) (Baldinoboy 3D) | 영구 | Fab Std | 5.3~5.6 | 중 | 장작 조각 — 장작 더미(C38) |
| [Stump Scanned](https://www.fab.com/listings/5f433961-8d90-49ab-a991-e4c7a78acccf) (GreenBugGames) | 영구 | Fab Std | 4.20~5.7 | 중 | 스캔 그루터기 |
| [Megascans: Large Fallen Tree](https://www.fab.com/listings/5796e9d5-14d8-45fa-874c-1858e2b0730e) | 영구 | Fab Std | Fab 플러그인 | 중 | 쓰러진 큰 나무(C11) |
| [Megascans: Mossy Tree Stump](https://www.fab.com/listings/6c32e10b-e715-4910-93a1-6d8d2fb5323c) | 영구 | Fab Std | Fab 플러그인 | 중 | 이끼 그루터기 |
| [Megascans: Mossy Forest Floor](https://www.fab.com/listings/31b154e8-dea5-49c8-9b29-6362fe1a4a8d) | 영구 | Fab Std | Fab 플러그인 | 중 | 숲 바닥 표면(C17 · 지형) |
| [Megascans: Icelandic Rock Moss Scatter](https://www.fab.com/listings/a47d5ce5-dd6f-42c1-b253-d75cc94301a0) | 영구 | Fab Std | Fab 플러그인 | 중 | 이끼 흩뿌림 |
| [Megascans: Bolete Mushrooms](https://www.fab.com/listings/83c5683b-19c3-483f-bf9e-1e2c35de1ba8) | 영구 | Fab Std | Fab 플러그인 | 중 | 그물버섯(C19). 빛나는 버섯은 재질로 |
| [Mushrooms](https://www.fab.com/listings/17a8192f-0195-4b7f-9df6-6e4921eebc20) (DzenGame) | 영구 | Fab Std | 5.5~5.7 | 하 | 만화풍 버섯 묶음 |
| [Fantasy Nature Asset: Druidic Throne Starter Pack](https://www.fab.com/listings/7548d357-0bed-4950-b912-56c3a4bf2875) (Marco Lo Bartolo) | 영구 | Fab Std | 5.5 | 중 | 반사실 판타지 — 뿌리 · 이끼 · 배너 낀 돌 왕좌. 거대 고목 뿌리(poi-c5) 분위기 참고 |

### 지형 재질 · 물 · 하늘

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Landscape Material — MW Landscape Auto Material](https://www.fab.com/listings/6602874e-ef24-48c9-9055-a7ac07384696) (MAWI United) | 영구 | Fab Std | 5.2~5.8 | 중 | 절차 레이어 5개 자동 지형 재질. 평점 4.4(200) |
| [Forest Landscape Materials Vol. II](https://www.fab.com/listings/96631c55-2f90-43bf-8dc4-55a93c69b96f) (Project Nature) | 영구 | Fab Std | 4.19~5.4 | 중 | 숲 바닥 지형 재질 |
| [Grass Landscape Material Vol. I](https://www.fab.com/listings/029abeaf-f109-4a66-9761-98ed53a511bb) (Project Nature) | 영구 | Fab Std | 4.19~5.4 | 중 | 평점 3.6 — 낮다 |
| [FREE 4x4km Coastal Landscape Sample](https://www.fab.com/listings/fe1373c0-969d-4ebf-a8f0-9468fd29d239) (FreshCan) | 영구 | Fab Std | 5.4~5.8 | 중 | 4 km 해안 지형 + 지형 재질 시스템. 곶 위 탑 지형 참고 |
| [Tileable Pine Forest Road (4K)](https://www.fab.com/listings/2e64dfe5-4727-4f94-b84d-e652a426b9e6) (Serkan CINAR) | 영구 | **CC BY 4.0** | 5.7 | 중 | 숲길 재질, 랜드스케이프 스플라인용. 크레딧 필요 |
| [Water Materials](https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861) (tharlevfx) | 영구 | **CC BY 4.0** | 4.13~5.8 | 중 | 바다 · 강 · 폭포 · 연못 물 재질 12종(+ 가벼운 8). 평점 4.5(279). 크레딧 필요 |
| [Good SKY](https://www.fab.com/listings/6eb8de95-710e-45cf-a029-e48e709aef03) (Uneasy Game Dev) | 영구 | Fab Std | 4.18~5.8 | 중 | 하늘 프리셋 20+ · 구름 4종 · 낮밤. 평점 4.5(274) |
| [Free Hand-Painted Stylized Clouds Sky Material](https://www.fab.com/listings/e07ed1a5-341a-49bb-810d-a4e54bad14da) (PurrSophy-Studio) | 영구 | Fab Std | 5.7~5.8 | **상** | 손그림 구름 텍스처 3장 + 움직이는 재질. 원경 붓 터치 구름 |

물 표면 · 강 흐름은 엔진의 Water 플러그인으로 만들고 위 재질 · 하늘은 참고 · 보조로 쓰는 것을 가정했다.

## 돌 건축 · 폐허 · 마법탑 느낌 — C20 · C21 · C22 · T01~T13

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Maridalen Ruins](https://www.fab.com/listings/1bb299e1-fc4c-4115-b39a-dde723288d17) (William Faucher) | 영구 | Fab Std | 5.7~5.8 | 중 | 오슬로 근교 중세 석조 폐허 스캔. 무너진 석벽 · 아치(C21) — 폐허 감시탑 · 관측소 형태 참고 |
| [Dark Ruins Megascans Sample](https://www.fab.com/listings/836ed2f8-e2d6-49be-98d3-59d104bd351e) | 영구 | Fab Std | 5.5~5.8 | 중 | 어두운 성 폐허 · 절벽 샘플 프로젝트 |
| [Medieval Game Environment](https://www.fab.com/listings/2e11a225-a6ea-4781-a3e1-fe975b461894) (Quixel Megascans, 옛 Medieval Village Megascans Sample) | 영구 | Fab Std · **UE 전용** | 4.26~5.8 | 중 | 중세 마을 · 숲 · 돌담. 설명에 「언리얼 엔진 기반 제품에만 사용」 |
| [Goddess Temple Megascans Sample](https://www.fab.com/listings/471eaad7-3c05-45bd-ac37-02dddb17780f) | 영구 | Fab Std | 4.25~5.8 | 중 | 신전 기둥 · 계단 — 시험의 방 · 선돌 제단 참고 |
| [Realistic Ruins - Free Sample (archway_pillar)](https://www.fab.com/listings/de667eaa-cced-4a13-b769-178972a6e2fc) (Symphonie Studio) | 영구 | Fab Std | 5.5 | 중 | 아치 기둥. 사막 사암 재질 |
| [Stone Pillar - Intact & Broken Variations](https://www.fab.com/listings/bfc1b365-2a75-4b95-ac2f-f7f1549b88e2) (GriidArt) | 영구 | Fab Std | 5.0~5.7 | 중 | 온전 · 부서진 돌기둥 |
| [Medieval Stone Bridge - Nanite Game-Ready](https://www.fab.com/listings/4c64ce34-195b-4b45-8e14-70273cc984b5) (Pierogi3) | 영구 | Fab Std | 5.7 | 중 | 아치 3개 돌다리 — 강가 돌 수문(lm-watchtower) 형태와 비슷 |
| [Free Sample Dark Fantasy Gothic Environment Kitbash](https://www.fab.com/listings/338af1ae-80da-4d94-b3ef-38368e287678) (Oleg Ushenok) | 영구 | Fab Std | 5.5~5.7 | 중 | 고딕 건물 한 채(유료 세트 샘플). 고딕 창 · 첨탑 참고 |
| [Last Wizard FREE - Stylized Castle Room](https://www.fab.com/listings/6c0c7f4f-f872-476b-82d7-b5bcf417adff) (RetroStyle Games) | 영구 | Fab Std | 5.8 | **상** | 성 방 한 칸 + 복도: 모듈 벽돌 벽 · 문 · 창 · 돌 벽난로 · 탁자 · 러그 · 배너 · 횃불 · 촛대 · 보물 상자 + 리깅된 기사. 탑 실내(T01 · T07 · T08 · C32 · C41) 분위기 |
| [Stylized Dungeon Pack](https://www.fab.com/listings/c6e91312-202a-4d80-a6f1-1b374bb27dce) (CobraGamesAssets) | 영구 | Fab Std | 5.3~5.5 | 중 | 횃불 켜진 석조 실내 · 나무 탁자 |
| [Stylized Fantasy Provencal](https://www.fab.com/listings/ced19ea1-31ed-437f-ae64-2b6b1561fede) (StylArts) | 영구 | Fab Std | 4.26~5.8 | 중 | 스타일라이즈드 판타지 마을 — 돌 탑 · 풍차 · 석조 집. 평점 4.4(113) |
| [ElderBoom Hollow Massive Medieval Village](https://www.fab.com/listings/e12de9d5-be28-40df-a387-42ae6f84e05c) (karaman) | 영구 | Fab Std | 5.6~5.7 | 중 | 사실적 중세 마을 큰 환경 — 돌담 · 목조 헛간(풍차 · 곳간 주변) |
| [Faymere River Village](https://www.fab.com/listings/6e579cff-a28f-499b-a0ad-30649d40774a) (karaman) | 영구 | Fab Std | 5.6~5.7 | 중 | 강가 중세 마을 — 무너진 부두 · 헛간 |
| [Modular Rural Cabins](https://www.fab.com/listings/508fe84a-4976-4cfe-9a40-c2b9533da601) (Maarten Hof) | 영구 | Fab Std | 4.22~5.8 | 중 | 모듈 목조 오두막. 평점 4.8(82) |
| [Free Stylized Stone Wall 01A - Material](https://www.fab.com/listings/bcbc00a4-48e2-402a-a907-1ae1f6a20b4f) (LarkArt Store) | 영구 | Fab Std | 5.0~5.8 | 중 | 스타일라이즈드 마름돌 벽 재질(C21 · T01 재질 참고) |
| [Ancient Ruins – Free Asset Pack (26 Pieces)](https://www.fab.com/listings/c6bd4209-8420-499f-99b7-f47b8492b2d1) (Hyperdense) | 영구 | CC BY 4.0 | 5.4~5.5 | 하 | **AI 생성 표시**. 붉은 사암 폐허 조각 26 |
| [FANTASTIC - Village Pack](https://www.fab.com/listings/52529a12-e88e-41a0-8834-b87306f20c24) (Tidal Flask Studios) | 영구 | Fab Std | 4.22~5.8 | 하 | 만화풍 마을. 평점 4.8(94) |
| [Stylized Medieval City](https://www.fab.com/listings/1474be0c-6f51-44ad-a44f-7146ae42a298) (Baturinets) | 영구 | Fab Std | 4.25~5.4 | 하 | 만화풍 |
| [Medieval Farm Barn – Windmill, Silo](https://www.fab.com/listings/3d4cf892-0971-49a8-b910-20b08e080816) (GESxQbee Studio) | 개인만 | Fab Std | 5.0~5.5 | 하 | 풍차 · 곳간 44메시, 만화풍 |

## 소품 · 캠프 · 마법 소품 — C27~C43

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [EnchantedPotionRoom_Free](https://www.fab.com/listings/6bee477a-f8a8-42d5-b987-c6937305ab58) (StellarMagic) | 영구 | Fab Std | 5.0~5.7 | 중 | 물약병 · 책 · 가마솥 · 약초 등 91모델 + 아이콘 71. 아늑한 마법 방 — 탑 주방 · 서고 소품 |
| [POLY - Medieval Camp](https://www.fab.com/listings/436d467a-6955-4aac-be0d-a05c99966ea2) (Animpic Studio) | 영구 | Fab Std | 4.24~5.7 | 하 | 캠프 74모델(천막 · 무기 · 소품). 로우 폴리 |
| [Stick Together FREE - Campfire, Axe, Log and Camp Food](https://www.fab.com/listings/afcaf183-4a9f-4a68-8256-94dfe7d7baa9) (RetroStyle Games) | 영구 | Fab Std | 5.8 | 중 | 불 · 연기 있는 모닥불 · 앉는 통나무 · 도끼 · 음식 · 덤불 2. C35 · C11 |
| [Medieval Campfire](https://www.fab.com/listings/f620242e-35cc-4708-b4ac-ff3cc75e1eb7) (Zombified Asset Studios) | 영구 | Fab Std | 4.27~5.8 | 중 | **AI 생성 표시**. 돌 고리 모닥불(C35) |
| [GanzSe FREE Camping - Fantasy Low Poly Props](https://www.fab.com/listings/d03138fe-7e29-4b3d-9ea8-8ba67f24f845) (GanzSe) | 영구 | CC BY 4.0 | 5.4~5.8 | 하 | 로우 폴리 캠프 소품 |
| [Medieval Burlap Sack Pack](https://www.fab.com/listings/9cbc5e08-eb36-4c0c-b96c-90b03e46a73a) (Animaster Studio) | 영구 | CC BY 4.0 | 5.3~5.8 | 중 | 자루 15(C29) |
| [Medieval Wooden Pottery Props Pack](https://www.fab.com/listings/072f3d98-e7aa-4aaf-a206-bb7b0802150a) (Animaster Studio) | 영구 | CC BY 4.0 | 5.3~5.8 | 중 | 나무 그릇 · 통 · 숟가락 25(C28 · C30) |
| [Medieval Blacksmith Set](https://www.fab.com/listings/4962b847-825e-4be8-8ab4-f61e9f88ac57) (J-M-Muzio) | 영구 | Fab Std | 5.8 | 중 | 모루 · 숫돌 · 작업대 11 |
| [Gothic props (pack 1)](https://www.fab.com/listings/7727a384-ec28-4773-a1da-876ad0b49457) (E.Bolday) | 영구 | CC BY 4.0 | 5.4~5.8 | 중 | 고딕 문 · 철물 등 21 |
| [Medieval Banquet](https://www.fab.com/listings/d422ac6c-ce50-4273-8d4a-2ac594a3a2b2) (Quixel Megascans) | 영구 | Fab Std | 5.4~5.8 | 중 | 연회 식탁 · 음식 · 식기 — 주방 소품 |
| [Old Mine](https://www.fab.com/listings/dc4dc139-f3a3-4375-875f-d1831506953e) (Quixel Megascans) | 영구 | Fab Std | 5.4~5.8 | 중 | 광산 갱도 · 수레 — 광석 노두(C06) 주변 |
| [Stone well](https://www.fab.com/listings/848faec1-a33c-4263-b531-42ddeae02539) (Procedural) | 영구 | Fab Std | 5.4~5.5 | 중 | 나무 지붕 돌 우물(tower-yard) |
| [Nordic Well](https://www.fab.com/listings/0de5240b-9012-4ace-8bdd-12620d0aa70c) (A.S.G - Creative) | 영구 | Fab Std | 5.4~5.5 | 중 | 도르래 우물 |
| [Alpine Village Props - Free Sample](https://www.fab.com/listings/7eb5ddfe-a210-440d-ac18-822efe0be095) (Maxlworks) | 영구 | Fab Std | 5.8 | 중 | 18메시, Nanite 예. 장작 · 나무 소품 |
| [Old West - VOL 2 - Tradesmen Tools and Props](https://www.fab.com/listings/e9700286-334e-4230-9de4-2d8db738fdeb) (Dekogon Studios) | 영구 | Fab Std | 4.27~5.8 | — | 공구 · 수레 소품. 서부 시대라 골라 쓴다(썸네일 안 봄) |
| [Old Furniture Pack](https://www.fab.com/listings/2e77512a-87a4-4992-8d80-02f0e8d07b7f) (BB Arts) | 영구 | CC BY 4.0 | 5.4~5.7 | 중 | 낡은 가구 · 책장(T11) |
| [Barrel: Stylized Wooden](https://www.fab.com/listings/870ce80d-e31a-4251-8307-9c07839ea3ae) (Cramb Studio) | 영구 | Fab Std | 5.0~5.7 | 중 | 나무 통(C28) |
| [Telescope — Static Astronomy Prop](https://www.fab.com/listings/eb0d345d-a808-4ca3-a00d-57122422a514) (David-SDUK) | 영구 | CC BY 4.0 | 5.8 | 하 | **AI 생성 표시**. 현대식 삼각대 망원경이라 놋쇠 망원경(poi-s4)과 다르다 |
| [Stylized Dungeon Props Pack](https://www.fab.com/listings/ddd47def-2332-4485-b399-952c4e69a569) (J's 3D) | 영구 | Fab Std | 5.6 | 하 | 통 · 상자 · 항아리 · 샹들리에, 만화풍 |
| [Magic Potion Free](https://www.fab.com/listings/2ccf2dbb-913b-45f4-808a-3336e0ee83c4) (BreeIri) | 영구 | Fab Std | 5.4 | — | 물약병 하나(썸네일 안 봄) |
| [Stylized Library — 48 Cozy Interior Props](https://www.fab.com/listings/6150cf8e-8105-4da0-9209-3edbc2afb673) (Daria Borovleva) | 개인만 | Fab Std | 5.7 | 하 | 책 · 촛대 · 의자 등 48. 귀여운 만화풍. 평점 5(10) |
| [Torn Banner Free](https://www.fab.com/listings/a95621bb-bc4c-4dc9-b45d-0af213ea89d7) (DzenGame) | 개인만 | Fab Std | 5.5~5.8 | 중 | 찢어진 깃발 · 배너 7(C39 · T07) |

## 실내

들어갈 수 있는 건물이 많으므로 실내 꾸밈을 따로 모았다. 먼저 건물마다 필요한 것을 정리하고(근거: [콘셉트 아트](./region1-concepts)의 실내 그림 · `Scripts/Tutorial/tower_greybox.py` 의 방 배치 · [필요한 3D 모델](./region1-assets)의 방별 표), 그다음 종류별 무료 후보를 적는다.

### 건물별로 필요한 것

| 건물 | 방 | 필요한 것 |
|---|---|---|
| 마법탑 1층 | 시작 방 · 숙소 | 1인 침대 · 책상 · 의자 · 양초 무리 · 옷궤 · 트렁크 · 옷걸이 랙 · 바구니 · 벽 선반 · 러그 · 둥근 창 |
| | 주방 | 돌 아궁이 · 무쇠 솥 · 긴 조리대 · 식료품 선반 · 냄비 걸이 · 채소 바구니 · 항아리 · 물통 · 도마 · 매단 허브 |
| | 홀 | 원형 기둥 · 큰 아치 창 · 나선 계단 · 퀘스트 게시판 · 강의단 · 창가 화단 · 배너 · 바닥 마법진 |
| | 서고 | 높은 책장 모듈 · 레일 사다리 · 독서대 · 두루마리 통 · 잉크병 · 책 더미 · 촛대 |
| | 수련실 · 시험의 방 | 허수아비 · 과녁 · 모래 상자 · 물 양동이 · 속성 받침대 · 시험관 의자 · 랜턴 기둥 |
| 마법탑 위층(계획) | 스승 방 · 연구실 · 관측실 | 큰 책상 · 안락의자 · 침대 · 혼천의 · 지구의 · 망원경 · 수정 · 물약 선반 · 별 지도 |
| 풍차 | 방아실 · 회랑 | 맷돌 · 호퍼 · 큰 나무 톱니바퀴 · 수직 축 · 자루 · 사다리 · 나무 마루 · 원형 석벽 |
| 마을 집 · 오두막 · 곳간 | 방 하나 ~ 둘 | 침대 · 탁자 · 의자 · 화덕 · 솥 · 궤짝 · 선반 · 통 · 바구니 · 장작 · 러그 |
| 폐허 · 분탑 | 관측소 안 · 감시탑 초소 · 지하 저장고 | 무너진 들보 · 깨진 창틀 · 작업대 · 지도 탁자 · 무기 걸이 · 돌 벽난로 · 선반 위 병 · 통 · 자루 |

### 모듈식 실내 키트 · 건축 내장

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Medieval Room Kit](https://www.fab.com/listings/52610433-0210-40e5-884b-abc12cf8901e) (1Luka2) | 영구 | CC BY 4.0 | 5.0~5.8 | 중 | **손그림 모듈 48** — 벽 · 바닥 · 창 · 문 · 가구 · 벽난로로 방을 짜는 키트. 마을 집 · 탑 방 그레이박스를 바로 바꿀 수 있다 |
| [Megascans: Historic Medieval Church Ruin Wall](https://www.fab.com/listings/7546d222-b9e8-482e-b257-df49046de573) · [Arch](https://www.fab.com/listings/68241c49-88a0-4382-888f-7c530f8c776c) · [Window](https://www.fab.com/listings/5b8e2348-6646-4cac-bc5c-f15fd2205326) · [Buttress](https://www.fab.com/listings/5fbae5ce-6f72-4dd4-87cd-4898b643df27) · [Floor Slab](https://www.fab.com/listings/25aa65b9-edf1-4bd2-abf3-799cd202fae0) | 영구 | Fab Std | Fab 플러그인 | 중 | 교회 폐허 모듈 연작. 폐허 내부 · 관측소 · 감시탑 · 탑 고딕 창 틀(T04) |
| [Megascans: Historic Medieval Castle Arch](https://www.fab.com/listings/814ec870-4397-4222-9eab-0db1b40dcc7b) · [Doorway](https://www.fab.com/listings/68876642-ed5f-46e9-b704-ec925d56aeda) · [Door](https://www.fab.com/listings/8aa70f74-dc9d-485a-81e5-f61fa328f052) · [Pillar](https://www.fab.com/listings/9cddffbc-5658-4cf0-9623-b0324bcae066) | 영구 | Fab Std | Fab 플러그인 | 중 | 성 아치 · 문틀 · 쇠 장식 문 · 돌기둥(T06 · C24) |
| [Megascans: Modular Wooden Staircase](https://www.fab.com/listings/738e3632-d178-428b-a591-108d4e9f3851) | 영구 | Fab Std | Fab 플러그인 | 중 | 나무 계단 · 난간 — 풍차 회랑 · 오두막 다락. 나선 계단은 없음 |
| [Megascans: Modular Wooden Door](https://www.fab.com/listings/7d3d5393-e57e-49c9-bfa9-9a154a0f0b04) | 영구 | Fab Std | Fab 플러그인 | 중 | 조각 장식 나무 쌍문 |
| [Megascans: Modular Wooden Ladder](https://www.fab.com/listings/e218c1c0-8aaf-49da-a183-c8f32e2394a0) | 영구 | Fab Std | Fab 플러그인 | 중 | 방아실 사다리 |
| [Megascans: Old Wooden Beam](https://www.fab.com/listings/13d1a4c3-1f5b-4e85-85bd-a5a343c1620f) | 영구 | Fab Std | Fab 플러그인 | 중 | 천장 들보(T03) · 무너진 들보(C22) |
| [STYLIZED Door Dungeon (Modular pack)](https://www.fab.com/listings/70011964-a923-484e-988c-40b9acba610a) (Iinmost) | 영구 | Fab Std | 5.3~5.6 | 중 | 돌 아치 틀 + 나무 · 쇠 문 — 지하 저장고 · 탑 방 문 |
| [Stone Tiles Material Pack](https://www.fab.com/listings/ae84413a-31e5-40b7-a840-26279a3deea2) (JulioSillet) | 영구 | Fab Std | 5.0~5.8 | 중 | 돌 바닥 · 벽 재질 9종(4K) — T02 판석 |
| [Medieval Dungeon](https://www.fab.com/listings/c13bd0dc-ac4d-4595-b284-f81386b2e6ef) (Infuse Studio) | 영구 | **UE 전용**(옛 마켓플레이스) | 4.21~5.4 | 중 | 지하 감옥 · 묘실 모듈. 평점 4.7(236). 지하 저장고 · 폐허 지하 |
| [Dungeon Environment / 135+ Assets](https://www.fab.com/listings/bb39bae4-7f7a-4127-b07e-151cf52db0f6) (PackDev) | 영구 | Fab Std | 4.26~5.4 | 중 | 어두운 석조 실내 + 소품 135+ |
| [Saloon Interior](https://www.fab.com/listings/57991a62-f98f-4c6a-9b94-a74ec12f242e) (Quixel Megascans) | 영구 | Fab Std | 5.4~5.8 | 중 | 서부 술집 모듈 실내. 나무 벽 · 마루 · 계단 · 탁자는 오두막 · 곳간에 골라 쓴다 |
| [Medieval Log Cabins](https://www.fab.com/listings/3c94b681-6cdb-4fd7-9366-4731c46ed1bd) (Polivantage) | 영구 | CC BY 4.0 | 5.6~5.7 | 중 | 실내 있는 통나무집 4채 |
| [Modular Housing Basic](https://www.fab.com/listings/9e7a1479-01e2-45cc-8898-590445d538f6) (imbrock) | 영구 | Fab Std | 5.6 | 하 | 1 m 격자 회색 집 키트 — 그레이박스용 |
| [Wooden Building Collection Tier 1](https://www.fab.com/listings/34028b89-8171-4b0b-968d-09003b804fb3) (ArtStrong Gaming) | 영구 | Fab Std | 5.0 | 하 | 나무 벽 · 문 11 |
| [Modular Fantasy Castle](https://www.fab.com/listings/6c8fd8d8-9295-4844-8466-0a9096e8872c) (Forge of Fantasy) | 영구 | CC BY 4.0 | 4.24~5.4 | 하 | 로우 폴리 성 모듈 |

### 가구 · 부엌 · 생활

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Megascans: Residential European Rural Furniture Bed](https://www.fab.com/listings/89b48726-c79c-460a-8aa1-5bbd2eaa7fbd) | 영구 | Fab Std | Fab 플러그인 | 중 | 나무 1인 침대 — 시작 방 · 숙소 · 오두막 |
| [Megascans: Historic Medieval Library Table](https://www.fab.com/listings/190c5e36-e136-4e30-8480-184448601fd4) · [Chair](https://www.fab.com/listings/96a694dc-46e8-4894-a4b5-66f764e42672) · [Stool](https://www.fab.com/listings/5863f916-6fe3-4de0-9410-25f020a6906c) · [Bench](https://www.fab.com/listings/03c43f08-9d3d-47cc-86b5-f2b7b938c136) · [Stand](https://www.fab.com/listings/9b10ae3d-cd17-4bd9-a33e-f95e53cea82f) | 영구 | Fab Std | Fab 플러그인 | 중 | 서고 가구 연작(T11). 같은 연작의 탁자 Old 02 는 유료 |
| [Megascans: Historic Medieval Castle Fireplace](https://www.fab.com/listings/745bb6fa-0704-4235-971c-7eb006410d9e) · [Farm Fireplace Stone 01](https://www.fab.com/listings/2a9894f9-f98a-46c2-9a0b-da3d1b2ac212) | 영구 | Fab Std | Fab 플러그인 | 중 | 돌 벽난로 · 농가 화덕 — 주방 아궁이 · 초소 벽난로 |
| [Megascans: Historic Medieval Farm Oven Clay](https://www.fab.com/listings/1dce3710-b51f-44e5-a4bd-6d8facba5b81) | 영구 | Fab Std | Fab 플러그인 | 중 | 흙 빵 화덕 — 마을 집 |
| [Megascans: Historic Medieval Farm Cauldron Iron 05](https://www.fab.com/listings/a65ea51a-2266-4cf7-a67e-6871adb205d2) | 영구 | Fab Std | Fab 플러그인 | 중 | 무쇠 솥(주방 핵심). 같은 연작 01 · 02 · 04 는 유료 |
| [Megascans: Historic Medieval Farm Stand Brazier](https://www.fab.com/listings/962e6b9a-0423-4c00-ae05-9f192a4311aa) | 영구 | Fab Std | Fab 플러그인 | 중 | 화로 받침 — 수련실 속성 화로 · 초소 화로 |
| [Megascans: Historic Medieval Farm Chest](https://www.fab.com/listings/140b25c2-049d-4cff-bece-ff8fd93cf838) · [Barrel](https://www.fab.com/listings/501609b5-2fb2-4de2-ae91-13e98f4dc9a6) · [Sack](https://www.fab.com/listings/8e88e206-341b-4ce5-a790-5e66b6fb37a7) · [Basket](https://www.fab.com/listings/b20d9eac-bfcc-46e5-872d-9af61de59761) · [Jug Clay](https://www.fab.com/listings/6fc32b15-6a6f-4e88-9af5-76425bdbc5ae) · [Log Pile](https://www.fab.com/listings/3a772086-a54b-40ba-8cb1-a3b0d1bba50d) | 영구 | Fab Std | Fab 플러그인 | 중 | 궤짝 · 통 · 자루 · 바구니 · 질그릇 · 장작(C27~C30 · C38) |
| [Megascans: Old Wooden Chest](https://www.fab.com/listings/27d16089-3202-4a4c-a0b4-e3e7c60720df) · [Wooden Shelf](https://www.fab.com/listings/7888c588-0240-4f4e-b2c8-78b96548b6ba) | 영구 | Fab Std | Fab 플러그인 | 중 | 녹색 칠 궤짝 · 벽 선반(T10) |
| [Medieval Banquet](https://www.fab.com/listings/d422ac6c-ce50-4273-8d4a-2ac594a3a2b2) (Quixel Megascans) | 영구 | Fab Std | 5.4~5.8 | 중 | 연회 식탁 · 음식 · 식기 |
| [Medieval House Props Vol 2 – Furniture & Household](https://www.fab.com/listings/53b5afa7-de38-4034-bd13-cbb92cc883d1) (GESxQbee Studio) | 개인만 | Fab Std | 5.0~5.6 | 중 | 침대 · 장 · 탁자 · 의자 · 생활 소품. 따뜻한 나무톤 |
| [Medieval Village Props – Furniture, Containers & Clutter](https://www.fab.com/listings/b8fc5a33-8dce-48d1-8175-1015aa4c9436) (DuplexG) | 개인만 | Fab Std | 5.3~5.7 | 중 | 가구 · 용기 · 잡동사니 70 |
| [Old West VOL.1 - Interior Furniture](https://www.fab.com/listings/c65a8dd1-ec4c-409c-9919-5eda4f764772) (Dekogon Studios) | 영구 | Fab Std | 4.27~5.8 | 중 | 침대 · 장 · 책상 · 선반. 서부풍이라 골라 쓴다 |
| [Old West - VOL 4 - Goods and Supplies](https://www.fab.com/listings/7ec26cda-15cf-4b62-ac9b-45f156676e51) (Dekogon Studios) | 영구 | Fab Std | 4.27~5.8 | 중 | 자루 · 단지 · 식료품 — 식료품 선반 · 저장고 |
| [Stylized medieval Tavern](https://www.fab.com/listings/31a8cf60-40e2-4df1-894c-75ce9feb2ddb) (Baturinets) | 영구 | Fab Std | 4.27~5.4 | 중 | 스타일라이즈드 선술집 실내 |
| [Stylized Kitchen Set](https://www.fab.com/listings/08dcd6bf-ac70-40b2-855f-ffc00a7ed9f8) (Baturinets) | 영구 | Fab Std | 4.27~5.4 | 중 | 중세 부엌 — 탁자 · 의자 · 통 · 양동이 · 냄비 |
| [5 Stylized Stools](https://www.fab.com/listings/9ff152ce-4077-4ce3-ae7b-3ac7cb70125c) (Hekos Studio) | 영구 | Fab Std | 4.27~5.6 | 중 | 나무 걸상 5 |
| [Wood Chest-01](https://www.fab.com/listings/7ebf7238-c4d9-4c89-bd51-41920bb3ca50) (hazalart) | 영구 | Fab Std | 5.1~5.5 | 중 | 나무 궤 |
| [Stylized House Interior](https://www.fab.com/listings/ab92e5d3-6db6-4cf3-bff5-c2c98ae8db5b) (StylArts) | 영구 | Fab Std | 4.25~5.8 | 하 | 만화풍 현대 부엌. 목록 데이터에 할인 종료일(2026-11-08)이 붙어 있으나 정가도 0원 |

### 마법 소품

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [EnchantedPotionRoom_Free](https://www.fab.com/listings/6bee477a-f8a8-42d5-b987-c6937305ab58) (StellarMagic) | 영구 | Fab Std | 5.0~5.7 | 중 | 물약병 · 책 · 가마솥 · 약초 91 — 연구실 · 주방 |
| [Megascans: Historic Medieval Library Scroll](https://www.fab.com/listings/961b7710-b373-4efc-9af2-5634f60b35dc) · [Book](https://www.fab.com/listings/97be92ab-9e71-4cd8-b401-a9826464ca2a) | 영구 | Fab Std | Fab 플러그인 | 중 | 두루마리 · 옛 책(C40 · C43). 연작 일부는 유료 |
| [Fantasy Bottle – Stylized Glass, Copper & Cork](https://www.fab.com/listings/babbe7b4-6429-479a-8cf9-0548c7bcb0df) (Luciana Lancaster) | 영구 | Fab Std | 5.8 | 상 | 구리 받침 유리 물약병, Nanite 예 |
| [Stylized Crystal Pack](https://www.fab.com/listings/ac4247b8-e7b8-4826-92b2-1cf5ebfedeac) (sensh1) | 영구 | CC BY 4.0 | 5.6~5.7 | 중 | 빛나는 결정 9 — 마나 결정 · 수정 |
| [Vintage Study](https://www.fab.com/listings/ff76f755-0c45-4913-a872-7cd5d2e43622) (Coded Arts Studios) | 개인만 | Fab Std | 5.5~5.7 | 중 | 서재 소품 — 지구의 · 책 · 안락의자. 빅토리아풍 |
| [Misc Props - UB Free Pack](https://www.fab.com/listings/e5bb46ab-edf1-4530-8797-703f4c98b4a6) (UB) | 개인만 | Fab Std | 4.26~5.8 | 중 | 잡소품 233메시(촛대 · 지구의 · 시계 등 섞임) |
| [Last Wizard FREE - Stylized Castle Room](https://www.fab.com/listings/6c0c7f4f-f872-476b-82d7-b5bcf417adff) | 영구 | Fab Std | 5.8 | 상 | 위 「돌 건축」 표 참고 — 성 방 한 칸 |

### 조명 · 직물

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Megascans: Historic Medieval Library Chandelier](https://www.fab.com/listings/51879e64-6d4d-453b-8369-bbb977f9f1b7) | 영구 | Fab Std | Fab 플러그인 | 중 | 쇠 촛대 샹들리에(T09) |
| [Megascans: Historic Medieval Library Torch](https://www.fab.com/listings/0be0490e-0324-47b5-b931-63d26855171d) · [Farm Candleholder](https://www.fab.com/listings/78674cbd-d699-44b9-a28e-40f5cbea288a) | 영구 | Fab Std | Fab 플러그인 | 중 | 벽 횃불 걸이 · 은 촛대(T10 · C41) |
| [Pirate chandelier](https://www.fab.com/listings/8d60e4f6-2823-408e-ad64-1a07f8d5ce82) (Haridon) | 영구 | CC BY 4.0 | 4.18~5.7 | 중 | 바퀴형 촛대 샹들리에 |
| [Free Torch Fire](https://www.fab.com/listings/7437ae0c-d67c-4f0a-af81-d03774cfdb82) (CodePhase Games) | 영구 | Fab Std | 5.1~5.8 | 중 | 횃불 불꽃 나이아가라(메시 아님) |
| [Megascans: Historic Bazaar Rug Fabric Old 06](https://www.fab.com/listings/05d847c0-7ab9-435c-8ca6-d18216b01c17) | 영구 | Fab Std | Fab 플러그인 | 중 | 접은 양탄자. 연작 여러 장이 0원(일부 유료) |
| [Torn Banner Free](https://www.fab.com/listings/a95621bb-bc4c-4dc9-b45d-0af213ea89d7) | 개인만 | Fab Std | 5.5~5.8 | 중 | 위 「소품」 표 참고 |

### 풍차 기계 장치

| 이름 | 무료 종류 | 라이선스 | UE | 스타일 | 메모 |
|---|---|---|---|---|---|
| [Megascans: Historic Medieval Farm Millstone S 01](https://www.fab.com/listings/53db74ab-0ec0-458b-b52c-52260af512ed) | 영구 | Fab Std | Fab 플러그인 | 중 | 돌 맷돌. S 03 도 0원, S 02 는 유료 |
| [Megascans: Historic Medieval Farm Cart](https://www.fab.com/listings/2d8a2fde-c4c5-449b-8458-cc556c11baba) | 영구 | Fab Std | Fab 플러그인 | 중 | 방앗간 마당 수레(C36) |

큰 나무 톱니바퀴 · 수직 축 · 호퍼는 무료로 맞는 것이 없다.

### 실내 빈 곳

무료로 맞는 것을 못 찾아 계획대로 만드는 것:

- **탑 구조**: 곡면 석벽(T01) · 둥근 창(T05) · 나선 돌계단 · 고딕 늑골 천장 · 놋쇠 띠 기둥 · 납 유리 창.
- **마법 기구**: 놋쇠 혼천의(C42) · 지구의(개인만 무료인 Vintage Study · UB 팩 안에만 있고 스타일이 다르다) · 큰 놋쇠 망원경 · 별 지도.
- **서고**: 아치 상단 높은 책장 모듈(무료 책장은 현대식뿐) · 레일 사다리 · 독서대 · 잉크병 · 깃펜.
- **탑 방 고유 소품**: 퀘스트 게시판 · 강의단 · 창가 화단 · 옷걸이 랙과 로브 · 짚 허수아비 · 마법 과녁 · 과녁 레일 · 모래 상자 · 속성 받침대 · 시험관 의자.
- **풍차**: 큰 나무 톱니바퀴(수직 · 수평) · 수직 축 · 호퍼.
- **직물**: 커튼 · 늘어진 휘장 · 남색 서클 배너(배너는 색만 바꿔 쓸 수 있는 것이 개인만 무료).

## 에픽 공식 · Quixel 상태 (2026-11-07 확인)

### Quixel Megascans

- 2024년 말로 「전부 무료」가 끝났고 지금은 **항목마다 다르다.** 판매자 「Quixel Megascans」 목록 표본(처음 24개)에서 15개가 개인 · 기업 모두 0원, 9개는 개인 약 $1.99~$3.99(예: [Nordic Forest Cluster Rock Small](https://www.fab.com/listings/f186c225-178a-482b-a122-5508a318a4f0) — 개인 ₩4,077 · 기업 ₩40,894, UEFN 참조 전용만 0원).
- Fab 검색의 「무료」 필터에는 **UEFN 참조 전용만 0원인 유료 항목도 섞여 나온다.** UEFN 참조 전용은 원본 파일을 주지 않으므로 UE5 프로젝트에 못 쓴다. 상세 페이지에서 「개인」 등급이 0원인지 본다.
- **같은 이름의 무료 · 유료가 공존한다**(예: 「Sharp Cliff」 둘). 이 문서의 링크로 받는다.
- Megascans 낱개는 언리얼 형식 대신 FBX · glTF 로 올라와 있다 → 에디터 안 Fab 플러그인으로 받아 넣는다(표의 「Fab 플러그인」).
- 숲 · 바위 · 식물 범주에 무료 낱개가 수백 개 있다(검색 결과가 5,000개 한도에서 잘려 정확한 수는 셀 수 없었다).
- **Megaplants**(절차적 Nanite 나무 · 식물, UE 5.7+)는 40종 전부 0원이다. 「Procedural Vegetation Editor」 플러그인을 켜야 들어온다.

### 기간 한정 무료(옛 「월간 무료」)

- Fab 첫 화면 「기간 한정 무료」 묶음이 정해진 마감일까지 0원이다. 지금 묶음은 **2026-11-17 23:59(한국 시각)까지 3개**: [Underground Tunnel](https://www.fab.com/listings/6c9e3f18-0b5e-4240-9aea-eecb515175c1)(모듈 지하 터널, UE 4.26~5.8, 정가 개인 ₩136,345) · [SciFi Beast06 Bull](https://www.fab.com/listings/126120a3-996e-4dc3-94f1-6550f0adac03) · 3D Platformer Game Bundle. 자연물은 없고, Underground Tunnel 만 지하 저장고(poi-s8-cellar) 참고가 된다.
- 마감 전에 받아 두면 내 라이브러리에 남는다. 다음 묶음은 첫 화면에서 다시 확인한다.
- 옛 UE 마켓플레이스 「Permanent Collection」 항목(Water Materials · Good SKY 등)은 지금도 영구 무료다.

### 에픽 공식 무료 샘플 (자연물이 들어 있는 것)

| 이름 | UE | 스타일 | 쓸 수 있는 것 |
|---|---|---|---|
| [Open World Demo Collection](https://www.fab.com/listings/3262ab8f-f64a-4124-8efd-82cb19df6249) | 4.7~**5.2** | 중 | 스코틀랜드 고지 나무 · 풀 · 바위 · 지형(「소년과 연」 데모 에셋). 평점 5(22) |
| [Electric Dreams Env](https://www.fab.com/listings/d79688f5-29be-4fb2-a650-2d4a813f5306) | 5.2~5.8 | 중 | 울창한 숲 · 이끼 바위 · 절벽, PCG 예제 |
| [Valley of the Ancient](https://www.fab.com/listings/0c19880e-21bd-42ba-8287-1caccc3951b1) | 5.0~5.7 | 중 | 사막 협곡 절벽 · 바위(Nanite). 지형이 달라 바위 조각만 |
| [Paragon: Agora and Monolith Environment](https://www.fab.com/listings/6f401fb5-88b5-41b4-bf1b-62321414e1f0) | 4.19~5.8 | 중 | 판타지 숲 · 물가 · 바위 · 석조 폐허 |
| [Soul: Cave](https://www.fab.com/listings/75f42402-40bb-4a1b-b557-18e2c9604273) | 4.18~5.8 | 중 | 동굴 · 바위 — 폭포 뒤 던전 입구 |

에픽 콘텐츠는 Fab Standard License 다(위 다섯 모두 개인 · 기업 0원).

## 먼저 받을 것 (추천 17)

바깥 12 + 실내 5. 실내 몫은 Megascans 낱개가 많아 연작 단위로 묶었다 — 링크 하나씩 받는다.

1. **[기간 한정 · 11-17 23:59 까지]** [Underground Tunnel](https://www.fab.com/listings/6c9e3f18-0b5e-4240-9aea-eecb515175c1) — 자연물은 아니지만 정가가 높고 지하 저장고 참고가 된다. 필요 없으면 건너뛴다.
2. [FREE Stylized Forest Sample](https://www.fab.com/listings/3c1a31b6-e523-4a0f-97d0-26b0db69b6bc) — 스타일 상. 나무 · 덤불 · 풀 · 바위 한 번에.
3. [FREE Stylized Foliage](https://www.fab.com/listings/b494d1c5-8d02-48c4-9038-54bb8970a26f) — 스타일 상. 풀 · 꽃(C12 · C13).
4. [FREE Stylized Rocks](https://www.fab.com/listings/3edeea52-d490-4ffd-8b51-70376f5ec82d) — 스타일 상. 바위(C01 · C04).
5. [Megaplants: English Oak](https://www.fab.com/listings/83642c38-7661-4df1-8629-0422e1898d26) · [European Hornbeam](https://www.fab.com/listings/43679e75-6ed0-4b03-9288-9e2bc49376d2) · [Bracken Fern](https://www.fab.com/listings/4d60e635-7a49-4f19-818c-d018046be39f) — UE 5.8 그대로, Nanite 폴리지. 활엽수 C07 의 형태 기준.
6. [temperate Vegetation: Foliage Collection](https://www.fab.com/listings/6a5ae8db-d80f-4b23-b276-87da390cfe56) — 풀 · 바닥 식생 · 들꽃 277묶음 + 지형 재질(5.4 로 받아 옮김).
7. [Stylized Painterly Rock Pack](https://www.fab.com/listings/7dcf8d2d-7b03-46a0-acc0-497e6735b05a) — 붓 터치 바위 재질 구조를 우리 마스터 재질에 참고.
8. [Dark Ruins Megascans Sample](https://www.fab.com/listings/836ed2f8-e2d6-49be-98d3-59d104bd351e) · [Maridalen Ruins](https://www.fab.com/listings/1bb299e1-fc4c-4115-b39a-dde723288d17) — 절벽 · 폐허 석벽.
9. [Last Wizard FREE - Stylized Castle Room](https://www.fab.com/listings/6c0c7f4f-f872-476b-82d7-b5bcf417adff) — 탑 실내 벽난로 · 배너 · 횃불 · 상자.
10. [EnchantedPotionRoom_Free](https://www.fab.com/listings/6bee477a-f8a8-42d5-b987-c6937305ab58) — 마법 방 소품 91.
11. [Stick Together FREE - Campfire](https://www.fab.com/listings/afcaf183-4a9f-4a68-8256-94dfe7d7baa9) — 캠프 · 모닥불(C35).
12. [Water Materials](https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861)(CC BY) · [Free Hand-Painted Stylized Clouds](https://www.fab.com/listings/e07ed1a5-341a-49bb-810d-a4e54bad14da) · [MW Landscape Auto Material](https://www.fab.com/listings/6602874e-ef24-48c9-9055-a7ac07384696) — 물 · 하늘 · 지형 재질 출발점.
13. **실내** [Medieval Room Kit](https://www.fab.com/listings/52610433-0210-40e5-884b-abc12cf8901e)(CC BY) — 손그림 모듈 48로 방 벽 · 바닥 · 창 · 문 · 가구를 한 번에. 마을 집 · 탑 방 그레이박스 교체 1순위.
14. **실내** Megascans 서고 연작 — [샹들리에](https://www.fab.com/listings/51879e64-6d4d-453b-8369-bbb977f9f1b7) · [탁자](https://www.fab.com/listings/190c5e36-e136-4e30-8480-184448601fd4) · [의자](https://www.fab.com/listings/96a694dc-46e8-4894-a4b5-66f764e42672) · [걸상](https://www.fab.com/listings/5863f916-6fe3-4de0-9410-25f020a6906c) · [두루마리](https://www.fab.com/listings/961b7710-b373-4efc-9af2-5634f60b35dc) · [책](https://www.fab.com/listings/97be92ab-9e71-4cd8-b401-a9826464ca2a) · [횃불](https://www.fab.com/listings/0be0490e-0324-47b5-b931-63d26855171d).
15. **실내** Megascans 부엌 · 생활 — [성 벽난로](https://www.fab.com/listings/745bb6fa-0704-4235-971c-7eb006410d9e) · [무쇠 솥](https://www.fab.com/listings/a65ea51a-2266-4cf7-a67e-6871adb205d2) · [침대](https://www.fab.com/listings/89b48726-c79c-460a-8aa1-5bbd2eaa7fbd) · [양탄자](https://www.fab.com/listings/05d847c0-7ab9-435c-8ca6-d18216b01c17) · [나무 계단](https://www.fab.com/listings/738e3632-d178-428b-a591-108d4e9f3851) · [농가 궤짝](https://www.fab.com/listings/140b25c2-049d-4cff-bece-ff8fd93cf838) · [통](https://www.fab.com/listings/501609b5-2fb2-4de2-ae91-13e98f4dc9a6) · [자루](https://www.fab.com/listings/8e88e206-341b-4ce5-a790-5e66b6fb37a7) · [바구니](https://www.fab.com/listings/b20d9eac-bfcc-46e5-872d-9af61de59761).
16. **실내 · 폐허** Megascans 교회 폐허 연작 — [벽](https://www.fab.com/listings/7546d222-b9e8-482e-b257-df49046de573) · [아치](https://www.fab.com/listings/68241c49-88a0-4382-888f-7c530f8c776c) · [창](https://www.fab.com/listings/5b8e2348-6646-4cac-bc5c-f15fd2205326) · [바닥 판석](https://www.fab.com/listings/25aa65b9-edf1-4bd2-abf3-799cd202fae0). 관측소 · 감시탑 · 폐허 내부와 탑 고딕 창 틀.
17. **실내 · 풍차** [Megascans 맷돌](https://www.fab.com/listings/53db74ab-0ec0-458b-b52c-52260af512ed) + [Medieval House Props Vol 2](https://www.fab.com/listings/53b5afa7-de38-4034-bd13-cbb92cc883d1)(개인만 — 침대 · 장 · 탁자, 자격 있을 때 받아 둔다).

## 라이선스 주의

- **개인(Personal) 등급 자격**: Fab EULA 기준 — 받는 시점에 본인(과 지배 · 공동 지배 관계 법인)이 최근 12개월 디지털 콘텐츠 사업 총매출 **10만 달러 이하**(투자 · 선급금 포함). 지금 이 프로젝트는 해당한다. 받은 뒤 매출이 넘어도 기업 등급으로 올릴 필요는 없다.
- **두 등급의 권리는 같다**: 상업 게임 배포 가능, 수정 가능, 크레딧 불필요, 같이 작업하는 사람과 공유 가능. 금지: 에셋 자체를 따로 팔거나 무료 배포하는 것.
- 그래서 「개인만 무료」 항목도 **지금 0원으로 받아 두면** 나중에 매출이 커져도 계속 쓸 수 있다. 다만 받은 시점의 자격이 기준이므로, 법인을 세우거나 투자를 받은 뒤에 새로 받을 때는 다시 확인한다.
- **CC BY 4.0** 항목(Water Materials · Tileable Pine Forest Road · 자루 · 그릇 · 고딕 소품 · Old Furniture · Ancient Ruins · Telescope · GanzSe Camping · Medieval Room Kit · Medieval Log Cabins · Modular Fantasy Castle · Stylized Crystal Pack · Pirate chandelier · IronBound 상자)은 게임 크레딧에 작가 · 에셋 이름 · 라이선스를 적어야 한다. 받을 때 크레딧 목록(예: `docs/art/credits.md`)을 같이 만든다.
- **UE 전용**: 「Medieval Game Environment」처럼 설명에 「언리얼 엔진 제품에만」이라고 적힌 것과 옛 UE 마켓플레이스 라이선스 항목(예: Medieval Dungeon · Advanced Village Pack)은 언리얼 게임 안에서만 쓴다. 우리 게임은 언리얼이라 문제없지만, 블렌더로 꺼내 다른 엔진용으로 쓰지 않는다.
- **UEFN 참조 전용**은 원본을 주지 않는다 → UE5 프로젝트에 못 쓴다(Megascans 유료 항목의 0원 등급이 이것).
- **AI 생성 표시** 항목(Ancient Ruins · Medieval Campfire · Telescope · 손그림 돌벽 재질)은 라이선스상 문제는 없지만, 프로젝트가 AI 생성물을 어디까지 받을지 정하고 쓴다.
- Fab EULA 는 「NoAI」 표시 콘텐츠를 생성형 AI 의 데이터셋 · 개발 · 학습 입력에 쓰는 것을 금한다(6조). 받은 에셋을 Tripo · GPT 작업에 넣을 일이 생기면 그 에셋의 NoAI 표시부터 본다.
- UE 버전: Project Nature(최고 5.4) · Open World Demo(5.2) 등은 그 버전 프로젝트에 받아 5.8 프로젝트로 **Migrate** 해야 한다.

## Q 후보 (사용자가 고를 것)

- **사실적 에셋을 스타일라이즈드 셰이더로 맞출지**: 무료 좋은 품질은 대부분 사실적 스캔(Megascans · Megaplants · Project Nature)이다. 마스터 재질 하나(붓 터치 노멀 · 채도 · 그림자 색 이동)로 맞춰 쓸지, StyleHex 같은 스타일라이즈드 무료만 쓰고 나머지는 계획대로 만들지.
- **Megaplants(UE 5.7+, PVE 플러그인)를 기본 나무로 쓸지**: 형태 · 바람 · Nanite 는 가장 좋지만 사실적이다. 카드 방식 나무 계획(C07~C09 「카드」)과 바꿀지.
- **CC BY 에셋을 받을지**: 쓰면 크레딧 목록 관리가 생긴다.
- **AI 생성 표시 에셋을 받을지**.
- **「개인만 무료」를 지금 받아 둘지**: 지금은 자격이 되지만, 받은 시점 기준이라 미리 받아 두는 편이 유리하다.
