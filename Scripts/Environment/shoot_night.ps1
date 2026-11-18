# 밤 화면 확인(Q101 · BUG-008) — docs/dev/qa.md 「검증 수단」 ③. RHI 를 켠 단독 게임으로 L_OpenBeta 를 열어
# 낮 · 해질녘 넷 · 밤 · 불씨(섬광 · 터짐) · 탑 앞 밤 · 모닥불 · 탑 안(밤 · 낮)을 같은 자리 · 같은 카메라로 찍고
# docs/public/qa/night-<장면>-<Tag>.jpg 로 줄여 둔다. 고치기 전후를 -Tag before / after 로 나눠 찍어 나란히 본다.
#
#   powershell -File Scripts\Environment\shoot_night.ps1 -Tag after
#   powershell -File Scripts\Environment\shoot_night.ps1 -Tag try1 -Only night,campfire -ExtraCmds "Grim.Sky.Set NightExposureMinEV100 -1"
#
# 시각은 하루 길이를 아주 길게 늘려 멈춰 둔다(찍는 동안 해가 움직이지 않게). 판정은 그림을 직접 열어 본다.
param(
	[string]$Tag = "after",
	# 찍을 장면 이름(쉼표). 비우면 전부
	[string]$Only = "",
	# 맨 앞에 더할 콘솔 명령(쉼표로) — 값을 바꿔 가며 볼 때
	[string]$ExtraCmds = "",
	[int]$Start = 45,
	[int]$ResX = 1600,
	[int]$ResY = 900,
	[int]$RunSeconds = 420,
	# 문서 그림 폴더 대신 다른 곳에(값을 고르는 동안의 시험 그림)
	[string]$OutDir = "",
	# 불씨 장면: 누른 뒤 찍을 때까지 실제 초(느린 시간 1/20) — 섬광 · 터짐
	[int]$EmberCast = 2,
	[int]$EmberHit = 4,
	# 불씨 순간 고르기: 누른 뒤 2 · 4 · … · 20초에 하나씩 찍는다(이 장면들만)
	[switch]$EmberProbe
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$LogDir = Join-Path $Root "Saved\QA"
$ShotRoot = Join-Path $Root "Saved\Screenshots"
$ArtDir = if ($OutDir) { $OutDir } else { Join-Path $Root "docs\public\qa" }
New-Item -ItemType Directory -Force $LogDir, $ArtDir | Out-Null
$Log = Join-Path $LogDir "shoot-night-$Tag.log"
Remove-Item $Log -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 큰길 200 m(성능 측정 「path200」 자리) — 트인 초원 · 나무 · 먼 능선이 함께 보인다. 탑 문 앞은 「door」 자리.
$Road = "Grim.Perf.Goto 23246 2090 -26 -8"
$Door = "Grim.Perf.Goto 4000 0 180 -5"
$RoadDown = "Grim.Perf.Goto 23246 2090 -26 -20"
$Day = "Grim.Creature.Time 0.45"
$Night = "Grim.Creature.Time 0.02"
# 탑 안 홀(스승 앞) — 고정 카메라로 동쪽 문 곁에서 서쪽을 본다(따라오는 카메라는 벽에 붙는다). 좌표는 Scripts/Tutorial/tower_greybox.py 의 탑 기준(바닥 Z 4500)
$Hall = "Grim.Go Npc_Master;Grim.Cam 1350 -250 4720 -8 167 75"
# 이름, 준비 명령(;로 나눔 — 1초 간격, 「wait N」 은 N초 쉼), 준비 뒤 기다릴 초(자동 노출 · 스트리밍이 가라앉게).
# 장면마다 시각 · 자리를 스스로 정한다 — -Only 로 몇 개만 골라 찍어도 같은 그림이 나온다. 찍은 뒤에는 느린 시간 · 고정 카메라를 되돌린다.
$Scenes = @(
	@("night-day", "$Day;$Road", 14),
	# 해넘이 19:30(0.8125) 앞뒤 — 해가 낮게 뜬 때 · 지평선 · 막 진 뒤 · 어스름(해가 약 8° 아래). 화면이 튀지 않고 차례로 어두워지는지
	@("night-dusk-a", "Grim.Creature.Time 0.80;$Road", 10),
	@("night-dusk-b", "Grim.Creature.Time 0.8125;$Road", 10),
	@("night-dusk-c", "Grim.Creature.Time 0.819;$Road", 10),
	@("night-dusk-d", "Grim.Creature.Time 0.83;$Road", 10),
	@("night-meadow", "$Night;$Road", 12),
	# 불씨 — 땅을 겨눠(피치 -20, 약 3 m 앞에 맞는다) 느린 시간(1/20)에서 누른 뒤 실제 2초(게임 0.1초) = 완드 섬광,
	# 4초(게임 0.2초) = 터짐. 순간은 -EmberProbe 로 골랐다(2026-11-18)
	@("night-ember-cast", "$Night;$RoadDown;wait 10;slomo 0.05;Grim.Input.Press Slot1 None;wait $($EmberCast)", 0),
	@("night-ember-hit", "$Night;$RoadDown;wait 10;slomo 0.05;Grim.Input.Press Slot1 None;wait $($EmberHit)", 0),
	@("night-tower", "$Night;$Door", 14),
	# 모닥불은 폰 앞 2 m 에 놓인다 — 옆에서 고정 카메라로 캐릭터와 불을 함께 본다
	@("night-campfire", "$Night;$Road;Grim.Items.SpawnFire;Grim.Cam 23556 2496 3030 -12 -116 70", 12),
	@("night-room-night", "$Night;$Hall", 12),
	@("night-room-day", "$Day;$Hall", 10)
)
if ($EmberProbe) {
	$Scenes = @()
	foreach ($W in 2, 4, 6, 8, 10, 12, 14, 16, 18, 20) {
		$Scenes += , @("night-ember-probe$W", "$Night;$RoadDown;wait 10;slomo 0.05;Grim.Input.Press Slot1 None;wait $W", 0)
	}
}
if ($Only) {
	$Names = $Only.Split(",") | ForEach-Object { "night-" + $_.Trim() }
	$Scenes = @($Scenes | Where-Object { $Names -contains $_[0] })
}

$Cmds = @("r.ScreenPercentage 100", "grim.Time.DayLength 100000", "GrimSuperCheat 1", "DisableAllScreenMessages")
if ($ExtraCmds) { $Cmds += $ExtraCmds.Split(",") }
$T = $Start
foreach ($Scene in $Scenes) {
	foreach ($Prep in ($Scene[1] -split ";")) {
		if ($Prep -match '^wait (\d+)$') { $T += [int]$Matches[1]; continue }
		$Cmds += "Grim.At $T $Prep"; $T += 1
	}
	$T += $Scene[2]
	$Cmds += "Grim.At $T shot showui"
	$Cmds += "Grim.At $($T + 1) slomo 1"
	$Cmds += "Grim.At $($T + 1) Grim.Cam"
	$T += 3
}
$Cmds += "Grim.At $($T + 3) quit"
$Cmds = $Cmds -join ","

$Before = Get-Date
$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-GrimStartingSpells", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-RenderOffscreen", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
Write-Output "[밤 화면] $($Scenes.Count) 장면, 약 $($T + 10) 초 뒤 끝남(PID $($Game.Id))"
$Deadline = (Get-Date).AddSeconds([Math]::Max($RunSeconds, $T + 240))
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) { Start-Sleep -Seconds 2 }
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

# 이번 실행에서 생긴 그림을 찍은 차례대로 장면 이름에 맞춘다
$Pngs = @(Get-ChildItem -Path $ShotRoot -Recurse -Filter *.png -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $Before } | Sort-Object LastWriteTime)
Add-Type -AssemblyName System.Drawing
$Codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$Params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$Params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]82)
$Failed = 0
for ($i = 0; $i -lt $Scenes.Count; $i++) {
	$Name = $Scenes[$i][0]
	if ($i -ge $Pngs.Count) { Write-Output "[FAIL] $Name — 스크린숏 없음"; $Failed++; continue }
	$Image = [System.Drawing.Image]::FromFile($Pngs[$i].FullName)
	# 문서용으로 가로 1280 에 맞춘다
	$W = [Math]::Min(1280, $Image.Width); $H = [int]($Image.Height * $W / $Image.Width)
	$Small = New-Object System.Drawing.Bitmap($Image, $W, $H)
	# 평균 밝기(0~255, 화면 가운데 60% — HUD 를 뺀 장면) — 전후를 숫자로도 견준다
	$Sum = 0.0; $N = 0
	for ($y = [int]($H * 0.2); $y -lt [int]($H * 0.8); $y += 8) {
		for ($x = [int]($W * 0.2); $x -lt [int]($W * 0.8); $x += 8) {
			$C = $Small.GetPixel($x, $y); $Sum += 0.2126 * $C.R + 0.7152 * $C.G + 0.0722 * $C.B; $N++
		}
	}
	$Jpg = Join-Path $ArtDir "$Name-$Tag.jpg"
	$Small.Save($Jpg, $Codec, $Params)
	$Small.Dispose(); $Image.Dispose()
	Write-Output ("[OK]   {0} 평균 밝기 {1:N1}" -f $Jpg, ($Sum / $N))
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
