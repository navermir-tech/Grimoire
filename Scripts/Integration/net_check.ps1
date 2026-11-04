# M1 2인 복제 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고, 디버그 콘솔 명령으로
# 서버에서 사물에 자극을 준 뒤 양쪽이 본 상태 · 불꽃 · 빛 · 완드 부착을 로그로 남긴다.
#
#   powershell -File Scripts\Integration\net_check.ps1            # 결과: Saved\NetCheck\server.log · client.log 의 요약
#
# 시간표(서버 시작 기준 초): 5 모닥불 점화(접속 전) → 12 클라이언트 시작 → 40 양초 점화(휴면 깨우기 · 항목 추가) →
# 50 양초 바람(세기 변화) → 60 양초 물(불 꺼짐 = 항목 제거, 젖음 추가). 45 · 55 · 70 에 양쪽 덤프 — 같은 줄이 나와야 한다.
param(
	[int]$Port = 17777,
	[int]$RunSeconds = 90,
	# 더 볼 로그 예: -LogCmds "LogNetDormancy Verbose"
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

# 서버 기준 초. 클라이언트는 12초 뒤에 시작하므로 클라이언트 지연 = 서버 시각 − 12.
$ServerCmds = @(
	"Grim.Interaction.Stimulus Element.Fire 1 5 Campfire",
	"Grim.Interaction.Stimulus Element.Fire 1 40 Candle",
	"Grim.Interaction.Dump 45",
	"Grim.Interaction.Stimulus Element.Wind 1 50 Candle",
	"Grim.Interaction.Dump 55",
	"Grim.Interaction.Stimulus Element.Water 1 60 Candle",
	"Grim.Interaction.Dump 70",
	"Grim.Magic.DumpWands 70"
) -join ","
$ClientCmds = @(
	"Grim.Interaction.Dump 33",
	"Grim.Interaction.Dump 43",
	"Grim.Interaction.Dump 58",
	"Grim.Magic.DumpWands 58"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimInteraction: Display|LogGrimMagicDebug|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line }
}
