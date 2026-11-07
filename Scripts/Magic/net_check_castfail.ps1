# Q71 기본 완드 · Q64 마법 실패 알림 2인 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고
# 호스트 · 클라이언트가 각자 ① 기본 완드를 쥐고 시작 → ② 쿨타임 중 다시 누름 → ③ 마나가 모자람 → ④ 2서클 스킬북 읽기(봉인된 책, Q122) →
# ⑤ 완드를 벗고 마법(완드 없음) · 기본 공격 입력(주먹질) → ⑥ 다시 입기. 서버가 까닭을 정하고 소유 클라이언트에 알렸는지, 연출 큐가 모두에게 갔는지 로그로 본다.
#
#   powershell -File Scripts\Magic\net_check_castfail.ps1      # 결과: Saved\NetCheckCastFail\server.log · client.log 요약, NETCHECK_RESULT fail=0
#
# 시간표(초)는 두 플레이어가 모인 때(Grim.Base.AfterJoin 2 …)부터. 시작 마법(-GrimStartingSpells): 슬롯1 불씨(마나 10 · 쿨 0.6초) · 슬롯2 물줄기.
# 마나 회복이 0 이라(ini) 클라이언트는 불씨 12번으로 마나를 다 쓴다(100 / 10). 호스트는 Grim.Save.SetAttr Mana 0.
param(
	[int]$Port = 17796,
	[int]$RunSeconds = 45,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox"
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckCastFail"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

function Join-Schedule([string[]]$Items) {
	($Items | ForEach-Object { $Parts = $_ -split ' ', 2; "Grim.Base.AfterJoin 2 $($Parts[0]) $($Parts[1])" }) -join ","
}

$ServerCmds = Join-Schedule @(
	"2 Grim.Base.Dump",
	"4 Grim.Input.Press Slot1 None 0 0 2 0.2",
	"8 Grim.Save.SetAttr Mana 0",
	"8.5 Grim.Input.Press Slot1 None",
	"11 Grim.Items.Give Skillbook_Fireball 1",
	"12 Grim.Items.Eat Skillbook_Fireball",
	"14 Grim.Base.Unequip 0",
	"15 Grim.Input.Press Slot2 None",
	"16 Grim.Input.Press Basic None",
	"18 Grim.Base.Equip Wand_Basic",
	"30 Grim.Base.Dump"
)
$ClientCmds = Join-Schedule @(
	"2 Grim.Base.Dump",
	"5 Grim.Input.Press Slot1 None 0 0 2 0.2",
	"7 Grim.Input.Press Slot1 None 0 0 13 0.7",
	"18 Grim.Items.Give Skillbook_Fireball 1",
	"19 Grim.Items.Eat Skillbook_Fireball",
	"21 Grim.Base.Unequip 0",
	"22.5 Grim.Input.Press Slot2 None",
	"23.5 Grim.Input.Press Basic None",
	"25 Grim.Base.Equip Wand_Basic",
	"30 Grim.Base.Dump"
)

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
$Waited = 0
while ($Waited -lt 240 -and -not ((Test-Path $ServerLog) -and (Select-String -Path $ServerLog -Pattern 'Bringing World' -Quiet))) { Start-Sleep -Seconds 1; $Waited++ }
Start-Sleep -Seconds 2
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)
$Waited = 0
while ($Waited -lt 240 -and -not ((Test-Path $ClientLog) -and (Select-String -Path $ClientLog -Pattern 'Welcomed by server' -Quiet))) { Start-Sleep -Seconds 1; $Waited++ }
Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Encoding UTF8 -Pattern "마법 실패|손의 완드|시작 장비|주먹질|가방 알림|BaseDump .* player|입음:|Welcomed by server|Error: " |
		ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Fail = 0
$ServerText = Get-Content -Encoding UTF8 -Raw $ServerLog
$ClientText = Get-Content -Encoding UTF8 -Raw $ClientLog
function Expect-Count([string]$Name, [string]$Text, [string]$Pattern, [int]$Min) {
	$Count = ([regex]::Matches($Text, $Pattern)).Count
	if ($Count -lt $Min) { $script:Fail++; "없음: $Name — 「$Pattern」 $Count 번(기대 $Min 이상)" } else { "맞음: $Name ($Count)" }
}

# ① 기본 완드를 쥐고 시작(실제 아이템 · 내구도 200)
Expect-Count "서버: 두 플레이어 시작 장비" $ServerText "시작 장비 1개를 입혔다" 2
Expect-Count "서버 덤프: 기본 완드(호스트 · 클라이언트)" $ServerText "BaseDump Server\] player .*wandMissing=0 .*gear=\[0:Wand_Basic\(\d+/200 r0\)" 2
Expect-Count "클라이언트 덤프: 내 기본 완드(소유자 복제)" $ClientText "BaseDump Client\] player .*wandMissing=0 .*gear=\[0:Wand_Basic\(\d+/200 r0\)" 1
# ②③⑤ 서버 판정(두 플레이어 몫)
foreach ($Reason in @("Cooldown", "NotEnoughMana", "NoWand")) {
	Expect-Count "서버 판정 $Reason" $ServerText "\[Server\] 마법 실패 \S+ $Reason " 2
	# 소유 클라이언트만 받는다: 호스트 것은 서버 로그(리슨 호스트), 클라이언트 것은 클라이언트 로그.
	Expect-Count "호스트 알림 $Reason" $ServerText "\[Server\] 마법 실패 알림 $Reason" 1
	Expect-Count "클라이언트 알림 $Reason" $ClientText "\[Client\] 마법 실패 알림 $Reason" 1
}
if (([regex]::Matches($ServerText, "\[Server\] 마법 실패 알림 ")).Count -gt 3 * 2) { $Fail++; "서버 로그에 알림이 너무 많다 — 클라이언트 몫이 서버에서 울렸나" }
# 연출 큐: 모든 기기 — 클라이언트는 자기 것 + 호스트 것
Expect-Count "클라이언트가 본 실패 연출" $ClientText "\[Client\] 마법 실패 연출 " 5
Expect-Count "호스트가 본 실패 연출" $ServerText "\[Server\] 마법 실패 연출 " 5
# ④ 2서클 스킬북 — Q122 부터는 봉인된 책으로 기록(책 소모)
Expect-Count "호스트 가방 알림 봉인" $ServerText "\[Server\] 가방 알림 Sealed Skillbook_Fireball 필요 서클 2" 1
Expect-Count "클라이언트 가방 알림 봉인" $ClientText "\[Client\] 가방 알림 Sealed Skillbook_Fireball 필요 서클 2" 1
# ⑤ 완드 벗음 → 손에서 숨김 · 주먹질, ⑥ 다시 입음 → 보임
Expect-Count "손의 완드 숨김" $ServerText "손의 완드 숨김" 2
Expect-Count "손의 완드 보임" $ServerText "손의 완드 보임" 2
Expect-Count "주먹질(서버)" $ServerText "\[(Server|Host)\] 주먹질" 2
Expect-Count "다시 입음" $ServerText "입음: Wand_Basic" 2
Expect-Count "마지막 덤프: 다시 기본 완드" $ServerText "BaseDump Server\] player .*wandMissing=0 .*gear=\[0:Wand_Basic" 4
"NETCHECK_RESULT fail=$Fail"
