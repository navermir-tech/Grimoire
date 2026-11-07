#include "MagicFX/GrimSpellChannelCue.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXSettings.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "MagicFX/GrimMagicFXVisualSet.h"
#include "MagicFX/GrimSpellPoseAnimInstance.h"
#include "Core/GrimSpellFXTypes.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"

AGrimCueNotify_SpellChannel::AGrimCueNotify_SpellChannel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCueTag = GrimCueTags::Spell_Channel;
	bAutoDestroyOnRemove = true;
	AutoDestroyDelay = 0.6f; // 줄기 파티클이 흩어질 시간
	bUniqueInstancePerInstigator = true;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // 완드(손)가 이번 프레임 자세로 움직인 뒤

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

bool AGrimCueNotify_SpellChannel::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	StartChannel(MyTarget, Parameters);
	return true;
}

bool AGrimCueNotify_SpellChannel::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 늦게 들어온 사람(이미 누르고 있는 중)도 같은 줄기를 본다. 이미 시작했으면 그대로.
	StartChannel(MyTarget, Parameters);
	return true;
}

bool AGrimCueNotify_SpellChannel::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	StopChannel();
	return true;
}

bool AGrimCueNotify_SpellChannel::Recycle()
{
	StopChannel();
	ActiveVisualSet = nullptr;
	CasterPawn.Reset();
	return Super::Recycle();
}

void AGrimCueNotify_SpellChannel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopChannel();
	Super::EndPlay(EndPlayReason);
}

void AGrimCueNotify_SpellChannel::StartChannel(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (bChanneling)
	{
		return;
	}
	UWorld* World = GetWorld();
	APawn* Pawn = UGrimMagicFXStatics::ResolveInstigatorPawn(MyTarget, Parameters);
	if (!World || !Pawn || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	bChanneling = true;
	CasterPawn = Pawn;
	ActiveVisualSet = UGrimMagicFXStatics::ResolveVisualSet(Parameters, MyTarget);
	ActiveColor = UGrimMagicFXStatics::ResolveColor(ActiveVisualSet, Parameters);
	const UGrimMagicFXVisualSet* Extended = Cast<UGrimMagicFXVisualSet>(ActiveVisualSet);

	// 포물선 값: 마법 정의(갈래 A 의 Stream 필드)가 있으면 그것, 없으면 VisualSet 기본값.
	const UGrimMagicFXSettings* Settings = UGrimMagicFXSettings::Get();
	const UObject* Definition = Parameters.SourceObject.Get();
	StreamDefinition = Cast<UGrimSpellDefinition>(Definition);
	LaunchSpeed = UGrimMagicFXStatics::ReadFloatProperty(Definition, Settings->StreamLaunchSpeedProperty, Extended ? Extended->StreamDefaultLaunchSpeed : 1400.f);
	const float GravityScale = UGrimMagicFXStatics::ReadFloatProperty(Definition, Settings->StreamGravityScaleProperty, Extended ? Extended->StreamDefaultGravityScale : 1.f);
	GravityZ = World->GetGravityZ() * GravityScale;
	FlightTime = Extended ? Extended->StreamFlightTime : 0.6f;

	FVector Muzzle;
	FVector Direction;
	UGrimMagicFXStatics::ResolveMuzzle(Pawn, FGameplayCueParameters(), Muzzle, Direction);
	Direction = UGrimMagicFXStatics::GetAimDirection(Pawn);

	if (ActiveVisualSet)
	{
		TrailComponent = UGrimMagicFXStatics::SpawnFX(World, ActiveVisualSet->Trail, Muzzle, FRotationMatrix::MakeFromZ(Direction).Rotator(), Extended ? Extended->TrailScale : 1.f, ActiveColor, false, 0.f);
		if (ActiveVisualSet->LoopSound)
		{
			LoopAudio = UGameplayStatics::SpawnSoundAtLocation(World, ActiveVisualSet->LoopSound, Muzzle, FRotator::ZeroRotator,
				Extended ? Extended->LoopSoundVolume : 0.25f, Extended ? Extended->LoopSoundPitch : 1.f, 0.f, nullptr, nullptr, false);
		}
	}

	const int32 BeadCount = Extended ? Extended->StreamBeadCount : 0;
	EnsureBeads(BeadCount);
	if (Beads.Num() > 0)
	{
		if (!BeadMaterial)
		{
			if (UMaterialInterface* Glow = UGrimMagicFXStatics::GetGlowMaterial())
			{
				BeadMaterial = UMaterialInstanceDynamic::Create(Glow, this);
			}
		}
		if (BeadMaterial)
		{
			BeadMaterial->SetVectorParameterValue(TEXT("Color"), ActiveColor);
			BeadMaterial->SetScalarParameterValue(TEXT("Glow"), Extended ? Extended->StreamBeadGlow : 3.f);
			BeadMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.9f);
			BeadMaterial->SetScalarParameterValue(TEXT("RimMix"), 0.25f);
		}
		for (UStaticMeshComponent* Bead : Beads)
		{
			Bead->SetMaterial(0, BeadMaterial);
		}
	}

	if (AGrimGrimoireBook* Book = UGrimMagicFXStatics::FindGrimoireBook(Pawn))
	{
		Book->BeginChannelFlip(ActiveColor);
	}
	if (UGrimSpellPoseAnimInstance* Pose = UGrimMagicFXStatics::FindPoseAnimInstance(Pawn))
	{
		Pose->BeginHoldMotion(EGrimCastMotion::Extend);
	}

	SetActorTickEnabled(true);
	UpdateStream();
}

void AGrimCueNotify_SpellChannel::StopChannel()
{
	if (!bChanneling)
	{
		return;
	}
	bChanneling = false;
	SetActorTickEnabled(false);

	UGrimMagicFXStatics::StopFX(TrailComponent);
	TrailComponent = nullptr;

	if (LoopAudio)
	{
		LoopAudio->bAutoDestroy = true;
		LoopAudio->FadeOut(0.2f, 0.f);
		LoopAudio = nullptr;
	}
	for (UStaticMeshComponent* Bead : Beads)
	{
		if (Bead)
		{
			Bead->SetVisibility(false);
		}
	}

	APawn* Pawn = CasterPawn.Get();
	if (AGrimGrimoireBook* Book = UGrimMagicFXStatics::FindGrimoireBook(Pawn))
	{
		Book->EndChannelFlip();
	}
	if (UGrimSpellPoseAnimInstance* Pose = UGrimMagicFXStatics::FindPoseAnimInstance(Pawn))
	{
		Pose->EndHoldMotion();
	}
}

void AGrimCueNotify_SpellChannel::EnsureBeads(int32 Count)
{
	UStaticMesh* Sphere = Count > 0 ? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")) : nullptr;
	while (Beads.Num() < Count && Sphere)
	{
		UStaticMeshComponent* Bead = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Bead->SetStaticMesh(Sphere);
		Bead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Bead->SetGenerateOverlapEvents(false);
		Bead->SetCastShadow(false);
		Bead->bReceivesDecals = false;
		Bead->SetupAttachment(RootComponent);
		Bead->SetUsingAbsoluteLocation(true);
		Bead->SetUsingAbsoluteScale(true);
		Bead->RegisterComponent();
		Beads.Add(Bead);
	}
	for (int32 Index = 0; Index < Beads.Num(); ++Index)
	{
		Beads[Index]->SetVisibility(Index < Count);
	}
}

void AGrimCueNotify_SpellChannel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bChanneling)
	{
		UpdateStream();
	}
}

void AGrimCueNotify_SpellChannel::UpdateStream()
{
	const APawn* Pawn = CasterPawn.Get();
	UWorld* World = GetWorld();
	if (!Pawn || !World)
	{
		return;
	}

	FVector Muzzle;
	FVector Unused;
	UGrimMagicFXStatics::ResolveMuzzle(Pawn, FGameplayCueParameters(), Muzzle, Unused);
	const FVector Direction = UGrimMagicFXStatics::GetAimDirection(Pawn);
	SetActorLocation(Muzzle);

	// 판정과 같은 호: 갈래 A 의 UGrimSpellAbility::TraceStreamArc 가 내는 점들(판정은 서버 — 이건 그림만).
	const UGrimSpellDefinition* Def = StreamDefinition.Get();
	TArray<FVector> ArcPoints;
	bool bArcBlocked = false;
	FVector LaunchDirection = Direction;
	float Speed = LaunchSpeed;
	float ArcTime = FlightTime;
	if (Def)
	{
		const FVector AimPoint = Muzzle + Direction * Def->Range;
		bArcBlocked = UGrimSpellAbility::TraceStreamArc(World, Def, Muzzle, AimPoint, Pawn, ArcPoints, nullptr);
		GrimSpellDelivery::SolveLaunchDirection(Muzzle, AimPoint, Def->StreamLaunchSpeed, GravityZ, LaunchDirection);
		Speed = Def->StreamLaunchSpeed;
		ArcTime = ArcPoints.Num() >= 2
			? GrimSpellDelivery::CalcArcTimeToPoint(Muzzle, LaunchDirection * Speed, ArcPoints.Last(), Def->GetStreamFlightTime())
			: Def->GetStreamFlightTime();
	}

	if (TrailComponent)
	{
		// 몸통 Niagara(NS_Stream_Trail)는 +Z 로 User.LaunchSpeed 만큼 쏘고 User.GravityZ 로 떨어뜨린다 — 판정 호와 같은 포물선.
		// 물방울은 호가 막힌 곳(User.ArcTime 뒤)에서 끝나고, 막혔으면 그 자리(User.EndPoint)에서 튄다.
		TrailComponent->SetWorldLocationAndRotation(Muzzle, FRotationMatrix::MakeFromZ(LaunchDirection).Rotator());
		if (UNiagaraComponent* Niagara = Cast<UNiagaraComponent>(TrailComponent))
		{
			Niagara->SetVariableFloat(TEXT("LaunchSpeed"), Speed);
			Niagara->SetVariableFloat(TEXT("GravityZ"), GravityZ);
			Niagara->SetVariableFloat(TEXT("ArcTime"), FMath::Max(ArcTime, 0.02f));
			Niagara->SetVariableFloat(TEXT("EndSplash"), bArcBlocked ? 1.f : 0.f);
			Niagara->SetVariableVec3(TEXT("EndPoint"), ArcPoints.Num() > 0 ? ArcPoints.Last() : Muzzle + LaunchDirection * Speed * ArcTime);
		}
	}
	if (LoopAudio)
	{
		LoopAudio->SetWorldLocation(Muzzle);
		if (!LoopAudio->IsPlaying())
		{
			LoopAudio->Play(); // 임시 소리(엔진 WhiteNoise)는 반복 설정이 없어 다시 튼다.
		}
	}

	const UGrimMagicFXVisualSet* Extended = Cast<UGrimMagicFXVisualSet>(ActiveVisualSet);
	const int32 Count = Extended ? FMath::Min(Extended->StreamBeadCount, Beads.Num()) : 0;
	if (Count <= 0)
	{
		return;
	}

	// 구슬도 위의 판정 호를 따라 놓는다.
	if (Def)
	{
		if (ArcPoints.Num() >= 2)
		{
			for (int32 Index = 0; Index < Beads.Num(); ++Index)
			{
				UStaticMeshComponent* Bead = Beads[Index];
				if (Index >= Count)
				{
					Bead->SetVisibility(false);
					continue;
				}
				// 호 전체 길이를 구슬 수로 나눠 점 사이를 보간한다.
				const float Alpha = static_cast<float>(Index + 1) / Count * (ArcPoints.Num() - 1);
				const int32 Segment = FMath::Min(FMath::FloorToInt(Alpha), ArcPoints.Num() - 2);
				const FVector Point = FMath::Lerp(ArcPoints[Segment], ArcPoints[Segment + 1], Alpha - Segment);
				const float Size = Extended->StreamBeadSize * FMath::Lerp(1.f, 1.6f, static_cast<float>(Index) / Count);
				Bead->SetVisibility(true);
				Bead->SetWorldLocation(Point);
				Bead->SetWorldScale3D(FVector(Size / 100.f));
			}
			return;
		}
	}

	// 정의가 없을 때(VisualSet 만 온 큐)는 같은 속도 · 중력의 포물선 근사.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GrimStreamVisual), false, Pawn);
	Query.AddIgnoredActor(this);
	FVector Previous = Muzzle;
	bool bBlocked = false;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Bead = Beads[Index];
		if (bBlocked)
		{
			Bead->SetVisibility(false);
			continue;
		}
		const float Time = FlightTime * static_cast<float>(Index + 1) / Count;
		FVector Point = GrimMagicFXMath::CalcStreamPoint(Muzzle, Direction, LaunchSpeed, GravityZ, Time);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Previous, Point, ECC_Visibility, Query))
		{
			Point = Hit.ImpactPoint;
			bBlocked = true;
		}
		const float Size = Extended->StreamBeadSize * FMath::Lerp(1.f, 1.6f, static_cast<float>(Index) / Count);
		Bead->SetVisibility(true);
		Bead->SetWorldLocation(Point);
		Bead->SetWorldScale3D(FVector(Size / 100.f));
		Previous = Point;
	}
}
