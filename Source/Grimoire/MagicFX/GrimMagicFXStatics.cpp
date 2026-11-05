#include "MagicFX/GrimMagicFXStatics.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXSettings.h"
#include "MagicFX/GrimMagicFXVisualSet.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimSpellPoseAnimInstance.h"
#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimPlayerState.h"
#include "Core/GrimSpellFXTypes.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimWand.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName CascadeColorParameter(TEXT("Color"));

	const UGrimSpellDefinition* GetDefinition(const FGameplayCueParameters& Parameters)
	{
		return Cast<UGrimSpellDefinition>(Parameters.SourceObject.Get());
	}

	const UGrimSpellDefinition* GetSelectedSpell(const AActor* Actor)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		const AGrimPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AGrimPlayerState>() : nullptr;
		const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
		return Spellbook ? Spellbook->GetSelectedSpell() : nullptr;
	}

	UMaterialInterface* LoadMaterialOrBasic(const TSoftObjectPtr<UMaterialInterface>& Soft)
	{
		if (UMaterialInterface* Material = Soft.LoadSynchronous())
		{
			return Material;
		}
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
}

UGrimSpellVisualSet* UGrimMagicFXStatics::FindVisualSetOnObject(const UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}
	if (const UGrimSpellVisualSet* Direct = Cast<UGrimSpellVisualSet>(Object))
	{
		return const_cast<UGrimSpellVisualSet*>(Direct);
	}

	// 갈래 A 의 UGrimSpellDefinition::VisualSet — 따로 빌드되는 동안은 이름으로 찾는다(통합 때 직접 접근으로 바꿔도 된다).
	const FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), TEXT("VisualSet"));
	if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		return Cast<UGrimSpellVisualSet>(ObjectProperty->GetObjectPropertyValue_InContainer(Object));
	}
	if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPtr* Soft = SoftProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Object);
		return Soft ? Cast<UGrimSpellVisualSet>(Soft->LoadSynchronous()) : nullptr;
	}
	return nullptr;
}

FGameplayTag UGrimMagicFXStatics::FindElementTagInContainer(const FGameplayTagContainer& Tags)
{
	const FGameplayTag ElementRoot = FGameplayTag::RequestGameplayTag(TEXT("Element"), false);
	if (!ElementRoot.IsValid())
	{
		return FGameplayTag();
	}
	for (const FGameplayTag& Tag : Tags)
	{
		if (Tag.MatchesTag(ElementRoot) && Tag != ElementRoot)
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UGrimMagicFXStatics::FindElementTag(const FGameplayCueParameters& Parameters)
{
	FGameplayTag Element = FindElementTagInContainer(Parameters.AggregatedSourceTags);
	if (!Element.IsValid())
	{
		if (const UGrimSpellDefinition* Definition = GetDefinition(Parameters))
		{
			Element = Definition->Element;
		}
	}
	return Element;
}

UGrimSpellVisualSet* UGrimMagicFXStatics::ResolveVisualSet(const FGameplayCueParameters& Parameters, const AActor* CueTarget)
{
	if (UGrimSpellVisualSet* FromSource = FindVisualSetOnObject(Parameters.SourceObject.Get()))
	{
		return FromSource;
	}

	FGameplayTag Element = FindElementTag(Parameters);
	if (!Element.IsValid())
	{
		// 최소 복제 큐처럼 파라미터가 비어 오면 시전자가 고른 마법으로 짐작한다.
		const AActor* Who = Parameters.Instigator.IsValid() ? Parameters.Instigator.Get() : CueTarget;
		if (const UGrimSpellDefinition* Selected = GetSelectedSpell(Who))
		{
			if (UGrimSpellVisualSet* FromSelected = FindVisualSetOnObject(Selected))
			{
				return FromSelected;
			}
			Element = Selected->Element;
		}
	}

	const UGrimMagicFXSettings* Settings = UGrimMagicFXSettings::Get();
	if (Element.IsValid())
	{
		if (const TSoftObjectPtr<UGrimSpellVisualSet>* Found = Settings->DefaultVisualSets.Find(Element))
		{
			if (UGrimSpellVisualSet* VisualSet = Found->LoadSynchronous())
			{
				return VisualSet;
			}
		}
	}
	return Settings->FallbackVisualSet.LoadSynchronous();
}

FLinearColor UGrimMagicFXStatics::ResolveColor(const UGrimSpellVisualSet* VisualSet, const FGameplayCueParameters& Parameters)
{
	if (VisualSet)
	{
		return VisualSet->ElementColor;
	}
	if (const UGrimSpellDefinition* Definition = GetDefinition(Parameters))
	{
		return Definition->Color;
	}
	return FLinearColor::White;
}

EGrimCastMotion UGrimMagicFXStatics::ResolveCastMotion(const UGrimSpellVisualSet* VisualSet, const FGameplayCueParameters& Parameters)
{
	if (const UGrimMagicFXVisualSet* Extended = Cast<UGrimMagicFXVisualSet>(VisualSet); Extended && Extended->CastMotion != EGrimCastMotion::Auto)
	{
		return Extended->CastMotion;
	}

	if (const UGrimSpellDefinition* Definition = GetDefinition(Parameters))
	{
		// 열거값 이름으로 본다 — 갈래 A 가 Delivery 값(Bolt · Stream · Cone …)을 바꿔도 그대로 맞는다.
		const FString DeliveryName = StaticEnum<EGrimSpellDelivery>()->GetNameStringByValue(static_cast<int64>(Definition->Delivery));
		const EGrimCastMotion FromDelivery = GrimMagicFXMath::CastMotionFromDeliveryName(DeliveryName);
		if (FromDelivery != EGrimCastMotion::Auto)
		{
			return FromDelivery;
		}
	}
	return GrimMagicFXMath::CastMotionFromElementName(FindElementTag(Parameters).GetTagName());
}

float UGrimMagicFXStatics::ReadFloatProperty(const UObject* Object, FName PropertyName, float Default)
{
	if (!Object || PropertyName.IsNone())
	{
		return Default;
	}
	const FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName);
	if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		return FloatProperty->GetPropertyValue_InContainer(Object);
	}
	if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(Object));
	}
	return Default;
}

APawn* UGrimMagicFXStatics::ResolveInstigatorPawn(AActor* CueTarget, const FGameplayCueParameters& Parameters)
{
	if (APawn* Pawn = Cast<APawn>(Parameters.Instigator.Get()))
	{
		return Pawn;
	}
	if (APawn* Pawn = Cast<APawn>(CueTarget))
	{
		return Pawn;
	}
	return CueTarget ? CueTarget->GetInstigator() : nullptr;
}

AGrimWand* UGrimMagicFXStatics::FindWand(const APawn* Pawn)
{
	const AGrimPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AGrimPlayerState>() : nullptr;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	return Spellbook ? Spellbook->GetWand() : nullptr;
}

AGrimGrimoireBook* UGrimMagicFXStatics::FindGrimoireBook(const APawn* Pawn)
{
	if (const AGrimPlayerCharacter* Character = Cast<AGrimPlayerCharacter>(Pawn))
	{
		if (AGrimGrimoireBook* Book = Character->GetGrimoireBook())
		{
			return Book;
		}
	}
	if (!Pawn)
	{
		return nullptr;
	}
	TArray<AActor*> Attached;
	Pawn->GetAttachedActors(Attached);
	for (AActor* Actor : Attached)
	{
		if (AGrimGrimoireBook* Book = Cast<AGrimGrimoireBook>(Actor))
		{
			return Book;
		}
	}
	return nullptr;
}

UGrimSpellPoseAnimInstance* UGrimMagicFXStatics::FindPoseAnimInstance(const APawn* Pawn)
{
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	return Mesh ? Cast<UGrimSpellPoseAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
}

FVector UGrimMagicFXStatics::GetAimDirection(const APawn* Pawn)
{
	return Pawn ? Pawn->GetBaseAimRotation().Vector() : FVector::ForwardVector;
}

void UGrimMagicFXStatics::ResolveMuzzle(const APawn* Pawn, const FGameplayCueParameters& Parameters, FVector& OutLocation, FVector& OutDirection)
{
	OutDirection = Parameters.Normal.IsNearlyZero() ? GetAimDirection(Pawn) : Parameters.Normal.GetSafeNormal();

	if (!Parameters.Location.IsNearlyZero())
	{
		OutLocation = Parameters.Location;
		return;
	}
	if (const AGrimWand* Wand = FindWand(Pawn))
	{
		OutLocation = Wand->GetMuzzleTransform().GetLocation();
		return;
	}
	OutLocation = Pawn ? Pawn->GetPawnViewLocation() : FVector::ZeroVector;
}

UFXSystemComponent* UGrimMagicFXStatics::SpawnFX(UWorld* World, UFXSystemAsset* Asset, const FVector& Location, const FRotator& Rotation, float Scale, const FLinearColor& Color, bool bAutoDestroy, float StopAfterSeconds)
{
	if (!World || !Asset || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	UFXSystemComponent* Component = nullptr;
	if (UNiagaraSystem* Niagara = Cast<UNiagaraSystem>(Asset))
	{
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Niagara, Location, Rotation, FVector(Scale), bAutoDestroy, true, ENCPoolMethod::None, true);
		if (NiagaraComponent)
		{
			UGrimSpellVisualSet::TintNiagara(NiagaraComponent, Color);
		}
		Component = NiagaraComponent;
	}
	else if (UParticleSystem* Cascade = Cast<UParticleSystem>(Asset))
	{
		UParticleSystemComponent* CascadeComponent = UGameplayStatics::SpawnEmitterAtLocation(World, Cascade, Location, Rotation, FVector(Scale), bAutoDestroy, EPSCPoolMethod::None, true);
		if (CascadeComponent)
		{
			CascadeComponent->SetColorParameter(CascadeColorParameter, Color);
		}
		Component = CascadeComponent;
	}

	if (Component && StopAfterSeconds > 0.f)
	{
		TWeakObjectPtr<UFXSystemComponent> Weak(Component);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Component, [Weak]()
		{
			StopFX(Weak.Get());
		}), StopAfterSeconds, false);
	}
	return Component;
}

void UGrimMagicFXStatics::StopFX(UFXSystemComponent* Component)
{
	if (!Component)
	{
		return;
	}
	if (UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(Component))
	{
		NiagaraComponent->SetAutoDestroy(true);
	}
	else if (UParticleSystemComponent* CascadeComponent = Cast<UParticleSystemComponent>(Component))
	{
		CascadeComponent->bAutoDestroy = true;
	}
	Component->Deactivate();
}

UAudioComponent* UGrimMagicFXStatics::PlaySound(UWorld* World, USoundBase* Sound, const FVector& Location, float Volume, float Pitch, float StopAfterSeconds)
{
	if (!World || !Sound || Volume <= 0.f || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	UAudioComponent* Audio = UGameplayStatics::SpawnSoundAtLocation(World, Sound, Location, FRotator::ZeroRotator, Volume, Pitch);
	if (Audio && StopAfterSeconds > 0.f)
	{
		TWeakObjectPtr<UAudioComponent> Weak(Audio);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Audio, [Weak]()
		{
			if (UAudioComponent* Playing = Weak.Get())
			{
				Playing->FadeOut(0.1f, 0.f);
			}
		}), StopAfterSeconds, false);
	}
	return Audio;
}

void UGrimMagicFXStatics::SpawnImpactDecal(UWorld* World, const FGrimImpactDecalSpec& Spec, const FVector& Location, const FVector& Normal)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UMaterialInterface* BaseMaterial = Spec.Material.LoadSynchronous();
	if (!BaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, World);
	Material->SetVectorParameterValue(TEXT("Color"), Spec.Color);
	Material->SetScalarParameterValue(TEXT("Roughness"), Spec.Roughness);
	Material->SetScalarParameterValue(TEXT("Opacity"), Spec.Opacity);

	// 데칼은 X 축으로 투영한다. 표면 쪽(−법선)을 보게 하고 자국마다 돌려 같은 모양이 반복되지 않게 한다.
	const FVector Axis = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	FRotator Rotation = (-Axis).Rotation();
	Rotation.Roll = FMath::FRandRange(0.f, 360.f);
	const float Radius = Spec.Size * 0.5f;
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(World, Material, FVector(Radius * 0.5f, Radius, Radius), Location, Rotation, Spec.Lifetime);
	if (Decal)
	{
		Decal->SetFadeScreenSize(0.002f);
		const float Fade = FMath::Clamp(Spec.FadeTime, 0.f, Spec.Lifetime);
		Decal->SetFadeOut(Spec.Lifetime - Fade, Fade, false);
	}
}

void UGrimMagicFXStatics::PlayCasterCameraShake(const APawn* Pawn, TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!Pawn || !ShakeClass || Scale <= 0.f || !Pawn->IsLocallyControlled())
	{
		return;
	}
	const APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
	}
}

UMaterialInterface* UGrimMagicFXStatics::GetGlowMaterial()
{
	return LoadMaterialOrBasic(UGrimMagicFXSettings::Get()->GlowMaterial);
}

UMaterialInterface* UGrimMagicFXStatics::GetBookMaterial()
{
	return LoadMaterialOrBasic(UGrimMagicFXSettings::Get()->BookMaterial);
}
