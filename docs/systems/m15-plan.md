# M1.5 작업 분할 — 시전 감각

> 목표: **2인 플레이에서 누르는 즉시 나가고, 셋이 구분되고, 락온된 양초를 켜고 · 키우고 · 끈다.** ([로드맵](./roadmap) M1.5, 설계 [투사체 · 시전 감각](./spell-feel), 조준 [마법 — 소프트 락온](./magic#소프트-락온-r17))
>
> M1 처럼 세 갈래를 병렬 세션으로 만들고, 마지막에 합쳐 2인으로 확인한다. 이펙트는 **엔진 기본 파티클로 임시** 작업한다(사용자 지시, 2026-11-05 — Niagara Examples Pack 은 나중에 바꿔 끼운다).

## 공통 기반 (main 에 있음 — 고치지 않는다)

| 파일 | 내용 |
|---|---|
| `Core/GrimTargetingTypes` | `EGrimAimMode`(Free · Soft · Locked · Zoom), `EGrimTargetKind`, `FGrimAimResult`(모드 · 대상 · 시선 시작 · 겨눈 지점), 인터페이스 `IGrimAimProvider::GetCurrentAim(MaxRange)` · `IGrimTargetable`(조준점 · 종류 · 대상 가능 여부), `UGrimTargetingStatics::FindAimProvider(Pawn)` · `FindTargetable(Actor)` |
| `Core/GrimSpellFXTypes` | 큐 태그 `GameplayCue.Spell.Cast` · `.Channel` · `.Impact`, 큐 파라미터 약속(아래), 연출 묶음 `UGrimSpellVisualSet`(섬광 · 꼬리 · 터짐 · 소리 3 · 흔들림 · 속성색, `UFXSystemAsset` 이라 Cascade · Niagara 둘 다) |
| M1 전부 | 캐릭터 · 완드 · 마법 3종 · 상호작용 · 환경 · 생존 · 치트 |

**큐 파라미터 약속** (`FGameplayCueParameters`): `SourceObject` = 그 마법의 `UGrimSpellDefinition`, `Location` · `Normal` = Cast 는 완드 끝 · 발사 방향, Impact 는 맞은 곳 · 법선, `AggregatedSourceTags` 에 `Element.*`, `Instigator` = 시전 폰.

## 갈래와 파일 소유

| 갈래 | 소유 (만들고 고치는 곳) | 다른 갈래와 만나는 곳 |
|---|---|---|
| **A. 전달 방식 · 네트워크** | `Source/Grimoire/Magic/**`(완드 제외 — 완드는 고쳐도 되지만 소켓 · Grip 은 그대로), `Content/Grimoire/Magic/`(DA_Spell_*), `Config/DefaultGame.ini` 의 `[/Script/Grimoire.GrimMagicSettings]` | 조준은 `UGrimTargetingStatics::FindAimProvider(Pawn)->GetCurrentAim(Range)` 로만 읽는다(없으면 지금처럼 카메라 정면). 연출은 `GrimCueTags` 로 큐를 실행만 한다. `UGrimSpellDefinition` 에 `VisualSet`(`UGrimSpellVisualSet`) · `HomingStrength` 필드 |
| **B. 조준** | 새 `Source/Grimoire/Targeting/**`, `Character/GrimPlayerController.*`, `Content/Grimoire/Input/` 의 새 조준 입력(IA_LockOn 등 · 별도 매핑 컨텍스트), `Interaction/GrimElementalObject.*` 에 `IGrimTargetable` 구현만 | 컨트롤러(또는 그 컴포넌트)가 `IGrimAimProvider` 구현. 카메라는 `AGrimPlayerCharacter::GetCameraBoom()` 을 런타임에 조절(캐릭터 파일은 안 고침). 대상 표식은 자기 위젯/디버그 그리기 |
| **C. 연출 · 동작** | 새 `Source/Grimoire/MagicFX/**`(큐 처리기 · `AGrimGrimoireBook`), `Character/GrimPlayerCharacter.*`(책 생성 · 부착 · 애님), `Content/Grimoire/MagicFX/`(VisualSet 3개 · 책 메시 · 애님 BP 레이어) | 큐 처리기가 `SourceObject` 의 `VisualSet` 을 읽는다(필드는 A 가 만든다 — C 는 큐 처리기에서 `UGrimSpellDefinition` 을 이름으로만 쓰지 말고 A 와 같은 필드 이름 `VisualSet` 을 쓴다). 책장 넘김은 Cast · Channel 큐에서 |

- 공용으로 손댈 수밖에 없는 것: `Grimoire.Build.cs` 는 목록 끝에 한 줄만. 새 태그는 자기 폴더의 `<시스템>Tags.*`. 맵 배치는 하지 않고 결과 보고에 적는다(통합 때 배치).
- A 와 C 가 `UGrimSpellDefinition::VisualSet` 을 같이 쓴다 → **A 가 만든다.** C 는 자기 브랜치에서 빌드가 되도록, 큐 처리기에서 `SourceObject` 를 `UObject` 로 받고 `FindFProperty<FObjectProperty>(..., "VisualSet")` 로 읽는다(합친 뒤 직접 접근으로 바꿔도 됨).

## 빌드 · 확인 규칙

- 빌드는 PowerShell `Build.bat`(CLAUDE.md). **에디터가 켜져 있으면 라이브 코딩이 빌드를 막는다** — 켜져 있으면 사용자에게 닫아 달라고 한다.
- 에셋은 헤드리스 에디터 파이썬(`-run=pythonscript`)으로 만든다(M1 과 같음). 다른 갈래가 같은 때 에디터를 띄우면 DDC · 셰이더 잠금으로 늦을 수 있다 — 기다리면 된다.
- 자동화 테스트 `Automation RunTests Grimoire` 가 모두 통과해야 끝. 새 테스트는 갈래 이름 아래(`Grimoire.Magic.Delivery.*` · `Grimoire.Targeting.*` · `Grimoire.MagicFX.*`).
- 2인 확인: `Scripts/Integration/net_check.ps1` 방식(헤드리스 리슨 서버 + 클라이언트 로그).
- 커밋은 자기 브랜치에만. main 에 합치는 것은 통합 세션(이 문서를 쓴 세션)이 한다.

## 통합 체크리스트

1. A → B → C 순으로 합치고 매번 빌드 · 테스트.
2. C 의 `FindFProperty` 를 `Definition->VisualSet` 으로 바꾼다.
3. 맵에 필요한 배치(각 갈래 보고) — `Scripts/Integration/place_m1_props.py` 를 넓힌다.
4. 2인 PIE 로 완료 기준 ①~⑤([투사체 · 시전 감각](./spell-feel#m1-5-범위-·-완료-기준)) → 사용자 플레이 테스트.
5. 줌인 조준(R24)은 조사가 끝나면 B 에 이어 붙인다.

## 통합 결과 (2026-11-05)

- A(`66e873e`) → B(`9c05c34`) → C(`178cb84`) 순으로 main 에 합쳤다. 문서 충돌(`decisions.md` · `magic.md` · `spell-feel.md`)은 양쪽 내용을 살려 풀었다.
- 유니티 빌드 이름 충돌 두 건을 고쳤다: `GrimMagicDebug.cpp` 의 익명 네임스페이스 → `GrimMagicDebugLocal`, MagicFX 테스트의 지역 변수 `SweepEnd` → `SweepEndPose`.
- `Scripts/Integration/link_visual_sets.py` 로 `DA_Spell_*` 의 `VisualSet` 에 `VS_Ember` · `VS_WaterJet` · `VS_Breeze` 를 이었다.
- 물줄기 그림(`AGrimCueNotify_SpellChannel`)이 판정과 같은 호(`UGrimSpellAbility::TraceStreamArc`)를 따라 구슬을 놓는다. VisualSet 찾기는 리플렉션(`FindFProperty`)을 그대로 둔다 — 동작이 같고, 정의 필드 이름이 바뀌어도 설정으로 맞출 수 있어서.
- `Grim.Magic.Place` 가 리슨 호스트의 폰을 사선 밖(옆 4m)으로 비켜 둔다 — 시작 지점에 선 호스트가 원격 클라이언트의 불씨를 몸으로 막아 확인이 실패했다(플레이어 몸에 맞으면 속성 자극이 들어가는 것은 의도대로).
- 확인: 빌드 성공, 자동화 테스트 40개 통과(M1 17 · 전달 7 · 조준 7 · 연출 9), `net_check_delivery.ps1` 2인 헤드리스 — 불씨 점화 → 먼 미풍 키움 → 가까운 미풍 끔 → 다시 점화 → 물줄기로 끄고 젖음, 서버 · 클라이언트 상태 같음.
- 아직 사람 눈으로 안 본 것: 이펙트 크기 · 색 · 셋의 구분(①②), 시전 동작 · 책 위치 · 책장 넘김(⑤), 고정 락온 카메라, 지연 보정(핑 0 이라 헤드리스에서 안 돎). → 사용자 2인 PIE 플레이 테스트.
- 남은 일: 줌인 조준(R24 — 조사 뒤), 이펙트 색 파라미터(엔진 템플릿에 `User.Color` 가 없어 지금은 속성색 펄스가 색을 맡음 — Niagara Examples Pack 으로 바꿀 때), 정식 시전 몽타주(에디터 작업).

