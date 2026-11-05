# 세이브 v1 확인(헤드리스, M2 갈래 V — docs/systems/world-multiplayer 「세이브」).
#
#   powershell -File Scripts\Save\check_save.ps1      # 결과: Saved\NetCheck\save_*.log 요약 + 확인 항목 PASS/FAIL
#
# 슬롯 SaveCheck(Saved\SaveGames\Grimoire\SaveCheck)를 지우고 시작한다. -unattended 실행이라 URL 에 ?SaveSlot= 을 줘야 저장된다.
# 같은 PC 에서 서버 · 클라이언트를 띄우므로 -GrimPlayerId= 로 각자 id 를 준다(호스트 SaveHost, 클라이언트 SaveClient).
#
# 1부 — 서버 S1(리슨) → 클라이언트 C1 → C2(같은 id 로 다시 접속). 초는 각 프로세스가 준비(로컬 폰)된 때 기준:
#   S1 +3 시각 15:00 · 날씨 Meadow=Fog · 양초 3개 켬 · 마른 풀 태움(8초 뒤 탄 자국 = 지속 상태)
#   S1 +4~7 불씨 +150 XP → 서클 2(강제) → 화염구 스킬북을 놓고 읽어 배움
#   S1 +8 HP 42 · 배고픔 15(배고파서 HP 회복 없음) · 마나 17,  +9~10 침대 놓고 눕기 → 부활 지점(낮잠이라 바로 깬다)
#   C1 +3~4 물줄기 +120 XP → 서클 2(강제), +10 끔 → 서버가 Player_SaveClient 저장(퇴장)
#   C2 +6 숙련도 덤프: C1 때 값이 이어져야 한다, +8 끔.  S1 은 두 번째 접속 +14 초에 끈다(quit → 종료 저장)
# 2부 — 서버 S2(같은 슬롯으로 다시 켬) → 클라이언트 C3:
#   S2 +3 덤프: 시각 15~16시대 · Fog · 양초 켜짐 · 풀 탄 자국 · 호스트 서클 2 + 화염구 · HP 42 · 배고픔 ≈15
#   S2 +6 호스트 사망 → 마지막 침대(다시 스폰된 침대 옆)에서 부활
#   C3 접속 +10 양초에 물(꺼짐) → C3 +14 덤프: 꺼짐 → 접속 +16 서버 Grim.Save.Load → C3 +22 덤프: 다시 켜짐(휴면 깨우기 순서)
#   C3 +23 숙련도: SaveClient 의 서클 2 · 물줄기 XP 가 서버를 껐다 켜도 이어진다
param(
	[int]$Port = 17790,
	[string]$Slot = "SaveCheck"
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheck"
$SlotDir = Join-Path $Root "Saved\SaveGames\Grimoire\$Slot"
New-Item -ItemType Directory -Force $OutDir | Out-Null
Remove-Item -Recurse -Force $SlotDir -ErrorAction SilentlyContinue

$Logs = @{}
foreach ($Name in @("S1", "C1", "C2", "S2", "C3")) {
	$Logs[$Name] = Join-Path $OutDir "save_$Name.log"
	Remove-Item $Logs[$Name] -ErrorAction SilentlyContinue
}

$Map = "/Game/Grimoire/Maps/L_M1_Sandbox?listen?SaveSlot=$Slot"
$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")

function Start-Game([string]$Name, [string]$Url, [string]$PlayerId, [string[]]$Cmds) {
	$ArgList = @("`"$Project`"", $Url, "-GrimPlayerId=$PlayerId", "-abslog=`"$($Logs[$Name])`"", "-ExecCmds=`"$($Cmds -join ",")`"") + $Common
	if ($Url -like "/Game/*") { $ArgList += "-port=$Port" }
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

# 시간표는 Grim.Save.At(그 프로세스가 준비 — 로컬 폰 — 된 때 기준)과 Grim.Save.AtLogins(서버: 원격 접속 N 번째 기준)로 짠다.
# 헤드리스 시동 시간이 다른 갈래의 빌드 · 에디터 때문에 들쭉날쭉해도 순서가 지켜진다.

# ───── 1부 ─────
$S1Cmds = @(
	"Grim.Save.At 3 grim.Time.Set 15",
	"Grim.Save.At 3 Grim.Weather.Set Meadow Fog 0",
	"Grim.Save.At 3 Grim.Interaction.Stimulus Element.Fire 1 0 Candle",
	"Grim.Save.At 3 Grim.Interaction.Stimulus Element.Fire 1 0 DryGrass",
	"Grim.Save.At 4 Grim.Magic.AddXP Ember 150",
	"Grim.Save.At 5 Grim.Magic.CircleUp 1",
	"Grim.Save.At 6 Grim.Magic.SpawnSkillbook Fireball 150",
	"Grim.Save.At 7 Grim.Magic.ReadSkillbook",
	"Grim.Save.At 8 Grim.Save.SetAttr Hunger 15",
	"Grim.Save.At 8 Grim.Save.SetAttr Health 42",
	"Grim.Save.At 8 Grim.Save.SetAttr Mana 17",
	"Grim.Save.At 9 grim.Survival.SpawnBed",
	"Grim.Save.At 10 grim.Survival.Sleep",
	"Grim.Save.At 16 Grim.Magic.Proficiency",
	"Grim.Save.At 16 grim.Survival.Dump",
	"Grim.Save.At 16 Grim.Interaction.Dump",
	"Grim.Save.At 16 Grim.Weather.Dump",
	"Grim.Save.AtLogins 2 12 Grim.Save.Dump",
	"Grim.Save.AtLogins 2 13 Grim.Magic.Proficiency",
	"Grim.Save.AtLogins 2 13 grim.Survival.Dump",
	"Grim.Save.AtLogins 2 14 Grim.Save.Quit"
)
$C1Cmds = @(
	"Grim.Save.At 3 Grim.Magic.AddXP WaterJet 120",
	"Grim.Save.At 4 Grim.Magic.CircleUp 1",
	"Grim.Save.At 8 Grim.Magic.Proficiency",
	"Grim.Save.At 10 Grim.Save.Quit"
)
$C2Cmds = @(
	"Grim.Save.At 6 Grim.Magic.Proficiency",
	"Grim.Save.At 8 Grim.Save.Quit"
)

Write-Host "1부: S1 → C1 → C2"
$S1 = Start-Game "S1" $Map "SaveHost" $S1Cmds
$null = Wait-Log "S1" "PlayerId SaveHost" 300
Start-Sleep -Seconds 18
$C1 = Start-Game "C1" "127.0.0.1:$Port" "SaveClient" $C1Cmds
Wait-Exit $C1 300
$null = Wait-Log "S1" "PlayerId SaveClient 저장\(퇴장\)" 30
$C2 = Start-Game "C2" "127.0.0.1:$Port" "SaveClient" $C2Cmds
Wait-Exit $C2 300
Wait-Exit $S1 60

# ───── 2부 ─────
$S2Cmds = @(
	"Grim.Save.At 3 Grim.Magic.Proficiency",
	"Grim.Save.At 3 grim.Survival.Dump",
	"Grim.Save.At 3 Grim.Interaction.Dump",
	"Grim.Save.At 3 Grim.Weather.Dump",
	"Grim.Save.At 3 Grim.Save.Dump",
	"Grim.Save.At 6 grim.Survival.Die",
	"Grim.Save.AtLogins 1 10 Grim.Interaction.Stimulus Element.Water 1 0 Candle",
	"Grim.Save.AtLogins 1 16 Grim.Save.Load",
	"Grim.Save.AtLogins 1 18 Grim.Interaction.Dump",
	"Grim.Save.AtLogins 1 30 Grim.Save.Quit"
)
$C3Cmds = @(
	"Grim.Save.At 4 Grim.Interaction.Dump",
	"Grim.Save.At 14 Grim.Interaction.Dump",
	"Grim.Save.At 22 Grim.Interaction.Dump",
	"Grim.Save.At 23 Grim.Magic.Proficiency",
	"Grim.Save.At 26 Grim.Save.Quit"
)

Write-Host "2부: S2(같은 슬롯) → C3"
$S2 = Start-Game "S2" $Map "SaveHost" $S2Cmds
$null = Wait-Log "S2" "PlayerId SaveHost\(로컬\) 불러옴" 300
$C3 = Start-Game "C3" "127.0.0.1:$Port" "SaveClient" $C3Cmds
Wait-Exit $C3 300
Wait-Exit $S2 60

# ───── 요약 ─────
$Pattern = "LogGrimSave: |LogGrimMagic: .*(Proficiency|세이브|서클|배운|  DA_)|LogGrimSurvival: .*(SurvivalDump|세이브 읽음|잠듦|부활|사망)|LogGrimInteraction: Display: \[.*\] Grim(Candle|DryGrass|Campfire)|LogGrimEnvironment: .*(세이브 읽음|Meadow)|Welcomed by server|Error: "
foreach ($Name in @("S1", "C1", "C2", "S2", "C3")) {
	"===== $Name"
	if (Test-Path $Logs[$Name]) {
		Select-String -Encoding UTF8 -Path $Logs[$Name] -Pattern $Pattern | ForEach-Object { $_.Line -replace "^\[[^\]]*\]\[[^\]]*\]", "" }
	}
}

function Text([string]$Name) { if (Test-Path $Logs[$Name]) { return [IO.File]::ReadAllText($Logs[$Name]) } return "" }
$T = @{}
foreach ($Name in $Logs.Keys) { $T[$Name] = Text $Name }

$Results = @()
function Check([string]$What, [bool]$Ok) {
	$script:Results += [PSCustomObject]@{ Result = $(if ($Ok) { "PASS" } else { "FAIL" }); What = $What }
}

# 세이브 파일
Check "세계 파일 WorldSave_$Slot.sav" (Test-Path (Join-Path $SlotDir "WorldSave_$Slot.sav"))
Check "백업 WorldSave_$Slot.sav.bak" (Test-Path (Join-Path $SlotDir "WorldSave_$Slot.sav.bak"))
Check "호스트 Player_SaveHost.sav" (Test-Path (Join-Path $SlotDir "Player_SaveHost.sav"))
Check "클라이언트 Player_SaveClient.sav" (Test-Path (Join-Path $SlotDir "Player_SaveClient.sav"))

# 1부: 클라이언트 퇴장 저장 → 다시 접속하면 이어짐
Check "S1: 새 세계로 시작" ($T["S1"] -match "세계 세이브 없음")
Check "S1: 클라이언트 id 는 로그인 옵션에서" ($T["S1"] -match "PlayerId SaveClient\(로그인 옵션\)")
Check "S1: C1 퇴장 때 저장" ($T["S1"] -match "PlayerId SaveClient 저장\(퇴장\)")
Check "S1: C2 접속 때 불러옴" ($T["S1"] -match "PlayerId SaveClient\(로그인 옵션\) 불러옴\(접속\)")
Check "S1: 종료 저장(세계)" ($T["S1"] -match "세계 저장\(종료\)")
Check "C2: 서클 2 이어짐" ($T["C2"] -match "\[Client\] Proficiency \S+ 서클=2")
Check "C2: 물줄기 XP 120 이어짐" ($T["C2"] -match "\[Client\]   DA_Spell_WaterJet \S+ 등급=\S+ XP=12\d\.")

# 2부: 서버를 껐다 켜도 이어짐
Check "S2: 세계 불러옴" ($T["S2"] -match "세계 불러옴\(시작\)")
Check "S2: 시각 15~16시대" ($T["S2"] -match "시간 세이브 읽음: \d+일 1[56]\.")
Check "S2: 날씨 Fog" ($T["S2"] -match "날씨 세이브 읽음: Meadow = Fog")
Check "S2: 켜진 양초" ($T["S2"] -match "\] GrimCandle\S* states=\{State\.Burning=1\.00")
Check "S2: 탄 풀(지속 상태)" ($T["S2"] -match "\] GrimDryGrass\S* states=\{State\.Charred=1\.00")
Check "S2: 모닥불은 꺼진 채" ($T["S2"] -match "\] GrimCampfire\S* states=\{\}")
Check "S2: 호스트 서클 2" ($T["S2"] -match "Proficiency \S+ 서클=2")
Check "S2: 호스트 화염구" ($T["S2"] -match "\]   DA_Spell_Fireball")
Check "S2: 호스트 불씨 XP 150" ($T["S2"] -match "\]   DA_Spell_Ember \S+ 등급=\S+ XP=15\d\.")
Check "S2: HP 42 · 배고픔 13~15" ($T["S2"] -match "세이브 읽음: HP 42\.\d/\d+\.\d 배고픔 1[3-5]\.")
Check "S2: 부활 지점 이어짐" ($T["S2"] -match "세이브 읽음: HP .* 부활 지점 V\(")
Check "S2: 침대 다시 스폰" ($T["S2"] -match "스폰 액터 [1-9]\d* 다시 만듦")
Check "S2: 마지막 침대에서 부활" ($T["S2"] -match "부활 — 마지막 침대")
Check "S2: 호스트 불러옴" ($T["S2"] -match "PlayerId SaveHost\(로컬\) 불러옴\(접속\)")

# 2부: 게임 중 Grim.Save.Load → 원격 클라이언트에 복제(휴면 깨우기)
$C3Lines = @()
if (Test-Path $Logs["C3"]) { $C3Lines = Select-String -Encoding UTF8 -Path $Logs["C3"] -Pattern "\[Client\] GrimCandle" | ForEach-Object { $_.Line } }
Check "C3: 접속 때 양초 켜짐(불러온 세계가 복제)" ($C3Lines.Count -ge 9 -and $C3Lines[0] -match "State\.Burning=1\.00")
Check "C3: 물을 붓자 꺼짐" ($C3Lines.Count -ge 9 -and $C3Lines[3] -notmatch "State\.Burning")
Check "C3: Grim.Save.Load 뒤 다시 켜짐" ($C3Lines.Count -ge 9 -and $C3Lines[6] -match "State\.Burning=1\.00")
Check "C3: 서버를 다시 켜도 클라이언트 서클 2" ($T["C3"] -match "\[Client\] Proficiency \S+ 서클=2")

""
"===== 확인"
$Results | Format-Table -AutoSize | Out-String -Width 200
$Failed = @($Results | Where-Object { $_.Result -eq "FAIL" }).Count
"실패 $Failed / $($Results.Count)"
exit $Failed
