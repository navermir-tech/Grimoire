# 지형 1차(M3 갈래 W) 확인 — 헤드리스 단독 게임으로 L_OpenBeta 를 열어 지형 위에서 걷고, 땅 높이 · 시간 · 날씨 · 물 볼륨을 로그로 본다.
#
#   powershell -File Scripts\Terrain\check_openbeta.ps1     # 결과: Saved\TerrainCheck\openbeta.log 의 요약, 끝에 PASS/FAIL
#
# 시간표(엔진 시작 기준 초) — 맵 로드(10~20초)를 넉넉히 둔다:
#   15 탐침(탑 안 시작 자리 · 표식(랜드마크 · POI 포함)의 땅 높이 · 시간 · 날씨) → 17 앞마당으로 → 18~24 북쪽(길, Yaw 0)으로 걷기 → 25 탐침
#   26 초원(재칼로프 서식 구역)으로 → 27~32 동쪽(Yaw 90)으로 걷기 → 33 계곡(검은 개)으로 → 34~39 북동으로 걷기
#   40 날씨 덤프 → 41 초원 비 → 45 날씨 덤프 · 46 탐침(시간이 흘렀나) → 47 저지대 연못 속으로 → 49 탐침(헤엄 · 물 볼륨)
param(
	[int]$RunSeconds = 75
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$OutDir = Join-Path $Root "Saved\TerrainCheck"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Log = Join-Path $OutDir "openbeta.log"
Remove-Item $Log -ErrorAction SilentlyContinue

# 월드 좌표(cm): X = 북쪽 = (지도 Y − 380) × 100, Y = 동쪽 = (지도 X − 1000) × 100 — 탑 자리(1000, 380), Scripts/Terrain/openbeta_layout.json
$Cmds = @(
	"Grim.Terrain.Probe 15",
	"Grim.Terrain.Teleport 3000 0 17",
	"Grim.Terrain.Walk 18 6 0",
	"Grim.Terrain.Probe 25",
	"Grim.Terrain.Teleport 62000 20000 26",
	"Grim.Terrain.Walk 27 5 90",
	"Grim.Terrain.Teleport 262000 0 33",
	"Grim.Terrain.Walk 34 5 45",
	"Grim.Weather.Dump 40",
	"Grim.Weather.Set Meadow Rain 0 41",
	"Grim.Weather.Dump 45",
	"Grim.Terrain.Probe 46",
	"Grim.Terrain.Teleport 102000 -40000 47",
	"Grim.Terrain.Probe 50"
) -join ","

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=320", "-ResY=240", "-abslog=`"$Log`"", "-ExecCmds=`"$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
Start-Sleep -Seconds $RunSeconds
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Select-String -Path $Log -Encoding UTF8 -Pattern "LogGrimTerrain: |LogGrimEnvironment: Display|Error: " | ForEach-Object { $_.Line }

$Text = Get-Content $Log -Raw -Encoding UTF8
$Checks = [ordered]@{
	"랜드스케이프 로드" = "스트리밍 프록시 32"
	"시작 자리 땅(랜드스케이프)" = "(Landscape"
	"서식 구역 표식 — 재칼로프" = "Habitat.Jackalope"
	"서식 구역 표식 — 켈피" = "Habitat.Kelpie"
	"서식 구역 표식 — 검은 개" = "Habitat.BlackDog"
	"걷기 끝(떨어지지 않음)" = "떨어지는 중 아니오"
	"시간" = "[Probe] 시간:"
	"날씨 지역 Meadow" = "지역 Meadow"
	"초원 비" = "Meadow: Rain"
	"연못 물 볼륨(헤엄)" = "물 볼륨 WaterVolume_Pond_Lowland"
}
$Failed = 0
foreach ($Name in $Checks.Keys) {
	$Ok = $Text -and $Text.Contains($Checks[$Name])
	if (-not $Ok) { $Failed++ }
	Write-Output ("{0} {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $Name)
}
# 그림(렌더) 높이 범위가 충돌 높이와 맞는가 — -nullrhi 로 만든 랜드스케이프는 -256 m 근처에 그려져 안 보인다
$Bounds = [regex]::Match($Text, "그림 높이 (-?\d+) ~ (-?\d+) cm")
$Ok = $Bounds.Success -and ([int]$Bounds.Groups[1].Value -gt -5000) -and ([int]$Bounds.Groups[2].Value -gt 5000)
if (-not $Ok) { $Failed++ }
Write-Output ("{0} 랜드스케이프 그림 높이 {1}" -f ($(if ($Ok) { "[OK]  " } else { "[FAIL]" })), $(if ($Bounds.Success) { "$($Bounds.Groups[1].Value) ~ $($Bounds.Groups[2].Value) cm" } else { "없음" }))
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
