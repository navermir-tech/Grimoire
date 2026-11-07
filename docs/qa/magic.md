# QA — 마법 · 완드 · 속성 상호작용

[QA 절차](../dev/qa) · 스펙 [마법](../spec/magic), 설계 [마법 · 상호작용](../systems/magic) · [투사체 · 시전 감각](../systems/spell-feel). 마지막 실행 **2026-11-06**(main `a6da3f7` 기준).

다시 돌리기: `powershell -File Scripts\QA\run_all.ps1 -Only automation,net_check,delivery,controls,growth,weather`.

## 전달 방식 · 비용 · 상성

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MAG-001 | R01 | 시작 마법 셋(불씨 Bolt · 물줄기 Stream · 미풍 Cone)과 전달 방식 | ① `Grimoire.Magic.StartingSpells` · `.Delivery.StartingSpells` | 통과 | ✅ 11-06 | 자동화 153/153 |
| QA-MAG-002 | R17 | 볼트 유도(약하게 휨 Q16) · 시차 보정 · 물줄기 호 · 원뿔 판정 · 거리별 세기 | ① `Grimoire.Magic.Delivery.Homing` · `.Parallax` · `.StreamArc` · `.Cone` · `.Falloff` | 통과 | ✅ 11-06 | 같음 |
| QA-MAG-003 | Q19 | 마나 · 쿨다운: Stream 만 초당, 나머지는 시전 한 번 | ① `Grimoire.Magic.Cost` · `.Effects` | 통과 | ✅ 11-06 | 같음 |
| QA-MAG-004 | R07 · R22 | 상성 표 — 4원소 순환, 빛 ↔ 어둠 극성, 무속성 ×1.0 | ① `Grimoire.Magic.Affinity` | 통과 | ✅ 11-06 | 같음 |
| QA-MAG-005 | R35 | 두 속성 대상의 상성 | ① `Grimoire.Creatures.DualAffinity` | 통과 | ✅ 11-06 | 같음 |
| QA-MAG-006 | R13 | 시전자 즉시 반응(가짜 볼트) · 지연 보정 · TargetData 검증 | ① `Grimoire.Magic.Delivery.Network` + ④ `net_check_delivery.ps1` 클라이언트 `가짜 볼트 발사` | 클라이언트가 누른 순간 가짜 볼트, 서버 판정 뒤 양쪽 상태 같음 | ✅ 11-06 | `delivery.txt` |
| QA-MAG-007 | — | 투사체 틱 의존 순환 경고 없이 돈다 | ④ `delivery` · `controls` 로그 | `LogTick: Warning … would form a cycle` 없음 | ❌ [BUG-002](./bugs) | 서버 215줄 · 클라이언트 423줄 |

## 속성 상호작용 (R16)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MAG-010 | R16 | 「양초 예시」 규칙: 불 → 켜짐, 먼 바람 → 커짐, 가까운 바람 → 꺼짐, 물 → 꺼짐 + 젖음 | ① `Grimoire.Interaction.CandleExample` | 통과 | ✅ 11-06 | 자동화 |
| QA-MAG-011 | R16 | 반응 규칙표(10행 이상) · 상태 컴포넌트(복제 · 휴면) · 전파 · 마른 풀이 타고 꺼짐 | ① `Grimoire.Interaction.ReactionTable` · `.Component` · `.Propagation` · `.DryGrassBurnsOut` | 통과 | ✅ 11-06 | 자동화 |
| QA-MAG-012 | R16 · R13 | 2인: 서버 자극 → 양초 켜짐 · 커짐 · 꺼짐 + 젖음, 접속 전 켠 모닥불도 클라이언트에 보임(휴면 깨우기) | ④ `net_check.ps1` | 45 · 55 · 70초 덤프가 서버 · 클라이언트에서 같은 줄 | ✅ 11-06 | `net_check.txt` — 양초 셋 `Burning=1` → `2` → `Wet=1`, 모닥불 `Burning=1` 양쪽 같음 |
| QA-MAG-013 | R16 · R13 | 2인: 원격 클라이언트가 실제 시전으로 켜고 · 키우고 · 끄고 · 물줄기로 끈다 | ④ `net_check_delivery.ps1` | [QA-CHR-013](./character#조준-r17-·-r24) | ✅ 11-06 | `delivery.txt` |
| QA-MAG-014 | R04 · R16 | 비가 야외 양초를 끄고 적신다(2인) | ④ `net_check_weather.ps1` | 비 뒤 양초 꺼짐 · 젖음 양쪽 | ✅ 11-06 — 비 뒤 양초 셋 `Wet=2.00` 서버 · 클라이언트 같음 | `weather.txt` |
| QA-MAG-015 | R26 | 기본 공격은 사물에 「충격」만 — 양초가 켜지지 않는다 | ④ `net_check_controls.ps1` 57~61초 | 4연타 뒤 양초 상태 그대로(`Burning=1` 유지) | ✅ 11-06 | `controls.txt` |

## 기본 공격 · 성장

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MAG-020 | R26 | 기본 공격: 무속성 · 비용 0 · 쿨 0 · 숙련도 안 쌓임, 연타 리듬, 4연타째 강타 | ① `Grimoire.Magic.BasicAttack.Definition` · `.Rhythm` · `.Combo` + ④ `controls` | 통과 + `기본 공격 1/4 … 4/4 강타` 서버 · 클라이언트 같음 | ✅ 11-06 | 자동화 · `controls.txt` |
| QA-MAG-021 | Q26 | 숙련도 D~S 등급 · 배수 · 반복 감쇠 · 표 편집 | ① `Grimoire.Magic.Proficiency.*`(7) | 통과 | ✅ 11-06 | 자동화 |
| QA-MAG-022 | R05 · R27 | 스킬북: 서클이 낮으면 `CircleTooLow`, 승급 뒤 `Learned`, 이미 아는 책은 숙련도 +80(C 구간 200 × 0.4), S 면 `AlreadyMastered` | ② `Scripts\Integration\check_growth.ps1` | 그 차례대로 | ✅ 11-06 | `growth.txt` |
| QA-MAG-023 | R06 | 서클 승급 1 → 2(마법 3 · 등급 합 2), 최대 마나 100 → 108 | ② 같은 스크립트 + ① `Grimoire.Magic.Proficiency.CircleUp` | `서클 1 → 2`, `마나=108/108` | ✅ 11-06 | `growth.txt` |
| QA-MAG-024 | R27 | 숙련도 · 슬롯 바꿈이 원격 클라이언트에 복제 | ④ `net_check_controls.ps1` 입력 덤프 · `check_save.ps1` 숙련도 덤프 | 서버가 본 클라이언트 슬롯 = 클라이언트 | ✅ 11-06 (슬롯) | `controls.txt` |

## 완드 · 마법 실패 알림 (Q71 · Q64)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MAG-040 | Q71 | 기본 완드 = 실제 장비(내구도 200 · 나무 수리) · 처음 입고 시작 · 닳으면 부러짐(칸에 남음) · 빈 칸 = `Status.WandMissing` · 옛 세이브(v1) 빈 칸에 기본 완드 | ① `Grimoire.Magic.CastFailure.BasicWand` | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-MAG-041 | Q71 · R13 | 2인: 두 플레이어 모두 기본 완드를 쥐고 시작(서버 · 소유 클라이언트) | ④ `Scripts\Magic\net_check_castfail.ps1` | `시작 장비 1개를 입혔다` ×2, `BaseDump … wandMissing=0 … gear=[0:Wand_Basic(200/200 r0)` | ✅ 11-07 | `Saved/castfail_result2.txt` — `[Server] GrimPlayerState_0 · _1 시작 장비 1개를 입혔다`, 서버 덤프 4 · 클라이언트 덤프 2 `wandMissing=0 … gear=[0:Wand_Basic(200/200 r0)]` |
| QA-MAG-042 | Q71 · Q4 | 2인: 완드를 벗으면 손에서 숨고 마법은 막히며 기본 공격 입력은 주먹질, 다시 입으면 보임 | ④ 같은 스크립트 | `손의 완드 숨김(주먹질)` ×2 · `주먹질` · `입음: Wand_Basic` · `손의 완드 보임` ×2 | ✅ 11-07 | `손의 완드 숨김(주먹질)` ×2 · `[Host]`/`[Server] 주먹질` · `입음: Wand_Basic` ×2 · `손의 완드 보임` ×2, 마지막 덤프 `gear=[0:Wand_Basic(188/200 r0)]` |
| QA-MAG-043 | Q64 · R13 | 2인: 쿨타임 · 마나 부족 · 완드 없음 — **서버가 판정**하고 그 플레이어의 소유 클라이언트만 알림(클라이언트 몫은 클라이언트 로그, 호스트 몫은 서버 로그) | ④ 같은 스크립트 | 서버 `마법 실패 GrimPlayerState_n Cooldown` · `NotEnoughMana` · `NoWand` 각 ×2, 클라이언트 `[Client] 마법 실패 알림 …` 셋 | ✅ 11-07 | 서버 `마법 실패 GrimPlayerState_1 Cooldown 값=1` · `… NotEnoughMana 값=10` · `… NoWand 값=0`(두 플레이어 몫), 클라이언트 `[Client] 마법 실패 알림 NotEnoughMana: 마나가 모자라다 (필요 10)` 등 셋, 서버 로그의 알림은 호스트 몫 셋뿐, `NETCHECK_RESULT fail=0` |
| QA-MAG-044 | Q64 | 2인: 실패 연출 큐 `GameplayCue.Magic.Fizzle` 이 모두에게(다른 플레이어 것도 보임) | ④ 같은 스크립트 | `[Client] 마법 실패 연출` · `[Server] 마법 실패 연출` 각 5번 이상(자기 것 + 상대 것) | ✅ 11-07 | 클라이언트 · 서버 각 6줄(자기 3 + 상대 3) `마법 실패 연출 GrimPlayerCharacter_0|1 …` — 헤드리스는 `이펙트=0`(nullrhi), ③ 에서 `이펙트=1` |
| QA-MAG-045 | Q64 | 2인: 서클이 모자란 스킬북(이그니스 2서클)을 가방에서 읽으면 까닭 알림 | ④ 같은 스크립트 | `가방 알림 CircleTooLow Skillbook_Fireball 필요 서클 2` (호스트 · 클라이언트) | ✅ 11-07 | `[Server] 가방 알림 CircleTooLow Skillbook_Fireball 필요 서클 2` · `[Client] 같은 줄` |
| QA-MAG-046 | Q64 | 까닭 고르기 순서(완드 → 서클 → 쿨타임 → 마나) · 기본 공격의 완드 없음은 알리지 않음(주먹질) · 같은 까닭 0.75초에 한 번 | ① `Grimoire.Magic.CastFailure.Classify` · `.BasicWand` | 통과 | ✅ 11-07 | 자동화 171/171 |
| QA-MAG-047 | Q64 | 화면: HUD 위쪽 실패 알림 한 줄(색 · 글) · 피식 파티클 | ③ `Scripts\QA\shoot_qa.ps1 -Set Fizzle`(느린 시간 1/10 에서 누르고 1초 뒤) | 붉은빛 한 줄, 완드 끝 · 손에서 작은 터짐 | ✅ 11-07 — 「마나가 모자라다 (필요 10)」 · 「완드가 없다 — 주먹만 나간다 …」 위쪽 한 줄, 완드 끝(완드 없으면 빈손 앞)에 작은 터짐 · 완드 없을 때 손의 완드 숨김. 임시 템플릿이라 터짐이 회색보다 하얗다(이펙트 갈래 몫) — [그림](#그림-완드-·-실패-알림) | `qa-castfail-mana.jpg` · `qa-castfail-nowand.jpg`, 로그 `이펙트=1 몽타주=0` |
| QA-MAG-048 | Q64 | 마법 실패 **몸짓**(호그와트 레거시처럼 완드 털기) | ③ 사람 눈 | 몽타주가 재생된다 | ⏸ 11-07 — 애니메이션 에셋이 없어 `FizzleMontage` 가 비어 있다(자리만 — 넣으면 `몽타주=1` 로그). 지금은 파티클만 | — |

### 그림 (완드 · 실패 알림)

![QA-MAG-047 — 마나 부족: 위쪽 알림 + 완드 끝 터짐](/qa/qa-castfail-mana.jpg)
![QA-MAG-047 — 완드 없음: 위쪽 알림 + 빈손 앞 터짐(손의 완드 숨김)](/qa/qa-castfail-nowand.jpg)

## 연출 · 그리모어 (R21)

| ID | R | 기능 · 무엇을 | 방법 | 기대 결과 | 결과 | 증거 |
|---|---|---|---|---|---|---|
| QA-MAG-030 | R21 | 책 액터 · 떠 있기 · 시전마다 한 장 · 고른 쪽까지 넘김 · 거두기, 속성 문양 찾기 | ① `Grimoire.MagicFX.Book.*`(5) · `.Sigil.*`(2) · `.CastMotion.*`(2) | 통과 | ✅ 11-06 | 자동화 |
| QA-MAG-031 | R21 | 시전할 때마다 **상대 화면에서도** 내 책장이 넘어간다(M1.5 완료 기준 ⑤) | ③ 2인 화면 | 상대 화면에서 책장이 넘어감 | ⏸ 두 창을 동시에 찍는 시나리오가 아직 없다 — 넘김은 Cast 큐(복제)로 돌고 큐 복제는 ④ 로 확인됨 | — |
| QA-MAG-032 | — | 볼트 이펙트(섬광 · 몸통 · 터짐) 모양 | ③ `Scripts\MagicFX\capture_bolt.ps1` | [QA-FX-004](./fx) | ✅ 11-06 | `qa-fx-ember-bolt.jpg` |
| QA-MAG-033 | R21 | 시전 순간 완드 휘두름 · 책 빛남 | ③ `shoot_qa.ps1 -Set Pose` | [QA-CHR-024](./character#그림) | ✅ 11-06 | `qa-pose-cast.jpg` |
