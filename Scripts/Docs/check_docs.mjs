// 문서가 코드와 어긋났는지 가볍게 검사한다. `npm run docs:check`
//
// docs/**/*.md 의 인라인 코드(`...`)에서 세 가지를 본다.
//   1. 경로   — `Source/...` `Config/...` `Scripts/...` `Content/...` `art/...` 가 실제로 있는가(오류)
//   2. 콘솔   — `Grim.X.Y` 와 `GrimSuperCheat 1` 같은 명령이 Source 에 등록돼 있는가(오류)
//   3. 식별자 — `UGrimFoo` · `GrimFoo` 가 Source 어딘가에 있는가(경고, --strict 면 오류)
// 줄에 「계획」「미구현」「설계안」「예정」「나중」「옛 이름」이 있으면 그 줄은 건너뛴다(아직 없는 것을 적는 줄).
// 조각 하나만 빼려면 바로 뒤에 `(안 생김)` · `(아직 없음)` 을 붙인다: `Content/Grimoire/Survival/`(안 생김)
// docs/spec/decisions.md 는 결정 기록(옛 이름이 남는다)이라 보지 않는다.
//
// 옵션: --strict (식별자 경고도 실패로), --quiet (요약만)

import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const Root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..')
const Args = new Set(process.argv.slice(2))
const Strict = Args.has('--strict')
const Quiet = Args.has('--quiet')

const SkipDocs = new Set(['docs/spec/decisions.md'])
const PlanWords = /계획|미구현|설계안|예정|나중|옛 이름/
const PathRoots = /^(Source|Config|Scripts|Content|art)\//
// .gitignore 의 art/**/raw/ · logs/ 는 그 PC 에만 있다
const IgnoredPaths = /^art\/.*\/(raw|logs)\//

function walk(dir, filter, out = []) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    if (entry.name === 'node_modules' || entry.name.startsWith('.')) continue
    const full = path.join(dir, entry.name)
    if (entry.isDirectory()) walk(full, filter, out)
    else if (filter(entry.name)) out.push(full)
  }
  return out
}

const rel = (p) => path.relative(Root, p).split(path.sep).join('/')

// ── Source 에서 콘솔 명령 · 식별자 모으기 ──
const consoleNames = new Set()   // 소문자
const identifiers = new Set()
for (const file of walk(path.join(Root, 'Source'), (n) => /\.(h|cpp|inl)$/.test(n))) {
  const text = fs.readFileSync(file, 'utf8')
  for (const m of text.matchAll(/TEXT\("((?:Grim|grim)\.[A-Za-z0-9_.]+)"\)/g)) consoleNames.add(m[1].toLowerCase())
  for (const m of text.matchAll(/UFUNCTION\(\s*Exec[^)]*\)\s*(?:virtual\s+)?[\w:<>*&\s]+?\b(\w+)\s*\(/g)) consoleNames.add(m[1].toLowerCase())
  for (const m of text.matchAll(/\b[UAFEIT]?(Grim\w+)/g)) identifiers.add(m[1])
}
// 커맨드렛 스크립트 · 설정에서만 쓰는 이름도 있다(태그 네임스페이스 등)
for (const file of walk(path.join(Root, 'Config'), (n) => n.endsWith('.ini'))) {
  for (const m of fs.readFileSync(file, 'utf8').matchAll(/\b[UAFEIT]?(Grim\w+)/g)) identifiers.add(m[1])
}

function pathExists(p) {
  const clean = p.replace(/[),.;:]+$/, '').replace(/\/$/, '')
  const wild = clean.search(/[*?{<]/)
  const base = wild >= 0 ? clean.slice(0, wild).replace(/\/[^/]*$/, '') : clean
  const full = path.join(Root, base)
  if (fs.existsSync(full)) return true
  // 에셋은 확장자 없이 적는다: Content/Grimoire/Maps/L_Tower
  return fs.existsSync(full + '.uasset') || fs.existsSync(full + '.umap')
}

// `Grim.Perf.Goto/Sample/Net` → 셋, `Grim.Terrain.*` → 앞부분이 맞는 것이 하나라도
function consoleExists(token) {
  const name = token.replace(/=.*$/, '').replace(/[),;:]+$/, '')
  if (name.includes('/')) {
    const [first, ...rest] = name.split('/')
    const prefix = first.slice(0, first.lastIndexOf('.') + 1)
    return [first, ...rest.map((r) => prefix + r)].every(consoleExists)
  }
  const lower = name.toLowerCase()
  if (lower.endsWith('*') || lower.endsWith('.')) {
    const prefix = lower.slice(0, -1)
    return [...consoleNames].some((n) => n.startsWith(prefix))
  }
  return consoleNames.has(lower)
}

const errors = []
const warnings = []
let spanCount = 0

for (const file of walk(path.join(Root, 'docs'), (n) => n.endsWith('.md'))) {
  const doc = rel(file)
  if (SkipDocs.has(doc)) continue
  const lines = fs.readFileSync(file, 'utf8').split(/\r?\n/)
  let fenced = false
  lines.forEach((line, i) => {
    if (/^\s*(```|~~~)/.test(line)) { fenced = !fenced; return }
    if (fenced || PlanWords.test(line)) return
    for (const m of line.matchAll(/`([^`\n]+)`/g)) {
      spanCount++
      const span = m[1].trim()
      const where = `${doc}:${i + 1}`
      const first = span.split(/\s+/)[0]
      // 바로 뒤에 (안 생김) · (아직 없음) 을 붙인 조각은 일부러 없는 것이다
      if (/^\((안 생김|아직 없)/.test(line.slice(m.index + m[0].length))) continue

      if (PathRoots.test(first)) {
        if (!/[<]/.test(first) && !IgnoredPaths.test(first) && !pathExists(first)) errors.push(`${where}  경로 없음  ${first}`)
        continue
      }
      if (/^grim\.[A-Za-z]/i.test(first)) {
        if (!consoleExists(first)) errors.push(`${where}  콘솔 명령 없음  ${first}`)
        continue
      }
      // `GrimSuperCheat 1` — 인자가 붙은 Grim 단어는 Exec 명령으로 본다
      if (/^Grim[A-Z]\w*$/.test(first) && span.includes(' ') && !/[(:<]/.test(first)) {
        if (!consoleExists(first) && !identifiers.has(first)) errors.push(`${where}  콘솔 명령 없음  ${first}`)
        continue
      }
      const id = first.match(/^[UAFEIT]?(Grim[A-Z]\w*)(?:::\w+)?(?:\(\))?$/)
      if (id && !identifiers.has(id[1])) warnings.push(`${where}  식별자 없음  ${first}`)
    }
  })
}

if (!Quiet) {
  for (const e of errors) console.log(`오류  ${e}`)
  for (const w of warnings) console.log(`경고  ${w}`)
}
console.log(`docs:check — 코드 조각 ${spanCount}개, 오류 ${errors.length}, 경고 ${warnings.length}` +
  ` (콘솔 명령 ${consoleNames.size} · 식별자 ${identifiers.size} 개를 Source 에서 읽음)`)
if (errors.length || (Strict && warnings.length)) process.exit(1)
