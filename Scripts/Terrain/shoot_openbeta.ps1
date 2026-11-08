# 지형 스크린숏 — RHI 를 켠 단독 게임(-game -RenderOffscreen, 창 없음)으로 L_OpenBeta 를 열어 정해 둔 자리에서 찍고,
# docs/public/art/terrain/ 에 jpg 로 둔다(사람 눈 확인용 — docs/dev/level-design.md 「6」 확인할 것 5 · 「7」 사람 눈높이).
#
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1              # 마탑 구간(-v3): 사람 눈높이 1.8 m
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set v2      # 2차 자리(전체 땅) — 이전 그림을 덮어쓰니 일부러만
#
# 자리(월드 cm, X = 북 = (지도 Y − 380) × 100, Y = 동 = (지도 X − 1000) × 100). Z 가 + 면 땅 위 cm(Grim.Terrain.ViewFrom).
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set dress -Tag before   # 레벨 아트 비교 — 측정 자리(Scripts/Perf/measure.ps1) 중 시작 구역
#                                                                               → docs/public/qa/level-dress-<자리>-<Tag>.jpg
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld2 -RunSeconds 900   # 레벨 디자인 2차(docs/dev/level-design.md 「9」) — 구간마다 눈높이 · 하늘
#                                                                               → docs/public/qa/ld2-<자리>.jpg(앞 다섯은 level-dress-*-after 와 같은 자리 = 전 · 후)
param(
	[ValidateSet("v3", "v2", "dress", "ld2")]
	[string]$Set = "v3",
	[string]$Tag = "after",
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
# dress — 레벨 아트 전 · 후 비교(docs/dev/level-design.md 「8」). 측정 자리와 같은 X · Y · Yaw(Scripts/Perf/measure.ps1), 눈높이 1.8 m
$ShotsDress = @(
	@("level-dress-door-$Tag", 4000, 0, "+180", 0, -2, $false),
	@("level-dress-path200-$Tag", 23246, 2090, "+180", -26, -2, $false),
	@("level-dress-pond-$Tag", 12500, -30000, "+180", -5, -3, $false),
	@("level-dress-hilltop-$Tag", 56800, -11000, "+180", 140, -4, $false),
	@("level-dress-path100-$Tag", 13776, 3029, "+180", 25, -2, $false),
	# Q124 비교(측정 자리 아님) — 사실적 스캔(쓰러진 나무)이 StyleHex 옆에 보이는 자리(dress_openbeta.py 로그의 소품 자리).
	# 그루터기 · Megaplants 참나무는 시험 뒤 맵에서 뺐다 — 그때 그림(level-dress-scan-stump-after · q125-oak-*)은 docs/public/qa 에 남긴다
	@("level-dress-compare-east-$Tag", 29000, 33500, "+170", 50, -4, $false),
	@("level-dress-compare-pond-$Tag", 14000, -27000, "+170", 50, -3, $false),
	@("level-dress-scan-trunk-$Tag", 1700, -12500, "+170", -40, -12, $false)    # 쓰러진 나무 스캔(Dress_FallenTrunk_01) 12 m 앞
)
# ld2 — 레벨 디자인 2차: 능선 · 안부 · 자연 분포 · 구간 구도 · 하늘 구역 이름 · 고목 큐브 목업. 지도 → 월드: X = (지도 Y − 380) × 100, Y = (지도 X − 1000) × 100
$ShotsLd2 = @(
	@("ld2-door", 4000, 0, "+180", 0, -2, $false),                      # 탑 문 앞 — level-dress-door-after 와 같은 자리(전 · 후)
	@("ld2-path100", 13776, 3029, "+180", 25, -2, $false),              # 큰길 100 m — 돌담 · 돌 표지 · 능선(전 · 후)
	@("ld2-path200", 23246, 2090, "+180", -26, -2, $false),             # 큰길 200 m — 연못 갈림길(전 · 후)
	@("ld2-pond", 12500, -30000, "+180", -5, -3, $false),               # 서쪽 연못 — 징검돌 · 섬 · 부유석(전 · 후)
	@("ld2-hilltop", 56800, -11000, "+180", 140, -4, $false),           # 봉화 언덕 꼭대기에서 남동(전 · 후)
	@("ld2-path400", 41318, -1871, "+180", 32, -2, $false),             # 큰길 400 m — 꽃길 초원 · 숲덩이
	@("ld2-westspine", 25000, -47000, "+180", 62, -4, $false),          # 서쪽 등줄기 등성이(지도 530, 630)에서 동북동 — 연못 골 · 탑
	@("ld2-eastsaddle", 22000, 36000, "+180", 0, 2, $false),             # 남동 숨은 골(지도 1360, 600)에서 북쪽 — 동쪽 안부(고갯마루) · 그 너머 풍차
	@("ld2-hiddenvale", 6000, 42000, "+180", 35, -3, $false),           # 남동 숨은 골(지도 1420, 440) — 두 능선 사이 들
	@("ld2-windmill", 33500, 28500, "+180", 45, 2, $false),             # 동쪽 길에서 풍차 마당 — 밀밭 · 울타리 · 곳간
	@("ld2-floatsteps", 31000, -34000, "+180", -42, 12, $false),        # 연못 길(지도 660, 690)에서 부유석 언덕 · 디딤돌
	@("ld2-tree-far", 114000, -4000, "+180", 17, 3, $false),            # 큰길(지도 960, 1520)에서 거대 고목(약 480 m)
	@("ld2-tree-near", 151500, 1500, "+180", 45, 10, $false),           # 고목 남서 120 m — 판뿌리 · 줄기 · 가지 · 잎 덩어리
	@("ld2-tree-under", 157800, 8000, "+180", 45, 28, $false),          # 고목 남서 30 m 에서 올려다봄 — 판뿌리 · 줄기 마디 · 가지
	@("ld2-labels-door", 4000, 0, "+180", 0, 14, $false),               # 탑 문 앞에서 하늘 — 구역 이름
	@("ld2-aerial-south", -26000, 0, "28000", 0, -20, $false),          # 바다 위 280 m 에서 북쪽 비스듬히 — 콘셉트 r1-key-aerial 구도
	@("ld2-aerial-region", -60000, 2000, "70000", 0, -24, $false),      # 바다 위 700 m — 지역 전체 · 하늘 구역 이름
	@("ld2-aerial-tree", 130000, 10000, "+22000", 0, -32, $false),      # 고목 남쪽 하늘 220 m — 중부 구릉 · 고목 실루엣
	@("ld2-sector-aerial", 21000, 9000, "125000", 0, -89.9, $false)     # 하늘 1.25 km 에서 마탑 구간(openbeta-sector-aerial-v3 와 같은 자리)
)
$Shots = if ($Set -eq "v2") { $ShotsV2 } elseif ($Set -eq "dress") { $ShotsDress } elseif ($Set -eq "ld2") { $ShotsLd2 } else { $ShotsV3 }
if ($Set -eq "dress" -or $Set -eq "ld2") { $ArtDir = Join-Path $Root "docs\public\qa" }

$Cmds = @()
$Delay = 25
foreach ($Shot in $Shots) {
	if ($Set -eq "dress" -or $Set -eq "ld2") {
		# 랜드스케이프 풀은 카메라 둘레에서 비동기로 자란다 — 먼저 옮겨 두고 25 초 뒤에 찍는다
		$Cmds += "Grim.Terrain.ViewFrom $($Shot[1]) $($Shot[2]) $($Shot[3]) $($Shot[4]) $($Shot[5]) $Delay"
		$Delay += 25
	}
	$Cmds += "Grim.Terrain.ViewFrom $($Shot[1]) $($Shot[2]) $($Shot[3]) $($Shot[4]) $($Shot[5]) $Delay $($Shot[0])"
	$Delay += $(if ($Set -eq "dress" -or $Set -eq "ld2") { 5 } else { 12 })
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
