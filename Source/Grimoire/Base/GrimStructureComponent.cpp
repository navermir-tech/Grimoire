#include "Base/GrimStructureComponent.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseSubsystem.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimElementalTypes.h"
#include "Items/GrimItemStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGrimStructureComponent::UGrimStructureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UGrimStructureComponent* UGrimStructureComponent::FindOn(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UGrimStructureComponent>() : nullptr;
}

void UGrimStructureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimStructureComponent, PieceId);
	DOREPLIFETIME(UGrimStructureComponent, Health);
}

void UGrimStructureComponent::InitializePiece(FName InPieceId)
{
	PieceId = InPieceId;
	Health = -1.f;
}

const FGrimBuildPieceDefinition* UGrimStructureComponent::GetDefinition() const
{
	return UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
}

float UGrimStructureComponent::GetMaxHealth() const
{
	const FGrimBuildPieceDefinition* Def = GetDefinition();
	return (Def ? Def->Row.MaxHealth : 100.f) * UGrimBaseSettings::Get()->StructureMaxHealthScale;
}

float UGrimStructureComponent::GetHealth() const
{
	return Health < 0.f ? GetMaxHealth() : FMath::Min(Health, GetMaxHealth());
}

FName UGrimStructureComponent::GetRepairItem() const
{
	const FGrimBuildPieceDefinition* Def = GetDefinition();
	return Def ? Def->Row.RepairItem : NAME_None;
}

int32 UGrimStructureComponent::GetMaterialsNeeded() const
{
	return GetRepairItem().IsNone() ? 0 : FGrimDurabilityMath::GetMaterialsNeeded(GetHealth(), GetMaxHealth(), UGrimBaseSettings::Get()->RepairFractionPerMaterial);
}

AGrimBaseCore* UGrimStructureComponent::FindBaseCore() const
{
	const AActor* Owner = GetOwner();
	const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this);
	if (!Owner || !Subsystem)
	{
		return nullptr;
	}
	// 거점 핵 자신은 자기 거점 안이다.
	if (AGrimBaseCore* SelfCore = Cast<AGrimBaseCore>(const_cast<AActor*>(Owner)))
	{
		return SelfCore;
	}
	return Subsystem->FindBaseAt(Owner->GetActorLocation());
}

void UGrimStructureComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->RegisterStructure(this);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const float Interval = UGrimBaseSettings::Get()->StructureTickSeconds;
		// 조각마다 틱 시각을 흩어 둔다(한 프레임에 몰리지 않게).
		GetWorld()->GetTimerManager().SetTimer(TickTimer, this, &ThisClass::TickStructure, Interval, true, FMath::FRandRange(0.1f, Interval));
	}
}

void UGrimStructureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimer);
	}
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->UnregisterStructure(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimStructureComponent::OnRep_Structure()
{
	OnStructureChanged.Broadcast(this);
}

void UGrimStructureComponent::SetHealthInternal(float NewHealth)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	const float Clamped = FMath::Clamp(NewHealth, 0.f, GetMaxHealth());
	if (FMath::IsNearlyEqual(Clamped, GetHealth(), 0.001f) && Health >= 0.f)
	{
		return;
	}
	// 휴면 사물은 상태를 바꾸기 전에 깨운다(결정 기록 2026-11-05).
	Owner->FlushNetDormancy();
	Health = Clamped;
	OnStructureChanged.Broadcast(this);
	if (Health <= 0.f)
	{
		Break(true, TEXT("내구도 0"));
	}
}

void UGrimStructureComponent::TickStructure()
{
	AActor* Owner = GetOwner();
	if (!Owner || bBroken)
	{
		return;
	}
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	const float Interval = Settings->StructureTickSeconds;
	float Damage = 0.f;

	if (Settings->bDecayEnabled)
	{
		// 거점 핵 자신은 늘 「안」이다.
		const bool bInside = FindBaseCore() != nullptr;
		Damage += FGrimDurabilityMath::GetDecayPerSecond(GetMaxHealth(), bInside, Settings->InsideDecayFractionPerHour, Settings->OutsideDecayFractionPerHour) * Interval;
	}

	const FGrimBuildPieceDefinition* Def = GetDefinition();
	if (Def && Def->IsFlammable())
	{
		const float Burning = UGrimElementalStatics::GetStateIntensity(Owner, GrimTags::State_Burning);
		Damage += Burning * Settings->FireDamagePerSecond * Interval;
	}

	if (Damage > 0.f)
	{
		// 노후는 피해 배율을 받지 않고 그대로 깎는다(배율은 습격 · 불 같은 「피해」 옵션).
		SetHealthInternal(GetHealth() - Damage);
	}
}

void UGrimStructureComponent::ApplyStructureDamage(float Amount, const TCHAR* Cause)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.f || bBroken)
	{
		return;
	}
	const float Scaled = Amount * UGrimBaseSettings::Get()->StructureDamageScale;
	UE_LOG(LogGrimBase, Verbose, TEXT("%s 피해 %.1f(%s)"), *GetNameSafe(GetOwner()), Scaled, Cause);
	SetHealthInternal(GetHealth() - Scaled);
}

void UGrimStructureComponent::ApplyRepairMaterials(int32 Materials)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Materials <= 0)
	{
		return;
	}
	SetHealthInternal(FGrimDurabilityMath::Repair(GetHealth(), GetMaxHealth(), Materials, UGrimBaseSettings::Get()->RepairFractionPerMaterial));
}

void UGrimStructureComponent::SetHealth(float NewHealth)
{
	SetHealthInternal(NewHealth);
}

TArray<FGrimItemInstance> UGrimStructureComponent::GetRefundItems(float Fraction) const
{
	TArray<FGrimItemInstance> Result;
	if (const FGrimBuildPieceDefinition* Def = GetDefinition())
	{
		for (const FGrimRecipeInput& Input : Def->Cost)
		{
			const int32 Count = FGrimDurabilityMath::GetRefundCount(Input.Count, Fraction);
			if (Count > 0 && !Input.bGroup)
			{
				Result.Add(FGrimItemInstance(Input.Id, Count));
			}
		}
	}
	return Result;
}

void UGrimStructureComponent::Break(bool bDropRubble, const TCHAR* Cause)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bBroken)
	{
		return;
	}
	bBroken = true;
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s(%s) 부서짐 — %s"), *Owner->GetName(), *PieceId.ToString(), Cause);
	if (bDropRubble)
	{
		const FVector Location = Owner->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		for (const FGrimItemInstance& Item : GetRefundItems(UGrimBaseSettings::Get()->RubbleFraction))
		{
			UGrimItemStatics::SpawnPickup(Owner, Item.ItemId, Item.Count, Location + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0.f));
		}
	}
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->NotifyStructureBroken(this);
	}
	Owner->Destroy();
}

// ───── 세이브 (Structure.<이름> v1) ─────

FName UGrimStructureComponent::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Structure.%s"), GetOwner() ? *GetOwner()->GetName() : TEXT("None")));
}

void UGrimStructureComponent::WriteSaveData(FArchive& Ar)
{
	FString IdText = PieceId.ToString();
	float SavedHealth = GetHealth();
	Ar << IdText;
	Ar << SavedHealth;
}

void UGrimStructureComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	FString IdText;
	float SavedHealth = -1.f;
	Ar << IdText;
	Ar << SavedHealth;
	if (Ar.IsError())
	{
		return;
	}
	// 다시 스폰할 때는 FinishSpawning 전에 불린다 — 복제 값을 바로 넣는다.
	PieceId = IdText.IsEmpty() || IdText == TEXT("None") ? PieceId : FName(*IdText);
	Health = SavedHealth;
	OnStructureChanged.Broadcast(this);
}
