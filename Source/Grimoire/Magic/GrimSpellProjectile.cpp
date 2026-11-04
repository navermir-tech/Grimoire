#include "Magic/GrimSpellProjectile.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGrimSpellProjectile::AGrimSpellProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(10.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 지형 · 사물에는 막히고(맞음), 폰 · 물리체는 겹치며 맞는다.
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
}

void AGrimSpellProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AGrimSpellProjectile, Definition, COND_InitialOnly);
}

void AGrimSpellProjectile::InitSpell(const UGrimSpellDefinition* InDefinition, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec)
{
	Definition = InDefinition;
	SourceASC = InSourceASC;
	DamageSpec = InDamageSpec;
	ApplyDefinitionVisuals();
}

void AGrimSpellProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* InstigatorPawn = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	if (HasAuthority())
	{
		Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		Movement->OnProjectileStop.AddDynamic(this, &ThisClass::OnProjectileStop);

		if (Definition && Definition->ProjectileSpeed > 0.f && Definition->Range > 0.f)
		{
			SetLifeSpan(Definition->Range / Definition->ProjectileSpeed);
		}
	}
}

void AGrimSpellProjectile::OnRep_Definition()
{
	ApplyDefinitionVisuals();
}

void AGrimSpellProjectile::ApplyDefinitionVisuals()
{
	if (!Definition)
	{
		return;
	}

	if (Definition->Radius > 0.f)
	{
		Collision->SetSphereRadius(Definition->Radius);
		// 기본 구 메시는 지름 100.
		Mesh->SetRelativeScale3D(FVector(Definition->Radius / 50.f));
	}

	if (Definition->ProjectileSpeed > 0.f)
	{
		Movement->InitialSpeed = Definition->ProjectileSpeed;
		Movement->MaxSpeed = Definition->ProjectileSpeed;
		// 이미 시작한 클라이언트 사본도 앞으로 날게 한다 — 위치는 복제가 바로잡는다.
		if (Movement->HasBegunPlay() || !HasAuthority())
		{
			Movement->Velocity = GetActorForwardVector() * Definition->ProjectileSpeed;
		}
	}

	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Definition->Color);
	}
}

void AGrimSpellProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleHit(OtherActor, SweepResult);
}

void AGrimSpellProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
	HandleHit(ImpactResult.GetActor(), ImpactResult);
}

void AGrimSpellProjectile::HandleHit(AActor* HitActor, const FHitResult& Hit)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !Definition)
	{
		return;
	}
	if (HitActor && (HitActor == GetInstigator() || HitActor == GetOwner() || HitActor == this))
	{
		return;
	}
	if (HitActor)
	{
		if (HitActors.Contains(HitActor))
		{
			return;
		}
		HitActors.Add(HitActor);
		UGrimSpellAbility::ApplySpellHit(Definition, SourceASC.Get(), DamageSpec, GetInstigator(), HitActor);
	}
	Destroy();
}
