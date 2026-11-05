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

생물은 서식 구역(수백 m)에 무리로 기록만 되어 있다가 **플레이어 60m 안**에 들어오면 나온다(설정 `Ecosystem.SpawnRadius`). 구역 한가운데에 서도 무리가 멀면 아무것도 안 보인다 — `Grim.Creature.Mark 1` 의 노란 기둥을 따라가거나 `Grim.Go <종>` 을 쓴다.

## 테스트

`Grimoire.Cheat.Super`(`Source/Grimoire/Core/Cheats/Tests/`) — 켜면 최대로 고정(비용 · 감소 뒤에도), 상태 태그 해제, MaxHealth 변화 추종, 끄면 최대에서 감소 재개.

```powershell
# 에디터 없이 전체 자동화 테스트
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire; Quit" -unattended -nopause -nosplash -NullRHI -log
```
