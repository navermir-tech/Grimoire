// 크레딧 생성 · 검사(Q137) — 정본은 Content/Grimoire/Data/Credits.json (게임 안 「설정 → 크레딧 확인」 탭이 읽는 파일)
//
//   node Scripts/Docs/credits.mjs                 # 검사: docs/art/credits.md 의 생성 구역이 데이터와 같은가(다르면 종료 코드 1)
//   node Scripts/Docs/credits.mjs --write         # docs/art/credits.md 의 생성 구역을 다시 쓴다
//   node Scripts/Docs/credits.mjs --txt <경로>    # 출시본 폴더용 CREDITS.txt 를 쓴다(UTF-8 BOM · CRLF — 메모장에서 바로 읽힌다)
//
// npm run docs:check 도 이 검사를 돈다. 생성 구역 = <!-- credits:begin --> ~ <!-- credits:end --> 사이(손으로 고치지 않는다).

import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const Root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..')
export const DataPath = path.join(Root, 'Content', 'Grimoire', 'Data', 'Credits.json')
export const DocPath = path.join(Root, 'docs', 'art', 'credits.md')
const Begin = '<!-- credits:begin — 생성물: Content/Grimoire/Data/Credits.json 을 고치고 node Scripts/Docs/credits.mjs --write -->'
const End = '<!-- credits:end -->'

export function loadCredits() {
  const data = JSON.parse(fs.readFileSync(DataPath, 'utf8'))
  const problems = []
  for (const [i, a] of (data.Attributions ?? []).entries()) {
    for (const key of ['Author', 'Asset', 'AssetUrl', 'License', 'LicenseUrl', 'ModifiedKo', 'ModifiedEn']) {
      if (typeof a[key] !== 'string' || a[key].trim() === '') problems.push(`Attributions[${i}].${key} 가 비었다`)
    }
  }
  return { data, problems }
}

/** CC BY 표기 한 줄(영어) — 게임 안 탭 · CREDITS.txt 와 같은 모양(GrimCredits.cpp 의 MakeAttributionLine) */
export function attributionEn(a) {
  return `${a.Asset} by ${a.Author} (${a.AssetUrl}), licensed under ${a.License} (${a.LicenseUrl}). ${a.ModifiedEn}`.trim()
}

export function attributionKo(a) {
  return `${a.Author} 의 ${a.Asset} — ${a.License}. ${a.ModifiedKo}`.trim()
}

const cell = (s) => String(s ?? '').replace(/\|/g, '\\|').replace(/\r?\n/g, ' ')

export function renderDocBlock(data) {
  const out = [Begin, '', '## 쓰고 있는 CC BY 에셋', '',
    '| 저작자 | 에셋 | 라이선스 | 어디에 쓰나 | 고친 점 | 확인 |', '|---|---|---|---|---|---|']
  for (const a of data.Attributions ?? []) {
    const store = a.Store ? ` (${a.Store})` : ''
    out.push(`| ${cell(a.Author)} | [${cell(a.Asset)}](${a.AssetUrl})${store} | [${cell(a.License)}](${a.LicenseUrl}) | ${cell(a.DocUsedFor)} | ${cell(a.DocChanges)} | ${cell(a.Checked)} |`)
  }
  out.push('', '## 표기 문구', '', '게임 안 크레딧 탭 · 출시본 `CREDITS.txt` 에 그대로 들어가는 문구:', '')
  const quotes = []
  for (const a of data.Attributions ?? []) quotes.push(`> ${attributionEn(a)}\n>\n> ${attributionKo(a)}`)
  out.push(quotes.join('\n\n'))
  if ((data.Notices ?? []).length) {
    out.push('', '## 엔진 표기', '', '언리얼 엔진 EULA 가 크레딧에 넣으라는 문구(게임 안 탭 · `CREDITS.txt` 끝에 들어간다):', '')
    for (const n of data.Notices) out.push(`- ${n}`)
  }
  out.push('', End)
  return out.join('\n')
}

export function renderTxt(data) {
  const lines = []
  const title = `${data.Game ?? 'Grimoire'} — 크레딧 / Credits`
  lines.push(title, '='.repeat(40), '')
  if (data.IntroKo) lines.push(data.IntroKo)
  if (data.IntroEn) lines.push(data.IntroEn)
  lines.push('')
  for (const a of data.Attributions ?? []) {
    lines.push(`* ${a.Asset} — ${a.Author}${a.Store ? ` (${a.Store})` : ''}`)
    lines.push(`  ${a.AssetUrl}`)
    lines.push(`  ${a.License} — ${a.LicenseUrl}`)
    lines.push(`  ${attributionEn(a)}`)
    lines.push(`  ${attributionKo(a)}`)
    lines.push('')
  }
  for (const n of data.Notices ?? []) lines.push(n)
  return '﻿' + lines.join('\r\n').replace(/\s+$/, '') + '\r\n'
}

/** 문서의 생성 구역을 바꾼 결과(구역 표시가 없으면 null) */
export function replaceDocBlock(doc, block) {
  const b = doc.indexOf('<!-- credits:begin')
  const e = doc.indexOf(End)
  if (b < 0 || e < b) return null
  return doc.slice(0, b) + block + doc.slice(e + End.length)
}

/** 검사 — 문제 목록(비면 통과) */
export function checkCredits() {
  const { data, problems } = loadCredits()
  const doc = fs.readFileSync(DocPath, 'utf8').replace(/\r\n/g, '\n')
  const want = replaceDocBlock(doc, renderDocBlock(data))
  if (want === null) problems.push('docs/art/credits.md 에 <!-- credits:begin --> · <!-- credits:end --> 표시가 없다')
  else if (want !== doc) problems.push('docs/art/credits.md 의 크레딧 표가 Content/Grimoire/Data/Credits.json 과 다르다 — node Scripts/Docs/credits.mjs --write')
  return problems
}

function main(argv) {
  const txtIndex = argv.indexOf('--txt')
  if (txtIndex >= 0) {
    const target = argv[txtIndex + 1]
    if (!target) {
      console.error('--txt 뒤에 쓸 경로를 준다. 예: node Scripts/Docs/credits.mjs --txt Saved/StagedBuilds/Windows/CREDITS.txt')
      return 2
    }
    const { data, problems } = loadCredits()
    if (problems.length) {
      problems.forEach((p) => console.error(`오류: ${p}`))
      return 1
    }
    fs.mkdirSync(path.dirname(path.resolve(target)), { recursive: true })
    fs.writeFileSync(path.resolve(target), renderTxt(data), 'utf8')
    console.log(`${target} 를 썼다 — 표기 ${data.Attributions.length}`)
    return 0
  }
  if (argv.includes('--write')) {
    const { data, problems } = loadCredits()
    if (problems.length) {
      problems.forEach((p) => console.error(`오류: ${p}`))
      return 1
    }
    const doc = fs.readFileSync(DocPath, 'utf8').replace(/\r\n/g, '\n')
    const next = replaceDocBlock(doc, renderDocBlock(data))
    if (next === null) {
      console.error('docs/art/credits.md 에 <!-- credits:begin --> · <!-- credits:end --> 표시를 먼저 넣는다')
      return 1
    }
    fs.writeFileSync(DocPath, next, 'utf8')
    console.log('docs/art/credits.md 의 크레딧 표를 다시 썼다')
    return 0
  }
  const problems = checkCredits()
  problems.forEach((p) => console.error(`오류: ${p}`))
  if (!problems.length) console.log('크레딧: 데이터와 문서가 같다')
  return problems.length ? 1 : 0
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  process.exit(main(process.argv.slice(2)))
}
