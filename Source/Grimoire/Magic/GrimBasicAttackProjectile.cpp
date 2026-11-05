#include "Magic/GrimBasicAttackProjectile.h"
#include "Magic/GrimBasicAttackTypes.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

void AGrimBasicAttackProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AGrimBasicAttackProjectile, bHeavy, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGrimBasicAttackProjectile, HeavyScale, COND_InitialOnly);
}

void AGrimBasicAttackProjectile::SetHeavy(float InKnockbackSpeed, float InKnockbackUpSpeed, float InPhysicsSpeed, float InVisualScale)
{
	bHeavy = true;
	HeavyScale = FMath::Max(InVisualScale, 0.1f);
	KnockbackSpeed = InKnockbackSpeed;
	KnockbackUpSpeed = InKnockbackUpSpeed;
	PhysicsSpeed = InPhysicsSpeed;
	ApplyHeavyScale();
}

void AGrimBasicAttackProjectile::BeginPlay()
{
	Super::BeginPlay();
	// 클라이언트 사본: 정의 복제(OnRep_Definition)가 크기를 되돌렸을 수 있다.
	if (bHeavy)
	{
		ApplyHeavyScale();
	}
}

void AGrimBasicAttackProjectile::OnRep_Heavy()
{
	ApplyHeavyScale();
}

void AGrimBasicAttackProjectile::ApplyHeavyScale()
{
	if (!bHeavy || !Definition || Definition->Radius <= 0.f)
	{
		return;
	}
	const float Radius = Definition->Radius * HeavyScale;
	Collision->SetSphereRadius(Radius);
	// 기본 구 메시는 지름 100.
	Mesh->SetRelativeScale3D(FVector(Radius / 50.f));
}

void AGrimBasicAttackProjectile::HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult)
{
	// 부모와 같은 거름(서버 · 진짜 · 시전자 아님 · 처음 맞음) — 부모가 맞음 처리 뒤 Destroy 하므로 먼저 민다.
	const bool bFirstHit = HitActor && !HitActors.Contains(HitActor) && HitActor != GetInstigator() && HitActor != GetOwner() && HitActor != this;
	if (bHeavy && bFirstHit && HasAuthority() && !IsFake() && !IsActorBeingDestroyed() && Definition)
	{
		const FVector Direction = Movement->Velocity.IsNearlyZero() ? GetActorForwardVector() : Movement->Velocity.GetSafeNormal();
		if (ApplyKnockback(HitActor, Hit.GetComponent(), GetInstigator(), Direction, KnockbackSpeed, KnockbackUpSpeed, PhysicsSpeed))
		{
			UE_LOG(LogGrimMagic, Display, TEXT("[BasicAttack] 강타 밀어냄 %s"), *HitActor->GetName());
		}
	}
	Super::HandleHit(HitActor, Hit, bHasHitResult);
}

bool AGrimBasicAttackProjectile::ApplyKnockback(AActor* Target, UPrimitiveComponent* HitComponent, const AActor* Instigator, const FVector& TravelDirection,
	float Speed, float UpSpeed, float PhysicsSpeed)
{
	if (!Target || !Target->HasAuthority())
	{
		return false;
	}

	if (ACharacter* Character = Cast<ACharacter>(Target))
	{
		// 협동: 플레이어끼리는 피해처럼 밀어냄도 아군 피해 설정을 따른다.
		const APawn* InstigatorPawn = Cast<APawn>(Instigator);
		const bool bBothPlayers = Character->GetPlayerState() && InstigatorPawn && InstigatorPawn->GetPlayerState();
		if (bBothPlayers && !UGrimMagicSettings::Get()->bFriendlyFire)
		{
			return false;
		}
		Character->LaunchCharacter(FGrimBasicAttackMath::KnockbackVelocity(TravelDirection, Speed, UpSpeed), true, true);
		return true;
	}

	UPrimitiveComponent* Primitive = HitComponent;
	if (!Primitive || Primitive->GetOwner() != Target)
	{
		Primitive = Cast<UPrimitiveComponent>(Target->GetRootComponent());
	}
	if (Primitive && Primitive->IsSimulatingPhysics())
	{
		Primitive->AddImpulse(FGrimBasicAttackMath::KnockbackVelocity(TravelDirection, PhysicsSpeed, PhysicsSpeed * 0.25f), NAME_None, true);
		return true;
	}
	return false;
}
