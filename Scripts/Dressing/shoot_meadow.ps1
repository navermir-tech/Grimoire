# 초원 꾸밈 스크린숏(docs/qa/meadow.md) — RHI 를 켠 단독 게임(-RenderOffscreen)으로 L_OpenBeta 를 열고,
# 배치 보고서(Saved/MeadowPlace/report.json)의 자리마다 반응 전 · 후를 찍어 docs/public/qa/meadow-<이름>.jpg 로 둔다. 전경 몇 장도.
#
#   powershell -File Scripts\Dressing\shoot_meadow.ps1                 # 전부(약 8분)
#   powershell -File Scripts\Dressing\shoot_meadow.ps1 -Only "D01|D02" # 일부만
#
# 순서(요소마다): 플레이어를 보고서 좌표의 그 소품 남쪽 8 m 로(Grim.Meadow GoNear — 스트리밍 · 반경 자극이 그 자리에) → 카메라 고정(Grim.Terrain.ViewFrom, 땅 위 cm) · 풀 25초 →
# 전 → 자극(Grim.Meadow Hit <속성> <세기> 1200 <PropId>) 또는 F(Grim.Meadow Interact) → 후. 시각은 낮(0.45)으로 고정, 등잔 · 모닥불은 늦은 오후(0.62) — 황혼은 자동 노출이 하얗게 날린다(BUG-008 계열).
param(
	[string]$Only = "",
	[int]$RunSeconds = 1200,
	[int]$ResX = 1600,
	[int]$ResY = 900
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$ShotDir = Join-Path $Root "Saved\TerrainCheck"
$OutDir = Join-Path $Root "docs\public\qa"
New-Item -ItemType Directory -Force $ShotDir | Out-Null
$Log = Join-Path $Root "Saved\MeadowPlace\shoot.log"
Remove-Item $Log -ErrorAction SilentlyContinue
$Report = Get-Content (Join-Path $Root "Saved\MeadowPlace\report.json") -Raw -Encoding UTF8 | ConvertFrom-Json

function First-Of([string]$Id, [int]$Nth = 0) {
	# "D27:Pink" 처럼 색으로도 고른다(꽃밭 · 꽃나무)
	$Parts = $Id -split ':'
	$Hits = @($Report.placed | Where-Object { $_.id -eq $Parts[0] -and ($Parts.Count -lt 2 -or $_.color -eq $Parts[1]) })
	if ($Hits.Count -le $Nth) { throw "보고서에 $Id $Nth 번째가 없다" }
	return $Hits[$Nth]
}

# 요소: 이름, PropId(Go 대상), 번째, 카메라 거리 cm, 카메라 높이(땅 위 cm), Pitch, 행동(Hit … 또는 Interact), 시각, 행동 뒤 기다림 초
$Elements = @(
	@("D01", "D01_WindCotton", 0, 260, 150, -20, "Hit Element.Wind 1.2 1200 D01_WindCotton", 0.45, 1.2),
	@("D02", "D02_DryGrass", 0, 420, 220, -22, "Hit Element.Fire 1 1200 D02_DryGrass", 0.45, 2.5),
	@("D02-burnt", "D02_DryGrass", 0, 420, 220, -22, "", 0.45, 0),
	@("D03", "D03_ThirstyFlower", 0, 260, 140, -22, "Hit Element.Water 0.6 1200 D03_ThirstyFlower", 0.45, 2.0),
	@("D04", "D04_StoneLamp", 1, 380, 160, -12, "Hit Element.Fire 1 1200 D04_StoneLamp", 0.62, 1.5),
	@("D20", "D20_Campfire", 0, 420, 190, -18, "Hit Element.Fire 1 1200 D20_Campfire", 0.62, 1.5),
	@("D16", "D16_Butterflies", 0, 260, 140, -4, "Hit Element.Wind 0.8 1200 D16_Butterflies", 0.45, 7.5),
	@("D21", "D21_Page", 4, 230, 150, -32, "Hit Element.Wind 1.5 1200 D21_Page", 0.45, 1.0),
	@("D29", "D29_FloatStone", 3, 300, 120, -6, "Hit Element.None 1 1200 D29_FloatStone", 0.45, 0.35),
	# 반응 없는 꾸밈(한 장씩): 색 꽃밭 · 꽃밭 위 나비 · 외딴 꽃나무
	@("D27-pink", "D27:Pink", 0, 700, 220, -14, "", 0.45, 0),
	@("D27-yellow", "D27:Yellow", 0, 700, 220, -14, "", 0.45, 0),
	@("D27-white", "D27:White", 0, 700, 220, -14, "", 0.45, 0),
	@("D26", "D26", 0, 3000, 700, -6, "", 0.45, 0)
)
if ($Only) { $Elements = @($Elements | Where-Object { $_[0] -match $Only }) }

$Cmds = @("r.ScreenPercentage 100", "Grim.UI.HUD 0")
# 이름 붙은 Grim.Terrain.ViewFrom 은 카메라를 옮긴 뒤 4 초 기다렸다 찍는다 — 「찍는 때 − 4」에 명령을 둔다
$T = 20
$Names = @()
foreach ($E in $Elements) {
	$P = First-Of $E[1] $E[2]
	$X = [double]$P.x - [double]$E[3]; $Y = [double]$P.y
	# 플레이어를 그 소품 남쪽 8 m 로(반경 자극 · 스트리밍이 그 자리에, 화면 밖) → 카메라를 먼저 옮겨 풀이 자랄 25 초를 기다린다
	$Cmds += "Grim.At $T Grim.Creature.Time $($E[7]) 0"
	$Cmds += "Grim.At $T Grim.Meadow GoNear $($P.x) $($P.y) 800"
	$Cmds += "Grim.Terrain.ViewFrom $X $Y +$($E[4]) 0 $($E[5]) $($T + 1)"
	if ($E[0] -eq "D02-burnt") {
		$Cmds += "Grim.Terrain.ViewFrom $X $Y +$($E[4]) 0 $($E[5]) $($T + 1) meadow-D02-burnt"
		$Names += "meadow-D02-burnt"
		$T += 7
		continue
	}
	if (-not $E[6]) {
		# 꾸밈 한 장: 풀이 자랄 25 초 뒤
		$Cmds += "Grim.Terrain.ViewFrom $X $Y +$($E[4]) 0 $($E[5]) $($T + 22) meadow-$($E[0])"
		$Names += "meadow-$($E[0])"
		$T += 28
		continue
	}
	$Cmds += "Grim.Terrain.ViewFrom $X $Y +$($E[4]) 0 $($E[5]) $($T + 22) meadow-$($E[0])-before"   # 찍힘 T+26
	$Names += "meadow-$($E[0])-before"
	$Act = $T + 29
	$Cmds += "Grim.At $Act Grim.Meadow $($E[6])"
	$Cmds += "Grim.Terrain.ViewFrom $X $Y +$($E[4]) 0 $($E[5]) $($Act + $E[8] - 4) meadow-$($E[0])-after"   # 찍힘 Act + 기다림
	$Names += "meadow-$($E[0])-after"
	$T = $Act + $E[8] + 3
}
if (-not $Only -or "D10" -match $Only) {
	# 곁가지 · 돌멩이: 첫 점 바로 앞으로(플레이어가 F 를 닿게) → 전 → F → 후
	foreach ($K in @(0, 1)) {
		$P = @($Report.placed | Where-Object { $_.id -eq "D10" -and $_.kind -like $(if ($K -eq 0) { "D10_Branch*" } else { "D10_Pebble*" }) })[0]
		$Name = if ($K -eq 0) { "D10-branch" } else { "D10-pebble" }
		$Cmds += "Grim.At $T Grim.Creature.Time 0.45 0"
		$Cmds += "Grim.At $T Grim.Meadow GoNear $($P.x) $($P.y) 120"
		$X = [double]$P.x + 230; $Y = [double]$P.y + 40   # 플레이어(남쪽 1.2 m)를 등지고 북쪽에서 본다
		$Cmds += "Grim.Terrain.ViewFrom $X $Y +150 190 -30 $($T + 1)"
		$Cmds += "Grim.Terrain.ViewFrom $X $Y +150 190 -30 $($T + 22) meadow-$Name-before"
		$Cmds += "Grim.At $($T + 28) Grim.Meadow Interact"
		$Cmds += "Grim.Terrain.ViewFrom $X $Y +150 190 -30 $($T + 26) meadow-$Name-after"
		$Names += @("meadow-$Name-before", "meadow-$Name-after")
		$T += 33
	}
}
# 전경 — 구간을 내려다보는 하늘 · 언덕 위 눈높이(지도 → 월드: X = (지도 Y − 380) × 100, Y = (지도 X − 1000) × 100)
$Vistas = @(
	@("meadow-vista-west", -1000, -32000, 6000, 30, -14),
	# 탑 앞마당(탑 시범 브랜치)과 그 바깥 초원이 함께 — 합친 뒤 둘 다 살아 있는지
	@("meadow-tower-yard", 16000, -3000, 600, 169, -8),
	@("meadow-tower-yard2", 14000, 12000, 300, 230, -6)
)
if (-not $Only -or "vista" -match $Only) {
	foreach ($V in $Vistas) {
		$Cmds += "Grim.At $T Grim.Creature.Time 0.45 0"
		$Cmds += "Grim.Terrain.ViewFrom $($V[1]) $($V[2]) +$($V[3]) $($V[4]) $($V[5]) $T"
		$Cmds += "Grim.Terrain.ViewFrom $($V[1]) $($V[2]) +$($V[3]) $($V[4]) $($V[5]) $($T + 18) $($V[0])"
		$Names += $V[0]
		$T += 25
	}
}
$Cmds = $Cmds -join ","
foreach ($N in $Names) { Remove-Item (Join-Path $ShotDir "$N.png") -ErrorAction SilentlyContinue }

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-RenderOffscreen", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
$Deadline = (Get-Date).AddSeconds($RunSeconds)
$Last = Join-Path $ShotDir "$($Names[-1]).png"
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline -and -not (Test-Path $Last)) { Start-Sleep -Seconds 2 }
Start-Sleep -Seconds 3
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Add-Type -AssemblyName System.Drawing
$Codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$Params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$Params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]80)
$Failed = 0
foreach ($N in $Names) {
	$Png = Join-Path $ShotDir "$N.png"
	if (-not (Test-Path $Png)) { Write-Output "[FAIL] $N — 스크린숏 없음"; $Failed++; continue }
	$Image = [System.Drawing.Image]::FromFile($Png)
	$Jpg = Join-Path $OutDir "$N.jpg"
	$Image.Save($Jpg, $Codec, $Params)
	$Image.Dispose()
	Write-Output "[OK]   $Jpg"
}
Select-String -Path $Log -Pattern "LogGrimDressing" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
