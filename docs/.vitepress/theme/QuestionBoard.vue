<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref } from 'vue'
import { useRouter } from 'vitepress'

// 카드 자체는 빌드할 때 만들어져 슬롯으로 들어온다(../questionCards.ts). 여기는 필터만 한다.
type Filter = 'open' | 'done' | 'all'

const props = defineProps<{ open: number; done: number }>()

const filter = ref<Filter>('open')
const root = ref<HTMLElement>()

const tabs = computed<{ key: Filter; label: string; count: number }[]>(() => [
  { key: 'open', label: '미결', count: props.open },
  { key: 'done', label: '답변됨', count: props.done },
  { key: 'all', label: '전체', count: props.open + props.done },
])

// 검색 · 링크로 #q12 처럼 지금 필터에 가려진 카드로 오면 전체를 펴고 그 카드로 간다
function revealHash() {
  const id = decodeURIComponent(location.hash.slice(1))
  if (!id || !root.value) return
  const target = root.value.querySelector<HTMLElement>(`[id="${CSS.escape(id)}"]`)
  const group = target?.closest<HTMLElement>('.q-card')?.dataset.group
  if (!target || !group || filter.value === 'all' || filter.value === group) return
  filter.value = 'all'
  nextTick(() => target.scrollIntoView())
}

// 링크 클릭은 hashchange 로 오지만, 검색창 Enter(router.go)는 라우터 후크로만 알 수 있다
const router = useRouter()
const previousAfterRouteChange = router.onAfterRouteChange

onMounted(() => {
  revealHash()
  window.addEventListener('hashchange', revealHash)
  router.onAfterRouteChange = async (to) => {
    await previousAfterRouteChange?.(to)
    revealHash()
  }
})
onBeforeUnmount(() => {
  window.removeEventListener('hashchange', revealHash)
  router.onAfterRouteChange = previousAfterRouteChange
})
</script>

<template>
  <div ref="root" class="q-board" :data-filter="filter">
    <div class="q-board__bar" role="group" aria-label="질문 상태로 거르기">
      <button
        v-for="tab in tabs"
        :key="tab.key"
        type="button"
        class="q-board__tab"
        :aria-pressed="filter === tab.key"
        @click="filter = tab.key"
      >
        {{ tab.label }}<span class="q-board__count">{{ tab.count }}</span>
      </button>
    </div>
    <slot />
  </div>
</template>

<style>
.q-board {
  --q-sticky-top: 49px; /* 좁은 화면: 「메뉴 · 목차」 줄 아래 */
}

@media (min-width: 960px) {
  .q-board {
    --q-sticky-top: calc(var(--vp-nav-height) + 49px);
  }
}

@media (min-width: 1280px) {
  .q-board {
    --q-sticky-top: var(--vp-nav-height);
  }
}

.q-board__bar {
  position: sticky;
  top: var(--q-sticky-top);
  z-index: 10;
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  padding: 10px 0;
  background-color: var(--vp-c-bg);
}

.q-board__tab {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 12px;
  border: 1px solid var(--vp-c-divider);
  border-radius: 999px;
  font-size: 13px;
  font-weight: 500;
  line-height: 20px;
  color: var(--vp-c-text-2);
  transition: color 0.2s, border-color 0.2s, background-color 0.2s;
}

.q-board__tab:hover {
  color: var(--vp-c-text-1);
  border-color: var(--vp-c-text-3);
}

.q-board__tab[aria-pressed='true'] {
  color: var(--vp-c-brand-1);
  border-color: var(--vp-c-brand-1);
  background-color: var(--vp-c-brand-soft);
}

.q-board__count {
  font-variant-numeric: tabular-nums;
  opacity: 0.75;
}

.q-board[data-filter='open'] .q-card[data-group='done'],
.q-board[data-filter='done'] .q-card[data-group='open'] {
  display: none;
}

.q-card {
  --q-accent: var(--vp-c-warning-1);
  --q-accent-soft: var(--vp-c-warning-soft);
  margin: 10px 0;
  padding: 14px 16px;
  border: 1px solid var(--vp-c-divider);
  border-left: 3px solid var(--q-accent);
  border-radius: 8px;
  background-color: var(--vp-c-bg-soft);
}

.q-card[data-status='answered'] {
  --q-accent: var(--vp-c-success-1);
  --q-accent-soft: var(--vp-c-success-soft);
}

.q-card[data-status='deferred'],
.q-card[data-status='moved'] {
  --q-accent: var(--vp-c-text-3);
  --q-accent-soft: var(--vp-c-default-soft);
}

.q-card__head {
  display: flex;
  align-items: flex-start;
  gap: 12px;
}

.vp-doc .q-card__title {
  position: relative;
  display: grid;
  flex: 1;
  grid-template-columns: 3.4em minmax(0, 1fr);
  min-width: 0;
  margin: 0;
  font-size: 15px;
  font-weight: 600;
  line-height: 24px;
  letter-spacing: normal;
  scroll-margin-top: calc(var(--q-sticky-top) + 60px);
}

.q-card__num {
  color: var(--q-accent);
  font-variant-numeric: tabular-nums;
}

.q-card[data-status='deferred'] .q-card__num,
.q-card[data-status='moved'] .q-card__num {
  color: var(--vp-c-text-2);
}

.q-card__question {
  overflow-wrap: anywhere;
}

/* 앵커는 번호 위에 투명하게 깐다 — 번호를 누르면 그 카드 링크가 된다 */
.vp-doc .q-card__title .header-anchor {
  top: 0;
  left: 0;
  width: 3em;
  height: 24px;
  margin-left: 0;
  opacity: 1;
}

.vp-doc .q-card__title .header-anchor::before {
  content: none;
}

.q-card__title:has(.header-anchor:hover) .q-card__num {
  text-decoration: underline;
}

.q-card__badge {
  flex: none;
  padding: 0 8px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 500;
  line-height: 22px;
  color: var(--q-accent);
  background-color: var(--q-accent-soft);
}

.q-card[data-status='deferred'] .q-card__badge,
.q-card[data-status='moved'] .q-card__badge {
  color: var(--vp-c-text-2);
}

.vp-doc .q-card__body {
  margin: 0;
}

.vp-doc .q-card__body:empty {
  display: none;
}

.q-field {
  display: grid;
  grid-template-columns: 3.4em minmax(0, 1fr);
  margin-top: 8px;
  font-size: 14px;
  line-height: 24px;
}

.q-field dt {
  font-size: 13px;
  color: var(--vp-c-text-3);
}

.q-field dd {
  margin: 0;
  min-width: 0;
  overflow-wrap: anywhere;
}

.q-field--answer dt {
  color: var(--q-accent);
  font-weight: 500;
}

/* 답이 난 카드는 결정이 먼저 읽히게 후보를 눌러 둔다 */
.q-card[data-group='done'] .q-field--proposal dd {
  color: var(--vp-c-text-2);
}

/* 폰: 번호 칸 · 머리말 칸을 없애 글이 카드 폭을 다 쓰게 한다. 번호와 뱃지가 첫 줄 양끝 */
@media (max-width: 479px) {
  .q-card {
    position: relative;
  }

  .q-card__head {
    display: block;
  }

  .vp-doc .q-card__title,
  .q-field {
    grid-template-columns: minmax(0, 1fr);
  }

  .q-card__badge {
    position: absolute;
    top: 14px;
    right: 16px;
  }
}
</style>
