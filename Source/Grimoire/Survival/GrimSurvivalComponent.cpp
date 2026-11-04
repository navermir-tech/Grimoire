#include "Survival/GrimSurvivalComponent.h"

UGrimSurvivalComponent::UGrimSurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}
