# 치트 2인 확인(헤드리스 리슨 서버 + 클라이언트, L_OpenBeta) — docs/qa/cheats.md 의 QA-CHT-006 · 013.
# ① 호스트가 Grim.Go 를 치면 모든 플레이어가 함께 옮겨진다. ② 클라이언트가 자기 콘솔에 GrimSuperCheat 1 을 치면 서버에서 그 사람에게만 걸린다.
#
#   powershell -File Scripts\QA\net_check_cheats.ps1      # 결과: Saved\QA\netcheats_server.log · netcheats_client.log 요약, 끝에 PASS / FAIL
#
# 시간표(초)는 두 플레이어가 모인 때(Grim.Taming.AfterJoin 2 …)부터:
#   3 서버 덤프 ①(둘 다 탑 곁) → 4 호스트 Grim.Go Jackalope → 10 서버 덤프 ②(둘 다 무리 곁 · 서로 가깝다)
#   클라이언트 11 GrimSuperCheat 1 → 13 클라이언트 쓰러짐(grim.Survival.Down) · 13 호스트 쓰러짐 → 16 서버 덤프 ③(클라이언트 HP 최대 그대로 · 호스트 HP 0)
# 덤프 줄 = [SurvivalDump Server] <이름> … HP= … pos=… — 플레이어 배열 차례(먼저 = 호스트).
param(
	[int]$Port = 18790,
	[int]$RunSeconds = 25
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$ServerLog = Join-Path $OutDir "netcheats_server.log"
$ClientLog = Join-Path $OutDir "netcheats_client.log"
Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Map = "/Game/Grimoire/Maps/L_OpenBeta"

$Common = @("-game", "-GrimStartingSpells", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-windowed", "-ResX=320", "-ResY=240")
$Common += "-ini:Engine:[/Script/OnlineSubsystemUtils.IpNetDriver]:InitialConnectTimeout=240,[/Script/OnlineSubsystemUtils.IpNetDriver]:ConnectionTimeout=240"

function Join-Schedule([string[]]$Items) {
	($Items | ForEach-Object { $Parts = $_ -split ' ', 2; "Grim.Taming.AfterJoin 2 $($Parts[0]) $($Parts[1])" }) -join ","
}
$ServerCmds = Join-Schedule @(
	"3 grim.Survival.Dump",
	"4 Grim.Go Jackalope",
	"10 grim.Survival.Dump",
	"13 grim.Survival.Down",
	"16 grim.Survival.Dump"
)
# 치트(CheatManager exec)는 플레이어 컨트롤러를 거쳐야 닿는다 — AfterJoin 은 엔진 Exec 로 돌리므로 Grim.At 0(컨트롤러 ConsoleCommand)으로 감싼다
$ClientCmds = Join-Schedule @(
	"11 Grim.At 0 GrimSuperCheat 1",
	"13 grim.Survival.Down",
	"16 grim.Survival.Dump"
)

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
	Select-String -Path $Log -Encoding UTF8 -Pattern "SurvivalDump|\[Grim\.Go\]|슈퍼치트|GrimSuperCheat|Welcomed by server|LogNet: Join" | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
}

# pos=V(X=…, Y=…, Z=…) — ToCompactString 은 0 인 칸을 뺀다
function AxisOf([string]$Pos, [string]$Axis) { if ($Pos -match "$Axis=([-\d.]+)") { [double]$Matches[1] } else { 0.0 } }
$Rows = @(Select-String -Path $ServerLog -Encoding UTF8 -Pattern '\[SurvivalDump Server\] (\S+) .*HP=([\d.]+) .*pos=V\(([^)]*)\)' | ForEach-Object {
	$Pos = $_.Matches[0].Groups[3].Value
	[PSCustomObject]@{ Name = $_.Matches[0].Groups[1].Value; HP = [double]$_.Matches[0].Groups[2].Value; X = (AxisOf $Pos "X"); Y = (AxisOf $Pos "Y") }
})
$Failed = 0
function Check([string]$Name, [bool]$Ok) {
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
	if (-not $Ok) { $script:Failed++ }
}
function Dist($A, $B) { [Math]::Sqrt(($A.X - $B.X) * ($A.X - $B.X) + ($A.Y - $B.Y) * ($A.Y - $B.Y)) / 100.0 }
"===== 판정 (서버 덤프 줄 $($Rows.Count) — 3번 × 2명이어야 한다)"
Check "서버 덤프 3번 × 2명" ($Rows.Count -eq 6)
if ($Rows.Count -ge 6) {
	$H1, $C1, $H2, $C2, $H3, $C3 = $Rows[0..5]
	"호스트 $($H1.Name): ① ($($H1.X), $($H1.Y)) → ② ($($H2.X), $($H2.Y)), 클라이언트 $($C1.Name): ① ($($C1.X), $($C1.Y)) → ② ($($C2.X), $($C2.Y))"
	Check "호스트가 무리 곁으로 옮겨졌다(50 m 넘게)" ((Dist $H1 $H2) -gt 50)
	Check "클라이언트도 함께 옮겨졌다(50 m 넘게)" ((Dist $C1 $C2) -gt 50)
	Check "옮긴 뒤 둘이 서로 가깝다(15 m 안)" ((Dist $H2 $C2) -lt 15)
	Check "슈퍼치트를 켠 클라이언트는 쓰러짐 명령 뒤에도 HP 그대로(HP $($C1.HP) → $($C3.HP))" ($C3.HP -gt 1 -and $C3.HP -ge $C1.HP - 0.5)
	Check "치트를 안 켠 호스트는 쓰러졌다(HP $($H3.HP))" ($H3.HP -lt 1)
}
Check "서버 로그에 슈퍼치트 켬 한 번" (@(Select-String -Path $ServerLog -Encoding UTF8 -Pattern '슈퍼치트 켬').Count -eq 1)
# 결과 메시지(ClientMessage 「GrimSuperCheat ON」)는 화면에만 뜨고 로그에 안 남는다 — 클라이언트가 자기 콘솔에서 쳤는지만 본다
Check "클라이언트가 자기 콘솔에서 GrimSuperCheat 1 을 쳤다" ([bool](Select-String -Path $ClientLog -Encoding UTF8 -Pattern '\[Grim\.At\] GrimSuperCheat 1' -Quiet))
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
