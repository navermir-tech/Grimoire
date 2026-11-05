# M2 갈래 K 조작 · 기본 공격 2인 확인(헤드리스): net_check_delivery.ps1 과 같은 방식(리슨 서버 + 클라이언트, -game -nullrhi).
# 원격 클라이언트가 실제 입력과 같은 길(Grim.Input.Press → 캐릭터 → ASC 입력 태그)로 슬롯 바로 시전 · 기본 공격을 하고,
# 양쪽이 본 양초 상태 · 입력 태그 · 연타 번째를 로그로 남긴다.
#
#   powershell -File Scripts\Integration\net_check_controls.ps1     # 결과: Saved\NetCheck\controls_server.log · controls_client.log 의 요약
#
# 대상 = 원점에 가장 가까운 양초(L_M1_Sandbox 의 Candle_1). 시간표(서버 시작 기준 초, 클라이언트는 12초 뒤 시작, 기기가 바쁘면 -Shift 60):
#   43 6m 에 둠 → 45 기본 공격 입력(아직 안 배움)  기대: 서버 · 클라이언트 모두 「주먹질」(볼트 없음)
#   46 뒤 1.5m 에 기본 공격 스킬북(사선 밖) → 48.5 읽기 → 49 입력 덤프  기대: BasicAttack=BasicAttack (튜토리얼 첫 단계와 같은 길)
#   51 슬롯1(첫 칸 = 불 Bolt)                   → 55 덤프  기대: Burning=1
#   57 기본 공격 4연타(0.35초 간격)             → 61 덤프  기대: 서버 · 클라이언트 모두 1/4 · 2/4 · 3/4 · 4/4 강타, 양초 상태 그대로(충격만)
#   63 슬롯2(둘째 칸 = 물 Stream) 1.5초         → 68 덤프  기대: Burning 없음, Wet ≥ 1
#   69 세트 2 → 71 슬롯1(빈 칸) → 72 입력 덤프   기대: 세트=2, 슬롯 태그 없음, BasicAttack 그대로
#   74 세트 1 → 76 입력 덤프                      기대: 첫 세 칸 = Slot1 · Slot2 · Slot3, BasicAttack=BasicAttack
#   78 기본 공격 3번을 0.1초 간격(리듬 0.3초보다 빠름) 기대: 1번만 나간다(서버 · 클라이언트 모두)
param(
	[int]$Port = 17779,
	[int]$RunSeconds = 86,
	# 기기가 바빠 맵 로드가 늦으면 시간표 전체를 이만큼 미룬다(RunSeconds 도 같이 늘어난다)
	[int]$Shift = 0,
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "controls_server.log"
$ClientLog = Join-Path $OutDir "controls_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ClientStart = 12
function ClientAt([double]$ServerTime) { return $ServerTime + $Shift - $ClientStart }
function ServerAt([double]$ServerTime) { return $ServerTime + $Shift }
$RunSeconds += $Shift

$ServerCmds = @(
	"Grim.Magic.Place Candle 600 $(ServerAt 43)",
	"Grim.Magic.SpawnSkillbook BasicAttack -150 $(ServerAt 46)",
	"Grim.Interaction.Dump $(ServerAt 55)",
	"Grim.Interaction.Dump $(ServerAt 61)",
	"Grim.Input.Dump $(ServerAt 61)",
	"Grim.Interaction.Dump $(ServerAt 68)",
	"Grim.Input.Dump $(ServerAt 72)",
	"Grim.Input.Dump $(ServerAt 76)",
	"Grim.Input.Dump $(ServerAt 81)"
) -join ","
$ClientCmds = @(
	"Grim.Input.Press Basic Candle 0 $(ClientAt 45)",
	"Grim.Magic.ReadSkillbook $(ClientAt 48.5)",
	"Grim.Input.Dump $(ClientAt 49)",
	"Grim.Input.Press Slot1 Candle 0 $(ClientAt 51)",
	"Grim.Interaction.Dump $(ClientAt 55)",
	"Grim.Input.Press Basic Candle 0 $(ClientAt 57) 4 0.35",
	"Grim.Interaction.Dump $(ClientAt 61)",
	"Grim.Input.Dump $(ClientAt 61)",
	"Grim.Input.Press Slot2 Candle 1.5 $(ClientAt 63)",
	"Grim.Interaction.Dump $(ClientAt 68)",
	"Grim.Input.SlotSet 2 $(ClientAt 69)",
	"Grim.Input.Press Slot1 Candle 0 $(ClientAt 71)",
	"Grim.Input.Dump $(ClientAt 72)",
	"Grim.Input.SlotSet 1 $(ClientAt 74)",
	"Grim.Input.Dump $(ClientAt 76)",
	"Grim.Input.Press Basic Candle 0 $(ClientAt 78) 3 0.1",
	"Grim.Input.Dump $(ClientAt 81)"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds $ClientStart
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimInteraction: Display: \[.*\] GrimCandle|LogGrimMagic:|LogGrimMagicDebug|LogGrimInputDebug|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line }
}
