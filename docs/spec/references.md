# 레퍼런스 조사

미결 질문을 정하려고 다른 게임이 어떻게 했는지 모은 자료다(2026-11-05 조사). 수치 대부분은 공식 문서가 아니라 커뮤니티 위키 · 서버 호스팅 가이드에서 왔다. 「불확실」로 적은 것은 믿지 않는다.

## 하루 길이와 낮밤 비율 (Q6)

| 게임 | 하루(실시간 분) | 낮 | 밤 | 밤 비율 | 서버에서 바꾸나 | 메모 |
|---|---|---|---|---|---|---|
| 인슈라오디드 | 42 | 30 | 12 | 약 29% | ✅ 낮 · 밤 길이를 **따로**(각 2~60분) | 30 / 12 는 기본값 |
| 마인크래프트 (Java) | 20 | 약 10 | 약 7 | 약 35% | ✗ 고정(멈추기 · 시간 옮기기만) | 해 뜰 녘 · 해 질 녘 각 1.5분. 침대로 밤을 건너뜀 |
| 팰월드 | 약 32 | 약 27 | 약 5 | 약 16% | ✅ 낮 · 밤 속도를 따로 | 27 / 5 는 커뮤니티 측정값. 공식 수치 없음 — 불확실 |
| 발헤임 | 30 | 21 | 9 | 30% | ✗ 고정 | 잠자면 밤을 건너뜀 |
| 아크 | 불확실(맵마다 다름) | | | 약 33%? | ✅ 낮 · 밤 속도를 따로 | |
| 브이 라이징 | 18 | 약 6(햇빛) | 약 12 | 약 67% | ✅ | 뱀파이어라 **낮이 위험** — 위험한 쪽을 짧게 둔다는 같은 원리 |
| 돈스타브 투게더 | 8 | 계절마다 다름 | | 가을 약 13% | ✅ 프리셋 | 기억 기준 — 불확실 |

**읽을거리**

- 대부분 **밤이 하루의 25~35%** 이고, 잠으로 밤을 건너뛰는 장치가 흔하다. 밤은 긴장 구간이지만 짜증이 쌓이지 않게 짧게 둔다(개발사 발언은 못 찾음 — 추정).
- 인슈라오디드 · 팰월드 · 아크는 **낮과 밤 길이를 따로** 설정하게 한다. 우리 설정(`DayLengthMinutes` + `DayRatio`)도 같은 일을 할 수 있다.
- 지금 그리모리 임시값: 하루 48분 = 낮 30 · 밤 18(밤 37.5%). 레퍼런스보다 밤이 길다.

**결정할 후보** — ✅ B 로 확정(2026-11-05), 잠으로 밤 건너뛰기 추가

| 안 | 하루 | 낮 · 밤 | 성격 |
|---|---|---|---|
| A. 인슈라오디드형 | 42분 | 30 · 12 (밤 29%) | 레퍼런스 평균. 밤은 짧고 굵게 |
| B. 지금 값 | 48분 | 30 · 18 (밤 37.5%) | 밤 생물 · 빛 마법 · 체온 플레이를 길게 |
| C. 팰월드형 | 32분 | 27 · 5 (밤 16%) | 밤은 거의 지나가는 구간 |

어느 안이든 **서버 옵션으로 낮 · 밤을 따로** 바꾸게 한다. 그리모리는 밤에만 나오는 생물 · 어둠 속성 · 체온이 밤의 할 일이라, 밤이 너무 짧으면 그 콘텐츠를 볼 시간이 없다 → A 나 B 를 권한다.

## 캐릭터를 다른 서버로 가져가기 (Q12)

| 게임 | 캐릭터가 묶이는 곳 | 가져가는 것 | 악용 막기 |
|---|---|---|---|
| 팰월드 | **월드** | 없음 | 서버가 저장 |
| 마인크래프트 | **월드** | 없음 | 서버가 저장 |
| 브이 라이징 | **서버** | 없음(공식 미지원) | 서버가 저장 |
| 아크 | **서버** | 같은 클러스터 안에서 생존자 · 아이템 · 공룡을 올리고 내림 | 운영자가 이동을 막거나 클러스터 안으로만 연다 |
| 코난 엑자일 | **서버** | 공식 서버끼리만 레벨 · 진행도 · 가방 | 방향 제한(PvP → PvE 만), 추종자 · 건물은 못 가져감 |
| 인슈라오디드 | **내 PC(로컬)** — 월드와 분리 | 레벨 · 스킬 · 장비 · 가방. 기지 · 퀘스트는 월드에 남음 | 확인 못 함 |
| 발헤임 | **내 PC(로컬)** | 스킬 · 가방 · 장비 전부 | 거의 없음 — 싱글에서 치트로 얻은 물건을 들고 들어온다 |
| 테라리아 | **내 PC(로컬)** | 가방 전체 | 서버 플러그인(서버 측 캐릭터)으로만 막음 |

**두 갈래**

- **로컬 캐릭터형**(인슈라오디드 · 발헤임 · 테라리아): 친구 월드를 자유롭게 오가고 성장이 남는다. 대신 파일 편집 · 싱글 치트로 얻은 것을 그대로 들고 들어오는 것을 막지 못한다.
- **서버 귀속형**(팰월드 · 아크 · 브이 라이징 · 마인크래프트): 서버가 진실이라 공정하다. 대신 서버를 옮기면 처음부터 다시 한다.
- 중간: 같은 운영자의 서버끼리만 옮기기(아크 클러스터), 옮길 수 있는 것 제한(코난 — 생물 · 건물 제외), 서버 옵션으로 고르기(테라리아 플러그인).

**그리모리에 맞춰 보면** — ✅ 권하는 안으로 확정(2026-11-05)

- 그리모리는 **서버 권위**가 설계 원칙이고(시스템 문서 「설계 원칙」), 길들인 생물 · 돌연변이 · 거점이 핵심 자산이다. 생물을 들고 다니면 번식 · 돌연변이 경제가 서버마다 무너진다.
- 권하는 안: **기본은 서버 귀속**, 서버 옵션으로 「캐릭터 내보내기 · 들여오기 허용」을 켤 수 있게 한다. 옮겨지는 것은 **서클 · 배운 마법 · 그리모어 기록**만(가방 · 생물 · 거점은 제외). 서클과 마법은 "마법사의 실력"이라 옮겨도 서버 경제를 덜 흔든다.

## 호그와트 레거시 — 락온 (Q17)

여러 공략 사이트가 일치한 내용이다. 공식 조작 문서는 찾지 못했다.

| 항목 | 호그와트 레거시 |
|---|---|
| 자동 대상 | 기본으로 켜짐. 설정 「Camera Relative Targeting」 켬 = 카메라 방향 기준, 끔 = 스틱 방향 기준 |
| 고정 락온 | **토글**. 패드 **R3**, PC 기본 **Caps Lock**. 다시 누르면 풂 |
| 고정 중 대상 바꾸기 | 오른쪽 스틱을 다음 대상 쪽으로 튕김(PC 는 마우스 — 불확실) |
| 사물 | 사물도 대상이 된다(기본 시전은 적과 사물에 피해, 아씨오는 사물에 흰 외곽선) |
| 조준 옵션 | 조준 누르기 / 토글 선택, 조준 보조 기본 켬 |
| 불확실 | 고정 중 카메라가 얼마나 따라가는지, 투사체 유도 세기(체감상 거의 필중이라는 말뿐), 자동 대상을 완전히 끄는 옵션 |

**조준 모드 (R24 조사, 2026-11-05)** — 공략 사이트 기준, 버튼 목록이 사이트마다 조금 다르다.

| 항목 | 호그와트 레거시 |
|---|---|
| 조준(줌인) | **누르는 동안** PC 오른쪽 마우스 · 패드 L2/LT. 화면에 원 조준점이 뜨고 아무 곳이나 겨눈다 |
| 옵션 | 접근성의 「조준 모드 토글」(누르기 / 토글), 「조준 가속」(조준 감도) |
| 대상 표시 | 아씨오 같은 마법은 겨눈 대상이 하얀 외곽선으로 빛날 때 쏜다 |
| 탈것 · 빗자루 | 타고 있어도 시전할 수 있다는 글이 있다(그래폰 위 전투 등) — 근거가 적어 불확실 |

**기본 시전 (R26 조사, 2026-11-05)** — 공략 사이트 기준.

| 항목 | 호그와트 레거시 |
|---|---|
| 성격 | 피해가 작은 원거리 공격, **쿨타임 없음** |
| 연타 | R2 를 연달아 누르면 볼트가 나가고, **네 번째**가 훨씬 세며 적을 밀어낸다 |
| 쓰임 | 공중에 띄운 적에게 기본 시전을 이어 넣는 콤보, 큰 마법 사이를 메움. 특성으로 기본 시전이 마법 쿨타임을 조금 줄이기도 한다 |
| 버튼 | 패드 R2(PlayStation) · RT(Xbox) · PC 좌클릭. 슬롯 마법은 R2 를 누른 채 얼굴 버튼 — 아래 「조작」 |

→ 그리모리 적용: 고정 락온 = **R3 · Caps Lock 토글**, 고정 중 오른쪽 스틱 튕김 · 마우스로 전환, 카메라는 대상을 부드럽게 따라감(이 부분은 레퍼런스 자료가 없어 우리 안). 「카메라 기준 / 스틱 기준」 옵션도 따른다.

## 호그와트 레거시 — 조작 (Q29)

공식 조작 문서는 찾지 못했다. 공략 사이트 여러 곳(아래 출처)이 일치한 기본 배치다. 패드 표는 PlayStation 5 / Xbox Series 를 함께 적은 공략 기준이다. 사이트마다 틀린 줄이 조금씩 있어(같은 버튼이 두 동작에 적힌 표 등) 두 곳 이상이 맞는 것만 확정으로 둔다.

| 동작 | PC (키보드 · 마우스) | PlayStation | Xbox | 그리모리 (M2 갈래 K) |
|---|---|---|---|---|
| 기본 시전 | 좌클릭 | R2 짧게 | RT 짧게 | **기본 공격** — `IA_BasicAttack`(좌클릭) · `IA_BasicAttackPad`(R2/RT, Tap 0.2초) |
| 슬롯 마법 시전 | 숫자 1 ~ 4 | **R2 누른 채** △ · ○ · ✕ · □ | **RT 누른 채** Y · B · A · X | 같음 — `IA_SpellSlot1~4` · `IA_SpellSlotPad1~4`(R2/RT 를 Chord 로) |
| 슬롯 세트 고르기 | F1 ~ F4 (5 ~ 8 도) | R2 누른 채 십자키 | RT 누른 채 십자키 | F1 ~ F4 = UI 갈래가 이미 같게 묶음 · 패드 `IA_SpellSetPad1~4` |
| 다음 · 이전 세트 | 마우스 휠 | — | — | `IA_SpellSetNext/Prev`(보조, 끌 수 있음) |
| 마법 배치 메뉴 | T | 십자키 → | 십자키 → | 그리모어 화면 = T · 십자키 → (같음 — 2026-11-05 사용자 결정으로 맞춤) |
| Protego(누름 = 막기 · 길게 = Stupefy) | Q | △ | Y | `IA_Protego` — 입력 자리만(`Input.Action.Protego`) |
| 회피 구르기 | Ctrl | ○ | B | `IA_Dodge` — 입력 자리만(`Input.Action.Dodge`) |
| 상호작용 | F | □ | X | F(침대 — 생존 갈래). 패드 □/X 는 아직 없음 |
| 점프 | Space | ✕ | A | 템플릿 `IA_Jump` 그대로(같음) |
| 달리기 | 왼쪽 Shift | L3 | LS 누르기 | 없음(후속) |
| 조준(줌인) | 우클릭 누름 | L2 | LT | `IA_AimZoom`(R24 — 이미) |
| 고정 락온 | Caps Lock | R3 | RS 누르기 | `IA_LockOn`(Q17 — 이미) |
| Revelio · 회복 · 나침반 | R · G · — | 십자키 ← · ↓ · ↑ | 십자키 ← · ↓ · ↑ | 없음 |
| 고대 마법 · 던지기 | X · Z | L1 + R1 · R1 | LB + RB · RB | 없음 |
| 도구(휠) | Tab | L1 | LB | 없음 |
| 지도 · 필드 가이드 | M · Esc | 터치패드 · Options | View · Menu | — |

**불확실** (공략에서 확인하지 못함 — 데이터만 바꾸면 된다, `Scripts/Character/make_input.py` 의 `PAD_SLOT_KEYS` · `PAD_SET_KEYS`):

- 패드 **칸 번호 ↔ 얼굴 버튼** 짝. 공략은 「칸마다 얼굴 버튼 하나」까지만 적는다. 지금은 위에서 시계 방향 — 칸 1 = Y/△, 2 = B/○, 3 = A/✕, 4 = X/□.
- 패드 **세트 ↔ 십자키 방향** 짝(세트 1 = ↑, 2 = →, 3 = ↓, 4 = ← 로 둠). 세트는 특성으로 늘어난다는 글만 있다.
- R2 를 **눌렀다가 길게 쥐면** 기본 시전이 나가는지(우리는 0.2초 안에 떼야 기본 공격 — 세트 활성과 안 겹치게). 기본 시전을 눌러 바로 쏘는지 뗄 때 쏘는지 공략에 없다.
- 마우스 휠 = 다음 · 이전 세트는 공략 한 곳만 적었다.
- 한 공략이 R1(RB) 을 세트 활성 버튼으로 적었으나 나머지가 모두 R2(RT) — R2 로 본다.

**호그와트와 맞춘 곳**: 그리모어 화면(마법 목록 · 슬롯 배치)을 처음엔 Tab · 패드 View 로 했다가, 호그와트의 마법 배치 메뉴 자리인 **T · 십자키 →** 로 옮겼다(사용자 결정). 패드 십자키 → 는 R2 를 누른 채면 세트 고르기로 쓰고, 그때는 그리모어를 열지 않는다.

## 마법 이펙트 에셋 (Q20)

**직접 만들 수 있나**

- Niagara 시스템을 코드(Python · C++)로 처음부터 짜는 공식 방법은 사실상 없다. 에디터에서 손으로 만드는 도구다.
- 현실적인 방법: **기존 시스템을 복제해 색 · 크기 · 개수 같은 파라미터를 데이터로 바꾸는 것**. 이건 코드로 할 수 있다 — 불꽃 하나를 받아 10속성 색으로 변형하는 식.
- 그림(파티클 텍스처 · 플립북)은 GPT 로 만들 수 있다.

**무료 출처** (상업 게임 사용 가능 — 각 리스팅의 라이선스는 받을 때 확인)

| 이름 | 내용 | 메모 |
|---|---|---|
| **Niagara Examples Pack** (Epic, 2026-01, Fab 무료) | Niagara 시스템 50개 이상 — 폭발 · 피격 · 불꽃 · 불 | UE 5.7 기준. 가장 먼저 쓸 후보. 물 · 얼음 · 신성 · 어둠은 없음 |
| **Paragon 에셋** (Epic, 무료) | 영웅별 스킬 이펙트(마법 계열 많음) | UE4 · 구형 파티클(Cascade)이라 변환 필요할 수 있음 |
| **Infinity Blade: Effects** (Epic, 무료) | 불 · 연기 · 번개 · 마법 | 구형 · Fab 에서 찾기 어려움 |
| Content Examples · Lyra (Epic 샘플) | Niagara 학습 맵, 피격 이펙트 | 마법 계열은 적음 |
| **Fab 기간 한정 무료**(2주마다) | 매번 다름 | 기간 안에 받아 두면 영구 소유 |

10속성을 고루 갖춘 무료 Niagara 마법 팩은 찾지 못했다.

**권하는 안** — ✅ 확정(2026-11-05): M1.5 는 **Niagara Examples Pack + 파라미터 변형 + GPT 텍스처**로 10속성 임시 이펙트를 만든다. 부족한 물 · 얼음 · 신성 · 흑마법은 Fab 무료 배포를 지켜보다가, M7(수직 슬라이스) 전에 유료 팩 구매를 판단한다. Fab 에서 받는 것은 사용자 계정으로 해야 한다(라이브러리에 추가 → 프로젝트에 넣기).

## 장비 · 건축 내구도 (R18)

| 게임 | 장비 내구도 | 장비 수리 | 건축 내구도 |
|---|---|---|---|
| 마인크래프트 | 있음. 0 이면 **사라짐** | 모루에 재료 · 같은 장비를 합침. 수리할수록 비용 ↑(「너무 비쌈」). 인챈트 「수선」은 경험치로 자동 수리 | 없음(블록은 폭발 · 채굴로만 부서짐) |
| 인슈라오디드 | 있음. 0 이면 **쓸 수 없게 될 뿐 사라지지 않는다**는 공략이 많다(사라진다는 글도 있어 불확실) | 작업대에서 장착 장비 전부 한 번에. 비용은 공략마다 달라 불확실 — 무료라는 글이 많다 | 확인 안 함 |
| 팰월드 | 있음 | 수리대(나무 10 · 돌 10)에서 재료를 내고 **완전히** 회복. 남은 양과 상관없이 같은 비용이라 바닥날 때 고치는 게 이득 | **있음.** 습격 피해 · 거점 범위 밖 노후. 「수리 키트」(섬유 · 돌)로 고침, 방어벽은 수리 안 돼 다시 지음. 서버 설정에 피해 배율 · 노후 배율 · 최대 HP 배율 |

→ 팰월드는 건축 내구도가 **있다**. 그리모리의 노후 · 서버 옵션은 팰월드를 따랐고, "수리할수록 비싸진다"는 마인크래프트 쪽을 "최대 내구도가 준다"로 바꿔 꾸준한 교체(인슈라오디드 느낌)를 만든다.

## 대규모 서버 (R32)

| 항목 | 내용 |
|---|---|
| 팰월드 전용 서버 인원 | 공식 상한 32(`ServerPlayerMaxNum` 1~32). 호스팅 업체들도 모드 없이는 32 를 넘길 수 없다고 안내 |
| 「100」의 출처 | 길드 인원 상한 설정(1~100, 기본 20)으로 보인다 — 서버 인원이 아님 |
| 1.0 서버 클러스터 | 서버 여러 개를 묶어 더 큰 인원을 받는다는 업체 블로그 한 곳 — 공식 확인 못 함(불확실) |

## 출처

- 인슈라오디드: [Nodecraft](https://nodecraft.com/support/games/enshrouded/setup/how-to-change-the-day-length-on-an-enshrouded-server) · [WabbaNode](https://wabbanode.com/help/enshrouded/how-to-configure-day-and-night-duration-on-your-enshrouded-server) · [멀티 진행](https://www.itechguides.com/how-progression-works-in-enshrouded-multiplayer/)
- 마인크래프트: [Minecraft Wiki](https://minecraft.wiki/w/Daylight_cycle)
- 팰월드: [TimeRate](https://wiki.indifferentbroccoli.com/Palworld/TimeRate) · [캐릭터 세이브](https://xgamingserver.com/docs/palworld/character-save-fix)
- 발헤임: [하루 길이](https://mein-mmo.de/en/how-long-does-a-day-actually-last-in-valheim,662039)
- 아크: [시간 배율](https://ark.wiki.gg/wiki/ARK_Wiki:Server_time_speed_scale_calculation) · [서버 간 이동](https://ark.wiki.gg/wiki/CrossARK_Transfers)
- 브이 라이징: [서버 설정](https://low.ms/knowledgebase/v-rising-server-configuration) · [공식 FAQ](https://guides.playvrising.com/hc/en-us/articles/28588823248029-Can-You-Transfer-Characters-or-Change-Servers-in-V-Rising)
- 코난 엑자일: [공지](https://www.conanexiles.com/?p=21107)
- 돈스타브 · 테라리아: [DST Wiki](https://dontstarve.wiki.gg/wiki/Day-Night_Cycle) · [Terraria Wiki](https://terraria.wiki.gg/wiki/Server)
- 호그와트 레거시 기본 시전: [MP1st 전투](https://mp1st.com/guides/hogwarts-legacy-advanced-combat-guide-optimizing-your-spell-usage) · [Fextralife Basic Cast](https://hogwartslegacy.wiki.fextralife.com/Basic+Cast)
- 호그와트 레거시 조준: [GameSkinny 조작 목록](https://www.gameskinny.com/b7804/all-hogwarts-legacy-controls-list-for-pc-playstation-xbox) · [CyberPost 탈것 전투](https://cyberpost.co/?p=1207300)
- 호그와트 레거시: [Fextralife 조작](https://hogwartslegacy.wiki.fextralife.com/Controls) · [Twinfinite](https://twinfinite.net/guides/how-to-lock-on-in-hogwarts-legacy/) · [GameSkinny 설정](https://www.gameskinny.com/tips/hogwarts-legacy-how-to-change-controller-settings-guide/) · [WB 접근성](https://portkeygamessupport.wbgames.com/hc/en-us/articles/11893402078355)
- 호그와트 레거시 조작 배치(Q29, 2026-11-05): [Samurai Gamers PS5 · Xbox](https://samurai-gamers.com/hogwarts-legacy/game-controls-52/) · [Magic Game World PC](https://www.magicgameworld.com/hogwarts-legacy-pc-controls-keyboard-gamepad/) · [Shortcut Posters PC · 패드](https://shortcutposters.com/games/hogwarts-legacy/) · [Gamepressure](https://www.gamepressure.com/hogwarts-legacy/controls/z31085e) · [SegmentNext 세트 바꾸기](https://segmentnext.com/hogwarts-legacy-change-spells/) · [DiamondLobby 슬롯](https://diamondlobby.com/hogwarts-legacy/how-to-unlock-spell-slots-hl/)
- 내구도: [팰월드 건축 피해 설정](https://xgamingserver.com/docs/palworld/structure-damage) · [팰월드 수리](https://vandal.elespanol.com/en/palworld/palworld-how-to-repair-weapons-armor-tools-and-base-structures.html) · [팰월드 수리대](https://gamewith.net/palworld/44330) · [인슈라오디드 수리](https://primagames.com/gaming/how-to-repair-weapons-and-tools-in-enshrouded) · [Destructoid](https://destructoid.com/?p=454095)
- 대규모 서버: [XGamingServer 최대 인원](https://xgamingserver.com/docs/palworld/max-players) · [Nodecraft](https://nodecraft.com/support/games/palworld/setup/how-to-change-the-max-players-on-a-palworld-server) · [FREAKHOSTING 길드 상한](https://help.freakhosting.com/games/palworld/increasing-the-32-player-limit-guild-limit) · [GeekChamp](https://geekchamp.com/how-to-play-palworld-with-more-than-32-players-on-a-server/)
- 이펙트: [Niagara Examples Pack 발표](https://www.unrealengine.com/en-US/news/discover-over-50-free-niagara-systems-ready-to-use-in-unreal-engine-5-7) · [Fab 라이선스](https://dev.epicgames.com/documentation/en-us/fab/licenses-and-pricing-in-fab) · [Paragon](https://www.pcgamer.com/epic-has-released-dollar12-million-worth-of-paragon-assets-free-of-charge) · [Fab 격주 무료](https://www.cgchannel.com/2024/11/epic-games-to-share-new-free-assets-on-fab-every-two-weeks)
