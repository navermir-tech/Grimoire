# R38 생물 데리고 다니기 · 보관함 · 거점 생물 상자 2인 복제 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고 디버그 콘솔 명령으로
# 호스트 · 클라이언트가 각자 재칼로프를 파크툼으로 길들이고 → 스킬북(가방 아이템)을 읽어 생물 보관함을 배우고 → 넣기(목록 보기) → 꺼내기 →
# 거점 생물 상자로 → 풀어놓기 → 다시 데리고 다니기. 클라이언트가 본 목록(Grim.Taming.StorageDump Local)과 서버가 본 그 클라이언트 목록(Remote)이 같은 차례끼리 같아야 하고,
# 호스트 자기 목록도 단계마다 맞아야 한다.
#
#   powershell -File Scripts\Creatures\net_check_storage.ps1        # 결과: Saved\NetCheckStorage\server.log · client.log 의 요약 + 덤프 비교, NETCHECK_RESULT fail=0
#
# 시간표(초)는 두 플레이어가 모인 때(Grim.Taming.AfterJoin 2 …)부터 센다 — 기계가 바빠 클라이언트가 늦게 붙어도 서버 · 클라이언트가 같은 차례로 움직인다:
#   2 파크툼 배우기(1서클 — 데리고 다님 3 · 보관함 8) · 4 클라이언트 앞 재칼로프 · 5 클라이언트를 그 4m 앞으로(사이에 호스트가 서 있지 않게) → 6.5 클라이언트 파크툼 · 9 호스트 앞 재칼로프 → 11 호스트 파크툼 →
#   15 스킬북 받기 → 17 읽기(배움) → 20 넣기(데리고 다님 → 보관함) → 23 덤프① → 25 꺼내기 → 28 덤프② → 30 상자 두 개(각자 앞 2.5m) →
#   32 데리고 다님 → 상자 · 33 상자 화면 열기 → 35 덤프③ → 37 풀어놓기 → 40 덤프④ → 42 풀어놓은 것 → 데리고 다님 → 45 덤프⑤.
param(
	[int]$Port = 17791,
	[int]$RunSeconds = 60,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox",
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckStorage"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
# 기계가 바쁘면 클라이언트 접속 · 첫 맵 로드가 기본 접속 시간 제한을 넘는다 — 넉넉히.
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

# 「t 명령」 → 두 플레이어가 모이고 t 초 뒤(명령 안의 지연 인자는 0 으로 둔다)
function Join-Schedule([string[]]$Items) {
	($Items | ForEach-Object { $Parts = $_ -split ' ', 2; "Grim.Taming.AfterJoin 2 $($Parts[0]) $($Parts[1])" }) -join ","
}

# 서버(호스트) — 호스트 자기 행동은 「로컬 플레이어」 명령이 서버 세계에서 돈다.
$ServerCmds = "Grim.Taming.ForceRoll 0,Grim.Creature.Time 0.5 0," + (Join-Schedule @(
	"2 Grim.Taming.Grant 1",
	"4 Grim.Taming.SpawnNearClient Jackalope 600",
	"5 Grim.Taming.Place Jackalope 400",
	"9 Grim.Creature.Spawn Jackalope 1 0 600",
	"11 Grim.Taming.Cast DA_Spell_Pactum Jackalope 0",
	"15 Grim.Items.Give Skillbook_CreatureStorage 1",
	"17 Grim.Items.Eat Skillbook_CreatureStorage",
	"20 Grim.Taming.Storage Party Storage Jackalope",
	"23 Grim.Taming.StorageDump 0 Remote",
	"23.5 Grim.Taming.StorageDump 0 Local",
	"25 Grim.Taming.Storage Storage Party Jackalope",
	"28 Grim.Taming.StorageDump 0 Remote",
	"28.5 Grim.Taming.StorageDump 0 Local",
	"30 Grim.Taming.SpawnBox 250 0 Client",
	"30.2 Grim.Taming.SpawnBox 250 0 Host",
	"32 Grim.Taming.Storage Party Box Jackalope",
	"35 Grim.Taming.StorageDump 0 Remote",
	"35.5 Grim.Taming.StorageDump 0 Local",
	"37 Grim.Taming.Storage Box Released Jackalope",
	"40 Grim.Taming.StorageDump 0 Remote",
	"40.5 Grim.Taming.StorageDump 0 Local",
	"42 Grim.Taming.Storage Released Party Jackalope",
	"45 Grim.Taming.StorageDump 0 Remote",
	"45.5 Grim.Taming.StorageDump 0 Local",
	"47 Grim.Taming.Dump"
))
$ClientCmds = Join-Schedule @(
	"6.5 Grim.Taming.Cast DA_Spell_Pactum Jackalope 0",
	"15 Grim.Items.Give Skillbook_CreatureStorage 1",
	"17 Grim.Items.Eat Skillbook_CreatureStorage",
	"20 Grim.Taming.Storage Party Storage Jackalope",
	"23 Grim.Taming.StorageDump 0 Local",
	"25 Grim.Taming.Storage Storage Party Jackalope",
	"28 Grim.Taming.StorageDump 0 Local",
	"32 Grim.Taming.Storage Party Box Jackalope",
	"33 Grim.Taming.Box Open",
	"35 Grim.Taming.StorageDump 0 Local",
	"37 Grim.Taming.Storage Box Released Jackalope",
	"40 Grim.Taming.StorageDump 0 Local",
	"42 Grim.Taming.Storage Released Party Jackalope",
	"45 Grim.Taming.StorageDump 0 Local",
	"47 Grim.Taming.Dump"
)

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
# 서버 맵이 뜰 때까지 기다린 뒤 클라이언트를 띄우고, 클라이언트가 붙은 뒤 RunSeconds 동안 돌린다.
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
	Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimTaming: (Display|Warning|Error)|LogGrimItems: (Display|Warning): \[Server\]|LogNet: Join|Welcomed by server|Error: " |
		Where-Object { $_.Line -notmatch 'tamepos|storagetext' } | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

# 덤프 덩어리: "StorageDump <Remote|Local>" 줄마다 새로 시작하고, 덩어리 안의 storage · storagerow 줄(역할 접두 뺌)을 모은다.
function Get-StorageBlocks([string]$Path, [string]$Which) {
	$Blocks = New-Object System.Collections.ArrayList
	$Current = $null
	$InBlock = $false
	foreach ($Line in (Get-Content -Encoding UTF8 $Path)) {
		if ($Line -match 'LogGrimTaming: Display: \[(Server|Client)\] StorageDump (Remote|Local) ') {
			if ($InBlock) { [void]$Blocks.Add($Current) }
			$Current = New-Object System.Collections.ArrayList
			$InBlock = ($Matches[2] -eq $Which)
			if (-not $InBlock) { $Current = $null }
		}
		elseif ($InBlock -and $Line -match 'LogGrimTaming: Display: \[(Server|Client)\] (storage(row)? .*)$') {
			[void]$Current.Add($Matches[2])
		}
	}
	if ($InBlock) { [void]$Blocks.Add($Current) }
	return ,$Blocks
}

$Fail = 0
$RemoteBlocks = Get-StorageBlocks $ServerLog "Remote"
$ClientBlocks = Get-StorageBlocks $ClientLog "Local"
$HostBlocks = Get-StorageBlocks $ServerLog "Local"
"===== 클라이언트 목록 비교 — 서버가 본 것(Remote $($RemoteBlocks.Count)) · 클라이언트가 본 것(Local $($ClientBlocks.Count))"
for ($i = 0; $i -lt [Math]::Min($RemoteBlocks.Count, $ClientBlocks.Count); $i++) {
	$S = @($RemoteBlocks[$i])
	$C = @($ClientBlocks[$i])
	$Diff = if ($S.Count -eq 0 -or $C.Count -eq 0) { if ($S.Count -ne $C.Count) { @("빈 덤프") } else { $null } } else { Compare-Object -ReferenceObject $S -DifferenceObject $C }
	if ($Diff) {
		$Fail++
		"덤프 $($i + 1): 다름"
		$Diff | ForEach-Object { "  $($_.SideIndicator) $($_.InputObject)" }
	}
	else {
		"덤프 $($i + 1): 같음 — $($S[0])"
	}
}
if ($RemoteBlocks.Count -ne 5 -or $ClientBlocks.Count -ne 5) { $Fail++; "덤프 수가 5 가 아님(서버 Remote $($RemoteBlocks.Count) · 클라이언트 $($ClientBlocks.Count))" }

# 단계마다 있어야 할 목록(클라이언트 · 호스트 같은 흐름): ① 보관함 1 ② 데리고 다님 1 ③ 상자 ④ 풀어놓음 ⑤ 다시 데리고 다님
$Expect = @(
	@{ Head = "storage learned=1 party=0/3 stored=1/8"; Row = "storagerow place=Storage" },
	@{ Head = "storage learned=1 party=1/3 stored=0/8"; Row = "storagerow place=Party" },
	@{ Head = "storage learned=1 party=0/3 stored=0/8 box=yes boxEntries=1 released=0"; Row = "storagerow place=Box" },
	@{ Head = "storage learned=1 party=0/3 stored=0/8 box=yes boxEntries=1 released=1"; Row = "storagerow place=Released" },
	@{ Head = "storage learned=1 party=1/3 stored=0/8 box=yes boxEntries=0 released=0"; Row = "storagerow place=Party" }
)
foreach ($Pair in @(@{ Name = "클라이언트"; Blocks = $ClientBlocks }, @{ Name = "호스트"; Blocks = $HostBlocks })) {
	for ($i = 0; $i -lt $Expect.Count; $i++) {
		$Block = if ($i -lt $Pair.Blocks.Count) { @($Pair.Blocks[$i]) } else { @() }
		$Text = $Block -join "`n"
		if ($Text -notmatch [regex]::Escape($Expect[$i].Head) -or $Text -notmatch [regex]::Escape($Expect[$i].Row)) {
			$Fail++
			"$($Pair.Name) 덤프 $($i + 1): 기대 「$($Expect[$i].Head)」 + 「$($Expect[$i].Row)」 — 실제:"
			$Block | ForEach-Object { "  $_" }
		}
		else {
			"$($Pair.Name) 덤프 $($i + 1): 맞음"
		}
	}
}

# 꼭 있어야 하는 서버 줄(두 플레이어 몫이라 두 번씩)
$ServerText = Get-Content -Encoding UTF8 -Raw $ServerLog
foreach ($Want in @(@{ T = "길들임 Jackalope"; N = 2 }, @{ T = "생물 보관함 배움"; N = 2 }, @{ T = "Party → Storage"; N = 2 }, @{ T = "Storage → Party"; N = 2 },
		@{ T = "Party → Box"; N = 2 }, @{ T = "Box → Released"; N = 2 }, @{ T = "Released → Party"; N = 2 }, @{ T = "풀어놓음 Jackalope"; N = 2 })) {
	# 옮김은 서버가 실제로 옮긴 줄(「생물 옮김 <종> seed=<n> From → To」)만 센다 — 콘솔 명령 줄은 빼고.
	$Pattern = if ($Want.T -match '→') { "생물 옮김 \S+ seed=\d+ " + [regex]::Escape($Want.T) } else { [regex]::Escape($Want.T) }
	$Count = ([regex]::Matches($ServerText, $Pattern)).Count
	if ($Count -lt $Want.N) { $Fail++; "서버 로그: 「$($Want.T)」 $Count 번(기대 $($Want.N))" }
}
$ClientText = Get-Content -Encoding UTF8 -Raw $ClientLog
foreach ($Want in @("Taming.Storage Party → Storage", "Taming.Storage Box → Released", "거점 생물 상자 화면 열기", "알림: 쿠스토디아(생물 보관함)를 배웠다")) {
	if ($ClientText -notmatch [regex]::Escape($Want)) { $Fail++; "클라이언트 로그에 없음: $Want" }
}
"NETCHECK_RESULT fail=$Fail"
