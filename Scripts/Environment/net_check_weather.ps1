# M2 날씨 2인 복제 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고, 서버에서 날씨를 바꾼 뒤
# 양쪽이 본 지역 날씨 · 기온 · 등급과 양초 상태를 로그로 남긴다. Scripts/Integration/net_check.ps1 과 같은 방식.
#
#   powershell -File Scripts\Environment\net_check_weather.ps1     # 결과: Saved\NetCheckWeather\server.log · client.log 의 요약
#
# 시간표(서버 시작 기준 초, T = -Start): 12 클라이언트 시작 → T 양초 점화 → T+5 덤프(맑음) → T+10 초원 비(블렌드 0) →
# 비 자극(10초 간격)이 야외 양초를 끄고 적신다 → T+30 덤프(비 · 양초 꺼짐 · 젖음) → T+35 뇌우 → T+40 낙뢰 하나 → T+45 덤프.
# 클라이언트 덤프는 클라이언트 부팅 시각 기준이라 몇 초~십몇 초 늦게 찍힐 수 있다 — 시각보다 상태 흐름이 같은지 본다:
# 맑음 → 비(클라이언트에도 "날씨 Meadow: Clear -> Rain") · 양초 꺼짐 · 젖음 → 뇌우 · "낙뢰 연출".
# 다른 헤드리스 에디터가 같이 돌면 클라이언트 접속이 1분 넘게 걸린다 — 그때는 -Start 를 늘린다(로그의 Join 시각을 본다).
param(
	[int]$Port = 17787,
	[int]$Start = 100,
	[string]$LogCmds = ""
)
$RunSeconds = $Start + 55

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckWeather"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$T = $Start
$C = $Start - 12
$ServerCmds = @(
	"Grim.Interaction.Stimulus Element.Fire 1 $T Candle",
	"Grim.Weather.Dump $($T + 5)",
	"Grim.Interaction.Dump $($T + 5)",
	"Grim.Weather.Set Meadow Rain 0 $($T + 10)",
	"Grim.Weather.Dump $($T + 30)",
	"Grim.Interaction.Dump $($T + 30)",
	"Grim.Weather.Set Meadow Thunderstorm 0 $($T + 35)",
	"Grim.Weather.Strike Meadow $($T + 40)",
	"Grim.Weather.Dump $($T + 45)"
) -join ","
$ClientCmds = @(
	"Grim.Weather.Dump $($C + 5)",
	"Grim.Interaction.Dump $($C + 5)",
	"Grim.Weather.Dump $($C + 30)",
	"Grim.Interaction.Dump $($C + 30)",
	"Grim.Weather.Dump $($C + 45)"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimEnvironment: Display|LogGrimInteraction: Display|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line }
}
