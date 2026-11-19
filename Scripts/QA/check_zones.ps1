# 구역 이름 표시(Q130) — 본 구역이 세이브에 남는가(헤드리스 혼자 두 번) — docs/qa/world-zones.md QA-ZON-009.
#
#   powershell -File Scripts\QA\check_zones.ps1      # 결과: Saved\QA\zones_run1.log · zones_run2.log 요약, 끝에 PASS / FAIL
#
# 슬롯 ZoneCheck 를 지우고 L_OpenBeta?SaveSlot=ZoneCheck 를 두 번 켠다(-unattended 라 URL 에 슬롯을 줘야 저장된다).
#   1번: 시작 자리(마법탑 안) → 마탑 곶 · 마탑 앞마당에 「처음」 들어섬 + 배너 → 준비 +6 초 덤프 → +7 초 끔(종료 저장)
#   2번: 같은 슬롯 → 세이브를 읽은 뒤에야 살핀다 → 마탑 곶은 「다시」(작은 배너), 앞마당은 「다시」(배너 없음) — 「처음」이 없어야 한다
param(
	[string]$Slot = "ZoneCheck",
	[int]$TimeoutSeconds = 240
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
$SlotDir = Join-Path $Root "Saved\SaveGames\Grimoire\$Slot"
New-Item -ItemType Directory -Force $OutDir | Out-Null
Remove-Item -Recurse -Force $SlotDir -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$Map = "/Game/Grimoire/Maps/L_OpenBeta?SaveSlot=$Slot"
$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Cmds = "Grim.Save.At 6 Grim.Zones Dump,Grim.Save.At 7 Grim.Save.Quit"

$Logs = @()
foreach ($Run in 1, 2) {
	$Log = Join-Path $OutDir "zones_run$Run.log"
	Remove-Item $Log -ErrorAction SilentlyContinue
	$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", $Map, "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"") + $Common)
	$Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
	while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) { Start-Sleep -Seconds 2 }
	if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force; Write-Output "[경고] $Run 번 실행이 $TimeoutSeconds 초 안에 끝나지 않아 껐다" }
	$Logs += $Log
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimZone|세이브 슬롯|Player_.*읽" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Failed = 0
function Check([string]$Name, [bool]$Ok) {
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
	if (-not $Ok) { $script:Failed++ }
}
function Count([string]$Log, [string]$Pattern) { @(Select-String -Path $Log -Encoding UTF8 -Pattern $Pattern).Count }

"===== 판정"
Check "1번: 마탑 곶 처음" ((Count $Logs[0] "Sec_TowerCape 에 들어섬\(처음\)") -eq 1)
Check "1번: 마탑 앞마당 처음" ((Count $Logs[0] "Sub_Yard 에 들어섬\(처음\)") -eq 1)
Check "1번: 큰 구역 · 소구역 배너" ((Count $Logs[0] "배너: Sec_TowerCape .*\(처음\)") -eq 1 -and (Count $Logs[0] "배너: Sub_Yard .*\(처음\)") -eq 1)
Check "1번: 덤프 — 본 구역 2" ((Count $Logs[0] "본 구역 2 —") -ge 1)
Check "2번: 처음 들어섬 없음(세이브에서 읽음)" ((Count $Logs[1] "에 들어섬\(처음\)") -eq 0)
Check "2번: 마탑 곶 다시 + 작은 배너" ((Count $Logs[1] "Sec_TowerCape 에 들어섬\(다시\)") -eq 1 -and (Count $Logs[1] "배너: Sec_TowerCape .*\(다시\)") -eq 1)
Check "2번: 소구역 다시는 배너 없음" ((Count $Logs[1] "배너: Sub_Yard") -eq 0)
Check "2번: 덤프 — 본 구역 2" ((Count $Logs[1] "본 구역 2 —") -ge 1)
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
