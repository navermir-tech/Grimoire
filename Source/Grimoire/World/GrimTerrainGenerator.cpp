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
			double Accumulated = 0.0;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				if (Index > 0)
				{
					Accumulated += FVector2D::Distance(Points[Index - 1], Points[Index]);
				}
				Cumulative[Index] = Accumulated;
				Bounds += Points[Index];
			}
		}

		bool IsValid() const { return Points.Num() >= 2; }
		double Length() const { return Cumulative.Num() ? Cumulative.Last() : 0.0; }

		bool Closest(const FVector2D& Query, double MaxDistance, double& OutDistance, double& OutArc) const
		{
			if (!IsValid()
				|| Query.X < Bounds.Min.X - MaxDistance || Query.X > Bounds.Max.X + MaxDistance
				|| Query.Y < Bounds.Min.Y - MaxDistance || Query.Y > Bounds.Max.Y + MaxDistance)
			{
				return false;
			}
			double Best = MaxDistance * MaxDistance;
			bool bFound = false;
			for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
			{
				const FVector2D& A = Points[Index];
				const FVector2D AB = Points[Index + 1] - A;
				const double Length2 = AB.SizeSquared();
				const double T = Length2 > 0.0 ? FMath::Clamp(((Query - A) | AB) / Length2, 0.0, 1.0) : 0.0;
				const double D2 = (Query - (A + AB * T)).SizeSquared();
				if (D2 < Best)
				{
					Best = D2;
					bFound = true;
					OutArc = Cumulative[Index] + T * FMath::Sqrt(Length2);
				}
			}
			if (bFound)
			{
				OutDistance = FMath::Sqrt(Best);
			}
			return bFound;
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

	/** 개울 물 높이 — 상류부터 Step(m) 마다 */
	struct FCreekProfile
	{
		FPolyline Line;
		const FGrimTerrainChannel* Spec = nullptr;
		double Step = 4.0;
		TArray<double> Water;

		double WaterAt(double Arc) const
		{
			if (Water.Num() == 0)
			{
				return 0.0;
			}
			const double F = FMath::Clamp(Arc / Step, 0.0, static_cast<double>(Water.Num() - 1));
			const int32 I0 = FMath::FloorToInt32(F);
			const int32 I1 = FMath::Min(I0 + 1, Water.Num() - 1);
			return FMath::Lerp(Water[I0], Water[I1], F - I0);
		}
	};

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
	float ReadHeight(const FGrimTerrainMaps& Maps, double U, double V)
	{
		const double FX = FMath::Clamp(U / Maps.QuadSize, 0.0, static_cast<double>(Maps.SizeX - 1));
		const double FY = FMath::Clamp(V / Maps.QuadSize, 0.0, static_cast<double>(Maps.SizeY - 1));
		const int32 X0 = FMath::Min(FMath::FloorToInt32(FX), Maps.SizeX - 2);
		const int32 Y0 = FMath::Min(FMath::FloorToInt32(FY), Maps.SizeY - 2);
		const float TX = static_cast<float>(FX - X0);
		const float TY = static_cast<float>(FY - Y0);
		const float H00 = Maps.Heights[Maps.Index(X0, Y0)];
		const float H10 = Maps.Heights[Maps.Index(X0 + 1, Y0)];
		const float H01 = Maps.Heights[Maps.Index(X0, Y0 + 1)];
		const float H11 = Maps.Heights[Maps.Index(X0 + 1, Y0 + 1)];
		return FMath::Lerp(FMath::Lerp(H00, H10, TX), FMath::Lerp(H01, H11, TX), TY);
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

	FPolyline Valley;
	Valley.Init(L.Valley.Points);
	FPolyline River;
	River.Init(L.River.Points);
	TArray<FPolyline> Paths;
	for (const FGrimTerrainChannel& Path : L.Paths)
	{
		Paths.AddDefaulted_GetRef().Init(Path.Points);
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

	// ── 1. 바탕 · 산등성이 · 계곡 · 탑 언덕 · 해안 · 큰 강 ──
	ParallelFor(Count.Y, [&](int32 Y)
	{
		const double V = Y * Q;
		for (int32 X = 0; X < Count.X; ++X)
		{
			const double U = X * Q;
			const FVector2D P(U, V);
			double H = L.BaseHeight
				+ L.NorthRise * SmoothStep(L.NorthRiseStart, L.NorthRiseEnd, V)
				+ L.NoiseAmplitude * Fbm(U / L.NoiseScale, V / L.NoiseScale, L.NoiseOctaves, BaseOff)
				+ L.RollingAmplitude * Fbm(U / L.RollingScale, V / L.RollingScale, 3, RollingOff)
				+ L.DetailAmplitude * Fbm(U / L.DetailScale, V / L.DetailScale, 3, DetailOff);

			// 가장자리 산등성이 — 폭이 들쭉날쭉하고 꼭대기는 능선 노이즈
			const double RidgeNoise = 0.6 + 0.4 * Ridged(U / L.RidgeNoiseScale, V / L.RidgeNoiseScale, RidgeOff);
			const double WobbleAlongV = 1.0 + L.RidgeWobble * Fbm(V / 300.0, 3.3, 3, RidgeOff);
			const double WobbleAlongU = 1.0 + L.RidgeWobble * Fbm(U / 300.0, 7.7, 3, RidgeOff);
			// 모서리에서 두 산등성이가 겹치면 높은 쪽만(더하면 모서리가 지나치게 솟는다)
			double Ridge = 0.0;
			auto AddRidge = [&](double DistanceFromEdge, double Width, double Height)
			{
				if (Width > 0.0)
				{
					const double T = 1.0 - DistanceFromEdge / Width;
					if (T > 0.0)
					{
						Ridge = FMath::Max(Ridge, Height * FMath::Pow(Smooth01(T), 1.5) * RidgeNoise);
					}
				}
			};
			AddRidge(U, L.WestRidgeWidth * WobbleAlongV, L.WestRidgeHeight);
			AddRidge(MapH - V, L.NorthRidgeWidth * WobbleAlongU, L.NorthRidgeHeight);
			AddRidge(MapW - U, L.EastRidgeWidth * WobbleAlongV, L.EastRidgeHeight);
			H += Ridge;

			// 계곡
			double Distance = 0.0, Arc = 0.0;
			const double ValleyHalf = L.Valley.Width * 0.5;
			if (Valley.Closest(P, ValleyHalf, Distance, Arc))
			{
				H -= L.Valley.Depth * Smooth01(1.0 - Distance / ValleyHalf);
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

			Out.Heights[Out.Index(X, Y)] = static_cast<float>(H);
		}
	});

	// ── 2. 개울 물 높이(상류 → 하류로 내려가기만) ──
	TArray<FCreekProfile> Creeks;
	for (const FGrimTerrainChannel& Spec : L.Creeks)
	{
		FCreekProfile& Profile = Creeks.AddDefaulted_GetRef();
		Profile.Spec = &Spec;
		Profile.Line.Init(Spec.Points);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Profile.Line.Length() / Profile.Step));
		double Previous = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const FVector2D At = Profile.Line.AtArc(Index * Profile.Step);
			const double Ground = ReadHeight(Out, At.X, At.Y);
			const double Water = FMath::Max(FMath::Min(Previous - 0.004 * Profile.Step, Ground - Spec.WaterBelowGround), L.SeaLevel + 0.2);
			Profile.Water.Add(Water);
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
				for (const FCreekProfile& Creek : Creeks)
				{
					const double Half = Creek.Spec->Width * 0.5;
					double Distance = 0.0, Arc = 0.0;
					if (Creek.Line.Closest(P, Half + Creek.Spec->BankWidth, Distance, Arc))
					{
						H = static_cast<float>(CarveChannel(H, Distance, Creek.WaterAt(Arc), Creek.Spec->Depth, Half, Creek.Spec->BankWidth));
					}
				}
			}
		});
	}

	// ── 3. 연못 · 거점 평지 ──
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

	// ── 4. 무게(물가 · 길 · 계곡 숲) · 물과 길까지 거리 ──
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
			for (const FCreekProfile& Creek : Creeks)
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
				if (Paths[Index].Closest(P, FarDistance, Distance, Arc))
				{
					PathDistance = FMath::Min(PathDistance, Distance);
					PathWeight = FMath::Max(PathWeight, 1.f - SmoothStep(Half, Half + 1.5, Distance));
				}
			}
			Out.PathDistance[I] = static_cast<float>(PathDistance);
			Out.Path[I] = ToByte(PathWeight);

			const double ValleyHalf = L.Valley.Width * 0.5;
			if (Valley.Closest(P, ValleyHalf, Distance, Arc))
			{
				Out.Forest[I] = ToByte(1.f - SmoothStep(ValleyHalf * 0.55, ValleyHalf * 0.9, Distance));
			}
		}
	});

	// ── 5. 물 목록(월드 cm) ──
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
	for (const FCreekProfile& Creek : Creeks)
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
			Body.Points.Add(MapToWorld(L, At.X, At.Y, Creek.WaterAt(ArcAt)));
		}
		Body.Size = (Creek.Spec->Width + 0.6f) * 100.f;
		Body.Depth = Creek.Spec->Depth * 100.f;
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

			const int32 VX = FMath::Clamp(FMath::RoundToInt32(U / Maps.QuadSize), 0, Maps.SizeX - 1);
			const int32 VY = FMath::Clamp(FMath::RoundToInt32(V / Maps.QuadSize), 0, Maps.SizeY - 1);
			const int32 I = Maps.Index(VX, VY);
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
	return Summary;
}
