#include "Items/GrimCookingPot.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemTags.h"
#include "Core/GrimPlayerState.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// 아이템 콘솔 명령(개발 빌드). 콘솔은 로컬에서 돌므로 친 플레이어의 가방 서버 RPC 로 서버에서 적용한다.
// 이름은 Grim.Items.*(콘솔은 대소문자를 가리지 않는다). 목록은 docs/systems/building 「콘솔」.

#if !UE_BUILD_SHIPPING

namespace GrimItemConsolePrivate
{
	UGrimInventoryComponent* FindLocalInventory(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController->IsLocalController())
			{
				return UGrimInventoryComponent::FindForActor(PlayerController);
			}
		}
		return nullptr;
	}

	/** 아이템 ID 를 첫 인자로 받는 명령 */
	bool TakesItemId(const FString& Command)
	{
		return Command == TEXT("Give") || Command == TEXT("Spawn") || Command == TEXT("Eat") || Command == TEXT("DropId");
	}

	void Run(UWorld* World, const FString& Command, const TArray<FString>& Args)
	{
		UGrimInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory)
		{
			UE_LOG(LogGrimItems, Warning, TEXT("Grim.Items.%s: 로컬 플레이어의 가방이 아직 없다"), *Command);
			return;
		}
		const int32 First = TakesItemId(Command) ? 1 : 0;
		const FString Arg = TakesItemId(Command) && Args.IsValidIndex(0) ? Args[0] : FString();
		const int32 A = Args.IsValidIndex(First) ? FCString::Atoi(*Args[First]) : 0;
		const int32 B = Args.IsValidIndex(First + 1) ? FCString::Atoi(*Args[First + 1]) : 0;
		Inventory->ServerDebugCommand(Command, Arg, A, B);
	}

	FConsoleCommandWithWorldAndArgsDelegate Make(const TCHAR* Command)
	{
		const FString CommandName(Command);
		return FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([CommandName](const TArray<FString>& Args, UWorld* World)
		{
			Run(World, CommandName, Args);
		});
	}

	/** 이 프로세스의 게임 월드에서 본 가방 · 픽업 · 냄비를 한 줄씩(헤드리스 확인용). 남의 가방은 복제되지 않는다(소유자에게만). */
	void Dump(UWorld* World)
	{
		const TCHAR* Side = World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
				const UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(PlayerState);
				const AGrimPlayerState* GrimPlayerState = Cast<AGrimPlayerState>(PlayerState);
				const UAbilitySystemComponent* ASC = GrimPlayerState ? GrimPlayerState->GetAbilitySystemComponent() : nullptr;
				FString Items;
				if (Inventory)
				{
					for (int32 Index = 0; Index < Inventory->GetSlots().Num(); ++Index)
					{
						const FGrimItemInstance& Slot = Inventory->GetSlots()[Index];
						if (!Slot.IsEmpty())
						{
							Items += FString::Printf(TEXT("%d:%s×%d "), Index, *Slot.ItemId.ToString(), Slot.Count);
						}
					}
				}
				FString Tags;
				if (ASC)
				{
					FGameplayTagContainer Owned;
					ASC->GetOwnedGameplayTags(Owned);
					for (const FGameplayTag& Tag : Owned)
					{
						if (Tag.ToString().StartsWith(TEXT("Status.Sick")) || Tag == GrimItemTags::Status_Buff_Food)
						{
							Tags += Tag.ToString() + TEXT(" ");
						}
					}
				}
				UE_LOG(LogGrimItems, Display, TEXT("[ItemsDump %s] %s inv=%s slots=%d items=[%s] Hunger=%.1f ColdResist=%.1f tags=%s"), Side, *PlayerState->GetPlayerName(),
					Inventory ? TEXT("yes") : TEXT("no"), Inventory ? Inventory->GetSlotCount() : 0, *Items,
					ASC ? ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHungerAttribute()) : -1.f,
					ASC ? ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()) : -1.f, *Tags);
			}
		}
		for (TActorIterator<AGrimItemPickup> It(World); It; ++It)
		{
			UE_LOG(LogGrimItems, Display, TEXT("[ItemsDump %s] pickup %s %s×%d cook=%.1f/%.1f heated=%d cooked=%d pos=%s"), Side, *It->GetName(),
				*It->GetItem().ItemId.ToString(), It->GetItem().Count, It->GetCookProgress(), It->GetCookSeconds(), It->IsHeated() ? 1 : 0, It->WasCooked() ? 1 : 0,
				*It->GetActorLocation().ToCompactString());
		}
		for (TActorIterator<AGrimCookingPot> It(World); It; ++It)
		{
			FString Contents;
			for (const FGrimItemInstance& Item : It->GetContents())
			{
				Contents += FString::Printf(TEXT("%s×%d "), *Item.ItemId.ToString(), Item.Count);
			}
			const FGrimRecipeDefinition* Recipe = It->GetMatchingRecipe();
			UE_LOG(LogGrimItems, Display, TEXT("[ItemsDump %s] pot %s contents=[%s] recipe=%s progress=%.1f heated=%d"), Side, *It->GetName(), *Contents,
				Recipe ? *Recipe->Id.ToString() : TEXT("-"), It->GetProgress(), It->IsHeated() ? 1 : 0);
		}
	}

	/** Grim.Items.At <초> <명령> [인자…] — 이 프로세스의 게임 월드마다 몇 초 뒤에. 명령은 Dump 또는 아래 명령의 끝 이름(Give · PickUp …). */
	void AtCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			return;
		}
		const float Delay = FMath::Max(0.f, FCString::Atof(*Args[0]));
		const FString Command = Args[1];
		TArray<FString> Rest;
		for (int32 Index = 2; Index < Args.Num(); ++Index)
		{
			Rest.Add(Args[Index]);
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Command, Rest](float)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					if (Command == TEXT("Dump"))
					{
						Dump(World);
					}
					else
					{
						Run(World, Command, Rest);
					}
				}
			}
			return false;
		}), Delay);
	}
}

static FAutoConsoleCommand GGrimItemsAtCommand(
	TEXT("Grim.Items.At"),
	TEXT("몇 초 뒤 아이템 명령(헤드리스 확인용). 예: Grim.Items.At 20 Give JackalopeMeat 2 / Grim.Items.At 30 Dump"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimItemConsolePrivate::AtCommand));

static FAutoConsoleCommandWithWorld GGrimItemsDumpCommand(
	TEXT("Grim.Items.Dump"),
	TEXT("이 월드에서 본 가방 · 픽업 · 냄비를 로그로."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&GrimItemConsolePrivate::Dump));

static FAutoConsoleCommandWithWorldAndArgs GGrimItemsGiveCommand(TEXT("Grim.Items.Give"),
	TEXT("가방에 넣는다. 예: Grim.Items.Give JackalopeMeat 2"), GrimItemConsolePrivate::Make(TEXT("Give")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsSpawnCommand(TEXT("Grim.Items.Spawn"),
	TEXT("폰 앞 1.5m 바닥에 픽업을 놓는다. 예: Grim.Items.Spawn JackalopeMeat 1"), GrimItemConsolePrivate::Make(TEXT("Spawn")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsPickUpCommand(TEXT("Grim.Items.PickUp"),
	TEXT("곁의 가장 가까운 픽업을 줍는다(F 와 같음)."), GrimItemConsolePrivate::Make(TEXT("PickUp")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsUseCommand(TEXT("Grim.Items.Use"),
	TEXT("칸을 쓴다(먹기 · 읽기). 예: Grim.Items.Use 0"), GrimItemConsolePrivate::Make(TEXT("Use")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsEatCommand(TEXT("Grim.Items.Eat"),
	TEXT("그 아이템이 든 첫 칸을 쓴다. 예: Grim.Items.Eat Stew"), GrimItemConsolePrivate::Make(TEXT("Eat")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsDropCommand(TEXT("Grim.Items.Drop"),
	TEXT("칸에서 버린다. 예: Grim.Items.Drop 0 1"), GrimItemConsolePrivate::Make(TEXT("Drop")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsDropIdCommand(TEXT("Grim.Items.DropId"),
	TEXT("그 아이템이 든 첫 칸에서 버린다. 예: Grim.Items.DropId JackalopeMeat 1"), GrimItemConsolePrivate::Make(TEXT("DropId")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsMoveCommand(TEXT("Grim.Items.Move"),
	TEXT("칸을 옮긴다(합치기 · 바꾸기). 예: Grim.Items.Move 0 5"), GrimItemConsolePrivate::Make(TEXT("Move")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsFireCommand(TEXT("Grim.Items.Fire"),
	TEXT("가장 가까운 픽업(6m)에 불 자극(불 마법을 직접 쏜 셈). 예: Grim.Items.Fire 1"), GrimItemConsolePrivate::Make(TEXT("Fire")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsSpawnFireCommand(TEXT("Grim.Items.SpawnFire"),
	TEXT("폰 앞 2m 에 불붙은 모닥불을 놓는다."), GrimItemConsolePrivate::Make(TEXT("SpawnFire")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsSpawnPotCommand(TEXT("Grim.Items.SpawnPot"),
	TEXT("가장 가까운 모닥불(8m) 위에 냄비를 놓는다(없으면 폰 앞)."), GrimItemConsolePrivate::Make(TEXT("SpawnPot")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsPotCommand(TEXT("Grim.Items.Pot"),
	TEXT("곁의 냄비에 재료를 넣는다(F 와 같음)."), GrimItemConsolePrivate::Make(TEXT("Pot")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsHungerCommand(TEXT("Grim.Items.Hunger"),
	TEXT("배고픔을 값으로(먹기 시험). 예: Grim.Items.Hunger 20"), GrimItemConsolePrivate::Make(TEXT("Hunger")));
static FAutoConsoleCommandWithWorldAndArgs GGrimItemsClearCommand(TEXT("Grim.Items.Clear"),
	TEXT("가방을 비운다."), GrimItemConsolePrivate::Make(TEXT("Clear")));

#endif
