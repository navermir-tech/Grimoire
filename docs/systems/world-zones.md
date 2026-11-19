# 구역 이름 표시 · 크레딧 탭

> 상태: 구현(2026-11-18, 브랜치 `claude/zone-name-credits-c14a54`). 사용자 답 — [Q130](../spec/decisions#미결-질문) 「젤다랑 엘든링식으로」, [Q137](../spec/decisions#미결-질문) 「설정 탭 안에 크레딧 확인 탭」.
> 확인 항목은 [QA — 구역 이름 · 크레딧](../qa/world-zones).

## 구역 이름 표시 (Q130)

플레이어가 구역에 **처음** 들어서면 화면 위쪽 가운데에 구역 이름 + 가는 줄 + 한 줄 설명을 크게 띄웠다가 사라진다(젤다 · 엘든 링식). 다시 들어서면 큰 구역만 이름을 작게, 소구역은 띄우지 않는다. 본 구역은 플레이어 세이브에 남고, 멀티에서는 플레이어마다 따로다. 하늘 글씨(`AGrimDevLabel`, [지형 — 하늘 구역 이름](./terrain#하늘-구역-이름-레벨-디자인-2차))는 그대로 개발 표시다 — 같은 데이터를 쓴다.

| 처음 — 큰 구역 | 처음 — 소구역 |
|---|---|
| ![마탑 곶 배너](/qa/qa-zone-region-first.jpg) | ![마탑 앞마당 배너](/qa/qa-zone-sector-first.jpg) |

### 데이터 — 레이아웃이 정본

| 무엇 | 어디 |
|---|---|
| 정본(이름 · 설명 · 경계) | `Scripts/Terrain/openbeta_layout.json` 의 `Sections` — 하늘 글씨와 같은 20 구역 |
| 게임이 읽는 생성물 | `Content/Grimoire/Data/WorldZones.json` — `python Scripts/Terrain/export_zones.py` 가 쓴다(`--check` 는 같은지만 본다). `Content/Grimoire/Data` 는 출시본에 들어간다(`DirectoriesToAlwaysStageAsUFS`) |
| 어긋남 검사 | 자동화 테스트 `Grimoire.World.Zones.DataMatchesLayout` — 생성물과 레이아웃을 둘 다 읽어 구역마다 비교한다 |

`Sections` 한 줄의 칸:

| 칸 | 뜻 |
|---|---|
| `Name` | 세이브 · 복제 · 지도가 쓰는 이름(`Sec_TowerCape` · `Sub_Yard`). 바꾸면 이미 본 기록이 그 구역을 잃는다 |
| `Tier` | `Region`(지역 1 의 큰 구역 8) · `Sector`(마탑 구간 소구역 12) |
| `Label` | 화면 · 지도에 보이는 이름 |
| `Subtitle` | 게임에 보이는 한 줄(비면 `Note`). 2026-11-18 에 20 구역 모두 처음 문구를 썼다 — 고쳐도 된다 |
| `Note` | 개발 설명(하늘 글씨). 「S1 꺼진 화로 제단」처럼 POI 번호가 들어 있어 게임에는 `Subtitle` 을 쓴다 |
| `Center` | 가운데(지도 좌표 m) — 지도 이름표 자리, 소구역 나누기 |
| `Polygon` | **큰 구역의 경계**(지도 좌표 m). 서로 겹치지 않게 맞닿게 그린다(겹치면 레이아웃에서 앞의 것) |
| `Radius` | 소구역만, 선택 — 주면 `Center` 에서 그 거리(m) 안에서만 그 소구역 |
| `Height` | 하늘 글씨 높이(게임은 안 씀) |

**경계 규칙**

- 큰 구역: 점을 품은 첫 `Polygon`. 지역 1 땅(2032 × 4064 m)을 빈틈 · 겹침 없이 덮는다(테스트가 50 m 격자로 확인). 땅 밖(바다 남쪽 끝 너머)은 구역 없음.
- 소구역: `Center` 가 든 큰 구역 안을 **가장 가까운 `Center`** 로 나눈다(보로노이). 지금 소구역은 모두 `Sec_TowerCape`(마탑 곶 = 레이아웃 `TowerSector` 다각형 + 남서 해안) 안에 있다.
- 처음 그린 큰 구역 경계(2026-11-18)는 레이아웃 `Zones` 다각형 · `Valley` 선 · 큰 강을 따라 손으로 맞췄다 — 마탑 곶 = `TowerSector` + 그 서남쪽 바다 끝(북쪽 0.64 km 까지), 남부 목초지 = 마탑 곶 북쪽 ~ 1.3 km(서쪽 경계 x 780 ~ 820 m), 연못 저지대 = 그 서쪽 해안 0.64 km ~ 2.3 km, 중부 구릉 = 그 동쪽 ~ 계곡 남쪽 둑, 강변 = 큰 강 동쪽 · 1.5 km 위 강둑, 작은 계곡 = 계곡 선 양쪽 약 120 m 띠, 북부 고지 = 계곡 북쪽, 북쪽 고개 = 바위 문 둘레 400 m 폭. 레벨 디자인이 바뀌면 레이아웃에서 고치고 `export_zones.py` 를 돌린다.

### 들어섬 판단 (서버)

```
UGrimZoneSubsystem(월드, 서버) ──0.5 초마다──▶ UGrimZoneDiscoveryComponent::ServerPoll (PlayerState 마다)
    폰 위치 → 지도 좌표 → FGrimZoneMath::FindAt → FGrimZoneVisitTracker::Step
    └ 들어섬 → Client_ZoneEntered(구역, 처음?) ──▶ 소유 클라이언트: UGrimZoneBannerWidget::Enqueue
```

- `FGrimZoneVisitTracker`(순수 계산, `Grimoire.World.Zones.Visit`): 새 자리에 `StableSeconds`(1 초) 머물러야 들어선 것으로 친다 — 경계를 따라 걸을 때 깜빡이지 않게. 들어서면 **큰 구역 → 소구역** 순서로 알린다. 처음이면 본 구역에 더한다. 「다시」는 그 구역을 마지막으로 알린 지 `RevisitCooldownSeconds`(180 초)가 지나야 알린다. 구역 밖으로 나가는 것은 알리지 않는다.
- 세이브가 그 플레이어 몫을 읽기 전(`UGrimSaveSubsystem::IsPlayerLoaded`)에는 살피지 않는다 — 이미 본 구역을 「처음」으로 띄우지 않게.
- 컴포넌트는 퀘스트처럼 서브시스템이 런타임에 붙인다(이름 `GrimZones`, Core 를 고치지 않음). 모든 맵에서 붙지만 살피기는 구역 맵(설정 `Maps`, 지금 `L_OpenBeta`)에서만 — 레이아웃 좌표는 지역 1 의 것이라 마법탑 맵 · 샌드박스에서는 맞지 않는다.

### 배너 (클라이언트)

| 경우 | 모양(설정, `DefaultGame.ini`) |
|---|---|
| 큰 구역 처음 | 이름 46 pt(금빛 흰색) + 줄 + 한 줄 19 pt · 나타남 1.2 초 · 머묾 3.5 초 · 사라짐 1.8 초 |
| 큰 구역 다시 | 이름만 30 pt · 0.6 / 1.8 / 1.0 초 |
| 소구역 처음 | 이름 32 pt(하늘빛 흰색) + 줄 + 한 줄 16 pt · 0.8 / 2.5 / 1.2 초 |
| 소구역 다시 | 띄우지 않음(`SectorRevisit.bEnabled=False`) |

- 자리: 화면 높이의 12 %(`BannerTopFraction`)에서 가운데. 나타날 때 8 px 내려오며 밝아진다. 입력을 받지 않는다(HitTestInvisible), 뷰포트 Z 6(HUD · 퀘스트 추적 위, 메뉴 아래).
- 여러 개가 오면 차례로. 줄이 `MaxQueuedBanners`(3) 를 넘으면 오래된 「다시」부터 버린다. 「다시」가 떠 있을 때 「처음」이 오면 지금 것을 바로 사라지게 한다.
- 글씨는 Slate 기본 폰트(한글은 엔진 폴백 폰트) + 검은 테두리. `bShowBanners=False` 면 배너만 끄고 본 구역 기록은 쌓인다.

### 세이브 · 복제

| SaveId | 범위 | 구현 | v1 내용 |
|---|---|---|---|
| `World.Zones` | 플레이어 | `UGrimZoneDiscoveryComponent` | 본 구역 이름(`TArray<FString>`). 읽으면 지금 목록과 **합친다**(지우지 않음) |

- 본 구역 목록은 소유 클라이언트에게만 복제(`COND_OwnerOnly`, `OnDiscoveredChanged`) — 나중 지도 · 미니맵의 「가 본 곳」.
- 기후 지역 첫 방문 경험치(`UGrimLevelComponent`, `Progression.Level` 의 「발견한 지역」)와는 다른 것이다 — 기후 지역은 날씨 · 체온용 볼륨, 이것은 이름 붙은 장소. 구역 첫 진입에 경험치를 줄지는 미결(Q195).

### 미니맵 · 큰 지도가 쓸 조회 (`UGrimZoneSubsystem`, 블루프린트)

| 함수 | 돌려주는 것 |
|---|---|
| `FindZonesAt(월드 위치)` | 그 자리의 큰 구역 · 소구역 이름(`FGrimZoneHit`) |
| `GetZone(이름)` | 정의(`FGrimZoneDef` — 이름 · 한 줄 · 층 · 가운데 · 경계) |
| `GetZonesOfTier(층)` | 한 층의 구역들(레이아웃 순서) — 지도 이름표 · 범례 |
| `GetZoneWorldOutline(이름)` | 큰 구역 경계를 월드 좌표(cm)로 — 지도에 선 긋기 |
| `GetZoneWorldCenter(이름)` | 이름표 자리(월드 cm) |
| `WorldToMap` · `MapToWorld` | 월드(cm, X = 북 · Y = 동) ↔ 지도(m, U = 동 · V = 북, 원점 `WorldOrigin`) |
| `UGrimZoneDiscoveryComponent::FindForActor(…)->GetDiscovered()` | 그 플레이어가 본 구역(안개 걷기) |

서버 · 클라이언트 모두 같은 파일을 읽으니(`UGrimZoneSettings::GetDatabase`) 클라이언트 혼자 지도를 그릴 수 있다.

### 설정 `[/Script/Grimoire.GrimZoneSettings]`

`ZonesFile` · `Maps` · `PollIntervalSeconds`(0.5) · `StableSeconds`(1) · `RevisitCooldownSeconds`(180) · `bShowBanners` · `BannerTopFraction`(0.12) · `MaxQueuedBanners`(3) · `RegionFirst` · `RegionRevisit` · `SectorFirst` · `SectorRevisit`(각각 `bEnabled` · `FadeInSeconds` · `HoldSeconds` · `FadeOutSeconds` · `TitleSize` · `SubtitleSize` · `bShowSubtitle` · `bShowRule` · `TitleColor`).

### 콘솔 (개발 빌드)

| 명령 | 하는 일 |
|---|---|
| `Grim.Zones Where` | 로컬 폰 자리의 지도 좌표 · 큰 구역 · 소구역 |
| `Grim.Zones Dump` | (서버) 플레이어마다 지금 자리 · 본 구역 |
| `Grim.Zones Reset` | (서버) 본 구역 · 지금 자리를 잊는다 — 서 있는 곳의 「처음」 배너가 다시 뜬다 |
| `Grim.Zones Show <구역> [1/0]` | 로컬 화면에 배너만(처음 = 1) |
| `Grim.Zones Reload` | 구역 표를 다시 읽는다 |
| `Grim.Zones.At <초> <명령…>` | 몇 초 뒤 위 명령(`-ExecCmds` 용) |

### 코드

| 파일 | 내용 |
|---|---|
| `Source/Grimoire/World/GrimZoneTypes.h` | `FGrimZoneDef` · `FGrimZoneDatabase` · `FGrimZoneHit` · `FGrimZoneMath`(읽기 · 다각형 · 찾기 · 좌표) · `FGrimZoneVisitTracker` |
| `Source/Grimoire/World/GrimZoneSettings.h` | 설정 · 배너 모양 · 표 읽기 |
| `Source/Grimoire/World/GrimZoneSubsystem.h` | 컴포넌트 붙이기 · 살피기 타이머 · 조회 |
| `Source/Grimoire/World/GrimZoneDiscoveryComponent.h` | 본 구역 · 들어섬 · RPC · 세이브 |
| `Source/Grimoire/UI/GrimZoneBannerWidget.h` | 배너 위젯(C++ 위젯 트리) |
| `Source/Grimoire/World/GrimZoneDebug.cpp` | 콘솔 |
| `Source/Grimoire/World/Tests/GrimZoneTests.cpp` | `Grimoire.World.Zones.*` 6 |

## 크레딧 확인 탭 (Q137)

메인 메뉴 「설정」과 게임 안 Esc 메뉴 「설정」은 같은 설정 칸(`UGrimMenuSettingsPanel`)을 쓴다. 그 위에 탭 둘 — **「설정」 · 「크레딧 확인」**. 크레딧 확인은 CC BY 저작자 표시와 엔진 표기를 스크롤로 보여 준다. 설정을 열면 늘 「설정」 탭부터(저장 · 뒤로 버튼은 그대로 — 크레딧 탭에서 저장해도 설정 값이 저장될 뿐이다).

| Esc 메뉴 — 설정 탭 | Esc 메뉴 — 크레딧 확인 | 메인 메뉴 — 크레딧 확인 |
|---|---|---|
| ![설정 탭](/qa/qa-settings-tab.jpg) | ![크레딧 탭](/qa/qa-credits-tab.jpg) | ![메인 메뉴 크레딧 탭](/qa/qa-menu-credits-tab.jpg) |

### 하나의 정본에서 셋

| | 어디 | 만드는 법 |
|---|---|---|
| **정본** | `Content/Grimoire/Data/Credits.json` | 손으로 고친다([크레딧 — 한 줄 더하는 법](../art/credits#한-줄-더하는-법)) |
| 게임 안 탭 | 같은 파일을 런타임에 읽는다(`FGrimCreditsData`, 경로 `UGrimUISettings::CreditsFile`) | — |
| 문서 표 · 표기 문구 | `docs/art/credits.md` 의 `credits:begin` ~ `credits:end` | `node Scripts/Docs/credits.mjs --write` |
| 출시본 `CREDITS.txt` | 출시본 폴더(UTF-8 BOM · CRLF) | `node Scripts/Docs/credits.mjs --txt <폴더>/CREDITS.txt` |

- 검사: `npm run docs:check`(= `node Scripts/Docs/credits.mjs` 와 같은 검사 — 문서 표가 데이터와 다르면 오류), 자동화 테스트 `Grimoire.UI.Credits.Data`(필수 칸 · 문서에 같은 표기 한 줄이 있나) · `Grimoire.UI.Credits.Panel`(탭 · 줄 수).
- 표기 한 줄의 모양(`<에셋> by <저작자> (<링크>), licensed under <라이선스> (<링크>). <고친 점>`)은 `FGrimCreditsData::MakeAttributionLine` 과 `credits.mjs` 의 `attributionEn` 이 같아야 한다 — 테스트가 문서에서 그 줄을 찾는다.
- 탭 내용(위에서 아래로): 게임 이름 · 소개(한국어 · 영어) → 표기마다 「에셋 — 저작자」, 받은 곳 링크, 라이선스 · 링크, 고친 점(한국어), 영어 표기 한 줄 → 엔진 표기. 링크는 글자로만 보인다(누르면 열리는 것은 아직 없음).
- 엔진 표기: 언리얼 엔진 EULA 가 크레딧에 넣으라는 상표 · 저작권 줄 두 개를 `Notices` 에 넣었다. 출시 전에 그때 EULA 문구와 맞춰 본다(Q196).

### 코드

| 파일 | 내용 |
|---|---|
| `Source/Grimoire/UI/GrimCredits.h` | `FGrimCreditEntry` · `FGrimCredits` · `FGrimCreditsData`(읽기 · 표기 한 줄) |
| `Source/Grimoire/UI/GrimMenuWidgets.h` | `UGrimMenuSettingsPanel` 탭(`ShowTab` · `ETab`), 메인 메뉴 `OpenSettings` · Esc 메뉴 `ShowSettings` |
| `Source/Grimoire/Online/GrimMenuSubsystem.h` | `OpenPauseSettings`, 콘솔 `Grim.Online.Settings [0/1]`(메인 메뉴가 떠 있으면 그 설정, 아니면 Esc 메뉴의 설정) |
| `Scripts/Docs/credits.mjs` | 문서 표 · `CREDITS.txt` 생성 · 검사 |
| `Source/Grimoire/UI/Tests/GrimCreditsTests.cpp` | `Grimoire.UI.Credits.*` 2 |
