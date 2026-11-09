# 지형 수치 검사(레벨 디자인 4차) — 생성기로 스냅숏을 뜨고(terrain_snapshot.py) terrain_stats.py 로 판정한다. 맵을 건드리지 않는다.
#
#   powershell -File Scripts\Terrain\check_terrain.ps1            # 끝에 PASS/FAIL. 결과: Saved\Terrain\snap\now\stats.json
#
# 기준(자세한 것은 docs/qa/level-art.md): 자연 개울 가운데선 25 cm 마다 수심 ≥ 0.30 m · 끊김 0(LOD0 · 2 m 솎음 LOD1) · 터 안 높이 차 ≤ 1.3 m · 경사 ≤ 12° · 걸어서 닿음 ·
#                                        의도한 벼랑 밖 못 오르는 칸(44.76° 넘음) ≤ 0.2 % · 다리 사이각 ≥ 75°
# 시스템 파이썬 + numpy · Pillow 필요.
param([string]$Label = "now")

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Log = Join-Path $Root "Saved\Terrain\snap_$Label.log"
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null
$env:GRIM_SNAP_LABEL = $Label
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" (Join-Path $Root "Grimoire.uproject") -run=pythonscript -script="$Root\Scripts\Terrain\terrain_snapshot.py" -unattended -nop4 -nosplash -nullrhi -stdout *> $Log
if ($LASTEXITCODE -ne 0) { "스냅숏 실패 — $Log"; exit 1 }
python (Join-Path $Root "Scripts\Terrain\terrain_stats.py") $Label --json (Join-Path $Root "Saved\Terrain\snap\$Label\stats.json") --check
exit $LASTEXITCODE
