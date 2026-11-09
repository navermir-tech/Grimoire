# 레벨 디자인 4차 — 실제 캐릭터로 걸어 본다(헤드리스 단독 게임). 비탈을 오르고 터 가장자리까지 가는지.
#
#   powershell -File Scripts\Terrain\check_walk.ps1    # 결과: Saved\TerrainCheck\walk.log, 끝에 PASS/FAIL
#
# 자리(지도 → 월드 X = (v − 380) × 100, Y = (u − 1000) × 100). 다리마다 땅으로 옮겨 놓고 12 초 걷는다.
param([int]$RunSeconds = 100)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\TerrainCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "walk.log"
Remove-Item $Log -ErrorAction SilentlyContinue

# 이름, X, Y, Yaw, 높이 변화 최소(cm, 음수면 재지 않음)
$Legs = @(
	@("봉화 언덕 남쪽 비탈 오르기", 53200, -10700, 0, 800),
	@("봉화 언덕 동쪽 오르기", 58000, -3500, 270, 800),
	@("봉화 터 가운데 → 동쪽 가장자리까지", 58100, -12400, 90, -1),
	@("봉화 터 가운데 → 서쪽 가장자리까지", 58100, -12400, 270, -1),
	@("서쪽 등줄기 안쪽 비탈 오르기", 30000, -38000, 268, 500),
	@("풍차 등성이 오르기", 40000, 28000, 120, 300)
)
$Cmds = @()
$At = 15
foreach ($Leg in $Legs) {
	$Cmds += "Grim.Terrain.Teleport $($Leg[1]) $($Leg[2]) $At"
	$Cmds += "Grim.Terrain.Walk $($At + 3) 12 $($Leg[3])"
	$At += 17
}
$Cmds = $Cmds -join ","
$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=320", "-ResY=240", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
$Deadline = (Get-Date).AddSeconds($RunSeconds + 30)
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) { Start-Sleep -Seconds 2 }
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

$Ends = @(Select-String -Path $Log -Encoding UTF8 -Pattern "\[Walk\] 끝 ")
$Failed = 0
for ($I = 0; $I -lt $Legs.Count; $I++) {
	$Leg = $Legs[$I]
	if ($I -ge $Ends.Count) { Write-Output "[FAIL] $($Leg[0]) — 결과 없음"; $Failed++; continue }
	$M = [regex]::Match($Ends[$I].Line, "수평 (\d+) cm, 높이 변화 (-?\d+) cm, 떨어지는 중 (.+)$")
	$H = [int]$M.Groups[1].Value
	$Z = [int]$M.Groups[2].Value
	$Fall = $M.Groups[3].Value.Trim()
	$Ok = ($H -gt 1500) -and ($Fall -eq "아니오") -and (($Leg[4] -lt 0) -or ($Z -ge $Leg[4]))
	if (-not $Ok) { $Failed++ }
	Write-Output ("{0} {1}: 수평 {2} cm, 높이 {3} cm, 떨어짐 {4}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Leg[0], $H, $Z, $Fall)
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
