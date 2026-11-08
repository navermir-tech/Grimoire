# QA 일괄 실행 — docs/dev/qa.md 「검증 수단」 ① 자동화 테스트 · ② 헤드리스 시나리오 · ④ 2인 확인.
# 자동화 테스트와 이미 있는 점검 스크립트를 차례로 돌려 Saved\QA\<이름>.txt 에 남기고, 판정 줄을 Saved\QA\summary.txt 로 모은다.
#
#   powershell -File Scripts\QA\run_all.ps1                          # 전부(약 35분)
#   powershell -File Scripts\QA\run_all.ps1 -Only automation,tutorial # 고른 것만
#
# 한 번에 하나씩 돈다 — 헤드리스 에디터 여럿을 같이 띄우면 접속 · 맵 로드가 늦어 시간표가 어긋난다.
# 포트는 PortBase 부터 하나씩(다른 세션의 점검 스크립트 기본 포트 17777~17799 와 겹치지 않게).
# 판정: PASS / FAIL / 「검토」(스크립트가 판정 줄을 내지 않음 — 요약을 사람이 · Claude 가 docs/qa/ 의 기대 결과와 맞춰 본다).
param(
	[string[]]$Only = @(),
	[int]$PortBase = 18700
)

# powershell -File 로 부르면 -Only a,b 가 문자열 하나로 온다 — 쉼표로 나눈다
$Only = @($Only | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\QA"
New-Item -ItemType Directory -Force $OutDir | Out-Null
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 이름, 스크립트(Scripts\ 기준), 인자(포트는 {P} 자리에), 판정 정규식(PASS 줄 · FAIL 줄). 판정식이 비면 「검토」.
$Steps = @(
	@("automation", "", "", "", ""),
	@("net_check", "Integration\net_check.ps1", "-Port {P}", "", ""),
	@("delivery", "Integration\net_check_delivery.ps1", "-Port {P}", "", ""),
	@("controls", "Integration\net_check_controls.ps1", "-Port {P}", "", ""),
	@("growth", "Integration\check_growth.ps1", "", "", ""),
	@("weather", "Environment\net_check_weather.ps1", "-Port {P}", "", ""),
	@("survival", "Survival\net_check_survival.ps1", "-Port {P}", "", ""),
	@("tutorial", "Tutorial\check_tutorial.ps1", "", "^PASS", "^FAIL"),
	@("save", "Save\check_save.ps1", "-Port {P}", "FAIL\s*0|^PASS", "^FAIL|FAIL [1-9]"),
	@("join", "Online\check_join.ps1", "-Port {P} -BadPort 18799", "FAIL\s*0|^PASS", "^FAIL|FAIL [1-9]"),
	@("items", "Items\net_check_items.ps1", "-Port {P}", "", ""),
	@("bagdouble", "Items\net_check_bag_doubleclick.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("creatures", "Creatures\net_check_creatures.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("taming", "Creatures\net_check_taming.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("storage", "Creatures\net_check_storage.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("castfail", "Magic\net_check_castfail.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("circle", "Magic\net_check_circle.ps1", "-Port {P}", "NETCHECK_RESULT fail=0", "NETCHECK_RESULT fail=[1-9]"),
	@("appearance", "Character\net_check_appearance.ps1", "-Port {P}", "^PASS", "^FAIL"),
	@("openbeta", "Terrain\check_openbeta.ps1", "", "^PASS", "^FAIL"),
	@("cheats", "QA\check_cheats.ps1", "", "^PASS", "^FAIL"),
	@("netcheats", "QA\net_check_cheats.ps1", "-Port {P}", "^PASS", "^FAIL")
)

$Summary = @()
$Port = $PortBase
foreach ($Step in $Steps) {
	$Name = $Step[0]
	if ($Only.Count -gt 0 -and $Only -notcontains $Name) { continue }
	$Out = Join-Path $OutDir "$Name.txt"
	$Started = Get-Date
	Write-Output "▶ $Name ($($Started.ToString('HH:mm:ss')))"
	if ($Name -eq "automation") {
		$Log = Join-Path $OutDir "automation.log"
		Remove-Item $Log -ErrorAction SilentlyContinue
		$P = Start-Process -FilePath $Exe -PassThru -Wait -ArgumentList @("`"$Project`"", "-ExecCmds=`"Automation RunTests Grimoire; Quit`"",
			"-unattended", "-nopause", "-nosplash", "-nop4", "-NullRHI", "-log", "-abslog=`"$Log`"")
		$Lines = Select-String -Path $Log -Pattern "Test Completed\. Result=\{(\w+)\}.*Path=\{([^}]+)\}" -AllMatches
		$Passed = @($Lines | Where-Object { $_.Matches[0].Groups[1].Value -match "Success|Passed" }).Count
		$Failed = @($Lines | Where-Object { $_.Matches[0].Groups[1].Value -notmatch "Success|Passed" })
		$Text = @("통과 $Passed · 실패 $($Failed.Count)") + ($Failed | ForEach-Object { "FAILED " + $_.Matches[0].Groups[2].Value })
		$Text | Out-File -Encoding utf8 $Out
		$Verdict = if ($Lines.Count -eq 0) { "FAIL(결과 없음)" } elseif ($Failed.Count -eq 0) { "PASS($Passed)" } else { "FAIL($($Failed.Count)/$($Lines.Count))" }
	} else {
		$Script = Join-Path $Root "Scripts\$($Step[1])"
		$ArgText = $Step[2].Replace("{P}", "$Port")
		$Port++
		$Cmd = "& '$Script' $ArgText"
		powershell -NoProfile -ExecutionPolicy Bypass -Command $Cmd *>&1 | Out-File -Encoding utf8 $Out
		$Body = Get-Content $Out -Raw -Encoding utf8
		$Verdict = if (-not $Step[3]) { "검토" }
			elseif ($Step[4] -and $Body -match "(?m)$($Step[4])") { "FAIL" }
			elseif ($Body -match "(?m)$($Step[3])") { "PASS" }
			else { "FAIL(판정 줄 없음)" }
	}
	$Took = [int]((Get-Date) - $Started).TotalSeconds
	$Line = "{0,-11} {1,-16} {2,4}s  Saved\QA\{0}.txt" -f $Name, $Verdict, $Took
	Write-Output $Line
	$Summary += $Line
}
$Summary | Out-File -Encoding utf8 (Join-Path $OutDir "summary.txt")
"===== 요약"
$Summary
