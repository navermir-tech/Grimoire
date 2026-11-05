# measure.ps1 결과 폴더를 마크다운 표(summary.md)로 요약한다. -Compare 를 주면 「전 → 후」 를 나란히.
#
#   powershell -File Scripts\Perf\summarize.ps1 -Dir Saved\Perf\after -Compare Saved\Perf\before
param(
	[Parameter(Mandatory = $true)][string]$Dir,
	[string]$Compare = "",
	# CSV 에서 자리마다 보여 줄 GPU 패스 수
	[int]$TopGpu = 8
)

function Read-PerfLines([string]$File, [string]$Prefix) {
	$Rows = @()
	if (-not (Test-Path $File)) { return $Rows }
	foreach ($M in (Select-String -Path $File -Pattern "$Prefix|" -SimpleMatch)) {
		$Text = $M.Line.Substring($M.Line.IndexOf("$Prefix|") + $Prefix.Length + 1)
		$Row = [ordered]@{}
		foreach ($Pair in $Text.Split("|")) {
			$Kv = $Pair.Split("=", 2)
			if ($Kv.Count -eq 2) { $Row[$Kv[0]] = $Kv[1].Trim() }
		}
		if ($Row["name"] -and $Row["name"] -ne "done") { $Rows += [pscustomobject]$Row }
	}
	return $Rows
}

function Read-CsvGpu([string]$File) {
	# CSV 프로파일러: 첫 줄 = 열 이름(첫 열은 이벤트 — 비어 있다), 끝의 메타데이터 줄 앞까지가 프레임
	$Lines = Get-Content $File
	if ($Lines.Count -lt 3) { return @{} }
	$Header = $Lines[0].Split(",")
	$Sums = @{}
	$N = 0
	for ($I = 1; $I -lt $Lines.Count; $I++) {
		$Cells = $Lines[$I].Split(",")
		if ($Cells.Count -ne $Header.Count) { break }
		$Value = 0.0
		if (-not [double]::TryParse($Cells[1], [ref]$Value)) { break }
		$N++
		for ($C = 0; $C -lt $Header.Count; $C++) {
			if ($Header[$C].StartsWith("GPU/") -and [double]::TryParse($Cells[$C], [ref]$Value)) { $Sums[$Header[$C]] = $Sums[$Header[$C]] + $Value }
		}
	}
	$Avg = @{}
	foreach ($K in $Sums.Keys) { $Avg[$K] = $Sums[$K] / [math]::Max(1, $N) }
	return $Avg
}

function Cell($Now, $Was, [string]$Key, [string]$Format = "{0:N1}") {
	$A = 0.0
	if (-not $Now -or -not [double]::TryParse([string]$Now.$Key, [ref]$A)) { return "–" }
	$Text = $Format -f $A
	$B = 0.0
	if ($Was -and [double]::TryParse([string]$Was.$Key, [ref]$B)) { $Text = ($Format -f $B) + " → " + $Text }
	return $Text
}

$Dir = (Resolve-Path $Dir).Path
$Out = @("# 성능 측정 $(Split-Path -Leaf $Dir)", "")
$Out += "측정: $(Get-Date -Format 'yyyy-MM-dd HH:mm') · $((Get-CimInstance Win32_VideoController | Where-Object { $_.AdapterRAM -gt 1GB } | Select-Object -First 1).Name) · $((Get-CimInstance Win32_Processor).Name.Trim())"
if ($Compare) { $Out += "비교: $(Split-Path -Leaf $Compare) → $(Split-Path -Leaf $Dir)" }
$Out += ""

# 끊겨서 다시 잰 로그(render-retry*.log)까지
function Read-Render([string]$D) { $R = @(); foreach ($F in (Get-ChildItem $D -Filter "render*.log" -ErrorAction SilentlyContinue | Sort-Object Name)) { $R += Read-PerfLines $F.FullName "GrimPerf" }; return $R }
$Render = Read-Render $Dir
$Before = if ($Compare) { Read-Render $Compare } else { @() }
if ($Render.Count -gt 0) {
	$Out += "## 렌더 (1080p, 에디터 바이너리 -game — 출시 빌드보다 느리다)", ""
	$Out += "| 자리 | fps | 프레임 ms | p95 ms | 게임 ms | 렌더 ms | RHI ms | GPU ms | 드로 콜 | 프리미티브 | 메모리 MB | 액터 | 생물 | 글씨 |"
	$Out += "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"
	foreach ($R in $Render) {
		$W = $Before | Where-Object { $_.name -eq $R.name } | Select-Object -First 1
		$Out += "| $($R.name) | $(Cell $R $W fps) | $(Cell $R $W frame_ms '{0:N2}') | $(Cell $R $W p95_ms) | $(Cell $R $W game_ms '{0:N2}') | $(Cell $R $W render_ms '{0:N2}') | $(Cell $R $W rhi_ms '{0:N2}') | $(Cell $R $W gpu_ms '{0:N2}') | $(Cell $R $W draws '{0:N0}') | $(Cell $R $W prims '{0:N0}') | $(Cell $R $W mem_mb '{0:N0}') | $(Cell $R $W actors '{0:N0}') | $($R.creatures) | $($R.labels) |"
	}
	$Out += ""
}

$CsvFiles = Get-ChildItem $Dir -Filter "csv-*.csv" -ErrorAction SilentlyContinue
if ($CsvFiles) {
	$Out += "## GPU 패스 (CSV 프로파일러, 평균 ms — 자리마다 큰 것 $TopGpu 개)", ""
	foreach ($F in $CsvFiles) {
		$Gpu = Read-CsvGpu $F.FullName
		if ($Gpu.Count -eq 0) { continue }
		$Top = $Gpu.GetEnumerator() | Where-Object { $_.Key -ne "GPU/Total" } | Sort-Object Value -Descending | Select-Object -First $TopGpu
		$Total = if ($Gpu.ContainsKey("GPU/Total")) { " (합 {0:N2})" -f $Gpu["GPU/Total"] } else { "" }
		$Out += "- **$($F.BaseName.Substring(4))**$Total — " + (($Top | ForEach-Object { "{0} {1:N2}" -f $_.Key.Substring(4), $_.Value }) -join " · ")
	}
	$Out += ""
}

$NetRows = @(Read-PerfLines (Join-Path $Dir "net-server.log") "GrimPerfNet") + @(Read-PerfLines (Join-Path $Dir "net-client.log") "GrimPerfNet")
$NetBefore = if ($Compare) { @(Read-PerfLines (Join-Path $Compare "net-server.log") "GrimPerfNet") + @(Read-PerfLines (Join-Path $Compare "net-client.log") "GrimPerfNet") } else { @() }
if ($NetRows.Count -gt 0) {
	$Out += "## 네트워크 (리슨 서버 + 클라이언트 1, 연결당)", ""
	$Out += "| 구간 | 쪽 | 받음 kB/s | 보냄 kB/s | 받은 패킷/s | 보낸 패킷/s | 복제 객체 | 활성 |"
	$Out += "|---|---|---|---|---|---|---|---|"
	foreach ($R in $NetRows) {
		$W = $NetBefore | Where-Object { $_.name -eq $R.name -and $_.role -eq $R.role } | Select-Object -First 1
		$Out += "| $($R.name) | $($R.role) | $(Cell $R $W in_kBps '{0:N2}') | $(Cell $R $W out_kBps '{0:N2}') | $(Cell $R $W in_pps) | $(Cell $R $W out_pps) | $(Cell $R $W net_objects '{0:N0}') | $(Cell $R $W net_active '{0:N0}') |"
	}
	$Out += ""
	$Server = Read-PerfLines (Join-Path $Dir "net-server.log") "GrimPerf"
	$ServerBefore = if ($Compare) { Read-PerfLines (Join-Path $Compare "net-server.log") "GrimPerf" } else { @() }
	if ($Server.Count -gt 0) {
		$Out += "| 서버 구간 | 프레임 ms | 월드 틱 ms | 게임 ms | 메모리 MB | 액터 | 생물 |"
		$Out += "|---|---|---|---|---|---|---|"
		foreach ($R in $Server) {
			$W = $ServerBefore | Where-Object { $_.name -eq $R.name } | Select-Object -First 1
			$Out += "| $($R.name) | $(Cell $R $W frame_ms '{0:N2}') | $(Cell $R $W world_ms '{0:N2}') | $(Cell $R $W game_ms '{0:N2}') | $(Cell $R $W mem_mb '{0:N0}') | $(Cell $R $W actors '{0:N0}') | $($R.creatures) |"
		}
		$Out += ""
	}
}

$Mem = Join-Path $Dir "memreport.txt"
if (Test-Path $Mem) {
	$Out += "## 메모리 (memreport -full 머리)", "", '```'
	$Out += Select-String -Path $Mem -Pattern "Process Physical Memory|Process Virtual Memory|Total texture memory|Texture Streaming|Nanite|RenderTargetPool|Total Physical" | Select-Object -First 12 | ForEach-Object { $_.Line.Trim() }
	$Out += '```', ""
}

$Summary = Join-Path $Dir "summary.md"
$Out | Set-Content -Path $Summary -Encoding utf8
# 콘솔(한국어 Windows 기본 CP949)에서도 글자가 깨지지 않게
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Out
