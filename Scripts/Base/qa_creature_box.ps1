# QA: 건축으로 지은 거점 생물 상자(R38 × M4) — 헤드리스 혼자 실행 두 번(같은 세이브 슬롯), 결과 PASS/FAIL. docs/qa/m4-base 의 C5~C9.
#
#   powershell -File Scripts\Base\qa_creature_box.ps1      # 로그: Saved\QA\creature_box_1.log · _2.log, 끝줄 QA_RESULT fail=0
#
# 1회차(슬롯 QABox 를 지우고 시작): 탁 트인 원점에서 재칼로프 하나를 길들임(빗나가면 다시 쏨) → 보관함 배움 →
#   원점에서 +X 를 보고 거점 핵(나무 5 · 돌 5) → 생물 상자 시설(칸 1,-2 · 나무 10 · 돌 4) → 상자에 넣고 풀어놓음 → 저장 → 끔
# 2회차(같은 슬롯으로 다시 켬): 지은 상자 · 칸 · 체력 · 풀어놓은 생물이 되살아났나 → 철거 → 생물이 주인 보관함으로 · 재료 절반 환급
#   → 꺼내서 데리고 다님 → 쿠스토디아를 안 배운 상태로 돌림(디버그) → 상자를 다시 지어 넣고 철거 → 안 배워도 기본 보관함(10칸, Q42)으로 옮겨짐
#   (옮길 곳이 정말 없어 사라지는 경우 — 보관함까지 가득 — 는 이 스크립트로 만들지 않는다)
# 길들이기는 하나만 쓴다 — 헤드리스에서 두 번째 파크툼이 소품에 맞아 빗나가는 일이 있었다.
param(
	[string]$Slot = "QABox",
	[int]$Timeout = 240
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
$SlotDir = Join-Path $Root "Saved\SaveGames\Grimoire\$Slot"
New-Item -ItemType Directory -Force $OutDir | Out-Null
Remove-Item -Recurse -Force $SlotDir -ErrorAction SilentlyContinue
$Log1 = Join-Path $OutDir "creature_box_1.log"
$Log2 = Join-Path $OutDir "creature_box_2.log"
Remove-Item $Log1, $Log2 -ErrorAction SilentlyContinue

function Run([string]$Log, [object[]]$Steps) {
	$Exec = ($Steps | ForEach-Object { "Grim.At $($_[0]) $($_[1])" }) -join ","
	$P = Start-Process -FilePath $Exe -PassThru -ArgumentList @("`"$Project`"", "/Game/Grimoire/Maps/L_M1_Sandbox?SaveSlot=$Slot", "-GrimPlayerId=QAHost",
		"-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-abslog=`"$Log`"", "-ExecCmds=`"$Exec`"")
	if (-not $P.WaitForExit($Timeout * 1000)) { Stop-Process -Id $P.Id -Force; "시간 초과 — 강제 종료" }
}

Run $Log1 @(
	@(5.5, "EnableCheats"), @(5.7, "BugItGo 0 0 200 0 0 0"),
	@(6, "GrimSuperCheat 1"), @(6.2, "Grim.Taming.ForceRoll 0"), @(6.4, "Grim.Taming.Grant 1"),
	@(7, "Grim.Creature.Spawn Jackalope 1 0 400"),
	@(8.5, "Grim.Taming.Cast DA_Spell_Pactum Jackalope 0"), @(10.5, "Grim.Taming.Cast DA_Spell_Pactum Jackalope 0"), @(12.5, "Grim.Taming.Cast DA_Spell_Pactum Jackalope 0"),
	@(14, "Grim.Taming.Dump"),
	@(14.5, "Grim.Items.Give Skillbook_CreatureStorage 1"), @(15, "Grim.Items.Eat Skillbook_CreatureStorage"),
	@(16, "Grim.Items.Give Wood 30"), @(16.2, "Grim.Items.Give Stone 20"),
	@(16.6, "BugItGo 0 0 200 0 0 0"), @(16.8, "Grim.Base.Core 1200"),
	@(18, "Grim.Base.PlaceAt CreatureBox 1 -2 0 100 0"), @(18.5, "Grim.Items.Dump"), @(19, "Grim.Base.Teleport 1 -1 0"),
	@(21, "Grim.Taming.Storage Party Box Jackalope"), @(23, "Grim.Taming.Storage Box Released Jackalope"),
	@(26, "Grim.Taming.StorageDump 0 Local"), @(26.5, "Grim.Base.Dump"),
	@(28, "Grim.Save.Now"), @(30, "quit")
)
Run $Log2 @(
	@(6, "Grim.Base.Teleport 1 -1 0"),
	@(9, "Grim.Taming.StorageDump 0 Local"), @(9.5, "Grim.Base.Dump"), @(9.7, "Grim.Taming.Dump"),
	@(11, "Grim.Base.Demolish GrimBaseCreatureBox_0"),
	@(13, "Grim.Taming.StorageDump 0 Local"), @(13.5, "Grim.Items.Dump"),
	@(15, "Grim.Taming.Storage Storage Party Jackalope"), @(17, "Grim.Taming.StorageLearn 0"),
	@(18, "Grim.Base.PlaceAt CreatureBox 1 -2 0 100 0"), @(19, "Grim.Base.Teleport 1 -1 0"),
	@(21, "Grim.Taming.Storage Party Box Jackalope"),
	@(23, "Grim.Taming.StorageDump 0 Local"), @(23.5, "Grim.Base.Dump"),
	@(24, "Grim.Base.Demolish GrimBaseCreatureBox_1"),
	@(26, "Grim.Taming.StorageDump 0 Local"), @(27, "quit")
)

# ── 판정 ── (한 줄뿐인 결과도 배열로 다루려고 늘 @() 와 At 을 쓴다 — PowerShell 은 원소 하나짜리 배열을 문자열로 풀어 [0] 이 첫 글자가 된다)
function Lines([string]$Log, [string]$Pattern) { if (Test-Path $Log) { @(Select-String -Path $Log -Pattern $Pattern | ForEach-Object { $_.Line }) } else { @() } }
function At([object[]]$Arr, [int]$I) { if ($Arr.Count -gt $I) { $Arr[$I] } else { "(없음)" } }
function Tail([string]$Line) { if ($Line -match "(storage learned=.*|items=\[[^\]]*\]|built .*|tame seed=.*)") { $Matches[1] } else { $Line } }
$Fail = 0
function Check([string]$Name, [bool]$Ok, [string]$Evidence) {
	$script:Fail += [int](-not $Ok)
	"{0} {1} — {2}" -f $(if ($Ok) { "PASS" } else { "FAIL" }), $Name, $Evidence
}

$Tamed1 = @(Lines $Log1 "tame seed=.* owner=QAHost")
Check "준비: 재칼로프를 길들임" ($Tamed1.Count -ge 1) (Tail (At $Tamed1 0))
$D1 = @(Lines $Log1 "\] storage learned=")
$D2 = @(Lines $Log2 "\] storage learned=")
$I1 = @(Lines $Log1 "ItemsDump .* inv=yes")
$I2 = @(Lines $Log2 "ItemsDump .* inv=yes")

$a = At $D1 0
Check "C5 지은 상자에 넣고 풀어놓기" ($a -match "learned=1 party=0/\d+ stored=0/\d+ box=yes boxEntries=1 released=1") (Tail $a)
$Built = @(Lines $Log2 "BaseDump Server\] built GrimBaseCreatureBox_0")
$b = At $D2 0; $Tamed2 = @(Lines $Log2 "tame seed=.* owner=QAHost")
Check "C8 저장 → 다시 켜기: 상자 · 체력 · 칸 · 풀어놓은 생물" (((At $Built 0) -match "hp=200.0/200.0") -and ($b -match "box=yes boxEntries=1 released=1") -and $Tamed2.Count -ge 1) "$(Tail (At $Built 0)) / $(Tail $b) / $(Tail (At $Tamed2 0))"
$c = At $D2 1
Check "C9 철거하면 주인 보관함으로(풀어놓은 액터는 거둠)" ($c -match "party=0/\d+ stored=1/\d+ box=no boxEntries=0 released=0") (Tail $c)
$i0 = At $I1 0; $i1 = At $I2 0
Check "C7 철거 환급 = 비용의 절반(나무 5 · 돌 2)" (($i0 -match "Wood×15" -and $i0 -match "Stone×11") -and ($i1 -match "Wood×20" -and $i1 -match "Stone×13")) "$(Tail $i0) → $(Tail $i1)"
$d = At $D2 2; $e = At $D2 3; $Lost = @(Lines $Log2 "옮길 곳이 없다")
Check "C6 쿠스토디아 없이 철거해도 기본 보관함(10칸, Q42)으로" (($d -match "learned=0 party=0/\d+ .*box=yes boxEntries=1") -and ($e -match "learned=0 party=0/\d+ stored=1/10 box=no") -and $Lost.Count -eq 0) "$(Tail $d) → $(Tail $e) / 경고 $($Lost.Count)줄"
"QA_RESULT fail=$Fail"
