#include "Factions/GrimTrialTarget.h"
#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestTypes.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

AGrimTrialTarget::AGrimTrialTarget()
{
	bReplicates = true;
	Kind = EGrimTargetKind::Enemy;
	bTargetable = false;
	MoveAmplitude = FVector(0.f, 300.f, 0.f);
	MovePeriodSeconds = 4.f;
}

void AGrimTrialTarget::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimTrialTarget, bTrialActive);
	DOREPLIFETIME(AGrimTrialTarget, bBroken);
}

void AGrimTrialTarget::BeginPlay()
{
	Super::BeginPlay();
	if (Mesh && Mesh->GetStaticMesh())
	{
		BodyMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (HasAuthority() && bStartActive)
	{
		bTrialActive = true;
	}
	ApplyActiveVisuals();
}

void AGrimTrialTarget::SetTrialActive(bool bActive)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bActive && !bTrialActive)
	{
		HitCount = 0;
		bBroken = false;
	}
	bTrialActive = bActive;
	ApplyActiveVisuals();
	ForceNetUpdate();
}

void AGrimTrialTarget::OnRep_TrialActive()
{
	ApplyActiveVisuals();
}

void AGrimTrialTarget::ApplyActiveVisuals()
{
	// 꺼져 있으면 조준되지 않는다(적으로 잡히면 근처 침대 잠도 막는다).
	SetTargetable(bTrialActive && !bBroken);
	SetActorHiddenInGame(bBroken);
	SetActorEnableCollision(!bBroken);
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), bTrialActive ? ActiveColor : InactiveColor);
	}
}

void AGrimTrialTarget::ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* StimulusInstigator)
{
	if (!HasAuthority() || !bTrialActive || bBroken || (bCountOnlySpells && Stimulus.Source != EGrimStimulusSource::Spell))
	{
		return;
	}
	++HitCount;

	UGrimQuestComponent* Quest = UGrimQuestComponent::FindForActor(StimulusInstigator);
	if (!Quest && StimulusInstigator)
	{
		Quest = UGrimQuestComponent::FindForActor(StimulusInstigator->GetInstigator());
	}
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] 시험 표적 %s 명중 %d (%s %.2f, 쏜 쪽 %s)"), *GetName(), HitCount,
		*Stimulus.Element.ToString(), Stimulus.Intensity, *GetNameSafe(StimulusInstigator));
	const bool bJustBroken = HitsToBreak > 0 && HitCount >= HitsToBreak;
	if (bJustBroken)
	{
		bBroken = true;
		ApplyActiveVisuals();
		ForceNetUpdate();
		UE_LOG(LogGrimQuest, Display, TEXT("[Server] 과녁 %s 부서짐"), *GetName());
	}
	if (!Quest)
	{
		return;
	}
	for (const FName& Tag : Tags)
	{
		Quest->ReceiveSignal({ EGrimQuestObjectiveType::HitTarget, Tag, 1 });
		if (bJustBroken)
		{
			Quest->ReceiveSignal({ EGrimQuestObjectiveType::BreakTarget, Tag, 1 });
		}
	}
}
