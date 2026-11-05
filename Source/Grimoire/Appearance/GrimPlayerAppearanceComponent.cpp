#include "Appearance/GrimPlayerAppearanceComponent.h"
#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Appearance/GrimCharacterCreatorWidget.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGrimPlayerAppearanceComponent::UGrimPlayerAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimPlayerAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimPlayerAppearanceComponent, bCustomized, COND_OwnerOnly);
}

UGrimPlayerAppearanceComponent* UGrimPlayerAppearanceComponent::Find(const APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UGrimPlayerAppearanceComponent>() : nullptr;
}

UGrimPlayerAppearanceComponent* UGrimPlayerAppearanceComponent::FindForController(const APlayerController* Controller)
{
	return Controller ? Find(Controller->PlayerState) : nullptr;
}

void UGrimPlayerAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	if (!PlayerState)
	{
		return;
	}

	if (PlayerState->HasAuthority())
	{
		// 세이브가 없으면 이 한 벌로 시작한다(세이브를 읽으면 덮인다) — 같이 들어온 사람끼리 똑같은 마네킹이 되지 않게
		if (!Appearance.IsSet())
		{
			RandomizeAuthority(0);
		}
		PlayerState->OnPawnSet.AddDynamic(this, &UGrimPlayerAppearanceComponent::HandlePawnSet);
		if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(PlayerState))
		{
			if (UAbilitySystemComponent* ASC = AbilityOwner->GetAbilitySystemComponent())
			{
				CircleHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGrimMagicAttributeSet::GetCircleAttribute())
					.AddUObject(this, &UGrimPlayerAppearanceComponent::HandleCircleChanged);
			}
		}
		PushToPawn();
	}

	// 소유 클라이언트(리슨 호스트 포함): 세이브 읽기 · 복제를 기다렸다가 새 캐릭터면 생성 화면
	if (UGrimAppearanceSettings::Get()->bOpenCreatorForNewCharacter)
	{
		FTimerHandle Timer;
		GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateUObject(this, &UGrimPlayerAppearanceComponent::MaybeOpenCreator), 2.f, false);
	}
}

void UGrimPlayerAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.RemoveDynamic(this, &UGrimPlayerAppearanceComponent::HandlePawnSet);
		if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(PlayerState))
		{
			if (UAbilitySystemComponent* ASC = AbilityOwner->GetAbilitySystemComponent())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UGrimMagicAttributeSet::GetCircleAttribute()).Remove(CircleHandle);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimPlayerAppearanceComponent::WriteSaveData(FArchive& Ar)
{
	bool bSavedCustomized = bCustomized;
	Ar << bSavedCustomized;
	Appearance.Serialize(Ar);
}

void UGrimPlayerAppearanceComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	bool bSavedCustomized = false;
	FGrimAppearance Loaded;
	Ar << bSavedCustomized;
	Loaded.Serialize(Ar);
	if (Ar.IsError() || !Loaded.IsSet())
	{
		UE_LOG(LogGrimAppearance, Warning, TEXT("Player.Appearance: 읽지 못해 지금 외형을 둔다"));
		return;
	}
	SetAppearanceAuthority(Loaded, bSavedCustomized);
}

void UGrimPlayerAppearanceComponent::SetAppearanceAuthority(const FGrimAppearance& NewAppearance, bool bMarkCustomized)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	FGrimAppearance Clean = NewAppearance;
	FGrimAppearanceMath::Sanitize(Clean, UGrimAppearanceSettings::GetCatalog(), true);
	Clean.Circle = static_cast<uint8>(FMath::Clamp(ReadCircle(), 0, FGrimAppearance::MaxCircle));
	Appearance = Clean;
	bCustomized = bCustomized || bMarkCustomized;
	PushToPawn();
}

void UGrimPlayerAppearanceComponent::RandomizeAuthority(int32 Seed)
{
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	const int32 UsedSeed = Seed != 0 ? Seed : FMath::Rand();
	FGrimAppearance Random = FGrimAppearanceMath::GenerateFromRule(Catalog, UGrimAppearanceSettings::Get()->PlayerRule, UsedSeed);
	if (!Random.IsSet())
	{
		UE_LOG(LogGrimAppearance, Warning, TEXT("플레이어 규칙 %s 가 없어 기본 외형을 쓴다"), *UGrimAppearanceSettings::Get()->PlayerRule.ToString());
		FGrimAppearanceMath::Sanitize(Random, Catalog, true);
		FGrimAppearanceMath::ApplyFaceDefaults(Random, Catalog);
	}
	SetAppearanceAuthority(Random, false);
}

void UGrimPlayerAppearanceComponent::SubmitAppearance(const FGrimAppearance& Requested)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetAppearanceAuthority(Requested, true);
		return;
	}
	Server_SubmitAppearance(Requested);
}

void UGrimPlayerAppearanceComponent::Server_SubmitAppearance_Implementation(const FGrimAppearance& Requested)
{
	SetAppearanceAuthority(Requested, true);
	UE_LOG(LogGrimAppearance, Log, TEXT("%s 외형 확정: %s"), *GetNameSafe(GetOwner()), *Appearance.ToString());
}

void UGrimPlayerAppearanceComponent::RequestRandomize(int32 Seed)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RandomizeAuthority(Seed);
		return;
	}
	Server_Randomize(Seed);
}

void UGrimPlayerAppearanceComponent::Server_Randomize_Implementation(int32 Seed)
{
#if !UE_BUILD_SHIPPING
	RandomizeAuthority(Seed);
#endif
}

void UGrimPlayerAppearanceComponent::OnRep_Customized()
{
}

void UGrimPlayerAppearanceComponent::HandlePawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	PushToPawn();
}

void UGrimPlayerAppearanceComponent::HandleCircleChanged(const FOnAttributeChangeData& Data)
{
	const uint8 NewCircle = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Data.NewValue), 0, FGrimAppearance::MaxCircle));
	if (Appearance.IsSet() && Appearance.Circle != NewCircle)
	{
		Appearance.Circle = NewCircle;
		PushToPawn();
	}
}

int32 UGrimPlayerAppearanceComponent::ReadCircle() const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !ASC->HasAttributeSetForAttribute(UGrimMagicAttributeSet::GetCircleAttribute()))
	{
		return 1;
	}
	return FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()));
}

void UGrimPlayerAppearanceComponent::PushToPawn()
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (!Pawn || !Pawn->HasAuthority() || !Appearance.IsSet())
	{
		return;
	}
	if (UGrimAppearanceComponent* Component = Pawn->FindComponentByClass<UGrimAppearanceComponent>())
	{
		Component->SetAppearance(Appearance);
	}
}

void UGrimPlayerAppearanceComponent::MaybeOpenCreator()
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	APlayerController* Controller = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (bCreatorOffered || bCustomized || !Controller || !Controller->IsLocalController())
	{
		return;
	}
	bCreatorOffered = true;
	UGrimCharacterCreatorWidget::Open(Controller);
}
