# M3 생물 2인 복제 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고, 디버그 콘솔 명령으로
# 서버에서 생물을 놓고(콘솔 · 서식 구역) · 약하게 · 죽이고 · 밤에 검은 개가 재칼로프를 사냥하게 한 뒤, 양쪽 덤프가 같은지 본다.
#
#   powershell -File Scripts\Creatures\net_check_creatures.ps1        # 결과: Saved\NetCheckCreatures\server.log · client.log 의 요약 + 덤프 비교
#
# 시간표(서버 시작 기준 초, 클라이언트는 12초 뒤 시작 → 클라이언트 지연 = 서버 시각 − 12):
#   18 정오 → 20 재칼로프 3 · 21 켈피 1 · 22 재칼로프 서식 구역(상한 4) → 30 덤프 ① → 34 재칼로프 피해 22(약해짐) → 37 덤프 ② →
#   40 재칼로프 모두 죽임(전리품 · 퀘스트 이벤트) → 43 덤프 ③ · 44 개체군 → 50 자정 → 52 검은 개 1 · 재칼로프 2(사냥) → 75 덤프 ④ →
#   80 검은 개 죽임 → 83 덤프 ⑤ · 84 개체군.
# 덤프 줄(seed · 종 · 성별 · 크기 · 색 · 체력 · 죽음 · 숨음 · 죽인 이 · 전리품)이 같은 차례끼리 서버 · 클라이언트에서 같아야 한다. AI 상태 줄은 판단 주기 차이로 다를 수 있어 비교에서 뺀다.
param(
	[int]$Port = 17781,
	[int]$RunSeconds = 100,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox",
	[string]$LogCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckCreatures"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @()
if ($LogCmds) { $Common += "-LogCmds=`"$LogCmds`"" }
$Common += @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ServerCmds = @(
	"Grim.Creature.Time 0.5 18",
	"Grim.Creature.Spawn Jackalope 3 20 2000",
	"Grim.Creature.Spawn Kelpie 1 21 1800",
	"Grim.Creature.Habitat Jackalope 4 22 3500 600",
	"Grim.Creature.Dump 30",
	"Grim.Creature.Damage 22 34 Jackalope",
	"Grim.Creature.Dump 37",
	"Grim.Creature.Kill 40 Jackalope",
	"Grim.Creature.Dump 43",
	"Grim.Creature.Eco 44",
	"Grim.Creature.Time 0.0 50",
	"Grim.Creature.Spawn BlackDog 1 52 1500",
	"Grim.Creature.Spawn Jackalope 2 52 1900",
	"Grim.Creature.Dump 75",
	"Grim.Creature.Kill 80 BlackDog",
	"Grim.Creature.Dump 83",
	"Grim.Creature.Eco 84"
) -join ","
$ClientCmds = @(
	"Grim.Creature.Dump 18",
	"Grim.Creature.Dump 25",
	"Grim.Creature.Dump 31",
	"Grim.Creature.Dump 63",
	"Grim.Creature.Dump 71"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimCreatures: (Display|Warning|Error)|LogNet: Join|Welcomed by server|Error: " | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

# 덤프 비교: "Dump 생물" 줄마다 덩어리를 새로 시작하고, 덩어리 안의 seed= 줄(역할 접두 뺌)을 모은다.
function Get-DumpBlocks([string]$Path) {
	# 빈 배열은 PowerShell 에서 거짓이라 덩어리 시작 여부를 따로 든다.
	$Blocks = New-Object System.Collections.ArrayList
	$Current = $null
	$InBlock = $false
	foreach ($Line in (Get-Content -Encoding UTF8 $Path)) {
		if ($Line -match 'LogGrimCreatures: Display: \[(Server|Client)\] Dump ') {
			if ($InBlock) { [void]$Blocks.Add($Current) }
			$Current = New-Object System.Collections.ArrayList
			$InBlock = $true
		}
		elseif ($InBlock -and $Line -match 'LogGrimCreatures: Display: \[(Server|Client)\] (seed=.*)$') {
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
"NETCHECK_RESULT fail=$Fail"
