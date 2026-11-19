#include "Core/GrimInteractable.h"
#include "GameFramework/Pawn.h"

void UGrimInteractableSubsystem::Register(UObject* Interactable)
{
	if (Interactable && Interactable->GetClass()->ImplementsInterface(UGrimInteractable::StaticClass()))
	{
		Interactables.AddUnique(Interactable);
	}
}

void UGrimInteractableSubsystem::Unregister(UObject* Interactable)
{
	Interactables.Remove(Interactable);
}

UObject* UGrimInteractableSubsystem::FindBest(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const
{
	if (!Pawn)
	{
		return nullptr;
	}
	UObject* Best = nullptr;
	for (const TWeakObjectPtr<UObject>& Weak : Interactables)
	{
		UObject* Object = Weak.Get();
		const IGrimInteractable* Interactable = Cast<IGrimInteractable>(Object);
		if (!Interactable)
		{
			continue;
		}
		FGrimInteractionOffer Offer;
		if (Interactable->GetInteractionOffer(Pawn, Offer) && Offer.Distance < OutOffer.Distance)
		{
			OutOffer = Offer;
			Best = Object;
		}
	}
	return Best;
}
