#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSkillbook.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellSelectAbility.h"
#include "Magic/GrimWand.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Core/GrimTargetingTypes.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

// ───────────────────────── 숙련도 복제 콜백 ─────────────────────────

void FGrimSpellProficiency::PostReplicatedAdd(const FGrimSpellProficiencyArray& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleProficiencyReplicated(*this, true);
	}
}

void FGrimSpellProficiency::PostReplicatedChange(const FGrimSpellProficiencyArray& InArraySerializer)
{
	if (InArraySerializer.Owner)
	{
		InArraySerializer.Owner->HandleProficiencyReplicated(*this, false);
	}
}

void UGrimSpellbookComponent::HandleProficiencyReplicated(FGrimSpellProficiency& Item, bool bAdded)
{
	const int32 Grade = GetProficiency(Item.Spell).GradeIndex;
	// 처음 받은 값(접속 · 세이브 불러오기)은 알림 없이 기준으로 삼는다.
	if (!bAdded && Item.LastGradeIndex != INDEX_NONE && Grade > Item.LastGradeIndex)
	{
		OnProficiencyGradeUp.Broadcast(this, Item.Spell, Grade);
	}
	Item.LastGradeIndex = Grade;
}

// ───────────────────────── 기본 ─────────────────────────

UGrimSpellbookComponent::UGrimSpellbookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Proficiency.Owner = this;
}

void UGrimSpellbookComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGrimSpellbookComponent, KnownSpells);
	DOREPLIFETIME(UGrimSpellbookComponent, SelectedIndex);
	DOREPLIFETIME(UGrimSpellbookComponent, Wand);
	DOREPLIFETIME_CONDITION(UGrimSpellbookComponent, Proficiency, COND_OwnerOnly);
	DOREPLIFETIME(UGrimSpellbookComponent, SlotSpells);
	DOREPLIFETIME(UGrimSpellbookComponent, ActiveSlotSet);
}

UGrimSpellbookComponent* UGrimSpellbookComponent::FindForASC(const UAbilitySystemComponent* ASC)
{
	const AGrimPlayerState* PlayerState = ASC ? Cast<AGrimPlayerState>(ASC->GetOwnerActor()) : nullptr;
	return PlayerState ? PlayerState->GetSpellbook() : nullptr;
}

void UGrimSpellbookComponent::BeginPlay()
{
	Super::BeginPlay();
	Proficiency.Owner = this;

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

bool UGrimSpellbookComponent::KnowsSpell(const UGrimSpellDefinition* Spell) const
{
	return Spell && KnownSpells.Contains(Spell);
}

EGrimLearnResult UGrimSpellbookComponent::CanLearnSpell(const UGrimSpellDefinition* Spell) const
{
	const UAbilitySystemComponent* ASC = GetASC();
	if (!Spell || !ASC)
	{
		return EGrimLearnResult::Invalid;
	}
	if (KnowsSpell(Spell))
	{
		return EGrimLearnResult::AlreadyKnown;
	}
	if (ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()) + KINDA_SMALL_NUMBER < Spell->Circle)
	{
		return EGrimLearnResult::CircleTooLow;
	}
	return EGrimLearnResult::Learned;
}

bool UGrimSpellbookComponent::LearnSpell(UGrimSpellDefinition* Spell)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority() || CanLearnSpell(Spell) != EGrimLearnResult::Learned)
	{
		return false;
	}

	const TSubclassOf<UGrimSpellAbility> AbilityClass = Spell->AbilityClass ? Spell->AbilityClass : TSubclassOf<UGrimSpellAbility>(UGrimSpellAbility::StaticClass());
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Spell));

	KnownSpells.Add(Spell);
	SpellHandles.Add(Handle);

	// 숙련도 칸(세이브를 불러왔다면 이미 있다).
	if (Spell->bTracksProficiency && !FindProficiencyItem(Spell))
	{
		FGrimSpellProficiency& Item = Proficiency.Items.AddDefaulted_GetRef();
		Item.Spell = Spell;
		Item.XP = 0.f;
		Item.LastGradeIndex = GetProficiency(Spell).GradeIndex;
		Proficiency.MarkItemDirty(Item);
	}

	// 첫 빈 단축 슬롯에(숙련도 없는 기본 공격 등은 슬롯과 따로).
	if (Spell->bTracksProficiency && UGrimSpellbookSettings::Get()->bAutoAssignSlots)
	{
		const int32 Total = UGrimSpellbookSettings::Get()->GetTotalSlots();
		if (SlotSpells.Num() < Total)
		{
			SlotSpells.SetNum(Total);
		}
		const int32 Empty = SlotSpells.IndexOfByKey(nullptr);
		if (Empty != INDEX_NONE)
		{
			SlotSpells[Empty] = Spell;
		}
	}

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

void UGrimSpellbookComponent::Server_SelectSpell_Implementation(int32 Index)
{
	SelectSpell(Index);
}

UGrimSpellDefinition* UGrimSpellbookComponent::GetSelectedSpell() const
{
	return KnownSpells.IsValidIndex(SelectedIndex) ? KnownSpells[SelectedIndex].Get() : nullptr;
}

void UGrimSpellbookComponent::OnRep_Selection()
{
	OnSelectedSpellChanged.Broadcast(this, GetSelectedSpell());
}

// ───────────────────────── 스킬북 ─────────────────────────

EGrimLearnResult UGrimSpellbookComponent::PreviewSkillbook(const UGrimSpellDefinition* Spell) const
{
	if (!Spell)
	{
		return EGrimLearnResult::Invalid;
	}
	if (!KnowsSpell(Spell))
	{
		return CanLearnSpell(Spell);
	}
	if (!Spell->bTracksProficiency)
	{
		return EGrimLearnResult::AlreadyKnown;
	}
	return GetProficiency(Spell).bMaxGrade ? EGrimLearnResult::AlreadyMastered : EGrimLearnResult::ProficiencyGained;
}

EGrimLearnResult UGrimSpellbookComponent::UseSkillbook(UGrimSpellDefinition* Spell)
{
	if (!GetOwner()->HasAuthority())
	{
		return EGrimLearnResult::Invalid;
	}
	EGrimLearnResult Result = PreviewSkillbook(Spell);
	if (Result == EGrimLearnResult::Learned)
	{
		if (!LearnSpell(Spell))
		{
			Result = EGrimLearnResult::Invalid;
		}
	}
	else if (Result == EGrimLearnResult::ProficiencyGained)
	{
		const UGrimSpellbookSettings* Settings = UGrimSpellbookSettings::Get();
		const float XP = FGrimProficiencyMath::ComputeSkillbookXP(GetProficiency(Spell), Settings->SkillbookXPFractionOfGrade, Settings->SkillbookFlatXP);
		AddProficiencyXP(Spell, XP);
		UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 스킬북 %s — 이미 앎, 숙련도 +%.1f"), *GetNameSafe(GetOwner()), *Spell->GetName(), XP);
	}
	NotifyLearnResult(Spell, Result);
	return Result;
}

void UGrimSpellbookComponent::NotifyLearnResult(UGrimSpellDefinition* Spell, EGrimLearnResult Result)
{
	OnLearnResult.Broadcast(this, Spell, Result);
	// 리슨 호스트는 위에서 이미 알렸다 — 원격 소유 클라이언트에게만 보낸다.
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PC && !PC->IsLocalController())
	{
		Client_LearnResult(Spell, Result);
	}
}

EGrimLearnResult UGrimSpellbookComponent::PickUpSkillbook(AGrimSkillbook* Book, const APawn* Reader)
{
	if (!GetOwner()->HasAuthority())
	{
		return EGrimLearnResult::Invalid;
	}
	UGrimSpellDefinition* Spell = Book ? Book->GetTeachesSpell() : nullptr;
	EGrimLearnResult Result = EGrimLearnResult::Invalid;
	if (Spell && !Book->IsConsumed())
	{
		if (!Book->IsWithinReadRange(Reader, Book->GetServerReadSlack()))
		{
			Result = EGrimLearnResult::TooFar;
			NotifyLearnResult(Spell, Result);
		}
		else
		{
			Result = UseSkillbook(Spell);
			if (Result == EGrimLearnResult::Learned || Result == EGrimLearnResult::ProficiencyGained)
			{
				Book->ConsumeBook();
			}
		}
	}
	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 스킬북 %s 집기(%s) → %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Book), *GetNameSafe(Spell),
		*StaticEnum<EGrimLearnResult>()->GetNameStringByValue(static_cast<int64>(Result)));
	return Result;
}

void UGrimSpellbookComponent::Server_PickUpSkillbook_Implementation(AGrimSkillbook* Book)
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	PickUpSkillbook(Book, PlayerState ? PlayerState->GetPawn() : nullptr);
}

void UGrimSpellbookComponent::RequestPickUpSkillbook(AGrimSkillbook* Book)
{
	if (!Book)
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
		PickUpSkillbook(Book, PlayerState ? PlayerState->GetPawn() : nullptr);
	}
	else
	{
		Server_PickUpSkillbook(Book);
	}
}

void UGrimSpellbookComponent::Client_LearnResult_Implementation(UGrimSpellDefinition* Spell, EGrimLearnResult Result)
{
	OnLearnResult.Broadcast(this, Spell, Result);
}

// ───────────────────────── 단축 슬롯 ─────────────────────────

void UGrimSpellbookComponent::AssignSlot(int32 SlotIndex, UGrimSpellDefinition* Spell)
{
	const int32 Total = UGrimSpellbookSettings::Get()->GetTotalSlots();
	if (!GetOwner()->HasAuthority() || SlotIndex < 0 || SlotIndex >= Total || (Spell && !KnowsSpell(Spell)))
	{
		return;
	}
	if (SlotSpells.Num() < Total)
	{
		SlotSpells.SetNum(Total);
	}
	if (Spell)
	{
		for (TObjectPtr<UGrimSpellDefinition>& Other : SlotSpells)
		{
			if (Other == Spell)
			{
				Other = nullptr;
			}
		}
	}
	SlotSpells[SlotIndex] = Spell;
	OnSelectedSpellChanged.Broadcast(this, GetSelectedSpell());
}

void UGrimSpellbookComponent::SetActiveSlotSet(int32 SetIndex)
{
	if (GetOwner()->HasAuthority())
	{
		ActiveSlotSet = FMath::Clamp(SetIndex, 0, FMath::Max(UGrimSpellbookSettings::Get()->SpellSlotSets, 1) - 1);
	}
}

void UGrimSpellbookComponent::Server_AssignSlot_Implementation(int32 SlotIndex, UGrimSpellDefinition* Spell)
{
	AssignSlot(SlotIndex, Spell);
}

void UGrimSpellbookComponent::Server_SetActiveSlotSet_Implementation(int32 SetIndex)
{
	SetActiveSlotSet(SetIndex);
}

// ───────────────────────── 숙련도 ─────────────────────────

FGrimSpellProficiency* UGrimSpellbookComponent::FindProficiencyItem(const UGrimSpellDefinition* Spell)
{
	return Proficiency.Items.FindByPredicate([Spell](const FGrimSpellProficiency& Item) { return Item.Spell == Spell; });
}

const FGrimSpellProficiency* UGrimSpellbookComponent::FindProficiencyItem(const UGrimSpellDefinition* Spell) const
{
	return Proficiency.Items.FindByPredicate([Spell](const FGrimSpellProficiency& Item) { return Item.Spell == Spell; });
}

float UGrimSpellbookComponent::GetProficiencyXP(const UGrimSpellDefinition* Spell) const
{
	const FGrimSpellProficiency* Item = FindProficiencyItem(Spell);
	return Item ? Item->XP : 0.f;
}

FGrimProficiencyView UGrimSpellbookComponent::GetProficiency(const UGrimSpellDefinition* Spell) const
{
	if (!Spell || !Spell->bTracksProficiency)
	{
		return FGrimProficiencyView();
	}
	TArray<FGrimProficiencyGrade> Grades;
	UGrimProficiencySettings::GetGradesFor(Spell, Grades);
	return FGrimProficiencyMath::MakeView(GetProficiencyXP(Spell), Grades, UGrimProficiencySettings::GetCurveFor(Spell).XPScale);
}

FGrimProficiencyMultipliers UGrimSpellbookComponent::GetProficiencyMultipliers(const UGrimSpellDefinition* Spell) const
{
	return GetProficiency(Spell).Multipliers;
}

float UGrimSpellbookComponent::GetProficiencyExtraEffect(const UGrimSpellDefinition* Spell) const
{
	if (!Spell || !Spell->ProficiencyExtraEffect.EffectTag.IsValid())
	{
		return 0.f;
	}
	return FGrimProficiencyMath::GetExtraEffectValue(Spell->ProficiencyExtraEffect.MaxValue, GetProficiencyMultipliers(Spell));
}

int32 UGrimSpellbookComponent::GetProficiencyGradeSum() const
{
	int32 Sum = 0;
	for (const UGrimSpellDefinition* Spell : KnownSpells)
	{
		const FGrimProficiencyView View = GetProficiency(Spell);
		if (View.bTracked)
		{
			Sum += FMath::Max(View.GradeIndex, 0);
		}
	}
	return Sum;
}

int32 UGrimSpellbookComponent::GetTrackedSpellCount() const
{
	int32 Count = 0;
	for (const UGrimSpellDefinition* Spell : KnownSpells)
	{
		Count += (Spell && Spell->bTracksProficiency) ? 1 : 0;
	}
	return Count;
}

void UGrimSpellbookComponent::AddProficiencyXP(const UGrimSpellDefinition* Spell, float Amount)
{
	if (!GetOwner()->HasAuthority() || !Spell || !Spell->bTracksProficiency || !KnowsSpell(Spell) || Amount <= 0.f)
	{
		return;
	}
	FGrimSpellProficiency* Item = FindProficiencyItem(Spell);
	if (!Item)
	{
		Item = &Proficiency.Items.AddDefaulted_GetRef();
		Item->Spell = const_cast<UGrimSpellDefinition*>(Spell);
		Item->LastGradeIndex = GetProficiency(Spell).GradeIndex;
	}

	const int32 OldGrade = Item->LastGradeIndex;
	Item->XP += Amount;
	Proficiency.MarkItemDirty(*Item);

	// 등급은 내려가지 않는다(XP 는 줄지 않는다). 표가 바뀌어 낮아져도 알림만 없다.
	const int32 NewGrade = GetProficiency(Spell).GradeIndex;
	Item->LastGradeIndex = NewGrade;
	if (OldGrade != INDEX_NONE && NewGrade > OldGrade)
	{
		UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 숙련도 등급 오름 %s → %d (XP %.1f)"), *GetNameSafe(GetOwner()), *Spell->GetName(), NewGrade, Item->XP);
		OnProficiencyGradeUp.Broadcast(this, Spell, NewGrade);
	}
}

void UGrimSpellbookComponent::AwardXP(const UGrimSpellDefinition* Spell, const AActor* Target, float BaseXP, uint8 Kind)
{
	if (!GetOwner()->HasAuthority() || !Spell || !Spell->bTracksProficiency || BaseXP <= 0.f)
	{
		return;
	}
	const UGrimProficiencySettings* Settings = UGrimProficiencySettings::Get();
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	FGrimRepeatKey Key;
	Key.Spell = Spell;
	Key.Target = Target;
	Key.Kind = Kind;
	const int32 Repeats = RepeatTracker.Register(Key, Now, Settings->RepeatWindowSeconds);
	if (RepeatTracker.Num() > 128)
	{
		RepeatTracker.Prune(Now, Settings->RepeatWindowSeconds);
	}

	const float Gain = FGrimProficiencyMath::ComputeXPGain(BaseXP, Repeats, Settings->RepeatTargetDecay, Settings->GlobalXPRate);
	UE_LOG(LogGrimMagic, Verbose, TEXT("[Server] %s 숙련도 +%.2f (기본 %.1f, 반복 %d, 대상 %s)"), *Spell->GetName(), Gain, BaseXP, Repeats, *GetNameSafe(Target));
	AddProficiencyXP(Spell, Gain);
}

void UGrimSpellbookComponent::AwardCastXP(const UGrimSpellDefinition* Spell, const AActor* Target)
{
	AwardXP(Spell, Target, UGrimProficiencySettings::GetCurveFor(Spell).XPPerCast, 0);
}

void UGrimSpellbookComponent::AwardHitXP(const UGrimSpellDefinition* Spell, const AActor* Target)
{
	AwardXP(Spell, Target, UGrimProficiencySettings::GetCurveFor(Spell).XPPerMeaningfulHit, 1);
}

void UGrimSpellbookComponent::Server_DebugAddXP_Implementation(UGrimSpellDefinition* Spell, float Amount)
{
#if !UE_BUILD_SHIPPING
	AddProficiencyXP(Spell, Amount);
#endif
}

bool UGrimSpellbookComponent::IsMeaningfulHit(const UGrimSpellDefinition* Spell, const UAbilitySystemComponent* SourceASC, const AActor* HitActor)
{
	if (!Spell || !HitActor)
	{
		return false;
	}

	// 적 · 생물: 조준 종류 또는 플레이어가 아닌 ASC.
	if (const IGrimTargetable* Targetable = UGrimTargetingStatics::FindTargetable(HitActor))
	{
		const EGrimTargetKind Kind = Targetable->GetTargetKind();
		if (Kind == EGrimTargetKind::Enemy || Kind == EGrimTargetKind::Creature)
		{
			return true;
		}
	}
	if (const UAbilitySystemComponent* TargetASC = UGrimSpellAbility::FindTargetASC(const_cast<AActor*>(HitActor)))
	{
		const bool bTargetIsPlayer = Cast<APlayerState>(TargetASC->GetOwnerActor()) != nullptr;
		if (!bTargetIsPlayer || UGrimMagicSettings::Get()->bFriendlyFire)
		{
			return TargetASC != SourceASC;
		}
	}

	// 반응하는 사물: 이 마법의 자극에 맞는 규칙이 있는가(조회만 — 상태를 바꾸지 않는다).
	if (const UGrimElementalStateComponent* State = HitActor->FindComponentByClass<UGrimElementalStateComponent>())
	{
		FGrimElementalStimulus Stimulus = Spell->Stimulus;
		if (!Stimulus.Element.IsValid())
		{
			Stimulus.Element = Spell->Element;
		}
		const UDataTable* Table = State->ReactionTable ? State->ReactionTable.Get() : FGrimElementalReactionLibrary::GetDefaultTable();
		return FGrimElementalReactionLibrary::FindStimulusRule(Table, State->GetStates(), State->Material, Stimulus) != nullptr;
	}
	return false;
}

// ───────────────────────── 완드 ─────────────────────────

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

	// Snap: 완드 루트를 소켓 위치 · 회전에 그대로 맞춘다(상대 변환 = 단위). 쥐는 자세는 완드 안의 Grip 이 가진다.
	// KeepRelativeTransform 으로 붙이면 스폰한 월드 좌표가 그대로 손 기준 오프셋이 되어 완드가 몸에서 떨어진다.
	const FAttachmentTransformRules Rules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
	if (PawnMesh && PawnMesh->DoesSocketExist(SocketName))
	{
		Wand->AttachToComponent(PawnMesh, Rules, SocketName);
	}
	else
	{
		Wand->AttachToComponent(Pawn->GetRootComponent(), Rules);
	}
}

// ───────────────────────── 세이브 (Magic.Spellbook v1) ─────────────────────────

namespace GrimSpellbookSaveLocal // 유니티 빌드 이름 충돌 방지
{
	UGrimSpellDefinition* LoadSpellPath(const FString& Path)
	{
		return Path.IsEmpty() ? nullptr : Cast<UGrimSpellDefinition>(FSoftObjectPath(Path).TryLoad());
	}

	/** 깨진 개수로 큰 배열을 만들지 않게 */
	constexpr int32 MaxSavedEntries = 4096;
}

void UGrimSpellbookComponent::WriteSaveData(FArchive& Ar)
{
	int32 NumKnown = KnownSpells.Num();
	Ar << NumKnown;
	for (const TObjectPtr<UGrimSpellDefinition>& Spell : KnownSpells)
	{
		FString Path = Spell ? Spell->GetPathName() : FString();
		float XP = GetProficiencyXP(Spell);
		Ar << Path << XP;
	}

	int32 Selected = SelectedIndex;
	Ar << Selected;

	int32 NumSlots = SlotSpells.Num();
	Ar << NumSlots;
	for (const TObjectPtr<UGrimSpellDefinition>& Spell : SlotSpells)
	{
		FString Path = Spell ? Spell->GetPathName() : FString();
		Ar << Path;
	}

	int32 SlotSet = ActiveSlotSet;
	Ar << SlotSet;
}

void UGrimSpellbookComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	using namespace GrimSpellbookSaveLocal;
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 소유 클라이언트가 곧 받을 숙련도 변화를 「등급 오름」으로 알리지 않게(RPC 가 속성 복제보다 먼저 간다).
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PC && !PC->IsLocalController())
	{
		Client_SaveRestored();
	}

	int32 NumKnown = 0;
	Ar << NumKnown;
	if (NumKnown < 0 || NumKnown > MaxSavedEntries)
	{
		Ar.SetError();
		return;
	}

	TArray<UGrimSpellDefinition*> SavedOrder;
	for (int32 i = 0; i < NumKnown && !Ar.IsError(); ++i)
	{
		FString Path;
		float XP = 0.f;
		Ar << Path << XP;
		UGrimSpellDefinition* Spell = LoadSpellPath(Path);
		if (!Spell)
		{
			UE_LOG(LogGrimMagic, Warning, TEXT("[Server] %s 세이브: 마법 %s 를 찾지 못해 건너뜀"), *GetNameSafe(GetOwner()), *Path);
			continue;
		}
		if (!KnowsSpell(Spell))
		{
			GiveSpellFromSave(Spell);
		}
		SavedOrder.AddUnique(Spell);

		if (Spell->bTracksProficiency)
		{
			FGrimSpellProficiency* Item = FindProficiencyItem(Spell);
			if (!Item)
			{
				Item = &Proficiency.Items.AddDefaulted_GetRef();
				Item->Spell = Spell;
			}
			Item->XP = FMath::Max(0.f, XP);
			Item->LastGradeIndex = GetProficiency(Spell).GradeIndex;
			Proficiency.MarkItemDirty(*Item);
		}
	}

	// 배운 순서: 세이브 순서 먼저, 세이브에 없던 것(설정에 새로 생긴 시작 마법 등)은 뒤에.
	TArray<TObjectPtr<UGrimSpellDefinition>> NewKnown;
	TArray<FGameplayAbilitySpecHandle> NewHandles;
	for (UGrimSpellDefinition* Spell : SavedOrder)
	{
		const int32 Index = KnownSpells.IndexOfByKey(Spell);
		if (Index != INDEX_NONE && SpellHandles.IsValidIndex(Index))
		{
			NewKnown.Add(KnownSpells[Index]);
			NewHandles.Add(SpellHandles[Index]);
		}
	}
	for (int32 i = 0; i < KnownSpells.Num() && i < SpellHandles.Num(); ++i)
	{
		if (!NewKnown.Contains(KnownSpells[i]))
		{
			NewKnown.Add(KnownSpells[i]);
			NewHandles.Add(SpellHandles[i]);
		}
	}
	KnownSpells = MoveTemp(NewKnown);
	SpellHandles = MoveTemp(NewHandles);

	int32 Selected = INDEX_NONE;
	int32 NumSlots = 0;
	Ar << Selected << NumSlots;
	if (NumSlots < 0 || NumSlots > MaxSavedEntries)
	{
		Ar.SetError();
		NumSlots = 0;
	}
	const int32 Total = UGrimSpellbookSettings::Get()->GetTotalSlots();
	TArray<TObjectPtr<UGrimSpellDefinition>> NewSlots;
	NewSlots.SetNum(Total);
	for (int32 i = 0; i < NumSlots && !Ar.IsError(); ++i)
	{
		FString Path;
		Ar << Path;
		UGrimSpellDefinition* Spell = LoadSpellPath(Path);
		if (i < Total && Spell && KnowsSpell(Spell) && !NewSlots.Contains(Spell))
		{
			NewSlots[i] = Spell;
		}
	}
	SlotSpells = MoveTemp(NewSlots);

	int32 SlotSet = 0;
	Ar << SlotSet;
	SetActiveSlotSet(Ar.IsError() ? 0 : SlotSet);

	if (KnownSpells.Num() > 0)
	{
		SelectSpell(KnownSpells.IsValidIndex(Selected) ? Selected : 0);
	}
	UE_LOG(LogGrimMagic, Display, TEXT("[Server] %s 세이브 읽음: 배운 마법 %d, 선택 %d, 슬롯 세트 %d"), *GetNameSafe(GetOwner()), KnownSpells.Num(), SelectedIndex, ActiveSlotSet);
}

void UGrimSpellbookComponent::GiveSpellFromSave(UGrimSpellDefinition* Spell)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !Spell || KnowsSpell(Spell))
	{
		return;
	}
	const TSubclassOf<UGrimSpellAbility> AbilityClass = Spell->AbilityClass ? Spell->AbilityClass : TSubclassOf<UGrimSpellAbility>(UGrimSpellAbility::StaticClass());
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Spell));
	KnownSpells.Add(Spell);
	SpellHandles.Add(Handle);
}

void UGrimSpellbookComponent::Client_SaveRestored_Implementation()
{
	for (FGrimSpellProficiency& Item : Proficiency.Items)
	{
		Item.LastGradeIndex = INDEX_NONE;
	}
}
