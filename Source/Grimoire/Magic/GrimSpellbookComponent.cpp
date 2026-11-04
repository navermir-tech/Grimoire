#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellSelectAbility.h"
#include "Magic/GrimWand.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UGrimSpellbookComponent::UGrimSpellbookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimSpellbookComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGrimSpellbookComponent, KnownSpells);
	DOREPLIFETIME(UGrimSpellbookComponent, SelectedIndex);
	DOREPLIFETIME(UGrimSpellbookComponent, Wand);
}

void UGrimSpellbookComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		ActorInfoReadyHandle = PlayerState->OnAbilityActorInfoReady.AddUObject(this, &ThisClass::HandleAbilityActorInfoReady);

		// 폰이 먼저 초기화를 마친 경우(BeginPlay 보다 빠른 빙의)도 놓치지 않는다.
		if (const UAbilitySystemComponent* ASC = GetASC(); ASC && ASC->AbilityActorInfo.IsValid())
		{
			if (APawn* Pawn = Cast<APawn>(ASC->GetAvatarActor()))
			{
				HandleAbilityActorInfoReady(PlayerState, Pawn);
			}
		}
	}
}

void UGrimSpellbookComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner()))
	{
		PlayerState->OnAbilityActorInfoReady.Remove(ActorInfoReadyHandle);
	}
	if (GetOwner() && GetOwner()->HasAuthority() && Wand)
	{
		Wand->Destroy();
		Wand = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UGrimSpellbookComponent::GetASC() const
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
}

void UGrimSpellbookComponent::HandleAbilityActorInfoReady(AGrimPlayerState* PlayerState, APawn* Pawn)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bInitialized)
	{
		if (UAbilitySystemComponent* ASC = GetASC())
		{
			bInitialized = true;
			InitializeSpellbook(ASC);
		}
	}

	EquipWand(Pawn);
}

void UGrimSpellbookComponent::InitializeSpellbook(UAbilitySystemComponent* ASC)
{
	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();

	// 서클 · 마나 초기값 (R06). 수치는 DefaultGame.ini.
	const float MaxMana = Settings->GetMaxManaForCircle(Settings->StartingCircle);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), Settings->StartingCircle);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), MaxMana);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), MaxMana);

	if (Settings->ManaRegenPerSecond > 0.f)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle Regen = ASC->MakeOutgoingSpec(UGrimGE_ManaRegen::StaticClass(), 1.f, Context);
		if (Regen.IsValid())
		{
			const float Period = Regen.Data->GetPeriod();
			Regen.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_ManaRegen, Settings->ManaRegenPerSecond * (Period > 0.f ? Period : 1.f));
			ASC->ApplyGameplayEffectSpecToSelf(*Regen.Data.Get());
		}
	}

	// 다음 / 이전 선택. 같은 클래스를 입력 태그만 달리해 두 번 준다.
	for (const FGameplayTag InputTag : { GrimTags::Input_Spell_Next.GetTag(), GrimTags::Input_Spell_Prev.GetTag() })
	{
		FGameplayAbilitySpec Spec(UGrimSpellSelectAbility::StaticClass(), 1, INDEX_NONE, this);
		Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
		ASC->GiveAbility(Spec);
	}

	for (const TSoftObjectPtr<UGrimSpellDefinition>& SpellRef : Settings->StartingSpells)
	{
		if (UGrimSpellDefinition* Spell = SpellRef.LoadSynchronous())
		{
			LearnSpell(Spell);
		}
	}
}

bool UGrimSpellbookComponent::LearnSpell(UGrimSpellDefinition* Spell)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!Spell || !ASC || !GetOwner()->HasAuthority() || KnownSpells.Contains(Spell))
	{
		return false;
	}
	if (ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()) + KINDA_SMALL_NUMBER < Spell->Circle)
	{
		return false;
	}

	const TSubclassOf<UGrimSpellAbility> AbilityClass = Spell->AbilityClass ? Spell->AbilityClass : TSubclassOf<UGrimSpellAbility>(UGrimSpellAbility::StaticClass());
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Spell));

	KnownSpells.Add(Spell);
	SpellHandles.Add(Handle);

	if (SelectedIndex == INDEX_NONE)
	{
		SelectSpell(0);
	}
	else
	{
		OnSelectedSpellChanged.Broadcast(this, GetSelectedSpell());
	}
	return true;
}

void UGrimSpellbookComponent::CycleSelection(int32 Step)
{
	if (KnownSpells.Num() == 0)
	{
		return;
	}
	const int32 Current = SelectedIndex == INDEX_NONE ? 0 : SelectedIndex;
	SelectSpell(((Current + Step) % KnownSpells.Num() + KnownSpells.Num()) % KnownSpells.Num());
}

void UGrimSpellbookComponent::SelectSpell(int32 Index)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || !SpellHandles.IsValidIndex(Index))
	{
		return;
	}

	// 선택된 마법 Spec 에만 시전 입력 태그. 바뀐 Spec 만 더럽혀 복제한다.
	for (int32 i = 0; i < SpellHandles.Num(); ++i)
	{
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SpellHandles[i]);
		if (!Spec)
		{
			continue;
		}
		FGameplayTagContainer& Tags = Spec->GetDynamicSpecSourceTags();
		const bool bShouldHave = (i == Index);
		if (bShouldHave != Tags.HasTagExact(GrimTags::Input_Spell_Cast))
		{
			if (bShouldHave)
			{
				Tags.AddTag(GrimTags::Input_Spell_Cast);
			}
			else
			{
				Tags.RemoveTag(GrimTags::Input_Spell_Cast);
			}
			ASC->MarkAbilitySpecDirty(*Spec);
		}
	}

	SelectedIndex = Index;
	OnSelectedSpellChanged.Broadcast(this, GetSelectedSpell());
}

UGrimSpellDefinition* UGrimSpellbookComponent::GetSelectedSpell() const
{
	return KnownSpells.IsValidIndex(SelectedIndex) ? KnownSpells[SelectedIndex].Get() : nullptr;
}

void UGrimSpellbookComponent::OnRep_Selection()
{
	OnSelectedSpellChanged.Broadcast(this, GetSelectedSpell());
}

void UGrimSpellbookComponent::EquipWand(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	if (!Wand)
	{
		const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();
		UClass* WandClass = Settings->StartingWandClass.LoadSynchronous();
		if (!WandClass)
		{
			WandClass = AGrimWand::StaticClass();
		}

		FActorSpawnParameters Params;
		Params.Owner = Pawn;
		Params.Instigator = Pawn;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Wand = GetWorld()->SpawnActor<AGrimWand>(WandClass, Pawn->GetActorTransform(), Params);
		if (!Wand)
		{
			return;
		}
	}

	// 완드의 관련성(relevancy)은 폰을 따른다.
	Wand->SetOwner(Pawn);
	Wand->SetInstigator(Pawn);

	const FName SocketName = UGrimMagicSettings::Get()->WandSocketName;
	USkeletalMeshComponent* PawnMesh = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		PawnMesh = Character->GetMesh();
	}
	if (!PawnMesh)
	{
		PawnMesh = Pawn->FindComponentByClass<USkeletalMeshComponent>();
	}

	const FAttachmentTransformRules Rules = FAttachmentTransformRules::KeepRelativeTransform;
	if (PawnMesh && PawnMesh->DoesSocketExist(SocketName))
	{
		Wand->AttachToComponent(PawnMesh, Rules, SocketName);
	}
	else
	{
		Wand->AttachToComponent(Pawn->GetRootComponent(), Rules);
	}
}
