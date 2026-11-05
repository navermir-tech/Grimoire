#include "Items/GrimCookingPot.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace GrimCookingPotLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor PotColor(0.12f, 0.12f, 0.13f);
	const FLinearColor BrothColor(0.55f, 0.42f, 0.2f);
	const FLinearColor BoilingColor(0.45f, 0.24f, 0.08f);
}

AGrimCookingPot::AGrimCookingPot()
{
	PrimaryActorTick.bCanEverTick = false;
	// 서버가 RPC 인자로 받으려면 네트워크로 가리킬 수 있어야 한다(놓인 냄비는 이름으로, 스폰한 것은 복제로).
	bReplicates = true;
	SetNetUpdateFrequency(4.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// 원점 = 냄비 바닥. 지름 50cm · 높이 35cm. 마법(투사체)이 아래 아궁이에 닿게 콜리전은 없다.
	PotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PotMesh"));
	PotMesh->SetupAttachment(Root);
	PotMesh->SetRelativeLocation(FVector(0.f, 0.f, 17.5f));
	PotMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.35f));
	PotMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BrothMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrothMesh"));
	BrothMesh->SetupAttachment(Root);
	BrothMesh->SetRelativeLocation(FVector(0.f, 0.f, 35.5f));
	BrothMesh->SetRelativeScale3D(FVector(0.44f, 0.44f, 0.02f));
	BrothMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BrothMesh->SetVisibility(false);

	if (CylinderMesh.Succeeded())
	{
		PotMesh->SetStaticMesh(CylinderMesh.Object);
		BrothMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void AGrimCookingPot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimCookingPot, Contents);
	DOREPLIFETIME(AGrimCookingPot, Progress);
	DOREPLIFETIME(AGrimCookingPot, bHeated);
}

void AGrimCookingPot::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (UMaterialInstanceDynamic* PotMaterial = PotMesh->CreateDynamicMaterialInstance(0))
		{
			PotMaterial->SetVectorParameterValue(TEXT("Color"), GrimCookingPotLocal::PotColor);
		}
		BrothMaterial = BrothMesh->CreateDynamicMaterialInstance(0);
	}
	RefreshVisuals();
	UpdateTimer();
}

void AGrimCookingPot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CookTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// ───── 세이브 ─────

FName AGrimCookingPot::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("CookingPot.%s"), *GetName()));
}

void AGrimCookingPot::WriteSaveData(FArchive& Ar)
{
	int32 Num = Contents.Num();
	Ar << Num;
	for (FGrimItemInstance Item : Contents)
	{
		Item.Serialize(Ar);
	}
	float SavedProgress = Progress;
	Ar << SavedProgress;
}

void AGrimCookingPot::ReadSaveData(FArchive& Ar, int32 Version)
{
	int32 Num = 0;
	Ar << Num;
	TArray<FGrimItemInstance> Loaded;
	for (int32 Index = 0; Index < FMath::Clamp(Num, 0, 64) && !Ar.IsError(); ++Index)
	{
		FGrimItemInstance Item;
		Item.Serialize(Ar);
		if (!Item.IsEmpty() && UGrimItemSettings::GetDatabase().Find(Item.ItemId))
		{
			Loaded.Add(Item);
		}
	}
	float LoadedProgress = 0.f;
	Ar << LoadedProgress;
	if (Ar.IsError() || !HasAuthority())
	{
		return;
	}
	Contents = Loaded;
	Progress = FMath::Max(0.f, LoadedProgress);
	RefreshVisuals();
	UpdateTimer();
}

// ───── 읽기 ─────

const FGrimRecipeDefinition* AGrimCookingPot::GetMatchingRecipe() const
{
	return UGrimItemSettings::GetDatabase().FindCompleteRecipe(StationId, Contents);
}

FGrimRecipeProgress AGrimCookingPot::DescribeBestRecipe() const
{
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	FGrimRecipeProgress Best;
	int32 BestScore = -1;
	for (const FGrimRecipeDefinition& Recipe : Database.Recipes)
	{
		if (Recipe.Row.Station != StationId)
		{
			continue;
		}
		const FGrimRecipeProgress Candidate = Database.EvaluateRecipe(Recipe, Contents);
		int32 Score = 0;
		for (int32 Filled : Candidate.Filled)
		{
			Score += Filled;
		}
		if (Candidate.bHasExtra)
		{
			Score -= 1000;
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

bool AGrimCookingPot::IsWithinReach(const APawn* Pawn, float Slack) const
{
	if (!Pawn)
	{
		return false;
	}
	const float Radius = UGrimItemSettings::Get()->PotInteractRadius + FMath::Max(0.f, Slack);
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

AGrimCookingPot* AGrimCookingPot::FindNearestReachable(const APawn* Pawn)
{
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	AGrimCookingPot* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGrimCookingPot> It(World); It; ++It)
	{
		if (It->IsWithinReach(Pawn))
		{
			const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), It->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
	}
	return Best;
}

FVector AGrimCookingPot::GetOutputLocation() const
{
	return GetActorTransform().TransformPosition(OutputOffset);
}

// ───── 서버: 넣기 · 꺼내기 ─────

void AGrimCookingPot::MergeContent(const FGrimItemInstance& Item)
{
	if (Item.IsEmpty())
	{
		return;
	}
	for (FGrimItemInstance& Existing : Contents)
	{
		if (Existing.CanStackWith(Item))
		{
			Existing.Count += Item.Count;
			return;
		}
	}
	Contents.Add(Item);
}

void AGrimCookingPot::AddContent(const FGrimItemInstance& Item)
{
	if (!HasAuthority())
	{
		return;
	}
	MergeContent(Item);
	RefreshVisuals();
	UpdateTimer();
	ForceNetUpdate();
}

int32 AGrimCookingPot::AddIngredientsFrom(UGrimInventoryComponent* Inventory)
{
	if (!HasAuthority() || !Inventory)
	{
		return 0;
	}
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	const TArray<FGrimItemInstance>& Bag = Inventory->GetSlots();

	for (const FGrimRecipeDefinition& Recipe : Database.Recipes)
	{
		if (Recipe.Row.Station != StationId)
		{
			continue;
		}
		const FGrimRecipeProgress Current = Database.EvaluateRecipe(Recipe, Contents);
		if (Current.bHasExtra)
		{
			continue; // 다른 레시피의 것이 들어 있다
		}

		// 모자란 만큼 가방 칸에서 고른다(ID 재료 먼저, 묶음 재료 나중 — EvaluateRecipe 와 같은 순서).
		TArray<int32> Available;
		for (const FGrimItemInstance& Slot : Bag)
		{
			Available.Add(Slot.IsEmpty() ? 0 : Slot.Count);
		}
		TArray<TPair<int32, int32>> Moves; // (칸, 개수)
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			for (int32 InputIndex = 0; InputIndex < Recipe.Inputs.Num(); ++InputIndex)
			{
				const FGrimRecipeInput& Input = Recipe.Inputs[InputIndex];
				if (Input.bGroup != (Pass == 1))
				{
					continue;
				}
				int32 Missing = Input.Count - Current.Filled[InputIndex];
				for (int32 SlotIndex = 0; SlotIndex < Bag.Num() && Missing > 0; ++SlotIndex)
				{
					if (Available[SlotIndex] > 0 && Database.MatchesInput(Bag[SlotIndex].ItemId, Input))
					{
						const int32 Take = FMath::Min(Available[SlotIndex], Missing);
						Available[SlotIndex] -= Take;
						Missing -= Take;
						Moves.Emplace(SlotIndex, Take);
					}
				}
			}
		}
		if (Moves.Num() == 0)
		{
			continue;
		}

		int32 Moved = 0;
		for (const TPair<int32, int32>& Move : Moves)
		{
			const FGrimItemInstance Taken = Inventory->RemoveFromSlot(Move.Key, Move.Value);
			MergeContent(Taken);
			Moved += Taken.Count;
			UE_LOG(LogGrimItems, Display, TEXT("[Server] 냄비 %s 에 %s × %d (레시피 %s)"), *GetName(), *Taken.ItemId.ToString(), Taken.Count, *Recipe.Id.ToString());
		}
		if (APlayerState* PlayerState = Cast<APlayerState>(Inventory->GetOwner()))
		{
			Contributors.AddUnique(PlayerState);
		}
		RefreshVisuals();
		UpdateTimer();
		ForceNetUpdate();
		return Moved;
	}
	return 0;
}

int32 AGrimCookingPot::TakeBackInto(UGrimInventoryComponent* Inventory)
{
	if (!HasAuthority() || !Inventory || Contents.Num() == 0)
	{
		return 0;
	}
	if (Progress > 0.f && GetMatchingRecipe())
	{
		return 0; // 끓는 중
	}
	int32 Taken = 0;
	for (int32 Index = Contents.Num() - 1; Index >= 0; --Index)
	{
		FGrimItemInstance& Item = Contents[Index];
		const int32 Added = Inventory->AddItem(Item, false);
		Item.Count -= Added;
		Taken += Added;
		if (Item.Count <= 0)
		{
			Contents.RemoveAt(Index);
		}
	}
	Progress = 0.f;
	RefreshVisuals();
	UpdateTimer();
	ForceNetUpdate();
	return Taken;
}

// ───── 서버: 끓이기 ─────

void AGrimCookingPot::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority() || !HasActorBegunPlay())
	{
		return;
	}
	if (Contents.Num() > 0)
	{
		if (!World->GetTimerManager().IsTimerActive(CookTimer))
		{
			const float Interval = UGrimItemSettings::Get()->CookCheckInterval;
			World->GetTimerManager().SetTimer(CookTimer, FTimerDelegate::CreateWeakLambda(this, [this, Interval]()
			{
				UpdateCooking(Interval);
			}), Interval, true);
		}
	}
	else
	{
		World->GetTimerManager().ClearTimer(CookTimer);
		if (bHeated)
		{
			bHeated = false;
		}
	}
}

void AGrimCookingPot::UpdateCooking(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	const float Heat = AGrimItemPickup::FindHeatIntensityNear(GetWorld(), GetActorLocation(), Settings->PotHeatRadius, this);
	const bool bNowHeated = Heat > 0.f && Contents.Num() > 0;
	if (bNowHeated != bHeated)
	{
		bHeated = bNowHeated;
		RefreshVisuals();
		ForceNetUpdate();
	}

	const FGrimRecipeDefinition* Recipe = GetMatchingRecipe();
	if (!Recipe)
	{
		Progress = 0.f;
		return;
	}
	if (!bNowHeated)
	{
		return; // 불이 꺼지면 멈춘다(진행은 남는다)
	}
	if (FGrimCookingModel::Advance(Progress, Heat, DeltaSeconds, Recipe->Row.CookSeconds, Settings->ExtraCookRatePerIntensity))
	{
		Complete(*Recipe);
	}
}

void AGrimCookingPot::Complete(const FGrimRecipeDefinition& Recipe)
{
	Contents.Reset();
	Progress = 0.f;
	bHeated = false;
	AGrimItemPickup* Output = UGrimItemStatics::SpawnPickup(this, Recipe.Row.Output, Recipe.Row.OutputCount, GetOutputLocation());
	UE_LOG(LogGrimItems, Display, TEXT("[Server] 냄비 %s: %s 완성 → %s × %d (%s)"), *GetName(), *Recipe.Id.ToString(), *Recipe.Row.Output.ToString(),
		Recipe.Row.OutputCount, *GetNameSafe(Output));

	for (const TWeakObjectPtr<APlayerState>& Weak : Contributors)
	{
		if (const UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(Weak.Get()))
		{
			Inventory->NotifyQuest(GrimItemEvents::CookingCompleted);
		}
	}
	Contributors.Reset();
	RefreshVisuals();
	UpdateTimer();
	ForceNetUpdate();
}

// ───── 연출 ─────

void AGrimCookingPot::OnRep_Contents()
{
	RefreshVisuals();
}

void AGrimCookingPot::RefreshVisuals()
{
	BrothMesh->SetVisibility(Contents.Num() > 0);
	if (BrothMaterial)
	{
		BrothMaterial->SetVectorParameterValue(TEXT("Color"), bHeated ? GrimCookingPotLocal::BoilingColor : GrimCookingPotLocal::BrothColor);
	}
}
