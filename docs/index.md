---
layout: home

hero:
  name: 그리모리
  text: Grimoire
  tagline: 완드 하나로 세계와 맞닿는 마법세계 오픈월드 생존 게임 — 스펙 · 시스템 · 개발 문서
  actions:
    - theme: brand
      text: 스펙 문서
      link: /spec/
    - theme: alt
      text: 시스템 문서
      link: /systems/
    - theme: alt
      text: 개발 가이드
      link: /dev/setup

features:
  - icon: 🕯️
    title: 모든 상호작용은 마법으로
    details: 양초에 불을 붙이려면 불 마법을 쓴다. 바람은 불을 키우고 물은 끈다. 세계의 모든 사물은 속성 상태를 가진다.
  - icon: 📖
    title: 1서클에서 10서클까지
    details: 마법탑에서 시작해 스킬북으로 마법을 배우고, 서클을 올려 신급 마법사가 된다.
  - icon: 🐉
    title: 살아 있는 마법 생물 생태계
    details: 사냥하고, 먹고, 타고, 일을 시키고, 번식시킨다. 돌연변이는 색이 다르고 더 강하다.
  - icon: 🔥
    title: 생존과 거점
    details: 마나 · 체력 · 배고픔 · 피로 · 질병 · 체온. 집을 짓고 농사짓고 생산하는 거점.
  - icon: 🌩️
    title: 변하는 세계
    details: 밤과 낮, 맑음 · 흐림 · 안개 · 비 · 번개. 지역마다 다른 기후가 생존과 마법에 직접 닿는다.
  - icon: 🤝
    title: 친구와 함께, 사냥꾼에 맞서
    details: 로컬 서버를 띄워 협동. 오픈월드 + 파티별 인스턴스 던전. 금지된 방식으로 서클을 올리는 사냥꾼 세력과 싸운다.
---

## 이 사이트는

- **스펙 문서**는 "무엇을 만드는가" — 사용자가 정한 요구사항 R01~R16 을 영역별로 풀어 쓴 기획서다. 결정이 바뀌면 [결정 기록](/spec/decisions)에 남긴다.
- **시스템 문서**는 "어떻게 만드는가" — Unreal Engine 5.8 위에서 각 시스템을 어떤 클래스 · 데이터 · 네트워크 구조로 짜는지 정한 설계서다.
- **개발 가이드**는 저장소를 받아 빌드하고, 규칙을 지키고, 이 문서 사이트를 고치는 방법이다.

문서 원본은 저장소의 `docs/` 폴더에 있는 Markdown 이고, `main` 에 push 하면 GitHub Pages 가 자동으로 다시 배포한다. 로컬에서는 `npm run docs:dev` 로 저장 즉시 반영되는 미리보기를 띄울 수 있다 — [문서 사이트 · 배포](/dev/docs-site).
