# QA — 이펙트 · 화면

[QA 절차](../dev/qa) · 설계 [투사체 · 시전 감각 — 볼트 이펙트](../systems/spell-feel) · [마법 이펙트 자체 제작](../systems/magic-fx-authoring). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\MagicFX\capture_bolt.ps1 -Spell Ember`(옆 카메라 · 느린 시간, `Saved\BoltCapture\Ember_side\shot_###.png`) · `Scripts\QA\shoot_qa.ps1`.

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-FX-001 | Q20 | 연출 에셋(VisualSet · Niagara · 큐)이 다 있고 이어져 있다 | ① `Grimoire.MagicFX.Assets` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-FX-002 | — | 물줄기 그림이 판정과 같은 호를 따른다 | ① `Grimoire.MagicFX.Stream.Shape` | 통과 | ✅ 11-06 | 같음 |
| QA-FX-003 | R21 | 책 · 문양 · 시전 동작(자세 고르기) | ① `Grimoire.MagicFX.Book.*` · `.Sigil.*` · `.CastMotion.*` | 통과 | ✅ 11-06 | 같음 |
| QA-FX-004 | Q85~Q87 | 불씨 볼트: 흰 심 → 속성색 몸통 · 불티 · 터짐 고리가 참고 그림과 닮았다 | ③ `capture_bolt.ps1 -Spell Ember`(연출만 `Grim.FX.Bolt`, slomo 0.02) | 비행 중 빛줄기 · 불티, 표적에 터짐 고리 | ✅ 11-06 — 흰 심 → 주황 빛줄기 · 완드 앞 불티, 표적 구에 불꽃 고리 · 방사 빛살. 「참고 그림과 얼마나 닮았나」는 사람 판단(⏸ — [Q87](../spec/decisions#미결-질문)) | `qa-fx-ember-bolt.jpg` · `qa-fx-ember-impact.jpg`(60장 중 4 · 16번) |
| QA-FX-005 | Q85 | 가까운 표적(7.5 m 안)에 실제 시전하면 날아가는 빛줄기 없이 바로 터진다 | ③ `capture_bolt.ps1 -Spell Ember -Cast` | — | ⏸ 이미 미결 [Q85](../spec/decisions#미결-질문)(사용자 결정 기다림) — 결정 뒤 확인 | — |
| QA-FX-006 | — | 모든 Niagara 시스템에 Effect Type(`EFT_Grim_Spell` · `_Weather` · `_Ambient`) — [성능 규칙 24](../systems/performance#렌더-·-연출) | 저장소 검색(`EFT_Grim`) | 에셋 · 생성 스크립트에 있음 | ❌ [BUG-006](./bugs) | `Scripts/MagicFX/*` · `Content/` 어디에도 `EFT_Grim` 없음 |
| QA-FX-007 | R04 | 날씨 연출(비) | ③ 장면 `qa-rain` | 빗줄기 | ❌ [BUG-009](./bugs) | `qa-rain.jpg` |
| QA-FX-008 | — | 물줄기 · 미풍 새 연출 | — | — | ⏸ 아직 시안만 있음(`docs/public/art/fx/*-concept.jpg`) — [Q86](../spec/decisions#미결-질문) | — |

## 그림

![QA-FX-004 — 불씨 볼트 비행(옆 카메라 · 느린 시간 1/50)](/qa/qa-fx-ember-bolt.jpg)

![QA-FX-004 — 불씨 볼트 터짐](/qa/qa-fx-ember-impact.jpg)
