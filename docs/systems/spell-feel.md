# 투사체 · 시전 감각 (Spell Feel)

> 상태: **구현 문서 v1.1** · 2026-11-05 · 마일스톤 **M1.5 「시전 감각」** (M1 다음, M2 전) — 전달 방식 · 네트워크(갈래 A) 코드 반영
>
> 레퍼런스: **호그와트 레거시**의 마법 시전 · 투사체. 관련: [마법 시스템](./magic) · [소프트 락온 (R17)](./magic#소프트-락온-r17) · [스펙 — 마법](../spec/magic)

## 왜 다시 만드나 — M1 플레이 테스트에서 본 문제

2026-11-05 2인 플레이 테스트에서 불씨 · 물줄기 · 미풍이 "날아가는 게 이상하다"는 피드백이 나왔다. 코드로 본 원인은 다음과 같다.

| 문제 | 원인 (M1 구현) |
|---|---|
| 세 마법이 다 똑같이 보인다 | 셋 다 `Projectile` 또는 `Beam` 한 가지 방식. 불 · 물 · 바람의 **움직임 자체**가 다르지 않다 |
| 흰 공이 날아간다 | 투사체 = 엔진 기본 구체 메시. 꼬리(트레일) · 빛 · 소리 · 피격 이펙트가 없다 |
| 누르고 나서 늦게 나간다 (원격 클라이언트) | 투사체는 서버에서만 생성되고 복제로 클라이언트에 온다. 왕복 지연만큼 늦게 보인다 |
| 미풍이 안 보인다 | 광선(Beam)은 판정만 있고 그림이 없다 |
| 화면 중앙과 다른 곳으로 날아가는 느낌 | 카메라 조준점과 완드 끝 사이의 시차(패럴랙스)를 보정하지 않는다. 가까운 대상일수록 크게 어긋난다 |
| 휘두르는 동작이 없다 | 시전 몽타주 · 손목 튕김이 없어 "완드에서 나간다"는 느낌이 약하다 |

## 호그와트 레거시에서 가져올 것

호그와트 레거시의 마법이 손에 붙는 이유를 우리 설계 원칙으로 옮긴다.

1. **빠르다.** 기본 공격은 거의 즉시 닿는 빠른 섬광이다. 오래 날아가는 공은 피하기용(적 마법)이나 특수 마법뿐이다. → 기본 시전은 **매우 빠른 볼트**.
2. **마법마다 움직임이 다르다.** 불은 폭발하며 퍼지고, 물은 줄기로 뿜어져 포물선을 그리고, 밀어내는 마법은 원뿔로 퍼지는 충격파다. → **전달 방식(Delivery)을 속성의 물리에 맞춘다.**
3. **대상에 붙는다.** 자동 조준된 대상 쪽으로 몸이 돌고 마법이 따라간다. → [소프트 락온](./magic#소프트-락온-r17)과 한 묶음으로 만든다.
4. **색이 곧 정보다.** 마법 계열마다 색이 정해져 있어 멀리서도 무엇을 쐈는지 읽힌다. → 우리는 **속성색**(물 `#3A7BD5` · 불 `#E0582B` · 바람 `#7CC9A6` …)을 트레일 · 피격 · 대상 표식에 같이 쓴다.
5. **손맛 3종 세트.** 완드를 튕기는 짧은 동작 → 완드 끝 섬광 → 맞은 곳의 터짐 · 흔들림 · 소리. 셋 중 하나라도 빠지면 허전하다.

> 수치 · 연출은 레퍼런스를 눈으로 보고 정한 **제안**이다. 구현하면서 플레이로 조정하고, 바뀐 값은 이 문서의 표를 고친다.

## 전달 방식 (Delivery) 다시 정의

| 방식 | 움직임 | M1.5 마법 | 판정 |
|---|---|---|---|
| **Bolt** (섬광) | 매우 빠른 직선 + 소프트 락온 대상 쪽으로 약간 휨. 짧은 꼬리 | 불씨 | 서버 투사체(스윕). 클라이언트는 즉시 가짜 섬광을 그림 |
| **Stream** (줄기) | 누르고 있는 동안 계속 뿜어지는 포물선 물줄기. 끝이 바닥에 떨어져 튄다 | 물줄기 | 서버가 0.1초마다 포물선을 여러 구간 스윕해 닿은 곳에 자극(물 세기 × 시간) |
| **Cone** (원뿔 파동) | 짧은 사거리의 원뿔 충격파. 공기가 일렁이는 고리가 퍼진다 | 미풍 | 즉시 원뿔 판정(각도 · 거리), 거리에 따라 자극 세기 감소 |
| Beam (광선) | 대상까지 이어지는 빛줄기 | (후속: 빛 속성) | 지금 방식 유지 + 그림 추가 |
| Lob (포물선 투척) | 느리게 호를 그려 떨어져 터지는 덩어리 | (후속: 땅 속성) | 서버 투사체 + 중력 |

### 세 마법의 새 모습

| 마법 | 방식 | 손맛 (시전 → 날아감 → 맞음) | 상호작용 (R16) |
|---|---|---|---|
| **불씨** (불) | Bolt | 손목 튕김 → 완드 끝 주황 섬광 → 불티를 흩뿌리는 짧은 꼬리의 불꽃 화살 → 맞으면 작은 화염 터짐 · 그을음 자국 | 맞은 사물에 불 자극 1.0 |
| **물줄기** (물) | Stream | 완드를 앞으로 뻗고 유지 → 포물선 물줄기 · 끝에서 물보라 → 젖은 바닥 자국(시간이 지나면 마름) | 줄기가 닿는 동안 물 자극이 쌓인다(초당 1.0). 양초는 한 번 스치면 꺼짐 |
| **미풍** (바람) | Cone | 완드를 옆으로 휘둘러 쓸기 → 반투명 공기 고리가 원뿔로 퍼짐 · 풀 · 먼지가 날림 → 가벼운 물체가 밀림 | 원뿔 안의 불에 바람 자극(가까울수록 셈 — 가까이서 쓰면 돌풍이 돼 꺼짐) |

미풍의 "가까울수록 셈"은 기존 반응 규칙(바람 0.3~2.5 = 불 키움, 2.5 이상 = 꺼짐)을 그대로 쓴다: 멀리서 = 키우기, 바로 앞에서 = 끄기. 같은 마법으로 거리 조절이 곧 기술이 된다.

## 구현 설계

### 데이터 — `UGrimSpellDefinition` 추가 필드

| 필드 | 뜻 |
|---|---|
| `Delivery` | `Bolt · Stream · Cone · Beam · Lob`(Lob 은 열거형만 — 지금은 중력 있는 Bolt) |
| `BoltSpeed` · `HomingStrength` | Bolt 속도(제안 6000~9000 cm/s), 락온 대상 유도 = 초당 최대 회전 각(°, 0 = 없음) |
| `StreamLaunchSpeed` · `StreamGravityScale` · `StreamTickInterval` · `StreamSegments` | 물줄기 포물선 · 판정 주기 · 구간 수. 비행 시간 = `Range ÷ StreamLaunchSpeed` |
| `StreamSplashRadius` · `StreamStimulusInterval` | 떨어진 곳 물보라 반지름(그 안의 것도 맞음) · 한 대상에 자극을 주는 간격(처음 닿으면 바로). 1.0 = "초당 1.0" |
| `ConeHalfAngle` · `ConeRange` · `ConeFalloff` | 원뿔 반각 · 거리 · 거리별 세기 `FGrimConeFalloff { NearDistance, NearIntensity, FarIntensity, Exponent }` |
| `bChanneled` · `ChannelManaPerSecond` | 누르고 있는 동안 지속(Stream) · 그동안 초당 마나(Q19) |
| `CastMontage` · `CastWindup` | 시전 동작 · 실제 발사까지 시간(제안 0.08~0.12초) |
| `VisualSet` | 속성별 이펙트 묶음(아래 `UGrimSpellVisualSet`) |

수치는 지금처럼 데이터 에셋(`DA_Spell_*`, 정본 `Scripts/Magic/make_spells.py`)과 `DefaultGame.ini`(`MaxLagCompensation` · `CloseAimDistance` · `ChestHeight` · `HomingMaxAngle` · 검증 허용치)에 둔다. 값 표는 [마법 시스템 — 시작 마법](./magic#시작-마법-m1-5-제안값). 순수 계산은 `Magic/GrimSpellDelivery`(액터를 모름 — 어빌리티 · 투사체 · 연출 · 테스트가 같은 식).

### 연출 — `UGrimSpellVisualSet` (DataAsset)

| 슬롯 | 내용 |
|---|---|
| `MuzzleFlash` | 완드 끝 섬광 (Niagara) |
| `Trail` | 볼트 꼬리 · 줄기 몸통 · 원뿔 파동 (Niagara) |
| `Impact` | 맞은 곳 (Niagara) + 자국 데칼(그을음 · 젖음) |
| `CastSound` · `LoopSound` · `ImpactSound` | 소리 |
| `CameraShake` | 아주 약한 흔들림(시전 · 큰 피격) |
| `ElementColor` | 트레일 · 대상 표식 공통 색 |

연출은 **GameplayCue** 세 태그로 띄운다 — `GameplayCue.Spell.Cast`(누르는 순간, 예측) · `.Channel`(Stream 누르는 동안 Add/Remove, 예측) · `.Impact`(서버 확정 뒤 모두). 마법마다 큐를 따로 두지 않고 처리기가 `SourceObject`(정의)의 `VisualSet` 을 읽는다. GAS 가 예측 · 복제를 맡아 시전자는 바로, 다른 사람은 복제로 본다. 실행 위치 · 파라미터는 [마법 시스템 — 연출 큐](./magic#연출-큐-gameplaycue). 볼트 꼬리만은 투사체가 `Trail` 을 직접 붙인다(움직이는 액터). 이펙트 에셋은 갈래 작업 때처럼 파이썬 헤드리스 또는 에디터에서 만든다.

#### 구현 (M1.5 갈래 C — `Source/Grimoire/MagicFX/`)

| 것 | 내용 |
|---|---|
| `UGrimCueNotify_SpellCast` (Static) · 에셋 `GC_Spell_Cast` | 누르는 순간: 상체 시전 동작 · 책장 한 장 · 몸을 조준 쪽으로. 정의의 `CastWindup`(불씨 0.1초) 뒤 완드 끝 섬광 파티클 + 속성색 펄스 · 소리 · 흔들림(시전자 화면만). 쓸기(Cone) 마법은 원뿔 파동(`Trail` + 원뿔 펄스)도 여기서. **Bolt 꼬리는 그리지 않는다** — 투사체(갈래 A)가 `Trail` 을 직접 붙인다 |
| `AGrimCueNotify_SpellChannel` (Actor, 시전자마다 하나) · `GC_Spell_Channel` | Stream 누르는 동안(Add/Remove): 완드 끝에서 조준 방향으로 `Trail` 파티클 + 포물선 물방울 구슬(앞을 막으면 끊김) · 도는 소리, 책장 초당 2장, 오른팔 뻗고 유지. 호는 지금 같은 속도(1400cm/s) · 중력의 근사 — 통합 때 A 의 `TraceStreamArc` 로 바꾼다 |
| `UGrimCueNotify_SpellImpact` (Static) · `GC_Spell_Impact` | 맞은 곳(`Location` · `Normal`): 터짐 파티클 + 속성색 펄스 · 소리 · 자국 데칼(그을음 · 젖음, 시간이 지나면 흐려짐) |
| `UGrimMagicFXVisualSet` | `UGrimSpellVisualSet` 의 자식. 임시 연출값(색 펄스 3종 · 자국 · 소리 세기 · 파티클 배율 · 줄기 구슬 · 시전 동작 지정)을 더한다. 에셋 `VS_Ember` · `VS_WaterJet` · `VS_Breeze` |
| `UGrimMagicFXSettings` (`[/Script/Grimoire.GrimMagicFXSettings]`) | 정의에 `VisualSet` 이 없을 때 속성(`Element.*`)으로 고르는 기본 묶음, 펄스 · 책 머티리얼, 그리모어 클래스, 정의에서 이름으로 읽는 필드(`CastWindup` · `StreamLaunchSpeed` · `StreamGravityScale` · `ConeRange` · `ConeHalfAngle`) |
| `UGrimSpellCameraShake` | 0.16초 감쇠 사인(피치 0.35° · 요 0.12°) — 「아주 약한」 시전 흔들림 |

- 연출 묶음 찾는 순서: `SourceObject` 가 VisualSet → `SourceObject.VisualSet` 필드(갈래 A 가 만든 필드를 이름으로 — 통합 때 직접 접근으로 바꿔도 됨) → 큐의 `Element.*`(없으면 정의의 `Element`, 그것도 없으면 시전자가 고른 마법) → 설정의 기본 묶음 → `FallbackVisualSet`.
- 큐 처리기 에셋(`GC_*`, C++ 클래스의 블루프린트 자식)은 `[/Script/GameplayAbilities.AbilitySystemGlobals] GameplayCueNotifyPaths=/Game/Grimoire` 에서 찾는다. 에셋 · 머티리얼 · 애님 BP 는 `Scripts/MagicFX/make_magicfx_assets.py`(헤드리스)가 만든다 — 값은 스크립트가 정본.

#### 임시 이펙트 — 엔진 기본 파티클 (사용자 지시 2026-11-05)

엔진 Niagara 플러그인의 기본 템플릿 시스템(`/Niagara/DefaultAssets/Templates/Systems/`)을 그대로 쓴다. 이 템플릿들은 **색 파라미터가 없어서** 색은 두 겹으로 맞춘다: ① 큐 처리기가 늘 Niagara `User.Color`(Cascade 는 `Color`)를 넘긴다 — 나중에 바꿔 낄 이펙트가 이 사용자 변수를 가지면 바로 속성색이 입혀진다. ② 지금은 **속성색 펄스**(엔진 기본 도형 + 가산 발광 머티리얼 `M_GrimFX_Glow` + 점광원)가 「색이 곧 정보」를 맡는다.

| 묶음 | 색 | 섬광 | 몸통(Trail) | 터짐 | 펄스 · 자국 — 멀리서 구분되는 모양 |
|---|---|---|---|---|---|
| `VS_Ember` 불씨 | `#E0582B` | DirectionalBurst | FountainLightweight(투사체 꼬리) | SimpleExplosion | 작은 주황 불덩어리 + 강한 빛, 터지면 큰 주황 구 · 그을음 |
| `VS_WaterJet` 물줄기 | `#3A7BD5` | DirectionalBurstLightweight | FountainLightweight(줄기) | DirectionalBurst | 파란 물방울 구슬 포물선, 터지면 납작한 물보라 원반 · 젖은 자국 |
| `VS_Breeze` 미풍 | `#7CC9A6` | DirectionalBurstLightweight | RadialBurst(파동) | RadialBurst | 반투명 연두 원뿔 껍질(사거리 · 반각만큼 퍼짐), 터지면 테두리 빛 구 · 자국 없음 |

소리도 임시로 엔진 기본(`/Engine/EngineSounds/1kSineTonePing` 을 높낮이만 달리 · `WhiteNoise` 를 짧게 끊어 · 줄기는 다시 틀어 반복)을 쓴다. 파티클 배율(0.25~0.6)은 눈으로 본 값이 아니라 추정이다 — 플레이로 맞춘다.

#### 볼트 이펙트 — 사용자 참고 그림대로 (2026-11-06)

불씨 · 기본 공격(튕기기)은 사용자가 준 참고 그림(`art/fx/refs/`) 방향으로 다시 만들었다: **가늘고 뾰족한 바늘 빛줄기**(흰 심 → 속성색, 깃털 같은 꼬리) + 길 위 불티, 완드 끝의 작은 방사형 튐, 표적의 방사형 불티 · 퍼지는 고리 · 번쩍임 · 빛살. 어두운 배경에 가산 발광. 속성별 시안(GPT)은 `art/fx/concepts/`, 시안 대 게임 비교는 `art/fx/compare/`.

| 에셋(`/Game/Grimoire/MagicFX/Bolt/`) | 내용 |
|---|---|
| `T_Bolt_*` | GPT 가 검은 바탕에 그린 빛줄기 · 불티 · 고리 · 빛살 · 실안개 → 회색조 마스크(`make_bolt_textures.py`). 번쩍임은 절차 생성 대체 그림 |
| `M_Bolt_Add` · `MI_Bolt_*` | 가산 · 무광. `Mask × 파티클 알파 × (파티클 색 × Intensity + Mask^CorePower × CoreWhite)` × 깊이 흐림 — 밝은 곳은 흰빛, 가장자리는 속성색 |
| `NS_Bolt_Cast` | 섬광 칸. 불티 18(속도 정렬) · 번쩍임 · 실안개 5 — 시스템 +Z 가 겨눈 방향 |
| `NS_Bolt_Trail` | 몸통 칸(투사체에 붙음). 심 빛줄기(투사체 속도) · 머리 번쩍임 · 꼬리(속도 0.75배) · 불티 |
| `NS_Bolt_Impact` | 터짐 칸. 불티 40 · 퍼지는 고리 · 번쩍임 · 빛살 |

- 색은 모두 Niagara 사용자 변수 `User.Color` 하나 — `TintNiagara` 가 연출 묶음의 속성색을 넣는다. 다른 속성 볼트는 같은 시스템에 색만 다른 연출 묶음이면 된다.
- Niagara 는 엔진 실험 플러그인 **NiagaraToolsets**(ToolsetRegistry 의 JSON 도구: 템플릿 이미터 붙이기 · 모듈 입력 · HLSL 식 · Set Parameters)로 `Scripts/MagicFX/make_bolt_fx.py` 가 데이터에서 만든다. 그다음 `make_magicfx_assets.py`(visual_sets) 를 돌린다.
- 볼트 연출 묶음은 `bHideProjectileMesh` 로 투사체의 색 구를 숨기고, 구 펄스(`MuzzlePulse` · `ImpactPulse`)는 끈다.
- 실제 시전은 투사체를 시전 대기(0.1초)만큼 앞당겨 만든다(볼트 75~90 m/s → 약 7.5 m). 그래서 **7.5 m 안의 표적은 비행 장면 없이 바로 터진다** — 화면 확인은 `Grim.FX.Bolt` · `Grim.Cam` 으로 한다(`Scripts/MagicFX/capture_bolt.ps1`, [치트](../dev/cheats)).

### 네트워크 — "내 화면에서는 즉시"

| 마법 방식 | 시전자(로컬) | 서버 | 다른 클라이언트 |
|---|---|---|---|
| Bolt | 누르는 즉시 Cast 큐(예측), `CastWindup` 뒤 **가짜 볼트**(판정 없음, 지형에서 멈춤)를 쏜다. 진짜 복제본은 숨긴다 | 진짜 투사체를 스폰해 판정. 지연 L(핑 ÷ 2)이 Windup 보다 작으면 남은 만큼 기다렸다가, 크면 바로 쏘고 넘은 만큼 앞으로 당겨 시작(`CatchUp`, 상한 0.15초) | 진짜 투사체 복제를 보고, 가짜는 시전자에게만 |
| Stream | 로컬에서 바로 Cast · Channel 큐(예측) | 0.1초마다 판정 · 자극 · 마나 | Channel 큐(Add/Remove)로 줄기를 그림 |
| Cone | 로컬에서 바로 Cast 큐(예측) | Windup 뒤 즉시 판정 | Cast 큐로 파동 그림 |

- 서버는 시전자가 보낸 TargetData 콜백 안(클라이언트 예측 키 창)에서 Cast · Channel 큐를 실행한다 → 시전자는 두 번 보지 않고, 다른 사람은 서버 것을 본다.
- 맞았을 때의 터짐은 **서버 확정 뒤** Impact 큐로 모두에게. 시전자의 가짜 볼트는 진짜가 사라지면 같이 사라진다(맞은 자리에서 터짐 큐가 대신함).
- 한 번 쏘는 마법(Bolt · Cone)은 서버가 판정한 뒤 어빌리티를 끝낸다 — 시전자가 먼저 끝내면 끝 RPC 가 판정보다 먼저 처리될 수 있다. Stream 은 시전자가 떼면 끝나고, 서버가 아직 한 번도 판정하지 않았으면 끝나기 전에 한 번 한다(짧게 눌러도 양초는 꺼진다).
- 확인: `Scripts/Integration/net_check_delivery.ps1`(원격 클라이언트가 실제로 시전, 양쪽 상태 비교).

### 조준 — 카메라와 완드의 시차 보정

1. 조준점: `IGrimAimProvider::GetCurrentAim` — 소프트 락온 대상이 있으면 그 대상의 조준점, 없으면 카메라 중앙에서 사거리까지 트레이스한 지점. 제공자가 없으면 어빌리티가 카메라 정면 트레이스. 대상이 있으면 Windup 뒤 발사할 때 대상의 지금 조준점을 다시 읽는다.
2. 발사 방향: **완드 끝 → 조준점**(`GrimSpellDelivery::ComputeLaunch`). 완드 끝과 조준점 사이가 막혀 있으면(벽 모서리) 투사체 · 스윕이 그 막힌 지점을 맞힌다. 조준점이 완드 끝 뒤(카메라와 완드 사이)면 시선 방향 그대로.
3. 아주 가까운 대상(`CloseAimDistance` 2m 안)은 완드 끝 대신 가슴 높이(폰 위치 + `ChestHeight` 40cm)에서 쏴서 어깨 너머로 엉뚱하게 날아가지 않게 한다.
4. Stream 은 조준점을 지나는 낮은 포물선으로 쏜다(`SolveLaunchDirection`). 닿지 않는 거리면 그 방향 45°(가장 멀리).

### 시전 동작

- 상체만 쓰는 시전 동작 3종: 튕기기(Bolt) · 뻗고 유지(Stream) · 쓸기(Cone). 하체는 이동 애니메이션을 그대로 둬 **움직이면서 시전**한다(레퍼런스와 같음).
- 시전 순간 캐릭터를 조준 방향(컨트롤 요)으로 0.1초 안에 돌리고 0.4초 동안 그쪽을 본다(그동안 이동 방향 회전을 끈다). 서버는 어빌리티 발동 콜백(`AbilityActivatedCallbacks`)에서, 소유 클라이언트는 예측 발동 · 시전 큐에서 같이 돌린다 — 다른 사람에게는 이동 복제로 간다. `AGrimPlayerCharacter::FaceAimForCast`, 수치 `CastFaceTurnTime` · `CastFaceHoldTime`.
- **M1.5 임시 구현 (몽타주 없음)**: 애님 BP `ABP_GrimPlayer` = 템플릿 `ABP_Unarmed` 를 복제해 부모를 C++ `UGrimSpellPoseAnimInstance` 로 바꾼 것. 이 클래스의 프록시가 그래프(이동 · 점프)를 평가한 **뒤** 컴포넌트 공간에서 뼈를 돌린다: 「뼈 → 자식 뼈」 방향을 목표 방향으로 무게만큼 돌리는 방식이라 뼈 축을 몰라도 된다.
  - 오른팔(`upperarm_r` · `lowerarm_r`): 동작별 목표 방향 + 조준 높이(복제되는 `GetBaseAimRotation` 피치)만큼 든다. 상체(`spine_03`)는 짧게 비틀고 숙인다.
  - 왼팔(`upperarm_l` · `lowerarm_l`): 「책 받치기」 고정 — 무게 = 1 − 책 거두기 값.
  - 트리거: 시전 큐 → `PlayCastMotion`, 줄기 큐 Add/Remove → `BeginHoldMotion` · `EndHoldMotion`. 그래서 다른 사람 화면에도 같은 동작이 보인다.

| 동작 | 시간 (제안 — `GrimMagicFXMath`) | 모양 |
|---|---|---|
| 튕기기 | 0.07초에 들고 0.2초부터 놓아 0.45초에 끝 | 손목을 젖혔다(0.03초) 앞으로 튕김(0.13초), 상체 −6° 비틀기 |
| 뻗고 유지 | 0.12초에 뻗고, 줄기 큐가 없으면 0.4초 뒤 · 있으면 Remove 뒤 0.25초에 놓음 | 앞으로 곧게, 조준 높이 그대로 |
| 쓸기 | 0.06초에 들고 0.32초까지 오른쪽 55° → 왼쪽 −40°, 0.55초에 끝 | 팔을 펴고 몸 앞을 가로지름 |

- 정식 몽타주가 생기면 `bProceduralSpellPose` 를 끄고 `ABP_GrimPlayer` 그래프에 상체 슬롯 · 본별 레이어 블렌드(`clavicle_r` 아래)를 넣는다(사용자가 에디터에서).

### 왼손 그리모어 (R21)

- `AGrimGrimoireBook`(`MagicFX/GrimGrimoireBook`): M1.5 임시 모양은 엔진 기본 Cube — 표지 두 장 · 책장 덩어리 두 개 · 넘기는 판 하나(펼친 크기 약 29 × 21cm) + 펼친 면 점광원(약하게). 머티리얼 `M_GrimFX_Book`(Color · EmissiveColor · Glow).
- **스폰 · 부착**: 캐릭터가 서버에서(`PossessedBy`) 스폰해 메시의 `hand_l` 에 Snap 부착한다(부착 · 포인터는 복제). 마네킹에는 소켓이 없어서, 메시에 `GrimoireFloat` 소켓이 있으면 그 자세를, 없으면 **`hand_l` 위치 + 월드 위쪽 12cm**에 몸 방향으로 수평(읽는 쪽으로 25° 기울임)으로 책을 둔다 — 손바닥 방향이 흔들려도 책은 위에 떠 있다.
- **떠다니기**: 위아래 1.5cm · 2초 주기 · 기울기 3°(다른 주기로 앞뒤 · 좌우). 각 기기가 틱(`TG_PostUpdateWork`, 손 자세가 정해진 뒤)에서 로컬로 그린다 — 복제 안 함. 시작 위상은 책마다 무작위.
- **책장 넘김**: 시전 큐(`GC_Spell_Cast`)가 **기본 공격 포함 모든 시전에** `FlipPageForSpell(ElementColor, 속성 태그, VisualSet.PageFlipDuration)` 을 부른다. 큐라서 시전자는 예측으로 바로, 다른 사람은 복제로 본다 — 별도 RPC 없음. 넘기는 중에 또 오면 최대 3장까지 쌓는다. 넘김 시간은 연출 묶음의 `PageFlipDuration`(0 = 책 기본 0.25초, **기본 공격 `VS_BasicAttack` = 0.15초** — 연타 리듬 0.3초에 맞춤). 줄기 큐(`GC_Spell_Channel`)는 `BeginChannelFlip` → `FlipRate`(초당 2장)로 반복, Remove 에서 `EndChannelFlip`.
- **속성 문양(M2 갈래 K)**: 책 면에는 마법마다가 아니라 **속성 문양 11개**(무속성 · 물 · 얼음 · 불 · 바람 · 땅 · 풀 · 신성 · 빛 · 흑마법 · 그림자술)만 있다. 임시 모양은 엔진 기본 도형(Cube = 네모 · 막대, Cylinder = 원, Cone 을 눕혀 누른 것 = 세모)을 납작하게 놓은 조각 1~6개 — 그림 텍스처 없음. 정의는 `[/Script/Grimoire.GrimBookSigilSettings] Sigils`(속성 태그 이름 · 조각 모양 · 자리 · 크기 · 돌림, cm), 하위 태그(`Element.Water.Liquid` · `Element.Dark.Hex`)는 점으로 끊긴 가장 긴 위 이름의 문양, 모르면 `FallbackElement`(무속성). 책에 조각 칸 두 벌(왼쪽 면 · 넘기는 판 뒷면, 각 6개): 시전 장의 뒷면에 문양이 실려 넘어가고, 다 넘기면 같은 자리의 왼쪽 면 문양이 이어받는다(세트 바꾸기 촤르륵 장에는 싣지 않는다). 빛 색 = 그 마법 `ElementColor`, 밝기 = `IdleGlow`(0.4) + 넘김 빛 × `FlashGlowScale`. 문양 정의 · 조각 변환은 `GrimBookSigilMath`(`FindSigilIndex` · `MakePieceTransform`).
- **넘김 구현**: 판을 책등(+X) 축으로 0.25초에 180° 돌린다(부드러운 시작 · 끝, 가운데에서 위로 들림). 다 넘긴 판은 오른쪽 면으로 돌아가 다음 장이 된다. 넘길 때 글자 면 발광이 0.07초에 속성색 최대(Glow 8)까지 올랐다가 0.45초에 걸쳐 사라지고, 점광원도 같은 색으로 번쩍인다. 정식은 책 스켈레탈 메시 + 넘김 애니메이션.
- **세트 바꾸기 촤르륵**: 보이는 슬롯 세트가 바뀌면(마법서 `ActiveSlotSet` — 모두에게 복제, 캐릭터가 틱에서 본다) `FlipTo(세트, 그 세트 첫 마법 색)` — 세트 하나에 3장(최대 12장, 앞 · 뒤 방향)을 0.5초에 나눠 넘긴다. 처음 본 세트는 기억만 하고 넘기지 않는다. (M2 갈래 K 전에는 「고른 마법」이 바뀔 때였다 — 시전이 슬롯 바로 시전으로 바뀌어 세트가 「마법 바꾸기」다.)
- **왼팔 자세**: 「책 받치기」는 위 「시전 동작」의 애님 인스턴스가 고정한다. 오른팔만 시전 동작을 받으므로 완드를 휘둘러도 책은 흔들리지 않는다.
- **거두기(Q25)**: 책이 매 틱 판정한다 — 완드 없음 · `State.Swimming`(또는 이동 컴포넌트 `IsSwimming`) · `State.BuildMode` · `State.Carrying` 이면 거둠, `State.Mounted` 면 `State.Aiming` 이거나 최근 `MountedCombatHoldSeconds`(4초) 안에 시전했을 때만 띄움. 거둘 때는 `GrimoireBelt` 소켓(없으면 `pelvis` 에서 몸 기준 왼쪽 옆구리, 세워서 0.8배)으로 0.3초 보간하고 빛을 끈다. 상태 태그는 아직 붙이는 시스템이 없어 **이름만** 정해 두었다(`MagicFX/GrimMagicFXTags`).
- 수치(높이 · 흔들림 · 넘김 시간 · FlipRate · 거두기)는 책 액터 기본값(UPROPERTY)이라 블루프린트 자식에서 바꾼다. 쓸 클래스는 설정 `GrimoireBookClass`.

## 마일스톤 결정 — M1.5 「시전 감각」

**결정 (2026-11-05)**: 투사체 개선은 M2 에 섞지 않고 **M1 바로 다음의 독립 마일스톤 M1.5** 로 한다. 소프트 락온(R17)도 M2 에서 이리로 옮긴다.

- 까닭 1: 마법 시전은 이 게임의 손 그 자체다(설계 기둥 1 「마법이 곧 손이다」). 생존 · 튜토리얼(M2)을 얹기 전에 손맛을 먼저 잡아야 이후 모든 테스트가 덜 거슬린다.
- 까닭 2: 소프트 락온과 투사체 유도 · 조준 보정은 같은 코드(시전 어빌리티 · 투사체)를 고친다. 따로 하면 두 번 뜯는다.
- 까닭 3: M2 는 이미 크다(생존 전체 · 날씨 · 튜토리얼 · 스킬북 · 세이브).

### M1.5 범위 · 완료 기준

| 항목 | 내용 |
|---|---|
| 전달 방식 | Bolt · Stream · Cone 구현, 불씨 · 물줄기 · 미풍을 새 방식으로 |
| 연출 | 마법 3종의 섬광 · 꼬리 · 터짐 · 자국 · 소리(임시 에셋 가능), GameplayCue 로 복제 |
| 네트워크 | 시전자 즉시 반응(가짜 볼트 · 예측 큐), 원격 클라이언트도 같은 그림 |
| 조준 | 시차 보정, 소프트 락온(R17) — 자동 대상 · 표식 · 유도 · 고정 락온 |
| 동작 | 상체 시전 동작 3종(임시 가능), 움직이며 시전, **왼손에 떠 있는 그리모어 · 시전마다 책장 넘김**(R21) |
| **완료 기준** | 2인 플레이에서 ① 누르는 즉시 나간다 ② 셋이 멀리서도 구분된다 ③ 락온된 양초를 불씨로 켜고, 멀리서 미풍으로 키우고, 가까이서 미풍으로 끈다 ④ 물줄기로 뛰어가며 마른 풀의 불을 끈다 ⑤ 시전할 때마다 상대 화면에서도 내 책장이 넘어간다 |

### 미결

| # | 질문 | 후보 |
|---|---|---|
| Q16 | 볼트 유도 세기 | ✅ 약하게 휨 |
| Q19 | 마나 소모 방식 | ✅ **누르고 있는 동안 나가는 마법(물줄기 같은 Stream)만 초당 소모**. 나머지(Bolt · Cone 등 대부분)는 시전할 때 한 번 |
| Q20 | 이펙트 에셋 | 지금은 Niagara Examples Pack. 자체 제작 길(GPT 시안 → 스크립트 조립)은 [마법 이펙트 자체 제작](./magic-fx-authoring) — [레퍼런스 조사](../spec/references#마법-이펙트-에셋-q20) |

## 테스트

- 순수 계산은 `Grimoire.Magic.Delivery.*` 자동화 테스트: `.Parallax`(시차 보정) · `.Homing`(유도) · `.StreamArc`(포물선 · 낮은 호) · `.Cone`(원뿔 판정) · `.Falloff`(거리별 세기 + 미풍 에셋이 양초 규칙에 맞음) · `.Network`(자극 주기 · 지연 보정 · TargetData 검증 · 왕복) · `.StartingSpells`(시작 마법 전달 방식).
- 2인 확인은 `Scripts/Integration/net_check_delivery.ps1`(net_check.ps1 과 같은 헤드리스 리슨 서버 + 클라이언트): 원격 클라이언트가 `Grim.Magic.Cast` 로 양초에 불씨 → 6m 미풍 → 1.2m 미풍 → 불씨 → 물줄기를 쏘고, 양쪽 `Grim.Interaction.Dump` 가 같은지 본다.
- 연출 · 동작은 `Grimoire.MagicFX.*`: 책 떠다니기 · 넘김 진행 · 촤르륵 장 수 · 거두기 판정(Book.*), 시전 동작 결정 · 자세(CastMotion.*), 줄기 포물선 · 원뿔 크기(Stream.Shape), 책 액터 틱(Book.Actor), 에셋 · 설정 · 큐 경로(Assets), 속성 문양 찾기 · ini 11개 · 조각 변환(Sigil.Lookup), 시전 장 → 펼친 면 문양 · 기본 공격 0.15초(Sigil.Flip).
- 조작 · 기본 공격(M2 갈래 K)은 `Grimoire.Input.*` · `Grimoire.Magic.BasicAttack.*` 와 `Scripts/Integration/net_check_controls.ps1`([마법 — 조작](./magic#조작-q29)).
- 손맛은 사람 눈이 필요하다 — 마일스톤 끝에 사용자 플레이 테스트.
