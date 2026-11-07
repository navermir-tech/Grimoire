# QA — 이펙트 · 화면

[QA 절차](../dev/qa) · 설계 [투사체 · 시전 감각 — 볼트 이펙트](../systems/spell-feel) · [마법 이펙트 자체 제작](../systems/magic-fx-authoring). 마지막 실행 **2026-11-07**(브랜치 `claude/fx-rework-1a2d5c` — Q85 · Q86).

다시 돌리기: `powershell -File Scripts\MagicFX\capture_bolt.ps1 -Spell Ember`(옆 카메라 · 느린 시간, `Saved\BoltCapture\Ember_side\shot_###.png`) · `Scripts\QA\shoot_qa.ps1`. 실제 시전은 `-Cast`(물줄기 `-Hold 10`), 연출만 옆 · 위에서 `-FxCmd "Grim.FX.Spawn …"`, 어깨 뒤 게임 시점은 `Scripts\MagicFX\capture_spell.ps1`(L_OpenBeta, 재칼로프 표적).

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-FX-001 | Q20 · Q85 | 연출 에셋(VisualSet · Niagara · 큐)이 다 있고 이어져 있다, 볼트 묶음에 발사 빛줄기 | ① `Grimoire.MagicFX.Assets` | 통과 | ✅ 11-07 | 자동화 169/169 |
| QA-FX-002 | — | 물줄기 그림이 판정과 같은 호를 따른다 | ① `Grimoire.MagicFX.Stream.Shape` | 통과 | ✅ 11-06 | 같음 |
| QA-FX-003 | R21 | 책 · 문양 · 시전 동작(자세 고르기) | ① `Grimoire.MagicFX.Book.*` · `.Sigil.*` · `.CastMotion.*` | 통과 | ✅ 11-06 | 같음 |
| QA-FX-004 | Q85~Q87 | 불씨 볼트: 흰 심 → 속성색 몸통 · 불티 · 터짐 고리가 참고 그림과 닮았다 | ③ `capture_bolt.ps1 -Spell Ember`(연출만 `Grim.FX.Bolt`, slomo 0.02) | 비행 중 빛줄기 · 불티, 표적에 터짐 고리 | ✅ 11-06 — 흰 심 → 주황 빛줄기 · 완드 앞 불티, 표적 구에 불꽃 고리 · 방사 빛살. 「참고 그림과 얼마나 닮았나」는 사람 판단(⏸ — [Q87](../spec/decisions#미결-질문)) | `qa-fx-ember-bolt.jpg` · `qa-fx-ember-impact.jpg`(60장 중 4 · 16번) |
| QA-FX-005 | Q85 | 가까운 표적(4 m)에 실제 시전하면 완드 끝부터 표적까지 빛줄기가 한 번 그어지고 표적에서 터진다(판정은 그대로) | ③ `capture_bolt.ps1 -Spell Ember -Cast -Tag _near`(L_Tower 옆 카메라, 1/50) · ① `Grimoire.Magic.Delivery.LaunchStreak` | 0.05초 비행 동안 빛줄기가 완드 → 표적으로 자라고, 적중 터짐 | ✅ 11-07 — 바늘 빛줄기가 완드에서 표적 앞까지 자라고(흰 심 → 주황, 불티), 표적에 고리 터짐. 로그 `[Bolt] DA_Spell_Ember 맞음 GrimTrialTarget_1` | `qa-fx-bolt-near.jpg`(60장 중 21 · 25번) |
| QA-FX-006 | — | 마법 Niagara 시스템에 Effect Type `EFT_Grim_Spell` — [성능 규칙 24](../systems/performance#렌더-·-연출) | ① `Grimoire.MagicFX.Assets`(연출 묶음 4개의 섬광 · 몸통 · 터짐 · 발사 빛줄기) · `Saved/MagicFX/make_spell_fx.txt` | 모두 `EFT_Grim_Spell`, 거리 컬 · 개수 상한 · 거리 중요도 | ✅ 11-07 — 만든 5개 + 그 밖 `/Game/Grimoire/MagicFX` 9개. 날씨 · 환경(`_Weather` · `_Ambient`)은 아직 — [BUG-006](./bugs) 진행 중 | 자동화 · `make_spell_fx.txt` |
| QA-FX-007 | R04 | 날씨 연출(비) | ③ 장면 `qa-rain` | 빗줄기 | ❌ [BUG-009](./bugs) | `qa-rain.jpg` |
| QA-FX-008 | Q86 | 물줄기: 이어진 바늘 빛줄기 + 물방울 불티가 판정 호를 따라가고, 막힌 곳에 물보라 · 고리, 표적에 터짐 | ③ `capture_bolt.ps1 -Spell WaterJet -Cast -Hold 10 -Slomo 0.25`(실제 시전) · `-FxCmd "Grim.FX.Spawn DA_Spell_WaterJet Trail …"`(옆) · `capture_spell.ps1 -Spell WaterJet -Hold 8`(어깨 뒤) | 흰 심 → 파랑 빛줄기 포물선, 물방울, 끝 물보라 | ✅ 11-07 — 옆에서 이어진 빛줄기가 표적 위로 포물선을 그리고 물방울 불티 · 끝 물보라, 실제 시전은 표적에 물보라 고리 · 판정 `[Stream] … → GrimTrialTarget_1`. `Grim.Cam` 고정 카메라에서는 조준 = 카메라라 줄기가 카메라 시선으로 나간다(그림 오른쪽 아래 — 캡처 방식 탓, 실제 플레이는 맞음) | `qa-fx-stream.jpg` |
| QA-FX-009 | Q86 | 미풍: 깃털 빛줄기 여러 가닥이 원뿔 사거리 · 반각만큼 부채꼴로, 맞은 곳에 바람 갈래 · 고리 | ③ `capture_bolt.ps1 -Spell Breeze -FxCmd "Grim.FX.Spawn DA_Spell_Breeze Trail 30 -1000 120 0 0 0 ConeRange=1000 ConeHalfAngle=30" -SideCam "350 -1000 400 -89 0 100"`(위) · `-Cast`(실제 시전) | 부채꼴(±30°)로 퍼지는 연두 깃털 빛줄기 | ✅ 11-07 — 위에서 9가닥 깃털 + 바늘 가닥이 부채꼴로 퍼졌다 사라지고, 실제 시전은 표적에 고리 · 바람 갈래 | `qa-fx-breeze.jpg` |
| QA-FX-010 | Q85 | 먼 표적(20 m): 발사 빛줄기가 첫 7.5 m 를 긋고 볼트 꼬리가 이어 날아가 표적에서 터진다 | ③ `capture_spell.ps1 -Spell Ember -SetupList "Grim.Go Jackalope;Grim.Creature.Time 0.8;Grim.Spawn Jack 1 20" -Slomo 0.05`(어깨 뒤) | 빛줄기 → 비행 → 터짐이 끊기지 않음 | ✅ 11-07 — 완드 앞 빛줄기 · 불티, 날아가는 볼트, 언덕 위 재칼로프에서 터짐 | `qa-fx-bolt-far.jpg` |
| QA-FX-011 | 규칙 24 | 새 이펙트의 입자 수가 마법 하나 2,000 안 | `make_spell_fx.py` 의 입자 어림(한 장 1 · 터짐 개수 · 계속 = 초당 × 최대 수명) | 모두 ≤ 2,000 | ✅ 11-07 — 물줄기 몸통 355 · 터짐 29, 미풍 52 · 22, 발사 빛줄기 19 | `Saved/MagicFX/make_spell_fx.txt` |

## 그림

![QA-FX-005 — 가까운 볼트(4 m, 실제 시전, 옆 1/50): 위 = 빛줄기가 자람, 아래 = 표적 터짐](/qa/qa-fx-bolt-near.jpg)

![QA-FX-010 — 먼 볼트(20 m, 어깨 뒤 1/20): 발사 빛줄기 → 비행 → 터짐](/qa/qa-fx-bolt-far.jpg)

![QA-FX-008 — 물줄기: 위 = 옆에서 본 연출, 아래 왼쪽 = 어깨 뒤 실제 시전(초원), 아래 오른쪽 = 고정 카메라 실제 시전(표적 물보라)](/qa/qa-fx-stream.jpg)

![QA-FX-009 — 미풍: 왼쪽 = 위에서 본 부채꼴 깃털, 오른쪽 = 실제 시전(표적 고리)](/qa/qa-fx-breeze.jpg)

![QA-FX-004 — 불씨 볼트 비행(옆 카메라 · 느린 시간 1/50)](/qa/qa-fx-ember-bolt.jpg)

![QA-FX-004 — 불씨 볼트 터짐](/qa/qa-fx-ember-impact.jpg)
