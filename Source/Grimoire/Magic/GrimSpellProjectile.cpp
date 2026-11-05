#include "Magic/GrimSpellProjectile.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimMagicSettings.h"
#include "Core/GrimSpellFXTypes.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

AGrimSpellProjectile::AGrimSpellProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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
	DOREPLIFETIME_CONDITION(AGrimSpellProjectile, HomingTarget, COND_InitialOnly);
}

void AGrimSpellProjectile::InitSpell(const UGrimSpellDefinition* InDefinition, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec, AActor* InHomingTarget)
{
	Definition = InDefinition;
	SourceASC = InSourceASC;
	DamageSpec = InDamageSpec;
	HomingTarget = InHomingTarget;
	ApplyDefinitionVisuals();
}

void AGrimSpellProjectile::InitFake(const UGrimSpellDefinition* InDefinition, AActor* InHomingTarget)
{
	bFake = true;
	SetReplicates(false);
	SetReplicatingMovement(false);

	// 판정 없음: 겹침 이벤트를 끄고 폰 · 물리체는 지나간다. 지형 · 사물에서는 멈춰 사라진다(터짐은 서버 Impact 큐가 그린다).
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);

	Definition = InDefinition;
	HomingTarget = InHomingTarget;
	ApplyDefinitionVisuals();
}

void AGrimSpellProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* InstigatorPawn = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	if (Definition && Definition->BoltSpeed > 0.f && Definition->Range > 0.f && (HasAuthority() || bFake))
	{
		SetLifeSpan(Definition->Range / Definition->BoltSpeed);
	}

	if (HasAuthority() && !bFake)
	{
		Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	}
	Movement->OnProjectileStop.AddDynamic(this, &ThisClass::OnProjectileStop);

	if (IsHiddenForLocalCaster())
	{
		SetActorHiddenInGame(true);
	}
	else
	{
		StartTrail();
	}

	SetActorTickEnabled(HomingTarget.IsValid() && Definition && Definition->HomingStrength > 0.f);
	// 유도는 이동보다 먼저 속도를 바꾼다.
	Movement->AddTickPrerequisiteActor(this);
}

void AGrimSpellProjectile::Destroyed()
{
	// 시전자 클라이언트: 진짜가 끝나면(맞음 · 수명) 같은 마법의 가장 오래된 가짜도 끝낸다 — 그 자리는 Impact 큐가 맡는다.
	if (!HasAuthority() && !bFake && GetWorld() && IsHiddenForLocalCaster())
	{
		AGrimSpellProjectile* Oldest = nullptr;
		for (TActorIterator<AGrimSpellProjectile> It(GetWorld()); It; ++It)
		{
			if (It->bFake && It->Definition == Definition && It->GetInstigator() == GetInstigator() && !It->IsActorBeingDestroyed())
			{
				if (!Oldest || It->CreationTime < Oldest->CreationTime)
				{
					Oldest = *It;
				}
			}
		}
		if (Oldest)
		{
			Oldest->Destroy();
		}
	}
	Super::Destroyed();
}

bool AGrimSpellProjectile::IsHiddenForLocalCaster() const
{
	if (bFake || HasAuthority())
	{
		return false;
	}
	const APawn* InstigatorPawn = GetInstigator();
	return InstigatorPawn && InstigatorPawn->IsLocallyControlled();
}

void AGrimSpellProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* Target = HomingTarget.Get();
	if (!Target || !Definition || Definition->HomingStrength <= 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	Movement->Velocity = GrimSpellDelivery::SteerTowards(Movement->Velocity, GetActorLocation(), UGrimSpellAbility::GetTargetPointOf(Target),
		Definition->HomingStrength, UGrimMagicSettings::Get()->HomingMaxAngle, DeltaSeconds);
}

void AGrimSpellProjectile::CatchUp(float Seconds)
{
	if (Seconds <= 0.f || IsActorBeingDestroyed())
	{
		return;
	}
	// 유도 한 번 + 그만큼 이동(스윕 · 겹침 판정 포함). UT 의 CatchupTick 과 같은 방식.
	if (AActor* Target = HomingTarget.Get(); Target && Definition && Definition->HomingStrength > 0.f)
	{
		Movement->Velocity = GrimSpellDelivery::SteerTowards(Movement->Velocity, GetActorLocation(), UGrimSpellAbility::GetTargetPointOf(Target),
			Definition->HomingStrength, UGrimMagicSettings::Get()->HomingMaxAngle, Seconds);
	}
	Movement->TickComponent(Seconds, LEVELTICK_All, nullptr);
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

	if (Definition->Delivery == EGrimSpellDelivery::Lob)
	{
		Movement->ProjectileGravityScale = 1.f;
	}

	if (Definition->BoltSpeed > 0.f)
	{
		Movement->InitialSpeed = Definition->BoltSpeed;
		Movement->MaxSpeed = Definition->BoltSpeed;
		// 이미 시작한 클라이언트 사본도 앞으로 날게 한다 — 위치는 복제가 바로잡는다.
		if (Movement->HasBegunPlay() || (!HasAuthority() && !bFake))
		{
			Movement->Velocity = GetActorForwardVector() * Definition->BoltSpeed;
		}
	}

	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		const FLinearColor Color = Definition->VisualSet ? Definition->VisualSet->ElementColor : Definition->Color;
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AGrimSpellProjectile::StartTrail()
{
	if (TrailComponent || !Definition || !Definition->VisualSet || !Definition->VisualSet->Trail || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UFXSystemAsset* Trail = Definition->VisualSet->Trail;
	const FLinearColor Color = Definition->VisualSet->ElementColor;
	if (UNiagaraSystem* Niagara = Cast<UNiagaraSystem>(Trail))
	{
		UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAttached(Niagara, Collision, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		if (Component)
		{
			Component->SetVariableLinearColor(TEXT("Color"), Color);
		}
		TrailComponent = Component;
	}
	else if (UParticleSystem* Cascade = Cast<UParticleSystem>(Trail))
	{
		UParticleSystemComponent* Component = UGameplayStatics::SpawnEmitterAttached(Cascade, Collision, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		if (Component)
		{
			Component->SetColorParameter(TEXT("Color"), Color);
		}
		TrailComponent = Component;
	}
}

void AGrimSpellProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleHit(OtherActor, SweepResult, bFromSweep);
}

void AGrimSpellProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
	if (bFake)
	{
		Destroy();
		return;
	}
	HandleHit(ImpactResult.GetActor(), ImpactResult, true);
}

void AGrimSpellProjectile::HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult)
{
	if (!HasAuthority() || bFake || IsActorBeingDestroyed() || !Definition)
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

	// 터짐은 서버 확정 뒤 큐로 모두에게(시전자 포함).
	const FVector Location = bHasHitResult && !Hit.ImpactPoint.IsNearlyZero() ? FVector(Hit.ImpactPoint) : GetActorLocation();
	const FVector Normal = bHasHitResult && !Hit.ImpactNormal.IsNearlyZero() ? FVector(Hit.ImpactNormal) : -GetActorForwardVector();
	UGrimSpellAbility::ExecuteSpellCue(SourceASC.Get(), GrimCueTags::Spell_Impact, Definition, GetInstigator(), Location, Normal);
	UE_LOG(LogGrimMagic, Display, TEXT("[Bolt] %s 맞음 %s"), *Definition->GetName(), *GetNameSafe(HitActor));

	Destroy();
}
