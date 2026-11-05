# 테스트 치트

개발 빌드(Editor · Development · DebugGame)에서만 쓰는 콘솔 명령. 출시 빌드(Shipping)에는 CheatManager 가 아예 만들어지지 않고(`UE_WITH_CHEAT_MANAGER` 0), 서버 RPC 본문도 `#if !UE_BUILD_SHIPPING` 으로 빠진다. 코드는 `Source/Grimoire/Core/Cheats/`.

## 슈퍼치트 `GrimSuperCheat`

| 명령 | 효과 |
|---|---|
| `GrimSuperCheat 1` | 친 플레이어의 HP · 마나 · 배고픔을 최대로, 체온을 정상 체온으로 **고정**한다 |
| `GrimSuperCheat 0` | 끈다. 값은 최대(체온은 정상)에서 원래 흐름(배고픔 감소 · 체온 수렴 · 비용)으로 돌아간다 |

- 켜 있으면 화면 왼쪽 위 생존 디버그 표시 아래에 노란 `CHEAT Super ON` 이 뜬다.
- 마법 비용 · 피해 · 생존 틱이 와도 값이 그대로다. `Status.Hungry` · `Status.Starving` · `Status.Hypothermia` · `Status.Hyperthermia` 는 켜는 즉시 풀리고 다시 붙지 않는다(마나 회복도 멈추지 않는다).
- 최대치가 바뀌면(서클 상승으로 MaxMana 증가 등) 따라간다.

### 동작 (GAS)

- 무한 지속 GE `UGrimSuperCheatEffect`(C++ CDO, 에셋 없음)가 **현재 값**을 Override 로 덮는다: Health ← `MaxHealth`, Mana ← `MaxMana`(대상 어트리뷰트, 스냅샷 없음), Hunger ← 설정 `MaxHunger`, BodyTemperature ← 설정 `NormalBodyTemperature`(SetByCaller). 태그 `Cheat.Super` 를 준다.
- 비용 · 생존 틱은 베이스 값을 바꾸므로 켜 있는 동안 베이스는 줄어든다. 끌 때 베이스를 최대(체온은 정상)로 채운 뒤 GE 를 뗀다 — 끈 순간 줄어든 값으로 떨어지지 않는다.
- 생존 컴포넌트는 현재 값으로 상태를 판정하므로 따로 고칠 것이 없다.

### 멀티(리슨 서버)에서

- 콘솔은 로컬에서 돌지만 `UGrimCheatManager::GrimSuperCheat` 는 `AGrimPlayerController::Server_SetSuperCheat` 로 **서버에서** 적용한다. 호스트도 클라이언트도 그냥 자기 콘솔(`~`)에 치면 되고, **친 사람에게만** 걸린다.
- 엔진 기본은 멀티 클라이언트에 CheatManager 를 만들지 않는다. `AGrimPlayerController::AddCheats` 가 개발 빌드에서 강제로 만들어 `EnableCheats` 를 따로 칠 필요가 없다.
- 결과는 그 클라이언트 화면에 `GrimSuperCheat ON/OFF` 메시지로, 서버 로그에 `LogGrimoire` 로 남는다.

## 이동 · 생물 · 지연 명령

플레이 테스트용 콘솔 명령(`~`). 개발 빌드에서만 있고, 서버(단독 · 리슨 호스트)에서 친다.

| 명령 | 효과 |
|---|---|
| `Grim.Go <이름 일부> [지연초]` | 그곳 4m 앞으로 순간이동(호스트면 모든 플레이어가 함께). 생물이 먼저 잡힌다 — `Grim.Go Jackalope` 는 서식 구역 한가운데가 아니라 **실제 무리** 곁으로 간다. 탑 안 대상(`Tut_*`)이면 그 층 바닥에 선다 |
| `Grim.Go.List [이름 일부]` | 갈 수 있는 곳 목록(화면 40개 · 전체는 로그). 생물 · 아직 안 나온 무리 · 표식(`Landmark.*` · `Poi.*` · `Habitat.*`) · 태그 액터(`Tut_` · `Npc_` · `Hunter_`) |
| `Grim.Spawn <종 이름 일부> [수] [거리m]` | 앞에 생물을 놓는다(기본 1마리 · 6m, 최대 20). 인자 없이 치면 종 목록. 예: `Grim.Spawn Jack 3`, `Grim.Spawn BlackDog` |
| `Grim.Creature.Mark [1/0]` | 생물 위치 표시 켜기 · 끄기. 나온 생물은 굵은 기둥(초록 순함 · 빨강 사나움), **아직 안 나온 무리는 노란 가는 기둥**. 화면 위에 가장 가까운 생물의 이름 · 거리 · 방향 |
| `Grim.Creature.Time <0~1>` | 시각(0 자정 · 0.5 정오). 검은 개는 밤에만 나온다 |
| `Grim.At <초> <명령 …>` | 명령을 몇 초 뒤 실행 — 점검 스크립트가 `-ExecCmds` 로 맵이 뜬 뒤 · 시전 도중에 무엇을 할 때(`Grim.At 20 HighResShot 1280x720`) |
| `Grim.Cam <X> <Y> <Z> <피치> <요> [FOV]` | 그 자리 고정 카메라로 본다(캐릭터는 계속 조종 · 시전). 인자 없으면 캐릭터 카메라로. 이펙트를 옆에서 찍을 때 |
| `Grim.FX.Bolt <마법 정의> <X> <Y> <Z> <요> [피치] [터질 거리cm]` | 시전 없이 볼트 연출만 날린다(판정 없음, 섬광 · 몸통 · 터짐). 예: `Grim.FX.Bolt DA_Spell_Ember 30 -1000 150 0 0 225` — `Scripts/MagicFX/capture_bolt.ps1` 가 `slomo` 와 함께 쓴다 |

생물은 서식 구역(수백 m)에 무리로 기록만 되어 있다가 **플레이어 60m 안**에 들어오면 나온다(설정 `Ecosystem.SpawnRadius`). 구역 한가운데에 서도 무리가 멀면 아무것도 안 보인다 — `Grim.Creature.Mark 1` 의 노란 기둥을 따라가거나 `Grim.Go <종>` 을 쓴다.

## 생물 데리고 다니기 · 보관함 · 거점 상자 (R38)

길들이기 명령(`Grim.Taming.*`)과 같은 꼴 — 마지막 인자 지연(초)은 점검 스크립트가 맵이 뜬 뒤 실행하려고 쓴다. 「로컬 플레이어」 명령은 호스트 · 클라이언트 누구나 자기 콘솔에 치면 서버 RPC 로 적용된다.

| 명령 | 효과 |
|---|---|
| `Grim.Taming.StorageLearn [1/0]` | (서버) 모든 플레이어가 생물 보관함을 배운다 · 잊는다. 스킬북으로 배우려면 `Grim.Items.Give Skillbook_CreatureStorage 1` → 가방(L)에서 사용(또는 `Grim.Items.Eat Skillbook_CreatureStorage`) |
| `Grim.Items.Give Solidus 200` | 솔 200 — 마법탑 마렌 「스킬북: 쿠스토디아 사기 (120 솔)」 시험 |
| `Grim.Taming.SpawnBox [거리cm] [지연초] [Host/Client]` | (서버) 그 플레이어 앞에 거점 생물 상자. 다가가 F 로 관리 화면 |
| `Grim.Taming.Storage <From> <To> [종]` | (로컬) 생물 옮기기 — 자리 `Party`(데리고 다님) · `Storage`(보관함) · `Box`(상자 안) · `Released`(풀어놓음). 예: `Grim.Taming.Storage Party Storage Jackalope` |
| `Grim.Taming.Box Open/Close` | (로컬) 곁의 상자 관리 화면 |
| `Grim.Taming.StorageDump [지연초] [Local/Remote]` | 데리고 다님 · 보관함 · 상자 목록을 로그로(Remote = 서버가 본 원격 클라이언트) |
| `Grim.Taming.Grant <서클>` | 서클을 올리면 데리고 다니는 수 · 보관함 칸이 오른다(1~2서클 3 · 8, 3서클 4 · 12, 5서클 5 · 16) |

- 데리고 다니는 수가 가득일 때 파크툼이 맞으면 보관함 → 거점 상자 순으로 보내고, 둘 다 안 되면 계약이 걸리지 않는다(「더 데리고 다닐 수 없다」).
- 2인 확인: `powershell -File Scripts\Creatures\net_check_storage.ps1` → `NETCHECK_RESULT fail=0`.

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

- 규칙: `Player_Start`(플레이어 시작) · `Tower_Apprentice` · `Tower_Instructor` · `Tower_Librarian` · `Tower_Examiner`.
- 지금 옷 · 머리는 엔진 기본 도형 자리 표시다. 서클 고리는 스톨 오른쪽 띠의 작은 원판 — 서클이 오르면 하나씩 는다.
- 2인 복제 확인: `powershell -File Scripts\Character\net_check_appearance.ps1 [-Start 35]` → 끝에 `PASS` / `FAIL`. 다른 에디터가 여럿 떠 있어 맵이 늦게 뜨면 `-Start 90`.

## 테스트

`Grimoire.Cheat.Super`(`Source/Grimoire/Core/Cheats/Tests/`) — 켜면 최대로 고정(비용 · 감소 뒤에도), 상태 태그 해제, MaxHealth 변화 추종, 끄면 최대에서 감소 재개.

`Grimoire.Appearance.*`(`Source/Grimoire/Appearance/Tests/`) — 표 읽기, 저장 · 복제 직렬화 왕복, NPC 씨앗 결정성, 서버 검사(Sanitize), 조립(고리 수 · 미리보기 · 체형).

```powershell
# 에디터 없이 전체 자동화 테스트
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire; Quit" -unattended -nopause -nosplash -NullRHI -log
```
