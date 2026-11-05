# M4 거점 2인 확인(헤드리스): net_check.ps1 과 같은 방식(리슨 서버 + 클라이언트, -game -nullrhi), 맵 L_M1_Sandbox.
# 완료 기준 「비 오는 밤을 집 안에서 보낸다」 — 두 사람이 나눠서 집 한 채(2칸)를 짓고, 비 · 22시에 집 안 침대에서 자서 밤을 넘긴다.
# 그 뒤 문 · 저장고 · 작업대 제작 · 장비 닳기 · 「수리」(장비 · 벽) · 밭(심기 · 비에 젖음 · 거두기) · 철거를 양쪽 덤프로 남긴다.
#
#   powershell -File Scripts\Base\net_check_base.ps1     # 결과: Saved\NetCheck\base_server.log · base_client.log 의 요약
#
# 시계: 모든 명령은 Grim.Base.AfterJoin 2 <초> — 「둘이 다 들어온 때」부터 잰다(바쁜 기계에서 부팅이 1분 넘게 걸려도 순서가 맞게).
# 스크립트는 서버 맵이 뜬 뒤 클라이언트를 띄우고, 클라이언트가 들어온 뒤 RunSeconds 를 기다린다.
# 격자(거점 핵 = 칸 0,0 · 호스트 앞 12 m — 샌드박스 소품을 피해): 집 = 칸 (2,0) · (3,0). 벽 = 서 (1,0,0,0) · 남북 (2,-1,0,1) (2,0,0,1) (3,-1,0,1) (3,0,0,1), 문 = 동 (3,0,0,0).
# 시간표(J = 둘 다 들어온 뒤 초):
#   J5~  호스트: 재료 · 「수리」 배우기 · 거점 핵 · 바닥 2 · 벽 3 · 침대 · 모닥불      기대: 양쪽에 같은 조각(서버 이름 GrimBuildPiece_0~4)
#   J20~ 클라이언트: 재료 · 비켜서기 · 벽 2 · 문 · 침대 · 저장고 · 지붕 2 · 작업대 · 밭   기대: 클라이언트가 지은 것도 양쪽에(조각 _5~_9)
#   J40 모닥불 점화 · 22시 · 비 · J42 두 사람 집 안으로
#   J52 덤프   기대: 둘 다 sheltered=1 wet=0 weather=3(Rain), 집 안 모닥불 burning>0 · sheltered=1, 밭(밖) wet>0
#   J55 둘 다 눕기 → 3초 페이드 → 밤 건너뜀   기대: J64 덤프 time≈0.2 day=1 sleeping=0
#   J66 문 열기(조각 _7) · J68 클라이언트가 저장고에 나무 5 · J70 호스트가 꺼냄 3
#   J72 털 두건 만들기 · 입기 · 100 닳게 · J75 작업대에 「수리」   기대: FurHood r1, 내구도 회복
#   J76 벽(_2) 피해 100 · J77 「수리」   기대: 벽 50 → 150(나무 3)
#   J79 곡물 씨앗 · J80 심기 · J82 자라기 3000초 · J84 거두기   기대: 가방 Grain +3 · GrainSeed +1
#   J86 문 철거   기대: 나무 2 환급, 지붕은 그대로(벽이 받침)
param(
	[int]$Port = 17791,
	[int]$RunSeconds = 105,
	[int]$BootTimeout = 240
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "base_server.log"
$ClientLog = Join-Path $OutDir "base_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

function J([double]$Seconds, [string]$Command) { "Grim.Base.AfterJoin 2 $Seconds $Command" }

$ServerCmds = @(
	(J 5 "Grim.Items.Give Wood 50"),
	(J 5.3 "Grim.Items.Give Wood 40"),
	(J 5.6 "Grim.Items.Give Stone 30"),
	(J 5.9 "Grim.Items.Give JackalopeFur 8"),
	(J 7 "Grim.Base.LearnRepair"),
	(J 8 "Grim.Base.Core 1200"),
	(J 10 "Grim.Base.Place WoodFoundation 2 0 0 0"),
	(J 11 "Grim.Base.Place WoodFoundation 3 0 0 0"),
	(J 12 "Grim.Base.Place WoodWall 1 0 0 0"),
	(J 13 "Grim.Base.Place WoodWall 2 -1 0 1"),
	(J 14 "Grim.Base.Place WoodWall 2 0 0 1"),
	(J 15 "Grim.Base.PlaceAt Bed 2 0 0 -80 0"),
	(J 16 "Grim.Base.PlaceAt Campfire 2 0 0 80 0"),
	(J 18 "Grim.Base.Dump"),
	(J 40 "Grim.Interaction.Stimulus Element.Fire 1 0 GrimBaseCampfire"),
	(J 40.5 "grim.Time.Set 22"),
	(J 41 "Grim.Weather.Set All Rain 0"),
	(J 42 "Grim.Base.Teleport 2 0 0"),
	(J 52 "Grim.Base.Dump"),
	(J 55 "grim.Survival.Sleep"),
	(J 64 "Grim.Base.Dump"),
	(J 66 "Grim.Base.Door GrimBuildPiece_7"),
	(J 70 "Grim.Base.Take Wood 3"),
	(J 72 "Grim.Base.Craft FurHood"),
	(J 73 "Grim.Base.Equip FurHood"),
	(J 74 "Grim.Base.Wear 1 100"),
	(J 75 "Grim.Base.RepairNearest Workbench"),
	(J 76 "Grim.Base.Damage GrimBuildPiece_2 100"),
	(J 77 "Grim.Base.Repair GrimBuildPiece_2"),
	(J 79 "Grim.Items.Give GrainSeed 2"),
	(J 80 "Grim.Base.Plot"),
	(J 82 "Grim.Base.Grow 3000"),
	(J 84 "Grim.Base.Plot"),
	(J 85 "Grim.Base.Dump"),
	(J 85.5 "Grim.Items.Dump"),
	(J 86 "Grim.Base.Demolish GrimBuildPiece_7"),
	(J 90 "Grim.Base.Dump"),
	(J 90.5 "Grim.Items.Dump")
) -join ","
$ClientCmds = @(
	(J 20 "Grim.Items.Give Wood 50"),
	(J 20.3 "Grim.Items.Give Stone 10"),
	(J 20.6 "Grim.Items.Give JackalopeFur 4"),
	(J 22 "Grim.Base.Teleport 0 -4 0"),
	(J 24 "Grim.Base.Place WoodWall 3 -1 0 1"),
	(J 25 "Grim.Base.Place WoodWall 3 0 0 1"),
	(J 26 "Grim.Base.Place WoodDoor 3 0 0 0"),
	(J 27 "Grim.Base.PlaceAt Bed 3 0 0 -80 0"),
	(J 28 "Grim.Base.PlaceAt StorageChest 3 0 0 90 0"),
	(J 29 "Grim.Base.Place WoodRoof 2 0 0 0"),
	(J 30 "Grim.Base.Place WoodRoof 3 0 0 0"),
	(J 31 "Grim.Base.PlaceAt Workbench 0 -2 0 0 0"),
	(J 32 "Grim.Base.PlaceAt CropPlot 2 -3 0 0 0"),
	(J 34 "Grim.Base.Dump"),
	(J 42 "Grim.Base.Teleport 3 0 0"),
	(J 52 "Grim.Base.Dump"),
	(J 55.5 "grim.Survival.Sleep"),
	(J 64 "Grim.Base.Dump"),
	(J 68 "Grim.Base.Store Wood 5"),
	(J 86 "Grim.Base.Dump"),
	(J 86.5 "Grim.Items.Dump")
) -join ","

function Wait-Log([string]$Path, [string]$Pattern, [int]$Timeout) {
	$Deadline = (Get-Date).AddSeconds($Timeout)
	while ((Get-Date) -lt $Deadline) {
		if ((Test-Path $Path) -and (Select-String -Path $Path -Pattern $Pattern -Quiet)) { return $true }
		Start-Sleep -Seconds 2
	}
	return $false
}

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
if (-not (Wait-Log $ServerLog "Bringing World /Game/Grimoire/Maps/L_M1_Sandbox" $BootTimeout)) { "서버 맵이 뜨지 않았다" }
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)
if (-not (Wait-Log $ClientLog "Welcomed by server" $BootTimeout)) { "클라이언트가 들어오지 못했다" }

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimBase|ItemsDump|밤 건너뛰기|잠 요청|잠듦|Welcomed by server|NOT Supported|Error: " | Select-Object -First 400 | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}
