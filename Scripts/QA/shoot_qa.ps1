# QA 화면 확인 — docs/dev/qa.md 「검증 수단」 ③. RHI 를 켠 단독 게임으로 L_OpenBeta 를 열어 장면마다 UI 까지 찍고(shot showui),
# docs/public/qa/<이름>.jpg 로 줄여 둔다. 한 프로세스에서 Grim.At 시간표로 장면을 차례로 만든다.
#
#   powershell -File Scripts\QA\shoot_qa.ps1                 # 창 없이(-RenderOffscreen)
#   powershell -File Scripts\QA\shoot_qa.ps1 -Windowed       # 창을 띄워(오프스크린에서 UI 가 안 찍히면)
#
# 장면 = 이름, 찍는 초, 앞서 칠 명령(쉼표 없이 ; 로 — 1초 간격으로 차례로). 맵 로드(10~40초)를 넉넉히 둔다.
param(
	# Main = 기본 장면들, Pose = 시작 자리 앞 고정 카메라로 완드 · 그리모어 앞모습과 시전 순간(R21)
	[ValidateSet("Main", "Pose", "Fizzle", "Circle", "Creator", "Bag")]
	[string]$Set = "Main",
	[int]$Start = 45,
	[int]$ResX = 1600,
	[int]$ResY = 900,
	[switch]$Windowed,
	[int]$RunSeconds = 300
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
$ShotRoot = Join-Path $Root "Saved\Screenshots"
$ArtDir = Join-Path $Root "docs\public\qa"
New-Item -ItemType Directory -Force $OutDir, $ArtDir | Out-Null
$Log = Join-Path $OutDir "shoot.log"
Remove-Item $Log -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 이름, 준비 명령(;로 나눔), 준비 뒤 기다릴 초
$Scenes = @(
	@("qa-start-hud", "Grim.Creature.Time 0.45", 4),
	@("qa-grimoire-ui", "Grim.UI.Grimoire 1", 3),
	@("qa-supercheat", "Grim.UI.Grimoire 0;GrimSuperCheat 1", 3),
	@("qa-creature-mark", "Grim.Go Jackalope;Grim.Spawn Jack 3;Grim.Creature.Mark 1", 5),
	@("qa-night", "Grim.Creature.Mark 0;Grim.Creature.Time 0.02", 5),
	# 같은 밤을 자동 노출 없이 — 밤이 낮처럼 밝게 보이면 자동 노출이 끌어올린 것인지 가린다
	@("qa-night-fixed-exposure", "r.EyeAdaptationQuality 0", 3),
	@("qa-rain", "r.EyeAdaptationQuality 2;Grim.Creature.Time 0.45;Grim.Weather.Set All Rain 0", 8),
	# 줄 세우기 · 상자는 트인 초원에서(탑 방 안은 카메라가 벽에 붙고, 상자가 위층에 놓인다)
	@("qa-lineup", "Grim.Weather.Set All Clear 0;Grim.Appearance.Lineup Tower_Examiner 3", 5),
	@("qa-creature-box", "Grim.Taming.StorageLearn 1;Grim.Taming.SpawnBox 250;Grim.Taming.Box Open", 4),
	@("qa-tower-room", "Grim.Taming.Box Close;Grim.Go Tut_", 4),
	@("qa-creator", "GrimCreator", 4)
)
# 시작 자리(탑 안, 폰 (-150, 800, 4598) · +X 를 봄 — net_check_cheats 덤프)의 3 m 앞에서 뒤돌아본다
$PoseScenes = @(
	@("qa-pose-front", "Grim.Cam 160 800 4700 -12 180 60", 3),
	# 느린 시간(1/20)에서 불씨를 누르고 1초(게임 0.05초) 뒤 — 휘두름 · 책장 넘김 · 섬광
	@("qa-pose-cast", "slomo 0.05;Grim.Input.Press Slot1 None", 0)
)
if ($Set -eq "Pose") { $Scenes = $PoseScenes }
# 마법 실패 알림 · 피식 연출(Q64) — 같은 앞모습 카메라, 느린 시간(1/10)에서 누르고 1초(게임 0.1초) 뒤: HUD 위쪽 까닭 한 줄 + 완드 끝(손) 회색 터짐
$FizzleScenes = @(
	@("qa-castfail-mana", "Grim.Cam 160 800 4700 -12 180 60;Grim.Save.SetAttr Mana 0;slomo 0.1;Grim.Input.Press Slot1 None", 0),
	@("qa-castfail-nowand", "slomo 1;Grim.Base.Unequip 0;slomo 0.1;Grim.Input.Press Slot2 None", 0)
)
if ($Set -eq "Fizzle") { $Scenes = $FizzleScenes }
# 서클(Q122) — 2서클 책을 가방에서 읽어 봉인된 책으로 만든 뒤 그리모어 「서클」 탭(지금 · 다음 서클 보상 · 승급 자격 · 봉인된 책),
# 「마법」 탭(1서클이라 슬롯 세트 2~4 가 흐림), 숙련도를 채워 승급한 뒤 다시 「서클」 탭
$CircleScenes = @(
	@("qa-grimoire-circle", "Grim.Items.Give Skillbook_Fireball 1;Grim.Items.Eat Skillbook_Fireball;Grim.UI.GrimoireTab 3", 2),
	@("qa-grimoire-slotsets", "Grim.UI.GrimoireTab 0", 2),
	@("qa-grimoire-circle2", "Grim.Magic.AddXP Ember 100000;Grim.Magic.AddXP WaterJet 100000;Grim.Magic.CircleUp 0;Grim.UI.GrimoireTab 3", 2)
)
if ($Set -eq "Circle") { $Scenes = $CircleScenes }
# 생성 화면 카메라(Q102) — 정면 전신 · 얼굴 칸(얼굴 가까이) · 마우스 끌기 대신 돌림(옆 45°) · 닫으면 평소 카메라
$CreatorScenes = @(
	@("qa-creator", "Grim.Weather.Set All Clear 0;Grim.Creature.Time 0.45;GrimCreator", 4),
	@("qa-creator-face", "Grim.Appearance.CreatorFocus Face", 3),
	@("qa-creator-orbit", "Grim.Appearance.CreatorOrbit 45 -5", 3),
	@("qa-creator-body-orbit", "Grim.Appearance.CreatorFocus Body", 3)
)
if ($Set -eq "Creator") { $Scenes = $CreatorScenes }
# 가방 더블클릭 — 슬레이트 라우팅으로 진짜 칸 단추에 입력을 흘린다(Grim.Items.BagMouse). 고른 뒤 · 음식 더블클릭 · 재료 더블클릭. 판정은 shoot.log 의 [BagInput] · [ItemsDump] 줄
$BagScenes = @(
	@("qa-bag-selected", "Grim.Items.Clear;Grim.Items.Give RoastedJackalopeMeat 3;Grim.Items.Give JackalopeFur 3;Grim.Items.Hunger 30;Grim.Items.BagClick 0 1;Grim.Items.Dump", 1),
	@("qa-bag-doubleclick-food", "Grim.Items.BagMouse 0;Grim.Items.Dump", 0),
	@("qa-bag-doubleclick-material", "Grim.Items.BagMouse 1;Grim.Items.Dump", 0)
)
if ($Set -eq "Bag") { $Scenes = $BagScenes }

$Cmds = @("r.ScreenPercentage 100")
$T = $Start
foreach ($Scene in $Scenes) {
	foreach ($Prep in ($Scene[1] -split ";")) { $Cmds += "Grim.At $T $Prep"; $T += 1 }
	$T += $Scene[2]
	$Cmds += "Grim.At $T shot showui"
	$T += 3
}
$Cmds += "Grim.At $($T + 3) quit"
$Cmds = $Cmds -join ","

$Before = Get-Date
$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-GrimStartingSpells", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
if (-not $Windowed) { $GameArgs += "-RenderOffscreen" }
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
$Deadline = (Get-Date).AddSeconds($RunSeconds)
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) { Start-Sleep -Seconds 2 }
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

# 이번 실행에서 생긴 그림을 찍은 차례대로 장면 이름에 맞춘다
$Pngs = @(Get-ChildItem -Path $ShotRoot -Recurse -Filter *.png -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $Before } | Sort-Object LastWriteTime)
Add-Type -AssemblyName System.Drawing
$Codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$Params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$Params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]80)
$Failed = 0
for ($i = 0; $i -lt $Scenes.Count; $i++) {
	$Name = $Scenes[$i][0]
	if ($i -ge $Pngs.Count) { Write-Output "[FAIL] $Name — 스크린숏 없음"; $Failed++; continue }
	$Image = [System.Drawing.Image]::FromFile($Pngs[$i].FullName)
	# 문서용으로 가로 1280 에 맞춘다
	$W = [Math]::Min(1280, $Image.Width); $H = [int]($Image.Height * $W / $Image.Width)
	$Small = New-Object System.Drawing.Bitmap($Image, $W, $H)
	$Jpg = Join-Path $ArtDir "$Name.jpg"
	$Small.Save($Jpg, $Codec, $Params)
	$Small.Dispose(); $Image.Dispose()
	Write-Output "[OK]   $Jpg ← $($Pngs[$i].Name)"
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
