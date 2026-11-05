# 마법 이펙트 자체 제작

2026-11-06 사용자 요청: 「이펙트가 마음에 안 든다 — 지피티로 시안을 뽑아 그대로 만들 수 없나?」. Niagara 에디터를 손으로 만지지 않고, **GPT 시안 → 사양(파이썬) → 자동 생성 에셋 → 게임 화면 비교** 로 이펙트를 만드는 길을 조사하고 불씨 · 기본 공격 두 마법으로 끝까지 증명했다. **이 시험 이펙트(`NS_GrimEmber_*` · `NS_GrimBasic_*`)는 마법에 연결하지 않았다** — 볼트 이펙트는 사용자가 준 레퍼런스(바늘 빛줄기)대로 별도 갈래가 만든다. 이 문서와 도구는 그 갈래와 이후 물줄기 · 미풍이 쓰는 저작 도구다. 연출이 어디서 그려지는지는 [투사체 · 시전 감각](./spell-feel) · [마법](./magic).

## 결과 먼저

왼쪽이 GPT 시안, 오른쪽이 게임 화면(해 질 녘 초원, 0.1배 느린 화면에서 찍음 — 찍는 동안만 연출 묶음에 잠시 연결했다). 시간은 누른 때부터 잰 게임 초.

![불씨: 시안 대 게임](/art/fx/ember-compare.jpg)

![기본 공격: 시안 대 게임](/art/fx/basic-attack-compare.jpg)

| | 시안과 맞는 것 | 아직 못 맞춘 것 |
|---|---|---|
| 불씨 | 주황 별 섬광 · 불티, 흰 심 + 주황 껍질 혜성과 꼬리, 터질 때 큰 섬광 · 주황 빛 번짐, 뒤늦게 오르는 짙은 연기 | 불꽃 **모양** — 시안은 말려 올라가는 불꽃 혀(꽃잎), 게임은 둥근 덩이. 절차적 플립북 텍스처의 한계(아래 「텍스처」). 터짐 한가운데가 아직 살구색으로 밝다. 바닥 그을음 링은 C++ 데칼 그대로 |
| 기본 공격 | 얇은 마법진 고리 + 별빛, 밝은 구슬 + 보랏빛 광륜 + 가는 빛 꼬리, 별 섬광 + 퍼지는 고리 + 빛 알갱이 | 구슬 둘레를 **감는 빛 띠**(시안 2) — 궤도 리본이 없다. 시전 고리가 휘두르는 완드 끝(아래쪽)에 생겨 몸통과 떨어져 보인다(섬광 위치는 C++ 큐가 정함). 고리에 룬 점이 없다 |

물줄기 · 미풍 시안도 뽑아 두었다(아래). 네 마법 모두 게임에는 아직 Niagara Examples Pack 이 그대로다.

| 물줄기 | 미풍 |
|---|---|
| ![물줄기 시안](/art/fx/waterjet-concept.jpg) | ![미풍 시안](/art/fx/breeze-concept.jpg) |

## 무엇으로 얼만큼 만들 수 있나

| 수단 | 할 수 있는 것 | 못 하는 것 · 함정 | 판정 |
|---|---|---|---|
| **(a) Niagara 툴셋**(엔진 실험 플러그인 `NiagaraToolsets` + `ToolsetRegistry`) | 템플릿에서 시스템 생성 · 에미터 추가/삭제 · 모듈 추가 · 모든 모듈 입력(숫자 · 색 · 열거 · **HLSL 식** · 링크) · 렌더러 값(머티리얼 · SubUV · 정렬) · 사용자 변수 · 컴파일 상태 읽기. 파이썬 `unreal.ToolsetRegistry.execute_tool(툴셋, 도구, JSON)` 한 줄로 부르고, **헤드리스 커맨드렛(-nullrhi)에서 돈다** | 실험 기능(이름 · 인자가 판마다 바뀔 수 있다). 곡선(Curve) 데이터 인터페이스 값은 「Unsupported」 — 대신 HLSL 식으로 쓴다. 경량(Lightweight) 에미터 · 데이터 채널 불가. 아래 「함정」 3개 | **채택** — 이펙트의 뼈대 |
| (a′) 같은 API 를 C++ 로 직접(`UNiagaraExternalEditUtilities`) | 툴셋과 같은 기능. `NiagaraEditor` 모듈 의존을 에디터 빌드에만 걸면 된다 | 툴셋이 이미 JSON 으로 감싸 주므로 이점이 없다. 우리 모듈엔 컴파일 대기 함수 하나만 감쌌다(`UGrimMagicFXEditorLibrary`) | 필요할 때만 |
| **(b) 머티리얼 파이썬**(`MaterialEditingLibrary`) | 노드 그래프를 통째로 만든다: 텍스처 · 파티클 색 · 패너(흘리기) · 프레넬 · **침식**(알파가 줄면 가장자리부터 깎임) · 플립북(ParticleSubUV) · 노출 되돌리기 · 인스턴스 | 쓰임 표시는 `set_material_usage` 대신 속성(`used_with_niagara_sprites`)으로 켜야 저장된다. 셰이더 결과는 게임에서만 보인다 | **채택** |
| **(c) GPT 텍스처** | 콘셉트 시트(4칸)는 아주 잘 나온다 — 방향 · 색 · 모양을 정하는 기준 | 4×4 플립북처럼 **칸이 정확히 맞아야 하는 시트**는 칸 · 크기 · 연속성이 어긋난다. 알파가 없어 검은 배경 → 밝기 = 알파로 바꿔야 한다 | 시안용 채택. 텍스처는 다음 단계(아래) |
| (c′) 절차적 텍스처(numpy) | 부드러운 빛 · 별 섬광 · 고리 · 불티 · 이어지는 노이즈 · 리본 결 · 불꽃/연기 플립북 — 칸이 정확하고 다시 만들기 쉽다 | 손그림 같은 불꽃 혀 · 물 왕관 같은 **복잡한 모양**은 못 낸다(불씨 터짐이 둥근 덩이인 까닭) | **채택**(지금 텍스처 전부) |
| (d) 블렌더 메시 | 소용돌이 · 고리 · 원뿔 · 리본 메시 + 흘리는 머티리얼 — 미풍의 휘어진 바람 줄기, 물줄기 몸통, 기본 공격의 궤도 띠에 맞다 | 이번엔 안 했다. 메시 파티클 렌더러 + UV 흘리기 머티리얼이 필요 | 다음(물줄기 · 미풍) |
| (e) C++ 펄스(`AGrimFXPulse`) | 빛(포인트 라이트) · 단순 구/원반/원뿔 번쩍임 | 모양이 단순해 그 자체로는 싸 보인다 | 빛만 남기고 모양은 1.0 으로 옅게 |

## 고른 길

```
GPT 콘셉트 시트(4칸: 시전 · 몸통 · 꼬리 · 터짐)
  → 사양: Scripts/MagicFX/make_fx_niagara.py 의 SYSTEMS (에미터마다 템플릿 · 수 · 수명 · 크기 · 색 · 속도 · 수명 곡선 HLSL)
  → 텍스처: Scripts/MagicFX/make_fx_textures.py (numpy, art/fx/textures/)
  → 머티리얼: Scripts/MagicFX/fx_materials.py
  → Niagara: Scripts/MagicFX/fx_niagara_builder.py (툴셋 호출)
  → (GPT 낱장 그림을 쓸 때) 알파 텍스처: Scripts/MagicFX/image_to_alpha.py
  → 연출 묶음 연결: Scripts/MagicFX/make_magicfx_assets.py (VS_*) — 이 갈래에서는 하지 않았다
  → 게임 캡처: Scripts/MagicFX/capture_spell.ps1 → 비교: Scripts/MagicFX/compare_fx.py
```

한 바퀴(사양 고침 → 조립 → 캡처 → 비교)가 6~8분이다. 이번 두 마법은 9바퀴 돌았다.

### 실행

```powershell
python Scripts/MagicFX/make_fx_textures.py
$UE = "C:\Program Files\Epic Games\UE_5.8"
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -run=pythonscript -script="$PWD\Scripts\MagicFX\make_fx_niagara.py" -unattended -nop4 -nosplash -nullrhi
powershell -File Scripts\MagicFX\capture_spell.ps1 -Spell Ember -Out art\fx\ingame\ember -Start 90
python Scripts/MagicFX/compare_fx.py art/fx/raw/ember.png art/fx/ingame/ember docs/public/art/fx/ember-compare.jpg 380,160,880,660 0.15,0.3,0.6,0.9
```

- 일부만: `GRIM_FX_SYSTEMS=NS_GrimEmber_Impact`(쉼표 여럿), 공용 머티리얼 다시 만들기 `GRIM_FX_MATERIALS=1`(일부만 · 사양 파일일 때 기본은 건너뜀), 호출 추적 `GRIM_FX_TRACE=1`. 결과는 `Saved/MagicFX/make_fx_niagara.txt` — 시스템마다 「컴파일=UpToDate 설정 실패=0」 이어야 한다.
- 기본 공격은 시작 마법에 없다: `-SetupList "Grim.Go Jackalope;Grim.Creature.Time 0.8;Grim.Magic.SpawnSkillbook BasicAttack 100;Grim.Magic.ReadSkillbook;Grim.Spawn Jack 1 10"`.
- 툴셋 플러그인 두 개(`ToolsetRegistry` · `NiagaraToolsets`)는 `Grimoire.uproject` 에 에디터 전용으로 켰다. 게임 빌드에는 들어가지 않는다.

### 다시 쓰는 도구

| 도구 | 하는 일 |
|---|---|
| `Scripts/MagicFX/make_fx_niagara.py` + `GRIM_FX_SPEC=<json>` | **JSON 사양 → Niagara 시스템.** 이 파일을 고치지 않고 다른 갈래가 자기 사양 파일로 만든다. 형식은 `art/fx/specs/example.json`(바늘 빛줄기 꼬리 · 잔불꽃 · 섬광 · 고리 — 2026-11-06 「설정 실패 0」 확인). 머티리얼은 경로나 별칭(`add` · `fire` · `smoke` · `ribbon` · `flare4` · `flare6` · `ring` · `spark`) |
| `Scripts/MagicFX/fx_niagara_builder.py` | 위의 바탕. 에미터 사양 키: template · name · material · loop · duration · burst · rate · lifetime · color · size · size_xy · rotation · spin · speed · cone · axis · radius · gravity · drag · alpha(HLSL) · rgb(HLSL) · grow(HLSL) · subuv · local · align · ribbon_width · renderer · raw. 파이썬에서 `SystemBuilder` 로 도구를 직접 불러도 된다 |
| `Scripts/MagicFX/fx_materials.py` | 가산 스프라이트(진한 곳 흰 심) · 반투명 불꽃 플립북(침식) · 연기 · 리본(결 흘리기) 머티리얼과 텍스처만 바꾼 인스턴스(`make_instance`). 모두 노출 되돌리기 · 쓰임 표시 포함 |
| `Scripts/MagicFX/image_to_alpha.py` | **검은 배경 그림 → 알파 텍스처.** `mask`(밝기 = 알파, 회색조) · `rgba`(배경을 뺀 색 + 알파) · `sheet`(콘셉트 시트의 칸 하나, 칸 번호 지움) · `flipbook`(낱장 여러 장을 같은 칸으로 묶음). 물체를 찾아 가운데 · 정사각형 · 여백으로 맞춘다 |
| `Scripts/MagicFX/make_fx_textures.py` | 절차 텍스처(빛 · 별 · 고리 · 불티 · 노이즈 · 리본 결 · 불꽃/연기 플립북) |
| `Scripts/MagicFX/capture_spell.ps1` · `compare_fx.py` | 느린 화면 캡처(자기가 띄운 프로세스만 끈다) · 시안 옆에 붙인 비교 그림 |
| `UGrimMagicFXEditorLibrary::CompileNiagaraSystemAndWait` | 저장 전 Niagara 컴파일 대기(에디터 전용, 파이썬에서 `unreal.GrimMagicFXEditorLibrary.compile_niagara_system_and_wait`) |

### 만든 것(시험 · 연결 안 함)

| 에셋 | 내용 |
|---|---|
| `MagicFX/Niagara/NS_GrimEmber_Cast` · `_Trail` · `_Impact` | 불씨 섬광(별 · 광륜 · 불꽃 혀 · 불티) / 몸통(흰 심 · 불꽃 껍질 · 리본 꼬리 · 불티 · 연기) / 터짐(섬광 · 광륜 · 꽃잎 · 불티 · 연기) |
| `MagicFX/Niagara/NS_GrimBasic_Cast` · `_Trail` · `_Impact` | 기본 공격 고리 · 별 · 빛살 / 심 · 광륜 · 리본 · 빛 알갱이 / 별 · 고리 · 광륜 · 빛 알갱이 |
| `MagicFX/Materials/Particles/` | `M_GrimFX_PAdd`(가산, 진한 곳 흰 심) · 인스턴스 Flare4 · Flare6 · Ring · Spark, `M_GrimFX_PFireSubUV`(반투명 불꽃 플립북, 침식, 가장자리 붉은빛), `M_GrimFX_PSmoke`, `M_GrimFX_RibbonAdd`(결 흘리기 + 노이즈, 꼬리 끝 가늘게) |
| `MagicFX/Textures/T_FX_*` | 절차적 회색조 9장(밝기 = 알파) |

캡처 때는 투사체의 엔진 기본 회색 구를 감췄다(Niagara 몸통이 있을 때). 그 변경은 볼트 갈래가 맡아 이 갈래에서는 되돌렸다.

## 함정 (이번에 밟은 것)

1. **같은 세션에서 시스템을 지우고 같은 이름으로 다시 만들면 저장 때 엔진이 죽는다**(널 참조 접근). 빌더는 있는 에셋을 지우지 않고 에미터만 비워 다시 채운다.
2. **컴파일 중 저장**도 위험해 저장 전에 `UGrimMagicFXEditorLibrary::CompileNiagaraSystemAndWait` 로 기다린다(엔진 함수가 파이썬에 없어서 감쌌다).
3. 커맨드렛이 죽으면 그 `UnrealEditor-Cmd` 와 `CrashReportClientEditor` 가 남아 다음 실행을 막는다 — `Start-Process -PassThru` 로 받은 **자기 프로세스 Id 로만** 끈다. 이름으로 끄면(Stop-Process -Name …) 다른 세션의 에디터 · 측정까지 죽는다. 다른 세션이 빌드 중이면 UBT 뮤텍스 때문에 게임이 시작 단계에서 멈춘다.
4. **자동 노출**: 해 질 녘 · 어두운 곳에서 노출이 오르면 발광 색이 통째로 하얗게 밀린다(어두운 연기가 회색). 모든 이펙트 머티리얼의 발광에 `EyeAdaptationInverse` 를 곱했다 — 1.0 = 화면에서 꽉 찬 색. 불꽃 몸통은 1.5, 가산 빛은 2.5~3 이 알맞았다.
5. 가산 합성은 밝은 하늘 앞에서 흰색으로 날아간다 — 불꽃 덩이는 반투명(가장자리 짙은 붉은빛)이어야 낮에도 주황으로 읽힌다.
6. 새로 붙인 `ScaleSpriteSize` 는 곡선 모드가 기본이라 `Scale Factor` 가 숨겨져 있다 — `Scale Sprite Size Mode = Uniform` 뒤 `Uniform Scale Factor`. 열거 값은 `UEnum_Info` 로 표시 이름 → `Enum::NewEnumeratorN` 을 찾는다.
7. `Grim.Magic.Cast` 는 대상을 본 뒤 **게임 시간 0.5초** 있다 누른다 — 느린 화면은 그 직전에 켜야 순간이 맞는다.

## 다음 단계

- **불꽃 · 물 · 바람 모양 텍스처를 GPT 로**: 시트 전체가 아니라 「검은 배경 위 불꽃 혀 하나」 같은 **낱장 4~8장**을 뽑아 `image_to_alpha.py flipbook` 으로 묶는다(칸 맞춤은 코드가). 불씨 터짐의 꽃잎 모양이 이걸로 시안에 가까워질 것이다.
- **물줄기 · 미풍**: 블렌더로 휘어진 리본/원뿔 메시 + UV 흘리는 머티리얼 + 메시 파티클(물줄기 몸통 · 미풍 바람 줄기), 물 왕관은 메시 플립북 또는 GPT 낱장.
- 기본 공격 궤도 띠: 심 둘레를 도는 파티클 하나에 리본을 달거나(로컬 공간 원운동 HLSL), 고리 메시를 빙글 돌린다.
- 시전 섬광 위치를 완드 끝 **발사 순간**으로(지금은 휘두르는 도중의 완드 끝).
- 소리는 아직 엔진 기본 임시 소리다.
