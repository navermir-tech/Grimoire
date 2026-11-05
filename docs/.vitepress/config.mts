import { defineConfig } from 'vitepress'

// 그리모리 문서 사이트 설정. 문서를 추가하면 아래 sidebar 에 한 줄 더한다.
export default defineConfig({
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
            { text: '마법', link: '/spec/magic' },
            { text: '생존', link: '/spec/survival' },
            { text: '마법 생물', link: '/spec/creatures' },
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
            { text: '생존', link: '/systems/survival' },
            { text: '환경 · 날씨', link: '/systems/environment' },
            { text: '마법 생물 · 생태계', link: '/systems/creatures' },
            { text: '건축 · 농사 · 생산', link: '/systems/building' },
            { text: '월드 · 멀티플레이 · 세이브', link: '/systems/world-multiplayer' },
            { text: '로드맵 · 리스크', link: '/systems/roadmap' },
            { text: 'M1 작업 분할', link: '/systems/m1-plan' },
          ],
        },
      ],
      '/art/': [
        {
          text: '아트 디자인',
          items: [
            { text: '마법사 의상 — 튜토리얼 NPC 시안', link: '/art/costumes-tutorial' },
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
            { text: '테스트 치트', link: '/dev/cheats' },
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
