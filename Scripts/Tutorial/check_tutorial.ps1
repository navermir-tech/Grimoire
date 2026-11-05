# M2 갈래 T 확인(헤드리스 단독 게임, L_Tower): 튜토리얼 퀘스트를 콘솔로 처음부터 끝까지 밟는다.
# 각 단계의 조건을 콘솔로 만족시키고(순간이동 · 마법 자극 · 대화 · 잠 · 스킬북), 퀘스트가 다음 단계로 넘어가는지 로그로 본다.
#
#   powershell -File Scripts\Tutorial\check_tutorial.ps1     # 결과: Saved\NetCheck\tutorial.log 의 요약, 끝에 PASS/FAIL
#
# 시간표(시작 기준 초) — 기대:
#   15 덤프: Tutorial 단계 0
#   16~17 스승 곁 → 대화 → 1 · 18 양초에 불 → 2 · 19 바람 → 3 · 20 물 → 4 · 21 탑 밖 → 5 · 22 모닥불 → 6
#   23~24 스승 곁 → 대화 → 7(시각이 해넘이 + 0.2시간으로) · 25 덤프 time≈0.821
#   26 침대 곁 → 27 눕기 → (페이드 3초) 밤 건너뜀 → 8 · 34 덤프 day=1
#   35 시험 양초를 미리 켜 둠 → 36 스승 → 서비스 Trial(Trial.Started) → 37 시험의 방 → 시험 단계 1(40초): 켜진 양초를 처음 상태로 되돌림(새로 놓음)
#   37.5 시험 포기(실패) → 38~39 다시 청함 → 시험의 방 → 39.5 덤프 counts=0 → 40 양초 셋에 불 → 통과, 서클 2 → 9
#   42~43 스킬북 곁 → 읽기(화염구 Learned) → 10 · 44 덤프
#   45~46 스승 → 서비스 Trial → 47 시험의 방 → 표적 켜짐 → 48~49 표적 세 번 · 50~51 젖은 양초에 불 셋(말림 · 말림 · 켜짐) → 통과, 서클 3 → 11
#   53~54 스승 → 대화 → Tutorial 완료 · 56 덤프
param(
	# 지연은 엔진 시작부터 센다 — 맵 로드(10초 안팎)를 더해 마지막 덤프(56)보다 넉넉히
	[int]$RunSeconds = 85
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "tutorial.log"
Remove-Item $Log -ErrorAction SilentlyContinue

$Cmds = @(
	"Grim.Quest.At 15 Dump",
	"Grim.Quest.At 16 Teleport Npc_Master 150",
	"Grim.Quest.At 17 Talk Master",
	"Grim.Quest.At 18 Stimulus Tut_Candle Element.Fire 1",
	"Grim.Quest.At 19 Stimulus Tut_Candle Element.Wind 1",
	"Grim.Quest.At 20 Stimulus Tut_Candle Element.Water 1",
	"Grim.Quest.At 21 Teleport Tut_Exit 0",
	"Grim.Quest.At 22 Stimulus Tut_Campfire Element.Fire 1",
	"Grim.Quest.At 23 Teleport Npc_Master 150",
	"Grim.Quest.At 24 Talk Master",
	"Grim.Quest.At 25 Dump",
	"Grim.Quest.At 26 Teleport Tut_Bed 150",
	"grim.Survival.At 27 Sleep",
	"Grim.Quest.At 34 Dump",
	"Grim.Quest.At 35 Stimulus Trial2_Candle Element.Fire 1",
	"Grim.Quest.At 36 Teleport Npc_Master 150",
	"Grim.Quest.At 36.5 Service Master Trial",
	"Grim.Quest.At 37 Teleport Trial_Room 0",
	"Grim.Quest.At 37.5 Fail Trial_Circle2",
	"Grim.Quest.At 38 Teleport Npc_Master 150",
	"Grim.Quest.At 38.5 Service Master Trial",
	"Grim.Quest.At 39 Teleport Trial_Room 0",
	"Grim.Quest.At 39.5 Dump",
	"Grim.Quest.At 40 Stimulus Trial2_Candle Element.Fire 1",
	"Grim.Quest.At 41 Dump",
	"Grim.Quest.At 42 Teleport Tut_Skillbook 150",
	"Grim.Magic.ReadSkillbook 43",
	"Grim.Quest.At 44 Dump",
	"Grim.Quest.At 45 Teleport Npc_Master 150",
	"Grim.Quest.At 46 Service Master Trial",
	"Grim.Quest.At 47 Teleport Trial_Room 0",
	"Grim.Quest.At 48 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 48.5 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 49 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 50 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 50.5 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 51 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 52 Dump",
	"Grim.Quest.At 53 Teleport Npc_Master 150",
	"Grim.Quest.At 54 Talk Master",
	"Grim.Quest.At 56 Dump"
) -join ","

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_Tower", "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=320", "-ResY=240", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
Start-Sleep -Seconds $RunSeconds
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimQuest: |LogGrimSurvival: .*(잠듦|건너|깨)|LogGrimMagic: .*(서클|Learned|배웠|스킬북)|Error: " | ForEach-Object { $_.Line }

$Text = Get-Content $Log -Raw -Encoding UTF8
$Checks = [ordered]@{
	"튜토리얼 시작" = "퀘스트 시작: Tutorial"
	"첫 밤 신호" = "밤을 건너뛰었다"
	"시험 소품 되돌림" = "처음 상태로"
	"시험 실패 뒤 다시" = "퀘스트 실패: Trial_Circle2"
	"2서클 시험 통과" = "퀘스트 완료: Trial_Circle2"
	"3서클 시험 통과" = "퀘스트 완료: Trial_Circle3"
	"튜토리얼 완료" = "퀘스트 완료: Tutorial"
	"서클 3" = "circle=3"
}
$Failed = 0
foreach ($Name in $Checks.Keys) {
	$Ok = $Text -and $Text.Contains($Checks[$Name])
	if (-not $Ok) { $Failed++ }
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
