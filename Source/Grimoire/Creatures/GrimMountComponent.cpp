#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingTags.h"
#include "Character/GrimPlayerCharacter.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

namespace GrimMountLocal // 유니티 빌드 이름 충돌 방지
{
	const FName SaddleSocket(TEXT("Saddle"));

	const TCHAR* RoleName(const UWorld* World)
	{
		if (!World)
		{
			return TEXT("?");
		}
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer:
		case NM_DedicatedServer: return TEXT("Server");
		default: return TEXT("Standalone");
		}
	}

	FVector GetSaddleOffset(const AGrimCreature* Creature)
	{
		const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
		const FGrimMountProfile* Profile = Settings->FindMountProfile(Creature->GetInstance().SpeciesId);
		const FVector Offset = Profile ? Profile->SaddleOffset : FVector(0.f, 0.f, 100.f);
		return Offset * FMath::Max(0.1f, Creature->GetInstance().SizeScale);
	}
}

UGrimMountComponent::UGrimMountComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

AGrimCreature* UGrimMountComponent::GetCreature() const
{
	return Cast<AGrimCreature>(GetOwner());
}

bool UGrimMountComponent::IsMountable() const
{
	const AGrimCreature* Creature = GetCreature();
	return Creature && Creature->GetSpecies() && Creature->GetSpecies()->bMountable;
}

UGrimMountComponent* UGrimMountComponent::FindMountOf(const AActor* RiderActor)
{
	const AGrimCreature* Creature = RiderActor ? Cast<AGrimCreature>(RiderActor->GetAttachParentActor()) : nullptr;
	UGrimMountComponent* Mount = Creature ? Creature->GetMount() : nullptr;
	return Mount && Mount->GetRider() == RiderActor ? Mount : nullptr;
}

void UGrimMountComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimMountComponent, Rider);
}

void UGrimMountComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 탈것이 사라지면(접기 · 놓아주기 뒤 정리 · 퇴장) 탄 사람을 내려 둔다.
	if (ACharacter* Old = AppliedRider.Get())
	{
		DetachRider(Old);
	}
	Super::EndPlay(EndPlayReason);
}

float UGrimMountComponent::GetRideSpeed() const
{
	const AGrimCreature* Creature = GetCreature();
	const UGrimCreatureSpecies* Species = Creature ? Creature->GetSpecies() : nullptr;
	if (!Species)
	{
		return 0.f;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	const FGrimMountProfile* Profile = Settings->FindMountProfile(Species->SpeciesId);
	const UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	const float StageScale = Taming ? GrimTamingMath::GetMountSpeedScale(Taming->GetStage(), Settings->Loyalty) : 1.f;
	return Species->RunSpeed * (Profile ? Profile->SpeedScale : 1.f) * StageScale;
}

// ───────────────────────── 서버: 타기 · 내리기 ─────────────────────────

bool UGrimMountComponent::TryMount(ACharacter* InRider, FString& OutReason)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !InRider)
	{
		OutReason = TEXT("탈 수 없다");
		return false;
	}
	UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	if (!IsMountable())
	{
		OutReason = TEXT("탈것이 아니다");
		return false;
	}
	if (!Taming || !Taming->IsTamed() || Creature->IsDead() || Taming->IsKnockedOut())
	{
		OutReason = TEXT("지금은 탈 수 없다");
		return false;
	}
	if (Taming->GetOwnerPlayerState() != InRider->GetPlayerState())
	{
		OutReason = TEXT("주인만 탈 수 있다");
		return false;
	}
	if (IsRidden() || FindMountOf(InRider))
	{
		OutReason = TEXT("이미 타고 있다");
		return false;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	if (GrimTamingMath::RollMountRefuse(Taming->GetStage(), Settings->Loyalty, FMath::FRand()))
	{
		OutReason = FString::Printf(TEXT("%s 이(가) 태우기를 거부했다"), *Taming->GetDisplayName());
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 타기 거부 %s seed=%d (충성도 %.0f)"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, Taming->GetLoyalty());
		return false;
	}

	Rider = InRider;
	PendingInput = FVector::ZeroVector;
	LastInputTime = -1000.0;
	NextBuckCheckTime = GetWorld()->GetTimeSeconds() + Settings->Loyalty.BuckCheckSeconds;
	ApplyRiderLocal();
	ApplyRideSpeed();
	SetComponentTickEnabled(true);
	Creature->ForceNetUpdate();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 탔다 %s seed=%d 탄 사람=%s 속도=%.0f"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed,
		*InRider->GetName(), GetRideSpeed());
	return true;
}

void UGrimMountComponent::Dismount(bool bThrown)
{
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !Rider)
	{
		return;
	}
	ACharacter* Old = Rider;
	Rider = nullptr;
	PendingInput = FVector::ZeroVector;
	ApplyRiderLocal();
	SetComponentTickEnabled(false);
	if (UCharacterMovementComponent* Move = Creature->GetCharacterMovement())
	{
		if (bWaterRunning)
		{
			Move->SetMovementMode(MOVE_Falling);
		}
	}
	bWaterRunning = false;
	WaterVolume = nullptr;
	Creature->SetRunning(false);
	Creature->ForceNetUpdate();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] %s %s seed=%d 탄 사람=%s"), bThrown ? TEXT("떨어뜨렸다") : TEXT("내렸다"),
		*Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, *GetNameSafe(Old));
}

void UGrimMountComponent::SetRiderInput(const FVector& WorldInput)
{
	PendingInput = WorldInput.GetClampedToMaxSize(1.f);
	PendingInput.Z = 0.f;
	LastInputTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UGrimMountComponent::ApplyRideSpeed()
{
	AGrimCreature* Creature = GetCreature();
	UCharacterMovementComponent* Move = Creature ? Creature->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	const float Speed = GetRideSpeed();
	Move->MaxWalkSpeed = Speed;
	Move->MaxFlySpeed = Speed;
	Move->MaxSwimSpeed = Speed;
}

// ───────────────────────── 각 기기: 붙이기 ─────────────────────────

void UGrimMountComponent::OnRep_Rider()
{
	ApplyRiderLocal();
}

void UGrimMountComponent::ApplyRiderLocal()
{
	ACharacter* Current = Rider;
	ACharacter* Old = AppliedRider.Get();
	if (Old == Current)
	{
		return;
	}
	if (Old)
	{
		DetachRider(Old);
	}
	AppliedRider = Current;
	if (Current)
	{
		AttachRider(Current);
	}
	OnMountChanged.Broadcast(this, Current ? Current : Old, Current != nullptr);
}

void UGrimMountComponent::AttachRider(ACharacter* InRider)
{
	using namespace GrimMountLocal;
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !InRider)
	{
		return;
	}
	if (UCharacterMovementComponent* RiderMove = InRider->GetCharacterMovement())
	{
		RiderMove->StopMovementImmediately();
		RiderMove->DisableMovement();
		RiderMove->SetComponentTickEnabled(false);
	}
	InRider->SetActorEnableCollision(false);

	USkeletalMeshComponent* CreatureMesh = Creature->GetMesh();
	if (CreatureMesh && CreatureMesh->DoesSocketExist(SaddleSocket))
	{
		InRider->AttachToComponent(CreatureMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SaddleSocket);
	}
	else
	{
		InRider->AttachToComponent(Creature->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		InRider->SetActorRelativeLocation(GetSaddleOffset(Creature));
		InRider->SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	// 탄 사람의 마법은 자기 탈것을 맞히지 않는다(볼트 투사체는 WorldDynamic).
	if (UCapsuleComponent* Capsule = Creature->GetCapsuleComponent())
	{
		SavedWorldDynamicResponse = Capsule->GetCollisionResponseToChannel(ECC_WorldDynamic);
		Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}

	// 그리모어 거두기(Q25): 탄 사람 ASC 에 State.Mounted(루즈 — 각 기기가 붙인다).
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InRider))
	{
		ASC->SetLooseGameplayTagCount(GrimMagicFXTags::State_Mounted, 1);
	}
	if (UAbilitySystemComponent* CreatureASC = Creature->GetAbilitySystemComponent())
	{
		CreatureASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Ridden, 1);
	}

	// 카메라: 탄 사람 화면에서만 붐을 늘린다.
	if (InRider->IsLocallyControlled())
	{
		if (const AGrimPlayerCharacter* Player = Cast<AGrimPlayerCharacter>(InRider))
		{
			if (USpringArmComponent* Boom = Player->GetCameraBoom())
			{
				const FGrimMountProfile* Profile = UGrimTamingSettings::Get()->FindMountProfile(Creature->GetInstance().SpeciesId);
				SavedArmLength = Boom->TargetArmLength;
				Boom->TargetArmLength += Profile ? Profile->CameraArmExtra : 150.f;
			}
		}
	}
	UE_LOG(LogGrimTaming, Display, TEXT("[%s] 탑승 붙임 %s seed=%d 탄 사람=%s"), RoleName(GetWorld()), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed, *InRider->GetName());
}

void UGrimMountComponent::DetachRider(ACharacter* OldRider)
{
	using namespace GrimMountLocal;
	AGrimCreature* Creature = GetCreature();
	if (!OldRider)
	{
		return;
	}
	OldRider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	OldRider->SetActorEnableCollision(true);
	if (Creature)
	{
		// 생물 오른쪽에 내려 선다(서버 · 탄 사람 클라이언트가 같은 자리를 구한다).
		const FGrimMountProfile* Profile = UGrimTamingSettings::Get()->FindMountProfile(Creature->GetInstance().SpeciesId);
		const float Side = (Profile ? Profile->DismountSideOffset : 150.f) * FMath::Max(0.1f, Creature->GetInstance().SizeScale);
		FVector Location = Creature->GetActorLocation() + Creature->GetActorRightVector() * Side;
		Location.Z = Creature->GetActorLocation().Z - Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OldRider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
		OldRider->SetActorLocationAndRotation(Location, FRotator(0.f, Creature->GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (UCapsuleComponent* Capsule = Creature->GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, SavedWorldDynamicResponse);
		}
		if (UAbilitySystemComponent* CreatureASC = Creature->GetAbilitySystemComponent())
		{
			CreatureASC->SetLooseGameplayTagCount(GrimTamingTags::Creature_State_Ridden, 0);
		}
	}
	if (UCharacterMovementComponent* RiderMove = OldRider->GetCharacterMovement())
	{
		RiderMove->SetComponentTickEnabled(true);
		RiderMove->SetMovementMode(MOVE_Falling);
	}
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OldRider))
	{
		ASC->SetLooseGameplayTagCount(GrimMagicFXTags::State_Mounted, 0);
	}
	if (SavedArmLength >= 0.f)
	{
		if (const AGrimPlayerCharacter* Player = Cast<AGrimPlayerCharacter>(OldRider))
		{
			if (USpringArmComponent* Boom = Player->GetCameraBoom())
			{
				Boom->TargetArmLength = SavedArmLength;
			}
		}
		SavedArmLength = -1.f;
	}
	if (AppliedRider.Get() == OldRider)
	{
		AppliedRider = nullptr;
	}
	UE_LOG(LogGrimTaming, Display, TEXT("[%s] 탑승 뗌 %s 탄 사람=%s"), RoleName(GetWorld()), Creature ? *Creature->GetInstance().SpeciesId.ToString() : TEXT("?"), *OldRider->GetName());
}

// ───────────────────────── 서버: 틱 ─────────────────────────

void UGrimMountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AGrimCreature* Creature = GetCreature();
	if (!Creature || !Creature->HasAuthority() || !Rider)
	{
		return;
	}
	if (Creature->IsDead() || !IsValid(Rider))
	{
		Dismount(true);
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	ApplyRideSpeed();
	if (Now - LastInputTime <= UGrimTamingSettings::Get()->Mount.InputTimeoutSeconds && !PendingInput.IsNearlyZero())
	{
		Creature->AddMovementInput(PendingInput, 1.f);
	}
	TickWaterRun(DeltaTime);
	if (Now >= NextBuckCheckTime)
	{
		NextBuckCheckTime = Now + FMath::Max(1.f, UGrimTamingSettings::Get()->Loyalty.BuckCheckSeconds);
		TickBuck();
	}
}

void UGrimMountComponent::TickBuck()
{
	AGrimCreature* Creature = GetCreature();
	const UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	if (Taming && GrimTamingMath::RollBuck(Taming->GetStage(), UGrimTamingSettings::Get()->Loyalty, FMath::FRand()))
	{
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 반항 — 탄 사람을 떨어뜨림 %s seed=%d"), *Creature->GetInstance().SpeciesId.ToString(), Creature->GetInstance().Seed);
		Dismount(true);
	}
}

void UGrimMountComponent::TickWaterRun(float DeltaTime)
{
	AGrimCreature* Creature = GetCreature();
	const UGrimCreatureSpecies* Species = Creature ? Creature->GetSpecies() : nullptr;
	UCharacterMovementComponent* Move = Creature ? Creature->GetCharacterMovement() : nullptr;
	if (!Species || !Move || Species->WaterMovement != EGrimCreatureWaterMovement::RunOnSurface)
	{
		return;
	}
	const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Location = Creature->GetActorLocation();
	const FVector Foot = Location - FVector(0.f, 0.f, HalfHeight);
	const FGrimMountTuning& Tuning = UGrimTamingSettings::Get()->Mount;

	// 물 볼륨에 들어갔다 → 수면에 붙어 달린다.
	if (!bWaterRunning && Move->IsSwimming())
	{
		APhysicsVolume* Volume = Move->GetPhysicsVolume();
		if (Volume && Volume->bWaterVolume)
		{
			WaterVolume = Volume;
			bWaterRunning = true;
			Move->SetMovementMode(MOVE_Flying);
			UE_LOG(LogGrimTaming, Display, TEXT("[Server] 물 위 달리기 시작 %s seed=%d"), *Species->SpeciesId.ToString(), Creature->GetInstance().Seed);
		}
	}
	if (bWaterRunning)
	{
		APhysicsVolume* Volume = WaterVolume.Get();
		const UBrushComponent* Brush = Volume ? Volume->GetBrushComponent() : nullptr;
		const FBox Box = Brush ? Brush->Bounds.GetBox() : (Volume ? Volume->GetComponentsBoundingBox() : FBox(ForceInit));
		// 발아래가 더는 물이 아니면(물가에 닿음) 내려선다.
		if (!Volume || !Volume->EncompassesPoint(Foot - FVector(0.f, 0.f, 20.f)))
		{
			bWaterRunning = false;
			WaterVolume = nullptr;
			Move->SetMovementMode(MOVE_Falling);
			UE_LOG(LogGrimTaming, Display, TEXT("[Server] 물 위 달리기 끝 %s seed=%d"), *Species->SpeciesId.ToString(), Creature->GetInstance().Seed);
			return;
		}
		if (Box.IsValid)
		{
			const float TargetZ = Box.Max.Z + Tuning.WaterRunSurfaceOffset + HalfHeight;
			if (!FMath::IsNearlyEqual(Location.Z, TargetZ, 1.f))
			{
				Creature->SetActorLocation(FVector(Location.X, Location.Y, TargetZ), false, nullptr, ETeleportType::None);
			}
			Move->Velocity.Z = 0.f;
		}
		return;
	}

	// 물 볼륨이 없는 맵: 물 태그 표면을 밟고 있으면 그대로 달린다(충돌이 있는 표면 — 자리).
	if (!bLoggedTaggedSurface && Tuning.WaterSurfaceTag != NAME_None && Move->CurrentFloor.IsWalkableFloor())
	{
		const FHitResult& Floor = Move->CurrentFloor.HitResult;
		const AActor* FloorActor = Floor.GetActor();
		const UPrimitiveComponent* FloorComponent = Floor.GetComponent();
		if ((FloorActor && FloorActor->ActorHasTag(Tuning.WaterSurfaceTag)) || (FloorComponent && FloorComponent->ComponentHasTag(Tuning.WaterSurfaceTag)))
		{
			bLoggedTaggedSurface = true;
			UE_LOG(LogGrimTaming, Display, TEXT("[Server] 물 표면(태그 %s) 위 달리기 %s seed=%d"), *Tuning.WaterSurfaceTag.ToString(), *Species->SpeciesId.ToString(), Creature->GetInstance().Seed);
		}
	}
}
