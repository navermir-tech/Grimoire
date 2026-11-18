# 구역 이름 표시(Q130) 2인 확인(헤드리스 리슨 서버 + 클라이언트, L_OpenBeta) — docs/qa/world-zones.md QA-ZON-010.
# 본 구역 · 배너는 플레이어마다 따로다: 호스트가 먼저 들어선 구역도 나중에 온 클라이언트에게는 「처음」이고, 배너는 그 플레이어 화면에만 뜬다.
#
#   powershell -File Scripts\QA\net_check_zones.ps1      # 결과: Saved\QA\netzones_server.log · netzones_client.log 요약, 끝에 PASS / FAIL
#
# 시간표(초)는 두 플레이어가 모인 때(Grim.Taming.AfterJoin 2 …)부터: 3 서버 덤프 → 4 Grim.Go GiantTree(둘 다 중부 구릉으로) → 10 서버 덤프
param(
	[int]$Port = 18791,
	[int]$RunSeconds = 20
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "netzones_server.log"
$ClientLog = Join-Path $OutDir "netzones_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Map = "/Game/Grimoire/Maps/L_OpenBeta"

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

$ServerCmds = (@("3 Grim.Zones Dump", "4 Grim.Go GiantTree", "10 Grim.Zones Dump") | ForEach-Object { $P = $_ -split ' ', 2; "Grim.Taming.AfterJoin 2 $($P[0]) $($P[1])" }) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
$Waited = 0
while ($Waited -lt 240 -and -not ((Test-Path $ServerLog) -and (Select-String -Path $ServerLog -Pattern 'Bringing World' -Quiet))) { Start-Sleep -Seconds 1; $Waited++ }
Start-Sleep -Seconds 2
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"") + $Common)
$Waited = 0
while ($Waited -lt 240 -and -not ((Test-Path $ClientLog) -and (Select-String -Path $ClientLog -Pattern 'Welcomed by server' -Quiet))) { Start-Sleep -Seconds 1; $Waited++ }
Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimZone|\[Grim\.Go\]|Welcomed by server" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Failed = 0
function Check([string]$Name, [bool]$Ok) {
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
	if (-not $Ok) { $script:Failed++ }
}
function Count([string]$Log, [string]$Pattern) { @(Select-String -Path $Log -Encoding UTF8 -Pattern $Pattern).Count }
# 들어섬 줄 = [Server] <PlayerState> 가 <구역> 에 들어섬(처음|다시)
$Entered = @(Select-String -Path $ServerLog -Encoding UTF8 -Pattern '\[Server\] (\S+) 가 (\S+) 에 들어섬\((\S+)\)' | ForEach-Object {
	[PSCustomObject]@{ Player = $_.Matches[0].Groups[1].Value; Zone = $_.Matches[0].Groups[2].Value; Kind = $_.Matches[0].Groups[3].Value }
})
$Players = @($Entered | Select-Object -ExpandProperty Player -Unique)
"===== 판정 (들어섬 줄 $($Entered.Count), 플레이어 $($Players -join ', '))"
Check "서버가 두 플레이어를 따로 본다" ($Players.Count -eq 2)
foreach ($Zone in "Sec_TowerCape", "Sub_Yard", "Sec_CentralHills") {
	$Firsts = @($Entered | Where-Object { $_.Zone -eq $Zone -and $_.Kind -eq "처음" })
	Check "$Zone — 두 플레이어 모두 「처음」(각자 기준)" ((@($Firsts | Select-Object -ExpandProperty Player -Unique)).Count -eq 2)
}
Check "호스트 화면 배너: 마탑 곶 1 · 중부 구릉 1(자기 것만)" ((Count $ServerLog "\[Server\] 배너: Sec_TowerCape") -eq 1 -and (Count $ServerLog "\[Server\] 배너: Sec_CentralHills") -eq 1)
Check "클라이언트 화면 배너: 마탑 곶 1 · 중부 구릉 1(RPC 가 소유 클라이언트에)" ((Count $ClientLog "\[Client\] 배너: Sec_TowerCape") -eq 1 -and (Count $ClientLog "\[Client\] 배너: Sec_CentralHills") -eq 1)
Check "덤프: 두 플레이어 본 구역 3(마탑 곶 · 앞마당 · 중부 구릉)" ((Count $ServerLog "본 구역 3 —") -eq 2)
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
