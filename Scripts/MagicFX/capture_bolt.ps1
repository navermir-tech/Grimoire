# 볼트 이펙트 화면 찍기(L_Tower): 훈련 표적 앞에서 마법을 쏘고, 아주 느린 시간(slomo)으로 시전 → 비행 → 적중을 여러 장 찍는다.
# 볼트는 75~90 m/s 라 4 m 거리를 0.05초에 날아간다 — 그대로는 비행 장면을 못 잡는다.
#
#   powershell -File Scripts/MagicFX/capture_bolt.ps1 -Spell Ember            # 옆에서(참고 그림 구도: 왼쪽 완드 · 오른쪽 표적), 연출만(Grim.FX.Bolt)
#   powershell -File Scripts/MagicFX/capture_bolt.ps1 -Spell Ember -Cast       # 실제 시전(적중 연출은 이쪽에서만 나온다)
#   powershell -File Scripts/MagicFX/capture_bolt.ps1 -Spell Ember -Side:$false  # 게임 카메라(어깨 뒤)
# 실제 시전은 투사체를 시전 대기(0.1초)만큼 앞당겨 만들어 4 m 앞 표적은 바로 맞는다 — 비행은 연출만 모드로 찍는다.
#   → Saved/BoltCapture/<Spell>[_side]/shot_###.png
#
# Grim.Go 는 표적(300, -1000) 4 m 앞(-100, -1000)에 +X 를 보고 세운다(BugIt 로 확인, 2026-11-06).
# 방은 Y -1180 ~ -420(벽). 옆 카메라(Grim.Cam)는 +Y 벽 가까이에서 -Y 를 본다 — 화면 오른쪽이 +X(표적).
# 표적 이름 None 은 겨누지 않고 정면으로 쏜다.
param(
    [string]$Spell = "Ember",
    [string]$Target = "GrimTrialTarget",
    [bool]$Side = $true,
    [switch]$Cast,
    [string]$BoltFrom = "30 -1000 150 0 0 225",  # X Y Z 요 피치 터질거리 — 표적(300, -1000) 앞면에서 터진다
    [double]$Exposure = 0,   # 음수면 어둡게(r.ExposureOffset) — 참고 그림처럼 어두운 배경에서 가산 빛을 볼 때
    [string]$Tag = "",
    [string]$SideCam = "100 -450 140 -3 -90 75",
    [double]$Slomo = 0.02,
    [int]$Shots = 60,
    [double]$Interval = 0.3,
    [double]$Start = 20.5
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\..\.."
$UE = "C:\Program Files\Epic Games\UE_5.8"
$Project = "$Root\Grimoire.uproject"
$Out = "$Root\Saved\BoltCapture\$Spell" + $(if ($Side) { "_side" } else { "" }) + $Tag
$ShotDir = "$Root\Saved\Screenshots\WindowsEditor"
if (Test-Path $Out) { Remove-Item "$Out\*" -Force }
New-Item -ItemType Directory -Force $Out | Out-Null
if (Test-Path $ShotDir) { Remove-Item "$ShotDir\*" -Force -ErrorAction SilentlyContinue }

# 14초에 표적 앞으로, 16초에 한 번 미리 쏴서 Niagara 컴파일을 끝내 두고(첫 사용 때 1~2초 걸려 시전 장면을 놓친다),
# 20초에 느린 시간, 20.1초에 다시 시전.
if ($Cast) {
    $cmds = @("Grim.Go Tut_TrainingTarget 14", "Grim.At 16 Grim.Magic.Cast $Spell $Target", "Grim.At 20 slomo $Slomo",
        "Grim.At 20.1 Grim.Magic.Cast $Spell $Target")
} else {
    $cmds = @("Grim.Go Tut_TrainingTarget 14", "Grim.At 16 Grim.FX.Bolt DA_Spell_$Spell $BoltFrom", "Grim.At 20 slomo $Slomo",
        "Grim.At $([math]::Round($Start - 0.05, 2)) Grim.FX.Bolt DA_Spell_$Spell $BoltFrom")
}
if ($Side) { $cmds += "Grim.At 18 Grim.Cam $SideCam" }
if ($Exposure -ne 0) { $cmds += "Grim.At 18.1 r.ExposureOffset $Exposure" }
for ($i = 0; $i -lt $Shots; $i++) {
    $t = [math]::Round($Start + $i * $Interval, 2)
    $cmds += "Grim.At $t HighResShot 1280x720"
}
$exec = $cmds -join ", "

$p = Start-Process -FilePath "$UE\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
    "`"$Project`"", "/Game/Grimoire/Maps/L_Tower", "-game", "-GrimStartingSpells", "-windowed", "-ResX=1280", "-ResY=720", "-nosound", "-nosplash",
    "-unattended", "-nop4", "-log", "-ExecCmds=`"$exec`"")
# 마지막 장이 나올 때까지 기다렸다 끈다(quit 은 디버그 카메라 상태에서 멈출 때가 있다).
$deadline = (Get-Date).AddSeconds(240)
while (-not $p.HasExited -and (Get-Date) -lt $deadline -and @(Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue).Count -lt $Shots) { Start-Sleep 2 }
Start-Sleep 2
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }

$i = 0
Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | ForEach-Object {
    Copy-Item $_.FullName ("$Out\shot_{0:D3}.png" -f $i)
    $i++
}
Write-Host "찍은 장 수: $i → $Out"
