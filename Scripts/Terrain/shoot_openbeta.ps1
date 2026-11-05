# 지형 스크린숏 — RHI 를 켠 단독 게임(-game -RenderOffscreen, 창 없음)으로 L_OpenBeta 를 열어 정해 둔 자리에서 찍고,
# docs/public/art/terrain/ 에 jpg 로 둔다(사람 눈 확인용 — docs/dev/level-design.md 「6」 확인할 것 5 · 「7」 사람 눈높이).
#
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1              # 마탑 구간(-v3): 사람 눈높이 1.8 m
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set v2      # 2차 자리(전체 땅) — 이전 그림을 덮어쓰니 일부러만
#
# 자리(월드 cm, X = 북 = (지도 Y − 380) × 100, Y = 동 = (지도 X − 1000) × 100). Z 가 + 면 땅 위 cm(Grim.Terrain.ViewFrom).
param(
	[ValidateSet("v3", "v2")]
	[string]$Set = "v3",
	[int]$RunSeconds = 200,
	[int]$ResX = 1600,
	[int]$ResY = 900
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$ShotDir = Join-Path $Root "Saved\TerrainCheck"
$ArtDir = Join-Path $Root "docs\public\art\terrain"
New-Item -ItemType Directory -Force $ShotDir | Out-Null
$Log = Join-Path $ShotDir "shoot.log"
Remove-Item $Log -ErrorAction SilentlyContinue

# 이름, X, Y, Z, Yaw, Pitch, 그림 돌리기(위에서 본 그림을 북쪽이 위로)
# v3 — 마탑 구간(docs/dev/level-design.md 「7」): 눈높이 1.8 m(+180). 큰길 자리는 레이아웃 Path_Main 곡선에서 탑 문 앞부터 잰 거리, Yaw = 길 방향
$ShotsV3 = @(
	@("openbeta-door-v3", 4000, 0, "+180", 0, -2, $false),               # 탑 문 앞(지도 1000, 420)에서 북쪽 — 앞마당 · 큰길 · 언덕들 · 글씨
	@("openbeta-path100-v3", 13776, 3029, "+180", 25, -2, $false),      # 큰길 100 m(지도 1030, 518)
	@("openbeta-path200-v3", 23246, 2090, "+180", -26, -2, $false),     # 큰길 200 m(지도 1021, 612) — 연못 갈림길 앞
	@("openbeta-path400-v3", 41318, -1871, "+180", 32, -2, $false),     # 큰길 400 m(지도 981, 793)
	@("openbeta-hilltop-v3", 56800, -11000, "+180", 140, -4, $false),   # 봉화 언덕 꼭대기(지도 890, 948 — 전망 바위 옆)에서 남동 — 탑 · 풍차 · 바다
	@("openbeta-pond-v3", 12500, -30000, "+180", -5, -3, $false),       # 연못 남쪽 물가(지도 700, 505)에서 북 — 연못 · 부유석 · 봉화 언덕
	@("openbeta-fence-v3", 62000, 10000, "+180", 10, -2, $false),       # 큰길 북쪽 끝(지도 1100, 1000) — 개울 · 공사 중 경계
	@("openbeta-sector-aerial-v3", 21000, 9000, "125000", 0, -89.9, $false) # 하늘 1.25 km 에서 마탑 구간(화면 위 = 북쪽)
)
$ShotsV2 = @(
	@("openbeta-tower-north", 5000, 0, "+170", 0, -3, $false),          # 탑 문 앞(지도 1000, 430)에서 북쪽 — 고목 · 풍차 · 멀리 선돌 고리
	@("openbeta-tree-north", 161400, 10000, "+300", 0, -4, $false),     # 고목 언덕 꼭대기(고목 둘레 14 m)에서 북 — 계곡 · 폭포 · 고개
	@("openbeta-tree-east", 160000, 11400, "+300", 90, -5, $false),     # 동 — 강 · 감시탑
	@("openbeta-tree-south", 158600, 10000, "+300", 180, -4, $false),   # 남 — 목초지 · 탑 · 바다
	@("openbeta-tree-west", 160000, 8600, "+300", 270, -5, $false),     # 서 — 연못 저지대 · 서쪽 산등성이
	@("openbeta-valley-entrance", 232000, 6000, "+200", 57, -8, $false), # 큰길이 계곡 가장자리에 닿는 곳(지도 1060, 2700)에서 폭포 쪽
	@("openbeta-highland", 319000, -20000, "+200", 32, -2, $false),     # 북부 고지(지도 800, 3570)에서 고개 바위 문 쪽
	@("openbeta-aerial", 165200, 1600, "290000", -90, -89.9, $true)      # 하늘 2.9 km 에서 내려다본 전체(화면 위 = 서쪽 → 돌려서 북쪽이 위)
)
$Shots = if ($Set -eq "v2") { $ShotsV2 } else { $ShotsV3 }

$Cmds = @()
$Delay = 25
foreach ($Shot in $Shots) {
	$Cmds += "Grim.Terrain.ViewFrom $($Shot[1]) $($Shot[2]) $($Shot[3]) $($Shot[4]) $($Shot[5]) $Delay $($Shot[0])"
	$Delay += 12
}
$Cmds = $Cmds -join ","
foreach ($Shot in $Shots) { Remove-Item (Join-Path $ShotDir "$($Shot[0]).png") -ErrorAction SilentlyContinue }

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-RenderOffscreen", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-abslog=`"$Log`"", "-ExecCmds=`"r.ScreenPercentage 100,$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
$Deadline = (Get-Date).AddSeconds($RunSeconds)
$Last = Join-Path $ShotDir "$($Shots[-1][0]).png"
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline -and -not (Test-Path $Last)) { Start-Sleep -Seconds 2 }
Start-Sleep -Seconds 3
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Add-Type -AssemblyName System.Drawing
$Codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$Params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$Params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]82)
$Failed = 0
foreach ($Shot in $Shots) {
	$Png = Join-Path $ShotDir "$($Shot[0]).png"
	if (-not (Test-Path $Png)) { Write-Output "[FAIL] $($Shot[0]) — 스크린숏 없음"; $Failed++; continue }
	$Image = [System.Drawing.Image]::FromFile($Png)
	if ($Shot[6]) { $Image.RotateFlip([System.Drawing.RotateFlipType]::Rotate270FlipNone) }
	$Jpg = Join-Path $ArtDir "$($Shot[0]).jpg"
	$Image.Save($Jpg, $Codec, $Params)
	$Image.Dispose()
	Write-Output "[OK]   $Jpg"
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
