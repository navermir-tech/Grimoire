# 거점 · 소지품 화면 찍기(창 모드, L_M1_Sandbox): 혼자 거점 핵 + 집 두 칸(바닥 · 벽 · 문 · 지붕) + 시설(침대 · 모닥불 · 저장고 · 작업대 · 밭 · 생물 상자)을 짓고,
# 재칼로프 둘을 길들여 하나는 보관함에 넣은 뒤 소지품 화면의 탭마다(가방 · 장비 · 생물 · 저장고 · 작업대 · 거점) + 생물 상자 화면 + 집 바깥을 찍는다.
# 헤드리스 확인(net_check_base · net_check_storage)이 못 보는 글꼴 · 배치 · 색을 사람이 보려고.
#
#   powershell -File Scripts\Base\capture_base.ps1                          # → Saved\BaseCapture\NN_<이름>.png, 로그 Saved\BaseCapture\capture.log
#   powershell -File Scripts\Base\capture_base.ps1 -Cam "X Y Z 피치 요"       # 집 바깥 장면의 카메라(Grim.Cam). none 이면 캐릭터 카메라.
#
# 거점 핵은 BugItGo 로 늘 (1200, 0) · 요 0 에 놓인다 — 칸은 300 cm, 집 = 칸 (2,0) · (3,0), 문 = 동쪽(+X). 로그 「[BaseDump Server] core …」로 확인.
# UI 가 든 장은 「shot showui」(화면 그대로), 집 바깥은 HighResShot.
param(
	# BugItGo 로 거점 핵 = (1200, 0) · 요 0 이 되게 해 두었다(칸 = +X · +Y 로 300 cm). 남동쪽에서 집(문 쪽) · 생물 상자 · 밭 · 작업대가 다 보이는 자리
	[string]$Cam = "3050 -1650 900 -23 135 70",
	[int]$ResX = 1600,
	[int]$ResY = 900,
	[int]$Timeout = 300
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\..\.."
$UE = "C:\Program Files\Epic Games\UE_5.8"
$Project = "$Root\Grimoire.uproject"
$Out = "$Root\Saved\BaseCapture"
$ShotDir = "$Root\Saved\Screenshots\WindowsEditor"
$Log = "$Out\capture.log"
New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item "$Out\*" -Force -ErrorAction SilentlyContinue
if (Test-Path $ShotDir) { Remove-Item "$ShotDir\*" -Force -ErrorAction SilentlyContinue }

# (초, 명령) — 초는 맵이 뜬 뒤부터(Grim.At)
$Steps = @(
	@(6, "GrimSuperCheat 1"), @(6.2, "grim.Time.Set 11"), @(6.4, "Grim.Weather.Set All Clear 0"), @(6.6, "Grim.Taming.ForceRoll 0"),
	# 길들이기 · 보관함(R38)
	@(8, "Grim.Taming.Grant 1"),
	@(9, "Grim.Creature.Spawn Jackalope 1 0 600"), @(11, "Grim.Taming.Cast DA_Spell_Pactum Jackalope 0"),
	@(13, "Grim.Creature.Spawn Jackalope 1 0 600"), @(15, "Grim.Taming.Cast DA_Spell_Pactum Jackalope 0"),
	@(17, "Grim.Items.Give Skillbook_CreatureStorage 1"), @(17.5, "Grim.Items.Eat Skillbook_CreatureStorage"),
	@(19, "Grim.Taming.Storage Party Storage Jackalope"),
	# 재료 · 장비
	@(20, "Grim.Items.Give Wood 50"), @(20.2, "Grim.Items.Give Wood 50"), @(20.4, "Grim.Items.Give Stone 40"), @(20.6, "Grim.Items.Give JackalopeFur 8"),
	@(20.8, "Grim.Items.Give GrainSeed 5"), @(21, "Grim.Items.Give FurHood 1"), @(21.2, "Grim.Items.Give JackalopeMeat 3"),
	@(21.4, "Grim.Base.LearnRepair"), @(21.6, "Grim.Base.Equip FurHood"),
	# 집(net_check_base 와 같은 배치) + 시설
	# 거점 핵 자리를 늘 같게: 길들이기로 돈 방향과 상관없이 원점에서 +X 를 보고 놓는다
	@(21.7, "EnableCheats"), @(21.8, "BugItGo 0 0 200 0 0 0"), @(22, "Grim.Base.Core 1200"),
	@(23, "Grim.Base.Place WoodFoundation 2 0 0 0"), @(23.5, "Grim.Base.Place WoodFoundation 3 0 0 0"),
	@(24, "Grim.Base.Place WoodWall 1 0 0 0"), @(24.5, "Grim.Base.Place WoodWall 2 -1 0 1"), @(25, "Grim.Base.Place WoodWall 2 0 0 1"),
	@(25.5, "Grim.Base.Place WoodWall 3 -1 0 1"), @(26, "Grim.Base.Place WoodWall 3 0 0 1"), @(26.5, "Grim.Base.Place WoodDoor 3 0 0 0"),
	@(27, "Grim.Base.PlaceAt Bed 2 0 0 -80 0"), @(27.5, "Grim.Base.PlaceAt Campfire 2 0 0 80 0"), @(28, "Grim.Base.PlaceAt StorageChest 3 0 0 90 0"),
	@(28.5, "Grim.Base.Place WoodRoof 2 0 0 0"), @(29, "Grim.Base.Place WoodRoof 3 0 0 0"),
	@(29.5, "Grim.Base.PlaceAt Workbench 0 -1 0 0 0"), @(30, "Grim.Base.PlaceAt CropPlot 2 -3 0 0 0"), @(30.5, "Grim.Base.PlaceAt CreatureBox 1 -2 0 100 0"),
	@(31, "Grim.Base.Store Wood 10"),
	@(32, "Grim.Base.Dump"), @(32.5, "Grim.Taming.StorageDump 0 Local"),
	# 소지품 화면 탭마다(저장고는 집 안, 작업대 · 거점 · 생물 상자는 칸 1,-1 에서)
	@(33, "Grim.Base.Teleport 3 0 0"),
	@(36, "Grim.Base.Bag Bag"), @(37.5, "shot showui"),
	@(38, "Grim.Base.Bag Equipment"), @(39, "shot showui"),
	@(40, "Grim.Base.Bag Creatures"), @(41.5, "shot showui"),
	@(43, "Grim.Base.Bag Storage"), @(44, "shot showui"),
	@(45, "Grim.Base.Bag Close"), @(45.5, "Grim.Base.Teleport 1 -1 0"),
	@(47, "Grim.Base.Bag Workbench"), @(48, "shot showui"),
	@(49, "Grim.Base.Bag Base"), @(50, "shot showui"),
	@(51, "Grim.Base.Bag Close"), @(52, "Grim.Taming.Box Open"), @(53.5, "shot showui"),
	@(54.5, "Grim.Taming.Box Close")
)
if ($Cam -and $Cam -ne "none") { $Steps += , @(57, "Grim.Cam $Cam") }
$Steps += @(@(59, "HighResShot ${ResX}x${ResY}"), @(61, "Grim.Base.Door GrimBuildPiece_7"), @(63, "HighResShot ${ResX}x${ResY}"))
$Names = @("bag", "equipment", "creatures", "storage", "workbench", "base", "creature_box", "house", "house_door_open")
$exec = ($Steps | ForEach-Object { "Grim.At $($_[0]) $($_[1])" }) -join ","

$p = Start-Process -FilePath "$UE\Engine\Binaries\Win64\UnrealEditor.exe" -PassThru -ArgumentList @(
	"`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox", "-game", "-GrimStartingSpells", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosound", "-nosplash",
	"-unattended", "-nop4", "-log", "-abslog=`"$Log`"", "-ExecCmds=`"$exec`"")
$deadline = (Get-Date).AddSeconds($Timeout)
while (-not $p.HasExited -and (Get-Date) -lt $deadline -and @(Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue).Count -lt $Names.Count) { Start-Sleep 2 }
Start-Sleep 3
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }

$i = 0
Get-ChildItem $ShotDir -Filter *.png -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | ForEach-Object {
	$Name = if ($i -lt $Names.Count) { $Names[$i] } else { "extra" }
	Copy-Item $_.FullName ("$Out\{0:D2}_{1}.png" -f $i, $Name)
	$i++
}
Write-Host "찍은 장 수: $i → $Out"
Select-String -Path $Log -Pattern "BaseDump Server\] core|Grim.Base.Bag|StorageDump|Warning: .*Grim" | Select-Object -First 20 | ForEach-Object { $_.Line }
