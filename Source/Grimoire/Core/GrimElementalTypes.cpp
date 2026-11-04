#include "Core/GrimElementalTypes.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

IGrimElementalReceiver* UGrimElementalStatics::FindReceiver(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	AActor* Mutable = const_cast<AActor*>(Actor);
	if (IGrimElementalReceiver* Self = Cast<IGrimElementalReceiver>(Mutable))
	{
		return Self;
	}

	for (UActorComponent* Component : Mutable->GetComponents())
	{
		if (IGrimElementalReceiver* Receiver = Cast<IGrimElementalReceiver>(Component))
		{
			return Receiver;
		}
	}
	return nullptr;
}

bool UGrimElementalStatics::ApplyStimulusToActor(AActor* Target, const FGrimElementalStimulus& Stimulus, AActor* Instigator)
{
	if (!Target || !Target->HasAuthority() || !Stimulus.Element.IsValid())
	{
		return false;
	}

	if (IGrimElementalReceiver* Receiver = FindReceiver(Target))
	{
		Receiver->ApplyElementalStimulus(Stimulus, Instigator);
		return true;
	}
	return false;
}

float UGrimElementalStatics::GetStateIntensity(const AActor* Target, FGameplayTag StateTag)
{
	if (const IGrimElementalReceiver* Receiver = FindReceiver(Target))
	{
		return Receiver->GetElementalStateIntensity(StateTag);
	}
	return 0.f;
}
