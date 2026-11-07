# 개발 기록 — 무엇을 · 어떻게 · 어디에

> 마일스톤 · 갈래별로 **무엇을** 만들었고 **어떻게**(핵심 설계 선택 · 도구) 했는지, **어디에**(코드 · 문서) 있는지, **언제** 합쳤는지, 어떤 **함정**을 밟았는지 짧게 적는다. 사람은 훑어보고, AI 는 새 세션에서 맥락을 잡는 데 쓴다.
>
> 자세한 설계는 각 링크의 시스템 문서가 정본이다. 커밋은 `git log --oneline` 의 짧은 해시. 새 갈래를 합칠 때 이 표에 한 줄 더한다([문서 갱신 체크리스트](./docs-checklist)).

## 일하는 방식

- **메인 세션 + 병렬 하위 세션.** 메인 세션이 마일스톤을 갈래로 나눠 「갈래와 파일 소유」 표([M1 작업 분할](../systems/m1-plan) 등)를 쓰고, 갈래마다 하위 세션이 자기 git 워크트리 · 브랜치(`claude/<이름>-<해시>`)에서 만든 뒤 메인이 `main` 에 「합치기:」 커밋으로 합친다. 공통 기반(태그 · 인터페이스 · 설정)은 갈래를 열기 전에 메인이 `main` 에 먼저 넣는다.
- **공유 파일 규칙**: `Grimoire.Build.cs` 는 끝에 한 줄, `Config/DefaultGame.ini` 는 파일 끝에 자기 섹션만, 새 태그는 자기 폴더, `docs/spec/decisions.md` 는 메인만 고친다([M2 후반 작업 분할](../systems/m2b-plan)).
- **에셋은 스크립트가 원본.** `.uasset` 을 손으로 만들지 않고 `Scripts/<시스템>/*.py` 를 헤드리스 에디터로 돌려 만든다 — 에셋은 결과물이라 언제든 다시 만들 수 있다([M1 — 에셋 만드는 방법](../systems/m1-plan#에셋-만드는-방법)).
- **확인은 자동으로.** 순수 계산은 `*Math` 클래스로 떼어 자동화 테스트(`Grimoire.<시스템>.*`), 복제는 헤드리스 리슨 서버 + 클라이언트 2명 스크립트(`Scripts/**/net_check*.ps1`)가 양쪽 상태를 비교한다. 사람 눈이 필요한 것(연출 · 카메라)은 통합 결과에 「사람 눈으로 볼 것」으로 남긴다.
- **도구**: 그림은 GPT(Codex 이미지 — `art/tools/run_codex_job.sh`), 3D 는 Tripo(`art/tools/tripo.py`) → 블렌더 정리(`art/tools/blender_cleanup.py`) → 언리얼 가져오기(`Scripts/Environment/import_env_asset.py`), 이펙트는 NiagaraToolsets(엔진 실험 플러그인)로 Niagara 를 헤드리스 조립. 문서는 VitePress.

## 타임라인

| 날짜 | 마일스톤 · 갈래 | 무엇을 | 합치기 |
|---|---|---|---|
| 11-04 | M0 셋업 | UE 5.8 C++ 뼈대 · Git LFS · 문서 사이트(VitePress + Netlify) · R01~R16 | `b87087b` `577af2d` |
| 11-05 | 경로 이동 | 한글 · OneDrive 경로 빌드 실패 → `C:\Dev\Grimoire` | `3d6c8ae` |
| 11-05 | M1 완드와 양초 (4갈래) | 캐릭터 · 마법 3개 · 속성 상호작용 · 낮밤 · 생존 | `ff12906`~`7772cf9`, 통합 `ba78ea6` |
| 11-05 | 치트 · 2인 버그 | `GrimSuperCheat`, 원격 불빛 · 완드 위치 수정 | `3a902fd` `7d35f92` |
| 11-05 | M1.5 시전 감각 (3갈래 + 줌) | 전달 방식 · 조준 · 연출, 줌인 조준 | `d0acfbe` `7c5938c` `a69bf88` `794ca1e` |
| 11-05 | M2 살아남기 (4갈래) | 줌 · 날씨 · 생존 · 마법 성장 UI | `24be533` `1df527a` `02de3a3` |
| 11-05 | M2 후반 (4갈래) | 레벨 · 성장 트리, 세이브 v1, 조작 · 기본 공격, 마법탑 튜토리얼 | `5b63272` `76ae11d` `9c27b38` `5ee83ed` |
| 11-05 | M3 생물과 친구 (5갈래) | 생물 · 생태계, 접속 · 메뉴, 아이템 · 요리, 지형 1차, 길들이기 · 탑승 | `34fb755` `6c4796e` `ebc373c` `f3d0dc9` `438b74e`, 통합 `7160145` |
| 11-05 | 지형 다시 만들기 | 레벨 디자인 노트 → 초원 2차 → 마탑 구간 먼저 | `38de2ac` `433c30e` |
| 11-06 | 첫 자체 제작 에셋 | 초원 바위 A(GPT → Tripo → 블렌더 → Nanite) | `89644cf` |
| 11-06 | 목록 · 제안 (문서) | 생물 명단 61종, 스킬북 89권, 인챈트 35종, 1번 지역 콘셉트 40장 | `1a5129c` `31aa6d7` `f8c1e7d` `142afdf` |
| 11-06 | 생물 보관함 (R38) | 데리고 다니기 · 쿠스토디아 · 거점 생물 상자 | `68650ec` |
| 11-06 | 커스터마이징 · 성능 · 이펙트 | 외형 조립, 성능 예산 · 첫 최적화, 이펙트 자체 제작 · 볼트 | `7800e3d` `9b815cf` `1f51324` `a6da3f7` |
| 11-07 | 이펙트 다시 (Q85 · Q86) | 볼트 발사 빛줄기, 물줄기 · 미풍을 볼트 빛 언어로, 사양 파일 조립 · `EFT_Grim_Spell` | 브랜치 `claude/fx-rework-1a2d5c` |
| 11-07 | 사용자 답 Q42 · Q64 · Q71 (코드) | 보관함 처음부터 10칸, 기본 완드 실제 아이템 · 빈 칸 = 주먹질, 마법 실패 알림 · 피식 연출 자리 | (메인이 합침) |
| 11-07 | 수리 해금 Q121 (코드) | 레스티투오(2서클) = 작업대 「수리」 탭 · 조각 하나, 막시마 = 거점 전체 수리, 마법탑 대리 수리(솔) | (메인이 합침) |

## M0 · 셋업 (2026-11-04 ~ 05)

- **무엇을**: UE 5.8 C++ 단일 모듈 `Grimoire`, Git LFS(`.uasset` · `.umap` · 그림), 문서 사이트, 요구사항 R01~R16([스펙 개요](../spec/)).
- **함정 — 한글 경로**: 처음 자리(OneDrive · 「바탕 화면」)에서 C++ 빌드가 `C1083` 으로 실패. UBT 응답 파일(BOM 없는 UTF-8)을 `cl.exe` 가 CP949 로 읽어 경로가 깨진다. 정션 우회도 안 됨 → `C:\Dev\Grimoire` 로 이동([시작하기 — 프로젝트 경로](./setup#프로젝트-경로-—-영문-경로에-둔다)).

## M1 · 완드와 양초 (2026-11-05)

[M1 작업 분할](../systems/m1-plan) · 공통 기반 `f27fce1`(GAS · 플레이어 상태 · 입력 태그 ASC · 속성 자극 인터페이스 · 공용 태그).

| 갈래 | 무엇을 · 어떻게 | 어디에 |
|---|---|---|
| A 캐릭터 | 3인칭 캐릭터, ASC 는 **PlayerState 가 소유**(죽어도 유지), 입력 액션 → GameplayTag 배열로 어빌리티에 전달, 샌드박스 맵을 스크립트로 | `Source/Grimoire/Character/`, `Scripts/Character/` |
| B 마법 · 완드 | 불씨 · 물줄기 · 미풍(`DA_Spell_*` = `UGrimSpellDefinition`), 비용 · 쿨다운 GE(SetByCaller), 상성 피해 `UGrimDamageExecution` | `Source/Grimoire/Magic/`, `Scripts/Magic/make_spells.py`, [마법](../systems/magic) |
| C 속성 상호작용 | `UGrimElementalStateComponent`(FastArray 복제 · Net Dormancy), 반응 규칙표는 **CSV 를 런타임에 읽음**(`Content/Grimoire/Data/ElementalReactions.csv`), 양초 · 모닥불 · 마른 풀 | `Source/Grimoire/Interaction/` |
| D 환경 · 생존 | 시간은 스냅숏 복제 + 클라이언트 외삽, 하늘 액터, 생존은 1초 주기 GE + 순수 계산 `GrimSurvivalModel` | `Source/Grimoire/Environment/`, `Source/Grimoire/Survival/`, [환경](../systems/environment) · [생존](../systems/survival) |

- **치트**: `GrimSuperCheat 1/0`([테스트 치트](./cheats)).
- **함정 — 휴면 깨우기 순서**: 상태를 바꾼 **뒤** `FlushNetDormancy` 를 부르면 엔진이 현재 값으로 기준을 새로 잡아 변화가 안 간다 → 원격 클라이언트에 불빛이 안 보였다. 바꾸기 **직전**에 flush(`7d35f92`).
- **함정 — 완드 부착**: `KeepRelativeTransform` 으로 붙여 손에서 떨어져 보였다 → `SnapToTargetNotIncludingScale` + Grip 오프셋.
- **함정 — 워크트리**: 메인 체크아웃에서 에디터가 떠 있으면 워크트리 빌드도 「Live Coding is active」 로 실패(뮤텍스 공유) → `-NoHotReloadFromIDE`. 워크트리의 `.uasset` 은 LFS 포인터라 에셋 테스트가 실패 → `git lfs checkout`.
- **함정 — 헤드리스 실행 뒤** 에디터가 `Config/DefaultEngine.ini` 에 AndroidFileServer 섹션을 자동으로 쓴다 → 커밋에 섞지 않는다.

## M1.5 · 시전 감각 (2026-11-05)

[M1.5 작업 분할](../systems/m15-plan) · [투사체 · 시전 감각](../systems/spell-feel).

- **A 전달 방식 · 네트워크**: Bolt · Stream · Cone 세 가지, 조준은 TargetData 로 서버에 보내 시차 보정, 시전자 화면엔 가짜 볼트로 즉시 반응.
- **B 조준**: 소프트 락온 · 고정 락온(R17 — 호그와트 레거시 참고), 대상 인터페이스 `IGrimTargetable`(`Source/Grimoire/Targeting/`).
- **C 연출 · 동작**: GameplayCue 로 시전 · 줄기 · 명중 연출, 마법별 연출 묶음 `VS_*`(VisualSet), 왼손 그리모어(책장 넘김). 연출은 **상태를 복제하고 클라이언트가 만든다**(RPC 로 뿌리지 않음).
- **Z 줌인 조준**(R24): 어깨 카메라 보간 · `State.Aiming`. 줌 중 마법은 유도 없이 겨눈 지점으로(`d4d592c`).
- **함정 — 유니티 빌드 이름 충돌**: 다른 .cpp 의 익명 네임스페이스 · 지역 이름이 한 번역 단위로 묶여 충돌. 이름 붙인 네임스페이스(`GrimMagicDebugLocal`)로. 이후 M3 에서도 두 번 더 났다(`d02b523` `d2616fd`) — 테스트 도우미 · 상수엔 파일별 고유 이름.
- **함정 — 확인 스크립트**: 시작 지점에 선 호스트가 클라이언트의 불씨를 몸으로 막았다 → `Grim.Magic.Place` 가 호스트를 비켜 세운다.

## M2 · 살아남기 (2026-11-05)

[M2 작업 분할](../systems/m2-plan) · [M2 후반 작업 분할](../systems/m2b-plan).

| 갈래 | 무엇을 · 어떻게 | 어디에 |
|---|---|---|
| W 날씨 | 기후 지역 볼륨 + 기후 표, 지역별 날씨 상태 머신(복제), 지붕 판정, 비 자극(양초가 꺼짐) · 낙뢰 | `Source/Grimoire/Environment/`, [환경](../systems/environment) |
| S 생존 | 체온 등급 · 저항, 피곤함, 질병 2종, 잠 · 전원 잠이면 밤 건너뛰기(`AGrimBed` · `UGrimSleepSubsystem`), 쓰러짐 · 부활 | `Source/Grimoire/Survival/`, [생존](../systems/survival) |
| G 마법 성장 · UI | 숙련도 D~S(데이터 표로 밸런스), 스킬북 사용, 서클 승급, HUD · 그리모어 화면 | `Source/Grimoire/Magic/`, `Source/Grimoire/UI/` |
| L 레벨 · 성장 트리 | 레벨 곡선 · 경험 출처, 속성 트리 4개 84노드 | `Source/Grimoire/Progression/`, [레벨 · 성장 트리](../systems/progression) |
| V 세이브 v1 | 세계 · 플레이어 세이브, 시스템마다 **세이브 참여 인터페이스**(`IGrimSaveParticipant`)를 구현해 붙는 방식, 모르는 레코드는 보존, CRC 봉투 + `.bak` 백업. PlayerId 는 PC 마다 만든 GUID(OSS Null id 가 실행마다 바뀌어서) | `Source/Grimoire/Save/`, [세이브](../systems/world-multiplayer) |
| K 조작 · 기본 공격 | 호그와트식 슬롯 바로 시전, 기본 공격(4연타 강타, 스킬북으로 배움) | `Source/Grimoire/Magic/`, `Source/Grimoire/Character/` |
| T 마법탑 튜토리얼 | 퀘스트 시스템(표 · 목표 · 진행 · 세이브), 탑의 스승 대화 · 서비스 · 승급 시험, `L_Tower` 그레이박스 | `Source/Grimoire/Quest/`, `Source/Grimoire/Factions/`, [튜토리얼](../systems/tutorial) |

- **설계 선택**: 서클은 최대 마나와 무관 — 레벨이 정한다(`3622f43`). 시작 마법은 개발 옵션 `-GrimStartingSpells` 로만(R31) — 그래서 확인 스크립트는 이 옵션을 붙인다.
- **통합 뒤 스펙 변경**(R29): 체온은 게이지 수렴 대신 등급 즉시 적용, 피곤함 탈진 · 강제 기절 없음 — 갈래 V 가 같은 파일을 고치는 중이라 합친 뒤 한 번에.
- **함정**: 탑 실내 피해는 `Zone.NoCombat` 로 막되 속성 자극은 그대로 둔다(`3b32cad`). 겹친 채 스폰된 스킬북이 마법 판정에 걸려 터짐 → 콜리전 종류 변경(`777bac0`).

## M3 · 생물과 친구 (2026-11-05)

[M3 작업 분할](../systems/m3-plan).

| 갈래 | 무엇을 · 어떻게 | 어디에 |
|---|---|---|
| C 생물 · 생태계 | 재칼로프 · 켈피 · 검은 개, 개체 굴림(암수 · 색 · 크기), AI, 서식 구역 스폰, 두 속성 상성(Q35 — 절반 반영) | `Source/Grimoire/Creatures/`, `Scripts/Creatures/make_creature_species.py`, [생물](../systems/creatures) |
| J 접속 · 메뉴 | `UGrimSessionSubsystem`(OSS Null — Steam 으로 바꿀 자리), `L_MainMenu`, IP 참가 · LAN 찾기 | `Source/Grimoire/Online/`, `Scripts/Online/` |
| I 아이템 · 요리 | 아이템 · 요리 표(CSV), 가방(FastArray · 세이브), 픽업 `SpawnPickup`, 요리는 **R16 그대로** — 날고기 + 불 자극 → `State.Cooked` | `Source/Grimoire/Items/` |
| W 지형 1차 | `L_OpenBeta`(World Partition, 2 × 4 km), 높이 지도 생성기(순수 C++) + JSON 레이아웃 + 맵 만들기 스크립트 | `Source/Grimoire/World/`, `Scripts/Terrain/`, [지형](../systems/terrain) |
| T 길들이기 · 탑승 | 파크툼 6권, 포획 확률 · 저항, 충성도 · 성격(Q27), 켈피 탑승(입력 중계 · 물 위 달리기 · 탄 채 시전), 세이브 `Creatures.Owned` | `Source/Grimoire/Creatures/`, `Scripts/Magic/make_pactum_spells.py` |

- **설계 선택**: 갈래 사이 결합을 줄이려 전리품은 **아이템 id + 개수** 로만 주고받았다(I 가 없으면 이벤트만). 튜토리얼 목표는 `UGrimQuestComponent::NotifyQuestEvent`(정적 함수) 이벤트 태그로. 개체는 `FGrimCreatureInstance` 하나(종 + 씨앗)로 다녀 복제 · 생태계 · 주인 세이브 · 보관함이 같은 구조체를 쓴다. AI 는 StateTree 대신 C++ 상태 기계 + 순수 판단 함수(`GrimCreatureMath::Decide`) — 바이너리 에셋 없이 자동화 테스트로 검증. 탑승은 Possess 가 아니라 **입력 중계** — ASC · 조준 · 가방 · 세이브가 「컨트롤러의 폰 = 플레이어」에 기대므로.
- **함정 — 상성 표**: `GetAffinity` 는 태그가 정확히 같을 때만 찾아 `Element.Dark.Shadow` · `Element.Earth.Plant` 가 부모 행을 못 받고 ×1. 연출 묶음 기본값도 정확히 같은 태그로만 골라 무속성 · 땅 · 그림자 파크툼이 불 이펙트로 보인다(Q113).
- **지형 1차 실패 → 다시**: 초원이 평지(기복 9 m — 경사 1~2°), 랜드마크가 탑 하나, POI 없음. 레벨 디자인 노트(린치 다섯 요소 · 랜드마크 · 삼각형 규칙 · POI 밀도)를 쓰고 2차(언덕 군 232 · 랜드마크 7 · POI 40, 설계 검사 자동 테스트), 이어 **마탑 구간 먼저 완성**(POI 21 · 한글 떠 있는 글씨 `AGrimDevLabel` · 공사 중 경계)([레벨 디자인 노트](./level-design)).
- **플레이 테스트 수정**: 마법이 트리거 상자에 맞아 터짐, 호그와트식 어깨 카메라, 달리기, 이동 · 소환 치트 `Grim.Go` · `Grim.Spawn` · 지연 명령 `Grim.At`, 이펙트를 Niagara Examples Pack 으로 임시 교체(`59dbe76` `21a414a`).

## 목록 · 아트 제안 (2026-11-06, 문서 위주)

- **생물 명단 61종**(8지역, 나무위키 상상의 동물 각색 + 창작) · 기능 분류 — [지역별 생물 명단](../spec/creature-roster). 콘셉트 프롬프트는 `art/creatures/make_prompts.py`.
- **스킬북 89권** — [스킬북 목록](../spec/skillbooks), 표는 `Scripts/Magic/skillbooks_md.py`.
- **인챈트 35종 · Ⅰ~Ⅲ 밸런스** — [인챈트 목록](../spec/enchants), `Content/Grimoire/Data/Enchants.csv` · `EnchantLevels.csv`.
- **1번 지역 콘셉트 40장 · 3D 모델 목록** — [콘셉트](../art/region1-concepts) · [모델 목록](../art/region1-assets). 함정: GPT 이미지를 동시에 여러 장 뽑으면 결과가 섞였다(`7adb2bb`) — 한 장씩 또는 잡마다 폴더를 나눈다.
- **미결 번호 나누기**: 병렬 세션이 미결 질문 번호를 미리 구간으로 나눠 가졌다(예: 인챈트 Q51~Q58) — 합칠 때 겹치지 않게.

## 자체 제작 에셋 파이프라인 (2026-11-06)

- **초원 바위 A**(`89644cf`): GPT 4면 시트 → Tripo 멀티뷰 → 블렌더 정리(실제 크기 · 바닥 기준점 · 충돌) → 언리얼 Nanite. 원본은 `art/env/rock-meadow-a/`, 결과는 `Content/Grimoire/Environment/`. 도구별 역할은 [아트 방향 — 리소스 만드는 길](../spec/art-direction#리소스-만드는-길-—-자체-제작-gpt-·-tripo-·-블렌더).

## 생물 보관함 (R38, 2026-11-06)

- 데리고 다니는 수 제한, 보관함 「쿠스토디아」(노말 스킬북 · 마법탑 상점), 가방 「생물」 탭, 거점 생물 상자, 세이브 · 2인 확인(`aaa1ba1`). 코드 `Source/Grimoire/Creatures/`(콘솔 `Grim.Taming.Storage*`), 확인 `Scripts/Creatures/net_check_storage.ps1`. 문서는 M4 통합 뒤 [생물 시스템](../systems/creatures)에 맞춘다.

## 커스터마이징 (R39, 2026-11-06)

- 외형 데이터는 **CSV 카탈로그**(`Content/Grimoire/Data/Appearance/`), 조립 `UGrimAppearanceComponent`, 플레이어 정본은 PlayerState 의 `UGrimPlayerAppearanceComponent`(세이브 · 서버 RPC), NPC 는 규칙 + 씨앗 생성, 최소 생성 화면 · 치트 `GrimCreator` · `GrimAppearanceRandom`([커스터마이징](../systems/customization)).
- **함정**: `UUserWidget` 안 지역 변수 이름 `Slot` 금지(`UWidget::Slot` 을 가려 C4458). 파이썬 일반 문자열의 `Scripts\Character\net_…` 은 `\n` 이 줄바꿈이 된다 — raw 문자열 · 슬래시로.

## 성능 (2026-11-06)

- 예산 · 규칙 26개([성능 예산](../systems/performance)), 되풀이 측정 `Scripts/Perf/measure.ps1`(콘솔 `Grim.Perf.Goto/Sample/Net`, [성능 측정 방법](./perf-measure)).
- **첫 최적화**: 하늘이 매 프레임 해를 조금씩 돌려 가상 섀도 맵 캐시가 통째로 무효 → 0.1도 단위로 돌림. GPU 그림자 깊이 2.0~2.6 → 0.4~0.6 ms. 개발 글씨 250 m 밖 숨김 · 위젯 틱 끔으로 프레임당 틱 83 → 39.
- **함정**: 같은 PC 의 다른 세션 게임 · 에디터가 숫자를 흔든다 — 절대값 말고 같은 조건 전후 비교. `UnrealEditor-Cmd` 를 **이름으로** 끄면(`Stop-Process -Name`) 다른 세션의 측정까지 죽는다 → `Start-Process -PassThru` 의 자기 Id 로만.

## 마법 이펙트 자체 제작 (2026-11-06)

- **길**: GPT 시안 → 파이썬 사양 → NiagaraToolsets(`unreal.ToolsetRegistry.execute_tool`, 헤드리스 커맨드렛)로 Niagara 조립 → 캡처 `Scripts/MagicFX/capture_spell.ps1` → 시안과 비교 `compare_fx.py`([이펙트 자체 제작](../systems/magic-fx-authoring)).
- **볼트**(사용자 참고 그림): 속성 7종 GPT 시안 · 레이어 분해 → GPT 텍스처 → `NS_Bolt_Cast/Trail/Impact` + 가산 머티리얼, 색은 `User.Color` 하나로 속성마다([투사체 · 시전 감각 — 볼트 이펙트](../systems/spell-feel)). 원본 `art/fx/`.
- **함정**: 같은 세션에서 Niagara 시스템을 지우고 같은 이름으로 만들면 저장 때 크래시 → 지우지 말고 비워 다시 채움. 컴파일 중 저장도 위험 → 기다리는 C++ 도우미. 자동 노출이 발광을 하얗게 민다 → 발광에 `EyeAdaptationInverse`. 볼트 앞당김은 원격 시전자의 지연이 시전 대기(`CastWindup`)를 넘을 때 넘은 만큼만(상한 0.15초, `Source/Grimoire/Magic/GrimSpellDelivery.cpp`) — 호스트 · 단독은 기다렸다 완드 끝에서 쏜다(Q110). `SpawnSystemAtLocation` 컴포넌트는 붙여도 안 따라온다.

## 이펙트 다시 — 발사 빛줄기 · 물줄기 · 미풍 (2026-11-07, Q85 · Q86)

- **무엇을**: 사용자가 Q85 · Q86 을 「제안대로」로 정했다. ① 볼트는 75~90 m/s 라 가까운 표적에서 꼬리가 거의 안 보였다 → 판정은 그대로, 보이는 볼트가 시작할 때 완드 끝부터 첫 0.1초 길(맞을 곳이 가까우면 거기까지, 원격 지연으로 당긴 틈은 다 덮음)을 한 번 긋는 `NS_Bolt_Launch`(`AGrimSpellProjectile::SpawnLaunchStreak`, 연출 묶음 칸 `LaunchStreak`). ② 물줄기 · 미풍을 볼트와 같은 빛 언어로 — 이어진 바늘 빛줄기 + 물방울 불티(`NS_Stream_*`, 줄기 큐가 판정 호의 발사 방향 · 속도 · 중력 · 막힌 곳을 넣음), 깃털 빛줄기 부채꼴(`NS_Cone_*`, 시전 큐가 원뿔 사거리 · 반각을 넣음). 구 펄스 · 구슬 · 원뿔 껍질은 껐다([투사체 · 시전 감각](../systems/spell-feel)).
- **어떻게**: `make_bolt_fx.py` 의 조립 방식을 **JSON 사양 파일**(`art/fx/specs/spell/*.json`) + 실행기 `Scripts/MagicFX/make_spell_fx.py`(`spell_fx_spec.py`)로 꺼내 다시 돌릴 수 있게 했다. 같은 실행기가 Effect Type `EFT_Grim_Spell` 을 만들어 마법 Niagara 14개에 붙인다(성능 규칙 24, BUG-006 마법 몫). 순수 계산 `GrimSpellDelivery::CalcLaunchStreakLength` · `CalcArcTimeToPoint` 는 자동화 테스트. 화면 QA 는 실제 시전 + 연출만 띄우는 새 개발 명령 `Grim.FX.Spawn`([QA — 이펙트](../qa/fx) 005 · 008 ~ 011).
- **함정**: Niagara 식에 난수가 없다 → 수명 난수를 0~1 로 펴서 각도 · 속력에 씀. `Particles.Position` 은 position 타입. Effect Type 중요도 처리기 클래스가 파이썬에 없다 → C++ 도우미. **`Grim.Cam` 고정 카메라는 조준도 바꾼다**(폰의 조준 = 카메라 시선) — 줄기 · 부채가 카메라 쪽으로만 나가 짧아 보였다 → 옆 · 위 시점은 `Grim.FX.Spawn` 으로. `capture_spell.ps1` 은 누름 시간(`-Hold`)이 없어 줄기를 못 찍었다 → 더함.

## 문서 맞추기 (2026-11-06)

- 빠른 합치기로 어긋난 시스템 · 스펙 문서를 코드에 맞추고, 구현된 절마다 「구현: `Source/...` 의 `UGrim...`」 로 코드 위치를 적었다. 앞으로는 [문서 갱신 체크리스트](./docs-checklist)와 검사 스크립트 `npm run docs:check` 로 지킨다.

## 사용자 답 Q42 · Q64 · Q71 → 코드 (2026-11-07)

- **Q42 생물 보관함**: 쿠스토디아 없이 처음부터 10칸(`StorageBaseCapacity`), 배우면 기존 설계 수치(8 · 12 · 16 + 노드 4)만큼 더한다 — 순수 계산 `GrimCreatureStorageMath::ComputeStorageCapacity`. 「안 배움」 거절(`NotLearned`)은 없애고 기본 칸이 차면 `StorageFullUnlearned`(「배우면 늘어난다」). 꽉 찼을 때 순서는 Q64 그대로(보관함 → 거점 상자 → 거절) — [생물 시스템](../systems/creatures#데리고-다니기-·-보관함-·-거점-상자-r38).
- **Q71 완드**: 「보이지 않는 기본 완드」를 없애고 장비 완드 칸이 정한다. 빈 칸 = 복제 루스 태그 `Status.WandMissing`(부러짐과 같은 길) → 마법 불가 · 기본 공격 입력은 주먹질, 손의 완드 액터는 서버가 숨긴다. 「기본 완드」 `Wand_Basic` 는 실제 장비(내구도 200, 기존 M4 수리 규칙), 처음 입고 시작(`StartingEquipment`), 마렌이 10 솔에 늘 판다. 옛 세이브(`Base.Equipment` v1)는 빈 칸에 기본 완드를 넣어 준다(v2) — [마법 — 완드](../systems/magic#완드-agrimwand).
- **Q64 실패 알림**: `GrimCastFailure` — GAS `NotifyAbilityFailed` → 소유 클라이언트는 복제 값으로 걸러 서버에 묻고(`Server_ReportCastFailed`), **서버가 판정**해 `Client_CastFailed` 로 소유 클라이언트 HUD 에 한 줄, `GameplayCue.Magic.Fizzle` 은 모두에게. 계약(파크툼) 실패 · 2서클 스킬북도 같은 길. 연출은 임시(엔진 기본 파티클 작게 + 비어 있는 몽타주 자리) — [마법 실패 알림](../systems/magic#마법-실패-알림-q64).
- **함정**: 예측 발동이 클라이언트에서 막히면 서버는 아무것도 모른다 — 「서버 판정」을 지키려고 클라이언트가 Spec 핸들만 보내 서버가 다시 판정하게 했다(연타 간격 같은 「알릴 것 없음」은 클라이언트가 미리 걸러 RPC 를 아낀다). C++ `UGameplayCueNotify_Static` 은 블루프린트 자식 에셋이 있어야 큐 관리자가 찾는다(`Scripts/Magic/make_fizzle_cue.py`).
- **확인**: 자동화 171/171, 2인 `Scripts/Magic/net_check_castfail.ps1` · `Scripts/Creatures/net_check_storage.ps1` fail=0, `Scripts/Base/qa_creature_box.ps1` PASS, 화면 `shoot_qa.ps1 -Set Fizzle`. 보류: 실패 몸짓(애니메이션 에셋 없음), 「모두 가득한 채 파크툼」 2인 시나리오.

## 수리 해금 Q121 → 코드 (2026-11-07)

- **무엇을**: 1서클 「수리」 볼트 하나가 장비 · 조각 · 거점을 다 고치던 것(Q70)을 나눴다. 레스티투오(같은 에셋 `DA_Spell_Repair` 를 2서클로)를 배우면 작업대 옆 「수리」 탭(`UGrimRepairTab`)이 열리고 볼트는 조각 · 시설 하나만 고친다(Q123 제안). 노말 스킬 레스티투오 막시마(`Skill:RestituoMaxima`, 4서클)는 거점 핵 「전체 수리」를 연다. 배우기 전엔 마법탑 스승의 대리 수리(재료 몫 × 5 솔) — [건축 — 수리](../systems/building#내구도-·-수리-·-인챈트-r18-·-r19).
- **어떻게**: 잠김은 Q64 실패 알림 길(`GrimCastFailure::Report` — `RepairLocked` · `BaseRepairLocked`, 연출 없음)로 서버가 판정해 소유 클라이언트에 알린다. 노말 스킬은 그 기능의 컴포넌트가 든다(막시마 = `UGrimBuilderComponent::LearnedSkills`, 세이브 `Base.Skills` v1). 장비 수리는 `UGrimEquipmentComponent::RepairWith`(재료 공급 함수) 하나로 작업대 · 대리 수리가 같은 규칙(수리 횟수 · 최대치 −10%)을 쓴다.
- **확인**: 자동화 173/173(새 `Grimoire.Base.Repair.Unlock` · `.NpcService`), 2인 `Scripts/Base/net_check_repair.ps1` fail=0.
- **함정**: 콘솔 `Grim.Base.Repair <작업대>` 는 볼트가 아니라 「수리」 탭이라 작업대 몸이 아니라 장비를 고친다 — 조각 하나 확인은 저장고 같은 다른 시설로.

