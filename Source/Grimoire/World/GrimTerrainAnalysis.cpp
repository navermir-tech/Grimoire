#include "World/GrimTerrainAnalysis.h"

namespace GrimTerrainAnalysisLocal
{
	bool IsLand(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, double U, double V)
	{
		const int32 I = Maps.NearestIndex(U, V);
		return Maps.Heights[I] > Layout.SeaLevel + 0.5f && Maps.WaterDistance[I] > 0.5f;
	}
}

const FGrimTerrainWaterBody* FGrimTerrainAnalysis::NearestFalls(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FVector2D& MapCenter)
{
	const FGrimTerrainWaterBody* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (const FGrimTerrainWaterBody& Water : Maps.Water)
	{
		if (Water.Kind == TEXT("Falls") && Water.Points.Num() >= 2)
		{
			const double Distance = FVector2D::Distance(FGrimTerrainGenerator::WorldToMap(Layout, FVector2D(Water.Points[0])), MapCenter);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = &Water;
			}
		}
	}
	return Best;
}

FString FGrimTerrainAnalysisResult::ToString() const
{
	FString Text = FString::Printf(TEXT("경사(샘플 %d): 5~25° %.1f%%, 평지(0~3°) %.1f%%, 3~5° %.1f%%, 25° 넘음 %.1f%%\n"),
		SlopeSamples, RollingFraction * 100.0, FlatFraction * 100.0, GentleFraction * 100.0, SteepFraction * 100.0);
	Text += TEXT("구역 평균 높이:");
	for (int32 Index = 0; Index < ZoneNames.Num(); ++Index)
	{
		Text += FString::Printf(TEXT(" %s %.1f m"), *ZoneNames[Index].ToString(), ZoneMeans[Index]);
		if (ZoneSteps.IsValidIndex(Index))
		{
			Text += FString::Printf(TEXT(" →(차 %.1f)"), ZoneSteps[Index]);
		}
	}
	Text += FString::Printf(TEXT("\n시선: 길 샘플 %d 곳, 보이는 POI · 랜드마크 최소 %d · 평균 %.1f, 모자란 곳 %d\n"), PathSamples, MinVisible, MeanVisible, SightFailures);
	Text += FString::Printf(TEXT("밀도: 길 위 가장 먼 POI 거리 %.0f m (%.0f, %.0f), 넘는 곳 %d\n"), MaxPoiDistance, MaxPoiDistanceAt.X, MaxPoiDistanceAt.Y, DensityFailures);
	Text += FString::Printf(TEXT("길: 길이 %.0f m, 가장 가파른 경사 %.1f° (%.0f, %.0f)"), PathLength, MaxPathGradeDeg, MaxPathGradeAt.X, MaxPathGradeAt.Y);
	return Text;
}

bool FGrimTerrainAnalysis::IsVisible(const FGrimTerrainMaps& Maps, const FVector& Eye, const FVector& Target, double SampleStep)
{
	const FVector Delta = Target - Eye;
	const double Distance = FVector2D(Delta.X, Delta.Y).Size();
	if (Distance < 1.0)
	{
		return true;
	}
	const double Step = FMath::Max(SampleStep, 0.5);
	for (double Along = Step; Along < Distance - 3.0; Along += Step)
	{
		const FVector At = Eye + Delta * (Along / Distance);
		if (FGrimTerrainGenerator::SampleHeight(Maps, At.X, At.Y) > At.Z)
		{
			return false;
		}
	}
	return true;
}

FVector FGrimTerrainAnalysis::LandmarkTop(const FGrimTerrainMaps& Maps, const FGrimTerrainLandmark& Landmark)
{
	return FVector(Landmark.Center.X, Landmark.Center.Y, FGrimTerrainGenerator::SampleHeight(Maps, Landmark.Center.X, Landmark.Center.Y) + Landmark.Height);
}

FVector FGrimTerrainAnalysis::PoiTop(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FGrimTerrainPoi& Poi)
{
	return FVector(Poi.Center.X, Poi.Center.Y, FGrimTerrainGenerator::SampleHeight(Maps, Poi.Center.X, Poi.Center.Y) + Layout.Analysis.PoiViewHeight);
}

TArray<FName> FGrimTerrainAnalysis::VisibleFrom(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, double U, double V, double EyeHeight, double FloorHeight)
{
	TArray<FName> Seen;
	const FVector Eye(U, V, FMath::Max(static_cast<double>(FGrimTerrainGenerator::SampleHeight(Maps, U, V)), FloorHeight) + EyeHeight);
	for (const FGrimTerrainLandmark& Landmark : Layout.Landmarks)
	{
		const FVector Top = LandmarkTop(Maps, Landmark);
		if (FVector2D::Distance(FVector2D(Eye), FVector2D(Top)) > Landmark.ViewDistance)
		{
			continue;
		}
		bool bSeen = IsVisible(Maps, Eye, Top);
		// 폭포는 위 끝에서 아래 끝까지 흰 띠 — 띠의 어느 부분(위 · 가운데 · 아래)이 보여도 보인다
		if (!bSeen && Landmark.Kind == TEXT("Waterfall"))
		{
			if (const FGrimTerrainWaterBody* Falls = NearestFalls(Layout, Maps, Landmark.Center))
			{
				const FVector2D A = FGrimTerrainGenerator::WorldToMap(Layout, FVector2D(Falls->Points[0]));
				const FVector2D B = FGrimTerrainGenerator::WorldToMap(Layout, FVector2D(Falls->Points[1]));
				const FVector TopPoint(A.X, A.Y, Falls->Points[0].Z / 100.0 + 2.0);
				const FVector BottomPoint(B.X, B.Y, Falls->Points[1].Z / 100.0 + 3.0);
				bSeen = IsVisible(Maps, Eye, (TopPoint + BottomPoint) * 0.5) || IsVisible(Maps, Eye, BottomPoint);
			}
		}
		if (bSeen)
		{
			Seen.Add(Landmark.Name);
		}
	}
	for (const FGrimTerrainPoi& Poi : Layout.Pois)
	{
		const FVector Top = PoiTop(Layout, Maps, Poi);
		if (FVector2D::Distance(FVector2D(Eye), FVector2D(Top)) <= Layout.Analysis.PoiViewDistance && IsVisible(Maps, Eye, Top))
		{
			Seen.Add(Poi.Name);
		}
	}
	return Seen;
}

FGrimTerrainAnalysisResult FGrimTerrainAnalysis::Run(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps)
{
	using namespace GrimTerrainAnalysisLocal;

	FGrimTerrainAnalysisResult Result;
	if (!Maps.IsValid())
	{
		return Result;
	}
	const FGrimTerrainAnalysisSpec& Spec = Layout.Analysis;

	// 1. 경사 분포
	{
		int64 Flat = 0, Gentle = 0, Rolling = 0, Steep = 0;
		const double Step = FMath::Max(Spec.SlopeSampleStep, 1.f);
		for (double V = Spec.BoundsMin.Y; V <= Spec.BoundsMax.Y; V += Step)
		{
			for (double U = Spec.BoundsMin.X; U <= Spec.BoundsMax.X; U += Step)
			{
				if (!IsLand(Layout, Maps, U, V))
				{
					continue;
				}
				const float Slope = FGrimTerrainGenerator::SampleSlopeDeg(Maps, U, V);
				if (Slope <= Spec.FlatMaxDeg)
				{
					++Flat;
				}
				else if (Slope < Spec.RollingMinDeg)
				{
					++Gentle;
				}
				else if (Slope <= Spec.RollingMaxDeg)
				{
					++Rolling;
				}
				else
				{
					++Steep;
				}
			}
		}
		const double All = static_cast<double>(FMath::Max<int64>(Flat + Gentle + Rolling + Steep, 1));
		Result.SlopeSamples = static_cast<int32>(Flat + Gentle + Rolling + Steep);
		Result.FlatFraction = Flat / All;
		Result.GentleFraction = Gentle / All;
		Result.RollingFraction = Rolling / All;
		Result.SteepFraction = Steep / All;
	}

	// 2. 구역 평균 높이 — 그 구역 다각형 안이고 뒤 구역(덮는 쪽) 다각형 밖인 땅
	for (const FName& ZoneName : Spec.ZoneSteps)
	{
		const int32 ZoneIndex = Layout.Zones.IndexOfByPredicate([&](const FGrimTerrainZone& Zone) { return Zone.Name == ZoneName; });
		double Sum = 0.0;
		int64 Samples = 0;
		if (ZoneIndex != INDEX_NONE)
		{
			const FGrimTerrainZone& Zone = Layout.Zones[ZoneIndex];
			FBox2D Box(ForceInit);
			for (const FVector2D& Point : Zone.Polygon)
			{
				Box += Point;
			}
			for (double V = Box.Min.Y; V <= Box.Max.Y; V += 8.0)
			{
				for (double U = Box.Min.X; U <= Box.Max.X; U += 8.0)
				{
					const FVector2D P(U, V);
					if (FGrimTerrainGenerator::PolygonSignedDistance(Zone.Polygon, P) < 0.0 || !IsLand(Layout, Maps, U, V))
					{
						continue;
					}
					bool bCovered = false;
					for (int32 Later = ZoneIndex + 1; Later < Layout.Zones.Num() && !bCovered; ++Later)
					{
						bCovered = FGrimTerrainGenerator::PolygonSignedDistance(Layout.Zones[Later].Polygon, P) >= 0.0;
					}
					if (!bCovered)
					{
						Sum += FGrimTerrainGenerator::SampleHeight(Maps, U, V);
						++Samples;
					}
				}
			}
		}
		Result.ZoneNames.Add(ZoneName);
		Result.ZoneMeans.Add(Samples > 0 ? Sum / Samples : 0.0);
	}
	for (int32 Index = 0; Index + 1 < Result.ZoneMeans.Num(); ++Index)
	{
		Result.ZoneSteps.Add(FMath::Abs(Result.ZoneMeans[Index + 1] - Result.ZoneMeans[Index]));
	}

	// 3 · 4 · 길 경사
	Result.MinVisible = TNumericLimits<int32>::Max();
	double VisibleSum = 0.0;
	for (const FGrimTerrainPathLine& Line : Maps.PathLines)
	{
		if (Line.Points.Num() < 2)
		{
			continue;
		}
		Result.PathLength += Line.Length;
		const double PointStep = Line.Length / (Line.Points.Num() - 1);

		// 시선 — PathSampleStep 마다(끝점 포함)
		const int32 Every = FMath::Max(1, FMath::RoundToInt32(Spec.PathSampleStep / FMath::Max(PointStep, 0.01)));
		for (int32 Index = 0; Index < Line.Points.Num(); Index += Every)
		{
			const FVector& P = Line.Points[Index];
			// 길 높이(P.Z) 위에 선다 — 개울을 건너는 곳은 다리 노면이다
			const int32 Seen = VisibleFrom(Layout, Maps, P.X, P.Y, Spec.EyeHeight, P.Z).Num();
			++Result.PathSamples;
			VisibleSum += Seen;
			Result.MinVisible = FMath::Min(Result.MinVisible, Seen);
			if (Seen < Spec.MinVisible)
			{
				++Result.SightFailures;
				Result.SightFailPoints.Add(FVector2D(P.X, P.Y));
			}
		}

		// 밀도 — 10 m 마다 가장 가까운 POI
		const int32 DensityEvery = FMath::Max(1, FMath::RoundToInt32(10.0 / FMath::Max(PointStep, 0.01)));
		for (int32 Index = 0; Index < Line.Points.Num(); Index += DensityEvery)
		{
			const FVector2D P(Line.Points[Index].X, Line.Points[Index].Y);
			double Nearest = TNumericLimits<double>::Max();
			for (const FGrimTerrainPoi& Poi : Layout.Pois)
			{
				Nearest = FMath::Min(Nearest, FVector2D::Distance(P, Poi.Center));
			}
			if (Nearest > Result.MaxPoiDistance)
			{
				Result.MaxPoiDistance = Nearest;
				Result.MaxPoiDistanceAt = P;
			}
			Result.DensityFailures += Nearest > Spec.PoiMaxDistance ? 1 : 0;
		}

		// 길 경사 — 다 만든 땅에서 4 m 간격. 개울을 건너는 여울(물가 6 m 안)은 다리 자리라 뺀다
		const int32 GradeEvery = FMath::Max(1, FMath::RoundToInt32(4.0 / FMath::Max(PointStep, 0.01)));
		for (int32 Index = GradeEvery; Index < Line.Points.Num(); Index += GradeEvery)
		{
			const FVector& A = Line.Points[Index - GradeEvery];
			const FVector& B = Line.Points[Index];
			if (Maps.WaterDistance[Maps.NearestIndex(A.X, A.Y)] < 6.f || Maps.WaterDistance[Maps.NearestIndex(B.X, B.Y)] < 6.f)
			{
				continue;
			}
			const double Run = FVector2D::Distance(FVector2D(A), FVector2D(B));
			const double RiseM = FGrimTerrainGenerator::SampleHeight(Maps, B.X, B.Y) - FGrimTerrainGenerator::SampleHeight(Maps, A.X, A.Y);
			const double Deg = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(RiseM), FMath::Max(Run, 0.01)));
			if (Deg > Result.MaxPathGradeDeg)
			{
				Result.MaxPathGradeDeg = Deg;
				Result.MaxPathGradeAt = FVector2D(B.X, B.Y);
			}
		}
	}
	if (Result.PathSamples == 0)
	{
		Result.MinVisible = 0;
	}
	Result.MeanVisible = Result.PathSamples > 0 ? VisibleSum / Result.PathSamples : 0.0;
	return Result;
}
