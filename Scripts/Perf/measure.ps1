# 성능 측정(docs/dev/perf-measure.md) — 같은 자리 · 같은 조건에서 되풀이해 재고 표로 남긴다.
#
#   powershell -File Scripts\Perf\measure.ps1 -Label before        # 렌더 + 2인 네트워크 둘 다
#   powershell -File Scripts\Perf\measure.ps1 -Label after -Only Render
#   powershell -File Scripts\Perf\summarize.ps1 -Dir Saved\Perf\after -Compare Saved\Perf\before
#
# 1) 렌더: RHI 를 켠 단독 게임(-game -RenderOffscreen, 1920×1080)으로 L_OpenBeta 를 열고 정해 둔 자리마다
#    Grim.Perf.Goto(폰을 옮겨 실제 플레이 카메라) → 안정 대기 → CsvProfile Start · Grim.Perf.Sample(프레임 · 스레드 · GPU) → CsvProfile Stop.
#    끝에 memreport -full.
# 2) 네트워크: 리슨 서버 + 클라이언트(-nullrhi)로 L_OpenBeta 에 둘이 들어가 가만히 · 생물 무리 옆 · 비 오는 동안
#    Grim.Perf.Net(연결당 kB/s · 패킷/s · 복제 객체 수) · Grim.Perf.Sample(서버 월드 틱).
# 결과: Saved\Perf\<Label>\ 의 로그 · CSV · memreport 와 summary.md(요약 표).
param(
	[string]$Label = (Get-Date -Format "yyyyMMdd-HHmm"),
	[ValidateSet("All", "Render", "Net")]
	[string]$Only = "All",
	[int]$ResX = 1920,
	[int]$ResY = 1080,
	# 자리마다: 옮긴 뒤 안정(스트리밍 · 셰이더 · 폴리지) 대기 → 표본 초
	[int]$Settle = 12,
	[int]$SampleSeconds = 10,
	[int]$StartDelay = 30,
	[int]$Port = 17790,
	# 렌더 실행 맨 앞에 더할 콘솔 명령(쉼표로) — 같은 바이너리로 전후 비교할 때.
	# 예: -ExtraCmds "Grim.Sky.SunRotationStep 0,Grim.DevLabels.MaxDistance 10000000"(2026-11-06 최적화 전 동작)
	[string]$ExtraCmds = ""
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\Perf\$Label"
$CsvDir = Join-Path $Root "Saved\Profiling\CSV"
$MemDir = Join-Path $Root "Saved\Profiling\MemReports"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Map = "/Game/Grimoire/Maps/L_OpenBeta"

# 측정 자리(월드 cm — Scripts/Terrain/shoot_openbeta.ps1 과 같은 좌표). 이름, X, Y, Yaw, 앞에 할 명령(쉼표 없이 ; 로)
$Views = @(
	@("door", 4000, 0, 0, ""),                 # 탑 문 앞에서 북쪽 — 앞마당 · 그레이박스 밀집 · 글씨
	@("path200", 23246, 2090, -26, ""),        # 큰길 200 m — 연못 갈림길 앞
	@("hilltop", 56800, -11000, 140, ""),      # 봉화 언덕 꼭대기에서 남동 — 가장 멀리 보이는 자리
	@("pond", 12500, -30000, -5, ""),          # 연못 남쪽 물가에서 북 — 물 · 반사
	@("valley", 232000, 6000, 57, ""),         # 계곡 입구(2.3 km 북) — 스트리밍 · 폴리지
	@("creatures", 0, 0, 0, "Grim.Go Jackalope;Grim.Spawn Jackalope 12 15"), # 재칼로프 무리 옆 + 12마리 더
	@("rain", 23246, 2090, -26, "Grim.Weather.Set All Rain 0")              # 큰길 200 m 에서 비(블렌드 없이)
)

# 같은 PC 에서 다른 언리얼 프로세스(다른 세션의 에디터 · 점검)가 돌면 GPU · CPU 를 나눠 써서 숫자가 흔들린다
$Others = Get-Process UnrealEditor, UnrealEditor-Cmd -ErrorAction SilentlyContinue
if ($Others) { Write-Output "[경고] 다른 언리얼 프로세스 $($Others.Count)개가 돌고 있다 — 숫자가 흔들린다(전후 비교는 같은 조건에서)" }

function Start-Grim([string[]]$ArgList) {
	Start-Process -FilePath $Exe -PassThru -ArgumentList $ArgList -WindowStyle Hidden
}

function Wait-Grim($Proc, [int]$Seconds, [string]$LogFile, [string]$DoneMarker) {
	$Deadline = (Get-Date).AddSeconds($Seconds)
	while (-not $Proc.HasExited -and (Get-Date) -lt $Deadline) {
		if ($DoneMarker -and (Test-Path $LogFile) -and (Select-String -Path $LogFile -Pattern $DoneMarker -SimpleMatch -Quiet)) { Start-Sleep -Seconds 5; break }
		Start-Sleep -Seconds 2
	}
	if (-not $Proc.HasExited) { Stop-Process -Id $Proc.Id -Force }
}

if ($Only -ne "Net") {
	Remove-Item (Join-Path $OutDir "render*.log"), (Join-Path $OutDir "csv-*.csv"), (Join-Path $OutDir "memreport.txt") -ErrorAction SilentlyContinue
	# 실행이 중간에 끊기면(다른 프로세스가 끄거나 멈춤) 빠진 자리만 다시 잰다 — 최대 2번 더
	$Pending = New-Object System.Collections.ArrayList
	foreach ($V in $Views) { [void]$Pending.Add($V) }
	for ($Try = 0; $Try -lt 3 -and $Pending.Count -gt 0; $Try++) {
		$Log = Join-Path $OutDir $(if ($Try -eq 0) { "render.log" } else { "render-retry$Try.log" })
		$Cmds = @("t.MaxFPS 0", "r.VSync 0", "r.GPUCsvStatsEnabled 1", "GrimSuperCheat 1", "Grim.Creature.Time 0.45 5")
		if ($ExtraCmds) { $Cmds += $ExtraCmds.Split(",") }
		# 예열: 비 이펙트를 한 번 켰다 꺼서 셰이더 컴파일 멈춤이 「rain」 자리에 섞이지 않게
		$Cmds += @("Grim.At 6 Grim.Weather.Set All Rain 0", "Grim.At $($StartDelay - 6) Grim.Weather.Set All Clear 0")
		$T = $StartDelay
		foreach ($V in $Pending) {
			$Name = $V[0]
			if ($V[4]) { foreach ($Pre in ($V[4] -split ";")) { $Cmds += "Grim.At $T $Pre" } }
			if ($Name -ne "creatures") { $Cmds += "Grim.Perf.Goto $($V[1]) $($V[2]) $($V[3]) -5 $($T + 1)" }
			# 생물 자리는 먼 곳으로 순간이동 + 소환이라 스트리밍 · 펼침이 더 걸린다
			$S = $T + 1 + $(if ($Name -eq "creatures") { $Settle + 10 } else { $Settle })
			$Cmds += "Grim.At $S CsvProfile Start"
			$Cmds += "Grim.Perf.Sample $Name $SampleSeconds $S"
			$Cmds += "Grim.At $($S + $SampleSeconds + 0.5) CsvProfile Stop"
			$T = $S + $SampleSeconds + 2
		}
		$Cmds += "Grim.At $T memreport -full"
		$Cmds += "Grim.At $($T + 8) Grim.Perf.Sample done 0.5"
		$CmdLine = $Cmds -join ","
		$StartTime = Get-Date
		Write-Output "[렌더$(if ($Try) { " 다시 $Try" })] $($Pending.Count) 자리, 약 $($T + 20) 초"
		$Game = Start-Grim @("`"$Project`"", $Map, "-game", "-RenderOffscreen", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-forcelogflush",
			"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-abslog=`"$Log`"", "-ExecCmds=`"$CmdLine`"")
		# 시작(셰이더 · 에셋 레지스트리)이 몇 분 걸릴 수 있다 — 끝 표시를 기다리고, 시간 제한은 넉넉히
		Wait-Grim $Game ($T + 480) $Log "GrimPerf|name=done"

		# 이번 실행의 CSV(Stop 할 때 써진다 — 시각 순 = 끝난 자리 순) · memreport 를 결과 폴더로
		$Done = @(Select-String -Path $Log -Pattern "GrimPerf|name=" -SimpleMatch -ErrorAction SilentlyContinue | ForEach-Object { ($_.Line -split "name=")[1].Split("|")[0] })
		$Csvs = @(Get-ChildItem $CsvDir -Filter *.csv -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $StartTime } | Sort-Object LastWriteTime)
		$I = 0
		$Next = New-Object System.Collections.ArrayList
		foreach ($V in $Pending) {
			if ($Done -contains $V[0]) {
				if ($I -lt $Csvs.Count) { Copy-Item $Csvs[$I].FullName (Join-Path $OutDir "csv-$($V[0]).csv") }
				$I++
			} else { [void]$Next.Add($V) }
		}
		$Mem = Get-ChildItem $MemDir -Recurse -Filter *.memreport -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $StartTime } | Sort-Object LastWriteTime | Select-Object -Last 1
		if ($Mem) { Copy-Item $Mem.FullName (Join-Path $OutDir "memreport.txt") }
		if ($Next.Count -gt 0) { Write-Output "[렌더] 끊김 — 빠진 자리: $(($Next | ForEach-Object { $_[0] }) -join ', ')" }
		$Pending = $Next
	}
}

if ($Only -ne "Render") {
	$ServerLog = Join-Path $OutDir "net-server.log"
	$ClientLog = Join-Path $OutDir "net-client.log"
	Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue
	# -forcelogflush: 끝 표시를 보고 끄므로 로그가 버퍼에 남으면 안 된다
	$Common = @("-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log", "-forcelogflush", "-windowed", "-ResX=320", "-ResY=240")
	# 서버 시각(초). 클라이언트는 15초 뒤에 띄우므로 클라이언트 지연 = 서버 시각 − 15.
	$ClientStart = 15
	$Phases = @(@("net-idle", 60, ""), @("net-creatures", 95, "Grim.Go Jackalope;Grim.Spawn Jackalope 12 15"), @("net-rain", 125, "Grim.Weather.Set All Rain 0"))
	# 60 fps 로 묶는다 — 헤드리스는 제한이 없으면 1,000 fps 로 돌아 이동 RPC 가 실제보다 훨씬 많이 나간다
	$ServerCmds = @("t.MaxFPS 60", "GrimSuperCheat 1", "Grim.Creature.Time 0.45 5")
	$ClientCmds = @("t.MaxFPS 60")
	foreach ($P in $Phases) {
		if ($P[2]) { foreach ($Pre in ($P[2] -split ";")) { $ServerCmds += "Grim.At $($P[1] - 10) $Pre" } }
		$ServerCmds += "Grim.Perf.Net $($P[0]) 20 $($P[1])"
		$ServerCmds += "Grim.Perf.Sample server-$($P[0]) 20 $($P[1])"
		$ClientCmds += "Grim.Perf.Net $($P[0]) 20 $($P[1] - $ClientStart)"
	}
	$End = $Phases[-1][1] + 25
	$ServerCmds += "Grim.Perf.Sample done 0.5 $End"
	# 끊기면(마지막 구간 줄이 없으면) 처음부터 다시 — 최대 2번 더
	for ($Try = 0; $Try -lt 3; $Try++) {
		Remove-Item $ServerLog, $ClientLog -ErrorAction SilentlyContinue
		Write-Output "[네트워크$(if ($Try) { " 다시 $Try" })] 리슨 서버 + 클라이언트 1, 약 $($End + 10) 초"
		$Server = Start-Grim (@("`"$Project`"", "$Map`?listen", "-port=$Port", "-abslog=`"$ServerLog`"", "-ExecCmds=`"$($ServerCmds -join ",")`"") + $Common)
		Start-Sleep -Seconds $ClientStart
		$Client = Start-Grim (@("`"$Project`"", "127.0.0.1:$Port", "-abslog=`"$ClientLog`"", "-ExecCmds=`"$($ClientCmds -join ",")`"") + $Common)
		Wait-Grim $Server ($End + 300) $ServerLog "GrimPerf|name=done"
		if (-not $Client.HasExited) { Stop-Process -Id $Client.Id -Force }
		if (Select-String -Path $ServerLog -Pattern "GrimPerfNet|name=$($Phases[-1][0])" -SimpleMatch -Quiet -ErrorAction SilentlyContinue) { break }
		Write-Output "[네트워크] 끊김"
	}
}

& (Join-Path $PSScriptRoot "summarize.ps1") -Dir $OutDir
