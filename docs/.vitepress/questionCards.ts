import type { MarkdownRenderer } from 'vitepress'

// spec/decisions.md 「미결 질문」 표를 빌드할 때 카드 목록(QuestionBoard)으로 바꾼다.
// 원본 Markdown 은 파이프 표 그대로라 세션들이 줄을 더해도 충돌하지 않는다.
// 다른 표 · 다른 페이지는 건드리지 않는다. 표 모양이 바뀌면(열이 4개 미만) 그냥 표로 둔다.

const PagePath = 'spec/decisions.md'
const SectionTitle = '미결 질문'

type Status = 'open' | 'answered' | 'deferred' | 'moved'

// 결정 칸의 첫 글자가 상태다: ✅ 답변 · ⏸ 보류 · → 다른 번호로 넘김 · 나머지(빈칸 · ▢)는 미결
const StatusMark = /^(?:✅|▢|⏸)️?\s*/u

function statusOf(answer: string): Status {
  const text = answer.trim()
  if (text.startsWith('✅')) return 'answered'
  if (text.startsWith('⏸')) return 'deferred'
  if (text.startsWith('→')) return 'moved'
  return 'open'
}

const StatusLabel: Record<Status, string> = {
  open: '미결',
  answered: '답변됨',
  deferred: '보류',
  moved: '넘김',
}

// 필터 묶음: 보류는 아직 안 정한 것이라 미결 쪽, 넘김은 이 번호로는 끝난 것이라 답변 쪽
const groupOf = (status: Status) => (status === 'open' || status === 'deferred' ? 'open' : 'done')

export function questionCards(md: MarkdownRenderer) {
  const escape = md.utils.escapeHtml

  md.core.ruler.push('grim_question_cards', (state) => {
    if (state.env?.relativePath !== PagePath) return
    const tokens = state.tokens

    const headingAt = tokens.findIndex(
      (t, i) => t.type === 'heading_open' && t.tag === 'h2' && tokens[i + 1]?.content === SectionTitle,
    )
    if (headingAt < 0) return
    const openAt = tokens.findIndex((t, i) => i > headingAt && t.type === 'table_open')
    if (openAt < 0) return
    const closeAt = tokens.findIndex((t, i) => i > openAt && t.type === 'table_close')
    if (closeAt < 0) return

    type Cell = (typeof tokens)[number]
    const rows: Cell[][] = []
    for (const token of tokens.slice(openAt, closeAt)) {
      if (token.type === 'tr_open') rows.push([])
      else if (token.type === 'inline') rows[rows.length - 1].push(token)
    }
    const [head, ...body] = rows
    if (!head || head.length < 4) return

    const out: Cell[] = []
    const html = (content: string) => {
      const token = new state.Token('html_block', '', 0)
      token.content = content
      out.push(token)
    }
    // 빈 칸은 줄째 뺀다
    const field = (kind: string, label: Cell, value: Cell) => {
      if (!value.content.trim()) return
      html(`<div class="q-field q-field--${kind}"><dt>`)
      out.push(label)
      html('</dt><dd>')
      out.push(value)
      html('</dd></div>')
    }

    const statuses = body.map((row) => statusOf(row[3].content))
    const open = statuses.filter((s) => groupOf(s) === 'open').length
    const seen = new Set<string>()

    html(`<QuestionBoard :open="${open}" :done="${body.length - open}">\n`)
    body.forEach(([num, question, proposal, answer], i) => {
      const status = statuses[i]
      const number = num.content.trim()
      const id = /^Q\d+$/i.test(number) && !seen.has(number.toLowerCase()) ? number.toLowerCase() : ''
      seen.add(number.toLowerCase())

      // 상태는 뱃지로 보이니 결정 칸 앞의 표식은 뗀다
      answer.content = answer.content.replace(StatusMark, '')
      const first = answer.children?.[0]
      if (first?.type === 'text') first.content = first.content.replace(StatusMark, '')

      html(`<article class="q-card" data-status="${status}" data-group="${groupOf(status)}"><div class="q-card__head">`)
      // 사이트 검색은 머리 끝이 「#앵커」 링크인 줄을 절로 나눈다 — 이 h4 는 한 줄이어야 번호로 찾아 바로 간다.
      // 목차는 h2 · h3 까지라 h4 를 쓰면 목차에 안 올라온다.
      html(`<h4${id ? ` id="${id}" tabindex="-1"` : ''} class="q-card__title"><span class="q-card__num">${escape(number)}</span> <span class="q-card__question">`)
      out.push(question)
      html(`</span>${id ? ` <a class="header-anchor" href="#${id}" aria-label="${escape(number)} 링크"></a>` : ''}</h4>`)
      html(`<span class="q-card__badge">${StatusLabel[status]}</span></div><dl class="q-card__body">`)
      field('proposal', head[2], proposal)
      field('answer', head[3], answer)
      html('</dl></article>\n')
    })
    html('</QuestionBoard>\n')

    tokens.splice(openAt, closeAt - openAt + 1, ...out)
  })
}
