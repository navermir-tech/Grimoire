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

	/**
	 * 뒤튼 능선형 다중 프랙탈(레벨 디자인 2차) — 결과는 대략 −1~1(평균 0 근처).
	 * 저주파 노이즈 두 장으로 자리를 밀고(domain warp), 옥타브마다 (1 − |펄린|)² 을 앞 옥타브 값으로 가중한다(Musgrave).
	 * 등성이가 날카롭게 이어지고 골은 넓다 — 둥근 언덕을 흩뿌린 「달걀판」과 달리 결이 생긴다.
	 */
	float WarpedRidged(double X, double Y, double Warp, const FVector2D& Offset)
	{
		const double WX = X + Warp * Fbm(X * 0.5 + 5.2, Y * 0.5 + 1.3, 3, Offset);
		const double WY = Y + Warp * Fbm(X * 0.5 + 9.7, Y * 0.5 + 2.8, 3, Offset);
		double Sum = 0.0, Norm = 0.0, Weight = 1.0, Amplitude = 1.0, Frequency = 1.0;
		for (int32 Octave = 0; Octave < 4; ++Octave)
		{
			double Signal = 1.0 - FMath::Abs(Perlin(WX * Frequency + Octave * 19.1, WY * Frequency + Octave * 7.7, Offset));
			Signal *= Signal;
			Signal *= Weight;
			Weight = FMath::Clamp(Signal * 1.6, 0.0, 1.0);
			Sum += Signal * Amplitude;
			Norm += Amplitude;
			Amplitude *= 0.5;
			Frequency *= 2.1;
		}
		// 능선 노이즈 평균이 0.35~0.45 쯤이라 빼서 바닥 높이를 크게 옮기지 않는다
		return static_cast<float>((Norm > 0.0 ? Sum / Norm : 0.0) * 2.0 - 0.8);
	}

	/** 뒤튼 fBm(−1~1) — 숲 무늬 · 무리처럼 둥근 얼룩이 아니라 길쭉하고 불규칙한 무늬 */
	float WarpedFbm(double X, double Y, double Warp, int32 Octaves, const FVector2D& Offset)
	{
		const double WX = X + Warp * Fbm(X * 0.7 + 3.1, Y * 0.7 + 8.4, 2, Offset);
		const double WY = Y + Warp * Fbm(X * 0.7 + 6.6, Y * 0.7 + 4.2, 2, Offset);
		return Fbm(WX, WY, Octaves, Offset);
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
		/** 자연 개울(레벨 디자인 3차): 호 길이 Step 마다 물 반폭 · 소 정도(0~1) · 부호 있는 곡률(1/m) */
		bool bNatural = false;
		TArray<double> HalfWidth;
		TArray<double> Pool;
		TArray<double> Curve;

		static double SampleArray(const TArray<double>& Array, double Arc, double InStep)
		{
			if (Array.Num() == 0)
			{
				return 0.0;
			}
			const double F = FMath::Clamp(Arc / InStep, 0.0, static_cast<double>(Array.Num() - 1));
			const int32 I0 = FMath::FloorToInt32(F);
			const int32 I1 = FMath::Min(I0 + 1, Array.Num() - 1);
			return FMath::Lerp(Array[I0], Array[I1], F - I0);
		}
		double HalfAt(double Arc) const { return bNatural ? SampleArray(HalfWidth, Arc, Step) : (Spec ? Spec->Width * 0.5 : 0.0); }
		double PoolAt(double Arc) const { return SampleArray(Pool, Arc, Step); }
		double CurveAt(double Arc) const { return SampleArray(Curve, Arc, Step); }

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

	/** 부드러운 최솟값(K = 섞는 폭 m) — 골 옆면이 둘레 땅과 만나는 모서리를 둥글린다 */
	double SmoothMin(double A, double B, double K)
	{
		const double H = FMath::Clamp(0.5 + 0.5 * (B - A) / FMath::Max(K, 0.001), 0.0, 1.0);
		return FMath::Lerp(B, A, H) - K * H * (1.0 - H);
	}

	/** 물가선 반지름(각도마다) — Irregularity 가 0 이면 원 */
	double PondRadiusAt(const FGrimTerrainPond& Pond, const FVector2D& Offset, double Angle)
	{
		if (Pond.Irregularity <= 0.f)
		{
			return Pond.Radius;
		}
		const double C = FMath::Cos(Angle), S = FMath::Sin(Angle);
		const double Wave = Perlin(C * 1.3 + 4.0, S * 1.3 + 4.0, Offset) + 0.45 * Perlin(C * 3.1 + 9.0, S * 3.1 + 2.0, Offset);
		return Pond.Radius * (1.0 + Pond.Irregularity * Wave);
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
	const FVector2D RidgedOff = SeedOffset(L.Seed, 14);

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
				const float Aspect = FMath::Lerp(FMath::Max(1.f, Field.MinAspect), FMath::Max(FMath::Max(1.f, Field.MinAspect), Field.MaxAspect), Random.FRand());
				// 방향: 흩어짐 90° 이상이면 아무 방향(예전과 같은 난수 하나), 아니면 가운데 ± 흩어짐 — 줄지은 언덕 · 골 결
				const float YawRand = Random.FRand();
				const float Yaw = Field.YawSpread >= 90.f
					? YawRand * UE_PI
					: FMath::DegreesToRadians(Field.YawCenter + (YawRand * 2.f - 1.f) * Field.YawSpread);
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
	TArray<float> HillMax, HillSum, SmallSum, RidgeMax;
	RidgeMax.SetNumZeroed(Total);
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

	// 능선(레벨 디자인 2차) — 언덕과 같은 버퍼에 그린다. 높이는 점마다 준 값을 호 길이로 보간(낮은 점 = 안부), 단면은 언덕 실루엣, 양쪽 폭이 다르다
	const FVector2D RidgelineOff = SeedOffset(L.Seed, 13);
	for (int32 RidgeIndex = 0; RidgeIndex < L.Ridgelines.Num(); ++RidgeIndex)
	{
		const FGrimTerrainRidgeline& Spec = L.Ridgelines[RidgeIndex];
		if (Spec.Points.Num() < 2)
		{
			continue;
		}
		constexpr int32 Subdiv = 8;
		FPolyline Line;
		Line.Init(Spec.Points, Subdiv);
		// 원래 꺾은점 i 의 호 길이 = 부드럽게 한 점 i × Subdiv 의 누적 길이(점이 2 개면 그대로)
		TArray<double> KnotArc;
		const bool bSmoothed = Spec.Points.Num() >= 3;
		for (int32 Index = 0; Index < Spec.Points.Num(); ++Index)
		{
			KnotArc.Add(Line.Cumulative[FMath::Min(bSmoothed ? Index * Subdiv : Index, Line.Cumulative.Num() - 1)]);
		}
		auto CrestAt = [&](double Arc) -> double
		{
			if (Spec.Heights.Num() != Spec.Points.Num())
			{
				return Spec.Height;
			}
			for (int32 Index = 0; Index + 1 < KnotArc.Num(); ++Index)
			{
				if (Arc <= KnotArc[Index + 1] || Index + 2 == KnotArc.Num())
				{
					const double Span = KnotArc[Index + 1] - KnotArc[Index];
					const double T = Span > 0.0 ? FMath::Clamp((Arc - KnotArc[Index]) / Span, 0.0, 1.0) : 0.0;
					// 점 사이는 부드럽게(안부가 V 자로 꺾이지 않게)
					return FMath::Lerp(static_cast<double>(Spec.Heights[Index]), static_cast<double>(Spec.Heights[Index + 1]), static_cast<double>(Smooth01(T)));
				}
			}
			return Spec.Heights.Last();
		};
		const float K = Spec.Sharpness > 0.f ? Spec.Sharpness : L.HillSharpness;
		const double Vary = FMath::Clamp(Spec.Variation, 0.f, 0.8f);
		const double VaryScale = FMath::Max(Spec.VariationScale, 10.f);
		const FVector2D Off = RidgelineOff + FVector2D(RidgeIndex * 4.31, RidgeIndex * 2.77);
		const double Reach = FMath::Max(Spec.LeftWidth, Spec.RightWidth) * (1.0 + Vary) + Q + (Spec.bSharpCrest ? Spec.CrestWobble * 2.0 : 0.0);
		const int32 X0 = FMath::Clamp(FMath::FloorToInt32((Line.Bounds.Min.X - Reach) / Q), 0, Count.X - 1);
		const int32 X1 = FMath::Clamp(FMath::CeilToInt32((Line.Bounds.Max.X + Reach) / Q), 0, Count.X - 1);
		const int32 Y0 = FMath::Clamp(FMath::FloorToInt32((Line.Bounds.Min.Y - Reach) / Q), 0, Count.Y - 1);
		const int32 Y1 = FMath::Clamp(FMath::CeilToInt32((Line.Bounds.Max.Y + Reach) / Q), 0, Count.Y - 1);
		if (Spec.bSharpCrest)
		{
			// 레벨 디자인 3차 — 날카로운 등성이: 꼭대기에서도 기울기가 0 이 아닌 단면 (1 − t)^p, 양쪽 폭 · 지수가 달라 한쪽은 급사면 · 한쪽은 완사면.
			// 등성이 선을 흔들고(CrestWobble), 작은 톱니(CrestJag) · 비탈 골(지맥과 골이 번갈아) · 급사면 바위 띠(지층 턱)를 더한다.
			// 겹치면 큰 값만(RidgeMax) — 지맥이 주능선에 또렷이 붙는다.
			const FVector2D WarpA = Off + FVector2D(11.3, 4.9);
			const FVector2D WarpB = Off + FVector2D(2.7, 17.1);
			const double WobbleScale = FMath::Max(Spec.CrestWobbleScale, 5.f);
			const double JagScale = FMath::Max(Spec.CrestJagScale, 5.f);
			const double Spacing = FMath::Max(Spec.GullySpacing, 10.f);
			const double Band = FMath::Max(Spec.ScarpBandHeight, 1.f);
			ParallelFor(Y1 - Y0 + 1, [&, X0, X1, Y0](int32 Row)
			{
				const int32 Y = Y0 + Row;
				for (int32 X = X0; X <= X1; ++X)
				{
					const FVector2D P(X * Q, Y * Q);
					const FVector2D Warped = P + FVector2D(
						Spec.CrestWobble * (Perlin(P.X / WobbleScale, P.Y / WobbleScale, WarpA) + 0.5 * Perlin(P.X / (WobbleScale * 0.4), P.Y / (WobbleScale * 0.4), WarpB)),
						Spec.CrestWobble * (Perlin(P.X / WobbleScale, P.Y / WobbleScale, WarpB) + 0.5 * Perlin(P.X / (WobbleScale * 0.4), P.Y / (WobbleScale * 0.4), WarpA)));
					double Distance = 0.0, Arc = 0.0, Signed = 0.0;
					if (!Line.Closest(Warped, Reach, Distance, Arc, &Signed))
					{
						continue;
					}
					const bool bLeft = Signed >= 0.0;
					const double Width = (bLeft ? Spec.LeftWidth : Spec.RightWidth) * (1.0 + Vary * Perlin(Arc / VaryScale, bLeft ? 3.7 : 8.9, Off));
					const double Round = FMath::Max(Spec.CrestRound, 0.1f);
					const double Rounded = FMath::Sqrt(Distance * Distance + Round * Round) - Round;
					if (Width <= 1.0 || Rounded >= Width)
					{
						continue;
					}
					const double T = Rounded / Width;
					double Crest = CrestAt(Arc) * (1.0 + 0.5 * Vary * Perlin(Arc / (VaryScale * 0.6), 13.1, Off));
					// 작은 톱니 — 절댓값 노이즈라 봉우리가 뾰족하고 안부가 넓다
					Crest *= 1.0 + Spec.CrestJag * (1.0 - 2.4 * FMath::Abs(Perlin(Arc / JagScale, 21.3, Off)));
					double Value = FMath::Max(Crest, 0.0) * FMath::Pow(1.0 - T, static_cast<double>(FMath::Max(bLeft ? Spec.LeftPower : Spec.RightPower, 0.5f)));
					// 비탈의 골: 등성이 바로 밑과 밑동에서는 사라진다. 골 선은 내려가며 비스듬히 휜다
					if (Spec.GullyStrength > 0.f)
					{
						const double Flank = SmoothStep(0.06, 0.28, T) * (1.0 - SmoothStep(0.72, 1.0, T));
						const double Along = Arc / Spacing + 0.45 * Perlin(Arc / (Spacing * 2.3), T * 2.0 + (bLeft ? 1.1 : 6.4), WarpA) + T * 0.35;
						const double Groove = 1.0 - Smooth01(FMath::Abs(Perlin(Along, (bLeft ? 31.7 : 47.3) + T * 0.5, WarpB)) / 0.2);
						// 완사면의 골은 약하게 — 곡선 안쪽에서 부챗살처럼 모여 인공적으로 보인다
						const bool bScarpFace = Spec.ScarpSide != 0 && (bLeft ? 1 : -1) == Spec.ScarpSide;
						Value *= 1.0 - Spec.GullyStrength * (bScarpFace || Spec.ScarpSide == 0 ? 1.0 : 0.4) * Flank * Groove;
					}
					// 급사면 바위 띠: 높이를 층지게 — 벽(가파름 → 바위 재질)과 턱(풀)이 번갈아, 층 높이는 길이를 따라 흔들린다
					if (Spec.ScarpSide != 0 && (bLeft ? 1 : -1) == Spec.ScarpSide && Spec.ScarpBandStrength > 0.f)
					{
						// 군데군데만 바위 — 온 비탈이 벽이면 성벽처럼 보인다. 나머지는 풀 비탈
						const double Patch = SmoothStep(-0.12, 0.22, Perlin(Arc / 62.0, T * 1.4 + 5.5, WarpA) + 0.5 * Perlin(Arc / 21.0, T * 3.0 + 1.5, WarpB));
						const double Mask = Patch * SmoothStep(0.02, 0.14, T) * (1.0 - SmoothStep(0.55, 0.85, T));
						// 층 높이 · 위치를 길이를 따라 크게 흔든다(같은 높이 고리가 생기면 지구라트처럼 보인다)
						const double Wiggle = Band * (1.1 * Perlin(Arc / 40.0, 3.3, WarpB) + 0.35 * Perlin(Arc / 11.0, T * 6.0 + 7.9, WarpA));
						const double LocalBand = Band * (1.0 + 0.45 * Perlin(Arc / 70.0, 15.5, WarpA));
						const double Layer = (Value + Wiggle) / LocalBand;
						const double LayerFloor = FMath::FloorToDouble(Layer);
						const double Terraced = (LayerFloor + Smooth01((Layer - LayerFloor - 0.45) / 0.55)) * LocalBand - Wiggle;
						Value = FMath::Lerp(Value, Terraced, Spec.ScarpBandStrength * Mask);
					}
					const int32 I = Y * Count.X + X;
					RidgeMax[I] = FMath::Max(RidgeMax[I], static_cast<float>(Value));
				}
			});
			continue;
		}
		ParallelFor(Y1 - Y0 + 1, [&, X0, X1, Y0](int32 Row)
		{
			const int32 Y = Y0 + Row;
			for (int32 X = X0; X <= X1; ++X)
			{
				const FVector2D P(X * Q, Y * Q);
				double Distance = 0.0, Arc = 0.0, Signed = 0.0;
				if (!Line.Closest(P, Reach, Distance, Arc, &Signed))
				{
					continue;
				}
				const double Side = Signed >= 0.0 ? Spec.LeftWidth : Spec.RightWidth;
				const double Width = Side * (1.0 + Vary * Perlin(Arc / VaryScale, Signed >= 0.0 ? 3.7 : 8.9, Off));
				if (Width <= 1.0 || Distance >= Width)
				{
					continue;
				}
				const double Crest = CrestAt(Arc) * (1.0 + 0.6 * Vary * Perlin(Arc / (VaryScale * 0.6), 13.1, Off));
				const float Value = static_cast<float>(FMath::Max(Crest, 0.0) * FGrimTerrainGenerator::HillProfile(static_cast<float>(Distance / Width), K));
				if (Value > 0.f)
				{
					const int32 I = Y * Count.X + X;
					HillMax[I] = FMath::Max(HillMax[I], Value);
					HillSum[I] += Value;
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
			double RidgedAmp = 0.0, RidgedScale = 220.0, RidgedWarp = 0.6;
			double BumpMul = 1.0;
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
					// 둔턱 배수는 곱한다(덮는 순서와 상관없이 — 배수 1 인 뒤 구역이 지우지 않게)
					BumpMul *= FMath::Lerp(1.0, static_cast<double>(Spec.BumpMultiplier), Weight);
					// 잔물결은 덮는 순서와 상관없이 가장 센 구역 것 — 높이만 맡는 구역이 뒤에 와도 지워지지 않게
					if (Weight * Spec.ExtraRollingAmplitude > ExtraAmplitude)
					{
						ExtraAmplitude = Weight * Spec.ExtraRollingAmplitude;
						ExtraScale = FMath::Max(Spec.ExtraRollingScale, 5.f);
					}
					if (Weight * Spec.RidgedAmplitude > RidgedAmp)
					{
						RidgedAmp = Weight * Spec.RidgedAmplitude;
						RidgedScale = FMath::Max(Spec.RidgedScale, 10.f);
						RidgedWarp = Spec.WarpStrength;
					}
				}
			}
			if (ExtraAmplitude > 0.0)
			{
				H += ExtraAmplitude * Fbm(U / ExtraScale, V / ExtraScale, 3, ExtraOff);
			}
			if (RidgedAmp > 0.0)
			{
				H += RidgedAmp * WarpedRidged(U / RidgedScale, V / RidgedScale, RidgedWarp, RidgedOff);
			}

			H += L.NoiseAmplitude * Fbm(U / L.NoiseScale, V / L.NoiseScale, L.NoiseOctaves, BaseOff)
				+ L.RollingAmplitude * Fbm(U / L.RollingScale, V / L.RollingScale, 3, RollingOff)
				+ L.DetailAmplitude * Fbm(U / L.DetailScale, V / L.DetailScale, 3, DetailOff)
				+ BumpMul * L.BumpAmplitude * Fbm(U / FMath::Max(L.BumpScale, 1.f), V / FMath::Max(L.BumpScale, 1.f), 3, BumpOff);

			// 언덕 · 바위 노두
			H += FMath::Max(HillMax[I] + L.HillOverlapBlend * (HillSum[I] - HillMax[I]), RidgeMax[I]) + SmallSum[I];

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

	// ── 1d. 물방울 침식(레벨 디자인 2차) — 사각형 안에 물방울을 떨어뜨려 비탈을 따라 흘리며 깎고, 느려지면 쌓는다(Lague 2019 방식).
	// 결과: 비탈에 가지 친 물길, 골 바닥 · 언덕 밑동에 쌓인 부드러운 흙. 한 스레드로 고정 시드 — 다시 돌려도 같다.
	// 계곡 · 길 · 연못 · 탑 · 거점은 이 뒤에 깎고 고르므로 침식이 그 모양을 망치지 않는다.
	if (L.ErosionDropletsPerCell > 0.f && L.ErosionBoundsMax.X > L.ErosionBoundsMin.X && L.ErosionBoundsMax.Y > L.ErosionBoundsMin.Y)
	{
		const int32 R = FMath::Clamp(L.ErosionRadius, 1, 8);
		const int32 BX0 = FMath::Clamp(FMath::FloorToInt32(L.ErosionBoundsMin.X / Q), R + 1, Count.X - R - 2);
		const int32 BX1 = FMath::Clamp(FMath::CeilToInt32(L.ErosionBoundsMax.X / Q), R + 1, Count.X - R - 2);
		const int32 BY0 = FMath::Clamp(FMath::FloorToInt32(L.ErosionBoundsMin.Y / Q), R + 1, Count.Y - R - 2);
		const int32 BY1 = FMath::Clamp(FMath::CeilToInt32(L.ErosionBoundsMax.Y / Q), R + 1, Count.Y - R - 2);
		if (BX1 > BX0 + 2 && BY1 > BY0 + 2)
		{
			TArray<FIntPoint> BrushOffsets;
			TArray<float> BrushWeights;
			float WeightSum = 0.f;
			for (int32 DY = -R; DY <= R; ++DY)
			{
				for (int32 DX = -R; DX <= R; ++DX)
				{
					const float D = FMath::Sqrt(static_cast<float>(DX * DX + DY * DY));
					if (D < R)
					{
						BrushOffsets.Add(FIntPoint(DX, DY));
						BrushWeights.Add(R - D);
						WeightSum += R - D;
					}
				}
			}
			for (float& W : BrushWeights)
			{
				W /= WeightSum;
			}
			TArray<float>& Hm = Out.Heights;
			const int32 W = Count.X;
			auto HeightGradient = [&](double PX, double PY, double& OutGX, double& OutGY) -> double
			{
				const int32 NX = FMath::FloorToInt32(PX);
				const int32 NY = FMath::FloorToInt32(PY);
				const double FX = PX - NX, FY = PY - NY;
				const int32 I = NY * W + NX;
				const double HNW = Hm[I], HNE = Hm[I + 1], HSW = Hm[I + W], HSE = Hm[I + W + 1];
				OutGX = ((HNE - HNW) * (1.0 - FY) + (HSE - HSW) * FY) / Q;
				OutGY = ((HSW - HNW) * (1.0 - FX) + (HSE - HNE) * FX) / Q;
				return HNW * (1.0 - FX) * (1.0 - FY) + HNE * FX * (1.0 - FY) + HSW * (1.0 - FX) * FY + HSE * FX * FY;
			};
			const int64 Cells = static_cast<int64>(BX1 - BX0) * (BY1 - BY0);
			const int64 Droplets = static_cast<int64>(Cells * L.ErosionDropletsPerCell);
			FRandomStream Random(static_cast<int32>(HashCombine(GetTypeHash(L.Seed), 0x5E01u)));
			const double Inertia = 0.05, MinCapacity = 0.005, Evaporate = 0.025, Gravity = 4.0;
			const double Erode = FMath::Clamp(L.ErosionErodeSpeed, 0.f, 1.f);
			const double Deposit = FMath::Clamp(L.ErosionDepositSpeed, 0.f, 1.f);
			const double MaxStep = FMath::Max(L.ErosionMaxStep, 0.01f);
			const int32 Life = FMath::Clamp(L.ErosionLifetime, 4, 200);
			for (int64 Drop = 0; Drop < Droplets; ++Drop)
			{
				double PX = BX0 + Random.FRand() * (BX1 - BX0 - 1);
				double PY = BY0 + Random.FRand() * (BY1 - BY0 - 1);
				double DirX = 0.0, DirY = 0.0, Speed = 1.0, Water = 1.0, Sediment = 0.0;
				for (int32 Step = 0; Step < Life; ++Step)
				{
					const int32 NX = FMath::FloorToInt32(PX);
					const int32 NY = FMath::FloorToInt32(PY);
					const double FX = PX - NX, FY = PY - NY;
					double GX = 0.0, GY = 0.0;
					const double H0 = HeightGradient(PX, PY, GX, GY);
					DirX = DirX * Inertia - GX * (1.0 - Inertia);
					DirY = DirY * Inertia - GY * (1.0 - Inertia);
					const double Len = FMath::Sqrt(DirX * DirX + DirY * DirY);
					if (Len < 1e-6)
					{
						break;
					}
					DirX /= Len;
					DirY /= Len;
					PX += DirX;
					PY += DirY;
					if (PX < BX0 || PX >= BX1 - 1 || PY < BY0 || PY >= BY1 - 1)
					{
						break;
					}
					double Unused0 = 0.0, Unused1 = 0.0;
					const double DH = HeightGradient(PX, PY, Unused0, Unused1) - H0;
					const double Capacity = FMath::Max(-DH * Speed * Water * L.ErosionCapacity, MinCapacity);
					const int32 I = NY * W + NX;
					if (Sediment > Capacity || DH > 0.0)
					{
						// 오르막이면 구덩이를 메울 만큼, 아니면 넘친 만큼 쌓는다 — 지나온 칸 네 정점에 나눠
						const double Amount = DH > 0.0 ? FMath::Min(DH, Sediment) : (Sediment - Capacity) * Deposit;
						Sediment -= Amount;
						Hm[I] += static_cast<float>(Amount * (1.0 - FX) * (1.0 - FY));
						Hm[I + 1] += static_cast<float>(Amount * FX * (1.0 - FY));
						Hm[I + W] += static_cast<float>(Amount * (1.0 - FX) * FY);
						Hm[I + W + 1] += static_cast<float>(Amount * FX * FY);
					}
					else
					{
						const double Amount = FMath::Min(FMath::Min((Capacity - Sediment) * Erode, -DH), MaxStep);
						for (int32 B = 0; B < BrushOffsets.Num(); ++B)
						{
							Hm[(NY + BrushOffsets[B].Y) * W + NX + BrushOffsets[B].X] -= static_cast<float>(Amount * BrushWeights[B]);
						}
						Sediment += Amount;
					}
					Speed = FMath::Sqrt(FMath::Max(Speed * Speed - DH * Gravity, 0.0));
					Water *= 1.0 - Evaporate;
				}
			}
			UE_LOG(LogGrimTerrain, Verbose, TEXT("침식: 물방울 %lld 개, %d × %d 칸"), Droplets, BX1 - BX0, BY1 - BY0);
		}
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

	// ── 2b. 자연 개울(레벨 디자인 3차): 물 높이 곡선 · 폭 · 소 · 곡률을 정하고, 지형이 파인 V 자 골(상류) → 넓은 바닥(하류)을 먼저 판다.
	// 물길 자체는 길 · 연못을 다 고른 뒤(5b)에 판다 — 길이 다리 자리에서 둑을 쌓고, 연못 둑이 물길 입구를 막지 않게.
	TMap<FName, double> JoinedPondLevels;
	TArray<FLineProfile> NaturalCreeks;
	const FVector2D NaturalOff = SeedOffset(L.Seed, 15);
	for (int32 CreekIndex = 0; CreekIndex < L.Creeks.Num(); ++CreekIndex)
	{
		const FGrimTerrainChannel& Spec = L.Creeks[CreekIndex];
		if (!Spec.bNatural || !CreekLines[CreekIndex].IsValid())
		{
			continue;
		}
		if (!Spec.JoinPond.IsNone() && !JoinedPondLevels.Contains(Spec.JoinPond))
		{
			for (const FGrimTerrainPond& Pond : L.Ponds)
			{
				if (Pond.Name == Spec.JoinPond)
				{
					JoinedPondLevels.Add(Pond.Name, RingAverage(Out, Pond.Center, Pond.Radius + Pond.ShoreWidth, 48) - Pond.WaterBelowGround);
				}
			}
		}
		FLineProfile& Profile = NaturalCreeks.AddDefaulted_GetRef();
		Profile.Spec = &Spec;
		Profile.bNatural = true;
		Profile.Line = CreekLines[CreekIndex];
		Profile.Step = 2.0;
		const double Length = Profile.Line.Length();
		const int32 Steps = FMath::Max(2, FMath::CeilToInt32(Length / Profile.Step));
		TArray<double> Ground;
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const FVector2D At = Profile.Line.AtArc(Index * Profile.Step);
			Ground.Add(ReadHeight(Out, At.X, At.Y));
		}
		const TArray<double> GroundSmooth = MovingAverage(Ground, 4);
		const double StartLevel = Ground[0] - Spec.WaterBelowGround;
		const double* Joined = JoinedPondLevels.Find(Spec.JoinPond);
		// 강 · 바다로 흘러드는 끝은 물 높이를 해수면에 맞춘다(강바닥까지 파지 않게)
		const double EndLevel = Joined ? FMath::Min(*Joined, StartLevel - 0.5) : FMath::Max(FMath::Min(Ground.Last() - Spec.WaterBelowGround, StartLevel - 0.5), static_cast<double>(L.SeaLevel) + 0.02);
		const double MinDrop = 0.002 * Profile.Step;
		double Previous = StartLevel + MinDrop;
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double S = static_cast<double>(Index) / Steps;
			// 위로 가파르고 아래로 완만한 오목한 종단 곡선, 땅이 더 낮으면 땅을 따른다
			const double Designed = EndLevel + (StartLevel - EndLevel) * FMath::Pow(1.0 - S, static_cast<double>(FMath::Max(Spec.LongProfilePower, 0.1f)));
			double Level = FMath::Min(Designed, GroundSmooth[Index] - FMath::Lerp(static_cast<double>(Spec.ValleyDepthStart), static_cast<double>(Spec.ValleyDepthEnd), S));
			Level = FMath::Max(Level, EndLevel + (Steps - Index) * MinDrop);
			Level = FMath::Min(Level, Previous - MinDrop);
			Profile.Values.Add(Level);
			Previous = Level;
		}
		// 상류일수록 계단(여울 · 작은 턱) — 단조 감소는 그대로
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double S = static_cast<double>(Index) / Steps;
			// 턱은 2 m 안에 부드럽게 내려간다 — 물 표면 칸이 계단으로 끊겨 보이지 않게
			const double Over = Profile.Values[Index] / 0.45;
			const double Stair = (FMath::FloorToDouble(Over) + Smooth01((Over - FMath::FloorToDouble(Over) - 0.55) / 0.45)) * 0.45;
			Profile.Values[Index] = FMath::Lerp(Profile.Values[Index], FMath::Max(Stair, EndLevel), 0.2 * (1.0 - S));
		}
		// 물 높이 곡선을 고른다(단조 감소인 값의 이동 평균은 단조 감소) — 물 표면이 작은 턱으로 끊겨 보이지 않게
		Profile.Values = MovingAverage(Profile.Values, 4);
		Profile.Values.Last() = EndLevel;
		// 폭 · 소 · 곡률
		const double WidthEnd = Spec.WidthEnd > 0.f ? Spec.WidthEnd : Spec.Width;
		const double PoolWave = FMath::Max(Spec.PoolSpacingWidths, 2.f) * FMath::Max((Spec.Width + WidthEnd) * 0.5, 0.5);
		const FVector2D Off = NaturalOff + FVector2D(CreekIndex * 5.13, CreekIndex * 3.37);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double Arc = Index * Profile.Step;
			const double S = static_cast<double>(Index) / Steps;
			// 소: 간격이 들쭉날쭉한 파동의 마루
			const double Phase = 2.0 * UE_DOUBLE_PI * (Arc / PoolWave + 0.5 * Perlin(Arc / (PoolWave * 2.1), 3.3, Off));
			// 소마다 크기가 다르다(일부는 거의 없다) — 같은 크기가 이어지면 하늘에서 구슬 꿴 줄처럼 보인다
			const double PoolSize = SmoothStep(-0.35, 0.4, Perlin(Arc / (PoolWave * 1.4), 11.7, Off));
			double Pool = PoolSize * SmoothStep(0.45, 0.92, 0.5 + 0.5 * FMath::Sin(Phase));
			double Half = 0.5 * FMath::Lerp(static_cast<double>(Spec.Width), WidthEnd, S) * (1.0 + 0.3 * Perlin(Arc / 13.0, 7.7, Off) + 0.15 * Perlin(Arc / 5.0, 2.9, Off)) * (1.0 + 0.32 * Pool);
			if (Spec.HeadPoolRadius > 0.f)
			{
				const double HeadR = Spec.HeadPoolRadius;
				const double Head = FMath::Exp(-FMath::Square(Arc / (HeadR * 1.1)));
				Half = FMath::Max(Half, HeadR * Head);
				Pool = FMath::Max(Pool, Head);
			}
			Profile.HalfWidth.Add(Half);
			Profile.Pool.Add(Pool);
			const FVector2D A = Profile.Line.AtArc(Arc - 5.0);
			const FVector2D B = Profile.Line.AtArc(Arc);
			const FVector2D C = Profile.Line.AtArc(Arc + 5.0);
			const FVector2D D1 = (B - A).GetSafeNormal();
			const FVector2D D2 = (C - B).GetSafeNormal();
			Profile.Curve.Add((D1.X * D2.Y - D1.Y * D2.X) / 5.0);
		}
		Profile.Curve = MovingAverage(Profile.Curve, 3);

		const double SlopeReach = 90.0;
		const TArray<float> Before = Out.Heights;
		ParallelFor(Count.Y, [&](int32 Y)
		{
			for (int32 X = 0; X < Count.X; ++X)
			{
				const FVector2D P(X * Q, Y * Q);
				double Distance = 0.0, Arc = 0.0, Signed = 0.0;
				if (!Profile.Line.Closest(P, SlopeReach, Distance, Arc, &Signed))
				{
					continue;
				}
				const double S = Arc / FMath::Max(Length, 1.0);
				const double Floor = Profile.ValueAt(Arc) + 0.35;
				const double Curvature = Profile.CurveAt(Arc);
				const bool bOuter = Signed * Curvature < 0.0;
				const double Bend = FMath::Clamp(FMath::Abs(Curvature) * 14.0, 0.0, 1.0);
				// 굽이 바깥은 좁고 가파른 깎인 둑, 안쪽은 넓고 완만한 모래톱
				const double FloorHalf = FMath::Lerp(static_cast<double>(Spec.ValleyHalfStart), static_cast<double>(Spec.ValleyHalfEnd), S)
					* (1.0 + 0.35 * Perlin(Arc / 30.0, Signed >= 0.0 ? 2.1 : 5.3, Off)) * (bOuter ? 1.0 - 0.5 * Bend : 1.0 + 0.6 * Bend);
				const double SlopeDeg = FMath::Lerp(static_cast<double>(Spec.ValleySlopeStart), static_cast<double>(Spec.ValleySlopeEnd), FMath::Pow(S, 0.8))
					* (1.0 + 0.18 * Perlin(Arc / 24.0, Signed >= 0.0 ? 9.2 : 12.6, Off));
				const double Tan = FMath::Tan(FMath::DegreesToRadians(SlopeDeg)) * (bOuter ? 1.0 + 0.35 * Bend : 1.0 - 0.3 * Bend);
				const double Beyond = FMath::Max(0.0, Distance - FloorHalf);
				const double Target = Floor + Beyond * Tan + 0.004 * Beyond * Beyond;
				const double H0 = Before[Out.Index(X, Y)];
				double H1 = SmoothMin(H0, Target, 1.6);
				// 바닥은 땅이 물보다 낮아도 메운다(물길이 끊기지 않게)
				if (Distance < FloorHalf)
				{
					H1 = FMath::Lerp(Floor, H1, static_cast<double>(SmoothStep(FloorHalf * 0.6, FloorHalf, Distance)));
				}
				Out.Heights[Out.Index(X, Y)] = static_cast<float>(FMath::Lerp(H1, H0, static_cast<double>(SmoothStep(SlopeReach * 0.8, SlopeReach, Distance))));
			}
		});
	}

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

		// 레벨 디자인 3차: 자연 개울을 건너는 자리 — 실제 곡선끼리의 교점에서 다리 자리를 계산하고, 그 구간 노면을 상판 높이로 평평하게 고정한다(양쪽은 경사 상한으로 잇는다)
		{
			TArray<bool> Locked;
			Locked.SetNumZeroed(Steps + 1);
			bool bAnyLock = false;
			const TArray<FVector2D>& PP = Profile.Line.Points;
			for (const FLineProfile& Creek : NaturalCreeks)
			{
				const TArray<FVector2D>& CP = Creek.Line.Points;
				for (int32 I = 0; I + 1 < PP.Num(); ++I)
				{
					for (int32 J = 0; J + 1 < CP.Num(); ++J)
					{
						const FVector2D A = PP[I], B = PP[I + 1], C = CP[J], D = CP[J + 1];
						const FVector2D AB = B - A, CD = D - C;
						const double Den = AB.X * CD.Y - AB.Y * CD.X;
						if (FMath::Abs(Den) < 1e-9)
						{
							continue;
						}
						const FVector2D AC = C - A;
						const double T = (AC.X * CD.Y - AC.Y * CD.X) / Den;
						const double U2 = (AC.X * AB.Y - AC.Y * AB.X) / Den;
						if (T < 0.0 || T >= 1.0 || U2 < 0.0 || U2 >= 1.0)
						{
							continue;
						}
						const FVector2D Hit = A + AB * T;
						const double PathArc = Profile.Line.Cumulative[I] + T * AB.Size();
						const double CreekArc = Creek.Line.Cumulative[J] + U2 * CD.Size();
						const FVector2D Dir = (Profile.Line.AtArc(PathArc + 2.0) - Profile.Line.AtArc(PathArc - 2.0)).GetSafeNormal();
						const FVector2D CreekDir = (Creek.Line.AtArc(CreekArc + 2.0) - Creek.Line.AtArc(CreekArc - 2.0)).GetSafeNormal();
						const double CreekHalf = Creek.HalfAt(CreekArc);
						const double Span = FMath::Max(CreekHalf * 2.0 + 6.0, 9.0);
						const double Water = Creek.ValueAt(CreekArc);
						const FVector2D BankA = Hit - Dir * (Span * 0.5 + 1.0);
						const FVector2D BankB = Hit + Dir * (Span * 0.5 + 1.0);
						const double Deck = FMath::Max3(static_cast<double>(ReadHeight(Out, BankA.X, BankA.Y)), static_cast<double>(ReadHeight(Out, BankB.X, BankB.Y)), Water + 1.6);
						for (int32 Index = 0; Index <= Steps; ++Index)
						{
							if (FMath::Abs(Index * Profile.Step - PathArc) <= Span * 0.5 + 2.5)
							{
								Profile.Values[Index] = Deck;
								Locked[Index] = true;
								bAnyLock = true;
							}
						}
						FGrimTerrainBridge& Bridge = Out.Bridges.AddDefaulted_GetRef();
						Bridge.Name = *FString::Printf(TEXT("Bridge_%s_%s"), *Profile.Spec->Name.ToString(), *Creek.Spec->Name.ToString());
						Bridge.PathName = Profile.Spec->Name;
						Bridge.CreekName = Creek.Spec->Name;
						Bridge.Center = MapToWorld(L, Hit.X, Hit.Y, Deck);
						// 지도 (동, 북) → 월드 (X = 북, Y = 동)
						Bridge.Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Dir.X, Dir.Y)));
						Bridge.Span = static_cast<float>(Span * 100.0);
						Bridge.Width = (Profile.Spec->Width + 0.8f) * 100.f;
						Bridge.WaterZ = static_cast<float>(Water * 100.0);
						Bridge.CrossingAngle = static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FMath::Abs(Dir | CreekDir), 0.0, 1.0))));
					}
				}
			}
			if (bAnyLock)
			{
				for (int32 Index = 1; Index <= Steps; ++Index)
				{
					if (!Locked[Index])
					{
						Profile.Values[Index] = FMath::Clamp(Profile.Values[Index], Profile.Values[Index - 1] - Rise, Profile.Values[Index - 1] + Rise);
					}
				}
				for (int32 Index = Steps - 1; Index >= 0; --Index)
				{
					if (!Locked[Index])
					{
						Profile.Values[Index] = FMath::Clamp(Profile.Values[Index], Profile.Values[Index + 1] - Rise, Profile.Values[Index + 1] + Rise);
					}
				}
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
		if (Spec.bNatural)
		{
			continue;	// 자연 개울은 2b · 5b
		}
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
	const FVector2D PondOff = SeedOffset(L.Seed, 16);
	for (const FGrimTerrainPond& Pond : L.Ponds)
	{
		const double* Joined = JoinedPondLevels.Find(Pond.Name);
		PondLevels.Add(Joined ? *Joined : RingAverage(Out, Pond.Center, Pond.Radius + Pond.ShoreWidth, 48) - Pond.WaterBelowGround);
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
				if (R >= Pond.Radius * (1.0 + 1.6 * Pond.Irregularity) + Pond.ShoreWidth + PondBlend)
				{
					continue;
				}
				const double Radius = PondRadiusAt(Pond, PondOff + FVector2D(Index * 2.3, Index * 1.7), FMath::Atan2(P.Y - Pond.Center.Y, P.X - Pond.Center.X));
				if (R < Radius)
				{
					H = Level - Pond.Depth * (1.0 - FMath::Square(R / Radius));
				}
				else if (R < Radius + Pond.ShoreWidth)
				{
					H = Level + 0.9 * (R - Radius) / Pond.ShoreWidth;
				}
				else if (R < Radius + Pond.ShoreWidth + PondBlend)
				{
					H = FMath::Lerp(Rim, H, static_cast<double>(Smooth01((R - Radius - Pond.ShoreWidth) / PondBlend)));
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

	// ── 5b. 자연 개울 물길: 길 · 연못 · 거점 뒤에 판다 — 다리 밑으로 흐르고, 연못 둑을 뚫고 물이 그대로 이어진다.
	// 소는 넓고 깊게, 여울은 얕게, 굽이에서는 물길이 바깥쪽으로 쏠린다.
	for (const FLineProfile& Creek : NaturalCreeks)
	{
		const double MaxHalf = FMath::Max(Creek.HalfWidth);
		ParallelFor(Count.Y, [&](int32 Y)
		{
			for (int32 X = 0; X < Count.X; ++X)
			{
				const FVector2D P(X * Q, Y * Q);
				double Distance = 0.0, Arc = 0.0, Signed = 0.0;
				if (!Creek.Line.Closest(P, MaxHalf + 8.0, Distance, Arc, &Signed))
				{
					continue;
				}
				const double Half = Creek.HalfAt(Arc);
				const double Pool = Creek.PoolAt(Arc);
				const double Shift = FMath::Clamp(Creek.CurveAt(Arc) * 14.0, -1.0, 1.0) * 0.28 * Half;
				const double D = FMath::Abs(Signed + Shift);
				const double Bank = FMath::Max(1.8, Half * 0.9);
				const double Depth = FMath::Lerp(0.28, static_cast<double>(Creek.Spec->PoolDepth), Pool);
				float& H = Out.Heights[Out.Index(X, Y)];
				const double Water = Creek.ValueAt(Arc);
				double NewH = CarveChannel(H, D, Water, Depth, Half, Bank);
				// 둑: 둘레 땅이 물보다 낮은 곳은 둑을 쌓아 물을 가둔다 — 물 표면이 땅 위에 떠 끊기지 않게
				if (D >= Half && D < Half + Bank * 1.6 && H < Water + 0.3)
				{
					NewH = FMath::Lerp(Water + 0.3, static_cast<double>(H), static_cast<double>(Smooth01((D - Half) / (Bank * 1.6))));
				}
				H = static_cast<float>(NewH);
			}
		});
	}
	Creeks.Append(NaturalCreeks);

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
			float WetBed = 0.f;

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
				if (Creek.bNatural)
				{
					if (Creek.Line.Closest(P, 16.0 + FarDistance, Distance, Arc))
					{
						const double NaturalHalf = Creek.HalfAt(Arc);
						WaterDistance = FMath::Min(WaterDistance, Distance - NaturalHalf);
						// 자갈 · 모래톱: 소 · 굽이 안쪽에서 넓어지고 여울에서는 풀이 물가까지 온다
						const double Bend = FMath::Clamp(FMath::Abs(Creek.CurveAt(Arc)) * 14.0, 0.0, 1.0);
						const double Bar = 0.1 + 0.55 * Creek.PoolAt(Arc) * Bend + 0.35 * Bend;
						Shore = FMath::Max(Shore, 1.f - SmoothStep(NaturalHalf + Bar * 0.3, NaturalHalf + Bar + 0.6, Distance));
						// 물길 바닥은 젖은 어두운 흙(숲 바닥 레이어) — 밝은 모래 띠로 보이지 않게
						WetBed = FMath::Max(WetBed, 1.f - SmoothStep(NaturalHalf - 0.3, NaturalHalf + 0.5, Distance));
					}
					continue;
				}
				const double Half = Creek.Spec->Width * 0.5;
				if (Creek.Line.Closest(P, Half + FarDistance, Distance, Arc))
				{
					WaterDistance = FMath::Min(WaterDistance, Distance - Half);
					Shore = FMath::Max(Shore, 1.f - SmoothStep(Half + 1.0, Half + 2.5, Distance));
				}
			}
			for (int32 PondIndex = 0; PondIndex < L.Ponds.Num(); ++PondIndex)
			{
				const FGrimTerrainPond& Pond = L.Ponds[PondIndex];
				const double R = FVector2D::Distance(P, Pond.Center);
				const double Angle = FMath::Atan2(V - Pond.Center.Y, U - Pond.Center.X);
				const double Radius = PondRadiusAt(Pond, PondOff + FVector2D(PondIndex * 2.3, PondIndex * 1.7), Angle);
				WaterDistance = FMath::Min(WaterDistance, R - Radius);
				// 자연 연못의 모래는 둘레가 끊겼다 이어진다(갈대 · 풀이 물가까지 오는 곳)
				const double SandWidth = Pond.Irregularity > 0.f
					? Pond.ShoreWidth * (0.04 + 0.4 * FMath::Max(0.0, static_cast<double>(Perlin(FMath::Cos(Angle) * 2.3 + 3.0, FMath::Sin(Angle) * 2.3 + 8.0, PondOff))))
					: Pond.ShoreWidth * 0.5;
				const double Sand = Radius + SandWidth;
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
			if (WetBed > 0.f)
			{
				Out.Forest[I] = FMath::Max(Out.Forest[I], ToByte(WetBed));
				Out.Shore[I] = ToByte(FMath::Min(Out.Shore[I] / 255.f, 1.f - WetBed));
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
		Body.bSurfaceMesh = Pond.Irregularity > 0.f;
		if (Body.bSurfaceMesh)
		{
			FGrimNaturalPondData& Data = Out.NaturalPonds.AddDefaulted_GetRef();
			Data.Name = Pond.Name;
			Data.Center = Pond.Center;
			Data.Level = static_cast<float>(PondLevels[Index]);
			for (int32 Slice = 0; Slice < FGrimNaturalPondData::AngleCount; ++Slice)
			{
				Data.Radii.Add(static_cast<float>(PondRadiusAt(Pond, PondOff + FVector2D(Index * 2.3, Index * 1.7), 2.0 * UE_DOUBLE_PI * Slice / FGrimNaturalPondData::AngleCount)));
			}
		}
	}
	for (const FLineProfile& Creek : Creeks)
	{
		FGrimTerrainWaterBody& Body = Out.Water.AddDefaulted_GetRef();
		Body.Kind = TEXT("Creek");
		Body.Name = Creek.Spec->Name;
		const double Step = Creek.bNatural ? 2.0 : 12.0;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Creek.Line.Length() / Step));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const double ArcAt = FMath::Min(Index * Step, Creek.Line.Length());
			const FVector2D At = Creek.Line.AtArc(ArcAt);
			Body.Points.Add(MapToWorld(L, At.X, At.Y, Creek.ValueAt(ArcAt)));
			if (Creek.bNatural)
			{
				Body.Widths.Add(static_cast<float>(Creek.HalfAt(ArcAt) * 200.0));
				Body.Pools.Add(static_cast<float>(Creek.PoolAt(ArcAt)));
			}
		}
		Body.Size = (Creek.Spec->Width + 0.6f) * 100.f;
		Body.Depth = Creek.Spec->Depth * 100.f;
		if (Creek.bNatural)
		{
			Body.bSurfaceMesh = true;
			FGrimNaturalCreekData& Data = Out.NaturalCreeks.AddDefaulted_GetRef();
			Data.Name = Creek.Spec->Name;
			for (double ArcAt = 0.0; ArcAt <= Creek.Line.Length(); ArcAt += 1.0)
			{
				Data.Points.Add(Creek.Line.AtArc(ArcAt));
				Data.Levels.Add(static_cast<float>(Creek.ValueAt(ArcAt)));
				Data.Halves.Add(static_cast<float>(Creek.HalfAt(ArcAt)));
			}
			continue;	// 자연 개울은 폭포를 만들지 않는다
		}

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

float FGrimTerrainGenerator::ForestWeight(const FGrimTerrainLayout& L, double U, double V)
{
	using namespace GrimTerrainGenLocal;
	const FVector2D ForestOff = SeedOffset(L.Seed, 6);
	const double Scale = FMath::Max(L.ForestNoiseScale, 1.f);
	// 뒤틀기 0 · 부드러움 0 이면 1 · 2차와 같은 칼 경계(같은 노이즈)
	const double Noise = L.ForestWarp > 0.f
		? WarpedFbm(U / Scale, V / Scale, L.ForestWarp, 3, ForestOff)
		: Perlin(U / Scale, V / Scale, ForestOff);
	if (L.ForestSoftness <= 0.f)
	{
		return Noise > L.ForestThreshold ? 1.f : 0.f;
	}
	return SmoothStep(L.ForestThreshold - L.ForestSoftness, L.ForestThreshold + L.ForestSoftness, Noise);
}

void FGrimTerrainGenerator::ScatterFoliage(const FGrimTerrainLayout& L, const FGrimTerrainMaps& Maps, const FGrimTerrainFoliageRule& Rule, TArray<FTransform>& OutWorld, TArray<float>* OutVariant)
{
	using namespace GrimTerrainGenLocal;

	OutWorld.Reset();
	if (OutVariant)
	{
		OutVariant->Reset();
	}
	if (!Maps.IsValid())
	{
		return;
	}
	const double MaxDensity = FMath::Max(FMath::Max(Rule.DensityPerHa, Rule.ForestDensityPerHa), FMath::Max(Rule.ValleyDensityPerHa, Rule.SteepDensityPerHa));
	if (MaxDensity <= 0.0)
	{
		return;
	}
	// 배수(무리 · 가장자리 · 등성이 등)가 1 을 넘을 수 있어 후보를 넉넉히(가장 큰 배수만큼) 뽑고 확률로 고른다
	const double MaxBoost = FMath::Max(1.0, static_cast<double>(FMath::Max3(FMath::Max(Rule.ForestEdgeFactor, Rule.WaterEdgeFactor), FMath::Max(Rule.RidgeFactor, Rule.ValleyFactor), FMath::Max(Rule.SlopeFactor, Rule.PathEdgeFactor))))
		* (Rule.ForestPatchScale > 0.f ? 1.6 : 1.0);
	const double Candidates = MaxDensity * MaxBoost;
	const double Cell = 100.0 / FMath::Sqrt(Candidates);
	const double MapW = (Maps.SizeX - 1) * Maps.QuadSize;
	const double MapH = (Maps.SizeY - 1) * Maps.QuadSize;
	const int32 CellsX = FMath::FloorToInt32(MapW / Cell);
	const int32 CellsY = FMath::FloorToInt32(MapH / Cell);
	const FVector2D ForestOff = SeedOffset(L.Seed, 6);
	const FVector2D ClumpOff = SeedOffset(L.Seed, 20 + Rule.ClumpChannel);
	const FVector2D PatchOff = SeedOffset(L.Seed, 30 + Rule.ClumpChannel);
	const uint32 RuleHash = HashCombine(GetTypeHash(L.Seed), GetTypeHash(Rule.Name.ToString()));
	const FVector2D VariantOff = SeedOffset(L.Seed, 40 + static_cast<int32>(RuleHash % 97u));
	// 「2차 방식」(새 값이 모두 기본값)이면 예전과 같은 지터 격자 · 칼 경계 그대로 — 테스트용 작은 레이아웃이 그대로 돈다
	const bool bNatural = Rule.MinSpacing > 0.f || Rule.ClumpScale > 0.f || L.ForestSoftness > 0.f;
	const double Spacing = Rule.MinSpacing > 0.f ? Rule.MinSpacing : (bNatural ? 45.0 / FMath::Sqrt(MaxDensity) : 0.0);

	// 먼저 뿌린 규칙 자리(바위가 나무 줄기 속에 들지 않게) — 한 단계만(그 규칙의 AvoidRules 는 보지 않는다)
	TArray<FVector2D> Avoid;
	for (const FName& Other : Rule.AvoidRules)
	{
		const FGrimTerrainFoliageRule* OtherRule = L.Foliage.FindByPredicate([&Other](const FGrimTerrainFoliageRule& Each) { return Each.Name == Other; });
		if (OtherRule && OtherRule->Name != Rule.Name)
		{
			FGrimTerrainFoliageRule Plain = *OtherRule;
			Plain.AvoidRules.Reset();
			TArray<FTransform> Placed;
			ScatterFoliage(L, Maps, Plain, Placed);
			for (const FTransform& Each : Placed)
			{
				Avoid.Add(WorldToMap(L, FVector2D(Each.GetLocation())));
			}
		}
	}

	// 포아송 원판: 받아들인 자리를 격자 칸(간격 크기)에 넣어 둘레만 본다
	const double HashCell = FMath::Max(Spacing, Rule.AvoidDistance > 0.f ? static_cast<double>(Rule.AvoidDistance) : 0.0);
	TMap<FIntPoint, TArray<FVector2D>> Accepted;
	TMap<FIntPoint, TArray<FVector2D>> AvoidGrid;
	auto CellOf = [HashCell](const FVector2D& P) { return FIntPoint(FMath::FloorToInt32(P.X / HashCell), FMath::FloorToInt32(P.Y / HashCell)); };
	if (HashCell > 0.0)
	{
		for (const FVector2D& P : Avoid)
		{
			AvoidGrid.FindOrAdd(CellOf(P)).Add(P);
		}
	}
	auto TooClose = [&](const TMap<FIntPoint, TArray<FVector2D>>& Grid, const FVector2D& P, double Distance)
	{
		if (Distance <= 0.0 || Grid.Num() == 0)
		{
			return false;
		}
		const FIntPoint C = CellOf(P);
		const int32 Reach = FMath::CeilToInt32(Distance / HashCell);
		for (int32 DY = -Reach; DY <= Reach; ++DY)
		{
			for (int32 DX = -Reach; DX <= Reach; ++DX)
			{
				if (const TArray<FVector2D>* Bucket = Grid.Find(C + FIntPoint(DX, DY)))
				{
					for (const FVector2D& Q : *Bucket)
					{
						if (FVector2D::DistSquared(P, Q) < Distance * Distance)
						{
							return true;
						}
					}
				}
			}
		}
		return false;
	};

	const double CurvR = FMath::Max(L.CurvatureRadius, 2.f);
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
			const float TiltA = Random.FRandRange(-1.f, 1.f);
			const float TiltB = Random.FRandRange(-1.f, 1.f);
			const float VariantJitter = Random.FRandRange(-0.12f, 0.12f);

			const int32 I = Maps.NearestIndex(U, V);
			const double Slope = SampleSlopeDeg(Maps, U, V);

			// 바탕 밀도: 계곡 숲 > 숲 무늬(부드러운 경계) > 초원
			double Density = Rule.DensityPerHa;
			double Grove = 0.0;   // 0~1: 숲 · 무리 안쪽일수록 큼(크기 · 가장자리 덤불)
			double ForestW = 0.0;
			if (Maps.Forest[I] > 128)
			{
				Density = Rule.ValleyDensityPerHa;
				Grove = 1.0;
			}
			else if (bNatural)
			{
				ForestW = ForestWeight(L, U, V);
				Density = FMath::Lerp(static_cast<double>(Rule.DensityPerHa), static_cast<double>(Rule.ForestDensityPerHa), ForestW);
				Grove = ForestW;
			}
			else if (Perlin(U / L.ForestNoiseScale, V / L.ForestNoiseScale, ForestOff) > L.ForestThreshold)
			{
				Density = Rule.ForestDensityPerHa;
			}
			if (bNatural)
			{
				// 초원 무리(숲덩이 · 빈터): 숲 밖에서만 크게 — 숲 안은 이미 빽빽
				if (Rule.ClumpScale > 0.f)
				{
					const double Clump = SmoothStep(Rule.ClumpThreshold - Rule.ClumpSoftness, Rule.ClumpThreshold + Rule.ClumpSoftness,
						WarpedFbm(U / Rule.ClumpScale, V / Rule.ClumpScale, 0.8, 3, ClumpOff));
					Density *= FMath::Lerp(FMath::Lerp(static_cast<double>(Rule.ClumpOutside), 1.0, Clump), 1.0, ForestW);
					Grove = FMath::Max(Grove, Clump);
				}
				// 숲 안 덩이: 몇 그루씩 모이고 작은 빈터 — 덩이 쪽이 더 빽빽하고 크다
				if (Rule.ForestPatchScale > 0.f && ForestW > 0.0)
				{
					const double Patch = SmoothStep(-0.2, 0.3, WarpedFbm(U / Rule.ForestPatchScale, V / Rule.ForestPatchScale, 0.7, 2, PatchOff));
					Density *= FMath::Lerp(1.0, FMath::Lerp(static_cast<double>(Rule.ForestPatchMin), 1.6, Patch), ForestW);
					Grove = FMath::Lerp(Grove, Grove * (0.35 + 0.65 * Patch), ForestW);
				}
				// 숲 가장자리(무게 0.5 언저리)
				Density *= 1.0 + (Rule.ForestEdgeFactor - 1.0) * 4.0 * ForestW * (1.0 - ForestW);
				// 등성이 · 골: 둘레(반지름 CurvR) 평균 높이 − 가운데 — 음수면 볼록(등성이), 양수면 오목(골)
				if (!FMath::IsNearlyEqual(Rule.RidgeFactor, 1.f) || !FMath::IsNearlyEqual(Rule.ValleyFactor, 1.f))
				{
					const double Center = ReadHeight(Maps, U, V);
					double Ring = 0.0;
					for (int32 K = 0; K < 8; ++K)
					{
						const double A = K * UE_DOUBLE_PI / 4.0;
						Ring += ReadHeight(Maps, U + CurvR * FMath::Cos(A), V + CurvR * FMath::Sin(A));
					}
					const double Curv = (Ring / 8.0 - Center) / CurvR;
					Density *= FMath::Lerp(1.0, static_cast<double>(Rule.RidgeFactor), static_cast<double>(SmoothStep(0.015, 0.08, -Curv)));
					Density *= FMath::Lerp(1.0, static_cast<double>(Rule.ValleyFactor), static_cast<double>(SmoothStep(0.015, 0.08, Curv)));
				}
				// 비탈(SlopeFactorDeg 언저리)
				if (!FMath::IsNearlyEqual(Rule.SlopeFactor, 1.f))
				{
					Density *= FMath::Lerp(1.0, static_cast<double>(Rule.SlopeFactor), static_cast<double>(SmoothStep(Rule.SlopeFactorDeg - 8.0, Rule.SlopeFactorDeg, Slope)));
				}
				// 길 가장자리
				if (!FMath::IsNearlyEqual(Rule.PathEdgeFactor, 1.f))
				{
					const double D = Maps.PathDistance[I] - Rule.MinPathDistance;
					const double Edge = D >= 0.0 && D < Rule.PathEdgeDistance ? FMath::Sin(UE_DOUBLE_PI * D / FMath::Max(Rule.PathEdgeDistance, 1.f)) : 0.0;
					Density *= 1.0 + (Rule.PathEdgeFactor - 1.0) * Edge;
				}
				// 물가
				if (!FMath::IsNearlyEqual(Rule.WaterEdgeFactor, 1.f))
				{
					const double D = Maps.WaterDistance[I] - Rule.MinWaterDistance;
					const double Edge = D >= 0.0 && D < Rule.WaterEdgeDistance ? 1.0 - SmoothStep(Rule.WaterEdgeDistance * 0.4, Rule.WaterEdgeDistance, D) : 0.0;
					Density *= 1.0 + (Rule.WaterEdgeFactor - 1.0) * Edge;
				}
				// 경사 상한은 부드럽게(상한 6° 앞부터 줄어든다)
				Density *= 1.0 - SmoothStep(Rule.MaxSlopeDeg - 6.0, Rule.MaxSlopeDeg, Slope);
			}
			const bool bSteep = Rule.SteepDensityPerHa > 0.f && Slope >= Rule.SteepSlopeDeg;
			if (bSteep)
			{
				Density = FMath::Max(Density, static_cast<double>(Rule.SteepDensityPerHa));
			}
			if (Pick >= Density / Candidates)
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
			if (bBlocked || TooClose(Accepted, P, Spacing) || TooClose(AvoidGrid, P, Rule.AvoidDistance))
			{
				continue;
			}
			if (HashCell > 0.0)
			{
				Accepted.FindOrAdd(CellOf(P)).Add(P);
			}

			// 크기: 숲 · 무리 가운데일수록 크다
			const float SizeT = FMath::Clamp(ScaleT + Rule.GroveScaleBoost * static_cast<float>(Grove - 0.5), 0.f, 1.f);
			const float Scale = FMath::Lerp(Rule.MinScale, Rule.MaxScale, SizeT);
			FVector Location = MapToWorld(L, U, V, Height);
			Location.Z -= Rule.SinkCm * Scale;

			// 방향: Yaw + 막 기울임 + 비탈에 맞춤
			FQuat Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
			if (Rule.MaxTiltDeg > 0.f)
			{
				Rotation = FQuat(FRotator(TiltA * Rule.MaxTiltDeg, 0.f, TiltB * Rule.MaxTiltDeg)) * Rotation;
			}
			if (Rule.AlignToSlope > 0.f)
			{
				const double D = Maps.QuadSize * 1.5;
				const double GU = (ReadHeight(Maps, U + D, V) - ReadHeight(Maps, U - D, V)) / (2.0 * D);
				const double GV = (ReadHeight(Maps, U, V + D) - ReadHeight(Maps, U, V - D)) / (2.0 * D);
				// 지도 (U 동, V 북) → 월드 (X 북, Y 동): 법선 = (−GV, −GU, 1)
				const FVector Normal = FVector(-GV, -GU, 1.0).GetSafeNormal();
				const FVector Up = FMath::Lerp(FVector::UpVector, Normal, FMath::Clamp(Rule.AlignToSlope, 0.f, 1.f)).GetSafeNormal();
				Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Up) * Rotation;
			}
			OutWorld.Add(FTransform(Rotation, Location, FVector(Scale)));
			if (OutVariant)
			{
				const float Variant = 0.5f + 0.5f * WarpedFbm(U / FMath::Max(Rule.VariantScale, 5.f), V / FMath::Max(Rule.VariantScale, 5.f), 0.6, 2, VariantOff) * 1.6f;
				OutVariant->Add(FMath::Clamp(Variant + VariantJitter, 0.f, 0.999f));
			}
		}
	}
}

bool FGrimTerrainGenerator::SampleNaturalWater(const FGrimTerrainMaps& Maps, const TArray<FName>& Bodies, double U, double V, double Margin, double& OutLevel, double& OutArc, double& OutSigned, double& OutFlow)
{
	const FVector2D P(U, V);
	for (const FGrimNaturalPondData& Pond : Maps.NaturalPonds)
	{
		if (!Bodies.Contains(Pond.Name))
		{
			continue;
		}
		const FVector2D D = P - Pond.Center;
		const double R = D.Size();
		const double A = FMath::Fmod(FMath::Atan2(D.Y, D.X) + 2.0 * UE_DOUBLE_PI, 2.0 * UE_DOUBLE_PI) / (2.0 * UE_DOUBLE_PI) * FGrimNaturalPondData::AngleCount;
		const int32 I0 = FMath::FloorToInt32(A) % FGrimNaturalPondData::AngleCount;
		const int32 I1 = (I0 + 1) % FGrimNaturalPondData::AngleCount;
		const double Radius = FMath::Lerp(static_cast<double>(Pond.Radii[I0]), static_cast<double>(Pond.Radii[I1]), A - FMath::FloorToDouble(A));
		if (R < Radius + Margin)
		{
			OutLevel = Pond.Level;
			OutArc = U;
			OutSigned = V;
			OutFlow = 0.0;
			return true;
		}
	}
	double Best = TNumericLimits<double>::Max();
	bool bFound = false;
	for (const FGrimNaturalCreekData& Creek : Maps.NaturalCreeks)
	{
		if (!Bodies.Contains(Creek.Name))
		{
			continue;
		}
		for (int32 Index = 0; Index < Creek.Points.Num(); ++Index)
		{
			const double D2 = FVector2D::DistSquared(P, Creek.Points[Index]);
			const double Reach = Creek.Halves[Index] + Margin;
			if (D2 < Reach * Reach && D2 < Best)
			{
				Best = D2;
				OutLevel = Creek.Levels[Index];
				OutArc = Index;
				const FVector2D Dir = (Creek.Points[FMath::Min(Index + 1, Creek.Points.Num() - 1)] - Creek.Points[FMath::Max(Index - 1, 0)]).GetSafeNormal();
				const FVector2D Q = P - Creek.Points[Index];
				OutSigned = Dir.X * Q.Y - Dir.Y * Q.X;
				OutFlow = 1.0;
				bFound = true;
			}
		}
	}
	return bFound;
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
	Summary.Bridges = Maps.Bridges;
	for (const FGrimTerrainPlacedHill& Hill : Maps.PlacedHills)
	{
		Hill.bSmallShape ? ++Summary.OutcropCount : ++Summary.HillCount;
	}
	return Summary;
}
