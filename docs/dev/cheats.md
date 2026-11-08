# 테스트 치트

개발 빌드(Editor · Development · DebugGame)에서만 쓰는 콘솔 명령(`~`). 출시 빌드(Shipping)에는 CheatManager 가 아예 만들어지지 않고(`UE_WITH_CHEAT_MANAGER` 0), 서버 RPC 본문과 `*Debug.cpp` 의 명령도 `#if !UE_BUILD_SHIPPING` 으로 빠진다. 코드는 `Source/Grimoire/Core/Cheats/` 와 시스템 폴더마다 `*Debug.cpp`.

- 출시 빌드에도 남는 것: 표 다시 읽기(`Grim.*.Reload` · `Grim.Interaction.ReloadReactions` · `Grim.Magic.ReloadProficiency` · `Grim.Magic.ReloadCircles`), 접속 · 메뉴(`Grim.Online.*`), UI(`Grim.UI.*`), 시간(`grim.Time.*`), 콘솔 변수 `Grim.Sky.SunRotationStep` · `Grim.Targeting.Debug` · `grim.Survival.ShowDebug`.
- 인자 끝의 `[지연초]` 는 점검 스크립트가 `-ExecCmds` 로 맵이 뜬 뒤 실행하려고 쓴다. 「(서버)」 는 단독 · 리슨 호스트에서, 「(로컬)」 은 누구나 자기 콘솔에서(서버 RPC 로 적용).
- 콘솔 명령은 대소문자를 가리지 않는다(`grim.Survival.*` · `grim.Time.*` 만 소문자로 등록돼 있다).

## 슈퍼치트 `GrimSuperCheat`

| 명령 | 효과 |
|---|---|
| `GrimSuperCheat 1` | 친 플레이어의 HP · 마나 · 배고픔을 최대로, 체온을 정상 체온으로 **고정**한다 |
| `GrimSuperCheat 0` | 끈다. 값은 최대(체온은 정상)에서 원래 흐름(배고픔 감소 · 체온 수렴 · 비용)으로 돌아간다 |

- 켜 있으면 화면 왼쪽 위 생존 디버그 표시 아래에 노란 `CHEAT Super ON` 이 뜬다.
- 마법 비용 · 피해 · 생존 틱이 와도 값이 그대로다. `Status.Hungry` · `Status.Starving` · `Status.Hypothermia` · `Status.Hyperthermia` 는 켜는 즉시 풀리고 다시 붙지 않는다(마나 회복도 멈추지 않는다).
- 최대치가 바뀌면(레벨업으로 MaxMana 증가 등) 따라간다.

### 동작 (GAS)

- 무한 지속 GE `UGrimSuperCheatEffect`(C++ CDO, 에셋 없음 — `Source/Grimoire/Core/Cheats/GrimSuperCheat.h`)가 **현재 값**을 Override 로 덮는다: Health ← `MaxHealth`, Mana ← `MaxMana`(대상 어트리뷰트, 스냅샷 없음), Hunger ← 설정 `MaxHunger`, BodyTemperature ← 설정 `NormalBodyTemperature`(SetByCaller). 태그 `Cheat.Super` 를 준다.
- 비용 · 생존 틱은 베이스 값을 바꾸므로 켜 있는 동안 베이스는 줄어든다. 끌 때 베이스를 최대(체온은 정상)로 채운 뒤 GE 를 뗀다 — 끈 순간 줄어든 값으로 떨어지지 않는다.
- 생존 컴포넌트는 현재 값으로 상태를 판정하므로 따로 고칠 것이 없다.

### 멀티(리슨 서버)에서

- 콘솔은 로컬에서 돌지만 `UGrimCheatManager::GrimSuperCheat`(`Source/Grimoire/Core/Cheats/GrimCheatManager.h`)는 `AGrimPlayerController::Server_SetSuperCheat` 로 **서버에서** 적용한다. 호스트도 클라이언트도 그냥 자기 콘솔(`~`)에 치면 되고, **친 사람에게만** 걸린다.
- 엔진 기본은 멀티 클라이언트에 CheatManager 를 만들지 않는다. `AGrimPlayerController::AddCheats` 가 개발 빌드에서 강제로 만들어 `EnableCheats` 를 따로 칠 필요가 없다.
- 결과는 그 클라이언트 화면에 `GrimSuperCheat ON/OFF` 메시지로, 서버 로그에 `LogGrimoire` 로 남는다.

## 이동 · 생물 · 지연 명령

플레이 테스트용. 서버(단독 · 리슨 호스트)에서 친다.

| 명령 | 효과 |
|---|---|
| `Grim.Go <이름 일부> [지연초]` | 그곳 4m 앞으로 순간이동(호스트면 모든 플레이어가 함께). 생물이 먼저 잡힌다 — `Grim.Go Jackalope` 는 서식 구역 한가운데가 아니라 **실제 무리** 곁으로 간다. 탑 안 대상(`Tut_*`)이면 그 층 바닥에 선다 |
| `Grim.Go.List [이름 일부]` | 갈 수 있는 곳 목록(화면 40개 · 전체는 로그). 생물 · 아직 안 나온 무리 · 표식(`Landmark.*` · `Poi.*` · `Habitat.*`) · 태그 액터(`Tut_` · `Npc_` · `Hunter_`) |
| `Grim.Spawn <종 이름 일부> [수] [거리m]` | 앞에 생물을 놓는다(기본 1마리 · 6m, 최대 20). 인자 없이 치면 종 목록. 예: `Grim.Spawn Jack 3`, `Grim.Spawn BlackDog` |
| `Grim.Creature.Mark [1/0]` | 생물 위치 표시 켜기 · 끄기. 나온 생물은 굵은 기둥(초록 순함 · 빨강 사나움), **아직 안 나온 무리는 노란 가는 기둥**. 화면 위에 가장 가까운 생물의 이름 · 거리 · 방향 |
| `Grim.Creature.Time <0~1> [지연초]` | 시각(0 자정 · 0.5 정오). 검은 개는 밤에만 나온다 |
| `Grim.At <초> <명령 …>` | 명령을 몇 초 뒤 실행 — 점검 스크립트가 `-ExecCmds` 로 맵이 뜬 뒤 · 시전 도중에 무엇을 할 때(`Grim.At 20 HighResShot 1280x720`) |
| `Grim.Cam <X> <Y> <Z> <피치> <요> [FOV]` | 그 자리 고정 카메라로 본다(캐릭터는 계속 조종 · 시전). 인자 없으면 캐릭터 카메라로. 이펙트를 옆에서 찍을 때 |
| `Grim.FX.Bolt <마법 정의> <X> <Y> <Z> <요> [피치] [터질 거리cm]` | 시전 없이 볼트 연출만 날린다(판정 없음, 섬광 · 발사 빛줄기 · 몸통 · 터짐). 예: `Grim.FX.Bolt DA_Spell_Ember 30 -1000 150 0 0 225` — `Scripts/MagicFX/capture_bolt.ps1` 가 `slomo` 와 함께 쓴다 |
| `Grim.FX.Spawn <마법 정의> <Muzzle\|Trail\|Impact\|Launch> <X> <Y> <Z> <요> [피치] [초] [이름=값 …]` | 시전 없이 연출 묶음의 Niagara 하나를 그 자리 · 방향(+Z)에 띄운다. 값은 사용자 변수(수 또는 `x,y,z`). 예: `Grim.FX.Spawn DA_Spell_Breeze Trail 30 -1000 120 0 0 0 ConeRange=1000 ConeHalfAngle=30` — 시전하면 조준 = 카메라라 `Grim.Cam` 옆 · 위 시점에서 쓸 수 없는 물줄기 · 미풍을 볼 때(`capture_bolt.ps1 -FxCmd`) |

생물은 서식 구역(수백 m)에 무리로 기록만 되어 있다가 **플레이어 60m 안**에 들어오면 나온다(설정 `Ecosystem.SpawnRadius`). 구역 한가운데에 서도 무리가 멀면 아무것도 안 보인다 — `Grim.Creature.Mark 1` 의 노란 기둥을 따라가거나 `Grim.Go <종>` 을 쓴다.

## 생물 데리고 다니기 · 보관함 · 거점 상자 (R38)

길들이기 명령(`Grim.Taming.*`)과 같은 꼴. 「로컬」 명령은 호스트 · 클라이언트 누구나 자기 콘솔에 치면 서버 RPC 로 적용된다.

| 명령 | 효과 |
|---|---|
| `Grim.Taming.StorageLearn [1/0] [지연초]` | (서버) 모든 플레이어가 생물 보관함을 배운다 · 잊는다. 스킬북으로 배우려면 `Grim.Items.Give Skillbook_CreatureStorage 1` → 가방(L)에서 사용(또는 `Grim.Items.Eat Skillbook_CreatureStorage`) |
| `Grim.Items.Give Solidus 200` | 솔 200 — 마법탑 마렌 「스킬북: 쿠스토디아 사기 (120 솔)」 시험 |
| `Grim.Taming.SpawnBox [거리cm] [지연초] [Host/Client]` | (서버) 그 플레이어 앞에 거점 생물 상자. 다가가 F 로 관리 화면 |
| `Grim.Taming.Storage <From> <To> [종 id\|*] [지연초]` | (로컬) 생물 옮기기 — 자리 `Party`(데리고 다님) · `Storage`(보관함) · `Box`(상자 안) · `Released`(풀어놓음). 예: `Grim.Taming.Storage Party Storage Jackalope` |
| `Grim.Taming.Box Open/Close [지연초]` | (로컬) 곁의 상자 관리 화면 |
| `Grim.Taming.StorageDump [지연초] [Local/Remote]` | 데리고 다님 · 보관함 · 상자 목록을 로그로(Remote = 서버가 본 원격 클라이언트) |
| `Grim.Taming.Grant [서클=3] [지연초]` | (서버) 모든 플레이어 서클을 올리고 파크툼을 가르친다. 서클이 오르면 데리고 다니는 수 · 보관함 칸이 오른다(1~2서클 3 · 8, 3서클 4 · 12, 5서클 5 · 16 — `[/Script/Grimoire.GrimCreatureStorageSettings]`) |

- 데리고 다니는 수가 가득일 때 파크툼이 맞으면 보관함 → 거점 상자 순으로 보내고, 둘 다 안 되면 계약이 걸리지 않는다(「더 데리고 다닐 수 없다」).
- 2인 확인: `powershell -File Scripts\Creatures\net_check_storage.ps1` → `NETCHECK_RESULT fail=0`.

## 길들이기 · 탑승

코드 `Source/Grimoire/Creatures/GrimTamingDebug.cpp`. 2인 확인 `Scripts/Creatures/net_check_taming.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Taming.Cast <마법 에셋 이름> <종 id\|*> [누르는 초] [지연초]` | (로컬) 가장 가까운 야생 생물에 시전(예: 파크툼) |
| `Grim.Taming.Act <Follow\|Wait\|Mount\|Feed\|Release\|Dismount\|Name> [종 id\|*] [이름] [지연초]` | (로컬) 내 생물에게 행동 |
| `Grim.Taming.Move <앞> <옆> <초> [지연초]` | (로컬) 이동 입력(탄 채 이동 시험) |
| `Grim.Taming.Loyalty <사건\|Set> [값] [종 id\|*] [지연초]` | (서버) 충성도 사건(`Fed` · `Starved` …) 또는 값 |
| `Grim.Taming.Place <종 id\|*> [거리cm] [지연초]` · `Grim.Taming.SpawnNearClient <종 id> [거리cm] [지연초]` | (서버) 원격 클라이언트를 야생 생물 앞에 · 원격 클라이언트 앞에 생물 |
| `Grim.Taming.AfterJoin <플레이어 수> <초> <명령 …>` | 플레이어가 그 수만큼 모이고 몇 초 뒤 명령 |
| `Grim.Taming.Dump [지연초]` | 이 프로세스에서 본 생물의 길들이기 상태 |
| `Grim.Taming.ForceRoll <0~1>` | (콘솔 변수) 포획 굴림 고정. 0 = 늘 성공, 0.999 = 거의 늘 실패, 음수 = 끔 |

## 생물 · 생태계 (서버)

코드 `Source/Grimoire/Creatures/GrimCreatureDebug.cpp`. 2인 확인 `Scripts/Creatures/net_check_creatures.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Creature.Spawn <종 id> [수] [지연초] [거리cm]` | 플레이어 앞에 생물(`Grim.Spawn` 의 스크립트용) |
| `Grim.Creature.Habitat <종 id> [상한=4] [지연초] [거리cm] [반크기cm]` | 플레이어 앞에 서식 구역 |
| `Grim.Creature.Damage <양> [지연초] [종 id\|*] [Element.*]` | 첫 플레이어가 쏜 마법 피해 |
| `Grim.Creature.Stimulus <Element.*> <세기> [지연초] [종 id\|*]` | 생물에 속성 자극 |
| `Grim.Creature.Kill [지연초] [종 id\|*]` | 첫 플레이어가 죽인 것으로(전리품 · 퀘스트 이벤트) |
| `Grim.Creature.Dump [지연초]` · `Grim.Creature.Eco [지연초]` | 본 생물 · 서식 구역 개체군을 로그로 |

## 마법 · 조작

코드 `Source/Grimoire/Magic/GrimMagicDebug.cpp` · `GrimMagicGrowthDebug.cpp` · `GrimSpellInputDebug.cpp`. 2인 확인 `Scripts/Integration/net_check_delivery.ps1` · `net_check_controls.ps1` · `check_growth.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Magic.Cast <마법 이름 일부> [대상 클래스\|None] [누르는 초] [지연초]` | (로컬) 대상 고정 조준으로 시전 |
| `Grim.Magic.Place <대상 클래스> <거리cm> [지연초]` | (서버) 원격 클라이언트 폰을 대상 앞에 두고 보게(호스트는 사선 밖으로 비킨다) |
| `Grim.Magic.DumpWands [지연초]` | 완드 부착 부모 · 소켓 · 손에서 떨어진 거리 |
| `Grim.Magic.CircleUp [강제 0\|1] [지연초]` | (로컬) 서클 승급(서클 표의 숙련도 자격 확인 — 모자라면 까닭 알림, 1 이면 무시) |
| `Grim.Magic.ReadSkillbook [지연초]` · `Grim.Magic.SpawnSkillbook <마법 이름 일부> [앞cm] [지연초]` | (로컬) 곁의 스킬북 읽기 · (서버) 플레이어마다 앞에 스킬북 |
| `Grim.Magic.AddXP <마법 이름 일부> <양> [지연초]` · `Grim.Magic.XPRate [배율]` | 숙련도 경험 더하기 · 서버 배율(`GlobalXPRate`) |
| `Grim.Magic.Proficiency [지연초]` | 서클 · 숙련도 · 승급 조건을 로그로 |
| `Grim.Magic.ReloadProficiency` | 숙련도 등급표 `ProficiencyGrades.csv` 다시 읽기 |
| `Grim.Magic.ReloadCircles` | 서클 표 `Circles.csv`(Q122 — 보정 · 해금 · 승급 자격) 다시 읽기 |
| `Grim.Input.Press <Basic\|Slot1~4\|Protego\|Dodge> [대상 클래스\|None] [누르는 초] [지연초] [횟수] [간격초]` | (로컬) 실제 입력과 같은 길로 누른다 |
| `Grim.Input.SlotSet <1부터> [지연초]` · `Grim.Input.Dump [지연초]` | 보이는 슬롯 세트 바꾸기 · 세트 · 입력 태그 · 연타 상태 로그 |

## 조준 (R17 · R24)

코드 `Source/Grimoire/Targeting/GrimTargetingDebug.cpp`.

| 명령 | 효과 |
|---|---|
| `Grim.Targeting.SoftLock <0\|1>` · `Grim.Targeting.Reference <Camera\|Movement>` | 소프트 락온 켜기(Q18) · 선정 기준 |
| `Grim.Targeting.Lock` | 고정 락온 토글(R3 · Caps Lock 과 같음) |
| `Grim.Targeting.Zoom <0\|1>` · `Grim.Targeting.ZoomToggle <0\|1>` | 줌인 조준 켜기 · 끄기, 방식(1 토글 · 0 누르는 동안) |
| `Grim.Targeting.Look <요> <피치>` | 시점 입력 한 번(고정 중 옆 대상 넘기기) |
| `Grim.Targeting.SpawnDummy [앞cm=800] [오른쪽cm=0] [Enemy\|Creature\|Object\|Ally]` | 로컬 폰 앞에 조준 더미(`AGrimTargetDummy`) |
| `Grim.Targeting.Dump` · `Grim.Targeting.DumpServer` · `Grim.Targeting.DumpAimingTags` | 로컬 조준 · 모든 컨트롤러 · `State.Aiming` 복제 로그 |
| `Grim.Targeting.Debug 1` | (콘솔 변수) 소프트 락온 후보 점수를 그린다 |

모든 명령 끝에 `[지연초]` 를 줄 수 있다(`Debug` 제외).

## 생존 · 시간

코드 `Source/Grimoire/Survival/GrimSurvivalDebug.cpp` · `Source/Grimoire/Environment/GrimTimeOfDaySubsystem.cpp`. 2인 확인 `Scripts/Survival/net_check_survival.ps1`.

| 명령 | 효과 |
|---|---|
| `grim.Survival.Sleep` · `grim.Survival.Wake` | 눕기(반경 안 침대가 있으면 그 침대 = 부활 지점) · 일어나기 |
| `grim.Survival.Down` · `grim.Survival.Revive` · `grim.Survival.Die` | 쓰러짐(`Status.Downed`) · 일으키기 · 바로 사망 → 마지막 침대(없으면 PlayerStart)에서 부활 |
| `grim.Survival.Fatigue <값>` · `grim.Survival.EatRaw` · `grim.Survival.Sick <0 배탈\|1 감기>` · `grim.Survival.Cure` | 피곤함 · 날것 먹기(배탈 굴림) · 질병 걸기 · 모두 낫기 |
| `grim.Survival.SpawnBed [1 = 침낭]` | (서버) 앞 1.5m 에 침대 |
| `grim.Survival.Resist <더위> <추위>` | 시험용 저항 버프(장비 M4 전). `0 0` 이면 뗀다 |
| `grim.Survival.Dump` · `grim.Survival.At <초> <명령 …>` | 생존 상태 로그 · 몇 초 뒤 생존 명령(명령은 앞 `grim.Survival.` 를 뺀 이름) |
| `grim.Survival.ShowDebug 0/1` | (콘솔 변수) 왼쪽 위 생존 · 시간 디버그 표시 |
| `grim.Time.Set <시 0~24>` · `grim.Time.DayLength <분>` | 시각 · 하루 길이 |

## 날씨 · 상호작용

코드 `Source/Grimoire/Environment/GrimWeatherDebug.cpp` · `Source/Grimoire/Interaction/GrimInteractionDebug.cpp`. 2인 확인 `Scripts/Environment/net_check_weather.ps1` · `Scripts/Integration/net_check.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Weather.Set <지역\|All> <날씨> [블렌드초] [지연초]` | (서버) 지역 날씨 — `Clear` · `Cloudy` · `Fog` · `Rain` · `Thunderstorm` · `Snow` · `Sandstorm`. 예: `Grim.Weather.Set Meadow Rain 0` |
| `Grim.Weather.Strike <지역> [지연초]` | (서버) 그 지역 플레이어 둘레에 낙뢰 하나 |
| `Grim.Weather.Dump [지연초]` · `Grim.Weather.Probe <X> <Y> <Z> [지연초]` | 지역 날씨 · 내 자리 / 그 자리의 기온 · 등급 · 비 · 지붕 로그 |
| `Grim.Interaction.Stimulus <Element> <세기> [지연초] [클래스 이름 일부]` | (서버) 모든 속성 사물에 자극 |
| `Grim.Interaction.Dump [지연초]` | 속성 사물 상태 · 불꽃 · 빛 로그 |
| `Grim.Interaction.ReloadReactions` | 반응 규칙표 `ElementalReactions.csv` 다시 읽기 |
| `Grim.Sky.SunRotationStep <도>` | (콘솔 변수) 해 · 달 빛을 돌리는 최소 각. 음수 = 액터 값, 0 = 매 프레임(성능 비교용) |

## 아이템 · 요리

코드 `Source/Grimoire/Items/GrimItemDebug.cpp`. 2인 확인 `Scripts/Items/net_check_items.ps1`. 아이템 id 는 `Content/Grimoire/Data/Items.csv`.

| 명령 | 효과 |
|---|---|
| `Grim.Items.Give <id> [수]` · `Grim.Items.Spawn <id> [수]` | 가방에 넣기 · 폰 앞 1.5m 에 픽업 |
| `Grim.Items.PickUp` · `Grim.Items.Use <칸>` · `Grim.Items.Eat <id>` | 가장 가까운 픽업 줍기(F) · 칸 쓰기(먹기 · 읽기) · 그 아이템 첫 칸 쓰기 |
| `Grim.Items.Drop <칸> [수]` · `Grim.Items.DropId <id> [수]` · `Grim.Items.Move <칸> <칸>` · `Grim.Items.Clear` | 버리기 · 옮기기(합치기 · 바꾸기) · 비우기 |
| `Grim.Items.SpawnFire` · `Grim.Items.SpawnPot` · `Grim.Items.Pot` · `Grim.Items.Fire [세기]` | 모닥불 · 냄비 놓기, 곁 냄비에 재료 넣기(F), 가장 가까운 픽업에 불 자극(요리) |
| `Grim.Items.Hunger <값>` | 배고픔을 값으로 |
| `Grim.Items.At <초> <하위 명령 …>` · `Grim.Items.Dump` | 몇 초 뒤 아이템 명령(`Give` · `Dump` …) · 가방 · 픽업 · 냄비 로그 |
| `Grim.Items.Reload` | 아이템 · 요리 표 다시 읽기 |

## 레벨 · 성장 트리 · UI

코드 `Source/Grimoire/Progression/GrimProgressionDebug.cpp` · `Source/Grimoire/UI/GrimUISubsystem.cpp`.

| 명령 | 효과 |
|---|---|
| `Grim.Level.AddXP <양> [출처: Debug\|Quest\|Hunt\|Craft\|Skillbook …] [지연초]` · `Grim.Level.Set <레벨> [지연초]` | 경험치 주기 · 레벨 정하기 |
| `Grim.Tree.Unlock <노드 id> [지연초]` · `Grim.Tree.Reset <트리 id> [지연초]` | 노드 찍기 · 트리 비우기(정화 의식, 비용 없이) |
| `Grim.Level.Dump [지연초]` | 레벨 · 경험 · 포인트 · 노드 · 보너스 어트리뷰트 로그 |
| `Grim.Level.Reload` · `Grim.Tree.Reload` | `LevelCurve.csv` · `SkillTreeNodes.csv` 다시 읽기 |
| `Grim.UI.HUD 0/1` · `Grim.UI.Grimoire [0\|1]` · `Grim.UI.GrimoireTab <0~3>` · `Grim.UI.SlotSet <세트>` · `Grim.UI.Level [0\|1]` | HUD 보이기 · 그리모어 화면 · 그리모어를 열고 탭 고르기(0 마법 · 1 성장 트리 · 2 도감 · 3 서클) · 단축 슬롯 세트 · 레벨 막대 단독 |

## 퀘스트 · 튜토리얼

코드 `Source/Grimoire/Quest/GrimQuestDebug.cpp`. 확인 `Scripts/Tutorial/check_tutorial.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Quest.Run <명령> [인자 …]` | 바로 — `Dump` · `Talk <NPC>` · `Service <NPC> <서비스>` · `Teleport <태그\|NPC> [거리]` · `Stimulus <태그> <Element.X> [세기]` · `Start <퀘스트>` · `Skip [퀘스트]` · `Fail <퀘스트>` · `Event <이벤트 ID> [수]` |
| `Grim.Quest.At <초> <명령> [인자 …]` | 위 명령을 몇 초 뒤 |
| `Grim.Quest.Dump` | 플레이어별 퀘스트 진행 로그 |
| `Grim.Quest.Reload` | 퀘스트 · NPC · 대사 표 다시 읽기 |

## 지형 · 개발 글씨

코드 `Source/Grimoire/World/GrimTerrainDebug.cpp` · `GrimDevLabel.cpp`. 확인 `Scripts/Terrain/check_openbeta.ps1` · `shoot_openbeta.ps1`.

| 명령 | 효과 |
|---|---|
| `Grim.Terrain.Teleport <X> <Y> [지연초]` | 로컬 플레이어를 그 XY 의 땅 위로 |
| `Grim.Terrain.ViewFrom <X> <Y> <Z> <Yaw> <Pitch> [지연초] [스크린숏 이름]` | 그 카메라로 보고, 이름을 주면 4초 뒤 `Saved/TerrainCheck/<이름>.png` |
| `Grim.Terrain.Walk <지연초> <걷는초> [Yaw]` · `Grim.Terrain.Probe [지연초]` | 걸려 보고 처음 · 끝 위치 로그 · 땅 높이 · 프록시 수 · 시간 · 날씨 로그 |
| `Grim.DevLabels 0/1` · `Grim.DevLabels.MaxDistance <cm>` | (콘솔 변수) 떠 있는 개발 글씨 보이기 · 숨기는 거리(0 = 액터 값 250 m) |

## 세이브 · 접속 · 성능

전체 표는 각 문서에 있다.

- 세이브 `Grim.Save.Now` · `Load` · `Dump` · `Delete` · `At` · `AtLogins` · `Quit` · `SetAttr` — [월드 · 멀티 — 세이브 콘솔](../systems/world-multiplayer#콘솔-서버-·-호스트-프로세스).
- 접속 · 메뉴 `Grim.Online.Host` · `Solo` · `Join` · `Cancel` · `FindLAN` · `JoinLAN` · `Leave` · `Quit` · `Name` · `Slots` · `Status` · `Invite` · `Pause` · `When` — [월드 · 멀티 — 콘솔](../systems/world-multiplayer#세션-·-접속-·-메뉴-m3-갈래-j).
- 성능 `Grim.Perf.Goto` · `Sample` · `Net` — [성능 측정 방법](./perf-measure#콘솔-명령-개발-빌드).

## 캐릭터 외형

[커스터마이징](../spec/customization) (R39) 확인용. 코드는 `Source/Grimoire/Appearance/`, 표는 `Content/Grimoire/Data/Appearance/*.csv`.

| 명령 | 어디서 | 효과 |
|---|---|---|
| `GrimCreator` | 누구나(자기 콘솔) | 캐릭터 생성 화면을 연다. 고르는 동안은 내 화면에만 보이고, 「확인」하면 서버가 검사해 모두에게 보인다. Esc · 「취소」 = 되돌림 |
| `GrimAppearanceRandom [씨앗]` | 누구나 | 친 사람 외형을 무작위 견습생 한 벌로(서버). 씨앗을 주면 늘 같은 한 벌 |
| `Grim.Appearance.Lineup <규칙> [수=5] [씨앗=1]` | 서버 | 첫 플레이어 5m 앞에 규칙대로 사람을 줄 세운다. 인자 없이 치면 규칙 목록. 예: `Grim.Appearance.Lineup Tower_Examiner 3` |
| `Grim.Appearance.Randomize [씨앗]` | 서버 | 모든 플레이어 외형을 무작위로(씨앗 + 순번) |
| `Grim.Appearance.Dump` | 아무 데나 | 외형마다 열쇠(플레이어 이름 · 규칙#씨앗) · 해시 · 조각 수 · 고리 수를 로그(`LogGrimAppearance`)에 |
| `Grim.Appearance.Reload` | 아무 데나 | 외형 CSV 를 다시 읽고 월드의 외형을 다시 조립한다 — 자리 표시 오프셋을 고치며 볼 때 |
| `Grim.Appearance.CreatorFocus <Face\|Body>` | 자기 화면 | 열린 생성 화면의 카메라를 얼굴 가까이 · 전신으로(칸을 누른 것과 같다 — Q102) |
| `Grim.Appearance.CreatorOrbit <도> [내려다보는 도]` | 자기 화면 | 열린 생성 화면의 카메라를 정면에서 돌린다(마우스 끌기와 같다). 예: `Grim.Appearance.CreatorOrbit 45 -5` |

- 규칙: `Player_Start`(플레이어 시작) · `Tower_Apprentice` · `Tower_Instructor` · `Tower_Librarian` · `Tower_Examiner`.
- 지금 옷 · 머리는 엔진 기본 도형 자리 표시다. 서클 고리는 스톨 오른쪽 띠의 작은 원판 — 서클이 오르면 하나씩 는다.
- 2인 복제 확인: `powershell -File Scripts\Character\net_check_appearance.ps1 [-Start 35]` → 끝에 `PASS` / `FAIL`. 다른 에디터가 여럿 떠 있어 맵이 늦게 뜨면 `-Start 90`.

## 테스트

`Grimoire.Cheat.Super`(`Source/Grimoire/Core/Cheats/Tests/GrimCheatTests.cpp`) — 켜면 최대로 고정(비용 · 감소 뒤에도), 상태 태그 해제, MaxHealth 변화 추종, 끄면 최대에서 감소 재개.

`Grimoire.Appearance.*`(`Source/Grimoire/Appearance/Tests/GrimAppearanceTests.cpp`) — 표 읽기, 저장 · 복제 직렬화 왕복, NPC 씨앗 결정성, 서버 검사(Sanitize), 조립(고리 수 · 미리보기 · 체형).

```powershell
# 에디터 없이 전체 자동화 테스트
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire; Quit" -unattended -nopause -nosplash -NullRHI -log
```
