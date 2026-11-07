# 우리 맵 · 에셋이 쓰는 마켓 · Fab 팩 파일만 저장소(LFS)에 더한다 — docs/dev/market-assets.md 「쓰는 에셋만 올리기」.
#
#   powershell -File Scripts\Terrain\add_pack_assets.ps1 -DryRun    # 목록 · 크기만(Saved\PackDeps\)
#   powershell -File Scripts\Terrain\add_pack_assets.ps1            # git add -f 까지(커밋은 따로)
#
# 1) 헤드리스 에디터로 collect_pack_deps.py — Content/Grimoire · 우리 맵 외부 액터에서 의존을 끝까지 따라가 팩 폴더 안 파일만 적는다.
# 2) 그 파일만 git add -f(.gitignore 의 /Content/* 를 넘는다). 팩 폴더가 정션(워크트리)이어도 된다.
# 3) 팩별 개수 · 크기와 합계를 찍는다. 수백 MB 를 넘으면 커밋 전에 메인 세션 · 사용자에게 먼저 알린다.
param(
	[switch]$DryRun
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\PackDeps"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "collect.log"
$List = Join-Path $OutDir "used_pack_files.txt"
Remove-Item $List -ErrorAction SilentlyContinue

$P = Start-Process -FilePath $Exe -PassThru -Wait -WindowStyle Hidden -ArgumentList @("`"$Project`"", "-run=pythonscript",
	"-script=`"$(Join-Path $PSScriptRoot 'collect_pack_deps.py')`"", "-unattended", "-nop4", "-nosplash", "-nullrhi", "-abslog=`"$Log`"")
if (-not (Test-Path $List)) { Write-Output "FAIL — 목록이 없다($Log)"; exit 1 }
Get-Content (Join-Path $OutDir "summary.txt") -Encoding utf8
$Files = @(Get-Content $List -Encoding utf8 | Where-Object { $_ })
if ($DryRun) { Write-Output "(DryRun) $($Files.Count)개 — git add 안 함"; exit 0 }

Push-Location $Root
try {
	# 한 번에 너무 긴 명령줄이 되지 않게 200개씩
	for ($i = 0; $i -lt $Files.Count; $i += 200) {
		$Chunk = $Files[$i..([Math]::Min($i + 199, $Files.Count - 1))]
		git add -f -- $Chunk
		if ($LASTEXITCODE -ne 0) { Write-Output "FAIL — git add"; exit 1 }
	}
	$Set = @{}; foreach ($F in $Files) { $Set[$F] = $true }
	$Staged = @(git diff --cached --name-only | Where-Object { $Set.ContainsKey($_) })
	Write-Output "git add -f: $($Files.Count)개(이번에 바뀐 것 $($Staged.Count)개)"
} finally { Pop-Location }
Write-Output "PASS"