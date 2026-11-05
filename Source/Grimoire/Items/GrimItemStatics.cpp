#include "Items/GrimItemStatics.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemEffects.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

AGrimItemPickup* UGrimItemStatics::SpawnPickup(UObject* World, FName ItemId, int32 Count, FVector Location)
{
	UWorld* GameWorld = GEngine ? GEngine->GetWorldFromContextObject(World, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!GameWorld || GameWorld->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(ItemId);
	if (!Def || Count <= 0)
	{
		UE_LOG(LogGrimItems, Warning, TEXT("[Server] 픽업을 놓을 수 없다: 아이템 %s × %d(표에 없거나 개수 0)"), *ItemId.ToString(), Count);
		return nullptr;
	}

	const FVector Ground = FindDropLocation(GameWorld, Location, FMath::Max(1.f, Def->Row.Size * 0.5f));
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Ground);
	AGrimItemPickup* Pickup = GameWorld->SpawnActorDeferred<AGrimItemPickup>(AGrimItemPickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
	{
		return nullptr;
	}
	Pickup->SetItem(FGrimItemInstance(ItemId, Count));
	Pickup->FinishSpawning(Transform);
	UE_LOG(LogGrimItems, Display, TEXT("[Server] 픽업 %s: %s × %d at %s"), *Pickup->GetName(), *ItemId.ToString(), Count, *Ground.ToCompactString());
	return Pickup;
}

int32 UGrimItemStatics::GiveItem(AActor* PlayerActor, FName ItemId, int32 Count)
{
	UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(PlayerActor);
	if (!Inventory || !PlayerActor->HasAuthority())
	{
		return 0;
	}
	return Inventory->AddItem(FGrimItemInstance(ItemId, Count));
}

bool UGrimItemStatics::IsKnownItem(FName ItemId)
{
	return UGrimItemSettings::GetDatabase().Find(ItemId) != nullptr;
}

FText UGrimItemStatics::GetItemDisplayName(FName ItemId)
{
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(ItemId);
	return Def ? Def->GetDisplayName() : FText::FromName(ItemId);
}

void UGrimItemStatics::ApplyFoodEffects(UAbilitySystemComponent* ASC, const FGrimItemDefinition& Food)
{
	if (!ASC)
	{
		return;
	}
	if (Food.Row.HungerRestore != 0.f)
	{
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimSurvivalTickEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HealthMagnitudeName, 0.f);
			Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::HungerMagnitudeName, Food.Row.HungerRestore);
			Spec.Data->SetSetByCallerMagnitude(UGrimSurvivalTickEffect::BodyTemperatureMagnitudeName, 0.f);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	if (Food.Row.BuffSeconds > 0.f)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(UGrimFoodBuffEffect::StaticClass(), nullptr);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimFoodBuffEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(UGrimFoodBuffEffect::DurationMagnitudeName, Food.Row.BuffSeconds);
			Spec.Data->SetSetByCallerMagnitude(UGrimFoodBuffEffect::ColdResistMagnitudeName, Food.Row.BuffColdResist);
			Spec.Data->SetSetByCallerMagnitude(UGrimFoodBuffEffect::HeatResistMagnitudeName, Food.Row.BuffHeatResist);
			Spec.Data->SetSetByCallerMagnitude(UGrimFoodBuffEffect::DiseaseResistMagnitudeName, Food.Row.BuffDiseaseResist);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

FVector UGrimItemStatics::FindDropLocation(const UWorld* World, const FVector& Location, float HalfHeight, const AActor* Ignore)
{
	if (!World)
	{
		return Location;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimItemDrop), false, Ignore);
	FHitResult Hit;
	const FVector Start = Location + FVector(0.f, 0.f, 60.f);
	const FVector End = Location - FVector(0.f, 0.f, 1000.f);
	if (World->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight);
	}
	return Location;
}
