# M2 갈래 T 확인(헤드리스 단독 게임, L_Tower): 튜토리얼 퀘스트를 콘솔로 처음부터 끝까지 밟는다.
# 각 단계의 조건을 콘솔로 만족시키고(순간이동 · 마법 자극 · 대화 · 이벤트 · 잠), 퀘스트가 다음 단계로 넘어가는지 로그로 본다.
#
#   powershell -File Scripts\Tutorial\check_tutorial.ps1     # 결과: Saved\NetCheck\tutorial.log 의 요약, 끝에 PASS/FAIL
#
# 아직 없는 것은 대신한다: 기본 공격 · 파크툼 정의(갈래 K · M3)가 없어 그 스킬북 단계는 Skip, 생물 · 요리 · 거점(M3 · M4)은 Event.
# 시작 마법 지급(UGrimMagicSettings::StartingSpells)이 아직 켜져 있으면 플라미스 · 아쿠엘 · 벤투스 단계는 시작하자마자 채워진다.
#
# 시간표(시작 기준 초) — 기대:
#   15 덤프: Tutorial 0 · 16~17 스승 → 대화 → 1(기본 공격 스킬북) → 18 Skip → 2(과녁 켜짐) → 19~20 과녁 세 번 → 부서짐 → 3
#   21 대화 → 4(세 스킬북을 가방에) → 22 가방에서 세 책 사용 → 5 · 23 양초 불 → 6 · 24 바람 → 7 · 25 물 → 8 · 26 작물 셋에 물 → 9
#   27 앞마당(Zone.NoCombat 뗌) → 10 · 28 사냥 · 28.5 고기 → 12(파크툼) → 29 Skip → 13 · 29.5 30% · 30 길들임 → 15
#   31 주방(Zone.NoCombat 붙임) → 16 · 32 아궁이 불 → 17 · 33~33.5 재료 · 완성 → 18 · 34 먹기 → 19
#   35~36 스승 → 대화 → 20(해넘이, 모닥불 곁 침낭) · 37 모닥불 → 21 · 38 침낭 곁 → 39 눕기 → (페이드 3초) 밤 건너뜀, 피로 0 → 22 · 45 덤프
#   46~47 스승 → 대화 → 23 · 48 거점 자리 → 24 · 49 거점 이벤트 → Tutorial 완료(경험치) · 50 덤프
#   51 시험 양초 미리 켬 → 52 스승 → 52.1 시험 청함: 자격 모자람(Trial.NotReady + PromotionGrade 알림, Q122) → 52.3 플라미스 · 아쿠엘 숙련도 채움 → 52.5~53 시험 → 시험의 방: 양초를 처음 상태로(새로 놓음) → 53.5 포기(실패) → 54~55 다시 → 56 양초 셋 → 통과, 서클 2, 이그니스
#   57.3 3서클 시험 청함: 자격 모자람(B 이상 2개 중 지금 서클 마법) → 57.5 이그니스 숙련도 채움 → 58~59 3서클 시험 → 60~61 표적 세 번 · 62~63 젖은 양초에 불 셋 → 통과, 서클 3 · 64 덤프
#   65 사냥꾼 거점(발동 볼륨) → 사냥꾼 퀘스트 → 66~67 스승 → 대화 → 완료 · 68 정화 의식(노말) · 69 덤프
param(
	# 지연은 엔진 시작부터 센다 — 맵 로드(10초 안팎)를 더해 마지막 덤프(69)보다 넉넉히
	[int]$RunSeconds = 95
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
	"Grim.Quest.At 18 Skip Tutorial",
	"Grim.Quest.At 19 Stimulus Tut_TrainingTarget Element.Fire 1",
	"Grim.Quest.At 19.5 Stimulus Tut_TrainingTarget Element.Fire 1",
	"Grim.Quest.At 20 Stimulus Tut_TrainingTarget Element.Fire 1",
	"Grim.Quest.At 21 Talk Master",
	"Grim.Items.At 22 Eat Skillbook_Ember",
	"Grim.Items.At 22.2 Eat Skillbook_WaterJet",
	"Grim.Items.At 22.4 Eat Skillbook_Breeze",
	"Grim.Quest.At 23 Stimulus Tut_Candle Element.Fire 1",
	"Grim.Quest.At 24 Stimulus Tut_Candle Element.Wind 1",
	"Grim.Quest.At 25 Stimulus Tut_Candle Element.Water 1",
	"Grim.Quest.At 26 Stimulus Tut_Crop Element.Water 1",
	"Grim.Quest.At 27 Teleport Tut_Yard 0",
	"Grim.Quest.At 28 Event Creature.Killed.Small",
	"Grim.Quest.At 28.5 Event Item.Obtained.Meat",
	"Grim.Quest.At 29 Skip Tutorial",
	"Grim.Quest.At 29.5 Event Creature.Weakened.Small",
	"Grim.Quest.At 30 Event Creature.Tamed.Small",
	"Grim.Quest.At 31 Teleport Tut_Kitchen 0",
	"Grim.Quest.At 32 Stimulus Tut_Stove Element.Fire 1",
	"Grim.Quest.At 33 Event Cooking.IngredientsAdded",
	"Grim.Quest.At 33.5 Event Cooking.Completed",
	"Grim.Quest.At 34 Event Food.Eaten",
	"Grim.Quest.At 35 Teleport Npc_Master 150",
	"Grim.Quest.At 36 Talk Master",
	"Grim.Quest.At 37 Stimulus Tut_Campfire Element.Fire 1",
	"Grim.Quest.At 38 Teleport Given_Bedroll 120",
	"grim.Survival.At 39 Sleep",
	"Grim.Quest.At 45 Dump",
	"Grim.Quest.At 46 Teleport Npc_Master 150",
	"Grim.Quest.At 47 Talk Master",
	"Grim.Quest.At 48 Teleport Tut_BaseSite 0",
	"Grim.Quest.At 49 Event Base.Founded",
	"Grim.Quest.At 50 Dump",
	"Grim.Quest.At 51 Stimulus Trial2_Candle Element.Fire 1",
	"Grim.Quest.At 52 Teleport Npc_Master 150",
	"Grim.Quest.At 52.1 Service Master Trial",
	"Grim.Magic.AddXP Ember 100000 52.3",
	"Grim.Magic.AddXP WaterJet 100000 52.35",
	"Grim.Quest.At 52.5 Service Master Trial",
	"Grim.Quest.At 53 Teleport Trial_Room 0",
	"Grim.Quest.At 53.5 Fail Trial_Circle2",
	"Grim.Quest.At 54 Teleport Npc_Master 150",
	"Grim.Quest.At 54.5 Service Master Trial",
	"Grim.Quest.At 55 Teleport Trial_Room 0",
	"Grim.Quest.At 55.5 Dump",
	"Grim.Quest.At 56 Stimulus Trial2_Candle Element.Fire 1",
	"Grim.Quest.At 57 Teleport Npc_Master 150",
	"Grim.Quest.At 57.3 Service Master Trial",
	"Grim.Magic.AddXP Fireball 100000 57.5",
	"Grim.Quest.At 58 Service Master Trial",
	"Grim.Quest.At 59 Teleport Trial_Room 0",
	"Grim.Quest.At 60 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 60.5 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 61 Stimulus Trial3_Target Element.Fire 1",
	"Grim.Quest.At 62 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 62.5 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 63 Stimulus Trial3_WetCandle Element.Fire 1",
	"Grim.Quest.At 64 Dump",
	"Grim.Quest.At 65 Teleport Hunter_Camp 0",
	"Grim.Quest.At 66 Teleport Npc_Master 150",
	"Grim.Quest.At 67 Talk Master",
	"Grim.Quest.At 68 Service Master Purify:Normal",
	"Grim.Quest.At 69 Dump"
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
	"과녁 부숨" = "부서짐"
	"전투 금지 구역 붙임" = "Zone.NoCombat 붙임"
	"전투 금지 구역 뗌" = "Zone.NoCombat 뗌"
	"스킬북 가방에" = "스킬북 Skillbook_Ember 를 가방에 넣었다"
	"침낭 놓음" = "침낭을 놓았다"
	"첫 밤 신호" = "밤을 건너뛰었다"
	"튜토리얼 완료" = "퀘스트 완료: Tutorial"
	"퀘스트 경험치" = "퀘스트 경험치 100"
	"시험 소품 되돌림" = "처음 상태로"
	"승급 자격 모자람 대사" = "서비스 Trial → Trial.NotReady"
	"승급 자격 까닭 알림" = " PromotionGrade 값="
	"시험 실패 뒤 다시" = "퀘스트 실패: Trial_Circle2"
	"2서클 시험 통과" = "퀘스트 완료: Trial_Circle2"
	"3서클 시험 통과" = "퀘스트 완료: Trial_Circle3"
	"서클 3" = "circle=3"
	"사냥꾼 첫 등장" = "퀘스트 완료: Hunter_FirstEncounter"
	"정화 의식 응답" = "서비스 Purify:Normal"
}
$Failed = 0
foreach ($Name in $Checks.Keys) {
	$Ok = $Text -and $Text.Contains($Checks[$Name])
	if (-not $Ok) { $Failed++ }
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
