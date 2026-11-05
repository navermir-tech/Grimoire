# 외형 복제 2인 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고,
# 서버가 모든 플레이어 외형을 씨앗으로 바꾸고 규칙대로 사람을 줄 세운 뒤, 양쪽 Grim.Appearance.Dump 의 해시를 비교한다.
#
#   powershell -File Scripts\Character\net_check_appearance.ps1      # 끝에 PASS / FAIL. 로그는 Saved\NetCheck\appearance_*.log
#
# 시간표(서버 시작 기준 초): 12 클라이언트 시작 → Start(35) 서버 Randomize 777 · 시험관 3명 · 사서 2명 줄 세움 → Start+15 양쪽 덤프.
# 클라이언트는 12초 늦게 시작하므로 클라이언트 지연 = 서버 시각 − 12. nullrhi 라 조각 수는 0 일 수 있다(전용 서버가 아니면 만든다).
param(
	[int]$Port = 17787,
	# 서버가 외형을 바꾸는 시각(서버 시작 기준 초). 기계가 바쁘면(에디터 여러 개) 맵이 늦게 뜨니 늘린다
	[int]$Start = 35
)
$RunSeconds = $Start + 40

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "appearance_server.log"
$ClientLog = Join-Path $OutDir "appearance_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ServerCmds = @(
	"Grim.At $Start Grim.Appearance.Randomize 777",
	"Grim.At $($Start + 1) Grim.Appearance.Lineup Tower_Examiner 3 10",
	"Grim.At $($Start + 2) Grim.Appearance.Lineup Tower_Librarian 2 20",
	"Grim.At $($Start + 15) Grim.Appearance.Dump"
) -join ","
$ClientCmds = @(
	"Grim.At $($Start + 3) Grim.Appearance.Dump"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

function Read-Dump([string]$Log) {
	$Map = @{}
	Select-String -Path $Log -Pattern "\[AppearanceDump\] \S+ key=(\S+) hash=(\S+)" | ForEach-Object {
		$Map[$_.Matches[0].Groups[1].Value] = $_.Matches[0].Groups[2].Value
	}
	return $Map
}

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "\[AppearanceDump\]|\[Appearance\]|LogNet: Join|Welcomed by server|LogGrimAppearance: Error" | ForEach-Object { $_.Line }
}

$ServerDump = Read-Dump $ServerLog
$ClientDump = Read-Dump $ClientLog
$Fail = @()
if ($ServerDump.Count -lt 7) { $Fail += "서버 덤프가 모자라다($($ServerDump.Count) < 플레이어 2 + 더미 5)" }
foreach ($Key in $ServerDump.Keys) {
	if (-not $ClientDump.ContainsKey($Key)) { $Fail += "클라이언트에 없음: $Key" }
	elseif ($ClientDump[$Key] -ne $ServerDump[$Key]) { $Fail += "해시 다름: $Key 서버 $($ServerDump[$Key]) · 클라이언트 $($ClientDump[$Key])" }
}
if ($Fail.Count -eq 0) { "PASS — 외형 $($ServerDump.Count)개가 양쪽에서 같다" } else { "FAIL"; $Fail }
