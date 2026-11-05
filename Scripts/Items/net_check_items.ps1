# M3 갈래 I 2인 확인(헤드리스): net_check.ps1 과 같은 방식(리슨 서버 + 클라이언트, -game -nullrhi), 맵 L_Tower(튜토리얼이 저절로 시작).
# 줍기 · 먹기 · 요리(냄비 · 직접 불)가 서버 · 클라이언트에서 맞는지, 튜토리얼 11 · 17 · 18 단계가 아이템 이벤트로 넘어가는지 양쪽 덤프로 남긴다.
#
#   powershell -File Scripts\Items\net_check_items.ps1     # 결과: Saved\NetCheck\items_server.log · items_client.log 의 요약
#
# 시간표(서버 시작 기준 초 S, 클라이언트는 12초 뒤 시작 — 클라이언트 명령 시각 C ≈ S − 12):
#   S30~34.5 튜토리얼 단계 건너뜀 ×10 → 11(고기 줍기). 4단계(스킬북 셋)는 -GrimStartingSpells 로 저절로 넘어가 건너뛰기가 하나 적다
#   C25 클라이언트 앞에 날고기 픽업 → C26 줍기   기대: 클라이언트 가방 JackalopeMeat×1(소유자 복제), Item.Obtained.Meat → 클라이언트 12단계
#   S38 호스트 가방에 날고기   기대: 호스트 12단계
#   S41~42 건너뜀 ×3 → 15 · S43 주방 → 16 · S44 아궁이에 불 → 17 + 지급(허브 · 소금 · 곡물이 두 사람 가방에) · S46 냄비 곁으로
#   C36 클라이언트 냄비에 넣기   기대: 냄비 [JackalopeMeat Herb Salt Grain], MeatStew, Cooking.IngredientsAdded
#   S49 호스트 냄비 F   기대: 넣을 것 없음(끓는 중)
#   ~S58 완성   기대: 스튜 픽업(양쪽에 보임), 클라이언트 Cooking.Completed → 18
#   C49 클라이언트 스튜 줍기 · C50 배고픔 30 · C51 먹기   기대: Hunger 85, Status.Buff.Food, ColdResist 1, Food.Eaten → 19
#   C55 날고기 픽업 · C56 · C60 · C64 직접 불(세기 3)   기대: C69 덤프에 RoastedJackalopeMeat cooked=1(양쪽)
#   C70 줍기 · C72 구운 고기 먹기 · C74~75 날고기 먹기(배탈 확률)
#   S90 호스트 냄비에 넣기 → ~S100 완성 → S103 줍기 · S105 먹기   기대: 호스트 19단계
param(
	[int]$Port = 17781,
	[int]$RunSeconds = 120
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "items_server.log"
$ClientLog = Join-Path $OutDir "items_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240", "-GrimStartingSpells")

$Skips = @()
for ($i = 0; $i -lt 10; $i++) { $Skips += "Grim.Quest.At $(30 + $i * 0.5) Skip Tutorial" }

$ServerCmds = (@(
	"Grim.Items.At 25 Dump"
) + $Skips + @(
	"Grim.Quest.At 36 Dump",
	"Grim.Items.At 38 Give JackalopeMeat 1",
	"Grim.Quest.At 40 Dump",
	"Grim.Quest.At 41 Skip Tutorial",
	"Grim.Quest.At 41.5 Skip Tutorial",
	"Grim.Quest.At 42 Skip Tutorial",
	"Grim.Quest.At 43 Teleport Tut_Kitchen 0",
	"Grim.Quest.At 44 Stimulus Tut_Stove Element.Fire 1",
	"Grim.Quest.At 46 Teleport Tut_CookingPot 150",
	"Grim.Items.At 47 Dump",
	"Grim.Quest.At 47 Dump",
	"Grim.Items.At 49 Pot",
	"Grim.Items.At 50 Dump",
	"Grim.Items.At 62 Dump",
	"Grim.Quest.At 63 Dump",
	"Grim.Items.At 69 Dump",
	"Grim.Items.At 82 Dump",
	"Grim.Items.At 90 Pot",
	"Grim.Items.At 92 Dump",
	"Grim.Items.At 102 Dump",
	"Grim.Items.At 103 PickUp",
	"Grim.Items.At 104 Hunger 30",
	"Grim.Items.At 105 Eat Stew",
	"Grim.Items.At 106 Dump",
	"Grim.Quest.At 107 Dump"
)) -join ","
$ClientCmds = @(
	"Grim.Items.At 25 Spawn JackalopeMeat 1",
	"Grim.Items.At 26 PickUp",
	"Grim.Items.At 27 Dump",
	"Grim.Items.At 36 Pot",
	"Grim.Items.At 38 Dump",
	"Grim.Items.At 48 Dump",
	"Grim.Items.At 49 PickUp",
	"Grim.Items.At 50 Hunger 30",
	"Grim.Items.At 51 Eat Stew",
	"Grim.Items.At 52 Dump",
	"Grim.Quest.At 53 Dump",
	"Grim.Items.At 55 Spawn JackalopeMeat 1",
	"Grim.Items.At 56 Fire 3",
	"Grim.Items.At 60 Fire 3",
	"Grim.Items.At 64 Fire 3",
	"Grim.Items.At 69 Dump",
	"Grim.Items.At 70 PickUp",
	"Grim.Items.At 71 Hunger 30",
	"Grim.Items.At 72 Eat RoastedJackalopeMeat",
	"Grim.Items.At 74 Give JackalopeMeat 1",
	"Grim.Items.At 75 Eat JackalopeMeat",
	"Grim.Items.At 76 Dump",
	"Grim.Quest.At 78 Dump"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_Tower?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimItems|QuestDump|신호 Event|단계|Welcomed by server|Error: " | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}
