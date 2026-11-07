# Q121 수리 해금 2인 확인(헤드리스): 리슨 서버 + 클라이언트(-game -nullrhi), 맵 L_M1_Sandbox.
# 호스트가 거점 핵 · 저장고(나무) · 작업대를 놓고 → 클라이언트가 레스티투오 없이 작업대 수리 · 조각 수리 · 거점 전체 수리를 눌러 잠김 까닭을 받고(Q64 방식)
# → 레스티투오를 배워 작업대 수리(장비) · 저장고 몸 수리 → 레스티투오 막시마를 배워 거점 전체 수리(저장고 재료). 호스트는 막시마 없이 전체 수리를 눌러 잠김.
#
#   powershell -File Scripts\Base\net_check_repair.ps1      # 결과: Saved\NetCheckRepair\server.log · client.log 요약, NETCHECK_RESULT fail=0
param(
	[int]$Port = 17797,
	[int]$RunSeconds = 45,
	[int]$BootTimeout = 240
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckRepair"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

function J([double]$Seconds, [string]$Command) { "Grim.Base.AfterJoin 2 $Seconds $Command" }

$ServerCmds = @(
	(J 3 "Grim.Items.Give Wood 50"),
	(J 3.3 "Grim.Items.Give Stone 30"),
	(J 5 "Grim.Base.Core 1200"),
	(J 7 "Grim.Base.PlaceAt StorageChest 1 0 0 0 0"),
	(J 8 "Grim.Base.PlaceAt Workbench 0 -2 0 0 0"),
	(J 9.5 "Grim.Base.Store Wood 20"),
	(J 10 "Grim.Base.Damage GrimStorageChest_0 60"),
	(J 20 "Grim.Base.RepairNearest Core"),
	(J 27 "Grim.Base.Damage GrimStorageChest_0 60"),
	(J 32 "Grim.Base.Dump")
) -join ","
$ClientCmds = @(
	(J 12 "Grim.Items.Give Wood 6"),
	(J 13 "Grim.Base.Wear 0 100"),
	(J 14 "Grim.Base.RepairNearest Workbench"),
	(J 16 "Grim.Base.RepairNearest Core"),
	(J 18 "Grim.Base.Repair GrimStorageChest_0"),
	(J 20 "Grim.Base.LearnRepair"),
	(J 22 "Grim.Base.RepairNearest Workbench"),
	(J 24 "Grim.Base.Repair GrimStorageChest_0"),
	(J 26 "Grim.Base.LearnBaseRepair"),
	(J 29 "Grim.Base.RepairNearest Core"),
	(J 32 "Grim.Base.Dump")
) -join ","

function Wait-Log([string]$Path, [string]$Pattern, [int]$Timeout) {
	$Deadline = (Get-Date).AddSeconds($Timeout)
	while ((Get-Date) -lt $Deadline) {
		if ((Test-Path $Path) -and (Select-String -Path $Path -Pattern $Pattern -Quiet)) { return $true }
		Start-Sleep -Seconds 2
	}
	return $false
}

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
if (-not (Wait-Log $ServerLog "Bringing World /Game/Grimoire/Maps/L_M1_Sandbox" $BootTimeout)) { "서버 맵이 뜨지 않았다" }
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)
if (-not (Wait-Log $ClientLog "Welcomed by server" $BootTimeout)) { "클라이언트가 들어오지 못했다" }
Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Encoding UTF8 -Pattern "수리|레스티투오|막시마|노말 스킬|마법 실패|놓음|Welcomed by server|Error: " | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Fail = 0
$ServerText = Get-Content -Encoding UTF8 -Raw $ServerLog
$ClientText = Get-Content -Encoding UTF8 -Raw $ClientLog
function Expect-Count([string]$Name, [string]$Text, [string]$Pattern, [int]$Min, [int]$Max = 999) {
	$Count = ([regex]::Matches($Text, $Pattern)).Count
	if ($Count -lt $Min -or $Count -gt $Max) { $script:Fail++; "없음: $Name — 「$Pattern」 $Count 번(기대 $Min~$Max)" } else { "맞음: $Name ($Count)" }
}

# 잠김 — 서버 판정, 소유 클라이언트 알림(연출 없음)
Expect-Count "서버: 레스티투오 잠김(작업대 · 조각)" $ServerText "수리 잠김: 레스티투오 필요" 2
Expect-Count "서버: 막시마 잠김(클라이언트 · 호스트)" $ServerText "수리 잠김: 레스티투오 막시마 필요" 2
Expect-Count "클라이언트 알림 RepairLocked" $ClientText "\[Client\] 마법 실패 알림 RepairLocked" 1
Expect-Count "클라이언트 알림 BaseRepairLocked" $ClientText "\[Client\] 마법 실패 알림 BaseRepairLocked" 1
Expect-Count "호스트 알림 BaseRepairLocked(서버 로그)" $ServerText "\[Server\] 마법 실패 알림 BaseRepairLocked" 1
Expect-Count "잠김은 피식 연출 없음" $ClientText "마법 실패 연출 \S+ (Base)?RepairLocked" 0 0
# 해금 뒤
Expect-Count "레스티투오 배움" $ServerText "「수리」 배우기: 배움" 1
Expect-Count "작업대 수리 — 기본 완드(재료 · 수리 횟수)" $ServerText "장비 수리 Wand_Basic: 재료 Wood ×\d+ .*수리 1번" 1
Expect-Count "조각 · 시설 하나(저장고)" $ServerText "수리 GrimStorageChest_0: Wood ×\d+" 1
Expect-Count "막시마 배움" $ServerText "노말 스킬 RestituoMaxima 배움" 1
Expect-Count "거점 전체 수리(저장고 재료)" $ServerText "거점 수리: 닳은 것 [1-9]\d*, 저장고 재료 [1-9]" 1
"NETCHECK_RESULT fail=$Fail"
