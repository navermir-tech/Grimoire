#include "Save/GrimPlayerSaveParticipants.h"
#include "Save/GrimSaveRecords.h"
#include "Save/GrimSaveSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UAbilitySystemComponent* UGrimPlayerSaveParticipantBase::GetASC() const
{
	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(PlayerState.Get());
	return Owner ? Owner->GetAbilitySystemComponent() : nullptr;
}

// ───────────────────────── Player.Attributes ─────────────────────────

bool UGrimPlayerAttributesSaveParticipant::FindAttributeByName(const UAbilitySystemComponent* ASC, const FString& Name, FGameplayAttribute& OutAttribute)
{
	if (!ASC)
	{
		return false;
	}
	FString SetName, PropertyName = Name.TrimStartAndEnd();
	if (PropertyName.Split(TEXT("."), &SetName, &PropertyName))
	{
		SetName.RemoveFromStart(TEXT("U"), ESearchCase::CaseSensitive);
	}
	for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}
		FString ClassName = Set->GetClass()->GetName();
		if (!SetName.IsEmpty() && ClassName != SetName)
		{
			continue;
		}
		if (FProperty* Property = FindFProperty<FProperty>(Set->GetClass(), *PropertyName))
		{
			if (FGameplayAttribute::IsGameplayAttributeDataProperty(Property))
			{
				OutAttribute = FGameplayAttribute(Property);
				return true;
			}
		}
	}
	return false;
}

void UGrimPlayerAttributesSaveParticipant::WriteSaveData(FArchive& Ar)
{
	const UAbilitySystemComponent* ASC = GetASC();
	TArray<TPair<FString, float>> Values;
	for (const FString& Name : UGrimSaveSettings::Get()->PlayerAttributes)
	{
		FGameplayAttribute Attribute;
		if (FindAttributeByName(ASC, Name, Attribute))
		{
			Values.Emplace(Attribute.GetName(), ASC->GetNumericAttributeBase(Attribute));
		}
	}

	int32 Count = Values.Num();
	Ar << Count;
	for (TPair<FString, float>& Pair : Values)
	{
		Ar << Pair.Key << Pair.Value;
	}
}

void UGrimPlayerAttributesSaveParticipant::ReadSaveData(FArchive& Ar, int32 Version)
{
	UAbilitySystemComponent* ASC = GetASC();
	int32 Count = 0;
	Ar << Count;
	for (int32 i = 0; i < Count && !Ar.IsError(); ++i)
	{
		FString Name;
		float Value = 0.f;
		Ar << Name << Value;
		FGameplayAttribute Attribute;
		if (FindAttributeByName(ASC, Name, Attribute))
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
		}
		else
		{
			UE_LOG(LogGrimSave, Warning, TEXT("Player.Attributes: 어트리뷰트 %s 를 찾지 못해 건너뜀"), *Name);
		}
	}
}

// ───────────────────────── Player.Transform ─────────────────────────

void UGrimPlayerTransformSaveParticipant::WriteSaveData(FArchive& Ar)
{
	const APawn* Pawn = PlayerState.IsValid() ? PlayerState->GetPawn() : nullptr;
	bool bHasPawn = Pawn != nullptr || bHasFallback;
	FVector Location = Pawn ? Pawn->GetActorLocation() : FallbackLocation;
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	float Yaw = Controller ? Controller->GetControlRotation().Yaw : (Pawn ? Pawn->GetActorRotation().Yaw : FallbackYaw);
	Ar << bHasPawn << Location << Yaw;
}

void UGrimPlayerTransformSaveParticipant::ReadSaveData(FArchive& Ar, int32 Version)
{
	bool bHasPawn = false;
	FVector Location = FVector::ZeroVector;
	float Yaw = 0.f;
	Ar << bHasPawn << Location << Yaw;

	APawn* Pawn = PlayerState.IsValid() ? PlayerState->GetPawn() : nullptr;
	if (!bHasPawn || !Pawn || Ar.IsError() || !UGrimSaveSettings::Get()->bRestorePlayerTransform)
	{
		return;
	}

	const FRotator Rotation(0.f, Yaw, 0.f);
	if (!Pawn->TeleportTo(Location, Rotation, false, false))
	{
		UE_LOG(LogGrimSave, Warning, TEXT("Player.Transform: %s 를 %s 로 옮기지 못함(막힘) — 그 자리에 둔다"), *GetNameSafe(Pawn), *Location.ToCompactString());
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
	if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		PC->SetControlRotation(Rotation);
		PC->ClientSetRotation(Rotation);
	}
}
