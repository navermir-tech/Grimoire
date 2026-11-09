# 지형 스크린숏 — RHI 를 켠 단독 게임(-game -RenderOffscreen, 창 없음)으로 L_OpenBeta 를 열어 정해 둔 자리에서 찍고,
# docs/public/art/terrain/ 에 jpg 로 둔다(사람 눈 확인용 — docs/dev/level-design.md 「6」 확인할 것 5 · 「7」 사람 눈높이).
#
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1              # 마탑 구간(-v3): 사람 눈높이 1.8 m
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set v2      # 2차 자리(전체 땅) — 이전 그림을 덮어쓰니 일부러만
#
# 자리(월드 cm, X = 북 = (지도 Y − 380) × 100, Y = 동 = (지도 X − 1000) × 100). Z 가 + 면 땅 위 cm(Grim.Terrain.ViewFrom).
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set dress -Tag before   # 레벨 아트 비교 — 측정 자리(Scripts/Perf/measure.ps1) 중 시작 구역
#                                                                               → docs/public/qa/level-dress-<자리>-<Tag>.jpg
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld3 -RunSeconds 1200  # 레벨 디자인 3차(「10」) — 고친 곳마다 눈높이 · 하늘, 2차와 같은 자리는 전 · 후 → docs/public/qa/ld3-<자리>.jpg
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld4 -RunSeconds 1500 # 레벨 디자인 4차(「11」) — 전망대 · 개울 · 어귀 · 능선 · 돌담 → docs/public/qa/ld4-<자리>.jpg
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set water -Tag day -RunSeconds 700                 # 물 재질(Q126) — docs/public/qa/water-<자리>-<Tag>.jpg
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set water -Tag dusk -TimeOfDay 0.74 -RunSeconds 700 # 같은 자리 황혼
#   powershell -File Scripts\Terrain\shoot_openbeta.ps1 -Set ld2 -RunSeconds 900   # 레벨 디자인 2차(docs/dev/level-design.md 「9」) — 구간마다 눈높이 · 하늘
#                                                                               → docs/public/qa/ld2-<자리>.jpg(앞 다섯은 level-dress-*-after 와 같은 자리 = 전 · 후)
param(
	[ValidateSet("v3", "v2", "dress", "ld2", "ld3", "ld4", "water")]
	[string]$Set = "v3",
	[string]$Tag = "after",
	# 시각 고정(0 자정 · 0.5 정오, Grim.Creature.Time) — 음수면 건드리지 않는다(시작 시각대로)
	[double]$TimeOfDay = -1,
	# 이 정규식에 맞는 이름만 찍는다(예: "creek-mid|pond$")
	[string]$Only = "",
	[int]$RunSeconds = 200,
	[int]$ResX = 1600,
	[int]$ResY = 900
)

$UE = "C:\Program Files\Epic Games\UE_5.8"
$Exe = "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $Root "Grimoire.uproject"
$ShotDir = Join-Path $Root "Saved\TerrainCheck"
$ArtDir = Join-Path $Root "docs\public\art\terrain"
New-Item -ItemType Directory -Force $ShotDir | Out-Null
$Log = Join-Path $ShotDir "shoot.log"
Remove-Item $Log -ErrorAction SilentlyContinue

# 이름, X, Y, Z, Yaw, Pitch, 그림 돌리기(위에서 본 그림을 북쪽이 위로)
# v3 — 마탑 구간(docs/dev/level-design.md 「7」): 눈높이 1.8 m(+180). 큰길 자리는 레이아웃 Path_Main 곡선에서 탑 문 앞부터 잰 거리, Yaw = 길 방향
$ShotsV3 = @(
	@("openbeta-door-v3", 4000, 0, "+180", 0, -2, $false),               # 탑 문 앞(지도 1000, 420)에서 북쪽 — 앞마당 · 큰길 · 언덕들 · 글씨
	@("openbeta-path100-v3", 13776, 3029, "+180", 25, -2, $false),      # 큰길 100 m(지도 1030, 518)
	@("openbeta-path200-v3", 23246, 2090, "+180", -26, -2, $false),     # 큰길 200 m(지도 1021, 612) — 연못 갈림길 앞
	@("openbeta-path400-v3", 41318, -1871, "+180", 32, -2, $false),     # 큰길 400 m(지도 981, 793)
	@("openbeta-hilltop-v3", 56800, -11000, "+180", 140, -4, $false),   # 봉화 언덕 꼭대기(지도 890, 948 — 전망 바위 옆)에서 남동 — 탑 · 풍차 · 바다
	@("openbeta-pond-v3", 12500, -30000, "+180", -5, -3, $false),       # 연못 남쪽 물가(지도 700, 505)에서 북 — 연못 · 부유석 · 봉화 언덕
	@("openbeta-fence-v3", 62000, 10000, "+180", 10, -2, $false),       # 큰길 북쪽 끝(지도 1100, 1000) — 개울 · 공사 중 경계
	@("openbeta-sector-aerial-v3", 21000, 9000, "125000", 0, -89.9, $false) # 하늘 1.25 km 에서 마탑 구간(화면 위 = 북쪽)
)
$ShotsV2 = @(
	@("openbeta-tower-north", 5000, 0, "+170", 0, -3, $false),          # 탑 문 앞(지도 1000, 430)에서 북쪽 — 고목 · 풍차 · 멀리 선돌 고리
	@("openbeta-tree-north", 161400, 10000, "+300", 0, -4, $false),     # 고목 언덕 꼭대기(고목 둘레 14 m)에서 북 — 계곡 · 폭포 · 고개
	@("openbeta-tree-east", 160000, 11400, "+300", 90, -5, $false),     # 동 — 강 · 감시탑
	@("openbeta-tree-south", 158600, 10000, "+300", 180, -4, $false),   # 남 — 목초지 · 탑 · 바다
	@("openbeta-tree-west", 160000, 8600, "+300", 270, -5, $false),     # 서 — 연못 저지대 · 서쪽 산등성이
	@("openbeta-valley-entrance", 232000, 6000, "+200", 57, -8, $false), # 큰길이 계곡 가장자리에 닿는 곳(지도 1060, 2700)에서 폭포 쪽
	@("openbeta-highland", 319000, -20000, "+200", 32, -2, $false),     # 북부 고지(지도 800, 3570)에서 고개 바위 문 쪽
	@("openbeta-aerial", 165200, 1600, "290000", -90, -89.9, $true)      # 하늘 2.9 km 에서 내려다본 전체(화면 위 = 서쪽 → 돌려서 북쪽이 위)
)
# dress — 레벨 아트 전 · 후 비교(docs/dev/level-design.md 「8」). 측정 자리와 같은 X · Y · Yaw(Scripts/Perf/measure.ps1), 눈높이 1.8 m
$ShotsDress = @(
	@("level-dress-door-$Tag", 4000, 0, "+180", 0, -2, $false),
	@("level-dress-path200-$Tag", 23246, 2090, "+180", -26, -2, $false),
	@("level-dress-pond-$Tag", 12500, -30000, "+180", -5, -3, $false),
	@("level-dress-hilltop-$Tag", 56800, -11000, "+180", 140, -4, $false),
	@("level-dress-path100-$Tag", 13776, 3029, "+180", 25, -2, $false),
	# Q124 비교(측정 자리 아님) — 사실적 스캔(쓰러진 나무)이 StyleHex 옆에 보이는 자리(dress_openbeta.py 로그의 소품 자리).
	# 그루터기 · Megaplants 참나무는 시험 뒤 맵에서 뺐다 — 그때 그림(level-dress-scan-stump-after · q125-oak-*)은 docs/public/qa 에 남긴다
	@("level-dress-compare-east-$Tag", 29000, 33500, "+170", 50, -4, $false),
	@("level-dress-compare-pond-$Tag", 14000, -27000, "+170", 50, -3, $false),
	@("level-dress-scan-trunk-$Tag", 1700, -12500, "+170", -40, -12, $false)    # 쓰러진 나무 스캔(Dress_FallenTrunk_01) 12 m 앞
)
# ld2 — 레벨 디자인 2차: 능선 · 안부 · 자연 분포 · 구간 구도 · 하늘 구역 이름 · 고목 큐브 목업. 지도 → 월드: X = (지도 Y − 380) × 100, Y = (지도 X − 1000) × 100
$ShotsLd2 = @(
	@("ld2-door", 4000, 0, "+180", 0, -2, $false),                      # 탑 문 앞 — level-dress-door-after 와 같은 자리(전 · 후)
	@("ld2-path100", 13776, 3029, "+180", 25, -2, $false),              # 큰길 100 m — 돌담 · 돌 표지 · 능선(전 · 후)
	@("ld2-path200", 23246, 2090, "+180", -26, -2, $false),             # 큰길 200 m — 연못 갈림길(전 · 후)
	@("ld2-pond", 12500, -30000, "+180", -5, -3, $false),               # 서쪽 연못 — 징검돌 · 섬 · 부유석(전 · 후)
	@("ld2-hilltop", 56800, -11000, "+180", 140, -4, $false),           # 봉화 언덕 꼭대기에서 남동(전 · 후)
	@("ld2-path400", 41318, -1871, "+180", 32, -2, $false),             # 큰길 400 m — 꽃길 초원 · 숲덩이
	@("ld2-westspine", 25000, -47000, "+180", 62, -4, $false),          # 서쪽 등줄기 등성이(지도 530, 630)에서 동북동 — 연못 골 · 탑
	@("ld2-eastsaddle", 22000, 36000, "+180", 0, 2, $false),             # 남동 숨은 골(지도 1360, 600)에서 북쪽 — 동쪽 안부(고갯마루) · 그 너머 풍차
	@("ld2-hiddenvale", 6000, 42000, "+180", 35, -3, $false),           # 남동 숨은 골(지도 1420, 440) — 두 능선 사이 들
	@("ld2-windmill", 33500, 28500, "+180", 45, 2, $false),             # 동쪽 길에서 풍차 마당 — 밀밭 · 울타리 · 곳간
	@("ld2-floatsteps", 31000, -34000, "+180", -42, 12, $false),        # 연못 길(지도 660, 690)에서 부유석 언덕 · 디딤돌
	@("ld2-tree-far", 114000, -4000, "+180", 17, 3, $false),            # 큰길(지도 960, 1520)에서 거대 고목(약 480 m)
	@("ld2-tree-near", 151500, 1500, "+180", 45, 10, $false),           # 고목 남서 120 m — 판뿌리 · 줄기 · 가지 · 잎 덩어리
	@("ld2-tree-under", 157800, 8000, "+180", 45, 28, $false),          # 고목 남서 30 m 에서 올려다봄 — 판뿌리 · 줄기 마디 · 가지
	@("ld2-labels-door", 4000, 0, "+180", 0, 14, $false),               # 탑 문 앞에서 하늘 — 구역 이름
	@("ld2-aerial-south", -26000, 0, "28000", 0, -20, $false),          # 바다 위 280 m 에서 북쪽 비스듬히 — 콘셉트 r1-key-aerial 구도
	@("ld2-aerial-region", -60000, 2000, "70000", 0, -24, $false),      # 바다 위 700 m — 지역 전체 · 하늘 구역 이름
	@("ld2-aerial-tree", 130000, 10000, "+22000", 0, -32, $false),      # 고목 남쪽 하늘 220 m — 중부 구릉 · 고목 실루엣
	@("ld2-sector-aerial", 21000, 9000, "125000", 0, -89.9, $false)     # 하늘 1.25 km 에서 마탑 구간(openbeta-sector-aerial-v3 와 같은 자리)
)
# ld3 — 레벨 디자인 3차: 능선 · 개울 · 다리 · 연못 어귀 · 샘 · 짐수레 · 돌담 · 채움. 앞 여덟은 ld2 와 같은 자리(전 · 후)
$ShotsLd3 = @(
	@("ld3-door", 4000, 0, "+180", 0, -2, $false),
	@("ld3-path100", 13776, 3029, "+180", 25, -2, $false),
	@("ld3-path200", 23246, 2090, "+180", -26, -2, $false),
	@("ld3-pond", 12500, -30000, "+180", -5, -3, $false),
	@("ld3-hilltop", 56800, -11000, "+180", 140, -4, $false),
	@("ld3-path400", 41318, -1871, "+180", 32, -2, $false),
	@("ld3-eastsaddle", 22000, 36000, "+180", 0, 2, $false),
	@("ld3-hiddenvale", 6000, 42000, "+180", 35, -3, $false),
	@("ld3-ridge-west", 24500, 1500, "+180", 285, 4, $false),           # 큰길에서 서쪽 등줄기 급사면
	@("ld3-ridge-north", 36000, -6000, "+180", 340, 6, $false),         # 큰길에서 봉화 언덕 쪽 능선
	@("ld3-ridge-east", 24500, 1500, "+180", 75, 4, $false),            # 큰길에서 동쪽 등줄기 급사면
	@("ld3-ridge-crest", 57800, -12000, "+180", 250, -2, $false),        # 봉화 언덕 등성이에서 서쪽으로 — 이어지는 등줄기 · 안부
	@("ld3-spring", 49000, -13500, "+650", 18, -18, $false),             # 샘 — 물머리와 상류 물길(바위 틈 · 내민 바위 · 웅덩이)
	@("ld3-spring-near", 50400, -13200, "+230", 22, -10, $false),         # 샘 — 바위 틈 · 웅덩이 가까이
	@("ld3-creek-upper", 45500, -14200, "+200", 200, -8, $false),        # V 자 상류
	@("ld3-creek-mid", 39400, -16800, "+180", 211, -5, $false),          # 개울 가운데 — 굽이 · 소
	@("ld3-bridge", 32200, -18800, "+180", 270, -4, $false),             # 다리 — 길에서 바로
	@("ld3-bridge-side", 28600, -20600, "+150", 12, 6, $false),          # 다리 — 하류 물가에서 올려봄
	@("ld3-bridge-top", 32000, -20100, "+1500", 0, -62, $false),         # 다리 — 위에서(직각 · 둑 얹힘 확인)
	@("ld3-pond-junction", 23200, -25000, "+180", 217, -4, $false),      # 개울이 연못에 드는 어귀
	@("ld3-pond-aerial", 21500, -28500, "+4500", 40, -60, $false),       # 연못 · 어귀 하늘에서
	@("ld3-creek-aerial", 34000, -20500, "+17000", 0, -89.9, $false),    # 개울 전체 하늘에서
	@("ld3-cart", 48050, 7350, "+180", 62, -7, $false),                  # 부서진 짐수레 — 길 굽이 길섶
	@("ld3-wall-a", 13375, 3146, "+180", 42, -3, $false),                # 돌담 가까이
	@("ld3-wall-b", 26325, 909, "+180", -24, -3, $false),
	@("ld3-wall-c", 33830, -3941, "+180", -13, -3, $false),
	@("ld3-wall-d", 41255, -1455, "+180", 46, -3, $false),
	@("ld3-wall-e", 49388, 7230, "+180", 48, -3, $false),
	@("ld3-fill-road520", 47323, 5075, "+180", 56, -2, $false),          # 길가 채움
	@("ld3-fill-road640", 56272, 10872, "+180", -9, -2, $false),
	@("ld3-fill-road780", 69000, 13000, "+180", 5, -2, $false),          # 북쪽 다리 앞
	@("ld3-aerial-south", -26000, 0, "28000", 0, -20, $false),           # 바다 위 — 능선 실루엣(ld2-aerial-south 와 같은 자리)
	@("ld3-sector-aerial", 21000, 9000, "125000", 0, -89.9, $false)
)
# water — 물 재질(Q126 Water Materials): ld3 와 같은 자리에서 낮 · 황혼 → docs/public/qa/water-<자리>-<Tag>.jpg
$ShotsLd4 = @(
	# 레벨 디자인 4차(docs/dev/level-design.md 「11」) — 3차와 같은 자리는 전 · 후. 좌표 X = (지도 Y − 380) × 100, Y = (지도 X − 1000) × 100
	@("ld4-sector-aerial", 21000, 9000, "125000", 0, -89.9, $false),
	@("ld4-door", 4000, 0, "+180", 0, -2, $false),
	@("ld4-path100", 13776, 3029, "+180", 25, -2, $false),
	@("ld4-path400", 41318, -1871, "+180", 32, -2, $false),
	@("ld4-pond", 12500, -30000, "+180", -5, -3, $false),
	# 전망대 — 사용자 스크린숏(남쪽 위에서 비스듬히) · 정상 눈높이 네 방향 · 가장자리 · 오르는 길
	@("ld4-beacon-aerial", 50500, -12400, "13200", 0, -32, $false),
	@("ld4-beacon-top-se", 58500, -11000, "+180", 140, -4, $false),
	@("ld4-beacon-top-nw", 57400, -13800, "+180", 300, -4, $false),
	@("ld4-beacon-edge", 58100, -10800, "+180", 270, -3, $false),
	@("ld4-beacon-trail", 52000, -9500, "+180", 297, 4, $false),
	@("ld4-beacon-trail-air", 52000, -7000, "13000", 305, -34, $false),
	# 개울 — 위에서(끊김) · 눈높이 · 먼 높이(LOD)
	@("ld4-creek-upper-air", 46600, -13800, "+3000", 0, -80, $false),
	@("ld4-creek-mid-air", 39000, -17700, "+3000", 0, -80, $false),
	@("ld4-creek-lower-air", 26700, -22000, "+3000", 0, -80, $false),
	@("ld4-creek-far-air", 38000, -18500, "+14000", 0, -80, $false),
	@("ld4-creek-eye-mid", 39300, -17300, "+180", 215, -6, $false),
	@("ld4-creek-eye-lower", 27200, -21800, "+180", 200, -6, $false),
	@("ld4-pasture-air", 69300, 20700, "+3000", 0, -80, $false),
	# 어귀
	@("ld4-mouth-air", 21000, -26400, "+3500", 0, -89, $false),
	@("ld4-mouth-oblique", 24500, -24000, "+900", 219, -18, $false),
	@("ld4-mouth-eye", 22600, -25200, "+180", 219, -5, $false),
	# 능선 — 사용자 스크린숏처럼 비스듬한 하늘 시점
	@("ld4-ridge-west-air", 32000, -30000, "13000", 270, -18, $false),
	@("ld4-ridge-beacon-air", 44000, -24000, "13000", 318, -16, $false),
	@("ld4-ridge-east-air", 22000, 15000, "11000", 95, -16, $false),
	@("ld4-ridge-windmill-air", 42000, 25000, "11000", 130, -16, $false),
	@("ld4-ridge-se-air", 12000, 40000, "11000", 165, -16, $false),
	@("ld4-river-bank", 12000, 64000, "+180", 90, -2, $false),
	# 돌담
	@("ld4-wall-close", 9000, 150, "+150", 270, -4, $false),
	@("ld4-wall-road", 6000, 800, "+170", 0, -2, $false),
	@("ld4-wall-mill-lane", 32000, 33000, "+170", 5, -3, $false),
	@("ld4-wall-field", 35800, 30500, "+170", 90, -3, $false),
	@("ld4-fold-mill", 41000, 26500, "+220", 40, -9, $false)
)
$ShotsWater = @(
	@("water-creek-mid-$Tag", 39400, -16800, "+180", 211, -5, $false),     # 개울 가운데(눈높이)
	@("water-creek-upper-$Tag", 45500, -14200, "+200", 200, -8, $false),   # V 자 상류
	@("water-bridge-$Tag", 32200, -18800, "+180", 270, -4, $false),        # 다리 — 길에서
	@("water-bridge-side-$Tag", 28600, -20600, "+150", 12, 6, $false),     # 다리 — 하류 물가에서
	@("water-pond-junction-$Tag", 23200, -25000, "+180", 217, -4, $false), # 연못 어귀
	@("water-pond-$Tag", 12500, -30000, "+180", -5, -3, $false),           # 연못 남쪽 물가
	@("water-spring-near-$Tag", 50400, -13200, "+230", 22, -10, $false),   # 샘 — 물머리 동심원
	@("water-spring-top-$Tag", 50850, -12950, "+380", 20, -62, $false),    # 샘 — 바로 위에서(Spring_Ripple 동심원)
	@("water-mouth-top-$Tag", 22000, -26500, "+500", 225, -40, $false),    # 어귀 — 위에서(개울 UV → 연못 UV 경계)
	@("water-pond-aerial-$Tag", 21500, -28500, "+4500", 40, -60, $false),  # 연못 · 어귀 하늘에서
	@("water-creek-aerial-$Tag", 34000, -20500, "+17000", 0, -89.9, $false) # 개울 전체 하늘에서
)
$Shots = if ($Set -eq "ld4") { $ShotsLd4 } elseif ($Set -eq "ld3") { $ShotsLd3 } elseif ($Set -eq "water") { $ShotsWater } elseif ($Set -eq "v2") { $ShotsV2 } elseif ($Set -eq "dress") { $ShotsDress } elseif ($Set -eq "ld2") { $ShotsLd2 } else { $ShotsV3 }
if ($Only) { $Shots = @($Shots | Where-Object { $_[0] -match $Only }) }   # 다듬는 중에는 몇 장만
$Settled = @("dress", "ld2", "ld3", "ld4", "water") -contains $Set   # 풀이 자랄 시간을 두고 docs/public/qa 에 두는 세트
if ($Settled) { $ArtDir = Join-Path $Root "docs\public\qa" }

$Cmds = @()
if ($TimeOfDay -ge 0) {
	# 시각은 시간이 흐르니 자리마다 다시 맞춘다 — 찍는 사이 해가 움직이지 않게
	$Cmds += "Grim.Creature.Time $TimeOfDay 2"
}
$Delay = 25
foreach ($Shot in $Shots) {
	if ($TimeOfDay -ge 0) { $Cmds += "Grim.Creature.Time $TimeOfDay $Delay" }
	if ($Settled) {
		# 랜드스케이프 풀은 카메라 둘레에서 비동기로 자란다 — 먼저 옮겨 두고 25 초 뒤에 찍는다
		$Cmds += "Grim.Terrain.ViewFrom $($Shot[1]) $($Shot[2]) $($Shot[3]) $($Shot[4]) $($Shot[5]) $Delay"
		$Delay += 25
	}
	$Cmds += "Grim.Terrain.ViewFrom $($Shot[1]) $($Shot[2]) $($Shot[3]) $($Shot[4]) $($Shot[5]) $Delay $($Shot[0])"
	$Delay += $(if ($Settled) { 5 } else { 12 })
}
$Cmds = $Cmds -join ","
foreach ($Shot in $Shots) { Remove-Item (Join-Path $ShotDir "$($Shot[0]).png") -ErrorAction SilentlyContinue }

$GameArgs = @("`"$Project`"", "/Game/Grimoire/Maps/L_OpenBeta", "-game", "-RenderOffscreen", "-nosound", "-unattended", "-nosplash", "-nop4", "-log",
	"-windowed", "-ResX=$ResX", "-ResY=$ResY", "-abslog=`"$Log`"", "-ExecCmds=`"r.ScreenPercentage 100,$Cmds`"")
$Game = Start-Process -FilePath $Exe -PassThru -ArgumentList $GameArgs
$Deadline = (Get-Date).AddSeconds($RunSeconds)
$Last = Join-Path $ShotDir "$($Shots[-1][0]).png"
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline -and -not (Test-Path $Last)) { Start-Sleep -Seconds 2 }
Start-Sleep -Seconds 3
if (-not $Game.HasExited) { Stop-Process -Id $Game.Id -Force }

Add-Type -AssemblyName System.Drawing
$Codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$Params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$Params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]82)
$Failed = 0
foreach ($Shot in $Shots) {
	$Png = Join-Path $ShotDir "$($Shot[0]).png"
	if (-not (Test-Path $Png)) { Write-Output "[FAIL] $($Shot[0]) — 스크린숏 없음"; $Failed++; continue }
	$Image = [System.Drawing.Image]::FromFile($Png)
	if ($Shot[6]) { $Image.RotateFlip([System.Drawing.RotateFlipType]::Rotate270FlipNone) }
	$Jpg = Join-Path $ArtDir "$($Shot[0]).jpg"
	$Image.Save($Jpg, $Codec, $Params)
	$Image.Dispose()
	Write-Output "[OK]   $Jpg"
}
if ($Failed -eq 0) { Write-Output "PASS"; exit 0 } else { Write-Output "FAIL ($Failed)"; exit 1 }
