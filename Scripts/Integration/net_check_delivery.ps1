# M1.5 전달 방식 2인 확인(헤드리스): net_check.ps1 과 같은 방식(리슨 서버 + 클라이언트, -game -nullrhi)으로
# 원격 클라이언트가 불씨(Bolt) · 미풍(Cone) · 물줄기(Stream)를 실제로 시전하고, 양쪽이 본 양초 상태를 로그로 남긴다.
#
#   powershell -File Scripts\Integration\net_check_delivery.ps1     # 결과: Saved\NetCheck\delivery_server.log · delivery_client.log 의 요약
#
# 대상 = 원점에 가장 가까운 양초(L_M1_Sandbox 의 Candle_1, (500,0)). 서버가 Grim.Magic.Place 로 클라이언트 폰을 양초 −X 쪽에 두고,
# 클라이언트가 Grim.Magic.Cast 로 그 양초를 고정 조준해 시전한다(0.5초 뒤 누름).
# 시간표(서버 시작 기준 초, 클라이언트는 12초 뒤 시작):
#   43 6m 에 둠 → 45 불씨      → 49 덤프  기대: Candle_1 Burning=1
#              → 51 미풍(6m)   → 55 덤프  기대: Burning=2 (먼 미풍 = 키움)
#   57 1.2m 에 둠 → 60 미풍     → 63 덤프  기대: Burning 없음 (가까운 미풍 = 돌풍)
#   65 6m 에 둠 → 67 불씨      (다시 켬)
#   71 4m 에 둠 → 73 물줄기 1.5초 누름 → 79 덤프  기대: Burning 없음, Wet ≥ 1
# 서버 · 클라이언트 덤프 줄의 states 가 같아야 한다.
param(
	[int]$Port = 17778,
	[int]$RunSeconds = 80,
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "delivery_server.log"
$ClientLog = Join-Path $OutDir "delivery_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ClientStart = 12
function ClientAt([double]$ServerTime) { return $ServerTime - $ClientStart }

$ServerCmds = @(
	"Grim.Magic.Place Candle 600 43",
	"Grim.Interaction.Dump 49",
	"Grim.Interaction.Dump 55",
	"Grim.Magic.Place Candle 120 57",
	"Grim.Interaction.Dump 63",
	"Grim.Magic.Place Candle 600 65",
	"Grim.Magic.Place Candle 400 71",
	"Grim.Interaction.Dump 79"
) -join ","
$ClientCmds = @(
	"Grim.Magic.Cast Ember Candle 0 $(ClientAt 45)",
	"Grim.Interaction.Dump $(ClientAt 49)",
	"Grim.Magic.Cast Breeze Candle 0 $(ClientAt 51)",
	"Grim.Interaction.Dump $(ClientAt 55)",
	"Grim.Magic.Cast Breeze Candle 0 $(ClientAt 60)",
	"Grim.Interaction.Dump $(ClientAt 63)",
	"Grim.Magic.Cast Ember Candle 0 $(ClientAt 67)",
	"Grim.Magic.Cast WaterJet Candle 1.5 $(ClientAt 73)",
	"Grim.Interaction.Dump $(ClientAt 79)"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds $ClientStart
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimInteraction: Display: \[.*\] GrimCandle|LogGrimMagic:|LogGrimMagicDebug|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line }
}
