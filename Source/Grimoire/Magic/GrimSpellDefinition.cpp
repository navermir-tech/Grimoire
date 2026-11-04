#include "Magic/GrimSpellDefinition.h"
#include "GameplayTagsManager.h"

UGrimSpellDefinition::UGrimSpellDefinition()
{
}

FPrimaryAssetId UGrimSpellDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Spell"), GetFName());
}

FGameplayTag UGrimSpellDefinition::RequestTagByName(FName TagName)
{
	return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
}
