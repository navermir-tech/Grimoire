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

## 1. 마법탑 · 온난 지대 — 10종 {#region-1}

서클 1~3 · 주 속성 무속성 · 바람 · 풀 · 물. 오픈 베타의 땅 — **10종 모두 들어간다**(고르는 대상이 아니다). 재칼로프 · 켈피 · 검은 개는 이미 게임 데이터가 있다.

| 이름 | 한 줄 | 크기 | 원전 | 기능 | 희귀도 |
|---|---|---|---|---|---|
| **재칼로프** `jackalope` | M3 기존 종. 뿔은 가지가 적은 짧은 사슴뿔(바람 결 재질 — 색 변이마다 뿔 색이 다름, 재질 파라미터), 몸은 산토끼 비율 | 소형(몸길이 0.5 m) | 재칼로프(북미 민담) | 없음(동행 · 전투) | 흔함 |
| **켈피** `kelpie` | M3 기존 종. 사람으로 변하는 원전 설정은 뺌 — 갈기 · 꼬리가 젖은 물풀인 말 | 중형(어깨 1.6 m) | 켈피(스코틀랜드 민담) | 탈것(물 위 달리기) · 속성 작업(물 나르기 · 급수) | 보통 |
| **검은 개** `black-dog` | M3 기존 종. 무섭지 않게 — 보통 늑대보다 큰 검은 늑대, 불씨처럼 빛나는 호박색 눈. 그림자 안개는 FX | 중형(어깨 0.9 m) | 블랙 독(영국 민담) | 방범(밤 경비) | 보통 |
| **볼퍼팅어** `wolpertinger` | 여러 동물을 이어 붙인 원전을 다람쥐 몸 + 작은 뿔 + 활공막으로 정리 | 소형(몸길이 0.4 m) | 볼퍼팅어(독일 바이에른 민담) | 없음(동행 · 전투) | 흔함 |
| **흙닭 에르트헨** `erdhenne` | 집을 지키는 검은 닭 정령 → 땅에 둥지를 파는 큰 들닭. 위험이 오기 전에 우는 습성은 남김 | 소형(키 0.6 m) | 에르트헨(독일 알프 지역 정령) | 목축(알) | 흔함 |
| **바로메츠 양** `barometz` | 줄기에 열리는 양 → 등에 이끼 · 고사리 털이 자라는 양. 밤에는 발굽 둘레 뿌리털을 땅에 내리고 쉰다 | 중형(어깨 0.9 m) | 바로메츠(타타르의 식물 양 전설) | 목축(털 · 젖) | 흔함 |
| **위스프 나방** `wisp-moth` | 길을 잃게 하는 불빛 → 빛나는 나방 떼. 따라가면 숨은 상자 · 길이 나온다 | 소형(날개 폭 0.15 m) | 윌 오 더 위스프(영국 민담 도깨비불) | 없음(동행 · 전투) | 드묾(조건부) |
| **맨드레이크** `mandrake` | 사람 모양 뿌리 그대로 — 두 다리로 걷는 인삼 뿌리 사람(2026-11-19 사용자). 뽑히면 지르는 비명은 귀가 먹먹해지는 울음(잠깐 기절) | 소형(키 0.4 m) | 맨드레이크(유럽 전승) | 손재주(농사 — 밭갈이) | 보통(비 온 뒤 많이) |
| **불씨여우** `foxfire-fox` | 여우가 켜는 도깨비불 → 꼬리 끝에 숯불 같은 불씨를 단 들여우 | 소형(몸길이 0.6 m) | 여우불(일본 키츠네비) | 속성 작업(불 지피기) | 보통 |
| **구릉 멧돼지** `hill-boar` | 전설의 한 마리 대신 그 혈통의 큰 멧돼지 종. 우두머리(거대 개체)가 1번 지역 필드 보스 | 대형(어깨 1.5 m) | 창작 — 에리만토스의 멧돼지(그리스 신화 ☆)의 먼 혈통 | 탈것(돌진) | 드묾(보스) |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/jackalope.jpg')" target="_blank"><img src="/art/creatures/jackalope.jpg" alt="재칼로프" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
<a class="shot" :href="withBase('/art/creatures/wolpertinger-v1.jpg')" target="_blank"><img src="/art/creatures/wolpertinger-v1.jpg" alt="볼퍼팅어 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">볼퍼팅어</span> <code>wolpertinger</code><br>
여러 동물을 이어 붙인 원전을 다람쥐 몸 + 작은 뿔 + 활공막으로 정리
<div class="meta">크기 소형(몸길이 0.4 m) · 속성 바람 · 땅 · 흔함<br>원전 볼퍼팅어(독일 바이에른 민담)<br>기능 없음(동행 · 전투)<br>1서클 벤투스 · 야생 도토리 저장고(굴을 파면 씨앗 · 견과)</div>
<div class="fx"><b>FX:</b> 뒤로 남는 바람 자취 · 그 자취를 따라 맴도는 나뭇잎 · 도토리</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/erdhenne.jpg')" target="_blank"><img src="/art/creatures/erdhenne.jpg" alt="흙닭 에르트헨" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
| **키키모라 늪새** `kikimora-heron` | 여자 모습 정령 → 실을 잣는 습성만 남긴 키 큰 늪새. 반짝이는 물건을 훔쳐 둥지에 엮는다 | 중형(키 1.4 m) | 키키모라(슬라브 신화 집 · 늪 정령) | 손재주(실 잣기) | 보통 |
| **히시 그루터기거북** `hiisi-tortoise` | 숲 정령 → 등딱지가 오래된 그루터기인 큰 거북. 등 위에 버섯 · 이끼 정원 | 대형(등높이 1.3 m) | 히시(핀란드 숲의 정령) | 운반(무거운 짐) · 손재주(벌목) | 보통 |
| **루 카르콜 달팽이** `lou-carcolh` | 사람을 끌어가는 뱀 달팽이 → 순한 큰 늪 달팽이. 점액이 물약 재료 | 중형(껍데기 높이 0.8 m) | 루 카르콜(프랑스 남서부 전승) | 목축(점액) | 보통(비 오면 흔함) |
| **대롱여우** `kuda-fox` | 대롱 속 작은 여우 → 속 빈 갈대 · 대나무 속에 사는 족제비만 한 여우 | 소형(몸길이 0.3 m) | 쿠다키츠네(일본 — 대롱 속 여우) | 없음(동행 · 전투) | 흔함 |
| **아팡크** `afanc` | 수달 + 악어 → 수달 · 비버 머리에 악어 등판. 늪을 막아 댐을 짓는다 | 거대(몸길이 6 m) | 아팡크(웨일스 · 아서왕 전설) | 탈것(헤엄) | 드묾(보스) |
| **오초** `otso` | 숲의 왕 곰. 지역에서 가장 잡기 어려운 전설 — 길들일 수 있다(Q48) | 거대(어깨 3.5 m) | 오초(핀란드 칼레발라 — 숲의 왕 곰) ☆ | 없음(동행 · 전투) | 전설 |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/baku.jpg')" target="_blank"><img src="/art/creatures/baku.jpg" alt="꿈먹이 맥" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
<a class="shot" :href="withBase('/art/creatures/kikimora-heron-v1.jpg')" target="_blank"><img src="/art/creatures/kikimora-heron-v1.jpg" alt="키키모라 늪새 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">키키모라 늪새</span> <code>kikimora-heron</code><br>
여자 모습 정령 → 실을 잣는 습성만 남긴 키 큰 늪새. 반짝이는 물건을 훔쳐 둥지에 엮는다
<div class="meta">크기 중형(키 1.4 m) · 속성 물 · 풀 · 보통<br>원전 키키모라(슬라브 신화 집 · 늪 정령)<br>기능 손재주(실 잣기)<br>2서클 아쿠아 · 실 잣기 일꾼 · 둥지에서 잃어버린 물건</div>
<div class="fx"><b>FX:</b> 몸에서 실타래로 이어지는 빛나는 실 가닥</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/hiisi-tortoise.jpg')" target="_blank"><img src="/art/creatures/hiisi-tortoise.jpg" alt="히시 그루터기거북" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
| **세이렌 바다새** `siren-bird` | 사람 머리의 새 → 사람 요소를 모두 빼고 노래로 홀리는 큰 바다새 무리. 노래가 들리면 걸음이 끌려간다 | 중형(날개 폭 2.5 m) | 세이렌(그리스 신화) | 방범(노래로 침입자를 붙잡음) | 보통 |
| **페리톤** `peryton` | 사슴 머리 + 큰 새 몸. 그림자가 사람 모양이라는 원전 설정은 뺌 | 대형(어깨 1.6 m) | 페리톤(중세 전설 — 사슴 머리의 새) | 날것(활공) | 보통 |
| **번개매** `thunder-hawk` | 전설의 천둥새 대신 그 혈통의 큰 매 종 | 대형(날개 폭 5 m) | 창작 — 천둥새(북미 원주민 전승 ☆)의 아래 혈통 | 날것(비행) | 드묾 |
| **폭풍 해파리** `storm-jelly` | 폭풍 바람을 타고 하늘을 떠다니는 해파리. 뇌우 뒤 해안에 내려앉는다 | 중형(우산 지름 1 m) | 창작 | 없음(동행 · 전투) | 드묾(조건부) |
| **카르키노스 게** `karkinos` | 전설의 한 마리 → 같은 이름의 거대 게 종. 등에 따개비 · 바위 | 거대(폭 6 m) | 카르키노스(그리스 신화 ☆ 거대 게) | 손재주(채광 — 바위 깨기) | 드묾(보스) |
| **천둥새** `thunderbird` | 폭풍 해안의 전설. 날갯짓이 천둥, 눈빛이 번개. 원전의 신성함을 존중해 묘사는 정중하게 — 잡기 · 교배는 다른 전설과 같다(Q162) | 거대(날개 폭 20 m) | 천둥새(북미 원주민 전승) ☆ | 없음(동행 · 전투) | 전설 |

<div class="creature-grid">
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/raiju.jpg')" target="_blank"><img src="/art/creatures/raiju.jpg" alt="라이쥬 번개족제비" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<a class="shot" :href="withBase('/art/creatures/siren-bird.jpg')" target="_blank"><img src="/art/creatures/siren-bird.jpg" alt="세이렌 바다새" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
<a class="shot" :href="withBase('/art/creatures/siren-bird-v1.jpg')" target="_blank"><img src="/art/creatures/siren-bird-v1.jpg" alt="세이렌 바다새 옛 판" loading="lazy"></a>
</details>
<div class="body">
<span class="name">세이렌 바다새</span> <code>siren-bird</code><br>
사람 머리의 새 → 사람 요소를 모두 빼고 노래로 홀리는 큰 바다새 무리. 노래가 들리면 걸음이 끌려간다
<div class="meta">크기 중형(날개 폭 2.5 m) · 속성 바람 · 물 · 보통<br>원전 세이렌(그리스 신화)<br>기능 방범(노래로 침입자를 붙잡음)<br>4서클 벤투스 · 거점 경비(노래로 침입자를 붙잡음) · 깃털</div>
<div class="fx"><b>FX:</b> 빛나는 음파 리본</div>
</div>
</div>
<div class="creature-card">
<a class="shot" :href="withBase('/art/creatures/peryton.jpg')" target="_blank"><img src="/art/creatures/peryton.jpg" alt="페리톤" loading="lazy"></a>
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
<details class="old"><summary>옛 판(v1, 2026-11-18) 보기</summary>
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
