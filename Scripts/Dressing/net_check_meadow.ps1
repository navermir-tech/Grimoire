# 초원 꾸밈 2인 복제 확인(헤드리스, docs/qa/meadow.md QA-MDW-020): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고
# 서버가 소품을 놓고 속성 자극을 준 뒤 → 클라이언트가 바뀐 모습(복제된 상태)을 보는지, 클라이언트가 F 로 곁가지를 거두고(세계 공유) 책장을 줍는지(사람별),
# 아침이 되면 다시 나는지를 양쪽 덤프(Grim.Meadow Dump)로 본다. 시간표는 두 플레이어가 모인 때부터(Grim.Taming.AfterJoin — BUG-007 처럼 고정 시계에 흔들리지 않게).
#
#   powershell -File Scripts\Dressing\net_check_meadow.ps1      # 결과: Saved\NetCheckMeadow\server.log · client.log 요약 + NETCHECK_RESULT fail=0
#
#   3   서버: 두 사람 앞 3 m 에 바람솜꽃, 클라이언트 뒤 1.5 m · 호스트 뒤에 책장, 두 사람 앞 1.2 m 에 곁가지 셀, 호스트 앞 4 m 에 목마른 들꽃
#   6   서버: 바람 1.0 → 솜꽃 둘, 물 0.5 → 들꽃           기대: 클라이언트 덤프 D01 State.Spent alt=1 · D03 State.Bloomed alt=1
#   10  클라이언트 F → 가장 가까운 곁가지                    기대: 서버 「거둠 — Wood」, 클라이언트 셀 hidden=1(공유 — 호스트 화면에서도 숨는다)
#   15  클라이언트 F → 책장                                   기대: 클라이언트 D21 collected=1 mine=1 · 도감 1 줄, 서버 D21 collected=1 mine=0
#   20  서버: 해 뜸(Grim.Meadow Dawn)                          기대: 서버 「아침 — 1 점이 다시 났다」, 클라이언트 덤프 거둔 기록 0 · 솜꽃 처음 모습
param(
	[int]$Port = 17795,
	[int]$RunSeconds = 75,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox"
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckMeadow"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

function Join-Schedule([string[]]$Items) {
	($Items | ForEach-Object { $Parts = $_ -split ' ', 2; "Grim.Taming.AfterJoin 2 $($Parts[0]) $($Parts[1])" }) -join ","
}

$ServerCmds = "Grim.Creature.Time 0.5 0," + (Join-Schedule @(
	"3 Grim.Meadow Spawn D01_WindCotton 300 All",
	"3.2 Grim.Meadow Spawn D21_Page -150 Client",
	"3.3 Grim.Meadow Spawn D21_Page -150 Host",
	"3.5 Grim.Meadow SpawnCell D10_Branch1 120 All",
	"3.7 Grim.Meadow Spawn D03_ThirstyFlower 400 Host",
	"6 Grim.Meadow Hit Element.Wind 1 0 D01_WindCotton",
	"6.2 Grim.Meadow Hit Element.Water 0.5 0 D03_ThirstyFlower",
	"8 Grim.Meadow Dump",
	"13 Grim.Meadow Dump",
	"18 Grim.Meadow Dump",
	"20 Grim.Meadow Dawn",
	"22 Grim.Meadow Dump"
))
$ClientCmds = Join-Schedule @(
	"9 Grim.Meadow Dump",
	"10 Grim.Meadow Interact",
	"12.5 Grim.Meadow Dump",
	"15 Grim.Meadow Interact",
	"17 Grim.Meadow Dump",
	"17.5 Grim.Journal",
	"24 Grim.Meadow Dump"
)

# 같은 PC 의 두 인스턴스는 GameUserSettings.ini 의 같은 PlayerId 를 읽는다 — 사람별(책장) 확인을 위해 따로 준다(FGrimPlayerIdentity)
$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-GrimPlayerId=MeadowHost", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 10
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-GrimPlayerId=MeadowClient", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimDressing|Welcomed by server|Error: " | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Checks = @(
	@($ServerLog, "MeadowDump prop .*D01_WindCotton states=State\.Spent", "서버: 솜꽃이 비었다"),
	@($ClientLog, "\[Client\] MeadowDump prop .*D01_WindCotton states=State\.Spent=1\.0 alt=1", "클라이언트: 솜꽃 상태 · 모습 복제"),
	@($ClientLog, "\[Client\] MeadowDump prop .*D03_ThirstyFlower states=State\.Bloomed=1\.0 alt=1", "클라이언트: 들꽃이 폈다"),
	@($ServerLog, "거둠 — Wood", "서버: 클라이언트 F 로 곁가지를 거뒀다"),
	@($ClientLog, "\[Client\] MeadowDump cell .* harvested=1 hidden=1", "클라이언트: 거둔 곁가지가 숨었다"),
	@($ServerLog, "\[ListenServer\] MeadowDump cell .* harvested=1 hidden=1", "호스트 화면: 공유 곁가지도 숨었다"),
	@($ClientLog, "\[Client\] MeadowDump prop .*D21_Page .*collected=1 mine=1", "클라이언트: 책장을 주웠다(자기 화면에서 사라짐)"),
	@($ServerLog, "\[ListenServer\] MeadowDump prop .*D21_Page .*collected=1 mine=0", "호스트: 남이 주운 책장은 호스트에게 그대로"),
	@($ClientLog, "\[Client\] 도감 1 줄", "클라이언트: 도감 한 줄(소유자 복제)"),
	@($ServerLog, "아침 — 1 점이 다시 났다", "서버: 아침에 곁가지가 다시 났다"),
	@($ClientLog, "\[Client\] MeadowDump 합계 .*거둔 기록 0", "클라이언트: 아침 뒤 거둔 기록 0")
)
$Fail = 0
foreach ($C in $Checks) {
	$Hit = Select-String -Path $C[0] -Pattern $C[1] -Quiet
	if ($Hit) { "PASS  $($C[2])" } else { "FAIL  $($C[2])   ($($C[1]))"; $Fail++ }
}
"NETCHECK_RESULT fail=$Fail"
