import DefaultTheme from 'vitepress/theme'
import type { Theme } from 'vitepress'
import QuestionBoard from './QuestionBoard.vue'

// 기본 테마 그대로에 「미결 질문」 카드 목록만 더한다(docs/dev/docs-site.md).
export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('QuestionBoard', QuestionBoard)
  },
} satisfies Theme
