#include "Core/GrimSpellFXTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace GrimCueTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Cast, "GameplayCue.Spell.Cast", "마법 시전 연출(완드 끝 섬광 · 그리모어 책장 넘김)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Channel, "GameplayCue.Spell.Channel", "누르는 동안 이어지는 마법 연출(물줄기)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Impact, "GameplayCue.Spell.Impact", "마법이 맞은 곳 연출");
}

void UGrimSpellVisualSet::TintNiagara(UNiagaraComponent* Component, const FLinearColor& Color)
{
	const UNiagaraSystem* System = Component ? Component->GetAsset() : nullptr;
	if (!System)
	{
		return;
	}
	TArray<FNiagaraVariable> UserParameters;
	System->GetExposedParameters().GetUserParameters(UserParameters);
	for (const FNiagaraVariable& Parameter : UserParameters)
	{
		const FString Name = Parameter.GetName().ToString();
		if (Parameter.GetType() != FNiagaraTypeDefinition::GetColorDef() || !Name.Contains(TEXT("Color"))
			|| Name.Contains(TEXT("Smoke")) || Name.Contains(TEXT("Dirt")) || Name.Contains(TEXT("Dust")))
		{
			continue;
		}
		Component->SetVariableLinearColor(Parameter.GetName(), Color);
	}
}
