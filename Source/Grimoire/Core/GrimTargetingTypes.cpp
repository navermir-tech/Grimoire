#include "Core/GrimTargetingTypes.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	template <typename InterfaceType>
	InterfaceType* FindOnActorOrComponents(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (InterfaceType* OnActor = Cast<InterfaceType>(const_cast<AActor*>(Actor)))
		{
			return OnActor;
		}
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (InterfaceType* OnComponent = Cast<InterfaceType>(Component))
			{
				return OnComponent;
			}
		}
		return nullptr;
	}
}

IGrimAimProvider* UGrimTargetingStatics::FindAimProvider(const APawn* Pawn)
{
	if (!Pawn)
	{
		return nullptr;
	}
	if (IGrimAimProvider* FromController = FindOnActorOrComponents<IGrimAimProvider>(Pawn->GetController()))
	{
		return FromController;
	}
	return FindOnActorOrComponents<IGrimAimProvider>(Pawn);
}

IGrimTargetable* UGrimTargetingStatics::FindTargetable(const AActor* Actor)
{
	return FindOnActorOrComponents<IGrimTargetable>(Actor);
}
