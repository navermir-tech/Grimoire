# 마법 하나를 게임 창으로 띄워 시전하고, 느린 화면(slomo)에서 여러 순간을 HighResShot 으로 찍는다(이펙트 확인용).
#   powershell -File Scripts\MagicFX\capture_spell.ps1 -Spell Ember -Out art\fx\ingame\ember
# 순간 = 시전 뒤 게임 시간(초) × 1/Slomo 만큼 실제로 기다렸다 찍는다. 결과 PNG 는 -Out 폴더에 shot_<게임초>.png 로 옮긴다.
param(
	[string]$Spell = "Ember",
	[string]$Out = "art\fx\ingame\ember",
	[string]$MomentList = "0.05,0.12,0.25,0.4,0.55,0.75,1.0",
	[double]$Slomo = 0.1,
	[double]$Start = 25,
	[string]$Map = "/Game/Grimoire/Maps/L_OpenBeta",
	# 자리 잡기 명령들(Start 초부터 1초 간격). 기본: 초원 해 질 녘, 8m 앞에 재칼로프 한 마리를 표적으로.
	# 기본 공격은 시작 마법에 없다 — 앞에 「;Grim.Magic.SpawnSkillbook BasicAttack 100;Grim.Magic.ReadSkillbook」 를 더한다.
	[string]$SetupList = "Grim.Go Jackalope;Grim.Creature.Time 0.8;Grim.Spawn Jack 1 8",
	[string]$TargetClass = "Jackalope"
)
$Moments = $MomentList.Split(",") | ForEach-Object { [double]$_ }
$Setup = $SetupList.Split(";")
$UE = "C:\Program Files\Epic Games\UE_5.8"
$Root = Split-Path (Split-Path $PSScriptRoot)
$ShotDir = Join-Path $Root "Saved\Screenshots\WindowsEditor"
if (Test-Path $ShotDir) { Remove-Item "$ShotDir\*.png" -Force }

$cmds = @()
for ($i = 0; $i -lt $Setup.Count; $i++) { $cmds += "Grim.At $($Start + $i) $($Setup[$i])" }
$cast = $Start + $Setup.Count + 2
# Grim.Magic.Cast 는 대상을 본 뒤 0.5초(게임 시간) 있다 누른다 — 그 직전에 느리게 하고, 순간은 누른 때부터 잰다.
$press = $cast + 0.5
$cmds += "Grim.At $cast Grim.Magic.Cast $Spell $TargetClass"
$cmds += "Grim.At $($cast + 0.45) slomo $Slomo"
foreach ($m in $Moments) { $cmds += "Grim.At $([math]::Round($press + $m / $Slomo, 2)) HighResShot 1280x720" }
$last = $press + ($Moments | Measure-Object -Maximum).Maximum / $Slomo + 4
$cmds += "Grim.At $last quit"

$p = Start-Process -FilePath "$UE\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
	"`"$Root\Grimoire.uproject`"", $Map, "-game", "-windowed", "-ResX=1280", "-ResY=720", "-GrimStartingSpells",
	"-nosound", "-unattended", "-ExecCmds=`"$($cmds -join ', ')`"")
if (-not $p.WaitForExit([int](($last + 240) * 1000))) { $p.Kill() }

New-Item -ItemType Directory -Force (Join-Path $Root $Out) | Out-Null
$shots = Get-ChildItem "$ShotDir\*.png" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime
for ($i = 0; $i -lt $shots.Count -and $i -lt $Moments.Count; $i++) {
	Move-Item $shots[$i].FullName (Join-Path $Root "$Out\shot_$($Moments[$i]).png") -Force
}
"찍힘 $($shots.Count) / $($Moments.Count) → $Out"
