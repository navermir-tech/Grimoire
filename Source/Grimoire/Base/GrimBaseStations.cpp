#include "Base/GrimBaseStations.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseVisuals.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimItemContainerComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Items/GrimItemSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GrimBaseStations"

const FName AGrimStorageChest::TabId(TEXT("Storage"));
const FName AGrimWorkbench::TabId(TEXT("Workbench"));
const FName AGrimWorkbench::StationId(TEXT("Workbench"));

// ───────────────────────── 침대 · 모닥불 ─────────────────────────

AGrimBaseBed::AGrimBaseBed()
{
	Structure = CreateDefaultSubobject<UGrimStructureComponent>(TEXT("Structure"));
}

AGrimBaseCampfire::AGrimBaseCampfire()
{
	Structure = CreateDefaultSubobject<UGrimStructureComponent>(TEXT("Structure"));
	// 둘레의 돌(불붙는 재질은 장작 — 부모가 Material.Flammable 을 준다). 구조 쪽은 표의 재료(돌)라 타서 닳지 않는다.
}

FName AGrimBaseCampfire::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("BuiltCampfire.%s"), *GetName()));
}

// ───────────────────────── 시설 공통 ─────────────────────────

AGrimStationBase::AGrimStationBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(30000.f));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(CubeMesh.Object);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Structure = CreateDefaultSubobject<UGrimStructureComponent>(TEXT("Structure"));
}

void AGrimStationBase::BeginPlay()
{
	Super::BeginPlay();
	const FGrimBuildPieceDefinition* Def = Structure ? Structure->GetDefinition() : nullptr;
	GrimBaseVisuals::SetColor(Body, Def ? Def->Row.Color : BodyColor);
}

void AGrimStationBase::InteractLocal(UGrimBuilderComponent* Builder)
{
	if (Builder)
	{
		Builder->OpenStation(this);
	}
}

FName AGrimStationBase::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("%s.%s"), GetSavePrefix(), *GetName()));
}

// ───────────────────────── 저장고 ─────────────────────────

AGrimStorageChest::AGrimStorageChest()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	// 상자 120 × 70 × 60 + 뚜껑.
	GrimBaseVisuals::ShapeAsBox(Body, FVector(0.f, 0.f, 30.f), FVector(60.f, 35.f, 30.f));
	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Root);
	Lid->SetStaticMesh(CubeMesh.Object);
	GrimBaseVisuals::ShapeAsBox(Lid, FVector(0.f, 0.f, 64.f), FVector(62.f, 37.f, 4.f));
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Container = CreateDefaultSubobject<UGrimItemContainerComponent>(TEXT("Container"));
	BodyColor = FLinearColor(0.42f, 0.27f, 0.12f);
}

void AGrimStorageChest::BeginPlay()
{
	Super::BeginPlay();
	GrimBaseVisuals::SetColor(Lid, BodyColor * 0.7f);
	if (HasAuthority() && Container->GetSlotCount() == 0)
	{
		Container->InitSlots(UGrimBaseSettings::Get()->StorageSlotCount);
	}
}

void AGrimStorageChest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 부서지거나 철거되면 내용물이 바닥에 남는다(맵을 떠날 때는 아님).
	if (HasAuthority() && EndPlayReason == EEndPlayReason::Destroyed && Container && !Container->IsEmpty())
	{
		Container->DropAll(GetActorLocation() + FVector(0.f, 0.f, 40.f));
	}
	Super::EndPlay(EndPlayReason);
}

FText AGrimStorageChest::GetInteractPrompt(const APawn* Pawn) const
{
	int32 Used = 0;
	for (const FGrimItemInstance& Slot : Container->GetSlots())
	{
		Used += Slot.IsEmpty() ? 0 : 1;
	}
	return FText::Format(LOCTEXT("ChestPrompt", "저장고 열기 ({0}/{1}칸)"), Used, Container->GetSlotCount());
}

// ───────────────────────── 작업대 ─────────────────────────

AGrimWorkbench::AGrimWorkbench()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	// 상판 160 × 80 × 10 (높이 90) + 다리 판 둘.
	GrimBaseVisuals::ShapeAsBox(Body, FVector(0.f, 0.f, 85.f), FVector(80.f, 40.f, 5.f));
	LegA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LegA"));
	LegA->SetupAttachment(Root);
	LegA->SetStaticMesh(CubeMesh.Object);
	GrimBaseVisuals::ShapeAsBox(LegA, FVector(-70.f, 0.f, 40.f), FVector(5.f, 35.f, 40.f));
	LegB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LegB"));
	LegB->SetupAttachment(Root);
	LegB->SetStaticMesh(CubeMesh.Object);
	GrimBaseVisuals::ShapeAsBox(LegB, FVector(70.f, 0.f, 40.f), FVector(5.f, 35.f, 40.f));
	BodyColor = FLinearColor(0.55f, 0.38f, 0.2f);
}

FText AGrimWorkbench::GetInteractPrompt(const APawn* Pawn) const
{
	return LOCTEXT("WorkbenchPrompt", "작업대 — 만들기 · 「수리」 마법을 걸면 장비 수리");
}

// ───────────────────────── 밭 ─────────────────────────

AGrimFarmPlot::AGrimFarmPlot()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	// 흙 두둑 200 × 200 × 20. 비 자극을 받도록 월드 다이내믹(날씨는 WorldDynamic · Pawn · PhysicsBody 를 적신다).
	GrimBaseVisuals::ShapeAsBox(Body, FVector(0.f, 0.f, 10.f), FVector(100.f, 100.f, 10.f));
	Body->SetCollisionObjectType(ECC_WorldDynamic);

	PlantMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlantMesh"));
	PlantMesh->SetupAttachment(Root);
	PlantMesh->SetStaticMesh(ConeMesh.Object);
	PlantMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlantMesh->SetVisibility(false);

	ElementalState = CreateDefaultSubobject<UGrimElementalStateComponent>(TEXT("ElementalState"));
	BodyColor = FLinearColor(0.25f, 0.16f, 0.08f);
}

void AGrimFarmPlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimFarmPlot, CropId);
	DOREPLIFETIME(AGrimFarmPlot, Growth);
	DOREPLIFETIME(AGrimFarmPlot, Moisture);
	DOREPLIFETIME(AGrimFarmPlot, bAsh);
}

void AGrimFarmPlot::BeginPlay()
{
	Super::BeginPlay();
	ElementalState->OnStateChangedNative.AddUObject(this, &ThisClass::HandleElementalChanged);
	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(CropTimer, this, &ThisClass::TickCrop, UGrimBaseSettings::Get()->CropTickSeconds, true);
		// 작물이 있을 때만 불이 붙는다(빈 흙은 타지 않는다).
		if (!CropId.IsNone())
		{
			ElementalState->Material.AddTag(GrimInteractionTags::Material_Flammable);
		}
	}
	RefreshPlant();
}

void AGrimFarmPlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CropTimer);
	}
	Super::EndPlay(EndPlayReason);
}

bool AGrimFarmPlot::IsMature() const
{
	return !CropId.IsNone() && FGrimCropMath::IsMature(Growth);
}

bool AGrimFarmPlot::Plant(FName InCropId)
{
	if (!HasAuthority() || !CropId.IsNone() || !UGrimBaseSettings::GetDatabase().FindCrop(InCropId))
	{
		return false;
	}
	CropId = InCropId;
	Growth = 0.f;
	ElementalState->Material.AddTag(GrimInteractionTags::Material_Flammable);
	RefreshPlant();
	return true;
}

TArray<FGrimItemInstance> AGrimFarmPlot::Harvest()
{
	TArray<FGrimItemInstance> Result;
	const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
	if (!HasAuthority() || !Crop || !IsMature())
	{
		return Result;
	}
	Result.Add(FGrimItemInstance(Crop->Row.HarvestItem, Crop->Row.HarvestCount));
	if (Crop->Row.SeedReturn > 0)
	{
		Result.Add(FGrimItemInstance(Crop->Row.SeedItem, Crop->Row.SeedReturn));
	}
	CropId = NAME_None;
	Growth = 0.f;
	bAsh = false;
	ElementalState->Material.RemoveTag(GrimInteractionTags::Material_Flammable);
	RefreshPlant();
	return Result;
}

FGrimCropConditions AGrimFarmPlot::GatherConditions() const
{
	FGrimCropConditions Conditions;
	Conditions.bWet = Moisture > 0.f;
	Conditions.bAsh = bAsh;
	const UWorld* World = GetWorld();
	if (const UGrimTimeOfDaySubsystem* TimeOfDay = World ? World->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr)
	{
		Conditions.bDaylight = !TimeOfDay->IsNight();
	}
	const FGameplayTag Illuminated = FGameplayTag::RequestGameplayTag(TEXT("State.Illuminated"), false);
	Conditions.bIlluminated = Illuminated.IsValid() && ElementalState->GetStateIntensity(Illuminated) > 0.f;
	if (const UGrimWeatherSubsystem* Weather = World ? World->GetSubsystem<UGrimWeatherSubsystem>() : nullptr)
	{
		// 지붕 아래(온실)면 날씨 몫을 뺀다 — 생존 체온과 같은 규칙.
		const FGrimClimateExposureBreakdown Breakdown = Weather->GetClimateExposureBreakdown(GetActorLocation());
		const FGrimClimateExposure& Exposure = Weather->IsActorSheltered(this) ? Breakdown.Sheltered : Breakdown.Total;
		Conditions.ColdGrade = Exposure.Cold;
		Conditions.HeatGrade = Exposure.Heat;
	}
	return Conditions;
}

void AGrimFarmPlot::AdvanceGrowth(float Seconds)
{
	const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
	if (!HasAuthority() || !Crop || Seconds <= 0.f)
	{
		return;
	}
	const int32 StageBefore = FGrimCropMath::GetStage(Growth, Crop->Row.Stages);
	Growth = FMath::Min(1.f, Growth + FGrimCropMath::GetGrowthRate(Crop->Row, GatherConditions()) * Seconds);
	if (FGrimCropMath::GetStage(Growth, Crop->Row.Stages) != StageBefore)
	{
		RefreshPlant();
	}
}

void AGrimFarmPlot::TickCrop()
{
	const float Interval = UGrimBaseSettings::Get()->CropTickSeconds;
	// 젖음(비 · 물 마법)이 물기를 채운다.
	const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
	if (ElementalState->GetStateIntensity(GrimTags::State_Wet) > 0.f)
	{
		Moisture = Crop ? Crop->Row.MoistureSeconds : 600.f;
	}
	else
	{
		Moisture = FMath::Max(0.f, Moisture - Interval);
	}
	AdvanceGrowth(Interval);
}

void AGrimFarmPlot::HandleElementalChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity)
{
	if (!HasAuthority())
	{
		return;
	}
	if (StateTag == GrimTags::State_Wet && NewIntensity > 0.f)
	{
		const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
		Moisture = Crop ? Crop->Row.MoistureSeconds : 600.f;
	}
	else if (StateTag == GrimTags::State_Burning && NewIntensity > 0.f && !CropId.IsNone())
	{
		// 작물이 타 없어지고 재가 남는다(다음 작물 비료). 불은 다 탄 뒤 꺼진다(흙으로 덮은 셈).
		UE_LOG(LogGrimBase, Log, TEXT("[Server] 밭 %s 의 %s 이(가) 타 버렸다 — 재가 남음"), *GetName(), *CropId.ToString());
		CropId = NAME_None;
		Growth = 0.f;
		bAsh = true;
		ElementalState->Material.RemoveTag(GrimInteractionTags::Material_Flammable);
		RefreshPlant();
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			FGrimElementalStimulus Smother;
			Smother.Element = GrimTags::Element_Earth;
			Smother.Intensity = 1.f;
			Smother.Source = EGrimStimulusSource::Environment;
			ElementalState->ApplyStimulus(Smother, this);
		}));
	}
}

void AGrimFarmPlot::OnRep_Crop()
{
	RefreshPlant();
}

void AGrimFarmPlot::RefreshPlant()
{
	const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
	GrimBaseVisuals::SetColor(Body, bAsh ? FLinearColor(0.12f, 0.12f, 0.12f) : BodyColor);
	if (!Crop)
	{
		PlantMesh->SetVisibility(false);
		return;
	}
	// 단계마다 커지는 원뿔(아트 전). 다 자라면 작물 색이 밝아진다.
	const int32 Stage = FGrimCropMath::GetStage(Growth, Crop->Row.Stages);
	const float Scale = 0.25f + 0.75f * Stage / FMath::Max(1, Crop->Row.Stages);
	PlantMesh->SetVisibility(true);
	PlantMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f + 50.f * Scale));
	PlantMesh->SetRelativeScale3D(FVector(1.2f * Scale, 1.2f * Scale, Scale));
	GrimBaseVisuals::SetColor(PlantMesh, IsMature() ? Crop->Row.Color * 1.4f : Crop->Row.Color * 0.7f);
}

FText AGrimFarmPlot::GetInteractPrompt(const APawn* Pawn) const
{
	const FGrimCropDefinition* Crop = UGrimBaseSettings::GetDatabase().FindCrop(CropId);
	if (!Crop)
	{
		return bAsh ? LOCTEXT("PlotEmptyAsh", "밭 — 씨앗 심기 (재 비료)") : LOCTEXT("PlotEmpty", "밭 — 씨앗 심기");
	}
	if (IsMature())
	{
		return FText::Format(LOCTEXT("PlotHarvest", "{0} 거두기"), Crop->GetDisplayName());
	}
	return FText::Format(LOCTEXT("PlotGrowing", "{0} 자라는 중 {1}%{2}"), Crop->GetDisplayName(), FMath::RoundToInt(Growth * 100.f),
		Moisture > 0.f ? FText::GetEmpty() : LOCTEXT("PlotDry", " — 말랐다(비 · 물 마법)"));
}

void AGrimFarmPlot::InteractLocal(UGrimBuilderComponent* Builder)
{
	if (Builder)
	{
		Builder->Server_Interact(this);
	}
}

void AGrimFarmPlot::InteractServer(UGrimBuilderComponent* Builder, APawn* Pawn)
{
	if (Builder)
	{
		Builder->PlantOrHarvest(this);
	}
}

void AGrimFarmPlot::WriteSaveData(FArchive& Ar)
{
	FString IdText = CropId.ToString();
	float SavedGrowth = Growth;
	float SavedMoisture = Moisture;
	bool bSavedAsh = bAsh;
	Ar << IdText << SavedGrowth << SavedMoisture << bSavedAsh;
}

void AGrimFarmPlot::ReadSaveData(FArchive& Ar, int32 Version)
{
	FString IdText;
	float SavedGrowth = 0.f;
	float SavedMoisture = 0.f;
	bool bSavedAsh = false;
	Ar << IdText << SavedGrowth << SavedMoisture << bSavedAsh;
	if (Ar.IsError())
	{
		return;
	}
	CropId = IdText.IsEmpty() || IdText == TEXT("None") ? NAME_None : FName(*IdText);
	Growth = FMath::Clamp(SavedGrowth, 0.f, 1.f);
	Moisture = FMath::Max(0.f, SavedMoisture);
	bAsh = bSavedAsh;
}

#undef LOCTEXT_NAMESPACE
