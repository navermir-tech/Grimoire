#include "Magic/GrimSpellbookComponent.h"

UGrimSpellbookComponent::UGrimSpellbookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}
