#include "Items/GrimItemPickup.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemTags.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalObject.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace GrimItemPickupLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	UStaticMesh* LoadShapeMesh(EGrimPickupShape Shape)
	{
		const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
		switch (Shape)
		{
		case EGrimPickupShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
		case EGrimPickupShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
		case EGrimPickupShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
		default: break;
		}
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
}

AGrimItemPickup::AGrimItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(4.f);

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitBoxExtent(FVector(12.5f));
	// 마법(투사체 = WorldDynamic)에 맞고(불로 익히기), 시야 추적에 걸리고, 폰은 지나간다(겹침 — 저절로 줍기).
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetStaticMesh(GrimItemPickupLocal::LoadShapeMesh(EGrimPickupShape::Cube));
	Mesh->SetRelativeScale3D(FVector(0.25f));
}

void AGrimItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimItemPickup, Item);
	DOREPLIFETIME(AGrimItemPickup, CookProgress);
	DOREPLIFETIME(AGrimItemPickup, bHeated);
	DOREPLIFETIME(AGrimItemPickup, bCooked);
}

void AGrimItemPickup::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (Item.IsEmpty() || !UGrimItemSettings::GetDatabase().Find(Item.ItemId))
		{
			UE_LOG(LogGrimItems, Warning, TEXT("[Server] 픽업 %s: 아이템 %s 가 비었거나 표에 없다 — 없앤다"), *GetName(), *Item.ItemId.ToString());
			Destroy();
			return;
		}
		Collision->OnComponentBeginOverlap.AddDynamic(this, &AGrimItemPickup::HandleOverlap);
		UpdateCookTimer();
	}
	RefreshVisuals();
}

void AGrimItemPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CookTimer);
	}
	Super::EndPlay(EndPlayReason);
}

FName AGrimItemPickup::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Item.Pickup.%s"), *GetName()));
}

void AGrimItemPickup::WriteSaveData(FArchive& Ar)
{
	FGrimItemInstance Saved = Item;
	Saved.Serialize(Ar);
	float SavedProgress = CookProgress;
	Ar << SavedProgress;
}

void AGrimItemPickup::ReadSaveData(FArchive& Ar, int32 Version)
{
	FGrimItemInstance Loaded;
	Loaded.Serialize(Ar);
	float LoadedProgress = 0.f;
	Ar << LoadedProgress;
	if (Ar.IsError() || !HasAuthority())
	{
		return;
	}
	// 다시 스폰할 때는 FinishSpawning 전에 불린다 — 표에 없으면 BeginPlay 가 없앤다.
	Item = Loaded;
	CookProgress = FMath::Max(0.f, LoadedProgress);
	if (HasActorBegunPlay())
	{
		SetItem(Loaded);
		CookProgress = FMath::Max(0.f, LoadedProgress);
	}
}

void AGrimItemPickup::SetItem(const FGrimItemInstance& NewItem)
{
	if (!HasAuthority())
	{
		return;
	}
	if (NewItem.ItemId != Item.ItemId)
	{
		CookProgress = 0.f;
	}
	Item = NewItem;
	if (!HasActorBegunPlay())
	{
		return; // 스폰 중(SpawnPickup · 세이브) — BeginPlay 가 맞춘다
	}
	if (Item.IsEmpty())
	{
		Destroy();
		return;
	}
	RefreshVisuals();
	UpdateCookTimer();
	ForceNetUpdate();
}

float AGrimItemPickup::GetCookSeconds() const
{
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(Item.ItemId);
	return Def && Def->IsCookable() ? Def->Row.CookSeconds : 0.f;
}

void AGrimItemPickup::OnRep_Item()
{
	RefreshVisuals();
}

void AGrimItemPickup::RefreshVisuals()
{
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	const FGrimItemDefinition* Def = Database.Find(Item.ItemId);
	if (!Def)
	{
		return;
	}
	const float Size = FMath::Max(1.f, Def->Row.Size);
	Collision->SetBoxExtent(FVector(Size * 0.5f));
	if (UStaticMesh* ShapeMesh = GrimItemPickupLocal::LoadShapeMesh(Def->Row.Shape))
	{
		Mesh->SetStaticMesh(ShapeMesh);
	}
	Mesh->SetRelativeScale3D(FVector(Size / 100.f));

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!Material)
	{
		Material = Mesh->CreateDynamicMaterialInstance(0);
	}
	if (Material)
	{
		// 익어 가는 동안 결과 아이템 색으로 조금씩.
		FLinearColor Color = Def->Row.Color;
		if (Def->IsCookable() && CookProgress > 0.f)
		{
			if (const FGrimItemDefinition* Result = Database.Find(Def->Row.CookResult))
			{
				Color = FMath::Lerp(Color, Result->Row.Color, FMath::Clamp(CookProgress / Def->Row.CookSeconds, 0.f, 1.f));
			}
		}
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

bool AGrimItemPickup::IsWithinReach(const APawn* Pawn, float Slack) const
{
	if (!Pawn || IsActorBeingDestroyed() || Item.IsEmpty())
	{
		return false;
	}
	const float Radius = UGrimItemSettings::Get()->PickupRadius + FMath::Max(0.f, Slack);
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

AGrimItemPickup* AGrimItemPickup::FindNearestReachable(const APawn* Pawn)
{
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	AGrimItemPickup* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGrimItemPickup> It(World); It; ++It)
	{
		if (It->IsWithinReach(Pawn))
		{
			const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), It->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
	}
	return Best;
}

void AGrimItemPickup::BlockAutoPickupFor(const APawn* Pawn, float Seconds)
{
	AutoPickupBlockedPawn = Pawn;
	AutoPickupBlockedUntil = FPlatformTime::Seconds() + Seconds;
}

void AGrimItemPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!HasAuthority() || !Pawn || !UGrimItemSettings::Get()->bAutoPickupOnOverlap || IsActorBeingDestroyed())
	{
		return;
	}
	if (AutoPickupBlockedPawn.Get() == Pawn && FPlatformTime::Seconds() < AutoPickupBlockedUntil)
	{
		return;
	}
	if (UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(Pawn))
	{
		Inventory->PickUp(this, false);
	}
}

// ───── 요리 (R16) ─────

void AGrimItemPickup::ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* InInstigator)
{
	if (!HasAuthority())
	{
		return;
	}
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	if (Stimulus.Element.MatchesTag(GrimTags::Element_Fire))
	{
		if (Stimulus.Intensity >= Settings->DirectFireMinIntensity && GetCookSeconds() > 0.f)
		{
			DirectHeatRemaining = Settings->DirectFireHeatSeconds;
			DirectHeatIntensity = FMath::Clamp(Stimulus.Intensity, 1.f, 3.f);
			UE_LOG(LogGrimItems, Display, TEXT("[Server] %s(%s) 에 불 자극 %.2f — %.1f초 달궈진다"), *GetName(), *Item.ItemId.ToString(), Stimulus.Intensity, DirectHeatRemaining);
			UpdateCookTimer();
		}
	}
	else if (Stimulus.Element.MatchesTag(GrimTags::Element_Water) || Stimulus.Element.MatchesTag(GrimTags::Element_Earth))
	{
		DirectHeatRemaining = 0.f;
	}
}

float AGrimItemPickup::GetElementalStateIntensity(const FGameplayTag& StateTag) const
{
	if (StateTag == GrimItemTags::State_Cooked)
	{
		return bCooked ? 1.f : 0.f;
	}
	return 0.f;
}

float AGrimItemPickup::FindHeatIntensityNear(const UWorld* World, const FVector& Location, float Radius, const AActor* Ignore)
{
	if (!World || Radius <= 0.f)
	{
		return 0.f;
	}
	// 열원 = 질의 콜리전이 있는 액터 중 State.Burning 세기가 있는 것(모닥불 · 아궁이 · 불붙은 풀). 양초는 너무 작다.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimItemHeatSources), false, Ignore);
	World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(Radius), Params);
	float Best = 0.f;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == Ignore || Actor->IsA<APawn>() || Actor->IsA<AGrimItemPickup>() || Actor->IsA<AGrimCandle>())
		{
			continue;
		}
		Best = FMath::Max(Best, UGrimElementalStatics::GetStateIntensity(Actor, GrimTags::State_Burning));
	}
	return Best;
}

void AGrimItemPickup::UpdateCookTimer()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority() || !HasActorBegunPlay())
	{
		return;
	}
	if (GetCookSeconds() > 0.f)
	{
		if (!World->GetTimerManager().IsTimerActive(CookTimer))
		{
			const float Interval = UGrimItemSettings::Get()->CookCheckInterval;
			World->GetTimerManager().SetTimer(CookTimer, FTimerDelegate::CreateWeakLambda(this, [this, Interval]()
			{
				UpdateCooking(Interval);
			}), Interval, true);
		}
	}
	else
	{
		World->GetTimerManager().ClearTimer(CookTimer);
	}
}

void AGrimItemPickup::UpdateCooking(float DeltaSeconds)
{
	if (!HasAuthority() || IsActorBeingDestroyed())
	{
		return;
	}
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(Item.ItemId);
	if (!Def || !Def->IsCookable())
	{
		if (bHeated)
		{
			bHeated = false;
			ForceNetUpdate();
		}
		return;
	}

	float Heat = FindHeatIntensityNear(GetWorld(), GetActorLocation(), Settings->CookHeatRadius, this);
	if (DirectHeatRemaining > 0.f)
	{
		Heat = FMath::Max(Heat, DirectHeatIntensity);
		DirectHeatRemaining = FMath::Max(0.f, DirectHeatRemaining - DeltaSeconds);
	}
	const bool bNowHeated = Heat > 0.f;
	if (bNowHeated != bHeated)
	{
		bHeated = bNowHeated;
		ForceNetUpdate();
	}
	if (!bNowHeated)
	{
		return;
	}

	if (FGrimCookingModel::Advance(CookProgress, Heat, DeltaSeconds, Def->Row.CookSeconds, Settings->ExtraCookRatePerIntensity))
	{
		const FName Result = Def->Row.CookResult;
		UE_LOG(LogGrimItems, Display, TEXT("[Server] %s: %s × %d 이(가) 익었다 → %s (State.Cooked)"), *GetName(), *Item.ItemId.ToString(), Item.Count, *Result.ToString());
		bCooked = true;
		bHeated = false;
		DirectHeatRemaining = 0.f;
		FGrimItemInstance Cooked = Item;
		Cooked.ItemId = Result;
		SetItem(Cooked); // 진행은 0 으로
		return;
	}
	RefreshVisuals();
}
