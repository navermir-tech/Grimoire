#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureStorageWidget.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FName UGrimCreatureStorageComponent::ComponentName(TEXT("GrimCreatureStorage"));

namespace GrimCreatureStorageCompLocal // 유니티 빌드 이름 충돌 방지
{
	double NowUtc()
	{
		return static_cast<double>(FDateTime::UtcNow().ToUnixTimestamp());
	}

	bool IsAtBox(EGrimCreaturePlace Place)
	{
		return Place == EGrimCreaturePlace::Box || Place == EGrimCreaturePlace::Released;
	}

	FString NameOfRecord(const FGrimOwnedCreatureRecord& Record)
	{
		if (!Record.Instance.Nickname.IsEmpty())
		{
			return Record.Instance.Nickname;
		}
		const UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Record.Instance.SpeciesId);
		return Species && !Species->DisplayName.IsEmpty() ? Species->DisplayName.ToString() : Record.Instance.SpeciesId.ToString();
	}
}

UGrimCreatureStorageComponent::UGrimCreatureStorageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimCreatureStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimCreatureStorageComponent, bStorageLearned, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UGrimCreatureStorageComponent, PartyLimit, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UGrimCreatureStorageComponent, StorageCapacity, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UGrimCreatureStorageComponent, Stored, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UGrimCreatureStorageComponent, OwnerPlayerId, COND_OwnerOnly);
}

UGrimCreatureStorageComponent* UGrimCreatureStorageComponent::FindForActor(const AActor* Actor)
{
	const APlayerState* PlayerState = Cast<APlayerState>(Actor);
	if (!PlayerState)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			PlayerState = Pawn->GetPlayerState();
		}
		else if (const AController* Controller = Cast<AController>(Actor))
		{
			PlayerState = Controller->PlayerState;
		}
	}
	return PlayerState ? PlayerState->FindComponentByClass<UGrimCreatureStorageComponent>() : nullptr;
}

APlayerState* UGrimCreatureStorageComponent::GetPlayerState() const
{
	return Cast<APlayerState>(GetOwner());
}

APawn* UGrimCreatureStorageComponent::GetPawn() const
{
	const APlayerState* PlayerState = GetPlayerState();
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

APlayerController* UGrimCreatureStorageComponent::GetLocalOwningController() const
{
	const APlayerState* PlayerState = GetPlayerState();
	APlayerController* PC = PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
	return PC && PC->IsLocalController() ? PC : nullptr;
}

UGrimTamerComponent* UGrimCreatureStorageComponent::GetTamer() const
{
	return UGrimTamerComponent::FindForActor(GetOwner());
}

bool UGrimCreatureStorageComponent::HasAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UGrimCreatureStorageComponent::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && GetWorld())
	{
		RefreshLimits();
		const float Interval = FMath::Max(0.2f, UGrimCreatureStorageSettings::Get()->LimitRefreshSeconds);
		GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &UGrimCreatureStorageComponent::RefreshLimits, Interval, true);
	}
}

void UGrimCreatureStorageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (BoxWidget)
	{
		BoxWidget->RemoveFromParent();
		BoxWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UGrimCreatureStorageComponent::OnRep_Storage()
{
	OnStorageChanged.Broadcast(this);
}

void UGrimCreatureStorageComponent::MarkChanged()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
	OnStorageChanged.Broadcast(this);
}

void UGrimCreatureStorageComponent::Notice(const FString& Text) const
{
	if (UGrimTamerComponent* Tamer = GetTamer())
	{
		Tamer->SendNotice(Text);
	}
}

FString UGrimCreatureStorageComponent::DescribeRecord(const FGrimOwnedCreatureRecord& Record) const
{
	return GrimCreatureStorageCompLocal::NameOfRecord(Record);
}

// ───────────────────────── 읽기 ─────────────────────────

bool UGrimCreatureStorageComponent::IsMine(const AGrimCreature* Creature) const
{
	if (!Creature || !Creature->GetInstance().IsTamed())
	{
		return false;
	}
	FString MyId = OwnerPlayerId;
	if (HasAuthority())
	{
		if (const UGrimTamerComponent* Tamer = GetTamer())
		{
			MyId = Tamer->GetPlayerId();
		}
	}
	if (!MyId.IsEmpty())
	{
		return Creature->GetInstance().OwnerId == MyId;
	}
	const UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	return Taming && Taming->GetOwnerPlayerState() == GetPlayerState();
}

TArray<AGrimCreature*> UGrimCreatureStorageComponent::GetPartyCreatures() const
{
	TArray<AGrimCreature*> Out;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Out;
	}
	for (TActorIterator<AGrimCreature> It(World); It; ++It)
	{
		const UGrimCreatureTamingComponent* Taming = It->GetTaming();
		if (It->IsDead() || It->IsActorBeingDestroyed() || !Taming || Taming->IsAtBase() || !IsMine(*It))
		{
			continue;
		}
		Out.Add(*It);
	}
	Out.Sort([](const AGrimCreature& A, const AGrimCreature& B) { return A.GetInstance().Seed < B.GetInstance().Seed; });
	return Out;
}

// ───────────────────────── 서버: 상한 ─────────────────────────

void UGrimCreatureStorageComponent::RefreshLimits()
{
	if (!HasAuthority())
	{
		return;
	}
	const UGrimCreatureStorageSettings* Settings = UGrimCreatureStorageSettings::Get();
	const AGrimPlayerState* GrimState = Cast<AGrimPlayerState>(GetOwner());
	const UAbilitySystemComponent* ASC = GrimState ? GrimState->GetAbilitySystemComponent() : nullptr;
	int32 Circle = 1;
	FGameplayTagContainer Tags;
	if (ASC)
	{
		Circle = FMath::Max(1, FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute())));
		ASC->GetOwnedGameplayTags(Tags);
	}
	const int32 NewParty = FMath::Max(1, GrimCreatureStorageMath::ComputeLimit(Settings->PartyLimitByCircle, Circle, Settings->PartyLimitTagBonuses, Tags, Settings->PartyLimitMax, 1));
	const int32 NewCapacity = GrimCreatureStorageMath::ComputeStorageCapacity(Settings->StorageBaseCapacity, bStorageLearned, Settings->StorageCapacityByCircle, Circle,
		Settings->StorageCapacityTagBonuses, Tags, Settings->StorageCapacityMax);
	FString NewId = OwnerPlayerId;
	if (const UGrimTamerComponent* Tamer = GetTamer())
	{
		NewId = Tamer->GetPlayerId();
	}
	if (NewParty != PartyLimit || NewCapacity != StorageCapacity || NewId != OwnerPlayerId)
	{
		PartyLimit = NewParty;
		StorageCapacity = NewCapacity;
		OwnerPlayerId = NewId;
		MarkChanged();
	}
	BindReleasedCreatures();
}

FGrimCreatureStorageCounts UGrimCreatureStorageComponent::MakeCounts(const AGrimCreatureBox* Box) const
{
	FGrimCreatureStorageCounts Counts;
	Counts.PartyCount = GetPartyCount();
	Counts.PartyLimit = PartyLimit;
	Counts.bStorageLearned = bStorageLearned;
	Counts.StorageCount = Stored.Num();
	Counts.StorageCapacity = StorageCapacity;
	Counts.bHasBox = Box != nullptr;
	if (Box)
	{
		Counts.BoxCount = Box->GetEntries().Num();
		Counts.BoxCapacity = Box->GetCapacity();
		Counts.ReleasedCount = Box->GetReleasedCount();
		Counts.ReleasedMax = UGrimCreatureStorageSettings::Get()->BoxMaxReleased;
	}
	return Counts;
}

void UGrimCreatureStorageComponent::BindReleasedCreatures()
{
	const UWorld* World = GetWorld();
	if (!HasAuthority() || !World)
	{
		return;
	}
	for (TActorIterator<AGrimCreature> It(World); It; ++It)
	{
		UGrimCreatureTamingComponent* Taming = It->GetTaming();
		if (Taming && Taming->IsAtBase() && !IsValid(Taming->GetOwnerPlayerState()) && IsMine(*It))
		{
			Taming->SetOwnerPlayerState(GetPlayerState());
		}
	}
}

// ───────────────────────── 서버: 배우기 ─────────────────────────

EGrimLearnResult UGrimCreatureStorageComponent::LearnStorageSkill()
{
	if (!HasAuthority())
	{
		return EGrimLearnResult::Invalid;
	}
	if (bStorageLearned)
	{
		Notice(TEXT("쿠스토디아(생물 보관함)는 이미 쓸 줄 안다"));
		return EGrimLearnResult::AlreadyKnown;
	}
	const AGrimPlayerState* GrimState = Cast<AGrimPlayerState>(GetOwner());
	const UAbilitySystemComponent* ASC = GrimState ? GrimState->GetAbilitySystemComponent() : nullptr;
	const int32 Circle = ASC ? FMath::Max(1, FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()))) : 1;
	const int32 Required = UGrimCreatureStorageSettings::Get()->StorageRequiredCircle;
	if (Circle < Required)
	{
		Notice(FString::Printf(TEXT("쿠스토디아(생물 보관함)를 배우려면 %d서클이 필요하다"), Required));
		return EGrimLearnResult::CircleTooLow;
	}
	bStorageLearned = true;
	RefreshLimits();
	MarkChanged();
	Notice(FString::Printf(TEXT("쿠스토디아를 배웠다 — 생물 보관함이 %d칸으로 늘었다 (가방(L) 「생물」 탭)"), StorageCapacity));
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 생물 보관함 배움 %s 칸=%d 데리고 다님=%d"), *GetNameSafe(GetOwner()), StorageCapacity, PartyLimit);
	return EGrimLearnResult::Learned;
}

EGrimLearnResult UGrimCreatureStorageComponent::LearnNormalSkill(AActor* PlayerActor, FName SkillId)
{
	UGrimCreatureStorageComponent* Storage = FindForActor(PlayerActor);
	if (!Storage)
	{
		APlayerState* PlayerState = Cast<APlayerState>(PlayerActor);
		if (UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(PlayerActor); Subsystem && PlayerState)
		{
			Subsystem->EnsureTamer(PlayerState);
		}
		Storage = FindForActor(PlayerActor);
	}
	if (!Storage || SkillId != UGrimCreatureStorageSettings::Get()->StorageSkillId)
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] 모르는 노말 스킬 %s"), *SkillId.ToString());
		return EGrimLearnResult::Invalid;
	}
	return Storage->LearnStorageSkill();
}

void UGrimCreatureStorageComponent::DebugSetLearned(bool bLearned)
{
	if (HasAuthority() && bStorageLearned != bLearned)
	{
		bStorageLearned = bLearned;
		RefreshLimits();
		MarkChanged();
	}
}

// ───────────────────────── 서버: 새로 길들임 ─────────────────────────

EGrimNewTameDestination UGrimCreatureStorageComponent::DecideNewTame()
{
	RefreshLimits();
	const UGrimCreatureStorageSettings* Settings = UGrimCreatureStorageSettings::Get();
	const APawn* Pawn = GetPawn();
	const AGrimCreatureBox* Box = Settings->bSendOverflowToBox ? AGrimCreatureBox::FindNearestWithRoom(GetWorld(), Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector) : nullptr;
	return GrimCreatureStorageMath::DecideNewTame(MakeCounts(Box), Settings->bSendOverflowToBox);
}

void UGrimCreatureStorageComponent::PlaceNewlyTamed(AGrimCreature* Creature, EGrimNewTameDestination Destination, bool bDeferred)
{
	if (!HasAuthority() || !Creature || Destination == EGrimNewTameDestination::Party || Destination == EGrimNewTameDestination::Refused)
	{
		return;
	}
	TWeakObjectPtr<AGrimCreature> WeakCreature = Creature;
	if (bDeferred && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakCreature, Destination]()
		{
			FinishPlaceNewlyTamed(WeakCreature, Destination);
		}));
		return;
	}
	FinishPlaceNewlyTamed(WeakCreature, Destination);
}

void UGrimCreatureStorageComponent::FinishPlaceNewlyTamed(TWeakObjectPtr<AGrimCreature> WeakCreature, EGrimNewTameDestination Destination)
{
	AGrimCreature* Creature = WeakCreature.Get();
	UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	if (!Taming || Creature->IsDead() || !IsMine(Creature))
	{
		return;
	}
	FGrimOwnedCreatureRecord Record = Taming->MakeRecord();
	Record.SavedUtcSeconds = GrimCreatureStorageCompLocal::NowUtc();
	const FString Name = DescribeRecord(Record);
	bool bPlaced = false;
	if (Destination == EGrimNewTameDestination::Storage && Stored.Num() < StorageCapacity)
	{
		Stored.Add(Record);
		bPlaced = true;
		Notice(FString::Printf(TEXT("데리고 다니는 수가 가득 — %s 을(를) 보관함에 넣었다 (%d/%d)"), *Name, Stored.Num(), StorageCapacity));
	}
	else if (Destination == EGrimNewTameDestination::Box)
	{
		const APawn* Pawn = GetPawn();
		AGrimCreatureBox* Box = AGrimCreatureBox::FindNearestWithRoom(GetWorld(), Pawn ? Pawn->GetActorLocation() : Creature->GetActorLocation());
		if (Box && Box->AddEntry(Record, false))
		{
			bPlaced = true;
			Notice(FString::Printf(TEXT("데리고 다니는 수 · 보관함이 가득 — %s 을(를) 거점 생물 상자로 보냈다"), *Name));
		}
	}
	if (!bPlaced)
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] 새로 길들인 %s seed=%d 를 보낼 곳이 없어 곁에 둔다(넘침)"), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed);
		return;
	}
	if (UGrimTamerComponent* Tamer = GetTamer())
	{
		Tamer->RemoveRecord(Record.Instance);
	}
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 새로 길들인 %s seed=%d → %s"), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed,
		Destination == EGrimNewTameDestination::Storage ? TEXT("Storage") : TEXT("Box"));
	Creature->Destroy();
	MarkChanged();
}

bool UGrimCreatureStorageComponent::AcceptOverflowRecord(const FGrimOwnedCreatureRecord& Record)
{
	RefreshLimits();
	if (!HasAuthority() || Stored.Num() >= StorageCapacity
		|| GrimCreatureStorageMath::FindRecord(Stored, Record.Instance.SpeciesId, Record.Instance.Seed) != INDEX_NONE)
	{
		return false;
	}
	FGrimOwnedCreatureRecord Copy = Record;
	Copy.SavedUtcSeconds = GrimCreatureStorageCompLocal::NowUtc();
	Stored.Add(Copy);
	MarkChanged();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 데리고 다니는 수를 넘어 보관함으로 %s seed=%d"), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed);
	return true;
}

// ───────────────────────── 서버: 옮기기 ─────────────────────────

AGrimCreature* UGrimCreatureStorageComponent::SpawnNearOwner(const FGrimOwnedCreatureRecord& Record)
{
	UWorld* World = GetWorld();
	UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Record.Instance.SpeciesId);
	if (!World || !Species)
	{
		return nullptr;
	}
	const APawn* Pawn = GetPawn();
	const FGrimTamedBehaviorTuning& Behavior = UGrimTamingSettings::Get()->Behavior;
	FVector Ground = FVector::ZeroVector;
	float Yaw = 0.f;
	if (Pawn)
	{
		const float Angle = 180.f + FMath::FRandRange(-60.f, 60.f);
		const FVector Dir = Pawn->GetActorForwardVector().GetSafeNormal2D().RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Point = Pawn->GetActorLocation() + Dir * Behavior.SummonDistance;
		Ground = Point - FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight());
		UGrimEcosystemSubsystem::FindGround(World, Point, Ground);
		Yaw = Pawn->GetActorRotation().Yaw;
	}
	AGrimCreature* Creature = AGrimCreature::SpawnCreature(World, Species, Record.Instance, Ground, Yaw, Record.HealthRatio);
	UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	if (!Taming)
	{
		if (Creature)
		{
			Creature->Destroy();
		}
		return nullptr;
	}
	FGrimOwnedCreatureRecord Restored = Record;
	Restored.Command = EGrimTamedCommand::Follow;
	Taming->RestoreOwned(GetPlayerState(), Restored, 0.0);
	return Creature;
}

EGrimCreatureStorageResult UGrimCreatureStorageComponent::Transfer(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To, bool bCheckReach)
{
	using namespace GrimCreatureStorageCompLocal;
	if (!HasAuthority())
	{
		return EGrimCreatureStorageResult::Invalid;
	}
	RefreshLimits();
	UGrimTamerComponent* Tamer = GetTamer();
	const FString MyId = Tamer ? Tamer->GetPlayerId() : OwnerPlayerId;
	auto Fail = [this, SpeciesId, Seed, From, To](EGrimCreatureStorageResult Result)
	{
		Notice(GrimCreatureStorageMath::ResultText(Result));
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 생물 옮기기 거절 %s seed=%d %s → %s: %s"), *SpeciesId.ToString(), Seed, GrimCreatureStorageMath::PlaceName(From),
			GrimCreatureStorageMath::PlaceName(To), *GrimCreatureStorageMath::ResultText(Result));
		return Result;
	};

	const bool bNeedsBox = IsAtBox(From) || IsAtBox(To);
	if (bNeedsBox && (!Box || (bCheckReach && !Box->IsWithinManageRange(GetPawn(), UGrimCreatureStorageSettings::Get()->BoxServerReachSlack))))
	{
		return Fail(EGrimCreatureStorageResult::NoBox);
	}

	// 1) 원래 자리에 있나
	AGrimCreature* SourceActor = nullptr;
	int32 StoredIndex = INDEX_NONE;
	switch (From)
	{
	case EGrimCreaturePlace::Party:
		for (AGrimCreature* Creature : GetPartyCreatures())
		{
			if (Creature->GetInstance().SpeciesId == SpeciesId && Creature->GetInstance().Seed == Seed)
			{
				SourceActor = Creature;
			}
		}
		if (!SourceActor)
		{
			return Fail(EGrimCreatureStorageResult::NotFound);
		}
		break;
	case EGrimCreaturePlace::Storage:
		StoredIndex = GrimCreatureStorageMath::FindRecord(Stored, SpeciesId, Seed);
		if (StoredIndex == INDEX_NONE)
		{
			return Fail(EGrimCreatureStorageResult::NotFound);
		}
		break;
	default:
	{
		const int32 BoxIndex = GrimCreatureStorageMath::FindBoxEntry(Box->GetEntries(), SpeciesId, Seed);
		const FGrimCreatureBoxEntry* Entry = BoxIndex != INDEX_NONE ? &Box->GetEntries()[BoxIndex] : nullptr;
		if (!Entry || Entry->Record.Instance.OwnerId != MyId || Entry->bReleased != (From == EGrimCreaturePlace::Released))
		{
			return Fail(EGrimCreatureStorageResult::NotFound);
		}
		break;
	}
	}

	// 2) 새 자리에 자리가 있나
	const EGrimCreatureStorageResult Check = GrimCreatureStorageMath::CheckTransfer(From, To, MakeCounts(bNeedsBox ? Box : nullptr));
	if (Check != EGrimCreatureStorageResult::Ok)
	{
		return Fail(Check);
	}

	// 3) 상자 안 ↔ 풀어놓음은 상자 안에서
	FString Name;
	if (From == EGrimCreaturePlace::Box && To == EGrimCreaturePlace::Released)
	{
		if (!Box->ReleaseEntry(SpeciesId, Seed))
		{
			return Fail(EGrimCreatureStorageResult::Invalid);
		}
	}
	else if (From == EGrimCreaturePlace::Released && To == EGrimCreaturePlace::Box)
	{
		if (!Box->RecallEntry(SpeciesId, Seed))
		{
			return Fail(EGrimCreatureStorageResult::Invalid);
		}
	}
	else
	{
		// 4) 빼기
		FGrimOwnedCreatureRecord Record;
		switch (From)
		{
		case EGrimCreaturePlace::Party:
		{
			if (UGrimMountComponent* Mount = SourceActor->GetMount(); Mount && Mount->IsRidden())
			{
				Mount->Dismount(true);
			}
			Record = SourceActor->GetTaming()->MakeRecord();
			if (Tamer)
			{
				Tamer->RemoveRecord(Record.Instance);
			}
			SourceActor->Destroy();
			break;
		}
		case EGrimCreaturePlace::Storage:
			Record = Stored[StoredIndex];
			Stored.RemoveAt(StoredIndex);
			break;
		default:
			if (!Box->TakeEntry(SpeciesId, Seed, Record))
			{
				return Fail(EGrimCreatureStorageResult::NotFound);
			}
			break;
		}
		// 세계에서 왔으면 지금부터 보관(회복) 시간을 센다. 보관함 ↔ 상자는 그대로 이어 센다.
		if (From == EGrimCreaturePlace::Party || From == EGrimCreaturePlace::Released)
		{
			Record.SavedUtcSeconds = NowUtc();
		}

		// 5) 넣기
		bool bPlaced = false;
		switch (To)
		{
		case EGrimCreaturePlace::Party:
		{
			Record.HealthRatio = GrimCreatureStorageMath::GetHealedRatio(Record.HealthRatio, Record.SavedUtcSeconds, NowUtc(), UGrimCreatureStorageSettings::Get()->StoredHealPerMinute);
			if (AGrimCreature* Spawned = SpawnNearOwner(Record))
			{
				if (Tamer)
				{
					Tamer->AddOrUpdateRecord(Spawned->GetTaming()->MakeRecord());
				}
				bPlaced = true;
			}
			break;
		}
		case EGrimCreaturePlace::Storage:
			Stored.Add(Record);
			bPlaced = true;
			break;
		case EGrimCreaturePlace::Box:
			bPlaced = Box->AddEntry(Record, false);
			break;
		case EGrimCreaturePlace::Released:
			bPlaced = Box->AddEntry(Record, true);
			break;
		default:
			break;
		}
		if (!bPlaced)
		{
			// 잃지 않는다 — 보관함에 그대로 둔다(칸을 넘더라도).
			UE_LOG(LogGrimTaming, Warning, TEXT("[Server] %s seed=%d 를 %s 에 넣지 못해 보관함에 둔다"), *SpeciesId.ToString(), Seed, GrimCreatureStorageMath::PlaceName(To));
			Stored.Add(Record);
			MarkChanged();
			Notice(TEXT("옮기지 못해 보관함에 두었다"));
			return EGrimCreatureStorageResult::Invalid;
		}
		Name = DescribeRecord(Record);
	}

	if (Name.IsEmpty())
	{
		const int32 BoxIndex = GrimCreatureStorageMath::FindBoxEntry(Box->GetEntries(), SpeciesId, Seed);
		Name = BoxIndex != INDEX_NONE ? DescribeRecord(Box->GetEntries()[BoxIndex].Record) : SpeciesId.ToString();
	}
	MarkChanged();
	Notice(FString::Printf(TEXT("%s → %s"), *Name, *GrimCreatureStorageMath::PlaceDisplayName(To)));
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 생물 옮김 %s seed=%d %s → %s (데리고 다님 %d/%d · 보관함 %d/%d%s)"), *SpeciesId.ToString(), Seed,
		GrimCreatureStorageMath::PlaceName(From), GrimCreatureStorageMath::PlaceName(To), GetPartyCount(), PartyLimit, Stored.Num(), StorageCapacity,
		Box ? *FString::Printf(TEXT(" · 상자 %d/%d"), Box->GetEntries().Num(), Box->GetCapacity()) : TEXT(""));
	return EGrimCreatureStorageResult::Ok;
}

void UGrimCreatureStorageComponent::Server_Transfer_Implementation(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To)
{
	Transfer(Box, SpeciesId, Seed, From, To, true);
}

void UGrimCreatureStorageComponent::RequestTransfer(AGrimCreatureBox* Box, FName SpeciesId, int32 Seed, EGrimCreaturePlace From, EGrimCreaturePlace To)
{
	if (HasAuthority())
	{
		Transfer(Box, SpeciesId, Seed, From, To, true);
	}
	else
	{
		Server_Transfer(Box, SpeciesId, Seed, From, To);
	}
}

// ───────────────────────── 로컬: 상자 화면 ─────────────────────────

void UGrimCreatureStorageComponent::OpenBoxScreen(AGrimCreatureBox* Box)
{
	APlayerController* PC = GetLocalOwningController();
	if (!PC || !Box || !PC->GetLocalPlayer())
	{
		return;
	}
	if (!BoxWidget)
	{
		BoxWidget = CreateWidget<UGrimCreatureStorageWidget>(PC, UGrimCreatureStorageWidget::StaticClass());
		if (!BoxWidget)
		{
			return;
		}
		BoxWidget->SetStandaloneScreen(true);
		BoxWidget->SetStorage(this);
		BoxWidget->AddToViewport(19);
	}
	BoxWidget->OpenScreen(Box);
}

void UGrimCreatureStorageComponent::CloseBoxScreen()
{
	if (BoxWidget)
	{
		BoxWidget->CloseScreen();
	}
}

bool UGrimCreatureStorageComponent::IsBoxScreenOpen() const
{
	return BoxWidget && BoxWidget->IsScreenOpen();
}

// ───────────────────────── 세이브 ─────────────────────────

void UGrimCreatureStorageComponent::WriteSaveData(FArchive& Ar)
{
	bool bLearned = bStorageLearned;
	Ar << bLearned;
	GrimTamingMath::SerializeOwnedRecords(Ar, Stored);
}

void UGrimCreatureStorageComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	bool bLearned = false;
	Ar << bLearned;
	TArray<FGrimOwnedCreatureRecord> Loaded;
	if (Ar.IsError() || !GrimTamingMath::SerializeOwnedRecords(Ar, Loaded))
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] Creatures.Storage 를 읽지 못했다(v%d)"), Version);
		return;
	}
	// 불러오기 전에 넘쳐 들어온 기록(접속 때 다시 부르기)은 잃지 않게 뒤에 붙인다.
	for (const FGrimOwnedCreatureRecord& Existing : Stored)
	{
		if (GrimCreatureStorageMath::FindRecord(Loaded, Existing.Instance.SpeciesId, Existing.Instance.Seed) == INDEX_NONE)
		{
			Loaded.Add(Existing);
		}
	}
	bStorageLearned = bLearned;
	Stored = MoveTemp(Loaded);
	RefreshLimits();
	MarkChanged();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 생물 보관함을 읽었다: 배움=%d 기록 %d"), bStorageLearned ? 1 : 0, Stored.Num());
}
