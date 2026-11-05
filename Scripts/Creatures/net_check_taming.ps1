# M3 갈래 T 길들이기 · 탑승 2인 복제 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고 디버그 콘솔 명령으로
# 클라이언트가 재칼로프를 마법으로 약하게 만들고 → 파크툼으로 길들이고 → 따라오게 하고, 켈피를 파크툼 아쿠아로 길들여 이름 짓고 → 타고 → 탄 채 시전 → 내리기 → 기다리기.
# 양쪽 Grim.Taming.Dump 의 tame 줄(주인 · 충성도 · 이름 · 명령 · 탄 사람 · State.Mounted · 붙음 · 까닭)이 같은 차례끼리 같아야 한다.
#
#   powershell -File Scripts\Creatures\net_check_taming.ps1        # 결과: Saved\NetCheckTaming\server.log · client.log 의 요약 + 덤프 비교
#
# 시간표(서버 시작 기준 초). 클라이언트는 12초 뒤 시작하지만 먼저 맵에 들어가 시계가 약 10초 늦다 — 클라이언트 명령 시각 = 서버 시각 − 10:
#   29 재칼로프 · 30 서클 3 · 파크툼 배우기 · 32 클라이언트를 재칼로프 4m 앞으로 → 33 플라미스(약하게 — 불붙어 더 약해짐) · 35 다시 5m 앞으로 → 36 파크툼 → 40 덤프① → 41 걷기(따라오기) → 48 덤프② →
#   50 클라이언트 앞에 켈피 → 52 파크툼 아쿠아 → 56 덤프③ → 58 이름 → 60 타기 → 62 타고 1.5초 달리기 · 66 표적 재칼로프(클라이언트 앞) → 68 탄 채 플라미스 → 71 덤프④ →
#   73 내리기 → 76 덤프⑤ · 78 먹이 사건 → 80 기다리기 → 83 덤프⑥.
param(
	[int]$Port = 17783,
	[int]$RunSeconds = 95,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox",
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckTaming"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ServerCmds = @(
	"Grim.Taming.ForceRoll 0",
	"Grim.Creature.Time 0.5 20",
	"Grim.Creature.Spawn Jackalope 1 29 900",
	"Grim.Taming.Grant 3 30",
	"Grim.Taming.Place Jackalope 400 32",
	"Grim.Taming.Place Jackalope 500 35",
	"Grim.Creature.Dump 38",
	"Grim.Taming.Dump 40",
	"Grim.Taming.Dump 48",
	"Grim.Taming.SpawnNearClient Kelpie 700 50",
	"Grim.Taming.Dump 56",
	"Grim.Taming.SpawnNearClient Jackalope 700 66",
	"Grim.Taming.Dump 71",
	"Grim.Taming.Dump 76",
	"Grim.Taming.Loyalty Fed 1 Kelpie 78",
	"Grim.Taming.Dump 83"
) -join ","
$ClientCmds = @(
	"Grim.Taming.Cast DA_Spell_Ember Jackalope 0 23",
	"Grim.Taming.Cast DA_Spell_Pactum Jackalope 0 26",
	"Grim.Creature.Dump 28",
	"Grim.Taming.Dump 30",
	"Grim.Taming.Move 1 0 3 31",
	"Grim.Taming.Dump 38",
	"Grim.Taming.Cast DA_Spell_Pactum_Aqua Kelpie 0 42",
	"Grim.Taming.Dump 46",
	"Grim.Taming.Act Name Kelpie Mulgyeol 48",
	"Grim.Taming.Act Mount Kelpie * 50",
	"Grim.Taming.Move 1 0.3 1.5 52",
	"Grim.Taming.Cast DA_Spell_Ember Jackalope 0 58",
	"Grim.Taming.Dump 61",
	"Grim.Taming.Act Dismount * * 63",
	"Grim.Taming.Dump 66",
	"Grim.Taming.Act Wait Kelpie * 70",
	"Grim.Taming.Dump 73"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimTaming: (Display|Warning|Error)|LogGrimCreatures: Display: \[(Server|Client)\] (seed=|약해짐|퀘스트 이벤트)|LogGrimMagic: Display: \[(Server|Client)\] DA_Spell_\w+ 시전|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

# 덤프 비교: "TamingDump" 줄마다 덩어리를 새로 시작하고, 덩어리 안의 tame seed= 줄(역할 접두 뺌)을 모은다.
function Get-DumpBlocks([string]$Path) {
	$Blocks = New-Object System.Collections.ArrayList
	$Current = $null
	$InBlock = $false
	foreach ($Line in (Get-Content -Encoding UTF8 $Path)) {
		if ($Line -match 'LogGrimTaming: Display: \[(Server|Client)\] TamingDump ') {
			if ($InBlock) { [void]$Blocks.Add($Current) }
			$Current = New-Object System.Collections.ArrayList
			$InBlock = $true
		}
		elseif ($InBlock -and $Line -match 'LogGrimTaming: Display: \[(Server|Client)\] (tame seed=.*)$') {
			[void]$Current.Add($Matches[2])
		}
	}
	if ($InBlock) { [void]$Blocks.Add($Current) }
	return ,$Blocks
}

$ServerBlocks = Get-DumpBlocks $ServerLog
$ClientBlocks = Get-DumpBlocks $ClientLog
"===== 덤프 비교 (서버 $($ServerBlocks.Count) · 클라이언트 $($ClientBlocks.Count))"
$Fail = 0
for ($i = 0; $i -lt [Math]::Min($ServerBlocks.Count, $ClientBlocks.Count); $i++) {
	$S = @($ServerBlocks[$i])
	$C = @($ClientBlocks[$i])
	$Diff = if ($S.Count -eq 0 -or $C.Count -eq 0) { if ($S.Count -ne $C.Count) { @("빈 덤프") } else { $null } } else { Compare-Object -ReferenceObject $S -DifferenceObject $C }
	if ($Diff) {
		$Fail++
		"덤프 $($i + 1): 다름"
		$Diff | ForEach-Object { "  $($_.SideIndicator) $($_.InputObject)" }
	}
	else {
		"덤프 $($i + 1): 같음 ($(@($ServerBlocks[$i]).Count)마리)"
	}
}
if ($ServerBlocks.Count -ne $ClientBlocks.Count) { $Fail++; "덤프 수가 다름" }

# 꼭 있어야 하는 서버 줄
$ServerText = Get-Content -Encoding UTF8 -Raw $ServerLog
foreach ($Want in @("길들임 Jackalope", "길들임 Kelpie", "퀘스트 이벤트 Creature.Tamed.Small", "탔다 Kelpie", "내렸다 Kelpie")) {
	if ($ServerText -notmatch [regex]::Escape($Want)) { $Fail++; "서버 로그에 없음: $Want" }
}
$ClientText = Get-Content -Encoding UTF8 -Raw $ClientLog
foreach ($Want in @("Taming.Cast DA_Spell_Ember → Jackalope 발동=1 탄채=1", "탑승 붙임 Kelpie", "탑승 뗌 Kelpie")) {
	if ($ClientText -notmatch [regex]::Escape($Want)) { $Fail++; "클라이언트 로그에 없음: $Want" }
}
"NETCHECK_RESULT fail=$Fail"
