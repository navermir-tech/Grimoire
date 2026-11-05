# M2 갈래 G 확인(헤드리스 단독 게임): 숙련도 · 스킬북 · 서클 승급을 콘솔로 한 줄씩 돌리고 로그를 요약한다.
#
#   powershell -File Scripts\Integration\check_growth.ps1     # 결과: Saved\NetCheck\growth.log 의 요약
#
# 기대(시작 기준 초):
#   16 화염구 스킬북 → 17 집기: CircleTooLow(1서클)
#   18 불씨를 양초에 시전 → 숙련도 경험 5(시전 1 + 의미 있는 명중 4 — 시전과 명중은 반복을 따로 센다)
#   22 불씨 · 물줄기 +100 → 둘 다 C → 23 승급: 서클 1 → 2(마법 3 · 등급 합 2)
#   24 화염구 집기: Learned → 25~26 불씨 스킬북: ProficiencyGained(C 구간 200 × 0.4 = 80)
#   27 불씨 +5000(S) → 28~29 불씨 스킬북: AlreadyMastered(책 남음)
param(
	[int]$RunSeconds = 55
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "growth.log"
Remove-Item $Log -ErrorAction SilentlyContinue

$Cmds = @(
	"Grim.Magic.Proficiency 15",
	"Grim.Magic.SpawnSkillbook Fireball 150 16",
	"Grim.Magic.ReadSkillbook 17",
	"Grim.Magic.Cast Ember Candle 0 18",
	"Grim.Magic.Proficiency 21",
	"Grim.Magic.AddXP Ember 100 22",
	"Grim.Magic.AddXP WaterJet 100 22",
	"Grim.Magic.CircleUp 0 23",
	"Grim.Magic.ReadSkillbook 24",
	"Grim.Magic.SpawnSkillbook Ember 150 25",
	"Grim.Magic.ReadSkillbook 26",
	"Grim.Magic.AddXP Ember 5000 27",
	"Grim.Magic.SpawnSkillbook Ember 150 28",
	"Grim.Magic.ReadSkillbook 29",
	"Grim.Magic.Proficiency 31",
	"Grim.UI.Grimoire 32"
) -join ","

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox", "-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=320", "-ResY=240", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
Start-Sleep -Seconds $RunSeconds
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Select-String -Path $Log -Pattern "LogGrimMagic: |LogGrimMagicDebug|LogGrimUI|Error: " | ForEach-Object { $_.Line }
