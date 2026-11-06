# 치트 · 개발 명령 확인(헤드리스 단독 게임, L_OpenBeta) — docs/qa/cheats.md 의 QA-CHT-*.
# Grim.At 으로 시간표를 짜서 Grim.Go · Grim.Go.List · Grim.Spawn · Grim.Creature.Mark · Grim.Creature.Dump · GrimSuperCheat 를 차례로 치고 로그로 판정한다.
#
#   powershell -File Scripts\QA\check_cheats.ps1      # 결과: Saved\QA\cheats.log 요약, 끝에 PASS / FAIL
#
# 시간표(엔진 시작 기준 초, 맵 로드 10~25초를 넉넉히):
#   30 Grim.Go.List Jack → 31 Grim.Go Jackalope(실제 무리 곁) → 35 생물 덤프 ① → 36 Grim.Spawn Jack 3 → 37 Grim.Spawn(인자 없음 = 종 목록)
#   39 생물 덤프 ②(① + 3 이상) → 40 Grim.Creature.Mark 1 → 41 GrimSuperCheat 1 → 43 HP 10 · 배고픔 5 로 내려 봄(지금 값은 최대 그대로)
#   46 GrimSuperCheat 0 → 48 HP 10 으로 내려 봄(지금 값 10) → 50 Grim.Go Tut_(탑 안) → 52 Grim.Go 없는이름 → 53 Grim.Creature.Mark 0 → 55 끔
param(
	[int]$RunSeconds = 90
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "cheats.log"
Remove-Item $Log -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$Cmds = @(
	"Grim.At 30 Grim.Go.List Jack",
	"Grim.At 31 Grim.Go Jackalope",
	"Grim.At 32 Grim.Terrain.Probe",
	"Grim.At 35 Grim.Creature.Dump",
	"Grim.At 36 Grim.Spawn Jack 3",
	"Grim.At 37 Grim.Spawn",
	"Grim.At 39 Grim.Creature.Dump",
	"Grim.At 40 Grim.Creature.Mark 1",
	"Grim.At 41 GrimSuperCheat 1",
	"Grim.At 43 Grim.Save.SetAttr Health 10",
	"Grim.At 43.5 Grim.Save.SetAttr Hunger 5",
	"Grim.At 46 GrimSuperCheat 0",
	"Grim.At 48 Grim.Save.SetAttr Health 10",
	"Grim.At 50 Grim.Go Tut_",
	"Grim.At 51 Grim.Terrain.Probe",
	"Grim.At 52 Grim.Go NoSuchPlaceQA",
	"Grim.At 53 Grim.Creature.Mark 0",
	"Grim.At 55 quit"
) -join ","

$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-GrimStartingSpells", "-nullrhi", "-nosound",
	"-unattended", "-nosplash", "-nop4", "-log", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Deadline = (Get-Date).AddSeconds($RunSeconds)
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) { Start-Sleep -Seconds 2 }
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

$Text = Get-Content $Log -Encoding utf8
"===== 로그 요약"
$Text | Select-String -Pattern "\[Grim\.Go|\[Grim\.At\]|LogGrimCreatures: Display|SetAttr|슈퍼치트|GrimSuperCheat|Probe|Error: " | ForEach-Object { $_.Line }

$Failed = 0
function Check([string]$Name, [bool]$Ok) {
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
	if (-not $Ok) { $script:Failed++ }
}
$Dumps = @($Text | Select-String -Pattern "Dump 생물 (\d+)" | ForEach-Object { [int]$_.Matches[0].Groups[1].Value })
$Attr = @($Text | Select-String -Pattern "SetAttr (\w+) = [\d.]+ \(지금 ([\d.]+)\)" | ForEach-Object { , @($_.Matches[0].Groups[1].Value, [double]$_.Matches[0].Groups[2].Value) })
Check "Grim.Go.List 가 재칼로프 자리를 보여 준다" ([bool]($Text -match "\[Grim\.Go\.List\] .*Jack"))
Check "Grim.Go Jackalope 가 이동을 알린다" ([bool]($Text -match "\[Grim\.Go\] .*Jackalope.*곁으로 이동"))
Check "무리 곁에 가면 생물이 나온다(덤프 ① > 0)" ($Dumps.Count -ge 1 -and $Dumps[0] -gt 0)
Check "Grim.Spawn Jack 3 이 3마리를 놓는다" ([bool]($Text -match "Spawn Jackalope x3") -and [bool]($Text -match "Jackalope × 3 를 앞 6m 에 놓았다"))
Check "인자 없는 Grim.Spawn 은 종 목록을 보여 준다" ([bool]($Text -match "사용: Grim\.Spawn <종>.*종: .*Jackalope"))
Check "덤프 ② = ① + 3 이상" ($Dumps.Count -ge 2 -and $Dumps[1] -ge $Dumps[0] + 3)
Check "Grim.Creature.Mark 1 · 0 이 켬 · 끔을 알린다" ([bool]($Text -match "생물 표시 켬") -and [bool]($Text -match "생물 표시 끔"))
Check "GrimSuperCheat 1 · 0 이 서버에서 적용된다" ([bool]($Text -match "슈퍼치트 켬") -and [bool]($Text -match "슈퍼치트 끔"))
Check "슈퍼치트 중에는 HP 를 10 으로 내려도 지금 값이 그대로(>10)" ($Attr.Count -ge 1 -and $Attr[0][0] -eq "Health" -and $Attr[0][1] -gt 10)
Check "슈퍼치트 중에는 배고픔을 5 로 내려도 지금 값이 그대로(>5)" ($Attr.Count -ge 2 -and $Attr[1][0] -eq "Hunger" -and $Attr[1][1] -gt 5)
Check "끈 뒤에는 HP 10 이 그대로 들어간다" ($Attr.Count -ge 3 -and $Attr[2][0] -eq "Health" -and [math]::Abs($Attr[2][1] - 10) -lt 0.5)
Check "Grim.Go Tut_ 가 탑 안 대상으로 간다" ([bool]($Text -match "\[Grim\.Go\] Tut_.*곁으로 이동"))
Check "없는 이름은 찾지 못했다고 알린다" ([bool]($Text -match "NoSuchPlaceQA.*찾지 못했다"))
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
