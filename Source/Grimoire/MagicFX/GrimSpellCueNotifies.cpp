#include "MagicFX/GrimSpellCueNotifies.h"
#include "MagicFX/GrimFXPulse.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXSettings.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "MagicFX/GrimMagicFXVisualSet.h"
#include "MagicFX/GrimSpellPoseAnimInstance.h"
#include "Core/GrimSpellFXTypes.h"
#include "Character/GrimPlayerCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimMagicFX, Log, All);

namespace
{
	FRotator AxisRotation(const FVector& Axis)
	{
		// 엔진 기본 파티클 템플릿은 대개 +Z 로 뿜는다 — +Z 를 방향에 맞춘다.
		return FRotationMatrix::MakeFromZ(Axis.IsNearlyZero() ? FVector::UpVector : Axis.GetSafeNormal()).Rotator();
	}

	UWorld* ResolveWorld(const AActor* MyTarget, const APawn* Pawn)
	{
		if (MyTarget)
		{
			return MyTarget->GetWorld();
		}
		return Pawn ? Pawn->GetWorld() : nullptr;
	}
}

// ---- Cast ----

UGrimCueNotify_SpellCast::UGrimCueNotify_SpellCast(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCueTag = GrimCueTags::Spell_Cast;
}

namespace
{
	/** 완드 끝 섬광 · 소리 · 흔들림 · 원뿔 파동. 실제 발사 순간(CastWindup 뒤)에 맞춰 부른다. Bolt 꼬리는 투사체(갈래 A)가 붙인다. */
	void PlayCastBurst(UWorld* World, const APawn* Pawn, const UGrimSpellVisualSet* VisualSet, const FLinearColor& Color, EGrimCastMotion Motion,
		const FVector& Muzzle, const FVector& Direction, const UObject* Definition)
	{
		const UGrimMagicFXVisualSet* Extended = Cast<UGrimMagicFXVisualSet>(VisualSet);
		if (VisualSet)
		{
			const float StopAfter = Extended ? Extended->OneShotStopSeconds : 0.6f;
			UGrimMagicFXStatics::SpawnFX(World, VisualSet->MuzzleFlash, Muzzle, AxisRotation(Direction), Extended ? Extended->MuzzleFlashScale : 1.f, Color, true, StopAfter);
			UGrimMagicFXStatics::PlaySound(World, VisualSet->CastSound, Muzzle, Extended ? Extended->CastSoundVolume : 0.4f, Extended ? Extended->CastSoundPitch : 1.f, 0.f);
			UGrimMagicFXStatics::PlayCasterCameraShake(Pawn, VisualSet->CameraShake, Extended ? Extended->CameraShakeScale : 1.f);

			// 원뿔 마법(쓸기)은 발사 순간에 파동이 다 나간다 — 몸통(Trail) 도 여기서. Bolt · Stream 몸통은 투사체 · Channel 큐가 그린다.
			if (Motion == EGrimCastMotion::Sweep && VisualSet->Trail)
			{
				UGrimMagicFXStatics::SpawnFX(World, VisualSet->Trail, Muzzle, AxisRotation(Direction), Extended ? Extended->TrailScale : 1.f, Color, true, StopAfter);
			}
		}

		if (Extended)
		{
			AGrimFXPulse::Spawn(World, Extended->MuzzlePulse, Muzzle, Direction, Color);
			if (Extended->CastWave.IsEnabled())
			{
				FVector WaveSize = FVector::ZeroVector;
				if (Extended->CastWave.Shape == EGrimFXPulseShape::Cone)
				{
					const UGrimMagicFXSettings* Settings = UGrimMagicFXSettings::Get();
					const float Range = UGrimMagicFXStatics::ReadFloatProperty(Definition, Settings->ConeRangeProperty, Settings->DefaultConeRange);
					const float HalfAngle = UGrimMagicFXStatics::ReadFloatProperty(Definition, Settings->ConeHalfAngleProperty, Settings->DefaultConeHalfAngle);
					WaveSize = GrimMagicFXMath::CalcConeWaveSize(Range, HalfAngle);
				}
				AGrimFXPulse::Spawn(World, Extended->CastWave, Muzzle, Direction, Color, WaveSize);
			}
		}
	}
}

bool UGrimCueNotify_SpellCast::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APawn* Pawn = UGrimMagicFXStatics::ResolveInstigatorPawn(MyTarget, Parameters);
	UWorld* World = ResolveWorld(MyTarget, Pawn);
	if (!World)
	{
		return false;
	}

	const UGrimSpellVisualSet* VisualSet = UGrimMagicFXStatics::ResolveVisualSet(Parameters, MyTarget);
	const FLinearColor Color = UGrimMagicFXStatics::ResolveColor(VisualSet, Parameters);
	const EGrimCastMotion Motion = UGrimMagicFXStatics::ResolveCastMotion(VisualSet, Parameters);
	const UObject* Definition = Parameters.SourceObject.Get();
	if (!VisualSet)
	{
		UE_LOG(LogGrimMagicFX, Verbose, TEXT("시전 큐: 연출 묶음을 찾지 못했다(%s)."), *GetNameSafe(Definition));
	}

	// 동작 · 책장 · 몸 돌리기는 누르는 순간(큐 시점)에. 큐라서 시전자는 예측으로, 다른 사람은 복제로 본다 (R21).
	if (AGrimGrimoireBook* Book = UGrimMagicFXStatics::FindGrimoireBook(Pawn))
	{
		Book->FlipPage(Color);
	}
	if (UGrimSpellPoseAnimInstance* Pose = UGrimMagicFXStatics::FindPoseAnimInstance(Pawn))
	{
		Pose->PlayCastMotion(Motion);
	}
	if (AGrimPlayerCharacter* Character = Cast<AGrimPlayerCharacter>(Pawn))
	{
		Character->FaceAimForCast();
	}

	// 섬광은 실제 발사(정의의 CastWindup, 불씨 0.1초) 뒤 — 그때 완드 끝 위치로 다시 잡는다.
	const float Windup = UGrimMagicFXStatics::ReadFloatProperty(Definition, UGrimMagicFXSettings::Get()->CastWindupProperty, 0.f);
	if (Windup <= KINDA_SMALL_NUMBER)
	{
		FVector Muzzle;
		FVector Direction;
		UGrimMagicFXStatics::ResolveMuzzle(Pawn, Parameters, Muzzle, Direction);
		PlayCastBurst(World, Pawn, VisualSet, Color, Motion, Muzzle, Direction, Definition);
		return true;
	}

	FVector FallbackMuzzle;
	FVector FallbackDirection;
	UGrimMagicFXStatics::ResolveMuzzle(Pawn, Parameters, FallbackMuzzle, FallbackDirection);
	TWeakObjectPtr<UWorld> WeakWorld(World);
	TWeakObjectPtr<const APawn> WeakPawn(Pawn);
	TWeakObjectPtr<const UGrimSpellVisualSet> WeakVisualSet(VisualSet);
	TWeakObjectPtr<const UObject> WeakDefinition(Definition);
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(World, [WeakWorld, WeakPawn, WeakVisualSet, WeakDefinition, Color, Motion, FallbackMuzzle, FallbackDirection]()
	{
		UWorld* LiveWorld = WeakWorld.Get();
		if (!LiveWorld)
		{
			return;
		}
		const APawn* LivePawn = WeakPawn.Get();
		FVector Muzzle = FallbackMuzzle;
		FVector Direction = FallbackDirection;
		if (LivePawn && UGrimMagicFXStatics::FindWand(LivePawn))
		{
			// 휘두르는 사이 손이 움직였다 — 지금 완드 끝 · 조준 방향.
			UGrimMagicFXStatics::ResolveMuzzle(LivePawn, FGameplayCueParameters(), Muzzle, Direction);
		}
		PlayCastBurst(LiveWorld, LivePawn, WeakVisualSet.Get(), Color, Motion, Muzzle, Direction, WeakDefinition.Get());
	}), Windup, false);
	return true;
}
// ---- Impact ----

UGrimCueNotify_SpellImpact::UGrimCueNotify_SpellImpact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCueTag = GrimCueTags::Spell_Impact;
}

bool UGrimCueNotify_SpellImpact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const APawn* Pawn = UGrimMagicFXStatics::ResolveInstigatorPawn(MyTarget, Parameters);
	UWorld* World = ResolveWorld(MyTarget, Pawn);
	if (!World)
	{
		return false;
	}

	const FVector Location = Parameters.Location.IsNearlyZero() && MyTarget ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
	const FVector Normal = Parameters.Normal.IsNearlyZero() ? FVector::UpVector : FVector(Parameters.Normal).GetSafeNormal();

	const UGrimSpellVisualSet* VisualSet = UGrimMagicFXStatics::ResolveVisualSet(Parameters, MyTarget);
	const UGrimMagicFXVisualSet* Extended = Cast<UGrimMagicFXVisualSet>(VisualSet);
	const FLinearColor Color = UGrimMagicFXStatics::ResolveColor(VisualSet, Parameters);
	if (!VisualSet)
	{
		return false;
	}

	UGrimMagicFXStatics::SpawnFX(World, VisualSet->Impact, Location, AxisRotation(Normal), Extended ? Extended->ImpactScale : 1.f, Color, true, Extended ? Extended->OneShotStopSeconds : 0.6f);
	UGrimMagicFXStatics::PlaySound(World, VisualSet->ImpactSound, Location, Extended ? Extended->ImpactSoundVolume : 0.4f, Extended ? Extended->ImpactSoundPitch : 1.f, Extended ? Extended->ImpactSoundDuration : 0.f);

	if (Extended)
	{
		AGrimFXPulse::Spawn(World, Extended->ImpactPulse, Location + Normal * 2.f, Normal, Color);
		UGrimMagicFXStatics::SpawnImpactDecal(World, Extended->ImpactDecal, Location, Normal);
	}
	return true;
}
