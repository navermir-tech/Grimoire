#include "World/GrimTerrainEditorLibrary.h"

#include "World/GrimTerrainAnalysis.h"
#include "World/GrimTerrainGenerator.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ActorFactories/ActorFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Builders/CubeBuilder.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "GameFramework/Volume.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"
#include "LandscapeSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionRuntimeHash.h"
#endif

namespace GrimTerrainEditorLocal
{
	/** 파이썬 한 번 실행에서 만든 지형 — GenerateTerrain 이 채운다 */
	struct FCache
	{
		bool bValid = false;
		FGrimTerrainLayout Layout;
		FGrimTerrainMaps Maps;
	};

	FCache& Cache()
	{
		static FCache Instance;
		return Instance;
	}

	bool RequireCache(const TCHAR* Caller)
	{
		if (!Cache().bValid)
		{
			UE_LOG(LogGrimTerrain, Error, TEXT("%s: GenerateTerrain 을 먼저 불러야 한다"), Caller);
			return false;
		}
		return true;
	}

	FString ResolvePath(const FString& Path)
	{
		return FPaths::IsRelative(Path) ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path) : Path;
	}

	/** 생성기 순서(지도 U 가 빠름) → 랜드스케이프 순서(월드 X = 지도 V 가 빠름) */
	template <typename T>
	TArray<T> ToLandscapeOrder(const TArray<T>& Source, int32 SizeU, int32 SizeV)
	{
		TArray<T> Result;
		Result.SetNumUninitialized(Source.Num());
		for (int32 V = 0; V < SizeV; ++V)
		{
			for (int32 U = 0; U < SizeU; ++U)
			{
				Result[U * SizeV + V] = Source[V * SizeU + U];
			}
		}
		return Result;
	}

	const TArray<uint8>* WeightsByLayerName(const FGrimTerrainMaps& Maps, FName LayerName)
	{
		if (LayerName == TEXT("Shore")) { return &Maps.Shore; }
		if (LayerName == TEXT("Path")) { return &Maps.Path; }
		if (LayerName == TEXT("Forest")) { return &Maps.Forest; }
		return nullptr;
	}

#if WITH_EDITOR
	/** 있으면 그 에셋, 없으면 새 패키지에 만든다(저장은 파이썬이 한다) */
	template <typename T>
	T* FindOrCreateAsset(const FString& PackagePath, bool& bOutCreated)
	{
		bOutCreated = false;
		const FString ObjectName = FPackageName::GetLongPackageAssetName(PackagePath);
		if (T* Existing = LoadObject<T>(nullptr, *(PackagePath + TEXT(".") + ObjectName), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Existing;
		}
		UPackage* Package = CreatePackage(*PackagePath);
		T* Asset = NewObject<T>(Package, *ObjectName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		bOutCreated = true;
		return Asset;
	}
#endif
}

bool UGrimTerrainEditorLibrary::LoadLayoutFromJson(const FString& JsonPath, FGrimTerrainLayout& OutLayout)
{
	FString Text;
	const FString FullPath = GrimTerrainEditorLocal::ResolvePath(JsonPath);
	if (!FFileHelper::LoadFileToString(Text, *FullPath))
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("레이아웃 JSON 을 읽지 못했다: %s"), *FullPath);
		return false;
	}
	OutLayout = FGrimTerrainLayout();
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Text, &OutLayout, 0, 0))
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("레이아웃 JSON 형식 오류: %s"), *FullPath);
		return false;
	}
	FString Error;
	if (!FGrimTerrainGenerator::Validate(OutLayout, Error))
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("레이아웃 값 오류(%s): %s"), *FullPath, *Error);
		return false;
	}
	return true;
}

bool UGrimTerrainEditorLibrary::GenerateTerrain(const FGrimTerrainLayout& Layout, FGrimTerrainSummary& OutSummary)
{
	FString Error;
	if (!FGrimTerrainGenerator::Validate(Layout, Error))
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("GenerateTerrain: %s"), *Error);
		return false;
	}
	GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const double Start = FPlatformTime::Seconds();
	Cache.Layout = Layout;
	FGrimTerrainGenerator::Generate(Layout, Cache.Maps);
	Cache.bValid = true;
	OutSummary = FGrimTerrainGenerator::MakeSummary(Layout, Cache.Maps);
	UE_LOG(LogGrimTerrain, Display, TEXT("지형 생성: %d × %d 정점, 높이 %.1f ~ %.1f m, 탑 평지 %.1f m, 물 %d곳 (%.2f초)"),
		OutSummary.SizeX, OutSummary.SizeY, OutSummary.MinHeight, OutSummary.MaxHeight, OutSummary.TowerPlateauHeight,
		OutSummary.Water.Num(), FPlatformTime::Seconds() - Start);
	return true;
}

float UGrimTerrainEditorLibrary::GetGroundHeight(float WorldX, float WorldY)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("GetGroundHeight")))
	{
		return 0.f;
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const FVector2D Map = FGrimTerrainGenerator::WorldToMap(Cache.Layout, FVector2D(WorldX, WorldY));
	return FGrimTerrainGenerator::SampleHeight(Cache.Maps, Map.X, Map.Y) * 100.f;
}

float UGrimTerrainEditorLibrary::GetSlopeDegrees(float WorldX, float WorldY)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("GetSlopeDegrees")))
	{
		return 0.f;
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const FVector2D Map = FGrimTerrainGenerator::WorldToMap(Cache.Layout, FVector2D(WorldX, WorldY));
	return FGrimTerrainGenerator::SampleSlopeDeg(Cache.Maps, Map.X, Map.Y);
}

FString UGrimTerrainEditorLibrary::RunTerrainAnalysis()
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("RunTerrainAnalysis")))
	{
		return FString();
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const FGrimTerrainAnalysisResult Result = FGrimTerrainAnalysis::Run(Cache.Layout, Cache.Maps);
	FString Report = Result.ToString();
	for (int32 Index = 0; Index < FMath::Min(Result.SightFailPoints.Num(), 20); ++Index)
	{
		Report += FString::Printf(TEXT("%s(%.0f, %.0f)"), Index == 0 ? TEXT("\n시선 모자란 곳: ") : TEXT(" "), Result.SightFailPoints[Index].X, Result.SightFailPoints[Index].Y);
	}
	for (const FGrimTerrainViewCheck& Check : Cache.Layout.Analysis.ViewChecks)
	{
		const TArray<FName> Seen = FGrimTerrainAnalysis::VisibleFrom(Cache.Layout, Cache.Maps, Check.From.X, Check.From.Y, Check.EyeHeight);
		Report += FString::Printf(TEXT("\n%s 에서 보임(%d):"), *Check.Name.ToString(), Seen.Num());
		for (const FName& Name : Seen)
		{
			Report += TEXT(" ") + Name.ToString();
		}
		for (const FName& Must : Check.MustSee)
		{
			if (!Seen.Contains(Must))
			{
				Report += FString::Printf(TEXT(" [안 보임: %s]"), *Must.ToString());
			}
		}
	}
	UE_LOG(LogGrimTerrain, Display, TEXT("지형 검사:\n%s"), *Report);
	return Report;
}

TArray<FName> UGrimTerrainEditorLibrary::GetVisibleFrom(float MapU, float MapV, float EyeHeight)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("GetVisibleFrom")))
	{
		return TArray<FName>();
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	return FGrimTerrainAnalysis::VisibleFrom(Cache.Layout, Cache.Maps, MapU, MapV, EyeHeight);
}

bool UGrimTerrainEditorLibrary::IsMapPointVisible(float FromU, float FromV, float EyeHeight, float ToU, float ToV, float TargetHeight)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("IsMapPointVisible")))
	{
		return false;
	}
	const FGrimTerrainMaps& Maps = GrimTerrainEditorLocal::Cache().Maps;
	const FVector Eye(FromU, FromV, FGrimTerrainGenerator::SampleHeight(Maps, FromU, FromV) + EyeHeight);
	const FVector Target(ToU, ToV, FGrimTerrainGenerator::SampleHeight(Maps, ToU, ToV) + TargetHeight);
	return FGrimTerrainAnalysis::IsVisible(Maps, Eye, Target);
}

bool UGrimTerrainEditorLibrary::ExportHeightmapR16(const FString& FilePath)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("ExportHeightmapR16")))
	{
		return false;
	}
	const FGrimTerrainMaps& Maps = GrimTerrainEditorLocal::Cache().Maps;
	// 랜드스케이프와 같은 순서(가로 = 월드 X = 북쪽 SizeY 개, 세로 = 동쪽 SizeX 개) — 에디터 랜드스케이프 가져오기에 바로 쓸 수 있게
	const TArray<float> Ordered = GrimTerrainEditorLocal::ToLandscapeOrder(Maps.Heights, Maps.SizeX, Maps.SizeY);
	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(Ordered.Num() * 2);
	for (int32 Index = 0; Index < Ordered.Num(); ++Index)
	{
		const uint16 Value = FGrimTerrainGenerator::HeightToLandscape(Ordered[Index]);
		Bytes[Index * 2] = static_cast<uint8>(Value & 0xFF);
		Bytes[Index * 2 + 1] = static_cast<uint8>(Value >> 8);
	}
	const FString FullPath = GrimTerrainEditorLocal::ResolvePath(FilePath);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FullPath), true);
	return FFileHelper::SaveArrayToFile(Bytes, *FullPath);
}

bool UGrimTerrainEditorLibrary::ExportPreviewImage(const FString& FilePath, int32 Downsample)
{
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("ExportPreviewImage")))
	{
		return false;
	}
	const FGrimTerrainMaps& Maps = GrimTerrainEditorLocal::Cache().Maps;
	const FGrimTerrainLayout& Layout = GrimTerrainEditorLocal::Cache().Layout;
	const int32 Step = FMath::Max(1, Downsample);
	const int32 Width = (Maps.SizeX - 1) / Step + 1;
	const int32 Height = (Maps.SizeY - 1) / Step + 1;
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Width * Height);

	const FLinearColor Low(0.20f, 0.42f, 0.14f);
	const FLinearColor Mid(0.45f, 0.52f, 0.22f);
	const FLinearColor High(0.42f, 0.34f, 0.24f);
	const FLinearColor Rock(0.50f, 0.49f, 0.47f);
	const FLinearColor Sand(0.80f, 0.72f, 0.52f);
	const FLinearColor PathColor(0.62f, 0.48f, 0.30f);
	const FLinearColor ForestColor(0.08f, 0.22f, 0.10f);
	const FVector Light = FVector(-1.0, 1.0, 1.2).GetSafeNormal(); // 북서쪽 위에서

	for (int32 PY = 0; PY < Height; ++PY)
	{
		const int32 Y = FMath::Min((Height - 1 - PY) * Step, Maps.SizeY - 1); // 그림 위 = 북쪽
		for (int32 PX = 0; PX < Width; ++PX)
		{
			const int32 X = FMath::Min(PX * Step, Maps.SizeX - 1);
			const int32 I = Maps.Index(X, Y);
			const double U = X * Maps.QuadSize;
			const double V = Y * Maps.QuadSize;
			const float H = Maps.Heights[I];
			FLinearColor Color;
			if (Maps.WaterDistance[I] < 0.f || H < Layout.SeaLevel)
			{
				const float Depth = FMath::Clamp((Layout.SeaLevel - H) / 20.f, 0.f, 1.f);
				Color = FMath::Lerp(FLinearColor(0.25f, 0.55f, 0.75f), FLinearColor(0.05f, 0.18f, 0.40f), Depth);
			}
			else
			{
				const float T = FMath::Clamp((H - 10.f) / 120.f, 0.f, 1.f);
				Color = T < 0.4f ? FMath::Lerp(Low, Mid, T / 0.4f) : FMath::Lerp(Mid, High, (T - 0.4f) / 0.6f);
				Color = FMath::Lerp(Color, ForestColor, Maps.Forest[I] / 255.f * 0.8f);
				const float Slope = FGrimTerrainGenerator::SampleSlopeDeg(Maps, U, V);
				Color = FMath::Lerp(Color, Rock, FMath::Clamp((Slope - 28.f) / 10.f, 0.f, 1.f));
				Color = FMath::Lerp(Color, Sand, Maps.Shore[I] / 255.f);
				Color = FMath::Lerp(Color, PathColor, Maps.Path[I] / 255.f);
			}
			const double D = Maps.QuadSize * Step;
			const double DX = (FGrimTerrainGenerator::SampleHeight(Maps, U + D, V) - FGrimTerrainGenerator::SampleHeight(Maps, U - D, V)) / (2.0 * D);
			const double DY = (FGrimTerrainGenerator::SampleHeight(Maps, U, V + D) - FGrimTerrainGenerator::SampleHeight(Maps, U, V - D)) / (2.0 * D);
			const FVector Normal = FVector(-DX, -DY, 1.0).GetSafeNormal();
			const float Shade = 0.45f + 0.65f * FMath::Max(0.f, static_cast<float>(Normal | Light));
			Pixels[PY * Width + PX] = (Color * Shade).ToFColor(true);
		}
	}

	// 랜드마크(흰 테두리 큰 점) · POI(묶음마다 같은 색 작은 점) — 그림 위 = 북쪽
	auto Dot = [&](const FVector2D& Map, int32 Radius, const FColor& Fill, const FColor& Rim)
	{
		const int32 CX = FMath::RoundToInt32(Map.X / Maps.QuadSize / Step);
		const int32 CY = Height - 1 - FMath::RoundToInt32(Map.Y / Maps.QuadSize / Step);
		for (int32 DY = -Radius - 1; DY <= Radius + 1; ++DY)
		{
			for (int32 DX = -Radius - 1; DX <= Radius + 1; ++DX)
			{
				const int32 PX = CX + DX;
				const int32 PY = CY + DY;
				const int32 D2 = DX * DX + DY * DY;
				if (PX >= 0 && PY >= 0 && PX < Width && PY < Height && D2 <= (Radius + 1) * (Radius + 1))
				{
					Pixels[PY * Width + PX] = D2 <= Radius * Radius ? Fill : Rim;
				}
			}
		}
	};
	for (const FGrimTerrainPoi& Poi : Layout.Pois)
	{
		const uint32 Hash = GetTypeHash(Poi.Cluster.ToString());
		const FColor Fill = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash % 255), 210, 255).ToFColor(true);
		Dot(Poi.Center, 3, Fill, FColor::Black);
	}
	for (const FGrimTerrainLandmark& Landmark : Layout.Landmarks)
	{
		Dot(Landmark.Center, 6, FColor(250, 60, 40), FColor::White);
	}

	const FString FullPath = GrimTerrainEditorLocal::ResolvePath(FilePath);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FullPath), true);
	const FImageView View(Pixels.GetData(), Width, Height);
	return FImageUtils::SaveImageByExtension(*FullPath, View);
}

UObject* UGrimTerrainEditorLibrary::CreateLayerInfoAsset(const FString& PackagePath, FName LayerName, FLinearColor DebugColor)
{
#if WITH_EDITOR
	bool bCreated = false;
	ULandscapeLayerInfoObject* Info = GrimTerrainEditorLocal::FindOrCreateAsset<ULandscapeLayerInfoObject>(PackagePath, bCreated);
	if (bCreated)
	{
		Info->SetLayerName(LayerName, false);
		Info->SetBlendMethod(ELandscapeTargetLayerBlendMethod::None, false);
		Info->SetLayerUsageDebugColor(DebugColor, false, EPropertyChangeType::ValueSet);
	}
	return Info;
#else
	return nullptr;
#endif
}

UObject* UGrimTerrainEditorLibrary::CreateFoliageTypeAsset(const FString& PackagePath, UStaticMesh* Mesh, UMaterialInterface* Material, int32 CullStartCm, int32 CullEndCm, bool bBlockCollision, bool bCastShadow)
{
#if WITH_EDITOR
	bool bCreated = false;
	UFoliageType_InstancedStaticMesh* Type = GrimTerrainEditorLocal::FindOrCreateAsset<UFoliageType_InstancedStaticMesh>(PackagePath, bCreated);
	if (bCreated)
	{
		Type->Mesh = Mesh;
		Type->OverrideMaterials.Reset();
		if (Material)
		{
			Type->OverrideMaterials.Add(Material);
		}
		Type->CullDistance = FInt32Interval(CullStartCm, CullEndCm);
		Type->CastShadow = bCastShadow;
		Type->BodyInstance.SetCollisionProfileName(bBlockCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
	}
	return Type;
#else
	return nullptr;
#endif
}

AActor* UGrimTerrainEditorLibrary::CreateLandscape(const UObject* WorldContextObject, UMaterialInterface* Material, const TArray<UObject*>& LayerInfos, int32 GridSizeInComponents, bool bSpatiallyLoaded)
{
#if WITH_EDITOR
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("CreateLandscape")))
	{
		return nullptr;
	}
	// 랜드스케이프 편집 레이어는 GPU 로 최종 높이 지도를 합친다 — -nullrhi 면 그림 높이 지도가 0(-256 m)으로 저장된다(충돌만 맞음).
	// 헤드리스 커맨드렛은 -AllowCommandletRendering 으로 RHI 를 켜고 돌린다(docs/systems/terrain.md).
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("CreateLandscape: 렌더링이 꺼져 있다(-nullrhi). -AllowCommandletRendering 으로 RHI 를 켜고 다시 돌린다"));
		return nullptr;
	}
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("CreateLandscape: 월드 없음"));
		return nullptr;
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const FGrimTerrainLayout& Layout = Cache.Layout;
	const FGrimTerrainMaps& Maps = Cache.Maps;
	const FGrimTerrainSummary Summary = FGrimTerrainGenerator::MakeSummary(Layout, Maps);

	// 랜드스케이프 로컬 X = 월드 X = 북쪽(지도 V), 로컬 Y = 동쪽(지도 U)
	const int32 LandscapeSizeX = Maps.SizeY;
	const int32 LandscapeSizeY = Maps.SizeX;
	const TArray<float> OrderedHeights = GrimTerrainEditorLocal::ToLandscapeOrder(Maps.Heights, Maps.SizeX, Maps.SizeY);
	TArray<uint16> Heights;
	Heights.SetNumUninitialized(OrderedHeights.Num());
	for (int32 Index = 0; Index < OrderedHeights.Num(); ++Index)
	{
		Heights[Index] = FGrimTerrainGenerator::HeightToLandscape(OrderedHeights[Index], Summary.LandscapeScale.Z);
	}

	TArray<FLandscapeImportLayerInfo> ImportLayers;
	TArray<ULandscapeLayerInfoObject*> Infos;
	for (UObject* Object : LayerInfos)
	{
		ULandscapeLayerInfoObject* Info = Cast<ULandscapeLayerInfoObject>(Object);
		const TArray<uint8>* Weights = Info ? GrimTerrainEditorLocal::WeightsByLayerName(Maps, Info->GetLayerName()) : nullptr;
		if (!Weights)
		{
			UE_LOG(LogGrimTerrain, Warning, TEXT("CreateLandscape: 모르는 레이어 정보 %s — 건너뜀"), *GetNameSafe(Object));
			continue;
		}
		FLandscapeImportLayerInfo& Import = ImportLayers.Emplace_GetRef(Info->GetLayerName());
		Import.LayerInfo = Info;
		Import.LayerData = GrimTerrainEditorLocal::ToLandscapeOrder(*Weights, Maps.SizeX, Maps.SizeY);
		Infos.Add(Info);
	}

	TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
	HeightDataPerLayers.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
	MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(ImportLayers));

	ALandscape* Landscape = World->SpawnActor<ALandscape>(Summary.LandscapeLocation, FRotator::ZeroRotator);
	if (!Landscape)
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("CreateLandscape: 랜드스케이프 액터를 만들지 못했다"));
		return nullptr;
	}
	Landscape->LandscapeMaterial = Material;
	Landscape->SetActorRelativeScale3D(Summary.LandscapeScale);
	Landscape->bAreNewLandscapeActorsSpatiallyLoaded = bSpatiallyLoaded;
	Landscape->SetActorLabel(TEXT("Landscape_OpenBeta"));
	Landscape->Import(FGuid::NewGuid(), 0, 0, LandscapeSizeX - 1, LandscapeSizeY - 1, Layout.SectionsPerComponent, Layout.SectionQuads,
		HeightDataPerLayers, TEXT(""), MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());

	ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
	if (!Info)
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("CreateLandscape: LandscapeInfo 없음"));
		return Landscape;
	}
	Info->UpdateLayerInfoMap(Landscape);
	for (ULandscapeLayerInfoObject* LayerInfo : Infos)
	{
		if (!Landscape->HasTargetLayer(LayerInfo->GetLayerName()))
		{
			Landscape->AddTargetLayer(LayerInfo->GetLayerName(), FLandscapeTargetLayerSettings(LayerInfo));
		}
	}

	ULandscapeSubsystem* LandscapeSubsystem = World->GetSubsystem<ULandscapeSubsystem>();
	if (LandscapeSubsystem && LandscapeSubsystem->IsGridBased() && GridSizeInComponents > 0)
	{
		LandscapeSubsystem->ChangeGridSize(Info, static_cast<uint32>(GridSizeInComponents));
	}
	int32 ProxyCount = 0;
	Info->ForEachLandscapeProxy([&ProxyCount, bSpatiallyLoaded](ALandscapeProxy* Proxy)
	{
		if (Cast<ALandscapeStreamingProxy>(Proxy))
		{
			++ProxyCount;
			if (Proxy->CanChangeIsSpatiallyLoadedFlag())
			{
				Proxy->SetIsSpatiallyLoaded(bSpatiallyLoaded);
			}
		}
		return true;
	});
	UE_LOG(LogGrimTerrain, Display, TEXT("랜드스케이프: %d × %d 정점(X 북 × Y 동), 구성 요소 %d × %d (섹션 %d칸 × %d²), 스트리밍 프록시 %d개(격자 %d 구성 요소, 공간 로드 %s)"),
		LandscapeSizeX, LandscapeSizeY, Layout.ComponentsY, Layout.ComponentsX, Layout.SectionQuads, Layout.SectionsPerComponent,
		ProxyCount, GridSizeInComponents, bSpatiallyLoaded ? TEXT("켬") : TEXT("끔"));
	return Landscape;
#else
	return nullptr;
#endif
}

TArray<FTransform> UGrimTerrainEditorLibrary::GetFoliageTransforms(FName RuleName, const FTransform& PartOffset)
{
	TArray<FTransform> Transforms;
	if (!GrimTerrainEditorLocal::RequireCache(TEXT("GetFoliageTransforms")))
	{
		return Transforms;
	}
	const GrimTerrainEditorLocal::FCache& Cache = GrimTerrainEditorLocal::Cache();
	const FGrimTerrainFoliageRule* Rule = Cache.Layout.Foliage.FindByPredicate([RuleName](const FGrimTerrainFoliageRule& Each) { return Each.Name == RuleName; });
	if (!Rule)
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("GetFoliageTransforms: 레이아웃에 식생 규칙 %s 가 없다"), *RuleName.ToString());
		return Transforms;
	}
	FGrimTerrainGenerator::ScatterFoliage(Cache.Layout, Cache.Maps, *Rule, Transforms);
	for (FTransform& Each : Transforms)
	{
		Each = PartOffset * Each;
	}
	return Transforms;
}

FString UGrimTerrainEditorLibrary::ConfigureWorldPartition(const UObject* WorldContextObject, int32 CellSizeCm, int32 LoadingRangeCm)
{
#if WITH_EDITOR
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UWorldPartition* Partition = World ? World->GetWorldPartition() : nullptr;
	UObject* Hash = Partition ? Partition->RuntimeHash.Get() : nullptr;
	if (!Hash)
	{
		UE_LOG(LogGrimTerrain, Warning, TEXT("ConfigureWorldPartition: World Partition 런타임 해시가 없다"));
		return FString();
	}

	// 숫자 속성 하나를 이름으로 바꾼다(정수 · 실수 모두)
	auto SetNumber = [](UStruct* Type, void* Container, const TCHAR* Name, int64 Value) -> bool
	{
		FNumericProperty* Property = FindFProperty<FNumericProperty>(Type, Name);
		if (!Property)
		{
			return false;
		}
		void* Address = Property->ContainerPtrToValuePtr<void>(Container);
		if (Property->IsFloatingPoint())
		{
			Property->SetFloatingPointPropertyValue(Address, static_cast<double>(Value));
		}
		else
		{
			Property->SetIntPropertyValue(Address, Value);
		}
		return true;
	};

	Hash->Modify();
	FString Result;
	if (FArrayProperty* Partitions = FindFProperty<FArrayProperty>(Hash->GetClass(), TEXT("RuntimePartitions")))
	{
		// RuntimeHashSet: RuntimePartitions[0].MainLayer(URuntimePartitionLHGrid).CellSize · LoadingRange
		FScriptArrayHelper Array(Partitions, Partitions->ContainerPtrToValuePtr<void>(Hash));
		FStructProperty* Inner = CastField<FStructProperty>(Partitions->Inner);
		if (Array.Num() > 0 && Inner)
		{
			FObjectProperty* MainLayerProperty = FindFProperty<FObjectProperty>(Inner->Struct, TEXT("MainLayer"));
			UObject* MainLayer = MainLayerProperty ? MainLayerProperty->GetObjectPropertyValue_InContainer(Array.GetRawPtr(0)) : nullptr;
			if (MainLayer)
			{
				MainLayer->Modify();
				const bool bCell = SetNumber(MainLayer->GetClass(), MainLayer, TEXT("CellSize"), CellSizeCm);
				const bool bRange = SetNumber(MainLayer->GetClass(), MainLayer, TEXT("LoadingRange"), LoadingRangeCm);
				Result = FString::Printf(TEXT("RuntimeHashSet %s(%s): 셀 %s, 스트리밍 거리 %s"), *Hash->GetName(), *MainLayer->GetClass()->GetName(),
					bCell ? *FString::Printf(TEXT("%d cm"), CellSizeCm) : TEXT("못 바꿈"), bRange ? *FString::Printf(TEXT("%d cm"), LoadingRangeCm) : TEXT("못 바꿈"));
			}
		}
	}
	else if (FArrayProperty* Grids = FindFProperty<FArrayProperty>(Hash->GetClass(), TEXT("Grids")))
	{
		// RuntimeSpatialHash: Grids[0].CellSize · LoadingRange
		FScriptArrayHelper Array(Grids, Grids->ContainerPtrToValuePtr<void>(Hash));
		FStructProperty* Inner = CastField<FStructProperty>(Grids->Inner);
		if (Array.Num() > 0 && Inner)
		{
			const bool bCell = SetNumber(Inner->Struct, Array.GetRawPtr(0), TEXT("CellSize"), CellSizeCm);
			const bool bRange = SetNumber(Inner->Struct, Array.GetRawPtr(0), TEXT("LoadingRange"), LoadingRangeCm);
			Result = FString::Printf(TEXT("RuntimeSpatialHash: 셀 %s, 스트리밍 거리 %s"),
				bCell ? *FString::Printf(TEXT("%d cm"), CellSizeCm) : TEXT("못 바꿈"), bRange ? *FString::Printf(TEXT("%d cm"), LoadingRangeCm) : TEXT("못 바꿈"));
		}
	}
	if (Result.IsEmpty())
	{
		UE_LOG(LogGrimTerrain, Warning, TEXT("ConfigureWorldPartition: 모르는 런타임 해시 %s"), *Hash->GetClass()->GetName());
		return FString();
	}
	World->GetOutermost()->MarkPackageDirty();
	UE_LOG(LogGrimTerrain, Display, TEXT("World Partition: %s"), *Result);
	return Result;
#else
	return FString();
#endif
}

AActor* UGrimTerrainEditorLibrary::SpawnBoxVolume(const UObject* WorldContextObject, TSubclassOf<AActor> VolumeClass, FVector Center, FVector Extent, float Yaw)
{
#if WITH_EDITOR
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !VolumeClass || !VolumeClass->IsChildOf(AVolume::StaticClass()))
	{
		UE_LOG(LogGrimTerrain, Error, TEXT("SpawnBoxVolume: 월드나 볼륨 클래스가 없다(%s)"), *GetNameSafe(VolumeClass.Get()));
		return nullptr;
	}
	AVolume* Volume = World->SpawnActor<AVolume>(VolumeClass.Get(), Center, FRotator(0.f, Yaw, 0.f));
	if (!Volume)
	{
		return nullptr;
	}
	UCubeBuilder* Builder = NewObject<UCubeBuilder>();
	Builder->X = Extent.X * 2.f;
	Builder->Y = Extent.Y * 2.f;
	Builder->Z = Extent.Z * 2.f;
	UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
	return Volume;
#else
	return nullptr;
#endif
}
