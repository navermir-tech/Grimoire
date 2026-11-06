import { defineConfig } from 'vitepress'

// 그리모리 문서 사이트 설정. 문서를 추가하면 아래 sidebar 에 한 줄 더한다.
export default defineConfig({
  // GitHub Pages 프로젝트 사이트(https://navermir-tech.github.io/Grimoire/)라 경로 앞에 저장소 이름이 붙는다.
  base: '/Grimoire/',
  lang: 'ko-KR',
  title: '그리모리 Grimoire',
  description: '마법세계 오픈월드 생존 게임 「그리모리」의 스펙 · 시스템 · 개발 문서',
  cleanUrls: true,
  lastUpdated: true,

  themeConfig: {
    siteTitle: '그리모리 문서',

    nav: [
      { text: '스펙', link: '/spec/', activeMatch: '/spec/' },
      { text: '시스템', link: '/systems/', activeMatch: '/systems/' },
      { text: '아트', link: '/art/costumes-tutorial', activeMatch: '/art/' },
      { text: '개발', link: '/dev/setup', activeMatch: '/dev/' },
    ],

    sidebar: {
      '/spec/': [
        {
          text: '스펙 문서 (무엇을)',
          items: [
            { text: '개요 · 요구사항 R01~R16', link: '/spec/' },
            { text: '세계 · 환경', link: '/spec/world' },
            { text: '아트 방향', link: '/spec/art-direction' },
            { text: '마법', link: '/spec/magic' },
            { text: '스킬북 목록', link: '/spec/skillbooks' },
            { text: '성장 — 레벨 · 성장 트리', link: '/spec/progression' },
            { text: '장비 · 내구도 · 인챈트', link: '/spec/equipment' },
            { text: '인챈트 목록 · 밸런스', link: '/spec/enchants' },
            { text: '캐릭터 커스터마이징', link: '/spec/customization' },
            { text: '생존', link: '/spec/survival' },
            { text: '마법 생물', link: '/spec/creatures' },
            { text: '지역별 생물 명단', link: '/spec/creature-roster' },
            { text: '하우징 · 거점', link: '/spec/housing' },
            { text: '세력 · 서사', link: '/spec/factions' },
            { text: '멀티플레이', link: '/spec/multiplayer' },
            { text: '결정 기록 · 미결 질문', link: '/spec/decisions' },
            { text: '레퍼런스 조사', link: '/spec/references' },
          ],
        },
      ],
      '/systems/': [
        {
          text: '시스템 문서 (어떻게)',
          items: [
            { text: '아키텍처 개요', link: '/systems/' },
            { text: '마법 · 상호작용', link: '/systems/magic' },
            { text: '투사체 · 시전 감각', link: '/systems/spell-feel' },
            { text: '마법 이펙트 자체 제작', link: '/systems/magic-fx-authoring' },
            { text: '생존', link: '/systems/survival' },
            { text: '환경 · 날씨', link: '/systems/environment' },
            { text: '마법 생물 · 생태계', link: '/systems/creatures' },
            { text: '건축 · 농사 · 생산', link: '/systems/building' },
            { text: '월드 · 멀티플레이 · 세이브', link: '/systems/world-multiplayer' },
            { text: '로드맵 · 리스크', link: '/systems/roadmap' },
            { text: 'M1 작업 분할', link: '/systems/m1-plan' },
            { text: 'M1.5 작업 분할', link: '/systems/m15-plan' },
            { text: 'M2 작업 분할', link: '/systems/m2-plan' },
            { text: 'M2 후반 작업 분할', link: '/systems/m2b-plan' },
            { text: 'M3 작업 분할', link: '/systems/m3-plan' },
            { text: 'M4 작업 계획 — 거점', link: '/systems/m4-plan' },
            { text: '레벨 · 성장 트리', link: '/systems/progression' },
            { text: '커스터마이징 — 외형 조립', link: '/systems/customization' },
            { text: '마법탑 튜토리얼 · 퀘스트', link: '/systems/tutorial' },
            { text: '지형 1차 (오픈 베타 땅)', link: '/systems/terrain' },
            { text: '성능 예산 · 최적화 규칙', link: '/systems/performance' },
          ],
        },
      ],
      '/art/': [
        {
          text: '아트 디자인',
          items: [
            { text: '마법사 의상 — 튜토리얼 NPC 시안', link: '/art/costumes-tutorial' },
            { text: '1번 지역 콘셉트 아트', link: '/art/region1-concepts' },
            { text: '1번 지역 — 필요한 3D 모델', link: '/art/region1-assets' },
          ],
        },
      ],
      '/dev/': [
        {
          text: '개발 가이드',
          items: [
            { text: '시작하기', link: '/dev/setup' },
            { text: '규칙 · 명명', link: '/dev/conventions' },
            { text: '문서 사이트 · 배포', link: '/dev/docs-site' },
            { text: '성능 측정 방법', link: '/dev/perf-measure' },
            { text: '테스트 치트', link: '/dev/cheats' },
            { text: '레벨 디자인 노트', link: '/dev/level-design' },
          ],
        },
      ],
    },

    search: {
      provider: 'local',
      options: {
        translations: {
          button: { buttonText: '검색', buttonAriaLabel: '문서 검색' },
          modal: {
            noResultsText: '결과가 없습니다',
            resetButtonTitle: '지우기',
            displayDetails: '자세히',
            footer: { selectText: '선택', navigateText: '이동', closeText: '닫기' },
          },
        },
      },
    },

    editLink: {
      pattern: 'https://github.com/navermir-tech/Grimoire/edit/main/docs/:path',
      text: 'GitHub 에서 이 문서 고치기',
    },
    lastUpdated: { text: '마지막 수정' },
    outline: { level: [2, 3], label: '이 문서의 목차' },
    docFooter: { prev: '이전 문서', next: '다음 문서' },
    sidebarMenuLabel: '메뉴',
    returnToTopLabel: '맨 위로',
    darkModeSwitchLabel: '테마',
    lightModeSwitchTitle: '밝은 테마로',
    darkModeSwitchTitle: '어두운 테마로',
    socialLinks: [{ icon: 'github', link: 'https://github.com/navermir-tech/Grimoire' }],
    footer: {
      message: '그리모리 Grimoire — 스펙 · 시스템 · 개발 문서',
      copyright: '© 2026 navermir',
    },
  },
})
