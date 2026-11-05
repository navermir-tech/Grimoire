# M3 작업 분할 — 생물과 친구

> 목표: **친구가 들어와 함께 사냥하고 탄다 — 진짜 지형 위에서.** ([로드맵](./roadmap) M3, 지형 계획 [지형은 언제 찍나](./roadmap#지형은-언제-찍나-2026-11-05))

## 묶음 1 — 병렬 네 갈래

| 갈래 | 소유 | 스펙 · 설계 | 다른 갈래와 만나는 곳 |
|---|---|---|---|
| **C. 생물 · 생태계** | 새 `Source/Grimoire/Creatures/**`, `Content/Grimoire/Creatures/`, `Content/Grimoire/Data/` 의 종 표, `Magic/GrimDamageExecution.*`(두 속성 상성 Q35 만) | [마법 생물](../spec/creatures) — M3 생물 3종 · 개체 차이 · 생태계 · 도감 룰, [생물 시스템](./creatures), [마법 — 두 속성](../spec/magic#두-속성-대상의-상성-r35) | 생물은 `IGrimTargetable`(소프트 락온)과 `IGrimElementalReceiver`(속성 상태)를 가진다. 전리품은 **아이템 id(FName) + 개수** 로만 떨군다 — `UGrimItemStatics::SpawnPickup`(갈래 I 가 만듦)이 없으면 이벤트만. 서식 구역은 볼륨/마커 액터(갈래 W 의 지형에 통합 때 놓는다). 튜토리얼 이벤트 `Creature.Killed.Small` · `Creature.Weakened.Small` 은 `UGrimQuestSubsystem::NotifyQuestEvent` 로 |
| **I. 아이템 · 가방 기초 · 식용 · 요리** | 새 `Source/Grimoire/Items/**`, `Content/Grimoire/Items/`, `Content/Grimoire/Data/` 의 아이템 · 요리 표 | [생존 — 먹기 · 요리](../spec/survival), [건축 시스템 — 인벤토리](./building#인벤토리-·-아이템) | 가방은 M4 에 완성 — 이번엔 **플레이어 가방(칸 수 고정) · 줍기 · 버리기 · 먹기 · 아궁이/모닥불 요리**까지. 요리는 R16: 날고기를 불 위에 두고 불 자극 → `State.Cooked`. 튜토리얼 이벤트 `Item.Obtained.Meat` · `Cooking.*` · `Food.Eaten` |
| **W. 지형 1차** | 새 맵 `Content/Grimoire/Maps/L_OpenBeta`(World Partition), `Scripts/Terrain/**`, `Content/Grimoire/Terrain/` | [세계 — 지역 1](../spec/world#지역과-크기-r34-—-q36-확정), [지형은 언제 찍나](./roadmap#지형은-언제-찍나-2026-11-05) | 2 × 4 km 높이 지도 생성(스크립트) → 랜드스케이프, 자동 머티리얼, PCG 식생. 마법탑 자리 · 연못 · 개울 · 계곡 · 생물 서식 구역 자리 · 기후 볼륨(Meadow). L_Tower 의 탑 그레이박스를 그 자리에 다시 짓는 스크립트(튜토리얼 갈래의 `build_tower_map.py` 를 재사용). 기본 맵은 **통합 때** 바꾼다 |
| **J. 접속 · 메뉴** | 새 `Source/Grimoire/Online/**`, `Content/Grimoire/Maps/L_MainMenu`, `UI/` 의 새 메뉴 위젯 파일 | [멀티플레이](../spec/multiplayer) | 메인 메뉴(새로 시작 · 이어하기(세이브 슬롯) · 서버 열기 · IP 참가 · 설정 · 나가기), 리슨 서버 열기 · IP 참가(OSS Null), 접속 실패 메시지, 세션 서브시스템 `UGrimSessionSubsystem`(Steam 으로 바꿀 자리). 세이브 슬롯 목록은 `UGrimSaveSubsystem` 을 읽기만 |

## 묶음 2 — 묶음 1 의 C 를 합친 뒤

| 갈래 | 내용 |
|---|---|
| **T. 길들이기 · 충성도 · 탑승** | 파크툼(무속성) · 파크툼 + 속성 스킬북 · 정의, 포획 확률(약해짐 · 먹이 · 속성 · 숙련도 추가 효과), 길들인 개체 기록 · 이름 짓기 · 따라오기 · 놓아주기, 충성도 · 성격(Q27), 켈피 탑승(물 위 달리기) · 탑승 중 시전 · 그리모어(Q25), 세이브 참여. 튜토리얼 `Creature.Tamed.Small` |

- 규칙은 앞 묶음들과 같다([M2 후반 작업 분할](./m2b-plan)): `Grimoire.Build.cs` 끝에 한 줄, 새 태그는 자기 폴더, `Config/DefaultGame.ini` 는 **파일 끝에** 자기 섹션만, `docs/spec/decisions.md` 에 줄을 더하지 않는다, 확인 스크립트는 `-GrimStartingSpells` 를 쓴다(시작 마법이 없으므로).
- 빌드 · 확인 규칙: [M1.5 작업 분할](./m15-plan#빌드-·-확인-규칙). 새 테스트는 `Grimoire.Creatures.*` · `Grimoire.Items.*` · `Grimoire.Terrain.*` · `Grimoire.Online.*` · `Grimoire.Taming.*` 아래.
