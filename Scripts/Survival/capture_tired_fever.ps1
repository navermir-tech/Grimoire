# 지침 「MISS」(Q132) · 열병(Q115) 화면 찍기 — 게임 창(L_OpenBeta, 단독 = 리슨 서버)에서:
#   1) 재칼로프 셋을 8 m 앞에 부르고 피곤함을 79(지침 — 빗나감 약 30 %)로, 불씨를 여러 번 쏘며 쏠 때마다 찍는다.
#      빗나간 발은 로그 「빗나감(지침)」과 맞은 자리 위의 「MISS」 글로 보인다.
#   2) 열병을 걸고(grim.Survival.Sick 2) 생존 디버그 글(grim.Survival.ShowDebug 1)을 켜 「Fever」 · 체온이 오르는 것을 찍는다.
#
#   powershell -File Scripts\Survival\capture_tired_fever.ps1
#   → Saved\TiredCapture\shot_###.png, 로그 Saved\TiredCapture\game.log 의 「빗나감(지침)」 줄 수
param(
	[int]$Casts = 12,
	[double]$CastInterval = 2.5,
	[double]$Start = 25
)
$ErrorActionPreference = "Stop"
$UE = "C:\Program Files\Epic Games\UE_5.8"
$Root = Split-Path (Split-Path $PSScriptRoot)
$Out = Join-Path $Root "Saved\TiredCapture"
$ShotDir = Join-Path $Root "Saved\Screenshots\WindowsEditor"
New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item "$Out\*" -Force -ErrorAction SilentlyContinue
if (Test-Path $ShotDir) { Remove-Item "$ShotDir\*.png" -Force -ErrorAction SilentlyContinue }

$cmds = @(
	"Grim.At $Start Grim.Go Jackalope",
	"Grim.At $($Start + 1) Grim.Creature.Time 0.45",
	"Grim.At $($Start + 2) Grim.Spawn Jack 3 8",
	"Grim.At $($Start + 3) grim.Survival.Fatigue 79"
)
$t = $Start + 5
for ($i = 0; $i -lt $Casts; $i++) {
	# Grim.Magic.Cast 는 대상을 본 뒤 0.5초 있다 누른다 — 누른 뒤 0.35초(볼트가 닿고 글이 떠오를 때) 찍는다.
	$cmds += "Grim.At $([math]::Round($t, 2)) grim.Survival.Fatigue 79"
	$cmds += "Grim.At $([math]::Round($t + 0.05, 2)) Grim.Magic.Cast Ember Jackalope"
	$cmds += "Grim.At $([math]::Round($t + 0.85, 2)) HighResShot 1280x720"
	$t += $CastInterval
}
# 열병: 걸고 디버그 글을 켜 두 번(바로 · 20초 뒤 — 체온이 오른다)
$cmds += "Grim.At $([math]::Round($t, 2)) grim.Survival.Fatigue 0"
$cmds += "Grim.At $([math]::Round($t + 0.5, 2)) grim.Survival.Sick 2"
$cmds += "Grim.At $([math]::Round($t + 1, 2)) grim.Survival.ShowDebug 1"
# 디버그 글은 HighResShot(장면만)에 안 찍힌다 — 화면 그대로(shot showui).
$cmds += "Grim.At $([math]::Round($t + 2, 2)) shot showui"
$cmds += "Grim.At $([math]::Round($t + 22, 2)) shot showui"
$last = $t + 26
$cmds += "Grim.At $([math]::Round($last, 2)) quit"
$Expected = $Casts + 2

$Log = Join-Path $Out "game.log"
$p = Start-Process -FilePath "$UE\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
	"`"$Root\Grimoire.uproject`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-windowed", "-ResX=1280", "-ResY=720", "-GrimStartingSpells",
	"-nosound", "-unattended", "-log", "-ABSLOG=`"$Log`"", "-ExecCmds=`"$($cmds -join ', ')`"")
$deadline = (Get-Date).AddSeconds($last + 240)
while (-not $p.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep 2 }
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }

$i = 0
Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | ForEach-Object {
	Copy-Item $_.FullName ("$Out\shot_{0:D3}.png" -f $i)
	$i++
}
$misses = @(Select-String -Path $Log -Pattern "빗나감\(지침\)" -Encoding utf8 -ErrorAction SilentlyContinue).Count
Write-Host "찍은 장 수: $i / $Expected → $Out, 빗나감 로그 $misses 줄"
