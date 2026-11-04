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

## 테스트

`Grimoire.Cheat.Super`(`Source/Grimoire/Core/Cheats/Tests/`) — 켜면 최대로 고정(비용 · 감소 뒤에도), 상태 태그 해제, MaxHealth 변화 추종, 끄면 최대에서 감소 재개.

```powershell
# 에디터 없이 전체 자동화 테스트
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" -ExecCmds="Automation RunTests Grimoire; Quit" -unattended -nopause -nosplash -NullRHI -log
```
