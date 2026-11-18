#include "World/GrimZoneTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogGrimZone);

namespace GrimZoneTypesLocal
{
	bool ReadPoint(const TSharedPtr<FJsonObject>& Object, FVector2D& Out)
	{
		double X = 0.0, Y = 0.0;
		if (!Object.IsValid() || !Object->TryGetNumberField(TEXT("X"), X) || !Object->TryGetNumberField(TEXT("Y"), Y))
		{
			return false;
		}
		Out = FVector2D(X, Y);
		return true;
	}

	FText ReadText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		return Object->TryGetStringField(Field, Value) ? FText::FromString(Value) : FText::GetEmpty();
	}
}

int32 FGrimZoneDatabase::IndexOf(FName Name) const
{
	return Zones.IndexOfByPredicate([Name](const FGrimZoneDef& Zone) { return Zone.Name == Name; });
}

const FGrimZoneDef* FGrimZoneDatabase::Find(FName Name) const
{
	const int32 Index = IndexOf(Name);
	return Zones.IsValidIndex(Index) ? &Zones[Index] : nullptr;
}

bool FGrimZoneMath::ParseJson(const FString& Text, FGrimZoneDatabase& Out, FString& OutError)
{
	using namespace GrimZoneTypesLocal;
	Out = FGrimZoneDatabase();
	TArray<FString> Problems;

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = FString::Printf(TEXT("JSON 을 읽지 못함: %s"), *Reader->GetErrorMessage());
		return false;
	}

	const TSharedPtr<FJsonObject>* Origin = nullptr;
	if (!Root->TryGetObjectField(TEXT("WorldOrigin"), Origin) || !ReadPoint(*Origin, Out.WorldOrigin))
	{
		Problems.Add(TEXT("WorldOrigin 이 없다"));
	}

	const TArray<TSharedPtr<FJsonValue>>* Sections = nullptr;
	if (!Root->TryGetArrayField(TEXT("Sections"), Sections))
	{
		OutError = TEXT("Sections 가 없다");
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *Sections)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			Problems.Add(TEXT("Sections 안에 객체가 아닌 값"));
			continue;
		}
		FGrimZoneDef Zone;
		FString Name, Tier;
		if (!Object->TryGetStringField(TEXT("Name"), Name) || Name.IsEmpty())
		{
			Problems.Add(TEXT("Name 이 없는 구역"));
			continue;
		}
		Zone.Name = FName(*Name);
		if (Out.IndexOf(Zone.Name) != INDEX_NONE)
		{
			Problems.Add(FString::Printf(TEXT("%s: 이름이 겹친다"), *Name));
			continue;
		}
		Object->TryGetStringField(TEXT("Tier"), Tier);
		if (Tier == TEXT("Region"))
		{
			Zone.Tier = EGrimZoneTier::Region;
		}
		else if (Tier == TEXT("Sector"))
		{
			Zone.Tier = EGrimZoneTier::Sector;
		}
		else
		{
			Problems.Add(FString::Printf(TEXT("%s: Tier 「%s」 — Region 또는 Sector"), *Name, *Tier));
			continue;
		}
		Zone.Label = ReadText(Object, TEXT("Label"));
		Zone.Subtitle = ReadText(Object, TEXT("Subtitle"));
		Zone.Note = ReadText(Object, TEXT("Note"));
		if (Zone.Label.IsEmpty())
		{
			Zone.Label = FText::FromName(Zone.Name);
		}
		const TSharedPtr<FJsonObject>* Center = nullptr;
		if (!Object->TryGetObjectField(TEXT("Center"), Center) || !ReadPoint(*Center, Zone.Center))
		{
			Problems.Add(FString::Printf(TEXT("%s: Center 가 없다"), *Name));
		}
		double Radius = 0.0;
		if (Object->TryGetNumberField(TEXT("Radius"), Radius))
		{
			Zone.Radius = static_cast<float>(FMath::Max(0.0, Radius));
		}
		const TArray<TSharedPtr<FJsonValue>>* Polygon = nullptr;
		if (Object->TryGetArrayField(TEXT("Polygon"), Polygon))
		{
			for (const TSharedPtr<FJsonValue>& PointValue : *Polygon)
			{
				FVector2D Point;
				if (PointValue.IsValid() && ReadPoint(PointValue->AsObject(), Point))
				{
					Zone.Polygon.Add(Point);
				}
			}
		}
		if (Zone.Tier == EGrimZoneTier::Region && Zone.Polygon.Num() < 3)
		{
			Problems.Add(FString::Printf(TEXT("%s: 큰 구역은 Polygon(꼭짓점 3 이상)이 있어야 한다"), *Name));
		}
		Out.Zones.Add(MoveTemp(Zone));
	}

	// 소구역의 큰 구역 = Center 를 가진 큰 구역
	for (FGrimZoneDef& Zone : Out.Zones)
	{
		if (Zone.Tier != EGrimZoneTier::Sector)
		{
			continue;
		}
		for (const FGrimZoneDef& Region : Out.Zones)
		{
			if (Region.Tier == EGrimZoneTier::Region && IsPointInPolygon(Zone.Center, Region.Polygon))
			{
				Zone.Parent = Region.Name;
				break;
			}
		}
		if (Zone.Parent.IsNone())
		{
			Problems.Add(FString::Printf(TEXT("%s: Center 가 어느 큰 구역에도 들지 않는다 — 나타나지 않는다"), *Zone.Name.ToString()));
		}
	}

	OutError = FString::Join(Problems, TEXT(" / "));
	return Problems.Num() == 0;
}

bool FGrimZoneMath::IsPointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
	const int32 Count = Polygon.Num();
	if (Count < 3)
	{
		return false;
	}
	bool bInside = false;
	for (int32 i = 0, j = Count - 1; i < Count; j = i++)
	{
		const FVector2D& A = Polygon[i];
		const FVector2D& B = Polygon[j];
		if ((A.Y > Point.Y) != (B.Y > Point.Y))
		{
			const double CrossX = A.X + (Point.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y);
			if (Point.X < CrossX)
			{
				bInside = !bInside;
			}
		}
	}
	return bInside;
}

FGrimZoneHit FGrimZoneMath::FindAt(const FGrimZoneDatabase& Database, const FVector2D& MapPoint)
{
	FGrimZoneHit Hit;
	for (const FGrimZoneDef& Zone : Database.Zones)
	{
		if (Zone.Tier == EGrimZoneTier::Region && IsPointInPolygon(MapPoint, Zone.Polygon))
		{
			Hit.Region = Zone.Name;
			break;
		}
	}
	if (Hit.Region.IsNone())
	{
		return Hit;
	}
	double BestDistSq = TNumericLimits<double>::Max();
	for (const FGrimZoneDef& Zone : Database.Zones)
	{
		if (Zone.Tier != EGrimZoneTier::Sector || Zone.Parent != Hit.Region)
		{
			continue;
		}
		const double DistSq = FVector2D::DistSquared(MapPoint, Zone.Center);
		if (Zone.Radius > 0.f && DistSq > FMath::Square(static_cast<double>(Zone.Radius)))
		{
			continue;
		}
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Hit.Sector = Zone.Name;
		}
	}
	return Hit;
}

FVector2D FGrimZoneMath::WorldToMap(const FGrimZoneDatabase& Database, const FVector& WorldLocation)
{
	return FVector2D(WorldLocation.Y / 100.0 + Database.WorldOrigin.X, WorldLocation.X / 100.0 + Database.WorldOrigin.Y);
}

FVector FGrimZoneMath::MapToWorld(const FGrimZoneDatabase& Database, const FVector2D& MapPoint, double WorldZ)
{
	return FVector((MapPoint.Y - Database.WorldOrigin.Y) * 100.0, (MapPoint.X - Database.WorldOrigin.X) * 100.0, WorldZ);
}

// ───────────────────────── 들어섬 판단 ─────────────────────────

bool FGrimZoneVisitTracker::Step(const FGrimZoneHit& Hit, double Now, double StableSeconds, double RevisitCooldown, TArray<FGrimZoneEntered>& OutEntered)
{
	if (!bHasPending || Hit != Pending)
	{
		Pending = Hit;
		PendingSince = Now;
		bHasPending = true;
	}
	if (Pending == Current || Now - PendingSince < StableSeconds)
	{
		return false;
	}
	const FGrimZoneHit Previous = Current;
	Current = Pending;
	bool bChanged = false;
	if (Current.Region != Previous.Region && !Current.Region.IsNone())
	{
		Enter(Current.Region, Now, RevisitCooldown, OutEntered, bChanged);
	}
	if (Current.Sector != Previous.Sector && !Current.Sector.IsNone())
	{
		Enter(Current.Sector, Now, RevisitCooldown, OutEntered, bChanged);
	}
	return bChanged;
}

void FGrimZoneVisitTracker::Enter(FName Zone, double Now, double RevisitCooldown, TArray<FGrimZoneEntered>& OutEntered, bool& bOutChanged)
{
	const bool bFirst = !Discovered.Contains(Zone);
	if (bFirst)
	{
		Discovered.Add(Zone);
		bOutChanged = true;
	}
	else if (const double* Last = LastAnnounced.Find(Zone); Last && Now - *Last < RevisitCooldown)
	{
		return;
	}
	LastAnnounced.Add(Zone, Now);
	OutEntered.Add({ Zone, bFirst });
}

bool FGrimZoneVisitTracker::MergeDiscovered(const TArray<FName>& Names)
{
	bool bChanged = false;
	for (const FName& Name : Names)
	{
		if (!Name.IsNone() && !Discovered.Contains(Name))
		{
			Discovered.Add(Name);
			bChanged = true;
		}
	}
	return bChanged;
}

void FGrimZoneVisitTracker::Reset()
{
	*this = FGrimZoneVisitTracker();
}
