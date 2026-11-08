# 가방 더블클릭 사용 2인 확인(헤드리스): 리슨 서버 + 클라이언트(-game -nullrhi), 맵 L_Tower.
# Grim.Items.BagClick 은 가방 화면의 칸 누름 처리(UGrimInventoryWidget::NotifySlotClicked)를 그대로 부른다 — 서버로 가는 것은 화면이 부르는 Server_UseItem 뿐.
# 클라이언트 쪽 결과는 복제된 소유자 가방으로, 서버 권위는 서버 프로세스 로그의 「먹었다」 줄 · 서버 덤프로 본다.
#
#   powershell -File Scripts\Items\net_check_bag_doubleclick.ps1     # 끝 줄 NETCHECK_RESULT fail=0
#
# 시간표(각 프로세스 시작 기준 초 — 클라이언트는 서버보다 12초 늦게 시작). 칸: 0 구운 고기 ×3 · 1 털 ×3 · 2 털 두건 ×1:
#   C30 비우고 지급 · 배고픔 30
#   C34 한 번 누름            기대: 고기 3 그대로
#   C37 느린 두 번(1초 간격)   기대: 고기 3 그대로
#   C40 더블클릭               기대: 고기 2 · 배고픔 증가 · 서버 로그 「먹었다」
#   C43 재료 더블클릭          기대: 털 3 그대로(알림이 없는지는 자동화 Grimoire.Items.BagDoubleClick.Use)
#   C46 장비 더블클릭          기대: 두건이 가방에서 빠짐(입기)
#   C49 빈 칸 더블클릭         기대: 변화 없음
#   S72~ 호스트도 같은 순서(한 번 · 더블클릭)   기대: 호스트 고기 3 → 3 → 2
param(
	[int]$Port = 17796,
	[int]$RunSeconds = 100
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "bag_dbl_server.log"
$ClientLog = Join-Path $OutDir "bag_dbl_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ClientCmds = @(
	"Grim.Items.At 30 Clear",
	"Grim.Items.At 31 Give RoastedJackalopeMeat 3",
	"Grim.Items.At 31.5 Give JackalopeFur 3",
	"Grim.Items.At 32 Give FurHood 1",
	"Grim.Items.At 32.5 Hunger 30",
	"Grim.Items.At 33.5 Dump",
	"Grim.Items.At 34 BagClick 0 1",
	"Grim.Items.At 35.5 Dump",
	"Grim.Items.At 37 BagClick 0 2 1.0",
	"Grim.Items.At 38.5 Dump",
	"Grim.Items.At 40 BagClick 0 2 0.1",
	"Grim.Items.At 41.5 Dump",
	"Grim.Items.At 43 BagClick 1 2 0.1",
	"Grim.Items.At 44.5 Dump",
	"Grim.Items.At 46 BagClick 2 2 0.1",
	"Grim.Items.At 47.5 Dump",
	"Grim.Items.At 49 BagClick 7 2 0.1",
	"Grim.Items.At 50.5 Dump"
) -join ","

$ServerCmds = @(
	"Grim.Items.At 70 Clear",
	"Grim.Items.At 71 Give RoastedJackalopeMeat 3",
	"Grim.Items.At 71.5 Hunger 30",
	"Grim.Items.At 72.5 Dump",
	"Grim.Items.At 73 BagClick 0 1",
	"Grim.Items.At 74.5 Dump",
	"Grim.Items.At 76 BagClick 0 2 0.1",
	"Grim.Items.At 77.5 Dump"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_Tower?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

$Fail = 0
function Check([string]$Name, [bool]$Ok, [string]$Detail) {
	if ($Ok) { "PASS  $Name  $Detail" } else { "FAIL  $Name  $Detail"; $script:Fail++ }
}

# 칸이 있는 덤프 줄만 차례로(남의 가방은 복제되지 않아 비어 있다)
function DumpLines([string]$Log, [string]$Side, [string]$Item) {
	@(Select-String -Path $Log -Pattern "\[ItemsDump $Side\].*items=\[[^\]]*$Item" | ForEach-Object { $_.Line })
}
function CountOf([string]$Line, [string]$Item) {
	if ($Line -match "\d+:$Item×(\d+)") { [int]$Matches[1] } else { 0 }
}
function HungerOf([string]$Line) {
	if ($Line -match "Hunger=([\d\.]+)") { [double]$Matches[1] } else { -1 }
}

$Client6 = DumpLines $ClientLog "Client" "JackalopeFur"
"클라이언트 가방 덤프 줄 수: $($Client6.Count)"
if ($Client6.Count -ge 6) {
	Check "한 번 누름은 쓰지 않음" ((CountOf $Client6[1] "RoastedJackalopeMeat") -eq 3) "meat=$(CountOf $Client6[1] 'RoastedJackalopeMeat')"
	Check "느린 두 번은 쓰지 않음" ((CountOf $Client6[2] "RoastedJackalopeMeat") -eq 3) "meat=$(CountOf $Client6[2] 'RoastedJackalopeMeat')"
	Check "더블클릭으로 하나 먹음(복제)" ((CountOf $Client6[3] "RoastedJackalopeMeat") -eq 2) "meat=$(CountOf $Client6[3] 'RoastedJackalopeMeat')"
	Check "더블클릭으로 배고픔 증가" ((HungerOf $Client6[3]) -gt (HungerOf $Client6[2])) "$(HungerOf $Client6[2]) -> $(HungerOf $Client6[3])"
	Check "재료 더블클릭은 그대로" ((CountOf $Client6[4] "JackalopeFur") -eq 3) "fur=$(CountOf $Client6[4] 'JackalopeFur')"
	Check "장비 더블클릭은 입기" ((CountOf $Client6[5] "FurHood") -eq 0) "hood=$(CountOf $Client6[5] 'FurHood')"
	if ($Client6.Count -ge 7) {
		Check "빈 칸 더블클릭은 그대로" (($Client6[6] -replace '.*items=\[', '' -replace '\].*', '') -eq ($Client6[5] -replace '.*items=\[', '' -replace '\].*', '')) ""
	}
} else {
	Check "클라이언트 덤프 6줄 이상" $false "로그 $ClientLog"
}

$ServerEat = @(Select-String -Path $ServerLog -Pattern "\[Server\].*RoastedJackalopeMeat 을\(를\) 먹었다").Count
Check "서버가 먹기를 처리(클라이언트 1 + 호스트 1)" ($ServerEat -eq 2) "먹었다 줄 $ServerEat"

$Host3 = @(Select-String -Path $ServerLog -Pattern "\[ItemsDump Server\].*items=\[\d+:RoastedJackalopeMeat" | ForEach-Object { $_.Line } | Where-Object { $_ -notmatch "JackalopeFur" })
"호스트 고기 덤프 줄 수: $($Host3.Count)"
if ($Host3.Count -ge 3) {
	$Last = $Host3[-3..-1]
	Check "호스트 한 번 누름은 쓰지 않음" ((CountOf $Last[1] "RoastedJackalopeMeat") -eq 3) "meat=$(CountOf $Last[1] 'RoastedJackalopeMeat')"
	Check "호스트 더블클릭으로 하나 먹음" ((CountOf $Last[2] "RoastedJackalopeMeat") -eq 2) "meat=$(CountOf $Last[2] 'RoastedJackalopeMeat')"
} else {
	Check "호스트 덤프 3줄 이상" $false "로그 $ServerLog"
}

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "BagInput|먹었다|Welcomed by server" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}
"NETCHECK_RESULT fail=$Fail"
