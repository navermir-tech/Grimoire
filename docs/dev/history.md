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
| 11-07 | 서클 세 축 Q122 (코드) | 서클 표 `Circles.csv` — 격 차이 위력 · 마나, 슬롯 세트 · 장비 등급 해금, 숙련도 승급 자격, 봉인된 책, 그리모어 「서클」 탭 | 브랜치 `claude/zen-goldwasser-c0e153` |
| 11-08 | 사용자 답 Q102 · Q116~Q120 (코드) | 생성 화면 정면 카메라(얼굴 · 전신 · 끌어서 돌림), 탈진 = 마법만 차단 · 지친 걸음 · 기절 삭제, 길들이기 서클 문턱 → 팰월드식 9제곱 확률 곡선(`CalcCircleScale`) · 파크툼 벤투스 1서클, 기본 공격 연타 창 = 마지막 발부터 1초 · 슬롯 마법이 끊지 않음 | 브랜치 `claude/answers-q116-032c8c`(메인이 합침) |
| 11-09 | 물 재질 Q126 | Water Materials 팩(CC BY) `M_River` 인스턴스로 개울 · 연못 물 표면 교체, 크레딧 문서, 호 길이 UV 연속화 | [물 재질 Q126](#물-재질-q126-2026-11-09) |
| 11-18 | 마법탑 구역 아트 시범 (Q96) | 탑 외관 · 1층 · 앞마당을 자체 제작 에셋으로 — 블렌더 모듈 키트 43 · 절차 텍스처 21 · `M_GrimEnv_Master` + 인스턴스 15, 그레이박스 · 팩은 숨김(충돌 유지), 풀 제외 볼륨(BUG-010) | [마법탑 구역 아트 시범](../art/tower-art) · 브랜치 `claude/tower-artstyle-3be908` |
| 11-09 | 초원 꾸밈 기획 (문서) | 소품을 마법 반응으로 — 요소 32 · 장면 8 · 분포 규칙 · 1차 11(Q150~Q157), 다른 게임 조사 근거 | [초원 꾸밈](../spec/meadow-dressing) |
| 11-18 | 밤 화면 Q101 | 해가 진 뒤 자동 노출 하한(−5.5 → −2 EV100) · 채도 빼기, 달빛 최저 높이 30°, 불씨 섬광 · 터짐 빛 — 달빛 아래 실루엣, 낮 · 노을 · 탑 안 그대로(BUG-008) | [밤 화면 Q101](#밤-화면-q101-—-달빛-아래-실루엣-2026-11-18) |
| 11-18 | 마법 · 생존 결정 맞추기 (코드) | 발사 빛줄기는 원격 지연 당김에만(Q110) · 연출 묶음 부모 태그로 · 무속성 · 땅 · 어둠 볼트 묶음 + 연기 가닥 · 돌가루(Q113 · Q87) · 열병 + 모자람 3 이상 5분 질병(Q115) · 지침 = 시전 시간 · 「MISS」(Q132) · 막시마 · 전설 7서클 규칙(Q133 · Q134) · 「포만」 · 쿠스토디아 막시마 줄 삭제(Q53 · Q61) | [마법 · 생존 결정 맞추기](#마법-·-생존-결정-맞추기-2026-11-18) |
| 11-18 | 사용자 답 Q46~Q110 (문서) | 전설도 길들일 수 있다(Q48) · 피곤함 · 배고픔은 인챈트로 못 막고 커피 · 에너지 드링크로(Q53) · 인챈트 칸 · 단계는 데이터에(Q51) · 묶음 알리는 법(Q54) · 보관은 크기 무관(Q61) · 천둥새도 잡는다(Q162) · 마법탑 아트풍 시범(Q96) · Q85 빛줄기는 원격만(Q110) 등, 새 미결 Q160 · Q161 · Q163 · Q164 | [사용자 답 Q46~Q110](#사용자-답-q46-q110-→-문서-2026-11-18) |
| 11-18 | 사용자 답 Q111~Q150 (문서) | 낙뢰는 빛 자극 하나(Q111) · 밤 건너뛰기 비율은 서버 옵션 기본 전원(Q114) · 열병 추가 · 그늘 더위 −1 없음(Q115) · 피곤함 「지침」 = 시전 시간 · 맞은 뒤 빗나감 MISS(Q132) — 시전 속도와 쿨다운 용어 정리 · 레스티투오 하나씩 수리(Q123) · 「개인만 무료」 받아 둘 14 개(Q128) · 크레딧은 설정 안 탭(Q137) · **지도 · 미니맵 스펙 새로**(Q130), 새 미결 Q165~Q169 | [사용자 답 Q111~Q150](#사용자-답-q111-q150-→-문서-2026-11-18) |
| 11-18 | 초원 꾸밈 1차 묶음 (Q150) | 반응 소품 공통 액터 · 채집 셀 관리자 · 도감 임시 목록 · F 대상 약속, 요소 11 을 `L_OpenBeta` 마탑 구간에 배치 스크립트로(소품 140 · 채집점 80 · 꽃 524), `EFT_Grim_Ambient` | 브랜치 `claude/meadow-dressing-1-e6c026` |
| 11-18 | 구역 이름 · 크레딧 탭 (Q130 · Q137, 코드) | 구역에 처음 들어서면 화면 위쪽에 이름 + 한 줄(젤다 · 엘든 링식, 본 구역은 플레이어 세이브), 레이아웃 `Sections` 에 큰 구역 경계 · 게임 문구, 설정 → 「크레딧 확인」 탭(정본 `Credits.json` → 문서 표 · `CREDITS.txt`) | [구역 이름 · 크레딧](#구역-이름-·-크레딧-탭-q130-·-q137-2026-11-18) |
| 11-19 | 합치기 묶음 통합 확인 | 탑 아트 시범 · 초원 꾸밈 1차 · 결정 맞추기(밤 화면) · 구역 이름 · 크레딧 탭을 한 브랜치에 모아 빌드 · 자동화 209 · 2인 · 성능(`merge-1119`) 확인. 시작 자리를 못 써 땅 밑에 생기던 것(BUG-013 — 탑 아트 메시 충돌이 켜져 몸통이 PlayerStart 를 덮음)을 고치고 `check_openbeta` 에 시작 자리 확인을 더함 | [QA — 성능](../qa/perf) · [BUG-013](../qa/bugs) · 브랜치 `claude/meadow-dressing-1-e6c026` |

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

## 서클 세 축 Q122 → 코드 (2026-11-07)

- **무엇을**: 사용자가 「서클 성장의 이점이 확실해야 한다」며 Q122 를 정했다 — 스킬 = 넓이 · 숙련도 = 깊이 · 서클 = 높이. 서클이 주는 것을 ① 수치 보정 ② 해금으로, 승급 자격을 숙련도로 바꿨다([성장 — 세 축의 역할](../spec/progression#세-축의-역할-q122), [마법 — 서클](../systems/magic#서클)).
- **어떻게**: 서클 1~10 의 보정 · 해금 · 승급 자격을 **표 하나**(`Content/Grimoire/Data/Circles.csv`, `FGrimCircleRow` · 순수 계산 `FGrimCircleMath` — `Source/Grimoire/Magic/GrimCircleTypes.h`)에 두고, 이미 있던 자리에 꽂았다: 격 차이 위력은 `MakeDamageSpec`(서버), 마나 효율은 `GetCasterMultipliers`(Cost GE · 판정 · 그리모어 표시가 함께), 슬롯 세트는 마법서 `GetUnlockedSlotSets`(자동 슬롯 · 배치 · 세트 돌리기 · HUD), 장비 등급은 `Equipment.csv` 의 새 `Tier` 칸과 `EquipFromBag`. 승급은 예전 「배운 마법 수 · 등급 합」(ini)을 없애고 등급별 마법 수(누적) + 지금 서클 마법 하나 이상, 모자라면 Q64 실패 알림 길(`PromotionGrade` · `PromotionCurrentCircle`, 글만)로 서버가 까닭을 보낸다. 두 승급 시험이 자격을 보도록 `RequireCircleConditions = True`. 서클이 모자란 책은 「봉인된 책」(`SealedSpells`, 세이브 v2)으로 받아 두고 승급 때 저절로 읽는다. 그리모어에 「서클」 탭(지금 · 다음 서클 보상, 자격, 봉인된 책).
- **미연결**: 각인 최고 단계 · 동시 유지 · 출입 ID · 4서클 이상 시험 과제는 그 시스템이 없어 표 · 조회 함수만. 생물 · 성장 트리 큰 노드 서클 값은 원래 표가 정본이라 다시 적지 않았다.
- **확인**: 자동화(새 `Grimoire.Magic.Circle.*` 다섯), 2인 `Scripts/Magic/net_check_circle.ps1`(봉인 · 거절 까닭 · 승급 · 격 차이를 서버가 정함), 튜토리얼 `check_tutorial.ps1`(자격 모자람 → 숙련도 채움 → 시험), 화면 `shoot_qa.ps1 -Set Circle` — [QA — 마법](../qa/magic#서클-보상-·-승급-자격-q122) · [QA — 튜토리얼](../qa/tutorial).
- **함정**: 1서클 슬롯 세트가 하나가 되면서 「세트 2」를 쓰던 테스트 · 2서클 책 알림을 보던 2인 스크립트(`net_check_castfail.ps1` — 이제 `Sealed`)를 함께 고쳐야 했다. 새 파일로 유니티 묶음이 바뀌자 `GrimPlayerSaveParticipants.h` 의 전방 선언만 있던 `APlayerState` 가 `TWeakObjectPtr` 대입에서 깨졌다 → 헤더에 include.

## 레벨 아트 1차 — Fab 팩 시험 적용 (2026-11-08)

- **무엇을**: 2026-11-07 받은 Fab 무료 팩으로 `L_OpenBeta` 시작 구역(마탑 구간 약 1 km²)을 입혔다 — 지형 재질(StyleHex 지형 텍스처, 맵 전체) · 랜드스케이프 풀 · 꽃(StyleHex) · 생성기 나무 · 덤불 · 바위 자리를 StyleHex 메시로 · 길가 돌 · 쓰러진 나무 · 큰 바위 · 먼 하늘 손그림 구름 카드. 저장소에 올린 팩 파일 116 개 202 MB(StyleHex 127.5 · BlackCherry 71.6 · 구름 3.2). 무엇을 어디에는 [레벨 디자인 노트 「8」](./level-design#_8-레벨-아트-1차-—-fab-팩-시험-적용-2026-11-08).
- **어떻게**: `Scripts/Terrain/dress_openbeta.py`(+ `dress_assets.py`) — 만든 맵 위에 덧입히는 헤드리스 스크립트. World Partition 액터를 모두 올리고, 지난번 것(태그 `LevelDress` · `FT_Dress_*`)을 걷고, 생성기 식생 자리를 그대로 써서(같은 시드 = 같은 자리) 구역 안만 팩 메시로 바꾼다. 우리 에셋은 `Content/Grimoire/Terrain/Dress/` 에만 만들고 팩은 참조만. 쓰는 팩 파일만 올리는 `Scripts/Terrain/add_pack_assets.ps1`(+ `collect_pack_deps.py` — 에셋 레지스트리 의존을 끝까지 따라감) — [마켓 · Fab 에셋 팩](./market-assets#스크립트로-2026-11-08).
- **확인**: 전 · 후 스크린숏(측정 자리 가운데 시작 구역 5 + Q124 · Q125 비교 3) · 다시 돌려도 같은 결과(외부 액터 수) · 성능 전체 측정 · 자동화 · 맵 점검 — [QA — 레벨 아트](../qa/level-art).
- **함정**:
  - 커맨드렛 저장은 **지운 액터의 외부 패키지 파일을 지우지 않는다** — 다시 돌릴 때마다 손 소품이 쌓였다(1519 → 1557 → 1595). 스크립트가 저장 뒤 그 파일을 직접 지운다.
  - **연 맵의 랜드스케이프 재질을 바꿔 저장하면 풀 지도 높이가 틀어진다**(탑 평지 · 언덕 꼭대기 풀이 묻히고 골 위 하늘에 풀이 뜸). 저장(`PreSave`)이 늘 풀 지도를 굽기 때문 — 텍스처 밉 강제 · 스트리밍 끄기 · 에디터 창 저장 · 재질 상태 바꾸기 · 시작 때 `EvictAll` 모두 안 됐다. 처음부터 새 재질로 만든 랜드스케이프는 맞다 → `build_openbeta.py` 가 `M_Terrain_Dress` 로 만들고 `dress_openbeta.py` 는 랜드스케이프를 건드리지 않는다. 쿡 빌드는 `grass.GrassMap.UseRuntimeGeneration=1`(`Config/DefaultEngine.ini`)로 런타임에 만든다. 그래서 이 브랜치는 맵을 다시 만들어 외부 액터 파일 이름이 모두 바뀌었다.
  - `GrassDensity` 단위는 **10 m × 10 m(100 m²) 당** 개수다 — 1 m² 당으로 넣었다가 풀이 100 배 성겼다.
  - 쓰는 팩 파일 모으기는 머티리얼 부모까지 따라가야 한다 — 스캔 그루터기 하나가 부모 `Universal_Mat` 의 기본 텍스처(다른 스캔 4K) 때문에 134 MB 가 되어 뺐다.
  - Megaplants 나무는 스켈레탈 메시 + Nanite 어셈블리라 `r.Nanite.Foliage`(읽기 전용 프로젝트 설정)가 꺼져 있으면 줄기만 보인다 — 놓지 않았다(Q125).
  - 이 PC 의 Bash 도구는 히어독 안 `\\` 를 하나로 줄인다 — 백슬래시가 든 문서 편집은 Write 도구로 만든 파이썬 파일로.

## 레벨 디자인 4차 — 전망대 · 개울 · 어귀 · 능선 · 돌담 (2026-11-09)

- **무엇을**: 3차를 에디터에서 본 사용자 피드백 전부 — 전망대 · 언덕 위 POI 터(20 곳, 봉화 언덕 18 m)와 정상 오솔길, 끊기지 않는 넓은 개울 · 연못 어귀(깔때기, 물 높이 일치), 벼랑 띠 · 톱니 그림자 제거(못 오르는 칸 3.27 → 0.93 %, 의도한 벼랑 밖 0.07 %), 돌담을 이유 있는 7 줄로(약 1,650 → 약 380 m) 납작한 단으로 다시 쌓기.
- **어떻게**: 생성기(`GrimTerrainGenerator.cpp` — `Pads` · `TalusAngleDeg` · `MaxFlankDeg` · `MinDepth` · `BedRatio` · `MouthLength` · `MaxGradeDeg`), 물 표면(`SampleNaturalWater` · `BuildWaterSurfaceMesh` — 어귀 UV 연장 · 흐름 정도), 레이아웃(`openbeta_layout.json` — `Pads` · `StoneWalls` · `IntentionalCliffs`), `Scripts/Terrain/ld4_walls.py`, 검사 `Scripts/Terrain/check_terrain.ps1` · `check_walk.ps1` · `check_walls.py`. 무엇을 왜: [레벨 디자인 「11」](./level-design#_11-레벨-디자인-4차-—-전망대-·-개울-·-어귀-·-능선-·-돌담-2026-11-09).
- **확인**: [QA — 레벨 아트](../qa/level-art) 「레벨 디자인 4차」.
- **함정**: 터 둘레 경사 조건이 비탈과 거의 평행하면 산 전체를 메운다(높이를 평균 쪽으로, 경사를 비탈보다 가파르게). 터는 개울 골 뒤 · 길 앞이어야 한다(골이 터를 깎고, 길은 터까지 올라와야). 경사 완화 뒤 넓어진 지맥이 안부 시선을 막을 수 있어 설계 검사를 함께 본다. `.ps1` 에 한글 문자열을 쓰면 UTF-8 BOM 이 있어야 Windows PowerShell 5 이 읽는다. `build_openbeta.py` 가 `new_level 실패`(맵 파일은 지워졌는데 `Intermediate/CachedAssetRegistry*` 에 남음)로 멈추면 맵 세 파일을 `git checkout` + `git lfs checkout` 으로 되돌리고 캐시를 지운 뒤 다시 돌린다.

## 레벨 디자인 3차 — 능선 · 개울 · 샘 · 돌담 · 채움 (2026-11-08)

- **무엇을**: 사용자 피드백(개울이 인위적 · 다리 비뚦 · 연못과 끊김 · 샘 · 짐수레가 언덕 꼭대기 · 빈 느낌 · 돌담이 뜸 · 능선이 고분) 전부. 능선 29 줄(날카로운 단면 · 지맥 · 호그백), 자연 개울 2 + 연못과 한 메시인 물 표면, 곡선 교점 다리 3, 샘 2, 돌담 27 줄(돌 4,525), 길섶 짐수레 2, 작은 소품 묶음 약 280 곳 · 큰 콘셉트 추가 6 곳.
- **어떻게**: 생성기 C++(`GrimTerrainGenerator.cpp` — `bSharpCrest` · `bNatural` · `FGrimTerrainBridge` · 물 표면 자료, `UGrimTerrainEditorLibrary::BuildWaterSurfaceMesh`), 레이아웃(`openbeta_layout.json`), `Scripts/Terrain/ld3_dress.py`(돌담을 실제로 쌓고 샘 · 짐수레 · 채움) · `check_walls.py`(뜬 돌 검사), `dress_assets.py`(이끼 · 물 · 동심원 재질). 무엇을 왜: [레벨 디자인 「10」](./level-design#_10-레벨-디자인-3차-—-능선-·-개울-·-샘-·-돌담-·-채움-2026-11-08), 만드는 법: [지형](../systems/terrain).
- **확인**: [QA — 레벨 아트](../qa/level-art) 「레벨 디자인 3차」 — 스크린숏 33 장(다섯 회차), 뜬 돌 0, 자동화 185/185, `openbeta` · `tutorial`, 성능 측정.
- **함정**:
  - 메인 체크아웃에 에디터가 떠 있으면 워크트리 `Build.bat` 이 Live Coding 뮤텍스로 실패 → `-NoHotReloadFromIDE`(`docs/dev/setup.md`).
  - UE 파이썬의 bool 속성은 `b` 가 빠진다(`bSurfaceMesh` → `.surface_mesh`).
  - `SM_Stone_*` 피벗은 메시 **가운데**다 — 바운드로 바닥을 재서 앉혀야 한다. 커맨드렛 `unreal.log` 는 `Saved/Logs` 에 간다.
  - 있는 스태틱 메시를 다시 `Build` 하면 비동기 RenderData ensure → 지우고 새로 만든다.
  - `new_level` 이 지운 맵이 「이미 있다」로 실패할 때가 있다 → `build_openbeta.py` 가 한 번 더 지우고 다시 훑는다.

## 레벨 디자인 2차 — 능선 · 자연 분포 · 구간 구도 (2026-11-08)

- **무엇을**: 사용자 피드백(「너무 규칙적 — 랜덤성 · 구릉과 능선, 콘셉트대로 구간 디테일, 하늘에 섹션 글씨, 큰 나무 자리는 큐브로 쪼갠 목업, 레퍼런스를 많이 보고 차용」)대로 `L_OpenBeta` 를 고쳤다 — 마탑 구간 땅을 **능선 7 줄(안부) + 뒤튼 능선형 노이즈 + 물방울 침식 + 결 맞춘 언덕**으로, 식생을 **포아송 원판 · 숲 무늬 · 무리 · 숲 안 덩이 · 등성이 / 골 · 물가**로(맵 전체 StyleHex, 잎 색 · 나무 끝 컬 480 m), 구간마다 콘셉트 구도(큰길 돌담 · 돌 표지 · 등불, 능선 꼭대기 바위 · 외딴 나무, 풍차 밭 · 곳간, 봉화대, 연못 징검돌 · 섬, 부유석 디딤돌, 고목 둥치), **하늘 구역 이름 20**, 거대 고목 **큐브 목업**.
- **어떻게**: 생성기 C++(`Source/Grimoire/World/GrimTerrainGenerator.cpp` — `Ridgelines` · 구역 `Ridged*` · `Erosion*` · 언덕 군 방향 · 식생 자연 분포, `GetFoliageInstances` 로 종 값), `AGrimDevLabel` 의 구역 이름 모드(`bSectionLabel`, 콘솔 `Grim.DevLabels.Sections`), 레이아웃 `Scripts/Terrain/openbeta_layout.json`, 새 `Scripts/Terrain/section_dress.py` · `preview_openbeta.py` · `overlay_foliage.py`, 고친 `dress_openbeta.py` · `dress_assets.py` · `landmark_greybox.py` · `shoot_openbeta.ps1`(`-Set ld2`). 레퍼런스 · 차용은 [레벨 디자인 「9」](./level-design#_9-레벨-디자인-2차-—-능선-·-자연-분포-·-구간-구도-2026-11-08).
- **확인**: [QA — 레벨 아트](../qa/level-art) 「레벨 디자인 2차」 — 스크린숏(눈높이 · 하늘, 같은 자리 전 · 후), 설계 검사, 자동화 테스트, `openbeta` · `tutorial`, 성능 측정.
- **함정**:
  - 워크트리의 LFS 파일이 포인터로만 있으면(`Invalid value for PACKAGE_FILE_TAG`) 맵 · 팩 에셋이 안 열린다 — 워크트리에서 `git lfs pull` 먼저.
  - 파이썬 커맨드렛의 `unreal.log` 는 표준 출력으로 오지 않는다 — `Saved/Logs/Grimoire.log`(지난 실행은 `Grimoire-backup-*.log`)를 본다.
  - 포아송 원판을 고른 밀도에 쓰면 숲이 **과수원처럼** 고른 간격이 된다 — 숲 안에도 덩이 노이즈(`ForestPatchScale`)로 몇 그루씩 모이고 빈터가 생겨야 자연스럽다.
  - 나무 끝 컬 300 m 는 눈높이에서도 먼 언덕을 민둥산으로 만든다 — 480 m(스트리밍 512 m 안). 더 먼 나무는 HLOD 가 필요하다(Q129).
  - 구역 순서: 높이가 같은 두 구역이 겹치면 뒤 구역이 「검사 영역」을 가져간다 — 마탑 구간을 목초지 뒤로 옮겨 목초지 평균 높이에 구간 능선이 섞이지 않게 했다(높이는 그대로).

## 「미결 질문」 표 → 카드 (2026-11-08)

- **무엇을**: 사용자 피드백(「표가 길어져 가로 스크롤이 불편하고, 줄이면 줄바꿈이 잦다」)대로 [미결 질문](../spec/decisions#미결-질문)을 카드 목록으로 보여 준다 — 번호 · 상태 뱃지 · 질문 · 후보 · 결정, 미결 · 답변됨 · 전체 필터(처음은 미결), Q 번호 앵커(`#q48`), 검색이 카드 단위로 찾음.
- **어떻게**: 여러 세션이 표에 줄을 더하므로 원본 파이프 표는 그대로 두고 빌드 단계에서 바꾼다 — markdown-it 규칙 `docs/.vitepress/questionCards.ts` + 커스텀 테마 `docs/.vitepress/theme/QuestionBoard.vue`. 「결정 기록」 표는 그대로. 쓰는 법은 [문서 사이트 · 배포](./docs-site#미결-질문-카드).
- **확인**: [QA — 문서 사이트](../qa/docs-site) — 빌드 결과를 1440 · 375 폭, 라이트 · 다크로.
- **함정**:
  - VitePress 검색창 Enter(`router.go`)는 `hashchange` 를 안 내므로 필터를 풀려면 `router.onAfterRouteChange` 도 걸어야 한다.
  - 검색 색인은 `<hN>…<a href="#…"></a></hN>` 이 한 줄일 때만 절로 나눈다 — 카드 제목 줄에 줄바꿈을 넣으면 번호 검색이 페이지 맨 위로 간다.
  - 한글 머리글 `id` 는 NFD(자모 분리)다 — `getElementById('미결-질문')` 처럼 NFC 문자열로는 못 찾는다.

## 마법 · 생존 결정 맞추기 (2026-11-18)

- **무엇을**: 사용자 답(2026-11-18)과 어긋난 마법 · 생존 코드 · 데이터를 맞췄다. 브랜치 `claude/align-decisions-1c6ee9`(밤 화면 Q101 브랜치 위).
  - **Q110** 볼트 발사 빛줄기는 원격 시전자의 지연을 당긴 볼트에만, 완드 끝부터 볼트 자리(틈)까지 — 혼자 · 호스트는 긋지 않는다. 당긴 시간을 투사체 `LaunchAdvanceSeconds`(처음 한 번 복제)로 보내고, 연출 묶음의 `LaunchStreakSeconds` 는 지웠다([투사체 · 시전 감각 — 발사 빛줄기](../systems/spell-feel)).
  - **Q113 · Q87** 연출 묶음 기본값을 부모 태그로 거슬러 찾고(`GrimMagicFXMath::FindByTagOrParent`), 볼트 · 포물선은 `DefaultBoltVisualSets` 먼저 — 무속성 `VS_Arcane` · 땅 `VS_Earth`(덧그림 돌가루 `NS_Bolt_Dust`) · 어둠 `VS_Dark`(덧그림 연기 가닥 `NS_Bolt_Smoke`), 모르면 무속성. 투사체도 같은 고르기를 써서 파크툼 볼트가 바늘 빛줄기로 난다. 연출 묶음 칸 `TrailAccent` 를 더했다.
  - **Q115** 체온 질병을 R29 대로 — 모자람 3 이상 5분 → 감기 · **열병**(새 `Status.Sick.Fever`, 감기와 대칭 제안: 300초 · 체온 게이지 +1.5 ℃ · 피곤함 ×1.5). 옛 「젖은 채 추위 60초마다 35 %」는 지웠다. 세이브 `Survival.Vitals` v2(열병 · 더위 시계, v1 도 읽음). 그늘 더위 −1 은 넣지 않았다([생존 — 질병](../systems/survival#질병)).
  - **Q132** 지침(피곤함 50~79, `Status.Tired`): 시전 시간 0.25초 이상인 마법만 시전 시간 ×1.3(지금 마법은 모두 즉발이라 체감 없음, 쿨다운 그대로), 서버가 생물 · 플레이어에게 맞은 뒤 10 → 30 % 로 빗나감을 굴려 피해 · 상태 · 숙련도를 막고 시전자 화면에 「MISS」(`AGrimMissText`)([생존 — 피곤함](../systems/survival#피곤함)).
  - **Q133 · Q134** 데이터 규칙: 파크툼 막시마(`GiantPactSpells`)는 거대 개체에 ×0.7 없이 속성 일치 +0.25, 전설 요구 서클은 모두 7(`LegendaryRequiredCircle`) — 전설 종 · 막시마 정의는 아직 없다([생물 — 길들이기](../systems/creatures)).
  - **Q53 · Q61** `Enchants.csv` 의 `Satiety`(포만), `Skillbooks.csv` 의 `Skillbook_Custodia_Maxima` 줄을 지우고 스킬북 표를 다시 뽑았다(90권). 레스티투오 설명에 「하나씩 수리」(Q123)를 넣었다.
  - **Q123** 은 코드가 이미 그 규칙(Q121 때)이라 테스트만 더했다. **Q114** `SleepSkipRatio` 기본 1 을 테스트로 확인.
- **어떻게**: 순수 계산은 `FGrimSurvivalModel`(`AccumulateExposureSickness` · `ComputeTiredMissChance` · `ComputeTiredCastTime`) · `GrimSpellDelivery::CalcLaunchStreakLength` · `GrimTamingMath::ResolveRequiredCircle` 로 떼어 자동화 테스트. 새 Niagara 는 사양 파일 `art/fx/specs/spell/bolt_accent.json` → `make_spell_fx.py`(`GRIM_FX_SPEC=bolt_accent`), 묶음은 `make_magicfx_assets.py`(`GRIM_FX_STEPS=visual_sets`). 화면은 `Grim.FX.Bolt … <당김초>`(새 인자) · `Scripts/Survival/capture_tired_fever.ps1`(새).
- **확인**: 자동화 195/195(새 `Grimoire.MagicFX.VisualSet.Resolve` · `Grimoire.Survival.Tired` · `Grimoire.Magic.Tired.Miss` · `Grimoire.Taming.GiantPact` · `Grimoire.Base.Repair.Piece`, 고친 `Grimoire.Magic.Delivery.LaunchStreak` · `Grimoire.Survival.Disease` · `.Sleep` · `.Effects` · `Grimoire.MagicFX.Assets`), 화면 [QA — 이펙트](../qa/fx) 012 · 014, [QA — 생존](../qa/survival) 004 · 037 · 038.
- **함정**:
  - 투사체의 발사 빛줄기를 `BeginPlay` 에서만 그으면 서버(호스트 화면)는 `CatchUp` 전이라 틈이 늘 0 이다 — 서버 사본은 당긴 직후에 긋는다.
  - 새 워크트리는 LFS 파일이 포인터로만 있다 — 헤드리스 스크립트가 「Invalid value for PACKAGE_FILE_TAG」로 에셋을 못 연다. `git lfs checkout`(공유 저장소에서 꺼냄, 1.9 GB)부터.
  - NiagaraToolsets 의 Fountain 템플릿 이미터에는 `ScaleSpriteSize` 모듈이 없다 — 끄기를 시키면 설정 실패로 센다.
  - 「시전 속도」 보정(완드 · 추위 · 성장 트리)은 코드가 여전히 쿨다운을 나눈다(Q166 미결) — 지침은 그것과 따로 시전 시간만 건드린다.

## 밤 화면 Q101 — 달빛 아래 실루엣 (2026-11-18)

- **무엇을**: 사용자 답(Q101 — 「호그와트 레거시 밤 정도로」)대로 밤을 「달빛 아래 땅 · 나무 · 캐릭터 윤곽이 보이고 색이 많이 빠진」 화면으로, 모닥불 · 불 마법 곁은 확실히 밝게. 낮 · 노을 · 등불 켠 탑 안은 그대로([BUG-008](../qa/bugs) 고침) — [환경 「밤 화면」](../systems/environment#밤-화면-q101-·-bug-008-2026-11-18).
- **어떻게**: 하늘 컨트롤러에 무한 범위 후처리 `NightLook` 하나. 해가 진 뒤에만(밤 섞임 `GetNightBlend` — 해가 지평선에서 약 10° 아래로 내려가는 동안 0 → 1) 자동 노출 **하한**을 해넘이 −5.5 → 밤 −2 EV100 으로 올리고 채도(전체 0.85 · 어두운 영역 0.4)를 뺀다. 하한이라 모닥불처럼 밝은 곳은 손대지 않고, 카메라가 실내(지붕 + 둘레 8 방향 중 6 이 막힘)면 뺀다. 달빛은 최저 높이 30°(`GetMoonDirection`). 불씨 연출에 빛만 내는 펄스(섬광 10 cd · 터짐 25 cd). 값은 `Config/DefaultGame.ini` 의 환경 설정 · `Scripts/MagicFX/make_magicfx_assets.py`.
- **확인**: [QA — 생존 「밤 화면」](../qa/survival#밤-화면-q101) — 같은 자리 · 같은 카메라 전후 그림 11쌍(`Scripts/Environment/shoot_night.ps1`), 자동화 `Grimoire.Environment.Sky.NightBlend`, 렌더 측정 낮 · 밤 전후.
- **함정**:
  - 노출 하한을 엔진 후처리 가중치에 맡기면 −10 에서 선형으로 오른다 — 어스름 초반엔 거의 묶이지 않아 막 진 뒤(EV100 −6.5) 장면을 자동 노출이 한낮처럼 끌어올렸다. 가중치는 0/1 로만 쓰고 값을 직접 섞는다.
  - 밤 섞임을 「1 − 낮 섞임」(지평선에서 절반)으로 하면 달이 지는 해와 같이 켜져 노을이 하얗게 뜬다 — 해가 진 뒤부터 섞는다.
  - 달을 늘 해의 맞은편에 두면 해가 막 진 때 달빛이 땅을 스치기만 해서, 노출을 묶은 뒤엔 해질녘이 한밤보다 훨씬 캄캄했다(평균 밝기 2.5 대 42).
  - 노출 값은 `ShowFlag.VisualizeHDR 1` 로 그림에 찍어 읽었다(장면 평균 EV100 · 하한).
  - 등불 켠 탑 홀도 장면 노출이 밤 하한보다 조금 낮아 처음엔 어두워졌다(154 → 138) — 하한만으로는 실내를 못 지킨다. 실내 판정으로 뺐다.
  - 이 프로젝트 해는 10 lux 뿐이라 모닥불 120 cd 는 낮에도 해보다 밝다 — 불빛 세기를 올릴 때 낮 그림도 본다.

## 물 재질 Q126 (2026-11-09)

- **무엇을**: 사용자 결정(「Water Materials 팩 쓸래 … 출처 표기하고 쓰자」)대로 개울 · 합류 연못의 물 표면을 팩 재질로 바꿨다. 다른 CC BY 에셋은 아직 안 쓴다. 게임 안 크레딧 화면은 [Q137](../spec/decisions#미결-질문).
- **어떻게**: 팩의 `M_River` 인스턴스 `MI_Dress_Water`(값은 `Scripts/Terrain/dress_assets.py` `WATER_SCALARS` · `WATER_COLORS`), `Scripts/Terrain/apply_water_material.py` 가 물 표면 메시를 다시 굽고 메시 · 맵 액터에 입힌다(맵 전체를 다시 만들지 않는다). 메시 UV0 의 호 길이를 꺾은점 번호 + 앞뒤 거리로 연속화(`FGrimTerrainGenerator::SampleNaturalWater`). 표기는 [크레딧](../art/credits), 쓰임은 [지형 「물 재질」](../systems/terrain#물-재질).
- **확인**: [QA — 레벨 아트](../qa/level-art) 「물 재질」 — 낮 · 황혼 · 밤 스크린숏, 자동화 187/187, `openbeta` · `tutorial`, 렌더 측정(`water-q126`).
- **함정**:
  - `M_River_Cheaper` 는 볼륨 라이팅이라 하늘을 반사하지 못한다(큐브맵 자체 발광뿐) — 낮에 위에서 보면 흙길이었다(큐브맵은 시각 변화를 따르지 않는다). 사진으로 먼저 가려야 한다. 재질 이름의 「Cheaper」만 보고 고르면 안 된다.
  - 물 표면 UV0 의 호 길이가 정수 꺾은점 번호라 큰 물결 텍스처에서 1 m 칸마다 끊겨 격자무늬가 보였다 — 자체 재질에서는 물결이 잘아서 눈에 띄지 않았다.
  - 워크트리 생성 직후에는 LFS 파일이 포인터뿐이라 맵이 안 열린다 — 네트워크 없이 `git lfs checkout`(이 워크트리는 3,340 개 1.9 GB).

## 가방 칸 더블클릭 사용 (2026-11-08)

- **무엇을**: 소지품(L) 가방 칸을 더블클릭하면 바로 쓴다(음식 먹기 · 스킬북 읽기 · 장비 입기). 재료 · 도구 · 씨앗 · 빈 칸은 아무 일 없다 — [건축 · 인벤토리 「화면」](../systems/building).
- **어떻게**: 사용 단추와 같은 `Server_UseItem` 을 부른다(새 RPC 없음, 판정 · 알림은 서버). `SButton` 은 더블클릭을 또 한 번의 누름으로 처리해 `OnClicked` 가 두 번 오므로, 칸 단추의 누름 간격을 `FGrimSlotDoubleClick` 이 본다(ini `SlotDoubleClickSeconds`, 기본 0.5초 = Windows 기본). 클라이언트는 `FGrimItemDefinition::IsUsable` 로 쓸 수 있는 종류만 보낸다 — 재료 더블클릭에 「쓸 수 없다」 알림이 뜨지 않게. 옮기기 중의 누름은 세지 않는다. 가방에는 끌어 놓기가 아직 없어(남은 것 「아이콘 · 끌어 놓기 UI」) 드래그와 겹치지 않는다.
- **확인**: 자동화 `Grimoire.Items.BagDoubleClick.*`(2), 2인 `Scripts/Items/net_check_bag_doubleclick.ps1`, 화면 `Scripts/QA/shoot_qa.ps1 -Set Bag` — [QA — 생존](../qa/survival#먹기-·-요리-r03-·-r16).
- **함정**: 합성 입력을 `RoutePointerDownEvent` · `RoutePointerMoveEvent` 에 경로와 함께 직접 넣으면 창 없는 실행(`-RenderOffscreen`)에서 엔진 배열 어서트로 죽었다. `ProcessMouseMoveEvent` + `OnMouseDown` · `OnMouseUp` · `OnMouseDoubleClick`(OS 메시지가 들어오는 함수)에 좌표를 주면 창 찾기 · 칸 맞히기까지 실제 입력과 같은 길로 돌고 죽지 않는다(`Grim.Items.BagMouse`). 화면이 그려지기 전이면 칸 크기가 0 이라 못 한다.

## 사용자 답 Q111~Q150 → 문서 (2026-11-18)

- **무엇을**: 사용자 답 두 번째 묶음(Q111~Q115 · Q123~Q125 · Q127~Q137 · Q140 · Q150 — 메인 세션이 모아 넘김)을 [결정 기록 · 미결 질문](../spec/decisions)과 스펙에 옮겼다. 코드 · 데이터는 고치지 않았다 — 코드 몫은 다른 세션 셋(마법 · 생존 결정 맞추기 / 구역 이름 · 크레딧 화면 / 초원 꾸밈 1차 묶음)이 한다.
- **바뀐 스펙**: ① 낙뢰는 빛 자극 하나(R16 규칙 4 — 코드가 이미 맞다). ② 그리모어 촤르륵은 슬롯 세트를 바꿀 때. ③ 밤 건너뛰기 비율은 서버 옵션으로 남기되 기본 전원(R29 의 「비율 옵션 없음」을 바꿈). ④ 열병을 넣고 그늘 더위 −1 은 없앤다. ⑤ 피곤함 50 이상 「지침」 = 시전 시간이 있는 마법만 느려지고 쿨다운은 그대로, 명중 저하는 조준 퍼짐이 아니라 맞은 뒤 확률 빗나감 + 「MISS」 숫자([생존 — 지침](../spec/survival#피곤함-—-지침과-탈진-q132)). ⑥ 레스티투오를 배우면 조각 · 시설 하나씩 수리. ⑦ 파크툼 막시마는 거대 개체 감점 없음 + 속성 일치 몫, 전설은 모두 요구 7. ⑧ 크레딧은 설정 화면 안 「크레딧 확인」 탭. ⑨ 구역에 처음 들어서면 이름을 띄우고(젤다 · 엘든 링식), 호그와트 레거시 같은 미니맵을 만든다 — 새 페이지 [지도 · 미니맵](../spec/map).
- **용어**: 시전 속도(시전 시간이 있는 마법의 시전 시간)와 쿨다운(재사용 대기)을 스펙 · 시스템 문서에서 갈라 적었다. 코드의 `CastSpeed`(완드) · `CastSpeedScale`(추위 · 성장 트리)은 이름과 달리 **쿨다운을 나눈다**(`UGrimSpellAbility::GetCasterMultipliers`) — 어느 쪽으로 맞출지는 미결 Q166.
- **에셋**: Megaplants 등 나무 · 풀은 나중에 채집 · 베기 대상이 되므로 시험 때 채집 가능한 개체로 바꿀 수 있는 구조인지 함께 본다(Q125). 「개인만 무료」 14 개를 받을 체크리스트로 정리했다([Fab 무료 에셋 — 개인만 무료 받아 둘 것](../art/fab-free-assets#개인만-무료-받아-둘-것-q128)) — 받기는 사용자가 한다.
- **새 미결**: Q165 지침 수치 · Q166 「시전 속도」 보정의 뜻 · Q167 미니맵 모양 · 돌아감 · Q168 지도 밝히기 · Q169 실내 · 던전 · 일행 · 적 표시.
- **남은 것(데이터 · 다른 세션 몫)**: `Content/Grimoire/Data/Skillbooks.csv` 레스티투오 설명의 「Q123 제안」 문구, 열병 · 감기 조건(Q115) · 연출 묶음 부모 태그(Q113) · 지침(Q132) 코드, 구역 이름 표시(Q130) · 크레딧 탭(Q137) 화면.
## 구역 이름 · 크레딧 탭 Q130 · Q137 (2026-11-18)

- **무엇을**: 사용자 답 Q130 「젤다랑 엘든링식으로」 · Q137 「설정 탭 안에 크레딧 확인 탭」. ① 구역(큰 구역 8 · 소구역 12)에 **처음** 들어서면 화면 위쪽 가운데에 이름 + 줄 + 한 줄 설명을 크게 띄웠다 사라진다. 다시 들어서면 큰 구역만 이름을 작게(3 분 안엔 다시 안 띄움), 소구역은 띄우지 않는다. 본 구역은 플레이어 세이브(`World.Zones`), 멀티는 플레이어마다. 미니맵 · 큰 지도가 쓸 조회(`UGrimZoneSubsystem` — 자리의 구역 · 정의 · 월드 경계 · 좌표 변환)를 뒀다. ② 메인 메뉴 · Esc 메뉴가 같이 쓰는 설정 칸에 「설정 · 크레딧 확인」 탭 — CC BY 표기 · 엔진 표기를 스크롤로. [구역 이름 표시 · 크레딧 탭](../systems/world-zones).
- **어떻게**: 정본은 레이아웃 `Sections`(하늘 글씨와 같은 데이터)에 큰 구역 `Polygon`(맞닿게 손으로) · 게임 문구 `Subtitle` 을 더하고, `Scripts/Terrain/export_zones.py` 가 게임용 `Content/Grimoire/Data/WorldZones.json` 을 만든다(테스트가 둘을 비교). 소구역은 자기 큰 구역 안의 보로노이. 서버가 0.5 초마다 폰 자리를 보고(1 초 머물러야 들어섬 — 경계 깜빡임 막기) 소유 클라이언트에 RPC, 컴포넌트는 퀘스트처럼 런타임에 PlayerState 에 붙인다(Core 안 고침). 크레딧은 `Content/Grimoire/Data/Credits.json` 하나를 정본으로 게임이 읽고, `Scripts/Docs/credits.mjs` 가 `docs/art/credits.md` 의 표 · 표기 문구와 출시본 `CREDITS.txt` 를 만든다(`docs:check` 가 어긋남을 잡는다).
- **확인**: 자동화 197/197(새 `Grimoire.World.Zones.*` 6 · `Grimoire.UI.Credits.*` 2), 헤드리스 `Scripts/QA/check_zones.ps1`(세이브 두 번 켜기) · `Scripts/QA/net_check_zones.ps1`(2인), 화면 `Scripts/QA/shoot_qa.ps1 -Set Zones` · `-Set MenuCredits` — [QA — 구역 이름 · 크레딧](../qa/world-zones).
- **함정**:
  - 레이아웃 `Zones` 의 `TowerSector` 다각형은 서쪽(x 650 ~ 860 m)에서 북쪽으로 y 1,000 m 를 넘는다 — 그 위에 「y 1,000 m 까지 목초지」를 그리면 다각형이 꼬여 겹친다. 경계는 서로 맞닿게(같은 꼭짓점 · 같은 변 위 점) 그리고, 테스트(`DataMatchesLayout` — 50 m 격자 빈틈 · 겹침)로 확인한다.
  - 레이아웃의 `Note` 에는 개발용 POI 번호(「S1 꺼진 화로 제단」)가 있어 게임 문구로 못 쓴다 — `Subtitle` 을 따로 둔다.
  - 이 워크트리도 처음엔 LFS 가 포인터뿐이라 입력 · 캐릭터 에셋을 쓰는 테스트 4 개가 실패하고 생물 보관 테스트에서 죽었다 — `git lfs checkout` 뒤 197/197.

## 사용자 답 Q46~Q110 → 문서 (2026-11-18)

- **무엇을**: 사용자가 문서 사이트의 미결 질문에 단 답(Q46 · Q48~Q57 · Q60~Q62 · Q87 · Q90 · Q95 · Q96 · Q100 · Q101 · Q110, 그리고 이 세션이 올린 Q162 — 메인 세션이 중간에 넘긴 추가 답 포함)를 [결정 기록 · 미결 질문](../spec/decisions)과 스펙에 옮겼다. 코드 · 데이터는 고치지 않았다.
- **바뀐 스펙**: ① 전설 생물도 길들이고 교배한다 — 천둥새 같은 신앙의 존재도 그대로(묘사만 정중하게), 「잡을 수 없는 시험 · 축복의 상대」 규칙 삭제([생물 명단 — 전설](../spec/creature-roster#전설-—-잡고-·-자랑하고-·-교배한다-q48)). ② 피곤함 · 배고픔은 인챈트 · 장신구로 못 막는다 — 인챈트 「포만」 삭제(35 → 34종), 피곤함은 잠 또는 거점에서 만든 커피 · 에너지 드링크([생존](../spec/survival#피곤함을-푸는-법-—-잠과-깨는-음료-q53)). ③ 인챈트 칸 · 단계는 데이터에만 두고, 함께 못 하는 묶음(견고 ↔ 자가 회복)을 알리는 법을 설계 · 확정, 장비 수명 기준(1등급 활동 2시간)과 그 재는 법 확정([인챈트](../spec/enchants)). ④ 생물 보관은 크기와 상관없음 — 쿠스토디아 막시마 없음. ⑤ 9 · 10서클은 잠정(나중에 더 넣음). ⑥ 마법탑 실외 · 실내 · 앞마당 아트풍 시범([아트 방향](../spec/art-direction#시범-—-마법탑-실외-·-실내-·-앞마당-q96)). ⑦ Q85 발사 빛줄기는 원격 · 고지연일 때 앞당긴 구간에만(Q110).
- **새 미결**: Q160 전설 길들이기 조건 · Q161 전설 재등장 · 번식 · Q163 들풀 반지 새 효과 · Q164 음료 이름 · 회복량 · 재료 · 시설.
- **코드에서 확인한 것**: 전설을 막는 코드는 없다(종 정의 `bTameable` 기본 참, 포획 곡선에 전설 몫이 이미 있음). 인챈트 칸 · 단계 숫자는 코드에 박혀 있지 않다(세는 코드가 아직 없음, 서클별 후보 단계는 `Circles.csv`). **발사 빛줄기는 결정(Q110)과 다르다** — `AGrimSpellProjectile::SpawnLaunchStreak` 가 혼자 · 호스트를 포함한 모든 볼트에 약 7.5 m 를 긋는다([투사체 · 시전 감각](../systems/spell-feel)).
- **남은 것(데이터 · 다른 세션 몫)**: `Content/Grimoire/Data/Enchants.csv` 의 `Satiety` 줄 · `Content/Grimoire/Data/Skillbooks.csv` 의 `Skillbook_Custodia_Maxima` 줄 지우기, 생물 명단 정본 `art/creatures/make_prompts.py` 의 전설 줄(「길들이기 불가」)을 명단 문서에 맞추기 — 문서의 전설 줄은 손으로 먼저 고쳤다. 발사 빛줄기를 원격 · 고지연일 때만 긋게 고치기(Q110 — 코드 세션).

## 초원 꾸밈 1차 묶음 (2026-11-18)

- **무엇을**: 사용자 답 Q150 「제안대로」 — 기획안의 1차 11 개(D01 바람솜꽃 · D02 마른 풀 · D03 목마른 들꽃 · D04 돌 등잔 · D10 곁가지 · 돌멩이 · D16 나비 · D20 식은 모닥불 · D21 흩날리는 책장 · D26 외딴 꽃나무 · D27 색 꽃밭 · D29 떠 있는 돌)를 만들어 마탑 구간에 놓았다. Q151~Q157 은 제안을 잠정값으로 표에 — [초원 꾸밈 구현](../systems/meadow-dressing).
- **어떻게**: 소품마다 클래스를 만들지 않고 `AGrimReactiveProp`(표 `ReactiveProps.csv` 한 행) + 속성 상태 단일 시스템(규칙표에 다섯 행). 줍는 것은 `AGrimHarvestCell`(256 m 셀 · ISM · FastArray). 도감은 PlayerState 에 런타임으로 붙는 `UGrimJournalComponent`, F 는 Core 의 `IGrimInteractable` 로 가방 위젯에 끼웠다. 에셋은 블렌더 스크립트(메시 8) · PIL(나비 · 솜) · 팩 재질 인스턴스. 배치는 `Scripts/Dressing/meadow_place.py`(시드 고정, 탑 둘레 260 m · 길 · 물 · POI · 전망대 터 · 바닷가 비움).
- **확인**: [QA — 초원 꾸밈](../qa/meadow) — 자동화 195/195(새 `Grimoire.Dressing.*` 6), 2인 `net_check_meadow.ps1` fail=0, 요소별 전 · 후 스크린숏, 성능 전 · 후.
- **함정**:
  - 파이썬 `StaticMesh.static_materials` 배열 원소를 고쳐 다시 넣어도 칸이 비었다 — `set_material(0, …)`.
  - `MaterialEditingLibrary.set_material_instance_*_parameter_value` 의 돌려준 값은 늘 False — 이름 목록으로 확인한다.
  - 팩 꽃 재질의 `Custom Color` 는 꽃잎 색을 바꾸지 못했다(`Desaturate Texture?` 를 켜도) — 색 꽃밭 · 시든 꽃 색은 [Q205](../spec/decisions#미결-질문).
  - 마탑 구간 다각형이 남쪽 바닷가까지 들어온다 — 첫 배치에서 꽃이 모래 위에 놓였다. 땅 높이 · 해안선으로 거른다.
  - 같은 PC 의 두 인스턴스는 같은 PlayerId(GameUserSettings.ini)를 읽는다 — 사람별 확인은 `-GrimPlayerId=` 를 따로 준다.
  - 이름 붙은 `Grim.Terrain.ViewFrom` 은 4 초 뒤에 찍는다, 그리고 `-ExecCmds` 지연은 맵 로딩 전부터 센다 — 반응 전 · 후 그림은 그만큼 띄운다.

