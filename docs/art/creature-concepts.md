---
aside: false
---

# 마법 생물 콘셉트 아트 — 오픈 베타 후보

> 상태: **검토용 v2** · 2026-11-19 · GPT 이미지(Codex CLI). v1(2026-11-18)이 「실제 동물에 뿔 · 이끼를 얹은」 느낌이라 「킥」 규칙으로 다시 그렸다 — 종마다 **있을 수 없는 특징 하나**(몸 일부가 속성 재질 · 떠 있는 부위 · 몸에서 나는 빛)와 **자기 마법을 쓰는 순간**. 몸은 메시 · 재질로 만들 수 있는 또렷한 형태이고, 흩어지는 것(안개 · 물방울 · 불티 · 음파)은 카드의 **FX** 줄에 적은 대로 나이아가라 효과로 낸다. 옛 판은 카드 아래 「옛 판 보기」에 접어 두었다.
>
> 아트 방향은 [Painterly High-Fidelity Stylized Realism](../spec/art-direction), 생물 톤 규칙은 [지역별 생물 명단 — 톤 · 겉모습 규칙](../spec/creature-roster#톤-·-겉모습-규칙).
>
> **쓰임**: 오픈 베타 30종(Q50)을 그림을 보고 고른다. 1번 지역 10종은 다 들어가고, 2 · 3번 지역 16종에서 골라 채운다 — **셋을 다 넣어도 26종**이라 30종에는 4종이 모자란다(4번 지역부터 당겨 오거나 종 수를 줄이는 것은 정할 일).
>
> **전설(☆)도 잡고 교배할 수 있다**(Q48 답 — [지역별 생물 명단 「전설」](../spec/creature-roster#전설-—-잡고-·-자랑하고-·-교배한다-q48)). 전설 카드에는 도감 그림과 「길들인 뒤 동행 장면」 시안 B 를 함께 둔다.

그림 읽는 법: 3/4 앞 시점 전신, 옆의 회색 실루엣은 키 1.8 m 사람(축척), 아래 띠는 색 변이 셋(기본 · 암수 · 돌연변이). 그림을 누르면 크게 열린다. 이름 · 설명의 정본은 [지역별 생물 명단](../spec/creature-roster)(`art/creatures/make_prompts.py`), 프롬프트는 `art/creatures/prompts/`, 이 페이지는 `art/creatures/publish.py` 가 만든다.

<script setup>
import { withBase } from 'vitepress'  // 그림 링크(href)는 base(/Grimoire/)가 저절로 붙지 않는다 — img src 는 붙는다
</script>

<style>
.creature-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 16px; margin: 16px 0 28px; }
.creature-card { border: 1px solid var(--vp-c-divider); border-radius: 8px; overflow: hidden; background: var(--vp-c-bg-soft); }
.creature-card a.shot { display: block; }
.creature-card img { display: block; width: 100%; height: auto; }
.creature-card .none { padding: 48px 12px; text-align: center; color: var(--vp-c-text-3); }
.creature-card .body { padding: 10px 12px 12px; font-size: 14px; line-height: 1.55; }
.creature-card .name { font-size: 16px; font-weight: 600; }
.creature-card .tag { display: inline-block; margin-left: 4px; padding: 0 6px; border-radius: 4px; font-size: 12px; background: var(--vp-c-default-soft); }
.creature-card .tag.legend { background: var(--vp-c-yellow-soft); }
.creature-card .tag.boss { background: var(--vp-c-red-soft); }
.creature-card .meta { margin-top: 6px; font-size: 12.5px; color: var(--vp-c-text-2); }
.creature-card .alt { padding: 4px 12px; font-size: 12px; color: var(--vp-c-text-2); border-top: 1px solid var(--vp-c-divider); }
.creature-card details.old { border-top: 1px solid var(--vp-c-divider); font-size: 12px; color: var(--vp-c-text-2); }
.creature-card details.old summary { padding: 4px 12px; cursor: pointer; }
.creature-card .fx { margin-top: 6px; font-size: 12.5px; }
</style>

## 한눈에

| 지역 | 종 | 전설 | 필드 보스 | 오픈 베타 |
|---|---|---|---|---|
| [1. 마법탑 · 온난 지대](#region-1) | 10 | — | 구릉 멧돼지 | 전부 |
| [2. 깊은 숲](#region-2) | 8 | 오초 | 아팡크 | 고른다 |
| [3. 폭풍 해안](#region-3) | 8 | 천둥새 | 카르키노스 게 | 고른다 |
| [새 종 후보 2차 — 처음 보는 보석 · 유리 생물](#new-species) | 카벙클 시안 3 + 후보 7 | — | — | 고른다(Q220 · Q221) |

## 1. 마법탑 · 온난 지대 — 10종 {#region-1}

서클 1~3 · 주 속성 무속성 · 바람 · 풀 · 물. 오픈 베타의 땅 — **10종 모두 들어간다**(고르는 대상이 아니다). 재칼로프 · 켈피 · 검은 개는 이미 게임 데이터가 있다.

| 이름 | 한 줄 | 크기 | 원전 | 기능 | 희귀도 |
|---|---|---|---|---|---|
| **재칼로프** `jackalope` | M3 기존 종. 뿔은 가지가 적은 짧은 사슴뿔(바람 결 재질 — 색 변이마다 뿔 색이 다름, 재질 파라미터), 몸은 산토끼 비율 | 소형(몸길이 0.5 m) | 재칼로프(북미 민담) | 없음(동행 · 전투) | 흔함 |
| **켈피** `kelpie` | M3 기존 종. 사람으로 변하는 원전 설정은 뺌 — 갈기 · 꼬리가 젖은 물풀인 말 | 중형(어깨 1.6 m) | 켈피(스코틀랜드 민담) | 탈것(물 위 달리기) · 속성 작업(물 나르기 · 급수) | 보통 |
| **검은 개** `black-dog` | M3 기존 종. 무섭지 않게 — 보통 늑대보다 큰 검은 늑대, 불씨처럼 빛나는 호박색 눈. 그림자 안개는 FX | 중형(어깨 0.9 m) | 블랙 독(영국 민담) | 방범(밤 경비) | 보통 |
| **볼퍼팅어** `wolpertinger` | 여러 동물을 이어 붙인 원전을 날다람쥐 · 슈가글라이더 체형 + 작은 뿔 + 빛나는 혈맥 비막으로 정리(2026-11-19 사용자) | 소형(몸길이 0.4 m) | 볼퍼팅어(독일 바이에른 민담) | 없음(동행 · 전투) | 흔함 |
| **흙닭 에르트헨** `erdhenne` | 집을 지키는 검은 닭 정령 → 땅에 둥지를 파는 큰 들닭. 위험이 오기 전에 우는 습성은 남김 | 소형(키 0.6 m) | 에르트헨(독일 알프 지역 정령) | 목축(알) | 흔함 |
| **바로메츠 양** `barometz` | 줄기에 열리는 양 → 등에 이끼 · 고사리 털이 자라는 양. 밤에는 발굽 둘레 뿌리털을 땅에 내리고 쉰다 | 중형(어깨 0.9 m) | 바로메츠(타타르의 식물 양 전설) | 목축(털 · 젖) | 흔함 |
| **위스프 나방** `wisp-moth` | 길을 잃게 하는 불빛 → 빛나는 나방 떼. 따라가면 숨은 상자 · 길이 나온다 | 소형(날개 폭 0.15 m) | 윌 오 더 위스프(영국 민담 도깨비불) | 없음(동행 · 전투) | 드묾(조건부) |
| **맨드레이크** `mandrake` | 사람 모양 뿌리 그대로 — 두 다리로 걷는 인삼 뿌리 사람(2026-11-19 사용자). 뽑히면 지르는 비명은 귀가 먹먹해지는 울음(잠깐 기절) | 소형(키 0.4 m) | 맨드레이크(유럽 전승) | 손재주(농사 — 밭갈이) | 보통(비 온 뒤 많이) |
| **불씨여우** `foxfire-fox` | 여우가 켜는 도깨비불 → 꼬리 끝에 숯불 같은 불씨를 단 들여우 | 소형(몸길이 0.6 m) | 여우불(일본 키츠네비) | 속성 작업(불 지피기) | 보통 |
| **구릉 멧돼지** `hill-boar` | 전설의 한 마리 대신 그 혈통의 큰 멧돼지 종. 우두머리(거대 개체)가 1번 지역 필드 보스 | 대형(어깨 1.5 m) | 창작 — 에리만토스의 멧돼지(그리스 신화 ☆)의 먼 혈통 | 탈것(돌진) | 드묾(보스) |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/jackalope.jpg')" target="_blank"><img src="/art/creatures/jackalope.jpg" alt="재칼로프" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/jackalope-v1.jpg')" target="_blank"><img src="/art/creatures/jackalope-v1.jpg" alt="재칼로프 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">재칼로프</span> <code>jackalope</code><br>
M3 기존 종. 뿔은 가지가 적은 짧은 사슴뿔(바람 결 재질 — 색 변이마다 뿔 색이 다름, 재질 파라미터), 몸은 산토끼 비율
<div class="meta">크기 소형(몸길이 0.5 m) · 속성 바람 · 풀 · 흔함<br>원전 재칼로프(북미 민담)<br>기능 없음(동행 · 전투)<br>1서클 벤투스 · 고기 · 털 · 뿔</div>
<div class="fx"><b>FX:</b> 뒷발 아래 바람 고리 · 몸을 따라 흐르는 바람 줄기 · 맴도는 씨앗</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/kelpie.jpg')" target="_blank"><img src="/art/creatures/kelpie.jpg" alt="켈피" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/kelpie-v1.jpg')" target="_blank"><img src="/art/creatures/kelpie-v1.jpg" alt="켈피 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">켈피</span> <code>kelpie</code><br>
M3 기존 종. 사람으로 변하는 원전 설정은 뺌 — 갈기 · 꼬리가 젖은 물풀인 말
<div class="meta">크기 중형(어깨 1.6 m) · 속성 물 · 바람 · 보통<br>원전 켈피(스코틀랜드 민담)<br>기능 탈것(물 위 달리기) · 속성 작업(물 나르기 · 급수)<br>2서클 아쿠아 · 탑승(물 위 달리기) · 물 나르기</div>
<div class="fx"><b>FX:</b> 발굽 아래 물 원판 · 뒤로 남는 물보라 자취 · 갈기 끝 물방울</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/black-dog.jpg')" target="_blank"><img src="/art/creatures/black-dog.jpg" alt="검은 개" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/black-dog-v1.jpg')" target="_blank"><img src="/art/creatures/black-dog-v1.jpg" alt="검은 개 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">검은 개</span> <code>black-dog</code><br>
M3 기존 종. 무섭지 않게 — 보통 늑대보다 큰 검은 늑대, 불씨처럼 빛나는 호박색 눈. 그림자 안개는 FX
<div class="meta">크기 중형(어깨 0.9 m) · 속성 그림자술 · 불 · 보통<br>원전 블랙 독(영국 민담)<br>기능 방범(밤 경비)<br>3서클 움브라 · 밤 경비</div>
<div class="fx"><b>FX:</b> 다리 · 등에서 피어오르는 그림자 안개 · 발밑 그림자 웅덩이(데칼) · 발자국의 불씨</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/wolpertinger.jpg')" target="_blank"><img src="/art/creatures/wolpertinger.jpg" alt="볼퍼팅어" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2</summary>
<a class="shot" :href="withBase('/art/creatures/wolpertinger-v2.jpg')" target="_blank"><img src="/art/creatures/wolpertinger-v2.jpg" alt="볼퍼팅어 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/wolpertinger-v1.jpg')" target="_blank"><img src="/art/creatures/wolpertinger-v1.jpg" alt="볼퍼팅어 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">볼퍼팅어</span> <code>wolpertinger</code><br>
여러 동물을 이어 붙인 원전을 날다람쥐 · 슈가글라이더 체형 + 작은 뿔 + 빛나는 혈맥 비막으로 정리(2026-11-19 사용자)
<div class="meta">크기 소형(몸길이 0.4 m) · 속성 바람 · 땅 · 흔함<br>원전 볼퍼팅어(독일 바이에른 민담)<br>기능 없음(동행 · 전투)<br>1서클 벤투스 · 야생 도토리 저장고(굴을 파면 씨앗 · 견과)</div>
<div class="fx"><b>FX:</b> 뒤로 남는 바람 자취 · 맴도는 나뭇잎</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/erdhenne.jpg')" target="_blank"><img src="/art/creatures/erdhenne.jpg" alt="흙닭 에르트헨" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2</summary>
<a class="shot" :href="withBase('/art/creatures/erdhenne-v2.jpg')" target="_blank"><img src="/art/creatures/erdhenne-v2.jpg" alt="흙닭 에르트헨 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/erdhenne-v1.jpg')" target="_blank"><img src="/art/creatures/erdhenne-v1.jpg" alt="흙닭 에르트헨 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">흙닭 에르트헨</span> <code>erdhenne</code><br>
집을 지키는 검은 닭 정령 → 땅에 둥지를 파는 큰 들닭. 위험이 오기 전에 우는 습성은 남김
<div class="meta">크기 소형(키 0.6 m) · 속성 땅 · 풀 · 흔함<br>원전 에르트헨(독일 알프 지역 정령)<br>기능 목축(알)<br>1서클 테라 · 거점 가축(알) · 밤 습격 전에 운다</div>
<div class="fx"><b>FX:</b> 발밑 금빛 동심원 진동 무늬 · 튀어 오르는 흙 알갱이</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/barometz.jpg')" target="_blank"><img src="/art/creatures/barometz.jpg" alt="바로메츠 양" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/barometz-v1.jpg')" target="_blank"><img src="/art/creatures/barometz-v1.jpg" alt="바로메츠 양 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">바로메츠 양</span> <code>barometz</code><br>
줄기에 열리는 양 → 등에 이끼 · 고사리 털이 자라는 양. 밤에는 발굽 둘레 뿌리털을 땅에 내리고 쉰다
<div class="meta">크기 중형(어깨 0.9 m) · 속성 풀 · 땅 · 흔함<br>원전 바로메츠(타타르의 식물 양 전설)<br>기능 목축(털 · 젖)<br>1서클 파크툼 · 거점 가축(털 · 젖) · 풀밭을 고르게 깎음</div>
<div class="fx"><b>FX:</b> 땅 위로 퍼지는 빛나는 맥(데칼) · 떠오르는 포자</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/wisp-moth.jpg')" target="_blank"><img src="/art/creatures/wisp-moth.jpg" alt="위스프 나방" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/wisp-moth-v1.jpg')" target="_blank"><img src="/art/creatures/wisp-moth-v1.jpg" alt="위스프 나방 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">위스프 나방</span> <code>wisp-moth</code><br>
길을 잃게 하는 불빛 → 빛나는 나방 떼. 따라가면 숨은 상자 · 길이 나온다
<div class="meta">크기 소형(날개 폭 0.15 m) · 속성 빛 · 바람 · 드묾(조건부)<br>원전 윌 오 더 위스프(영국 민담 도깨비불)<br>기능 없음(동행 · 전투)<br>길들이기 없음 — 병에 담아 등불</div>
<div class="fx"><b>FX:</b> 나방이 남기는 빛 자취 · 빛가루</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/mandrake.jpg')" target="_blank"><img src="/art/creatures/mandrake.jpg" alt="맨드레이크" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/mandrake-v1.jpg')" target="_blank"><img src="/art/creatures/mandrake-v1.jpg" alt="맨드레이크 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">맨드레이크</span> <code>mandrake</code><br>
사람 모양 뿌리 그대로 — 두 다리로 걷는 인삼 뿌리 사람(2026-11-19 사용자). 뽑히면 지르는 비명은 귀가 먹먹해지는 울음(잠깐 기절)
<div class="meta">크기 소형(키 0.4 m) · 속성 풀 · 땅 · 보통(비 온 뒤 많이)<br>원전 맨드레이크(유럽 전승)<br>기능 손재주(농사 — 밭갈이)<br>2서클 테라 · 농사(밭갈이) · 약재</div>
<div class="fx"><b>FX:</b> 입에서 퍼지는 음파 고리 · 튀는 흙먼지 · 잔돌</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/foxfire-fox.jpg')" target="_blank"><img src="/art/creatures/foxfire-fox.jpg" alt="불씨여우" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/foxfire-fox-v1.jpg')" target="_blank"><img src="/art/creatures/foxfire-fox-v1.jpg" alt="불씨여우 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">불씨여우</span> <code>foxfire-fox</code><br>
여우가 켜는 도깨비불 → 꼬리 끝에 숯불 같은 불씨를 단 들여우
<div class="meta">크기 소형(몸길이 0.6 m) · 속성 불 · 바람 · 보통<br>원전 여우불(일본 키츠네비)<br>기능 속성 작업(불 지피기)<br>2서클 이그니스 · 가마 불씨</div>
<div class="fx"><b>FX:</b> 여우불 구슬 · 꼬리 불티 · 풀끝 불씨</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/hill-boar.jpg')" target="_blank"><img src="/art/creatures/hill-boar.jpg" alt="구릉 멧돼지" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/hill-boar-v1.jpg')" target="_blank"><img src="/art/creatures/hill-boar-v1.jpg" alt="구릉 멧돼지 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">구릉 멧돼지</span> <code>hill-boar</code><span class="tag boss">필드 보스</span><br>
전설의 한 마리 대신 그 혈통의 큰 멧돼지 종. 우두머리(거대 개체)가 1번 지역 필드 보스
<div class="meta">크기 대형(어깨 1.5 m) · 속성 땅 · 풀 · 드묾(보스)<br>원전 창작 — 에리만토스의 멧돼지(그리스 신화 ☆)의 먼 혈통<br>기능 탈것(돌진)<br>3서클 테라 · 돌진 탈것 · 밭 · 운반</div>
<div class="fx"><b>FX:</b> 갈라지며 솟는 땅 · 떠오르는 흙덩이 · 돌 조각 · 발자국 들꽃</div>
</div>
</div>
</div>

## 2. 깊은 숲 — 8종 {#region-2}

서클 2~5 · 주 속성 풀 · 물 · 땅. 오픈 베타로 **미리 옮겨 올 후보**. 숲 · 늪 생물이라 1번 지역의 고목 둘레 · 연못 저지대 · 작은 계곡에 놓기 쉽다.

| 이름 | 한 줄 | 크기 | 원전 | 기능 | 희귀도 |
|---|---|---|---|---|---|
| **꿈먹이 맥** `baku` | 여러 짐승을 섞은 원전 → 말레이맥 체형 + 짧은 코끼리 코 + 별무늬 털 | 대형(어깨 1.2 m) | 맥(동아시아 — 악몽을 먹는 짐승) | 없음(동행 · 전투) | 보통 |
| **드라이아드 사슴** `dryad-stag` | 인간형 님프 → 사람 모습을 없애고 나무와 하나인 사슴으로. 뿔이 살아 있는 가지 · 꽃 | 대형(어깨 1.5 m) | 드라이아드(그리스 신화 나무 님프) | 탈것(숲길) | 희귀 |
| **키키모라 늪새** `kikimora-heron` | 여자 모습 정령 → 실을 잣는 습성만 남긴 키 큰 늪새. 반짝이는 물건을 훔쳐 알 같은 실타래 속에 감아 둔다 | 중형(키 1.4 m) | 키키모라(슬라브 신화 집 · 늪 정령) | 손재주(실 잣기) | 보통 |
| **히시 그루터기거북** `hiisi-tortoise` | 숲 정령 → 등딱지가 오래된 그루터기인 큰 거북. 등 위에 버섯 · 이끼 정원 | 대형(등높이 1.3 m) | 히시(핀란드 숲의 정령) | 운반(무거운 짐) · 손재주(벌목) | 보통 |
| **루 카르콜 달팽이** `lou-carcolh` | 사람을 끌어가는 뱀 달팽이 → 순한 큰 늪 달팽이. 점액이 물약 재료 | 중형(껍데기 높이 0.8 m) | 루 카르콜(프랑스 남서부 전승) | 목축(점액) | 보통(비 오면 흔함) |
| **대롱여우** `kuda-fox` | 대롱 속 작은 여우 → 속 빈 갈대 · 대나무 속에 사는 족제비만 한 여우 | 소형(몸길이 0.3 m) | 쿠다키츠네(일본 — 대롱 속 여우) | 없음(동행 · 전투) | 흔함 |
| **아팡크** `afanc` | 수달 + 악어 → 수달 · 비버 머리에 악어 등판. 늪을 막아 댐을 짓는다 | 거대(몸길이 6 m) | 아팡크(웨일스 · 아서왕 전설) | 탈것(헤엄) | 드묾(보스) |
| **오초** `otso` | 숲의 왕 곰. 지역에서 가장 잡기 어려운 전설 — 길들일 수 있다(Q48) | 거대(어깨 3.5 m) | 오초(핀란드 칼레발라 — 숲의 왕 곰) ☆ | 없음(동행 · 전투) | 전설 |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/baku.jpg')" target="_blank"><img src="/art/creatures/baku.jpg" alt="꿈먹이 맥" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/baku-v1.jpg')" target="_blank"><img src="/art/creatures/baku-v1.jpg" alt="꿈먹이 맥 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">꿈먹이 맥</span> <code>baku</code><br>
여러 짐승을 섞은 원전 → 말레이맥 체형 + 짧은 코끼리 코 + 별무늬 털
<div class="meta">크기 대형(어깨 1.2 m) · 속성 풀 · 그림자술 · 보통<br>원전 맥(동아시아 — 악몽을 먹는 짐승)<br>기능 없음(동행 · 전투)<br>3서클 · 동행(곁에 있으면 잠이 깊어짐 — 고유 능력 후보)</div>
<div class="fx"><b>FX:</b> 악몽 안개 · 별가루</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/dryad-stag.jpg')" target="_blank"><img src="/art/creatures/dryad-stag.jpg" alt="드라이아드 사슴" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/dryad-stag-v1.jpg')" target="_blank"><img src="/art/creatures/dryad-stag-v1.jpg" alt="드라이아드 사슴 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">드라이아드 사슴</span> <code>dryad-stag</code><br>
인간형 님프 → 사람 모습을 없애고 나무와 하나인 사슴으로. 뿔이 살아 있는 가지 · 꽃
<div class="meta">크기 대형(어깨 1.5 m) · 속성 풀 · 빛 · 희귀<br>원전 드라이아드(그리스 신화 나무 님프)<br>기능 탈것(숲길)<br>5서클 · 탑승 · 지나간 자리에 어린나무</div>
<div class="fx"><b>FX:</b> 발굽 자국에서 솟는 새싹 · 흩날리는 빛 가루 · 꽃잎</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/kikimora-heron.jpg')" target="_blank"><img src="/art/creatures/kikimora-heron.jpg" alt="키키모라 늪새" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2</summary>
<a class="shot" :href="withBase('/art/creatures/kikimora-heron-v2.jpg')" target="_blank"><img src="/art/creatures/kikimora-heron-v2.jpg" alt="키키모라 늪새 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/kikimora-heron-v1.jpg')" target="_blank"><img src="/art/creatures/kikimora-heron-v1.jpg" alt="키키모라 늪새 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">키키모라 늪새</span> <code>kikimora-heron</code><br>
여자 모습 정령 → 실을 잣는 습성만 남긴 키 큰 늪새. 반짝이는 물건을 훔쳐 알 같은 실타래 속에 감아 둔다
<div class="meta">크기 중형(키 1.4 m) · 속성 물 · 풀 · 보통<br>원전 키키모라(슬라브 신화 집 · 늪 정령)<br>기능 손재주(실 잣기)<br>2서클 아쿠아 · 실 잣기 일꾼 · 둥지에서 잃어버린 물건</div>
<div class="fx"><b>FX:</b> 부리에서 실타래로 이어지는 가는 빛 실 한 가닥(실타래의 빛은 재질)</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/hiisi-tortoise.jpg')" target="_blank"><img src="/art/creatures/hiisi-tortoise.jpg" alt="히시 그루터기거북" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/hiisi-tortoise-v1.jpg')" target="_blank"><img src="/art/creatures/hiisi-tortoise-v1.jpg" alt="히시 그루터기거북 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">히시 그루터기거북</span> <code>hiisi-tortoise</code><br>
숲 정령 → 등딱지가 오래된 그루터기인 큰 거북. 등 위에 버섯 · 이끼 정원
<div class="meta">크기 대형(등높이 1.3 m) · 속성 땅 · 풀 · 보통<br>원전 히시(핀란드 숲의 정령)<br>기능 운반(무거운 짐) · 손재주(벌목)<br>3서클 테라 · 벌목 · 운반 · 등에서 버섯 농사</div>
<div class="fx"><b>FX:</b> 솟는 뿌리(애니메이션 메시) · 흙먼지</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/lou-carcolh.jpg')" target="_blank"><img src="/art/creatures/lou-carcolh.jpg" alt="루 카르콜 달팽이" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/lou-carcolh-v1.jpg')" target="_blank"><img src="/art/creatures/lou-carcolh-v1.jpg" alt="루 카르콜 달팽이 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">루 카르콜 달팽이</span> <code>lou-carcolh</code><br>
사람을 끌어가는 뱀 달팽이 → 순한 큰 늪 달팽이. 점액이 물약 재료
<div class="meta">크기 중형(껍데기 높이 0.8 m) · 속성 물 · 풀 · 보통(비 오면 흔함)<br>원전 루 카르콜(프랑스 남서부 전승)<br>기능 목축(점액)<br>2서클 아쿠아 · 점액(물약 재료)</div>
<div class="fx"><b>FX:</b> 공중에 멈춘 빗방울 · 점액 구슬</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/kuda-fox.jpg')" target="_blank"><img src="/art/creatures/kuda-fox.jpg" alt="대롱여우" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2</summary>
<a class="shot" :href="withBase('/art/creatures/kuda-fox-v2.jpg')" target="_blank"><img src="/art/creatures/kuda-fox-v2.jpg" alt="대롱여우 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/kuda-fox-v1.jpg')" target="_blank"><img src="/art/creatures/kuda-fox-v1.jpg" alt="대롱여우 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">대롱여우</span> <code>kuda-fox</code><br>
대롱 속 작은 여우 → 속 빈 갈대 · 대나무 속에 사는 족제비만 한 여우
<div class="meta">크기 소형(몸길이 0.3 m) · 속성 바람 · 풀 · 흔함<br>원전 쿠다키츠네(일본 — 대롱 속 여우)<br>기능 없음(동행 · 전투)<br>2서클 벤투스 · 털</div>
<div class="fx"><b>FX:</b> 몸을 따라 흐르는 바람 줄기 · 흩날리는 대나무 잎</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/afanc.jpg')" target="_blank"><img src="/art/creatures/afanc.jpg" alt="아팡크" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/afanc-v1.jpg')" target="_blank"><img src="/art/creatures/afanc-v1.jpg" alt="아팡크 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">아팡크</span> <code>afanc</code><span class="tag boss">필드 보스</span><br>
수달 + 악어 → 수달 · 비버 머리에 악어 등판. 늪을 막아 댐을 짓는다
<div class="meta">크기 거대(몸길이 6 m) · 속성 물 · 땅 · 드묾(보스)<br>원전 아팡크(웨일스 · 아서왕 전설)<br>기능 탈것(헤엄)<br>5서클 아쿠아 · 헤엄 탈것</div>
<div class="fx"><b>FX:</b> 솟는 물벽(물 셰이더 메시) · 물보라</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/otso.jpg')" target="_blank"><img src="/art/creatures/otso.jpg" alt="오초" loading="lazy"></a>
<div class="alt">시안 B — 길들인 뒤 동행 장면</div>
<a class="shot" :href="withBase('/art/creatures/otso-b.jpg')" target="_blank"><img src="/art/creatures/otso-b.jpg" alt="오초 시안 B" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/otso-v1.jpg')" target="_blank"><img src="/art/creatures/otso-v1.jpg" alt="오초 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/otso-b-v1.jpg')" target="_blank"><img src="/art/creatures/otso-b-v1.jpg" alt="오초 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">오초</span> <code>otso</code><span class="tag legend">전설</span><br>
숲의 왕 곰. 지역에서 가장 잡기 어려운 전설 — 길들일 수 있다(Q48)
<div class="meta">크기 거대(어깨 3.5 m) · 속성 땅 · 신성 · 전설<br>원전 오초(핀란드 칼레발라 — 숲의 왕 곰) ☆<br>기능 없음(동행 · 전투)<br>길들일 수 있다(Q48) — 요구 7서클(Q134) · 조건 Q160 · 쓰임은 종 데이터 때</div>
<div class="fx"><b>FX:</b> 발밑 빛 꽃 고리 · 반딧불 같은 숲의 빛</div>
</div>
</div>
</div>

## 3. 폭풍 해안 — 8종 {#region-3}

서클 3~6 · 주 속성 바람 · 빛(번개) · 물. 오픈 베타로 **미리 옮겨 올 후보**. 바닷가 · 절벽 생물이라 1번 지역 남쪽 바닷가 곶 · 북쪽 고지 절벽에 놓을 수 있다. 뇌우 조건부 종은 날씨가 있어야 나온다.

| 이름 | 한 줄 | 크기 | 원전 | 기능 | 희귀도 |
|---|---|---|---|---|---|
| **라이쥬 번개족제비** `raiju` | 뇌수 → 뇌우 때만 나오는 담비. 번개 털은 빛 마법 재료 | 소형(몸길이 0.5 m) | 라이쥬(일본 — 번개와 함께 떨어지는 짐승) | 없음(동행 · 전투) | 드묾(조건부) |
| **할시온 물총새** `halcyon` | 그대로. 둥지 근처는 파도 · 바람이 약해진다 | 소형(몸길이 0.3 m) | 할시온(그리스 신화 — 파도를 잠재우는 새) | 없음(동행 · 전투) | 보통 |
| **세이렌** `siren` | 사람 머리의 새 → 사람 아닌 얼굴의 어인(인어형, 다리 없음, 2026-11-19 사용자). 노래로 홀리는 습성 그대로 — 노래가 들리면 걸음이 끌려간다 | 중형(몸길이 2.4 m(꼬리가 2/3)) | 세이렌(그리스 신화) | 방범(노래로 침입자를 붙잡음) | 보통 |
| **페리톤** `peryton` | 사슴 머리 + 큰 새 몸. 그림자가 사람 모양이라는 원전 설정은 뺌 | 대형(어깨 1.6 m) | 페리톤(중세 전설 — 사슴 머리의 새) | 날것(활공) | 보통 |
| **번개매** `thunder-hawk` | 전설의 천둥새 대신 그 혈통의 큰 매 종 | 대형(날개 폭 5 m) | 창작 — 천둥새(북미 원주민 전승 ☆)의 아래 혈통 | 날것(비행) | 드묾 |
| **폭풍 해파리** `storm-jelly` | 폭풍 바람을 타고 하늘을 떠다니는 해파리. 뇌우 뒤 해안에 내려앉는다 | 중형(우산 지름 1 m) | 창작 | 없음(동행 · 전투) | 드묾(조건부) |
| **카르키노스 게** `karkinos` | 전설의 한 마리 → 같은 이름의 거대 게 종. 등에 따개비 · 바위 | 거대(폭 6 m) | 카르키노스(그리스 신화 ☆ 거대 게) | 손재주(채광 — 바위 깨기) | 드묾(보스) |
| **천둥새** `thunderbird` | 폭풍 해안의 전설. 날갯짓이 천둥, 눈빛이 번개. 원전의 신성함을 존중해 묘사는 정중하게 — 잡기 · 교배는 다른 전설과 같다(Q162) | 거대(날개 폭 20 m) | 천둥새(북미 원주민 전승) ☆ | 없음(동행 · 전투) | 전설 |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/raiju.jpg')" target="_blank"><img src="/art/creatures/raiju.jpg" alt="라이쥬 번개족제비" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/raiju-v1.jpg')" target="_blank"><img src="/art/creatures/raiju-v1.jpg" alt="라이쥬 번개족제비 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">라이쥬 번개족제비</span> <code>raiju</code><br>
뇌수 → 뇌우 때만 나오는 담비. 번개 털은 빛 마법 재료
<div class="meta">크기 소형(몸길이 0.5 m) · 속성 빛 · 바람 · 드묾(조건부)<br>원전 라이쥬(일본 — 번개와 함께 떨어지는 짐승)<br>기능 없음(동행 · 전투)<br>4서클 · 번개 털(빛 마법 재료)</div>
<div class="fx"><b>FX:</b> 번개 줄기 · 몸 둘레 전기 아크 · 발밑 그을린 고리</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/halcyon.jpg')" target="_blank"><img src="/art/creatures/halcyon.jpg" alt="할시온 물총새" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/halcyon-v1.jpg')" target="_blank"><img src="/art/creatures/halcyon-v1.jpg" alt="할시온 물총새 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">할시온 물총새</span> <code>halcyon</code><br>
그대로. 둥지 근처는 파도 · 바람이 약해진다
<div class="meta">크기 소형(몸길이 0.3 m) · 속성 물 · 바람 · 보통<br>원전 할시온(그리스 신화 — 파도를 잠재우는 새)<br>기능 없음(동행 · 전투)<br>3서클 아쿠아 · 둥지 근처 날씨 완화</div>
<div class="fx"><b>FX:</b> 물 깃 끝에서 떨어지는 물방울 · 잔잔해지는 둥근 수면(바다 셰이더 범위)</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/siren.jpg')" target="_blank"><img src="/art/creatures/siren.jpg" alt="세이렌" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2 · v3</summary>
<a class="shot" :href="withBase('/art/creatures/siren-v3.jpg')" target="_blank"><img src="/art/creatures/siren-v3.jpg" alt="세이렌 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/siren-v2.jpg')" target="_blank"><img src="/art/creatures/siren-v2.jpg" alt="세이렌 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/siren-v1.jpg')" target="_blank"><img src="/art/creatures/siren-v1.jpg" alt="세이렌 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">세이렌</span> <code>siren</code><br>
사람 머리의 새 → 사람 아닌 얼굴의 어인(인어형, 다리 없음, 2026-11-19 사용자). 노래로 홀리는 습성 그대로 — 노래가 들리면 걸음이 끌려간다
<div class="meta">크기 중형(몸길이 2.4 m(꼬리가 2/3)) · 속성 바람 · 물 · 보통<br>원전 세이렌(그리스 신화)<br>기능 방범(노래로 침입자를 붙잡음)<br>4서클 벤투스 · 거점 경비(노래로 침입자를 붙잡음) · 비늘</div>
<div class="fx"><b>FX:</b> 빛나는 음파 리본 · 꼬리 끝 물보라</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/peryton.jpg')" target="_blank"><img src="/art/creatures/peryton.jpg" alt="페리톤" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18) · v2</summary>
<a class="shot" :href="withBase('/art/creatures/peryton-v2.jpg')" target="_blank"><img src="/art/creatures/peryton-v2.jpg" alt="페리톤 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/peryton-v1.jpg')" target="_blank"><img src="/art/creatures/peryton-v1.jpg" alt="페리톤 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">페리톤</span> <code>peryton</code><br>
사슴 머리 + 큰 새 몸. 그림자가 사람 모양이라는 원전 설정은 뺌
<div class="meta">크기 대형(어깨 1.6 m) · 속성 바람 · 땅 · 보통<br>원전 페리톤(중세 전설 — 사슴 머리의 새)<br>기능 날것(활공)<br>5서클 벤투스 · 활공 탈것</div>
<div class="fx"><b>FX:</b> 날개를 감는 바람 줄기 · 흩날리는 깃</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/thunder-hawk.jpg')" target="_blank"><img src="/art/creatures/thunder-hawk.jpg" alt="번개매" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/thunder-hawk-v1.jpg')" target="_blank"><img src="/art/creatures/thunder-hawk-v1.jpg" alt="번개매 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">번개매</span> <code>thunder-hawk</code><br>
전설의 천둥새 대신 그 혈통의 큰 매 종
<div class="meta">크기 대형(날개 폭 5 m) · 속성 빛 · 바람 · 드묾<br>원전 창작 — 천둥새(북미 원주민 전승 ☆)의 아래 혈통<br>기능 날것(비행)<br>7서클 · 비행 탈것</div>
<div class="fx"><b>FX:</b> 번개 빛 자취 · 깃 끝 전기</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/storm-jelly.jpg')" target="_blank"><img src="/art/creatures/storm-jelly.jpg" alt="폭풍 해파리" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/storm-jelly-v1.jpg')" target="_blank"><img src="/art/creatures/storm-jelly-v1.jpg" alt="폭풍 해파리 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">폭풍 해파리</span> <code>storm-jelly</code><br>
폭풍 바람을 타고 하늘을 떠다니는 해파리. 뇌우 뒤 해안에 내려앉는다
<div class="meta">크기 중형(우산 지름 1 m) · 속성 빛 · 물 · 드묾(조건부)<br>원전 창작<br>기능 없음(동행 · 전투)<br>길들이기 없음 — 빛 젤(등불 연료)</div>
<div class="fx"><b>FX:</b> 촉수 끝 번개 · 모래 위 유리 꽃(소품 메시)</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/karkinos.jpg')" target="_blank"><img src="/art/creatures/karkinos.jpg" alt="카르키노스 게" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/karkinos-v1.jpg')" target="_blank"><img src="/art/creatures/karkinos-v1.jpg" alt="카르키노스 게 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">카르키노스 게</span> <code>karkinos</code><span class="tag boss">필드 보스</span><br>
전설의 한 마리 → 같은 이름의 거대 게 종. 등에 따개비 · 바위
<div class="meta">크기 거대(폭 6 m) · 속성 물 · 땅 · 드묾(보스)<br>원전 카르키노스(그리스 신화 ☆ 거대 게)<br>기능 손재주(채광 — 바위 깨기)<br>6서클 · 바위 깨기 일꾼 · 껍데기(방어구)</div>
<div class="fx"><b>FX:</b> 솟는 물기둥 · 바위 파편</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/thunderbird.jpg')" target="_blank"><img src="/art/creatures/thunderbird.jpg" alt="천둥새" loading="lazy"></a>
<div class="alt">시안 B — 길들인 뒤 동행 장면</div>
<a class="shot" :href="withBase('/art/creatures/thunderbird-b.jpg')" target="_blank"><img src="/art/creatures/thunderbird-b.jpg" alt="천둥새 시안 B" loading="lazy"></a>
<details class="old"><summary>옛 판 보기 — v1(2026-11-18)</summary>
<a class="shot" :href="withBase('/art/creatures/thunderbird-v1.jpg')" target="_blank"><img src="/art/creatures/thunderbird-v1.jpg" alt="천둥새 옛 판" loading="lazy"></a>
<a class="shot" :href="withBase('/art/creatures/thunderbird-b-v1.jpg')" target="_blank"><img src="/art/creatures/thunderbird-b-v1.jpg" alt="천둥새 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">천둥새</span> <code>thunderbird</code><span class="tag legend">전설</span><br>
폭풍 해안의 전설. 날갯짓이 천둥, 눈빛이 번개. 원전의 신성함을 존중해 묘사는 정중하게 — 잡기 · 교배는 다른 전설과 같다(Q162)
<div class="meta">크기 거대(날개 폭 20 m) · 속성 빛 · 바람 · 전설<br>원전 천둥새(북미 원주민 전승) ☆<br>기능 없음(동행 · 전투)<br>길들일 수 있다(Q48) — 요구 7서클(Q134) · 조건 Q160 · 쓰임은 종 데이터 때</div>
<div class="fx"><b>FX:</b> 둘레로 밀려나는 구름 고리 · 번개</div>
</div>
</div>
</div>

## 새 종 후보 2차 — 처음 보는 보석 · 유리 생물 {#new-species}

2026-11-20 사용자 피드백 — 1차 새 종 5종(2026-11-19, 참고 그림 5장의 보석 · 유리 느낌) 중 **카벙클은 「귀엽다, 다듬자」**, 나머지 넷은 **「너무 뻔하다」**(유리 도마뱀 · 날개 뱀 · 새 같은 용 · 머리 여럿 뱀 — 이미 본 판타지 원형에 보석 재질만 입혔다). 그래서 이번에는 시트부터 그리지 않고, 글로 후보 27개를 쓰고 아래 기준으로 쳐낸 뒤 남은 7개를 큰 그림 한 장씩(마법 쓰는 순간) 그렸다. **고르는 건 사용자다** — 카벙클 시안은 [Q220](../spec/decisions#미결-질문), 새 후보는 Q221. 고른 뒤에 4면 시트를 그린다. 정본은 `art/creatures/ideas.py`.

**「뻔하지 않다」 기준** — 그리기 전에 글로 통과시켰다:

1. 「실제 동물(또는 용 · 유니콘 · 그리핀 같은 판타지 원형) + 붙인 마법 재료(뿔 · 이끼 · 보석 · 유리 · 불꽃)」 공식이 아니다 — 마법이 몸의 구조나 사는 방식을 바꾼다
2. 한 줄 고리 — 플레이어가 「걔 ○○한다?」 하고 남에게 말할 행동 · 생태 · 관계(켈피 · 맨드레이크 · 재칼로프처럼 하는 짓이 기억된다)
3. 대비 하나 — 귀여운데 이상하다 · 작은데 위험하다 · 아름다운데 섬뜩하다(공포 아님) · 하찮은 것이 마법적이다
4. 실루엣만 봐도 알아보고, 포켓몬 · 팰월드 · 파이널판타지 · 지브리 · 몬스터헌터의 알려진 생물과 겹치지 않는다
5. 그리모리 시스템 하나와 닿는다 — 서클 · 스킬북 · 마나 · 속성 상태 · 파크툼 계약 · 마법탑 · 생존(추위 · 배고픔 · 밤) · 교배
6. 구현 가능성 — 몸은 메시 · 재질, 흩어지는 것은 FX(FX 를 다 지워도 몸 실루엣이 선다)

### 카벙클 시안 — 하나를 고른다(Q220) {#carbuncle-drafts}

지킨 것: 여우 · 고양이 얼굴 · 큰 귀 · 흰 가슴털 · 무지갯빛 투명 날개 넷 · 지느러미 꼬리 · 회화적인 빛. 고친 것: 파이널판타지 카벙클(긴 귀 · 푸른 몸 · 이마의 붉은 보석)과 거리를 두려고 **보석 자리 · 색 · 모양과 귀 모양을 모두 바꿨고**, 흔한 잠자리 날개를 **보석을 깎은 면(패싯) 같은 얇은 판**으로, 크기가 읽히게 사람 실루엣을 바로 옆에, 아래 띠에 **표정 넷**(보석 빛이 기분)을 넣었다. 원전 고리: 남미 · 스페인 광부 전승 — 동굴에서 보석이 등불처럼 빛나고, 얻으면 부자가 되지만 결코 잡히지 않는다.

<div class="creature-card" style="margin:12px 0"><a class="shot" :href="withBase('/art/creatures/carbuncle-drafts.jpg')" target="_blank"><img src="/art/creatures/carbuncle-drafts.jpg" alt="카벙클 시안 모아 보기" loading="lazy"></a></div>

| 시안 | 고리 한 줄 | 몸 · 보석 · 귀 · 날개 |
|---|---|---|
| 지금 판(1차) | 이마 보석으로 빛을 모은다 | 푸른 털 · 이마 붉은 보석 · 큰 귀 · 잠자리 날개 넷 |
| **A 지금 판 다듬기** | 「동굴에 들어가면 가슴 보석이 등불처럼 켜지는데, 그 빛 색이 기분이야」 | 따뜻한 모래색 · 크림 털, 귀 끝 · 꼬리 쪽으로 연보라 그러데이션(푸른 몸 아님) · 이마가 아니라 가슴 흰 털 한가운데 둥근 오팔 카보숑 — 무지갯빛, 기분 따라 도는 색이 바뀐다 · 크고 둥근 부채 귀, 끝에 스라소니 같은 털 술 · 잠자리 날개 대신 보석을 깎은 면(패싯)처럼 각진 얇은 수정 판 네 장 — 면마다 빛이 다르게 꺾인다(얇은 메시 + 프레넬) |
| **B 이야기 고리 밀기** | 「동굴에서 등불처럼 빛나서 따라가면, 날개로 보석을 덮는 순간 깜깜해지고 사라져. 아무도 못 잡아」 | 짙은 밤색 · 구릿빛 털, 흰 가슴털 · 이마가 아니라 등 한가운데 날개 뿌리 사이에 박힌 길쭉한 눈물 모양 호박색 보석(패싯) · 넓고 큰 귀, 끝이 둥글게 살짝 접힘 · 딱정벌레 겉날개처럼 등 보석 위로 포개 접히는 얇은 패싯 수정 판 네 장 — 접으면 보석 빛이 가려진다 |
| **C 자유 — 계약하면 보석을 나눠 줌** | 「계약하면 걔가 목의 보석 하나를 떼어 줘. 걔가 멀어지면 내 보석이 식어」 | 은백색 털, 귀 끝 · 발끝은 먹빛 · 이마가 아니라 목둘레에 작은 패싯 결정 여러 개가 목걸이처럼 자란 결정 깃 — 연분홍 · 연초록 · 크고 뾰족한데 귀 끝 가장자리가 지느러미 막처럼 투명 · 접부채처럼 주름이 접힌 얇은 수정 막 네 장(펴면 부채) |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/carbuncle-a.jpg')" target="_blank"><img src="/art/creatures/carbuncle-a.jpg" alt="카벙클 A 지금 판 다듬기" loading="lazy"></a>
<div class="body">
<span class="name">카벙클 A 지금 판 다듬기</span> <code>carbuncle-a</code><br>
<b>「동굴에 들어가면 가슴 보석이 등불처럼 켜지는데, 그 빛 색이 기분이야」</b>
<div class="meta">어두운 동굴 입구, 사람 실루엣이 그림 왼쪽에 머리부터 발까지 그림 높이 거의 전부를 차지하고 서 있고, 카벙클은 그 발치 바로 옆 낮은 바위 위에 날개를 반쯤 펴고 앉아 사람을 올려다본다 — 가슴 오팔이 등불처럼 빛나 둘레 바위벽의 광맥이 반짝인다</div>
<div class="fx"><b>FX:</b> 가슴 빛(조명) · 날개 둘레 빛가루</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/carbuncle-b.jpg')" target="_blank"><img src="/art/creatures/carbuncle-b.jpg" alt="카벙클 B 이야기 고리 밀기" loading="lazy"></a>
<div class="body">
<span class="name">카벙클 B 이야기 고리 밀기</span> <code>carbuncle-b</code><br>
<b>「동굴에서 등불처럼 빛나서 따라가면, 날개로 보석을 덮는 순간 깜깜해지고 사라져. 아무도 못 잡아」</b>
<div class="meta">깜깜한 옛 광산 굴. 등 보석이 등불처럼 굴을 비추다가, 손을 뻗은 사람 실루엣 앞에서 날개를 접어 보석을 덮는 순간 — 빛이 반쯤 가려져 몸 뒤쪽 절반이 어둠에 잠긴다. 장난스러운 눈</div>
<div class="fx"><b>FX:</b> 날개 틈으로 새는 빛줄기 · 굴 바닥 먼지</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/carbuncle-c.jpg')" target="_blank"><img src="/art/creatures/carbuncle-c.jpg" alt="카벙클 C 자유 — 계약하면 보석을 나눠 줌" loading="lazy"></a>
<div class="body">
<span class="name">카벙클 C 자유 — 계약하면 보석을 나눠 줌</span> <code>carbuncle-c</code><br>
<b>「계약하면 걔가 목의 보석 하나를 떼어 줘. 걔가 멀어지면 내 보석이 식어」</b>
<div class="meta">사람 실루엣이 내민 손 앞 공중에 떠, 목의 결정 하나를 떼어 빛 실로 이어 건네는 순간(파크툼 계약)</div>
<div class="fx"><b>FX:</b> 결정과 손 사이 빛 실 · 둘레 빛가루</div>
</div>
</div>
</div>

### 새 후보 7 — 고른다(Q221) {#ideas}

<div class="creature-card" style="margin:12px 0"><a class="shot" :href="withBase('/art/creatures/ideas-board.jpg')" target="_blank"><img src="/art/creatures/ideas-board.jpg" alt="새 후보 모아 보기" loading="lazy"></a></div>

| # | 이름 | 고리 한 줄 | 대비 | 닿는 시스템 | 제안 지역 · 크기 |
|---|---|---|---|---|---|
| 1 | **산산이** `shatterling` | 「놀라면 몸이 색유리 조각으로 와르르 흩어졌다가 다시 붙어. 조각 하나 주워 가면 그거 찾으러 집까지 따라와」 | 귀여운데 이상하다 | 길들이기 — 파크툼 대신 「떨어진 조각 줍기」로 거점까지 따라오게 하는 길들이기 변형 · 교배(판 색 조합이 새끼에게) | 1. 마법탑 · 온난 지대 · 몸길이 0.5 m |
| 2 | **방울코** `bubble-snout` | 「코로 유리 비눗방울을 부는데, 큰 걸 불면 그 안에 들어가 둥실 떠서 강을 건너」 | 하찮은 것(비눗방울)이 마법적이다 | 탈것(방울을 타고 개울 · 골짜기를 짧게 떠서 건넘) · 운반(짐을 방울에 담아 띄움) · 굳은 방울 = 얇은 유리 그릇(제작 재료) | 1. 마법탑 · 온난 지대 · 어깨 높이 1.3 m |
| 3 | **불씨도둑** `ember-thief` | 「밤에 모닥불을 훔쳐 목주머니에 담아 가. 따라가면 둥지에 훔친 불이 잔뜩이야」 | 작은데 위험하다(밤에 불을 잃으면 춥다) | 생존(밤 · 추위 · 모닥불) · 속성(곁에서 쓴 불 · 빛 주문도 빨아 감) | 2. 깊은 숲 · 몸길이 0.7 m(꼬리 빼고) |
| 4 | **정동걸음이** `geode-walker` | 「길가 바위인 줄 알았는데, 밤에 쩍 갈라지더니 안에서 보석 다리가 나와 걸어가」 | 하찮은 것(돌)이 마법적이다 | 생존(밤에만 움직임) · 교배(먹인 광물에 따라 속 수정 색이 바뀌고 새끼에게 이어짐) · 채광(떨어진 수정 조각) | 2. 깊은 숲 · 바위 지름 2 m · 일어서면 키 2.6 m |
| 5 | **거울못** `mirror-pond` | 「숲속 못인 줄 알고 들어갔는데, 못이 일어났어」 | 아름다운데 섬뜩하다(공포가 아니라 놀라움) | 대형 탈것(등이 평평한 거울 뗏목 — 일행 여럿이 타고 호수 · 늪을 건넘) · 파크툼 막시마(큰 것에 잘 듣는 계약) | 2. 깊은 숲 · 몸 지름 8 m · 두께 1 m |
| 6 | **번개뿌리** `fulgurite` | 「번개 떨어진 모래에서 번개 모양 그대로 굳은 유리가 일어나 기어 다녀. 건드리면 찌릿해」 | 하찮은 것(모래가 녹은 유리)이 마법적이다 | 날씨(뇌우 뒤에만 깨어남) · 속성 상태(감전) · 길들이면 거점 피뢰침(뇌우 때 번개를 받아 마나를 모음) | 3. 폭풍 해안 · 키 2.5 m · 펼친 길이 4 m |
| 7 | **무지개그늘** `rainbow-shade` | 「걔는 거의 안 보여. 풀밭에 무지개 그림자만 따라와」 | 아름다운데 섬뜩하다(공포 아님) | 속성 상태(빛 — 눈부심) · 사냥(무지개 그림자를 보고 쫓아 잡기) | 3. 폭풍 해안 · 키 2.2 m |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/shatterling.jpg')" target="_blank"><img src="/art/creatures/shatterling.jpg" alt="산산이" loading="lazy"></a>
<div class="alt">첫 판 — 다른 순간</div>
<a class="shot" :href="withBase('/art/creatures/shatterling-t1.jpg')" target="_blank"><img src="/art/creatures/shatterling-t1.jpg" alt="산산이 첫 판" loading="lazy"></a>
<div class="body">
<span class="name">산산이</span> <code>shatterling</code><br>
<b>「놀라면 몸이 색유리 조각으로 와르르 흩어졌다가 다시 붙어. 조각 하나 주워 가면 그거 찾으러 집까지 따라와」</b>
<div class="meta">속성 바람 · 빛 · 소형(몸길이 0.5 m) · 겁 많음<br>원전 창작<br>길들이기 · 쓰임 1서클 · 조각을 주워 거점으로 데려옴 · 동행<br>개수 다리 4 · 귀 2 · 심장 빛 1<br>겹침 확인: 알려진 「흩어졌다 붙는 유리 짐승」 없음. 마스코트 6종 · 1번 지역 10종과 겹치는 고리 없음</div>
<div class="fx"><b>FX:</b> 흩어진 판 사이를 잇는 가는 빛 실 · 판 모서리 반짝임</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/bubble-snout.jpg')" target="_blank"><img src="/art/creatures/bubble-snout.jpg" alt="방울코" loading="lazy"></a>
<div class="body">
<span class="name">방울코</span> <code>bubble-snout</code><br>
<b>「코로 유리 비눗방울을 부는데, 큰 걸 불면 그 안에 들어가 둥실 떠서 강을 건너」</b>
<div class="meta">속성 물 · 바람 · 중형(어깨 높이 1.3 m) · 온순(느긋함)<br>원전 창작<br>길들이기 · 쓰임 2서클 · 방울 탈것 · 유리 구슬<br>개수 다리 4 · 코 1<br>겹침 확인: 켈피(물 위를 달림)와 달리 방울로 떠서 건넌다. 코끼리 · 맥 몸이 아니다</div>
<div class="fx"><b>FX:</b> 방울이 터질 때 튀는 물방울 · 방울 표면의 무지갯빛 일렁임(재질)</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/ember-thief.jpg')" target="_blank"><img src="/art/creatures/ember-thief.jpg" alt="불씨도둑" loading="lazy"></a>
<div class="body">
<span class="name">불씨도둑</span> <code>ember-thief</code><br>
<b>「밤에 모닥불을 훔쳐 목주머니에 담아 가. 따라가면 둥지에 훔친 불이 잔뜩이야」</b>
<div class="meta">속성 불 · 바람 · 소형(몸길이 0.7 m(꼬리 빼고)) · 장난기(겁 많음)<br>원전 창작 — 「불을 훔친 짐승」 설화들(여러 지역의 불 도둑 동물 이야기)<br>길들이기 · 쓰임 3서클 · 둥지의 「꺼지지 않는 불씨」(재료) · 길들이면 불씨를 나르는 동행<br>개수 다리 4 · 꼬리 1<br>겹침 확인: 마스코트 꼬리등불(불을 지키고 꼬리 끝 꽈리 등)과 반대로 불을 훔치고, 불은 목주머니에. 불씨여우(불을 냄)와 다르게 불을 가져간다. 그림은 앞쪽에 있어 커 보이지만 몸길이 0.7 m</div>
<div class="fx"><b>FX:</b> 빨려 드는 불꽃 줄기 · 숯 위 연기</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/geode-walker.jpg')" target="_blank"><img src="/art/creatures/geode-walker.jpg" alt="정동걸음이" loading="lazy"></a>
<div class="body">
<span class="name">정동걸음이</span> <code>geode-walker</code><br>
<b>「길가 바위인 줄 알았는데, 밤에 쩍 갈라지더니 안에서 보석 다리가 나와 걸어가」</b>
<div class="meta">속성 땅 · 빛 · 대형(바위 지름 2 m · 일어서면 키 2.6 m) · 겁 많음(낮엔 꼼짝 않음)<br>원전 창작 — 정동(속이 빈 돌 안에 수정이 자란 것)<br>길들이기 · 쓰임 3서클 · 수정 조각 · 교배로 수정 색 모으기<br>개수 수정 다리 4 · 눈 2<br>겹침 확인: 포켓몬 멜리시(Carbink)는 돌에 박힌 보석, 이쪽은 「열려야 보이는 속」과 걷는 수정 다리. 히시 거북 · 카르키노스와 다름</div>
<div class="fx"><b>FX:</b> 껍데기가 열릴 때 떨어지는 돌가루 · 수정 둘레 작은 빛가루</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/mirror-pond.jpg')" target="_blank"><img src="/art/creatures/mirror-pond.jpg" alt="거울못" loading="lazy"></a>
<div class="body">
<span class="name">거울못</span> <code>mirror-pond</code><br>
<b>「숲속 못인 줄 알고 들어갔는데, 못이 일어났어」</b>
<div class="meta">속성 물 · 빛 · 거대(몸 지름 8 m · 두께 1 m) · 온순(수줍음)<br>원전 창작 — 「호수에 사는 것」 전승을 뒤집음(호수 속이 아니라 호수 자체)<br>길들이기 · 쓰임 5서클 · 일행 탈것(늪 · 호수 건너기)<br>개수 다리 4 · 머리 1<br>겹침 확인: 아스피도켈론(등이 섬)과 달리 등이 비어 있는 거울, 아팡크(호수 보스)와 달리 순한 탈것. 1차의 늪 히드라 자리(큰 탈것)를 대신한다</div>
<div class="fx"><b>FX:</b> 가장자리에서 쏟아지는 물 · 물안개 · 물결 고리</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/fulgurite.jpg')" target="_blank"><img src="/art/creatures/fulgurite.jpg" alt="번개뿌리" loading="lazy"></a>
<div class="body">
<span class="name">번개뿌리</span> <code>fulgurite</code><br>
<b>「번개 떨어진 모래에서 번개 모양 그대로 굳은 유리가 일어나 기어 다녀. 건드리면 찌릿해」</b>
<div class="meta">속성 빛 · 땅 · 대형(키 2.5 m · 펼친 길이 4 m) · 겁 많음(건드리면 찌릿)<br>원전 창작 — 풀구라이트(번개가 모래를 녹여 만든 실제 유리 관)<br>길들이기 · 쓰임 4서클 · 번개 유리(빛 마법 재료) · 거점 피뢰침<br>개수 머리 1 · 땅을 짚는 가지 다리 6<br>겹침 확인: 폭풍 해파리(유리 꽃을 남김)와 달리 그 자신이 번개 유리. 1차의 피랄리스(도마뱀 + 유리)를 대신한다</div>
<div class="fx"><b>FX:</b> 가지 끝에서 튀는 전기 불꽃 · 모래에서 오르는 김 · 내리꽂히는 번개</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/rainbow-shade.jpg')" target="_blank"><img src="/art/creatures/rainbow-shade.jpg" alt="무지개그늘" loading="lazy"></a>
<div class="body">
<span class="name">무지개그늘</span> <code>rainbow-shade</code><br>
<b>「걔는 거의 안 보여. 풀밭에 무지개 그림자만 따라와」</b>
<div class="meta">속성 빛 · 바람 · 대형(키 2.2 m) · 경계(사냥꾼 — 사람은 피함)<br>원전 창작<br>길들이기 · 쓰임 5서클 · 프리즘 유리(빛 마법 재료)<br>개수 다리 4<br>겹침 확인: 숨는 생물(히드비하인드 — 6번)과 달리 빛 때문에 들킨다</div>
<div class="fx"><b>FX:</b> 풀에 떨어진 무지개 빛(라이트 함수 · 데칼) · 모서리 반짝임</div>
</div>
</div>
</div>

### 쳐낸 후보 20

| 후보 | 고리 · 생김새 | 쳐낸 까닭 |
|---|---|---|
| 피랄리스 유리도마뱀(1차) | 번개 유리 모래에서 깨어나는 유리 도마뱀 | 도마뱀 + 유리 날개 — 공식 그대로. 「번개 유리」 생각은 번개뿌리로 |
| 나비볏 암피프테르(1차) | 스테인드글라스 나비 볏을 두른 뱀 | 뱀 + 색유리 볏 — 원형에 재질만 입힘 |
| 꽃깃 용새(1차) | 꽃가루 구름으로 짐승을 재우는 용새 | 새 · 용 원형 + 꽃. 고리(졸음)가 생김새에서 안 읽힘 |
| 늪 히드라(1차) | 후드를 펴 겁주는 목 다섯 늪 짐승 | 머리 여럿 뱀 원형 그대로. 큰 탈것 자리는 거울못으로 |
| 나비떼 사슴 | 가까이 가면 색유리 나비 수백 마리로 흩어지는 사슴 | FX 를 다 지우면 몸이 없다 — 구현 규칙 위반 |
| 유리 흉내쟁이 | 마지막에 본 생물 모양으로 유리 몸을 굳힘 | 메타몽 · 조로아크의 변신 고리와 겹침 |
| 거울 비늘 짐승 | 맞은 주문을 거울 비늘로 튕겨 돌려보냄 | 동물 + 거울 비늘 공식, 마스코트 마나결정 천산갑과 겹침 |
| 주문 앵무 | 들은 주문을 따라 하다 엉뚱하게 틀림 | 새 + 유리, 말 따라 하는 앵무 고리 그대로 |
| 샹들리에 사슴 | 수정 뿔에 촛불이 켜진 사슴 떼 | 사슴 + 수정 뿔 — 공식 그 자체 |
| 유리 온실 짐승 | 등의 유리 온실에서 겨울에도 작물이 자람 | 히시 거북 · 아스피도켈론의 「등 위 세계」와 겹침 |
| 유리 연꽃 미끼 | 못의 유리 연꽃을 따려 하면 못이 눈을 뜸 | 켈피 · 아팡크의 「물가로 꾀기」와 겹침 |
| 무지개 누에 | 햇빛을 먹고 무지개 실을 뽑음 | 키키모라의 실 잣기와 겹침 |
| 보석 먹는 짐승 | 먹은 보석 색으로 털 색이 바뀜 | 재칼로프의 베리 · 뿔 색 고리와 겹침 |
| 돋보기 벌집 | 유리벌 벌집이 돋보기라 해 뜨면 밑에 불이 붙음 | 개체가 너무 작고 무리 · 집이 주인공 — 길들이기 · 교배 단위가 안 맞음 |
| 주문 유리병 | 맞은 주문이 몸속 병에 담겨 그 속성이 됨 | 불씨도둑과 같은 「유리 몸에 담기」 — 하나로 합침(불씨도둑이 곁의 주문도 빨아 감) |
| 모래시계 짐승 | 모래가 다 떨어지면 잠들어 뒤집어 줘야 깨어남 | 물건 + 다리 — 생물보다 소품으로 읽힘 |
| 이슬보석 거미 | 밤새 이슬을 보석으로 굳혀 거미줄에 걺, 해 뜨면 녹음 | 거미 + 보석 공식, 「아침 산물」 고리가 흙닭 · 바로메츠와 겹침 |
| 보석 눈물 짐승 | 슬플 때 흘린 눈물이 보석이 됨 | 플레이어가 괴롭혀야 얻는 고리 — 톤 위반 |
| 하늘 유리 고래 | 구름 위를 헤엄치는 유리 고래 | 판타지에서 너무 흔한 그림(하늘 고래) |
| 소금 결정 가축 | 등에 자라는 소금 결정으로 고기를 절임 | 가축 + 결정 공식 |

### 1차 판(2026-11-19) — 접어 둠

「뻔하다」로 쳐낸 1차 넷. 이름 · 설명은 `make_prompts.py` 의 `NEW_SPECIES` 에 남겨 두었다(카벙클 지금 판은 위 시안 표의 「지금 판」).

<details class="old"><summary>1차 판 보기 — 피랄리스 유리도마뱀 · 나비볏 암피프테르 · 꽃깃 용새 · 늪 히드라</summary>

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/pyrallis.jpg')" target="_blank"><img src="/art/creatures/pyrallis.jpg" alt="피랄리스 유리도마뱀" loading="lazy"></a>
<div class="body">
<span class="name">피랄리스 유리도마뱀</span> <code>pyrallis</code><br>
용광로 불에서 나는 짐승 → 번개가 모래를 녹인 유리(뇌우 뒤 모래톱) 사이에서 깨어나는 유리 도마뱀
<div class="meta">크기 소형(몸길이 0.6 m(꼬리 포함)) · 속성 빛 · 바람 · 드묾(조건부)<br>원전 피랄리스(플리니우스 「박물지」 — 구리 용광로의 불 속에서 사는 날개 달린 작은 짐승)<br>기능 없음(동행 · 전투)<br>4서클 · 동행 · 허물(유리 비늘 — 빛 마법 재료)</div>
<div class="fx"><b>FX:</b> 발밑 유리 모래의 반짝임 · 몸 둘레 작은 전기 불꽃</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/amphiptere.jpg')" target="_blank"><img src="/art/creatures/amphiptere.jpg" alt="나비볏 암피프테르" loading="lazy"></a>
<div class="body">
<span class="name">나비볏 암피프테르</span> <code>amphiptere</code><br>
날개 달린 뱀 → 날개 대신 머리 둘레에 스테인드글라스 같은 나비 날개 볏을 목도리처럼 두른 순한 뱀
<div class="meta">크기 중형(몸길이 2.5 m) · 속성 풀 · 빛 · 보통<br>원전 암피프테르(유럽 문장 — 다리 없는 날개 뱀)<br>기능 방범(볏 빛으로 침입자를 눈부시게)<br>3서클 · 동행 · 거점 경비(볏 빛으로 침입자를 눈부시게) · 허물(색유리 재료)</div>
<div class="fx"><b>FX:</b> 땅에 비친 색 빛 무늬(라이트 함수 · 데칼) · 볏 둘레 빛가루</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/blossom-drake.jpg')" target="_blank"><img src="/art/creatures/blossom-drake.jpg" alt="꽃깃 용새" loading="lazy"></a>
<div class="body">
<span class="name">꽃깃 용새</span> <code>blossom-drake</code><br>
봉황 · 불사조와 다르게 땅에 사는 키 큰 용새(긴 다리로 달리고 짧게만 난다). 몸이 꽃 피는 숲과 하나 — 잎 비늘 · 꽃잎 볏
<div class="meta">크기 대형(머리 높이 2.4 m) · 속성 풀 · 빛 · 희귀<br>원전 창작 — 꽃 핀 숲의 용새<br>기능 탈것(달리기 · 짧은 활공)<br>5서클 · 탑승(달리기 · 짧은 활공) · 꽃가루(잠 물약 재료)</div>
<div class="fx"><b>FX:</b> 빛나는 꽃가루 구름 · 흩날리는 분홍 꽃잎</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/hydra.jpg')" target="_blank"><img src="/art/creatures/hydra.jpg" alt="늪 히드라" loading="lazy"></a>
<div class="body">
<span class="name">늪 히드라</span> <code>hydra</code><br>
전설의 한 마리 대신 그 혈통의 큰 늪 짐승 종. 원전의 독 · 머리가 다시 자람은 「후드 빛으로 겁주기」로 순하게
<div class="meta">크기 거대(몸길이 9 m) · 속성 물 · 풀 · 드묾<br>원전 창작 — 레르네의 히드라(그리스 신화 ☆)의 먼 혈통<br>기능 탈것(늪 건너기)<br>5서클 아쿠아 · 대형 탈것(늪 건너기) · 후드 막(가죽)</div>
<div class="fx"><b>FX:</b> 둘레 짙은 늪 안개 · 발밑 물결 고리</div>
</div>
</div>
</div>

</details>
