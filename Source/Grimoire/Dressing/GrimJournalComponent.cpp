#include "Dressing/GrimJournalComponent.h"
#include "Dressing/GrimDressingSettings.h"
#include "Dressing/GrimDressingTypes.h"
#include "Items/GrimItemStatics.h"
#include "UI/GrimHUDWidget.h"
#include "UI/GrimUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "GrimJournal"

const FName UGrimJournalComponent::ComponentName(TEXT("GrimJournal"));

UGrimJournalComponent::UGrimJournalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UGrimJournalComponent* UGrimJournalComponent::Find(const AActor* PlayerActor)
{
	const APlayerState* PlayerState = Cast<APlayerState>(PlayerActor);
	if (!PlayerState)
	{
		if (const APawn* Pawn = Cast<APawn>(PlayerActor))
		{
			PlayerState = Pawn->GetPlayerState();
		}
		else if (const AController* Controller = Cast<AController>(PlayerActor))
		{
			PlayerState = Controller->PlayerState;
		}
	}
	return PlayerState ? PlayerState->FindComponentByClass<UGrimJournalComponent>() : nullptr;
}

void UGrimJournalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimJournalComponent, Entries, COND_OwnerOnly);
}

void UGrimJournalComponent::WriteSaveData(FArchive& Ar)
{
	Ar << Entries;
}

void UGrimJournalComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FName> Loaded;
	Ar << Loaded;
	Entries = MoveTemp(Loaded);
}

int32 UGrimJournalComponent::CountGroup(FName Group) const
{
	int32 Count = 0;
	for (const FName& Id : Entries)
	{
		const FGrimJournalRow* Row = FGrimDressingData::FindJournal(Id);
		if (Row && Row->Group == Group)
		{
			++Count;
		}
	}
	return Count;
}

bool UGrimJournalComponent::AddEntry(FName EntryId)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || EntryId.IsNone() || Entries.Contains(EntryId))
	{
		return false;
	}
	Entries.Add(EntryId);

	const FGrimJournalRow* Row = FGrimDressingData::FindJournal(EntryId);
	const FName Group = Row ? Row->Group : NAME_None;
	const int32 GroupCount = Group.IsNone() ? 0 : CountGroup(Group);
	UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s 도감 +%s (%s %d)"), *GetNameSafe(Owner), *EntryId.ToString(), *Group.ToString(), GroupCount);

	// 잠정(Q152): 「쪽」이 설정만큼 찰 때마다 한 번 — 새 화폐 없이 세계 안의 물건으로
	const UGrimDressingSettings* Settings = GetDefault<UGrimDressingSettings>();
	if (Group == FName(TEXT("Page")) && Settings->PageMilestoneEvery > 0 && GroupCount % Settings->PageMilestoneEvery == 0
		&& !Settings->PageMilestoneItem.IsNone())
	{
		UGrimItemStatics::GiveItem(Owner, Settings->PageMilestoneItem, Settings->PageMilestoneCount);
	}

	const APlayerState* PlayerState = Cast<APlayerState>(Owner);
	const APlayerController* PC = PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
	if (PC && PC->IsLocalController())
	{
		ShowDiscovered(EntryId, GroupCount); // 리슨 호스트 자신
	}
	else
	{
		Client_Discovered(EntryId, GroupCount);
	}
	return true;
}

void UGrimJournalComponent::OnRep_Entries()
{
}

void UGrimJournalComponent::Client_Discovered_Implementation(FName EntryId, int32 GroupCount)
{
	ShowDiscovered(EntryId, GroupCount);
}

void UGrimJournalComponent::ShowDiscovered(FName EntryId, int32 GroupCount)
{
	OnEntryAdded.Broadcast(this, EntryId);

	const FGrimJournalRow* Row = FGrimDressingData::FindJournal(EntryId);
	const FString Title = Row ? Row->Title : EntryId.ToString();
	const FString Text = Row ? Row->Text : FString();
	UE_LOG(LogGrimDressing, Display, TEXT("[%s] 도감 발견 %s — %s"), GetWorld() && GetWorld()->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"), *EntryId.ToString(), *Title);

	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UGrimUISubsystem* UI = LocalPlayer->GetSubsystem<UGrimUISubsystem>(); UI && UI->GetHUD())
		{
			const FText Line = Row && Row->Group == FName(TEXT("Page"))
				? FText::Format(LOCTEXT("Page", "도감 — 「{0}」 ({1}쪽째)\n{2}"), FText::FromString(Title), GroupCount, FText::FromString(Text))
				: FText::Format(LOCTEXT("Entry", "도감 — 「{0}」\n{1}"), FText::FromString(Title), FText::FromString(Text));
			UI->GetHUD()->ShowToast(Line, FLinearColor(0.95f, 0.9f, 0.7f));
		}
	}
}

#undef LOCTEXT_NAMESPACE
