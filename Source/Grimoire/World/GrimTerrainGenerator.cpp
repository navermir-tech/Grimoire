#include "World/GrimTerrainGenerator.h"

#include "Async/ParallelFor.h"
#include "Math/RandomStream.h"

DEFINE_LOG_CATEGORY(LogGrimTerrain);

// 유니티 빌드에서 다른 파일과 이름이 부딪히지 않게 이름 붙은 네임스페이스를 쓴다.
namespace GrimTerrainGenLocal
{
	float Smooth01(double X)
	{
		const double C = FMath::Clamp(X, 0.0, 1.0);
		return static_cast<float>(C * C * (3.0 - 2.0 * C));
	}

	float SmoothStep(double A, double B, double X)
	{
		if (FMath::IsNearlyEqual(A, B))
		{
			return X < A ? 0.f : 1.f;
		}
		return Smooth01((X - A) / (B - A));
	}

	/** 노이즈 채널마다 다른 시작점 — Perlin 은 256 마다 되풀이되므로 그 안에서 고른다 */
	FVector2D SeedOffset(int32 Seed, int32 Channel)
	{
		FRandomStream Stream(static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(Channel * 7919 + 13))));
		return FVector2D(Stream.FRandRange(0.f, 256.f), Stream.FRandRange(0.f, 256.f));
	}

	float Perlin(double X, double Y, const FVector2D& Offset)
	{
		return FMath::PerlinNoise2D(FVector2D(X + Offset.X, Y + Offset.Y));
	}

	float Fbm(double X, double Y, int32 Octaves, const FVector2D& Offset)
	{
		float Sum = 0.f;
		float Amplitude = 1.f;
		float Norm = 0.f;
		double Frequency = 1.0;
		for (int32 Octave = 0; Octave < Octaves; ++Octave)
		{
			Sum += Amplitude * Perlin(X * Frequency + Octave * 31.7, Y * Frequency + Octave * 17.3, Offset);
			Norm += Amplitude;
			Amplitude *= 0.5f;
			Frequency *= 2.0;
		}
		return Norm > 0.f ? Sum / Norm : 0.f;
	}

	float Ridged(double X, double Y, const FVector2D& Offset)
	{
		return 1.f - FMath::Abs(Fbm(X, Y, 3, Offset));
	}

	/** 꺾은선 — 가장 가까운 점까지 거리와 그 점의 호 길이(상류부터) */
	struct FPolyline
	{
		TArray<FVector2D> Points;
		TArray<double> Cumulative;
		/** 조각마다 가운데 · 반 길이 — 먼 조각을 빨리 건너뛴다 */
		TArray<FVector2D> SegmentMid;
		TArray<double> SegmentHalf;
		FBox2D Bounds = FBox2D(ForceInit);

		/** 꺾은점을 지나는 부드러운 곡선(캣멀-롬)으로 바꾼다 — 강 · 개울 · 계곡 · 길이 꺾이지 않게 */
		static TArray<FVector2D> Smooth(const TArray<FVector2D>& In, int32 Subdivisions)
		{
			if (In.Num() < 3 || Subdivisions < 2)
			{
				return In;
			}
			TArray<FVector2D> Out;
			Out.Reserve((In.Num() - 1) * Subdivisions + 1);
			for (int32 Index = 0; Index + 1 < In.Num(); ++Index)
			{
				const FVector2D& P0 = In[FMath::Max(Index - 1, 0)];
				const FVector2D& P1 = In[Index];
				const FVector2D& P2 = In[Index + 1];
				const FVector2D& P3 = In[FMath::Min(Index + 2, In.Num() - 1)];
				for (int32 Step = 0; Step < Subdivisions; ++Step)
				{
					const double T = static_cast<double>(Step) / Subdivisions;
					const double T2 = T * T;
					const double T3 = T2 * T;
					Out.Add(0.5 * ((2.0 * P1) + (P2 - P0) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * T3));
				}
			}
			Out.Add(In.Last());
			return Out;
		}

		void Init(const TArray<FVector2D>& InPoints, int32 Subdivisions = 8)
		{
			Points = Smooth(InPoints, Subdivisions);
			Cumulative.SetNum(Points.Num());
			SegmentMid.Reset();
			SegmentHalf.Reset();
			double Accumulated = 0.0;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				if (Index > 0)
				{
					const double Segment = FVector2D::Distance(Points[Index - 1], Points[Index]);
					Accumulated += Segment;
					SegmentMid.Add((Points[Index - 1] + Points[Index]) * 0.5);
					SegmentHalf.Add(Segment * 0.5);
				}
				Cumulative[Index] = Accumulated;
				Bounds += Points[Index];
			}
		}

		bool IsValid() const { return Points.Num() >= 2; }
		double Length() const { return Cumulative.Num() ? Cumulative.Last() : 0.0; }

		/** MaxDistance 안에 있으면 참 — 거리 · 호 길이, 그리고 부호 있는 거리(진행 방향 왼쪽이 +) */
		bool Closest(const FVector2D& Query, double MaxDistance, double& OutDistance, double& OutArc, double* OutSigned = nullptr) const
		{
			if (!IsValid()
				|| Query.X < Bounds.Min.X - MaxDistance || Query.X > Bounds.Max.X + MaxDistance
				|| Query.Y < Bounds.Min.Y - MaxDistance || Query.Y > Bounds.Max.Y + MaxDistance)
			{
				return false;
			}
			double Best = MaxDistance * MaxDistance;
			double BestDistance = MaxDistance;
			int32 BestIndex = INDEX_NONE;
			for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
			{
				const double Reach = SegmentHalf[Index] + BestDistance;
				if ((Query - SegmentMid[Index]).SizeSquared() > Reach * Reach)
				{
					continue;
				}
				const FVector2D& A = Points[Index];
				const FVector2D AB = Points[Index + 1] - A;
				const double Length2 = AB.SizeSquared();
				const double T = Length2 > 0.0 ? FMath::Clamp(((Query - A) | AB) / Length2, 0.0, 1.0) : 0.0;
				const double D2 = (Query - (A + AB * T)).SizeSquared();
				if (D2 < Best)
				{
					Best = D2;
					BestDistance = FMath::Sqrt(D2);
					BestIndex = Index;
					OutArc = Cumulative[Index] + T * FMath::Sqrt(Length2);
				}
			}
			if (BestIndex == INDEX_NONE)
			{
				return false;
			}
			OutDistance = BestDistance;
			if (OutSigned)
			{
				const FVector2D& A = Points[BestIndex];
				const FVector2D AB = Points[BestIndex + 1] - A;
				const FVector2D AQ = Query - A;
				*OutSigned = (AB.X * AQ.Y - AB.Y * AQ.X) >= 0.0 ? BestDistance : -BestDistance;
			}
			return true;
		}

		FVector2D AtArc(double Arc) const
		{
			if (!IsValid())
			{
				return Points.Num() ? Points[0] : FVector2D::ZeroVector;
			}
			for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
			{
				const double Segment = Cumulative[Index + 1] - Cumulative[Index];
				if (Arc <= Cumulative[Index + 1] || Index + 2 == Points.Num())
				{
					const double T = Segment > 0.0 ? FMath::Clamp((Arc - Cumulative[Index]) / Segment, 0.0, 1.0) : 0.0;
					return FMath::Lerp(Points[Index], Points[Index + 1], T);
				}
			}
			return Points.Last();
		}
	};

	/** 꺾은선을 따라 Step(m) 마다 값 하나(개울 물 높이 · 길 높이 · 계곡 바닥) */
	struct FLineProfile
	{
		FPolyline Line;
		const FGrimTerrainChannel* Spec = nullptr;
		double Step = 4.0;
		TArray<double> Values;

		double ValueAt(double Arc) const
		{
			if (Values.Num() == 0)
			{
				return 0.0;
			}
			const double F = FMath::Clamp(Arc / Step, 0.0, static_cast<double>(Values.Num() - 1));
			const int32 I0 = FMath::FloorToInt32(F);
			const int32 I1 = FMath::Min(I0 + 1, Values.Num() - 1);
			return FMath::Lerp(Values[I0], Values[I1], F - I0);
		}
	};

	/** 이동 평균(창 = 양쪽 Half 개). 립시츠 조건(기울기 상한)을 지킨다 */
	TArray<double> MovingAverage(const TArray<double>& In, int32 Half)
	{
		if (Half < 1 || In.Num() < 3)
		{
			return In;
		}
		TArray<double> Out;
		Out.SetNumUninitialized(In.Num());
		for (int32 Index = 0; Index < In.Num(); ++Index)
		{
			double Sum = 0.0;
			int32 Count = 0;
			for (int32 Offset = -Half; Offset <= Half; ++Offset)
			{
				Sum += In[FMath::Clamp(Index + Offset, 0, In.Num() - 1)];
				++Count;
			}
			Out[Index] = Sum / Count;
		}
		return Out;
	}

	/** 물길 단면: 가운데 Depth 만큼 파고 물가는 물보다 0.3 m 위, 둑은 둘레 땅으로 부드럽게 */
	double CarveChannel(double Height, double Distance, double Water, double Depth, double HalfWidth, double Bank)
	{
		if (Distance < HalfWidth)
		{
			const double T = Distance / HalfWidth;
			return FMath::Min(Height, Water + 0.3 - (Depth + 0.3) * (1.0 - T * T));
		}
		const double T = Smooth01((Distance - HalfWidth) / FMath::Max(Bank, 0.01));
		return FMath::Min(Height, FMath::Lerp(Water + 0.3, Height, T));
	}

	/** 두 정점 사이를 선형 보간해 높이를 읽는다 */
	float ReadHeightFrom(const TArray<float>& Heights, int32 SizeX, int32 SizeY, double Quad, double U, double V)
	{
		const double FX = FMath::Clamp(U / Quad, 0.0, static_cast<double>(SizeX - 1));
		const double FY = FMath::Clamp(V / Quad, 0.0, static_cast<double>(SizeY - 1));
		const int32 X0 = FMath::Min(FMath::FloorToInt32(FX), SizeX - 2);
		const int32 Y0 = FMath::Min(FMath::FloorToInt32(FY), SizeY - 2);
		const float TX = static_cast<float>(FX - X0);
		const float TY = static_cast<float>(FY - Y0);
		const float H00 = Heights[Y0 * SizeX + X0];
		const float H10 = Heights[Y0 * SizeX + X0 + 1];
		const float H01 = Heights[(Y0 + 1) * SizeX + X0];
		const float H11 = Heights[(Y0 + 1) * SizeX + X0 + 1];
		return FMath::Lerp(FMath::Lerp(H00, H10, TX), FMath::Lerp(H01, H11, TX), TY);
	}

	float ReadHeight(const FGrimTerrainMaps& Maps, double U, double V)
	{
		return ReadHeightFrom(Maps.Heights, Maps.SizeX, Maps.SizeY, Maps.QuadSize, U, V);
	}

	/** 원 둘레 평균 높이 */
	double RingAverage(const FGrimTerrainMaps& Maps, const FVector2D& Center, double Radius, int32 Samples)
	{
		double Sum = 0.0;
		for (int32 Index = 0; Index < Samples; ++Index)
		{
			const double Angle = 2.0 * UE_DOUBLE_PI * Index / Samples;
			Sum += ReadHeight(Maps, Center.X + Radius * FMath::Cos(Angle), Center.Y + Radius * FMath::Sin(Angle));
		}
		return Sum / Samples;
	}

	uint8 ToByte(float Weight)
	{
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(Weight * 255.f), 0, 255));
	}

	/** 구역 하나 — 다각형 바운드 · 남북 끝 */
	struct FZonePrep
	{
		const FGrimTerrainZone* Spec = nullptr;
		FBox2D Bounds = FBox2D(ForceInit);
		double Reach = 0.0;
	};

	/** 놓인 언덕 + 계산용 값 */
	struct FHillPrep
	{
		FGrimTerrainPlacedHill Hill;
		FVector2D NoiseOffset = FVector2D::ZeroVector;
		double Cos = 1.0;
		double Sin = 0.0;
	};

	float HillValue(const FHillPrep& Prep, double U, double V, float Irregularity)
	{
		const FGrimTerrainPlacedHill& Hill = Prep.Hill;
		const double DX = U - Hill.Center.X;
		const double DY = V - Hill.Center.Y;
		const double LX = DX * Prep.Cos + DY * Prep.Sin;
		const double LY = (-DX * Prep.Sin + DY * Prep.Cos) * Hill.Aspect;
		const double R2 = LX * LX + LY * LY;
		const double Outer = Hill.Radius * (1.0 + Irregularity);
		if (R2 >= Outer * Outer)
		{
			return 0.f;
		}
		const double R = FMath::Sqrt(R2);
		double Radius = Hill.Radius;
		if (R > 1e-3)
		{
			Radius *= 1.0 + Irregularity * Perlin(LX / R * 1.7, LY / R * 1.7, Prep.NoiseOffset);
		}
		return Hill.Height * FGrimTerrainGenerator::HillProfile(static_cast<float>(R / FMath::Max(Radius, 1.0)), Hill.Sharpness);
	}

	/** 시드 언덕이 피하는 선(길 · 개울) */
	struct FAvoidLine
	{
		const FPolyline* Line = nullptr;
		double Half = 0.0;
	};
}

bool FGrimTerrainGenerator::Validate(const FGrimTerrainLayout& Layout, FString& OutError)
{
	const int32 SectionVerts = Layout.SectionQuads + 1;
	if (Layout.SectionQuads < 7 || Layout.SectionQuads > 255 || !FMath::IsPowerOfTwo(SectionVerts))
	{
		OutError = FString::Printf(TEXT("SectionQuads %d — 7 · 15 · 31 · 63 · 127 · 255 중 하나여야 한다"), Layout.SectionQuads);
		return false;
	}
	if (Layout.SectionsPerComponent != 1 && Layout.SectionsPerComponent != 2)
	{
		OutError = FString::Printf(TEXT("SectionsPerComponent %d — 1 또는 2"), Layout.SectionsPerComponent);
		return false;
	}
	if (Layout.ComponentsX < 1 || Layout.ComponentsY < 1 || Layout.ComponentsX * Layout.ComponentsY > 1024)
	{
		OutError = FString::Printf(TEXT("구성 요소 %d × %d — 1 이상, 합 1024 이하"), Layout.ComponentsX, Layout.ComponentsY);
		return false;
	}
	if (Layout.QuadSize <= 0.f)
	{
		OutError = TEXT("QuadSize 는 0 보다 커야 한다");
		return false;
	}
	for (const FGrimTerrainZone& Zone : Layout.Zones)
	{
		if (Zone.Polygon.Num() < 3)
		{
			OutError = FString::Printf(TEXT("구역 %s 의 다각형 꼭짓점이 3 개보다 적다"), *Zone.Name.ToString());
			return false;
		}
	}
	return true;
}

FIntPoint FGrimTerrainGenerator::GetVertexCount(const FGrimTerrainLayout& Layout)
{
	const int32 ComponentQuads = Layout.SectionQuads * Layout.SectionsPerComponent;
	return FIntPoint(Layout.ComponentsX * ComponentQuads + 1, Layout.ComponentsY * ComponentQuads + 1);
}

uint16 FGrimTerrainGenerator::HeightToLandscape(float HeightM, float ZScaleCm)
{
	const double Local = HeightM * 100.0 / ZScaleCm;
	return static_cast<uint16>(FMath::Clamp(FMath::RoundToInt64(32768.0 + Local * 128.0), int64(0), int64(65535)));
}

float FGrimTerrainGenerator::LandscapeToHeight(uint16 Value, float ZScaleCm)
{
	return static_cast<float>((static_cast<double>(Value) - 32768.0) / 128.0 * ZScaleCm / 100.0);
}

float FGrimTerrainGenerator::SampleHeight(const FGrimTerrainMaps& Maps, double U, double V)
{
	return Maps.IsValid() ? GrimTerrainGenLocal::ReadHeight(Maps, U, V) : 0.f;
}

float FGrimTerrainGenerator::SampleSlopeDeg(const FGrimTerrainMaps& Maps, double U, double V)
{
	if (!Maps.IsValid())
	{
		return 0.f;
	}
	const double D = Maps.QuadSize;
	const double DX = (GrimTerrainGenLocal::ReadHeight(Maps, U + D, V) - GrimTerrainGenLocal::ReadHeight(Maps, U - D, V)) / (2.0 * D);
	const double DY = (GrimTerrainGenLocal::ReadHeight(Maps, U, V + D) - GrimTerrainGenLocal::ReadHeight(Maps, U, V - D)) / (2.0 * D);
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(DX * DX + DY * DY))));
}

double FGrimTerrainGenerator::PolygonSignedDistance(const TArray<FVector2D>& Polygon, const FVector2D& P)
{
	const int32 Count = Polygon.Num();
	if (Count < 3)
	{
		return -1.0e9;
	}
	bool bInside = false;
	double Best = TNumericLimits<double>::Max();
	for (int32 I = 0, J = Count - 1; I < Count; J = I++)
	{
		const FVector2D& A = Polygon[J];
		const FVector2D& B = Polygon[I];
		if (((B.Y > P.Y) != (A.Y > P.Y)) && (P.X < (A.X - B.X) * (P.Y - B.Y) / (A.Y - B.Y) + B.X))
		{
			bInside = !bInside;
		}
		const FVector2D AB = B - A;
		const double Length2 = AB.SizeSquared();
		const double T = Length2 > 0.0 ? FMath::Clamp(((P - A) | AB) / Length2, 0.0, 1.0) : 0.0;
		Best = FMath::Min(Best, (P - (A + AB * T)).SizeSquared());
	}
	const double Distance = FMath::Sqrt(Best);
	return bInside ? Distance : -Distance;
}

float FGrimTerrainGenerator::HillProfile(float T, float Sharpness)
{
	if (T >= 1.f)
	{
		return 0.f;
	}
	const float C = FMath::Max(T, 0.f);
	const float K = FMath::Max(Sharpness, 1.f);
	return FMath::Pow(1.f - C, K) * (1.f + K * C);
}

float FGrimTerrainGenerator::HillMaxSlopeFactor(float Sharpness)
{
	// d/dt [(1 − t)^k (1 + k t)] = −k(k + 1) t (1 − t)^(k − 1) — 가장 가파른 곳 t = 1/k
	const float K = FMath::Max(Sharpness, 1.01f);
	return (K + 1.f) * FMath::Pow(1.f - 1.f / K, K - 1.f);
}

FVector2D FGrimTerrainGenerator::WorldToMap(const FGrimTerrainLayout& Layout, const FVector2D& WorldCm)
{
	// 월드 +X = 북쪽(지도 V), +Y = 동쪽(지도 U)
	return FVector2D(WorldCm.Y / 100.0 + Layout.WorldOrigin.X, WorldCm.X / 100.0 + Layout.WorldOrigin.Y);
}

FVector FGrimTerrainGenerator::MapToWorld(const FGrimTerrainLayout& Layout, double U, double V, double HeightM)
{
	return FVector((V - Layout.WorldOrigin.Y) * 100.0, (U - Layout.WorldOrigin.X) * 100.0, HeightM * 100.0);
}

void FGrimTerrainGenerator::Generate(const FGrimTerrainLayout& L, FGrimTerrainMaps& Out)
{
	using namespace GrimTerrainGenLocal;

	const FIntPoint Count = GetVertexCount(L);
	Out = FGrimTerrainMaps();
	Out.SizeX = Count.X;
	Out.SizeY = Count.Y;
	Out.QuadSize = L.QuadSize;
	const int32 Total = Count.X * Count.Y;
	Out.Heights.SetNumUninitialized(Total);
	Out.Shore.SetNumZeroed(Total);
	Out.Path.SetNumZeroed(Total);
	Out.Forest.SetNumZeroed(Total);
	Out.WaterDistance.SetNumUninitialized(Total);
	Out.PathDistance.SetNumUninitialized(Total);
	Out.TowerPlateauHeight = L.TowerHill.Height;

	const double Q = L.QuadSize;
	const double MapW = (Count.X - 1) * Q;
	const double MapH = (Count.Y - 1) * Q;

	const FVector2D BaseOff = SeedOffset(L.Seed, 1);
	const FVector2D DetailOff = SeedOffset(L.Seed, 2);
	const FVector2D RidgeOff = SeedOffset(L.Seed, 3);
	const FVector2D CoastOff = SeedOffset(L.Seed, 4);
	const FVector2D HillOff = SeedOffset(L.Seed, 5);
	const FVector2D RollingOff = SeedOffset(L.Seed, 7);
	const FVector2D ZoneOff = SeedOffset(L.Seed, 8);
	const FVector2D BumpOff = SeedOffset(L.Seed, 9);
	const FVector2D GullyOff = SeedOffset(L.Seed, 10);
	const FVector2D GullyDepthOff = SeedOffset(L.Seed, 11);
	const FVector2D ExtraOff = SeedOffset(L.Seed, 12);

	FPolyline Valley;
	Valley.Init(L.Valley.Points);
	FPolyline River;
	River.Init(L.River.Points);
	TArray<FPolyline> PathLines;
	for (const FGrimTerrainChannel& Path : L.Paths)
	{
		PathLines.AddDefaulted_GetRef().Init(Path.Points);
	}
	TArray<FPolyline> CreekLines;
	for (const FGrimTerrainChannel& Creek : L.Creeks)
	{
		CreekLines.AddDefaulted_GetRef().Init(Creek.Points);
	}
	TArray<FPolyline> RidgeLines;
	for (const FGrimTerrainRidge& Ridge : L.Ridges)
	{
		RidgeLines.AddDefaulted_GetRef().Init(Ridge.Points);
	}

	// ── 0. 언덕 놓기(손 + 시드). 길 · 개울 · 강 · 계곡 · 탑 · 연못 · 거점을 피한다 ──
	TArray<FHillPrep> Hills;
	{
		TArray<FAvoidLine> Avoid;
		for (int32 Index = 0; Index < PathLines.Num(); ++Index)
		{
			Avoid.Add({ &PathLines[Index], L.Paths[Index].Width * 0.5 + L.Paths[Index].BankWidth });
		}
		for (int32 Index = 0; Index < CreekLines.Num(); ++Index)
		{
			Avoid.Add({ &CreekLines[Index], L.Creeks[Index].Width * 0.5 + L.Creeks[Index].BankWidth });
		}

		auto AddHill = [&](const FGrimTerrainPlacedHill& Hill)
		{
			FHillPrep& Prep = Hills.AddDefaulted_GetRef();
			Prep.Hill = Hill;
			Prep.NoiseOffset = HillOff + FVector2D(Hills.Num() * 3.17, Hills.Num() * 1.91);
			Prep.Cos = FMath::Cos(Hill.YawRad);
			Prep.Sin = FMath::Sin(Hill.YawRad);
		};

		for (const FGrimTerrainHillSpec& Spec : L.Hills)
		{
			FGrimTerrainPlacedHill Hill;
			Hill.Name = Spec.Name;
			Hill.Center = Spec.Center;
			Hill.Height = Spec.Height;
			Hill.Radius = Spec.Diameter * 0.5f;
			Hill.Aspect = FMath::Max(Spec.Aspect, 1.f);
			Hill.YawRad = FMath::DegreesToRadians(Spec.Yaw);
			Hill.Sharpness = Spec.Sharpness > 0.f ? Spec.Sharpness : L.HillSharpness;
			Hill.bHandPlaced = true;
			AddHill(Hill);
		}

		auto IsClear = [&](const FVector2D& C, double R, bool bSmall, float Spacing, float LineClearance, float HandSpacing)
		{
			if (C.X < 0.0 || C.Y < 0.0 || C.X > MapW || C.Y > MapH)
			{
				return false;
			}
			double Distance = 0.0, Arc = 0.0;
			for (const FAvoidLine& Line : Avoid)
			{
				const double Need = Line.Half + (bSmall ? R + 3.0 : R * LineClearance + 6.0);
				if (Line.Line->Closest(C, Need, Distance, Arc))
				{
					return false;
				}
			}
			if (River.Closest(C, L.River.Width * 0.5 + L.River.BankWidth + R, Distance, Arc))
			{
				return false;
			}
			if (Valley.Closest(C, L.Valley.FloorWidth * 0.5 + R * 0.5, Distance, Arc))
			{
				return false;
			}
			if (FVector2D::Distance(C, L.TowerHill.Center) < L.TowerHill.Radius + R * 0.7)
			{
				return false;
			}
			for (const FGrimTerrainPond& Pond : L.Ponds)
			{
				if (FVector2D::Distance(C, Pond.Center) < Pond.Radius + Pond.ShoreWidth + 25.0 + R)
				{
					return false;
				}
			}
			for (const FGrimTerrainSite& Site : L.BaseSites)
			{
				if (FVector2D::Distance(C, Site.Center) < Site.Radius + Site.BlendWidth + R * 0.8)
				{
					return false;
				}
			}
			for (const FHillPrep& Other : Hills)
			{
				const float Factor = (Other.Hill.bHandPlaced && HandSpacing > 0.f) ? HandSpacing : Spacing;
				if (Other.Hill.bSmallShape == bSmall && FVector2D::Distance(C, Other.Hill.Center) < (R + Other.Hill.Radius) * Factor)
				{
					return false;
				}
			}
			return true;
		};

		for (int32 FieldIndex = 0; FieldIndex < L.HillFields.Num(); ++FieldIndex)
		{
			const FGrimTerrainHillField& Field = L.HillFields[FieldIndex];
			FRandomStream Random(static_cast<int32>(HashCombine(GetTypeHash(L.Seed), GetTypeHash(Field.Name.ToString()) + FieldIndex * 7)));
			FBox2D Box(ForceInit);
			for (const FVector2D& Point : Field.Polygon)
			{
				Box += Point;
			}
			const float K = Field.Sharpness > 0.f ? Field.Sharpness : L.HillSharpness;
			TArray<int32> BigHills;
			for (int32 Index = 0; Index < Hills.Num(); ++Index)
			{
				if (!Hills[Index].Hill.bSmallShape && !Hills[Index].Hill.bHandPlaced)
				{
					BigHills.Add(Index);
				}
			}
			int32 Placed = 0;
			for (int32 Attempt = 0; Attempt < Field.Count * 60 && Placed < Field.Count; ++Attempt)
			{
				const float SizeT = FMath::Pow(Random.FRand(), FMath::Max(Field.SizeBias, 0.1f));
				const float HeightJitter = Random.FRandRange(0.85f, 1.15f);
				const float Aspect = FMath::Lerp(1.f, FMath::Max(1.f, Field.MaxAspect), Random.FRand());
				const float Yaw = Random.FRandRange(0.f, UE_PI);
				const double PU = Random.FRand();
				const double PV = Random.FRand();
				const int32 Pick = Random.RandHelper(FMath::Max(BigHills.Num(), 1));

				FVector2D Center;
				if (Field.bNearHillTops)
				{
					if (BigHills.Num() == 0)
					{
						break;
					}
					const FGrimTerrainPlacedHill& Host = Hills[BigHills[Pick]].Hill;
					const double Angle = PU * 2.0 * UE_DOUBLE_PI;
					const double Along = FMath::Sqrt(PV) * 0.4 * Host.Radius / Host.Aspect;
					Center = Host.Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Along;
				}
				else
				{
					if (!Box.bIsValid)
					{
						break;
					}
					Center = Box.Min + (Box.Max - Box.Min) * FVector2D(PU, PV);
					if (PolygonSignedDistance(Field.Polygon, Center) < 0.0)
					{
						continue;
					}
				}
				const float Radius = FMath::Lerp(Field.MinDiameter, Field.MaxDiameter, SizeT) * 0.5f;
				float Height = FMath::Clamp(FMath::Lerp(Field.MinHeight, Field.MaxHeight, SizeT) * HeightJitter, Field.MinHeight, Field.MaxHeight);
				if (!Field.bSmallShape)
				{
					// 가장 가파른 곳(타원의 짧은 쪽)이 HillMaxSlopeDeg 를 넘지 않게 — 둔턱 · 침식 골이 더해져도 25° 안팎(언덕 = 10~25°, docs/dev/level-design.md 「4」)
					const float SlopeCap = Field.MaxSlopeDeg > 0.f ? Field.MaxSlopeDeg : L.HillMaxSlopeDeg;
					const float MaxGrade = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(SlopeCap, 5.f, 60.f)));
					Height = FMath::Max(FMath::Min(Height, Radius * MaxGrade / (HillMaxSlopeFactor(K) * Aspect)), FMath::Min(Field.MinHeight, Field.MaxHeight));
				}
				if (!IsClear(Center, Radius, Field.bSmallShape, Field.MinSpacing, Field.LineClearance >= 0.f ? Field.LineClearance : L.HillLineClearance, Field.HandHillSpacing))
				{
					continue;
				}
				FGrimTerrainPlacedHill Hill;
				Hill.Name = *FString::Printf(TEXT("%s_%02d"), *Field.Name.ToString(), Placed);
				Hill.Center = Center;
				Hill.Height = Height;
				Hill.Radius = Radius;
				Hill.Aspect = Aspect;
				Hill.YawRad = Yaw;
				Hill.Sharpness = K;
				Hill.bSmallShape = Field.bSmallShape;
				AddHill(Hill);
				++Placed;
			}
			if (Placed < Field.Count)
			{
				UE_LOG(LogGrimTerrain, Display, TEXT("언덕 군 %s: %d / %d 개만 놓였다(자리 부족)"), *Field.Name.ToString(), Placed, Field.Count);
			}
		}
		for (const FHillPrep& Prep : Hills)
		{
			Out.PlacedHills.Add(Prep.Hill);
		}
	}

	// 언덕을 버퍼에 그린다 — 언덕마다 바운드 안 정점만(겹침: 가장 큰 것 + 나머지 × HillOverlapBlend, 작은 모양은 더한다)
	TArray<float> HillMax, HillSum, SmallSum;
	HillMax.SetNumZeroed(Total);
	HillSum.SetNumZeroed(Total);
	SmallSum.SetNumZeroed(Total);
	for (const FHillPrep& Prep : Hills)
	{
		const double Reach = Prep.Hill.Radius * (1.0 + L.HillIrregularity) + Q;
		const int32 X0 = FMath::Clamp(FMath::FloorToInt32((Prep.Hill.Center.X - Reach) / Q), 0, Count.X - 1);
		const int32 X1 = FMath::Clamp(FMath::CeilToInt32((Prep.Hill.Center.X + Reach) / Q), 0, Count.X - 1);
		const int32 Y0 = FMath::Clamp(FMath::FloorToInt32((Prep.Hill.Center.Y - Reach) / Q), 0, Count.Y - 1);
		const int32 Y1 = FMath::Clamp(FMath::CeilToInt32((Prep.Hill.Center.Y + Reach) / Q), 0, Count.Y - 1);
		ParallelFor(Y1 - Y0 + 1, [&, X0, X1, Y0](int32 Row)
		{
			const int32 Y = Y0 + Row;
			for (int32 X = X0; X <= X1; ++X)
			{
				const float Value = HillValue(Prep, X * Q, Y * Q, L.HillIrregularity);
				if (Value > 0.f)
				{
					const int32 I = Y * Count.X + X;
					if (Prep.Hill.bSmallShape)
					{
						SmallSum[I] += Value;
					}
					else
					{
						HillMax[I] = FMath::Max(HillMax[I], Value);
						HillSum[I] += Value;
					}
				}
			}
		});
	}

	TArray<FZonePrep> Zones;
	for (const FGrimTerrainZone& Zone : L.Zones)
	{
		FZonePrep& Prep = Zones.AddDefaulted_GetRef();
		Prep.Spec = &Zone;
		for (const FVector2D& Point : Zone.Polygon)
		{
			Prep.Bounds += Point;
		}
		Prep.Reach = Zone.BlendWidth * 0.5 + FMath::Abs(Zone.EdgeWobble) + 1.0;
	}

	// ── 1. 바탕: 구역 바닥 + 노이즈 + 언덕 + 바위 능선 + 둔턱 + 가장자리 산등성이(고개) ──
	ParallelFor(Count.Y, [&](int32 Y)
	{
		const double V = Y * Q;
		for (int32 X = 0; X < Count.X; ++X)
		{
			const double U = X * Q;
			const FVector2D P(U, V);
			const int32 I = Out.Index(X, Y);
			double H = L.BaseHeight + L.NorthRise * SmoothStep(L.NorthRiseStart, L.NorthRiseEnd, V);

			// 구역 바닥(뒤가 앞을 덮는다) — 경계는 노이즈로 들쭉날쭉하게. 구역 잔물결(ExtraRolling*)도 같은 무게로 섞는다
			const double Wobble = Fbm(U / 260.0, V / 260.0, 2, ZoneOff);
			double ExtraAmplitude = 0.0, ExtraScale = 60.0;
			for (const FZonePrep& Zone : Zones)
			{
				if (U < Zone.Bounds.Min.X - Zone.Reach || U > Zone.Bounds.Max.X + Zone.Reach || V < Zone.Bounds.Min.Y - Zone.Reach || V > Zone.Bounds.Max.Y + Zone.Reach)
				{
					continue;
				}
				const FGrimTerrainZone& Spec = *Zone.Spec;
				const double Signed = PolygonSignedDistance(Spec.Polygon, P) + Spec.EdgeWobble * Wobble;
				const double Weight = Smooth01((Signed + Spec.BlendWidth * 0.5) / FMath::Max(Spec.BlendWidth, 1.f));
				if (Weight > 0.0)
				{
					const double ZoneHeight = Spec.BaseHeight + Spec.NorthRise * Smooth01((V - Zone.Bounds.Min.Y) / FMath::Max(Zone.Bounds.Max.Y - Zone.Bounds.Min.Y, 1.0));
					H = FMath::Lerp(H, ZoneHeight, Weight);
					// 잔물결은 덮는 순서와 상관없이 가장 센 구역 것 — 높이만 맡는 구역이 뒤에 와도 지워지지 않게
					if (Weight * Spec.ExtraRollingAmplitude > ExtraAmplitude)
					{
						ExtraAmplitude = Weight * Spec.ExtraRollingAmplitude;
						ExtraScale = FMath::Max(Spec.ExtraRollingScale, 5.f);
					}
				}
			}
			if (ExtraAmplitude > 0.0)
			{
				H += ExtraAmplitude * Fbm(U / ExtraScale, V / ExtraScale, 3, ExtraOff);
			}

			H += L.NoiseAmplitude * Fbm(U / L.NoiseScale, V / L.NoiseScale, L.NoiseOctaves, BaseOff)
				+ L.RollingAmplitude * Fbm(U / L.RollingScale, V / L.RollingScale, 3, RollingOff)
				+ L.DetailAmplitude * Fbm(U / L.DetailScale, V / L.DetailScale, 3, DetailOff)
				+ L.BumpAmplitude * Fbm(U / FMath::Max(L.BumpScale, 1.f), V / FMath::Max(L.BumpScale, 1.f), 3, BumpOff);

			// 언덕 · 바위 노두
			H += HillMax[I] + L.HillOverlapBlend * (HillSum[I] - HillMax[I]) + SmallSum[I];

			// 바위 능선 — 날카로운 등성이, 끝은 낮아진다
			double Distance = 0.0, Arc = 0.0;
			for (int32 Index = 0; Index < RidgeLines.Num(); ++Index)
			{
				const FGrimTerrainRidge& Ridge = L.Ridges[Index];
				if (RidgeLines[Index].Closest(P, Ridge.HalfWidth, Distance, Arc))
				{
					const double Taper = Smooth01(Arc / 80.0) * Smooth01((RidgeLines[Index].Length() - Arc) / 80.0);
					const double Rough = 0.7 + 0.3 * Ridged(U / 70.0, V / 70.0, RidgeOff);
					H += Ridge.Height * FMath::Pow(1.0 - Distance / Ridge.HalfWidth, 1.3) * Rough * Taper;
				}
			}

			// 가장자리 산등성이 — 폭이 들쭉날쭉하고 꼭대기는 능선 노이즈. 북쪽은 고개(틈)에서 낮아진다
			const double RidgeNoise = 0.6 + 0.4 * Ridged(U / L.RidgeNoiseScale, V / L.RidgeNoiseScale, RidgeOff);
			const double WobbleAlongV = 1.0 + L.RidgeWobble * Fbm(V / 300.0, 3.3, 3, RidgeOff);
			const double WobbleAlongU = 1.0 + L.RidgeWobble * Fbm(U / 300.0, 7.7, 3, RidgeOff);
			double NorthGap = 1.0;
			for (const FGrimTerrainGap& Gap : L.PassGaps)
			{
				NorthGap = FMath::Min(NorthGap, static_cast<double>(SmoothStep(Gap.Width * 0.5, Gap.Width * 0.5 + Gap.BlendWidth, FMath::Abs(U - Gap.CenterU))));
			}
			// 모서리에서 두 산등성이가 겹치면 높은 쪽만(더하면 모서리가 지나치게 솟는다)
			double EdgeRidge = 0.0;
			auto AddRidge = [&](double DistanceFromEdge, double Width, double Height)
			{
				if (Width > 0.0)
				{
					const double T = 1.0 - DistanceFromEdge / Width;
					if (T > 0.0)
					{
						EdgeRidge = FMath::Max(EdgeRidge, Height * FMath::Pow(Smooth01(T), 1.5) * RidgeNoise);
					}
				}
			};
			AddRidge(U, L.WestRidgeWidth * WobbleAlongV, L.WestRidgeHeight);
			AddRidge(MapH - V, L.NorthRidgeWidth * WobbleAlongU, L.NorthRidgeHeight * NorthGap);
			AddRidge(MapW - U, L.EastRidgeWidth * WobbleAlongV, L.EastRidgeHeight);
			H += EdgeRidge;

			Out.Heights[I] = static_cast<float>(H);
		}
	});

	// ── 1b. 침식 골 — 비탈(경사 GullySlopeStart 이상)에 아래로 뻗은 좁은 골 ──
	if (L.GullyDepth > 0.f)
	{
		const TArray<float> Before = Out.Heights;
		const int32 D = FMath::Max(1, FMath::RoundToInt32(2.0 / Q));
		ParallelFor(Count.Y, [&](int32 Y)
		{
			if (Y < D || Y >= Count.Y - D)
			{
				return;
			}
			const double V = Y * Q;
			for (int32 X = D; X < Count.X - D; ++X)
			{
				const double U = X * Q;
				const double GX = (Before[Out.Index(X + D, Y)] - Before[Out.Index(X - D, Y)]) / (2.0 * D * Q);
				const double GY = (Before[Out.Index(X, Y + D)] - Before[Out.Index(X, Y - D)]) / (2.0 * D * Q);
				const double G = FMath::Sqrt(GX * GX + GY * GY);
				const double SlopeDeg = FMath::RadiansToDegrees(FMath::Atan(G));
				const double Mask = SmoothStep(L.GullySlopeStart, L.GullySlopeFull, SlopeDeg);
				if (Mask <= 0.0 || G < 1e-4)
				{
					continue;
				}
				const double NX = GX / G, NY = GY / G;
				const double Across = (U * -NY + V * NX) / FMath::Max(L.GullyAcrossScale, 1.f);
				const double Along = (U * NX + V * NY) / FMath::Max(L.GullyAlongScale, 1.f);
				const double Noise = FMath::Abs(Perlin(Across, Along, GullyOff));
				const double Groove = Smooth01(1.0 - Noise / 0.16);
				const double DepthScale = 0.5 + 0.5 * Perlin(U / 220.0, V / 220.0, GullyDepthOff);
				Out.Heights[Out.Index(X, Y)] -= static_cast<float>(L.GullyDepth * Mask * Groove * FMath::Max(DepthScale, 0.0));
			}
		});
	}

	// ── 1c. 계곡 바닥 높이: 가운데선 땅 − 깊이, 고르고, 하류로 내려가기만 ──
	FLineProfile ValleyFloor;
	if (Valley.IsValid())
	{
		ValleyFloor.Line = Valley;
		ValleyFloor.Step = 4.0;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Valley.Length() / ValleyFloor.Step));
		// 「둘레」 = 양쪽 둑(바닥 반폭 + 비탈 + 10 m) 중 낮은 쪽 — 한쪽이 고지(높음)여도 낮은 쪽에서 Depth 만큼 판다
		TArray<double> Ground;
		const double GentleReach = L.Valley.FloorWidth * 0.5 + L.Valley.GentleWidth + 10.0;
		const double CliffReach = L.Valley.FloorWidth * 0.5 + L.Valley.CliffWidth + 10.0;
		const double CliffSign = L.Valley.CliffSide >= 0 ? 1.0 : -1.0;
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double ArcAt = Index * ValleyFloor.Step;
			const FVector2D At = Valley.AtArc(ArcAt);
			const FVector2D Dir = (Valley.AtArc(ArcAt + 4.0) - Valley.AtArc(ArcAt - 4.0)).GetSafeNormal();
			const FVector2D Left(-Dir.Y, Dir.X);
			const FVector2D CliffPoint = At + Left * (CliffSign * CliffReach);
			const FVector2D GentlePoint = At - Left * (CliffSign * GentleReach);
			Ground.Add(FMath::Min(ReadHeight(Out, CliffPoint.X, CliffPoint.Y), ReadHeight(Out, GentlePoint.X, GentlePoint.Y)));
		}
		Ground = MovingAverage(Ground, FMath::RoundToInt32(L.Valley.FloorSmoothing * 0.5 / ValleyFloor.Step));
		double Previous = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double Floor = FMath::Max(FMath::Min(Previous - L.Valley.FloorGrade * ValleyFloor.Step, Ground[Index] - L.Valley.Depth), static_cast<double>(L.Valley.MinFloorHeight));
			ValleyFloor.Values.Add(Floor);
			Previous = Floor;
		}
	}

	const FGrimTerrainHill& Hill = L.TowerHill;
	auto CoastV = [&](double U)
	{
		const double Head = FMath::Exp(-FMath::Square((U - Hill.Center.X) / FMath::Max(L.HeadlandWidth, 1.f)));
		return L.CoastLine + L.CoastWobble * Perlin(U / FMath::Max(L.CoastWobbleScale, 1.f), 0.5, CoastOff) - L.HeadlandBulge * Head;
	};
	auto CoastWidthAt = [&](double U)
	{
		const double Head = FMath::Exp(-FMath::Square((U - Hill.Center.X) / FMath::Max(L.HeadlandWidth, 1.f)));
		return FMath::Lerp(static_cast<double>(L.CoastWidth), static_cast<double>(L.CliffWidth), Head);
	};
	const double ValleyHalf = L.Valley.FloorWidth * 0.5;
	const double WidthVary = FMath::Clamp(L.Valley.WidthVariation, 0.f, 0.8f);
	const double CliffVary = FMath::Clamp(L.Valley.CliffVariation, 0.f, 0.9f);
	const double ValleyReach = ValleyHalf * (1.0 + WidthVary) + FMath::Max(L.Valley.GentleWidth * (1.0 + WidthVary), L.Valley.CliffWidth * (1.0 + CliffVary));
	const double VaryScale = FMath::Max(L.Valley.VariationScale, 10.f);

	// ── 2. 계곡(한쪽 절벽) · 탑 언덕 · 해안 · 큰 강 ──
	ParallelFor(Count.Y, [&](int32 Y)
	{
		const double V = Y * Q;
		for (int32 X = 0; X < Count.X; ++X)
		{
			const double U = X * Q;
			const FVector2D P(U, V);
			const int32 I = Out.Index(X, Y);
			double H = Out.Heights[I];

			// 계곡: 바닥은 고른 높이(+ 작은 둔턱), 절벽 쪽은 곧게 떨어지고 완만한 쪽은 부드럽게
			double Distance = 0.0, Arc = 0.0, Signed = 0.0;
			if (Valley.Closest(P, ValleyReach, Distance, Arc, &Signed))
			{
				const double Floor = ValleyFloor.ValueAt(Arc) + 0.8 * L.BumpAmplitude * Fbm(U / 30.0, V / 30.0, 2, BumpOff);
				const bool bCliff = (Signed >= 0.0 ? 1 : -1) == (L.Valley.CliffSide >= 0 ? 1 : -1);
				// 폭이 계곡을 따라 들쭉날쭉 — 양쪽이 서로 다른 노이즈라 바닥이 굽이친다
				const double Half = ValleyHalf * (1.0 + WidthVary * Perlin(Arc / VaryScale, bCliff ? 2.3 : 5.9, RidgeOff));
				const double SideWidth = FMath::Max(bCliff
					? L.Valley.CliffWidth * (1.0 + CliffVary * Perlin(Arc / (VaryScale * 0.7), 8.1, RidgeOff))
					: L.Valley.GentleWidth * (1.0 + WidthVary * Perlin(Arc / VaryScale, 11.3, RidgeOff)), 6.0);
				double Carved = H;
				if (Distance < Half)
				{
					Carved = Floor;
				}
				else if (Distance < Half + SideWidth)
				{
					const double T = (Distance - Half) / SideWidth;
					// 절벽: 거의 곧게(위 끝만 살짝 둥글게), 완만한 쪽: 부드러운 S 자
					const double Shape = bCliff ? FMath::Pow(T, 0.85) : Smooth01(T);
					Carved = FMath::Lerp(Floor, H, Shape);
				}
				H = FMath::Min(H, Carved);
			}

			// 탑 언덕 — 꼭대기는 완전한 평지
			const FVector2D ToHill = P - Hill.Center;
			const double R = ToHill.Size();
			const double Angle = FMath::Atan2(ToHill.Y, ToHill.X);
			const double Outer = FMath::Max(Hill.Radius * (1.0 + Hill.Irregularity * Perlin(FMath::Cos(Angle) * 1.7 + 10.0, FMath::Sin(Angle) * 1.7 + 10.0, HillOff)), Hill.PlateauRadius + 1.0);
			if (R <= Hill.PlateauRadius)
			{
				H = Hill.Height;
			}
			else if (R < Outer)
			{
				H = FMath::Lerp(static_cast<double>(Hill.Height), H, Smooth01((R - Hill.PlateauRadius) / (Outer - Hill.PlateauRadius)));
			}

			// 해안 — 물가선(S = 0) 북쪽은 모래밭으로 내려오고, 남쪽은 바닥이 깊어진다
			const double S = V - CoastV(U);
			const double CoastW = CoastWidthAt(U);
			if (S < CoastW)
			{
				H = S >= 0.0
					? FMath::Lerp(static_cast<double>(L.SeaLevel), H, Smooth01(S / CoastW))
					: L.SeaLevel - 0.05 - L.SeaDepth * Smooth01(-S / FMath::Max(L.SeaSlopeWidth, 1.f));
			}

			// 큰 강 — 물은 해수면
			const double RiverHalf = L.River.Width * 0.5;
			if (River.Closest(P, RiverHalf + L.River.BankWidth, Distance, Arc))
			{
				H = CarveChannel(H, Distance, L.SeaLevel, L.River.Depth, RiverHalf, L.River.BankWidth);
			}

			Out.Heights[I] = static_cast<float>(H);
		}
	});

	// ── 3. 길: 땅을 따라가되 경사 PathGradeDeg 아래로 고른 높이로 깎고 메운다(언덕은 레이아웃에서 길이 피해 휜다) ──
	TArray<FLineProfile> Paths;
	const double Grade = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(L.PathGradeDeg, 1.f, 40.f)));
	for (int32 PathIndex = 0; PathIndex < L.Paths.Num(); ++PathIndex)
	{
		FLineProfile& Profile = Paths.AddDefaulted_GetRef();
		Profile.Spec = &L.Paths[PathIndex];
		Profile.Line = PathLines[PathIndex];
		Profile.Step = 2.0;
		if (!Profile.Line.IsValid())
		{
			continue;
		}
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Profile.Line.Length() / Profile.Step));
		TArray<double> Ground, Lower, Upper;
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const FVector2D At = Profile.Line.AtArc(Index * Profile.Step);
			Ground.Add(ReadHeight(Out, At.X, At.Y));
		}
		// 기울기 상한을 지키는 가장 높은 「깎기만」 선과 가장 낮은 「메우기만」 선의 가운데 — 둘 다 상한을 지키므로 가운데도 지킨다
		const double Rise = Grade * Profile.Step;
		Lower = Ground;
		Upper = Ground;
		for (int32 Index = 1; Index <= Steps; ++Index)
		{
			Lower[Index] = FMath::Min(Lower[Index], Lower[Index - 1] + Rise);
			Upper[Index] = FMath::Max(Upper[Index], Upper[Index - 1] - Rise);
		}
		for (int32 Index = Steps - 1; Index >= 0; --Index)
		{
			Lower[Index] = FMath::Min(Lower[Index], Lower[Index + 1] + Rise);
			Upper[Index] = FMath::Max(Upper[Index], Upper[Index + 1] - Rise);
		}
		Profile.Values.SetNum(Steps + 1);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			Profile.Values[Index] = 0.5 * (Lower[Index] + Upper[Index]);
		}
		Profile.Values = MovingAverage(Profile.Values, FMath::RoundToInt32(L.PathSmoothing * 0.5 / Profile.Step));
		// 갈림길은 큰길 노면 높이에서 시작한다 — 첫 값을 큰길 높이로 박고, 경사 상한을 지키며 고른 높이를 따라간다
		if (PathIndex > 0)
		{
			Profile.Values[0] = Ground[0];
			for (int32 Index = 1; Index <= Steps; ++Index)
			{
				Profile.Values[Index] = FMath::Clamp(Profile.Values[Index], Profile.Values[Index - 1] - Rise, Profile.Values[Index - 1] + Rise);
			}
		}

		const double Half = Profile.Spec->Width * 0.5;
		const double Bank = FMath::Max(Profile.Spec->BankWidth, 0.5f);
		ParallelFor(Count.Y, [&](int32 Y)
		{
			for (int32 X = 0; X < Count.X; ++X)
			{
				const FVector2D P(X * Q, Y * Q);
				if (FVector2D::Distance(P, Hill.Center) <= Hill.PlateauRadius + 0.5)
				{
					continue;
				}
				double Distance = 0.0, Arc = 0.0;
				// 앞서 깎은 길(큰길)의 노면은 갈림길이 덮지 않는다 — 갈림목에서 턱이 지지 않게
				bool bEarlierRoad = false;
				for (int32 Earlier = 0; Earlier < PathIndex && !bEarlierRoad; ++Earlier)
				{
					bEarlierRoad = Paths[Earlier].Line.Closest(P, L.Paths[Earlier].Width * 0.5 + L.Paths[Earlier].BankWidth * 0.5, Distance, Arc);
				}
				if (bEarlierRoad)
				{
					continue;
				}
				if (Profile.Line.Closest(P, Half + Bank, Distance, Arc))
				{
					float& H = Out.Heights[Out.Index(X, Y)];
					const double Target = Profile.ValueAt(Arc);
					H = static_cast<float>(Distance < Half ? Target : FMath::Lerp(Target, static_cast<double>(H), static_cast<double>(Smooth01((Distance - Half) / Bank))));
				}
			}
		});
	}

	// ── 4. 개울 물 높이(상류 → 하류로 내려가기만) · 폭포 ──
	TArray<FLineProfile> Creeks;
	for (int32 CreekIndex = 0; CreekIndex < L.Creeks.Num(); ++CreekIndex)
	{
		const FGrimTerrainChannel& Spec = L.Creeks[CreekIndex];
		FLineProfile& Profile = Creeks.AddDefaulted_GetRef();
		Profile.Spec = &Spec;
		Profile.Line = CreekLines[CreekIndex];
		Profile.Step = 4.0;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Profile.Line.Length() / Profile.Step));
		double Previous = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const FVector2D At = Profile.Line.AtArc(Index * Profile.Step);
			const double Ground = ReadHeight(Out, At.X, At.Y);
			const double Water = FMath::Max(FMath::Min(Previous - 0.004 * Profile.Step, Ground - Spec.WaterBelowGround), L.SeaLevel + 0.2);
			Profile.Values.Add(Water);
			Previous = Water;
		}
	}
	if (Creeks.Num() > 0)
	{
		ParallelFor(Count.Y, [&](int32 Y)
		{
			for (int32 X = 0; X < Count.X; ++X)
			{
				const FVector2D P(X * Q, Y * Q);
				float& H = Out.Heights[Out.Index(X, Y)];
				for (const FLineProfile& Creek : Creeks)
				{
					const double Half = Creek.Spec->Width * 0.5;
					double Distance = 0.0, Arc = 0.0;
					if (Creek.Line.Closest(P, Half + Creek.Spec->BankWidth, Distance, Arc))
					{
						H = static_cast<float>(CarveChannel(H, Distance, Creek.ValueAt(Arc), Creek.Spec->Depth, Half, Creek.Spec->BankWidth));
					}
				}
			}
		});
	}

	// ── 5. 연못 · 거점 평지 ──
	TArray<double> PondLevels;
	for (const FGrimTerrainPond& Pond : L.Ponds)
	{
		PondLevels.Add(RingAverage(Out, Pond.Center, Pond.Radius + Pond.ShoreWidth, 48) - Pond.WaterBelowGround);
	}
	TArray<double> SiteLevels;
	for (const FGrimTerrainSite& Site : L.BaseSites)
	{
		double Sum = RingAverage(Out, Site.Center, Site.Radius * 0.5, 16) + RingAverage(Out, Site.Center, Site.Radius, 24);
		SiteLevels.Add(Sum * 0.5);
	}
	const double PondBlend = 20.0;
	ParallelFor(Count.Y, [&](int32 Y)
	{
		for (int32 X = 0; X < Count.X; ++X)
		{
			const FVector2D P(X * Q, Y * Q);
			double H = Out.Heights[Out.Index(X, Y)];
			for (int32 Index = 0; Index < L.Ponds.Num(); ++Index)
			{
				const FGrimTerrainPond& Pond = L.Ponds[Index];
				const double R = FVector2D::Distance(P, Pond.Center);
				const double Level = PondLevels[Index];
				const double Rim = Level + 0.9;
				if (R < Pond.Radius)
				{
					H = Level - Pond.Depth * (1.0 - FMath::Square(R / Pond.Radius));
				}
				else if (R < Pond.Radius + Pond.ShoreWidth)
				{
					H = Level + 0.9 * (R - Pond.Radius) / Pond.ShoreWidth;
				}
				else if (R < Pond.Radius + Pond.ShoreWidth + PondBlend)
				{
					H = FMath::Lerp(Rim, H, static_cast<double>(Smooth01((R - Pond.Radius - Pond.ShoreWidth) / PondBlend)));
				}
			}
			for (int32 Index = 0; Index < L.BaseSites.Num(); ++Index)
			{
				const FGrimTerrainSite& Site = L.BaseSites[Index];
				const double R = FVector2D::Distance(P, Site.Center);
				if (R < Site.Radius)
				{
					H = SiteLevels[Index];
				}
				else if (R < Site.Radius + Site.BlendWidth)
				{
					H = FMath::Lerp(SiteLevels[Index], H, static_cast<double>(Smooth01((R - Site.Radius) / Site.BlendWidth)));
				}
			}
			Out.Heights[Out.Index(X, Y)] = static_cast<float>(H);
		}
	});

	// ── 6. 무게(물가 · 길 · 계곡 숲) · 물과 길까지 거리 ──
	const double FarDistance = 64.0;
	ParallelFor(Count.Y, [&](int32 Y)
	{
		const double V = Y * Q;
		for (int32 X = 0; X < Count.X; ++X)
		{
			const double U = X * Q;
			const FVector2D P(U, V);
			const int32 I = Out.Index(X, Y);
			const double H = Out.Heights[I];

			double WaterDistance = FarDistance;
			float Shore = 0.f;

			// 바다(+ 바다 높이의 강)
			const double S = V - CoastV(U);
			if (H < L.SeaLevel)
			{
				WaterDistance = -1.0;
				Shore = 1.f;
			}
			else
			{
				WaterDistance = FMath::Min(WaterDistance, FMath::Max(S, 0.0));
				if (S < CoastWidthAt(U))
				{
					Shore = FMath::Max(Shore, 1.f - SmoothStep(1.5, 3.0, H - L.SeaLevel));
				}
			}

			double Distance = 0.0, Arc = 0.0;
			const double RiverHalf = L.River.Width * 0.5;
			if (River.Closest(P, RiverHalf + FarDistance, Distance, Arc))
			{
				WaterDistance = FMath::Min(WaterDistance, Distance - RiverHalf);
				Shore = FMath::Max(Shore, 1.f - SmoothStep(RiverHalf + 8.0, RiverHalf + 14.0, Distance));
			}
			for (const FLineProfile& Creek : Creeks)
			{
				const double Half = Creek.Spec->Width * 0.5;
				if (Creek.Line.Closest(P, Half + FarDistance, Distance, Arc))
				{
					WaterDistance = FMath::Min(WaterDistance, Distance - Half);
					Shore = FMath::Max(Shore, 1.f - SmoothStep(Half + 1.0, Half + 2.5, Distance));
				}
			}
			for (const FGrimTerrainPond& Pond : L.Ponds)
			{
				const double R = FVector2D::Distance(P, Pond.Center);
				WaterDistance = FMath::Min(WaterDistance, R - Pond.Radius);
				const double Sand = Pond.Radius + Pond.ShoreWidth * 0.5;
				Shore = FMath::Max(Shore, 1.f - SmoothStep(Sand, Sand + 4.0, R));
			}
			Out.WaterDistance[I] = static_cast<float>(FMath::Clamp(WaterDistance, -FarDistance, FarDistance));
			Out.Shore[I] = ToByte(Shore);

			double PathDistance = FarDistance;
			float PathWeight = 0.f;
			for (int32 Index = 0; Index < Paths.Num(); ++Index)
			{
				const double Half = L.Paths[Index].Width * 0.5;
				if (Paths[Index].Line.Closest(P, FarDistance, Distance, Arc))
				{
					PathDistance = FMath::Min(PathDistance, Distance);
					PathWeight = FMath::Max(PathWeight, 1.f - SmoothStep(Half, Half + 1.5, Distance));
				}
			}
			Out.PathDistance[I] = static_cast<float>(PathDistance);
			Out.Path[I] = ToByte(PathWeight);

			if (Valley.Closest(P, ValleyReach, Distance, Arc))
			{
				const double ForestEdge = ValleyHalf + 0.5 * L.Valley.GentleWidth;
				Out.Forest[I] = ToByte(1.f - SmoothStep(ForestEdge * 0.75, ForestEdge, Distance));
			}
		}
	});

	// ── 7. 길 선(검사용) · 물 목록(월드 cm) ──
	for (const FLineProfile& Path : Paths)
	{
		FGrimTerrainPathLine& Line = Out.PathLines.AddDefaulted_GetRef();
		Line.Name = Path.Spec->Name;
		Line.Length = Path.Line.Length();
		for (int32 Index = 0; Index < Path.Values.Num(); ++Index)
		{
			const double ArcAt = FMath::Min(Index * Path.Step, Line.Length);
			const FVector2D At = Path.Line.AtArc(ArcAt);
			Line.Points.Add(FVector(At.X, At.Y, Path.Values[Index]));
		}
	}
	for (int32 Index = 0; Index < L.Ponds.Num(); ++Index)
	{
		const FGrimTerrainPond& Pond = L.Ponds[Index];
		FGrimTerrainWaterBody& Body = Out.Water.AddDefaulted_GetRef();
		Body.Kind = TEXT("Pond");
		Body.Name = Pond.Name;
		Body.Points.Add(MapToWorld(L, Pond.Center.X, Pond.Center.Y, PondLevels[Index]));
		Body.Size = (Pond.Radius + Pond.ShoreWidth * 0.3f) * 100.f;
		Body.Depth = Pond.Depth * 100.f;
	}
	for (const FLineProfile& Creek : Creeks)
	{
		FGrimTerrainWaterBody& Body = Out.Water.AddDefaulted_GetRef();
		Body.Kind = TEXT("Creek");
		Body.Name = Creek.Spec->Name;
		const double Step = 12.0;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Creek.Line.Length() / Step));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double ArcAt = FMath::Min(Index * Step, Creek.Line.Length());
			const FVector2D At = Creek.Line.AtArc(ArcAt);
			Body.Points.Add(MapToWorld(L, At.X, At.Y, Creek.ValueAt(ArcAt)));
		}
		Body.Size = (Creek.Spec->Width + 0.6f) * 100.f;
		Body.Depth = Creek.Spec->Depth * 100.f;

		// 폭포: 4 m 마다 1.2 m 넘게 떨어지는 구간 중 가장 크게 떨어지는 것(8 m 이상)
		int32 BestStart = INDEX_NONE, BestEnd = INDEX_NONE;
		double BestDrop = 0.0;
		for (int32 Start = 0; Start + 1 < Creek.Values.Num();)
		{
			if (Creek.Values[Start] - Creek.Values[Start + 1] <= 1.2)
			{
				++Start;
				continue;
			}
			int32 End = Start + 1;
			while (End + 1 < Creek.Values.Num() && Creek.Values[End] - Creek.Values[End + 1] > 1.2)
			{
				++End;
			}
			const double Drop = Creek.Values[Start] - Creek.Values[End];
			if (Drop > BestDrop)
			{
				BestDrop = Drop;
				BestStart = Start;
				BestEnd = End;
			}
			Start = End;
		}
		if (BestStart != INDEX_NONE && BestDrop >= 8.0)
		{
			FGrimTerrainWaterBody& Falls = Out.Water.AddDefaulted_GetRef();
			Falls.Kind = TEXT("Falls");
			Falls.Name = Creek.Spec->Name;
			const FVector2D Top = Creek.Line.AtArc(BestStart * Creek.Step);
			const FVector2D Bottom = Creek.Line.AtArc(BestEnd * Creek.Step);
			Falls.Points.Add(MapToWorld(L, Top.X, Top.Y, Creek.Values[BestStart]));
			Falls.Points.Add(MapToWorld(L, Bottom.X, Bottom.Y, Creek.Values[BestEnd]));
			Falls.Size = (Creek.Spec->Width + 3.f) * 100.f;
			Falls.Depth = static_cast<float>(BestDrop * 100.0);
		}
	}
	if (River.IsValid())
	{
		FGrimTerrainWaterBody& Body = Out.Water.AddDefaulted_GetRef();
		Body.Kind = TEXT("River");
		Body.Name = L.River.Name;
		for (const FVector2D& Point : River.Points)
		{
			Body.Points.Add(MapToWorld(L, Point.X, Point.Y, L.SeaLevel));
		}
		Body.Size = L.River.Width * 100.f;
		Body.Depth = L.River.Depth * 100.f;
	}
	{
		FGrimTerrainWaterBody& Body = Out.Water.AddDefaulted_GetRef();
		Body.Kind = TEXT("Sea");
		Body.Name = TEXT("Sea");
		Body.Points.Add(MapToWorld(L, MapW * 0.5, MapH * 0.5, L.SeaLevel));
		Body.Size = 0.f;
		Body.Depth = L.SeaDepth * 100.f;
	}
}

void FGrimTerrainGenerator::ScatterFoliage(const FGrimTerrainLayout& L, const FGrimTerrainMaps& Maps, const FGrimTerrainFoliageRule& Rule, TArray<FTransform>& OutWorld)
{
	using namespace GrimTerrainGenLocal;

	OutWorld.Reset();
	if (!Maps.IsValid())
	{
		return;
	}
	const double MaxDensity = FMath::Max(FMath::Max(Rule.DensityPerHa, Rule.ForestDensityPerHa), FMath::Max(Rule.ValleyDensityPerHa, Rule.SteepDensityPerHa));
	if (MaxDensity <= 0.0)
	{
		return;
	}
	const double Cell = 100.0 / FMath::Sqrt(MaxDensity);
	const double MapW = (Maps.SizeX - 1) * Maps.QuadSize;
	const double MapH = (Maps.SizeY - 1) * Maps.QuadSize;
	const int32 CellsX = FMath::FloorToInt32(MapW / Cell);
	const int32 CellsY = FMath::FloorToInt32(MapH / Cell);
	const FVector2D ForestOff = SeedOffset(L.Seed, 6);
	const uint32 RuleHash = HashCombine(GetTypeHash(L.Seed), GetTypeHash(Rule.Name.ToString()));

	for (int32 CY = 0; CY < CellsY; ++CY)
	{
		for (int32 CX = 0; CX < CellsX; ++CX)
		{
			FRandomStream Random(static_cast<int32>(HashCombine(RuleHash, HashCombine(GetTypeHash(CX), GetTypeHash(CY * 92821)))));
			const double U = (CX + Random.FRand()) * Cell;
			const double V = (CY + Random.FRand()) * Cell;
			const float Pick = Random.FRand();
			const float ScaleT = Random.FRand();
			const float Yaw = Random.FRandRange(0.f, 360.f);

			const int32 I = Maps.NearestIndex(U, V);
			const double Slope = SampleSlopeDeg(Maps, U, V);

			double Density = Rule.DensityPerHa;
			if (Maps.Forest[I] > 128)
			{
				Density = Rule.ValleyDensityPerHa;
			}
			else if (Perlin(U / L.ForestNoiseScale, V / L.ForestNoiseScale, ForestOff) > L.ForestThreshold)
			{
				Density = Rule.ForestDensityPerHa;
			}
			const bool bSteep = Rule.SteepDensityPerHa > 0.f && Slope >= Rule.SteepSlopeDeg;
			if (bSteep)
			{
				Density = FMath::Max(Density, static_cast<double>(Rule.SteepDensityPerHa));
			}
			if (Pick >= Density / MaxDensity)
			{
				continue;
			}
			if (!bSteep && Slope > Rule.MaxSlopeDeg)
			{
				continue;
			}
			const double Height = ReadHeight(Maps, U, V);
			if (Height < Rule.MinHeight || Height > Rule.MaxHeight)
			{
				continue;
			}
			if (Maps.WaterDistance[I] < Rule.MinWaterDistance || Maps.PathDistance[I] < Rule.MinPathDistance)
			{
				continue;
			}
			const FVector2D P(U, V);
			if (FVector2D::Distance(P, L.TowerHill.Center) < L.TowerHill.PlateauRadius + L.TowerClearance)
			{
				continue;
			}
			bool bBlocked = false;
			for (const FGrimTerrainSite& Site : L.BaseSites)
			{
				bBlocked |= FVector2D::Distance(P, Site.Center) < Site.Radius + 4.0;
			}
			for (const FGrimTerrainLandmark& Landmark : L.Landmarks)
			{
				bBlocked |= FVector2D::Distance(P, Landmark.Center) < L.LandmarkClearance;
			}
			for (const FGrimTerrainPoi& Poi : L.Pois)
			{
				bBlocked |= FVector2D::Distance(P, Poi.Center) < Poi.Radius + 6.0;
			}
			if (bBlocked)
			{
				continue;
			}

			const float Scale = FMath::Lerp(Rule.MinScale, Rule.MaxScale, ScaleT);
			FVector Location = MapToWorld(L, U, V, Height);
			Location.Z -= Rule.SinkCm * Scale;
			OutWorld.Add(FTransform(FRotator(0.f, Yaw, 0.f), Location, FVector(Scale)));
		}
	}
}

FGrimTerrainSummary FGrimTerrainGenerator::MakeSummary(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps)
{
	FGrimTerrainSummary Summary;
	Summary.SizeX = Maps.SizeX;
	Summary.SizeY = Maps.SizeY;
	Summary.LandscapeLocation = MapToWorld(Layout, 0.0, 0.0, 0.0);
	Summary.LandscapeScale = FVector(Layout.QuadSize * 100.0, Layout.QuadSize * 100.0, 100.0);
	Summary.MapSize = FVector2D((Maps.SizeX - 1) * Maps.QuadSize, (Maps.SizeY - 1) * Maps.QuadSize);
	Summary.TowerPlateauHeight = Maps.TowerPlateauHeight;
	if (Maps.Heights.Num() > 0)
	{
		Summary.MinHeight = FMath::Min(Maps.Heights);
		Summary.MaxHeight = FMath::Max(Maps.Heights);
	}
	Summary.Water = Maps.Water;
	for (const FGrimTerrainPlacedHill& Hill : Maps.PlacedHills)
	{
		Hill.bSmallShape ? ++Summary.OutcropCount : ++Summary.HillCount;
	}
	return Summary;
}
