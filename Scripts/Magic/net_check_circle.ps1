# Q122 서클 보상 · 승급 자격 2인 확인(헤드리스): 리슨 서버 + 클라이언트를 -game -nullrhi 로 띄우고
# 호스트 · 클라이언트가 각자 ① 1서클 덤프(열린 슬롯 세트 1) → ② 2서클 스킬북(이그니스)을 가방에서 읽음 → 봉인된 책 →
# ③ 숙련도 없이 승급 요청 → 서버가 거절하고 까닭(PromotionGrade)을 소유 클라이언트에 알림 → ④ 불씨 · 물줄기 숙련도를 채움 →
# ⑤ 승급 → 서클 2, 봉인 풀려 이그니스 배움, 열린 슬롯 세트 2 → ⑥ 불씨(1서클) 시전 → 서버가 격 차이 위력 · 마나 보정을 붙임.
#
#   powershell -File Scripts\Magic\net_check_circle.ps1      # 결과: Saved\NetCheckCircle\server.log · client.log 요약, NETCHECK_RESULT fail=0
#
# 시간표(초)는 두 플레이어가 모인 때(Grim.Base.AfterJoin 2 …)부터. 시작 마법(-GrimStartingSpells): 슬롯1 불씨 · 슬롯2 물줄기 · 슬롯3 미풍(모두 1서클).
param(
	[int]$Port = 17797,
	[int]$RunSeconds = 40,
	[string]$Map = "/Game/Grimoire/Maps/L_M1_Sandbox"
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\NetCheckCircle"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "server.log"
$ClientLog = Join-Path $OutDir "client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue

$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

function Join-Schedule([string[]]$Items) {
	($Items | ForEach-Object { $Parts = $_ -split ' ', 2; "Grim.Base.AfterJoin 2 $($Parts[0]) $($Parts[1])" }) -join ","
}

$Steps = @(
	"2 Grim.Magic.Proficiency",
	"3 Grim.Items.Give Skillbook_Fireball 1",
	"3.5 Grim.Items.Eat Skillbook_Fireball",
	"5 Grim.Magic.CircleUp 0",
	"6 Grim.Magic.Proficiency",
	"7 Grim.Magic.AddXP Ember 100000",
	"7.2 Grim.Magic.AddXP WaterJet 100000",
	"9 Grim.Magic.CircleUp 0",
	"11 Grim.Magic.Proficiency",
	"12 Grim.Input.Press Slot1 None",
	"16 Grim.Magic.Proficiency"
)
# 클라이언트는 0.5초 늦게(서버 로그에서 두 사람 몫이 섞이지 않게)
$ServerCmds = Join-Schedule $Steps
$ClientCmds = Join-Schedule ($Steps | ForEach-Object { $Parts = $_ -split ' ', 2; "$([double]$Parts[0] + 0.5) $($Parts[1])" })

$Server = Start-Process -FilePath $Exe -PassThru -ArgumentList (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$ServerCmds`"") + $Common)
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
	Select-String -Path $Log -Encoding UTF8 -Pattern "Proficiency .*서클=|봉인|승급 실패|서클 \d → \d|마법 실패|격 차이|가방 알림|Welcomed by server|Error: " |
		ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

$Fail = 0
$ServerText = Get-Content -Encoding UTF8 -Raw $ServerLog
$ClientText = Get-Content -Encoding UTF8 -Raw $ClientLog
function Expect-Count([string]$Name, [string]$Text, [string]$Pattern, [int]$Min) {
	$Count = ([regex]::Matches($Text, $Pattern)).Count
	if ($Count -lt $Min) { $script:Fail++; "없음: $Name — 「$Pattern」 $Count 번(기대 $Min 이상)" } else { "맞음: $Name ($Count)" }
}

# ① 1서클: 열린 슬롯 세트 1(서버 = 호스트 몫, 클라이언트 = 자기 몫 — 복제된 서클로 같은 값)
Expect-Count "호스트 1서클 슬롯 세트 1" $ServerText "Proficiency \S+ 서클=1 .*슬롯세트=1 봉인=0" 1
Expect-Count "클라이언트 1서클 슬롯 세트 1" $ClientText "Proficiency \S+ 서클=1 .*슬롯세트=1 봉인=0" 1
# ② 봉인된 책(서버 판정 → 소유 클라이언트 가방 알림)
Expect-Count "서버: 봉인된 책 기록(두 사람)" $ServerText "\[Server\] \S+ 봉인된 책 기록: DA_Spell_Fireball \(2서클\)" 2
Expect-Count "호스트 가방 알림 봉인" $ServerText "\[Server\] 가방 알림 Sealed Skillbook_Fireball 필요 서클 2" 1
Expect-Count "클라이언트 가방 알림 봉인" $ClientText "\[Client\] 가방 알림 Sealed Skillbook_Fireball 필요 서클 2" 1
Expect-Count "클라이언트: 봉인 목록 복제(봉인=1)" $ClientText "Proficiency \S+ 서클=1 .*봉인=1" 1
# ③ 자격 없는 승급 → 서버가 거절 · 까닭
Expect-Count "서버: 승급 실패(두 사람)" $ServerText "\[Server\] \S+ 승급 실패: 서클 1, 다음 2, C이상 0/2" 2
Expect-Count "서버 판정 PromotionGrade(두 사람)" $ServerText "\[Server\] 마법 실패 \S+ PromotionGrade " 2
Expect-Count "호스트 알림 PromotionGrade" $ServerText "\[Server\] 마법 실패 알림 PromotionGrade" 1
Expect-Count "클라이언트 알림 PromotionGrade" $ClientText "\[Client\] 마법 실패 알림 PromotionGrade" 1
# ⑤ 자격을 채운 승급 → 봉인 풀림 · 슬롯 세트 2
Expect-Count "서버: 서클 1 → 2(두 사람)" $ServerText "\[Server\] \S+ 서클 1 → 2" 2
Expect-Count "서버: 봉인 풀려 이그니스 배움(두 사람)" $ServerText "봉인 풀림\(2서클\): DA_Spell_Fireball → Learned" 2
Expect-Count "호스트 2서클 슬롯 세트 2 · 봉인 0" $ServerText "Proficiency \S+ 서클=2 .*슬롯세트=2 봉인=0" 1
Expect-Count "클라이언트 2서클 슬롯 세트 2 · 봉인 0" $ClientText "Proficiency \S+ 서클=2 .*슬롯세트=2 봉인=0" 1
# ⑥ 격 차이 — 서버가 피해 Spec · 마나 비용에 붙임(1서클 마법 · 2서클 시전자 = ×1.03 · ×0.97)
Expect-Count "서버: 격 차이 위력 ×1.03(두 사람)" $ServerText "\[Server\] 서클 격 차이 위력 \S+: DA_Spell_Ember 피해 [\d.]+ \(격 차이 ×1\.03, 서클 2 · 마법 1서클\)" 2
Expect-Count "서버: 격 차이 마나 ×0.97(두 사람)" $ServerText "\[Server\] 서클 격 차이 마나 \S+: DA_Spell_Ember [\d.]+ \(격 차이 ×0\.97, 서클 2 · 마법 1서클\)" 2
if ($ClientText -match "\[Server\] 서클 격 차이") { $Fail++; "클라이언트 로그에 서버 판정 줄이 있다 — 클라이언트가 보정을 정했나" }
"NETCHECK_RESULT fail=$Fail"
