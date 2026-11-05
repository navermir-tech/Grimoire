#include "Progression/GrimProgressionEffects.h"
#include "Progression/GrimProgressionTags.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"

namespace GrimProgressionEffectsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	struct FAttributeEntry
	{
		const TCHAR* Key;
		FGameplayAttribute (*GetAttribute)();
		bool bScale;
		FGameplayTag (*GetTag)();
		float FGrimProgressionBonus::* Field;
	};

	FGameplayTag TagMaxHealth() { return GrimProgressionTags::SetByCaller_MaxHealth; }
	FGameplayTag TagMaxMana() { return GrimProgressionTags::SetByCaller_MaxMana; }
	FGameplayTag TagHeatResist() { return GrimProgressionTags::SetByCaller_HeatResist; }
	FGameplayTag TagColdResist() { return GrimProgressionTags::SetByCaller_ColdResist; }
	FGameplayTag TagDiseaseResist() { return GrimProgressionTags::SetByCaller_DiseaseResist; }
	FGameplayTag TagMoveSpeedScale() { return GrimProgressionTags::SetByCaller_MoveSpeedScale; }
	FGameplayTag TagCastSpeedScale() { return GrimProgressionTags::SetByCaller_CastSpeedScale; }
	FGameplayTag TagManaCostScale() { return GrimProgressionTags::SetByCaller_ManaCostScale; }

	const FAttributeEntry Entries[] = {
		{ TEXT("MaxHealth"), &UGrimVitalAttributeSet::GetMaxHealthAttribute, false, &TagMaxHealth, &FGrimProgressionBonus::MaxHealth },
		{ TEXT("MaxMana"), &UGrimMagicAttributeSet::GetMaxManaAttribute, false, &TagMaxMana, &FGrimProgressionBonus::MaxMana },
		{ TEXT("HeatResist"), &UGrimVitalAttributeSet::GetHeatResistAttribute, false, &TagHeatResist, &FGrimProgressionBonus::HeatResist },
		{ TEXT("ColdResist"), &UGrimVitalAttributeSet::GetColdResistAttribute, false, &TagColdResist, &FGrimProgressionBonus::ColdResist },
		{ TEXT("DiseaseResist"), &UGrimVitalAttributeSet::GetDiseaseResistAttribute, false, &TagDiseaseResist, &FGrimProgressionBonus::DiseaseResist },
		{ TEXT("MoveSpeedScale"), &UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute, true, &TagMoveSpeedScale, &FGrimProgressionBonus::MoveSpeedScale },
		{ TEXT("CastSpeedScale"), &UGrimVitalAttributeSet::GetCastSpeedScaleAttribute, true, &TagCastSpeedScale, &FGrimProgressionBonus::CastSpeedScale },
		{ TEXT("ManaCostScale"), &UGrimVitalAttributeSet::GetManaCostScaleAttribute, true, &TagManaCostScale, &FGrimProgressionBonus::ManaCostScale },
	};

	const FAttributeEntry* FindEntry(FName Key)
	{
		for (const FAttributeEntry& Entry : Entries)
		{
			if (Key == FName(Entry.Key))
			{
				return &Entry;
			}
		}
		return nullptr;
	}
}

UGrimGE_ProgressionBonus::UGrimGE_ProgressionBonus()
{
	using namespace GrimProgressionEffectsLocal;
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	for (const FAttributeEntry& Entry : Entries)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = Entry.GetTag();
		FGameplayModifierInfo Info;
		Info.Attribute = Entry.GetAttribute();
		Info.ModifierOp = Entry.bScale ? EGameplayModOp::MultiplyCompound : EGameplayModOp::AddBase;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Info);
	}
}

bool FGrimProgressionBonus::IsNeutral() const
{
	return FMath::IsNearlyZero(MaxHealth) && FMath::IsNearlyZero(MaxMana) && FMath::IsNearlyZero(HeatResist) && FMath::IsNearlyZero(ColdResist)
		&& FMath::IsNearlyZero(DiseaseResist) && FMath::IsNearlyEqual(MoveSpeedScale, 1.f) && FMath::IsNearlyEqual(CastSpeedScale, 1.f)
		&& FMath::IsNearlyEqual(ManaCostScale, 1.f) && GrantedTags.IsEmpty();
}

bool FGrimProgressionEffects::IsAttributeKey(FName Key)
{
	return GrimProgressionEffectsLocal::FindEntry(Key) != nullptr;
}

FGrimProgressionBonus FGrimProgressionEffects::MakeBonus(const TMap<FName, FGrimSkillEffectTotal>& Totals, TArray<FName>* OutUnknownKeys)
{
	FGrimProgressionBonus Bonus;
	for (const TPair<FName, FGrimSkillEffectTotal>& Pair : Totals)
	{
		if (const GrimProgressionEffectsLocal::FAttributeEntry* Entry = GrimProgressionEffectsLocal::FindEntry(Pair.Key))
		{
			Bonus.*(Entry->Field) = Entry->bScale ? Pair.Value.Mul * (1.f + Pair.Value.Add) : Pair.Value.Add;
			continue;
		}
		const FGameplayTag Tag = GrimProgressionTags::FindNodeEffectTag(Pair.Key);
		if (Tag.IsValid())
		{
			Bonus.GrantedTags.AddTag(Tag);
		}
		else if (OutUnknownKeys)
		{
			OutUnknownKeys->Add(Pair.Key);
		}
	}
	return Bonus;
}

bool FGrimProgressionEffects::ApplyBonus(UAbilitySystemComponent* ASC, const FGrimProgressionBonus& Bonus, FActiveGameplayEffectHandle& InOutHandle)
{
	using namespace GrimProgressionEffectsLocal;
	if (!ASC || !ASC->AbilityActorInfo.IsValid() || !ASC->IsOwnerActorAuthoritative())
	{
		return false;
	}

	FActiveGameplayEffectHandle NewHandle;
	if (!Bonus.IsNeutral())
	{
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGrimGE_ProgressionBonus::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (!Spec.IsValid())
		{
			return false;
		}
		for (const FAttributeEntry& Entry : Entries)
		{
			Spec.Data->SetSetByCallerMagnitude(Entry.GetTag(), Bonus.*(Entry.Field));
		}
		Spec.Data->DynamicGrantedTags.AppendTags(Bonus.GrantedTags);
		NewHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	if (InOutHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(InOutHandle);
	}
	InOutHandle = NewHandle;
	return true;
}
