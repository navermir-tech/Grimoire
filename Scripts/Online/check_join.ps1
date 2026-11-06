# 접속 · 메뉴 확인(헤드리스, M3 갈래 J — docs/systems/world-multiplayer 「세션」).
#
#   powershell -File Scripts\Online\check_join.ps1      # 결과: Saved\NetCheck\join_*.log 요약 + 확인 항목 PASS/FAIL
#
# 모든 프로세스가 메인 메뉴 맵(L_MainMenu)에서 시작해 메뉴 함수를 콘솔(Grim.Online.*)로 부른다.
# 순서는 Grim.Online.When <이벤트[번째]> <초> <명령> 으로 짠다(그 프로세스에서 그 일이 생긴 때 기준 — 시동 시간이 들쭉날쭉해도 순서가 지켜진다).
#
#   S  메뉴 → 서버 열기(슬롯 JoinCheck · 최대 2명 = 호스트 + 1) → 두 번째 접속 +8초에 Grim.Online.Quit(호스트가 나감 → 종료 저장)
#   C1 메뉴 → 없는 포트로 IP 참가(실패 메시지) → 진짜 포트로 참가(이름 「시험손님」 — 한글이 서버에 그대로) → 150초 뒤 메인 메뉴로 → 끔
#   C3 C1 이 들어가 있는 동안: LAN 찾기 → 참가 → 「서버가 가득 찼다」
#   C2 C1 이 나간 뒤: 참가 → 호스트가 나가자 「호스트가 서버를 닫았다」 → 메인 메뉴로 돌아와 메시지
param(
	[int]$Port = 17791,
	[int]$BadPort = 17799,
	[string]$Slot = "JoinCheck"
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
$SlotDir = Join-Path $Root "Saved\SaveGames\Grimoire\$Slot"
New-Item -ItemType Directory -Force $OutDir | Out-Null
Remove-Item -Recurse -Force $SlotDir -ErrorAction SilentlyContinue

$Names = @("S", "C1", "C3", "C2")
$Logs = @{}
foreach ($Name in $Names) {
	$Logs[$Name] = Join-Path $OutDir "join_$Name.log"
	Remove-Item $Logs[$Name] -ErrorAction SilentlyContinue
}

$Menu = "/Game/Grimoire/Maps/L_MainMenu"
$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240", "-GrimConnectTimeout=8")

function Start-Game([string]$Name, [string]$PlayerId, [string]$DisplayName, [string[]]$Cmds, [string[]]$Extra = @()) {
	$ArgList = @("`"$Project`"", $Menu, "-GrimPlayerId=$PlayerId", "-GrimName=$DisplayName", "-abslog=`"$($Logs[$Name])`"", "-ExecCmds=`"$($Cmds -join ",")`"") + $Common + $Extra
	return Start-Process -FilePath $Exe -PassThru -ArgumentList $ArgList
}

function Wait-Exit($Process, [int]$Seconds) {
	if (-not $Process.WaitForExit($Seconds * 1000)) {
		Write-Host "  (제때 꺼지지 않아 강제로 끔: $($Process.Id))"
		Stop-Process -Id $Process.Id -Force
	}
}

function Wait-Log([string]$Name, [string]$Pattern, [int]$Seconds) {
	$Deadline = (Get-Date).AddSeconds($Seconds)
	while ((Get-Date) -lt $Deadline) {
		if ((Test-Path $Logs[$Name]) -and (Select-String -Encoding UTF8 -Path $Logs[$Name] -Pattern $Pattern -Quiet)) { return $true }
		Start-Sleep -Seconds 1
	}
	Write-Host "  ($Name 로그에서 '$Pattern' 을 $Seconds 초 안에 못 봄)"
	return $false
}

$SCmds = @(
	"Grim.Online.When Menu 1 Grim.Online.Host $Slot 2",
	"Grim.Online.When Hosting 2 Grim.Online.Status",
	"Grim.Online.When Login 3 Grim.Online.Status",
	"Grim.Online.When Login2 6 Grim.Online.Status",
	"Grim.Online.When Login2 8 Grim.Online.Quit",
	"Grim.Online.When Hosting 600 quit"
)
$C1Cmds = @(
	"Grim.Online.When Menu 1 Grim.Online.Join 127.0.0.1:$BadPort",
	"Grim.Online.When Failed 2 Grim.Online.Join 127.0.0.1 $Port",
	"Grim.Online.When Connected 3 Grim.Online.Status",
	"Grim.Online.When Connected 150 Grim.Online.Leave",
	"Grim.Online.When Connected 156 Grim.Online.Status",
	"Grim.Online.When Connected 158 quit",
	"Grim.Online.When Menu 400 quit"
)
$C3Cmds = @(
	"Grim.Online.When Menu 1 Grim.Online.FindLAN",
	"Grim.Online.When Menu 6 Grim.Online.Join 127.0.0.1:$Port",
	"Grim.Online.When Failed 2 Grim.Online.Status",
	"Grim.Online.When Failed 4 quit",
	"Grim.Online.When Connected 5 quit",
	"Grim.Online.When Menu 200 quit"
)
$C2Cmds = @(
	"Grim.Online.When Menu 1 Grim.Online.Join 127.0.0.1:$Port",
	"Grim.Online.When Connected 3 Grim.Online.Status",
	"Grim.Online.When Lost 1 Grim.Online.Status",
	"Grim.Online.When Menu2 2 Grim.Online.Status",
	"Grim.Online.When Menu2 4 quit",
	"Grim.Online.When Menu 300 quit"
)

Write-Host "S: 메뉴에서 서버 열기"
$S = Start-Game "S" "JoinHost" "호스트" $SCmds @("-port=$Port")
$null = Wait-Log "S" "\[Online\] 서버 열림" 300

Write-Host "C1: 없는 포트 → 진짜 포트"
$C1 = Start-Game "C1" "JoinGuest1" "시험손님" $C1Cmds
$null = Wait-Log "S" "\[Online\] 접속: " 300

Write-Host "C3: 가득 찬 서버"
$C3 = Start-Game "C3" "JoinGuest3" "Guest3" $C3Cmds
Wait-Exit $C3 300

Write-Host "C1 이 메인 메뉴로 나가기를 기다림"
$null = Wait-Log "S" "\[Online\] 나감: " 300
Wait-Exit $C1 120

Write-Host "C2: 호스트가 나가는 서버"
$C2 = Start-Game "C2" "JoinGuest2" "Guest2" $C2Cmds
Wait-Exit $C2 300
Wait-Exit $S 90

# ───── 요약 ─────
$Pattern = "LogGrimOnline: |LogGrimSave: .*(세이브 꺼짐|세계 저장|PlayerId .* 저장)|LogNet: .*(NetworkFailure|TravelFailure)|Error: "
foreach ($Name in $Names) {
	"===== $Name"
	if (Test-Path $Logs[$Name]) {
		Select-String -Encoding UTF8 -Path $Logs[$Name] -Pattern $Pattern | ForEach-Object { $_.Line -replace "^\[[^\]]*\]\[[^\]]*\]", "" }
	}
}

function Text([string]$Name) { if (Test-Path $Logs[$Name]) { return [IO.File]::ReadAllText($Logs[$Name]) } return "" }
$T = @{}
foreach ($Name in $Names) { $T[$Name] = Text $Name }

$Results = @()
function Check([string]$What, [bool]$Ok, [switch]$Optional) {
	$Result = if ($Ok) { "PASS" } elseif ($Optional) { "WARN" } else { "FAIL" }
	$script:Results += [PSCustomObject]@{ Result = $Result; What = $What }
}

# 메뉴
Check "S: 메뉴 월드는 세이브를 쓰지 않음" ($T["S"] -match "메인 메뉴 맵 — 이 월드에서는 세이브를 쓰지 않는다" -and $T["S"] -match "세이브 꺼짐: 설정 bEnabled=False")
# M3 통합(2026-11-06) 뒤에는 ini 의 GameDefaultMap 이 이미 L_MainMenu 라 이 프로세스에서 바꿀 일이 없다 — 둘 중 하나면 된다
$IniDefaultMenu = [bool](Select-String -Path (Join-Path $Root "Config\DefaultEngine.ini") -Pattern "^GameDefaultMap=/Game/Grimoire/Maps/L_MainMenu" -Quiet)
Check "S: 실패 때 돌아갈 기본 맵 = 메인 메뉴" ($IniDefaultMenu -or $T["S"] -match "돌아갈 기본 맵을 이 프로세스에서만 /Game/Grimoire/Maps/L_MainMenu")

# 서버 열기 — 게임 맵은 설정 GrimOnlineSettings.GameMap(M3 통합 뒤 L_OpenBeta)
Check "S: 서버 열림(포트 $Port · 최대 2 · 슬롯 $Slot)" ($T["S"] -match "서버 열림: 맵 /Game/Grimoire/Maps/(L_OpenBeta|L_Tower) · 주소 \S*:$Port · 최대 2명 · 슬롯 $Slot")
Check "S: 호스트 이름 = 호스트" ($T["S"] -match "로컬 플레이어 이름: 호스트")
Check "S: 온라인 세션 만듦(LAN)" ($T["S"] -match "온라인 세션\(\S+\) 만듦") -Optional

# IP 참가 · 실패
Check "C1: 없는 포트 → 접속 실패 메시지" ($T["C1"] -match "접속 실패: 127\.0\.0\.1:$BadPort")
Check "C1: 진짜 포트 → 접속됨" ($T["C1"] -match "접속됨: 127\.0\.0\.1:$Port")
Check "S: 표시 이름(한글)이 서버로" ($T["S"] -match "접속: 시험손님 \(GrimName\) — 2/2명")
Check "C3: LAN 찾기로 서버 1개 이상" ($T["C3"] -match "LAN 찾기 끝: [1-9]") -Optional
Check "S: 가득 찬 서버가 거절" ($T["S"] -match "접속 거절: Server full")
Check "C3: 「서버가 가득 찼다」 메시지" ($T["C3"] -match "접속 실패: .*가득")

# 메인 메뉴로(클라이언트)
Check "C1: 메인 메뉴로" ($T["C1"] -match "메인 메뉴로 \(접속됨\)")
Check "S: C1 나감" ($T["S"] -match "나감: 시험손님")
Check "S: C1 퇴장 저장" ($T["S"] -match "PlayerId JoinGuest1 저장\(퇴장\)")
Check "C1: 나간 뒤 메인 메뉴 맵에 다시 옴" ([regex]::Matches($T["C1"], "\[Online\] 메인 메뉴(\r|\n| —)").Count -ge 2)

# 호스트가 나감
Check "S: C2 접속" ($T["S"] -match "접속: Guest2 \(GrimName\)")
Check "S: 호스트가 나간다(종료 저장)" ($T["S"] -match "호스트가 나간다 — 종료 저장" -and $T["S"] -match "세계 저장\(종료\)")
Check "C2: 끊김 — 호스트가 서버를 닫았다" ($T["C2"] -match "접속 끊김: .*(호스트가 서버를 닫았다|끊겼다)")
Check "C2: 메인 메뉴로 돌아와 메시지" ($T["C2"] -match "메인 메뉴 — 메시지: ")
Check "세이브 파일 WorldSave_$Slot.sav" (Test-Path (Join-Path $SlotDir "WorldSave_$Slot.sav"))

""
"===== 확인"
$Results | Format-Table -AutoSize | Out-String -Width 200
$Failed = @($Results | Where-Object { $_.Result -eq "FAIL" }).Count
"실패 $Failed / $($Results.Count)"
exit $Failed
