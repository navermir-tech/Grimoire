# M2 생존 2인 확인(헤드리스): net_check.ps1 과 같은 방식(리슨 서버 + 클라이언트, -game -nullrhi).
# 잠 · 밤 건너뛰기(전원 비율) · 저항 · 쓰러짐 · 일으킴 · 침대 부활 · 피곤함 패널티를 양쪽 덤프로 남긴다.
#
#   powershell -File Scripts\Survival\net_check_survival.ps1     # 결과: Saved\NetCheck\survival_server.log · survival_client.log 의 요약
#
# 시간표(서버 시작 기준 초, 클라이언트는 12초 뒤 시작 — 클라이언트 지연 = 서버 시각 − 12):
#   0  22시로
#   25 호스트 앞에 침대 → 26 호스트가 침대에 눕기 → 28 덤프   기대: 호스트 Sleeping, 시간 그대로(클라이언트가 깨어 있음)
#   30 클라이언트 저항 0 3 → 31 클라이언트 그 자리에 눕기 → (3초 페이드) 밤 건너뜀
#   37 덤프   기대: 양쪽 time≈0.19 day=1, Fatigue 0, 아무도 Sleeping 아님, 클라이언트 Cold 등급 있어도 태그 없음(저항 3)
#   39 클라이언트 쓰러짐 → 41 덤프   기대: 클라이언트 Status.Downed, HP 0, Walk 줄어듦(양쪽 같은 태그)
#   42 클라이언트 일으킴 → 44 호스트 사망 → 46 덤프   기대: 클라이언트 HP 30, 호스트 침대 옆에서 부활(HP 50)
#   47 클라이언트 피곤함 85 → 49 덤프   기대: 클라이언트 Status.Exhausted, Move 0.70(지친 걸음), Cast 1.00(시전은 차단 태그로 — Q116)
#   50 클라이언트 탈진 중 슬롯 1 → 서버 「마법 실패 … Exhausted」(Q116), 51.5 기본 공격 → 주먹질(마법이 아니라 됨)
param(
	[int]$Port = 17779,
	[int]$RunSeconds = 60
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "survival_server.log"
$ClientLog = Join-Path $OutDir "survival_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

$ServerCmds = @(
	"grim.Time.Set 22",
	"grim.Survival.At 25 SpawnBed",
	"grim.Survival.At 26 Sleep",
	"grim.Survival.At 28 Dump",
	"grim.Survival.At 37 Dump",
	"grim.Survival.At 41 Dump",
	"grim.Survival.At 44 Die",
	"grim.Survival.At 46 Dump",
	"grim.Survival.At 49 Dump"
) -join ","
$ClientCmds = @(
	"grim.Survival.At 16 Dump",
	"grim.Survival.At 18 Resist 0 3",
	"grim.Survival.At 19 Sleep",
	"grim.Survival.At 25 Dump",
	"grim.Survival.At 27 Down",
	"grim.Survival.At 29 Dump",
	"grim.Survival.At 30 Revive",
	"grim.Survival.At 34 Dump",
	"grim.Survival.At 35 Fatigue 85",
	"grim.Survival.At 37 Dump",
	"Grim.At 38 Grim.Input.Press Slot1 None",
	"Grim.At 39.5 Grim.Input.Press Basic None"
) -join ","

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
Start-Sleep -Seconds 12
$Client = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$ClientCmds`"") + $Common)

Start-Sleep -Seconds $RunSeconds
foreach ($P in @($Client, $Server)) { if (-not $P.HasExited) { Stop-Process -Id $P.Id -Force } }

foreach ($Log in @($ServerLog, $ClientLog)) {
	"===== $(Split-Path -Leaf $Log)"
	Select-String -Path $Log -Pattern "LogGrimSurvival|Welcomed by server|Error: |LogGrimMagic" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}
