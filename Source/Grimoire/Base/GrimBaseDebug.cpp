#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseTags.h"
#include "Base/GrimBuildPiece.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimItemContainerComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimInventoryWidget.h"
#include "Survival/GrimSurvivalComponent.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// 거점 콘솔 명령(개발 빌드). 콘솔은 로컬에서 돌므로 친 플레이어의 건축 컴포넌트 서버 RPC 로 서버에서 적용한다.
// 이름은 Grim.Base.*. 목록은 docs/systems/m4-plan 「콘솔」 · docs/systems/building 「콘솔」.

#if !UE_BUILD_SHIPPING

namespace GrimBaseConsoleLocal
{
	UGrimBuilderComponent* FindLocalBuilder(UWorld* World)
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
				return UGrimBuilderComponent::FindForActor(PlayerController);
			}
		}
		return nullptr;
	}

	/** 첫 인자가 이름(조각 · 아이템 · 액터 · 레시피)인 명령 */
	bool TakesName(const FString& Command)
	{
		static const TCHAR* Names[] = { TEXT("Place"), TEXT("PlaceAt"), TEXT("Demolish"), TEXT("Door"), TEXT("Repair"), TEXT("Damage"), TEXT("SetHealth"),
			TEXT("RepairNearest"), TEXT("Store"), TEXT("Take"), TEXT("Craft"), TEXT("Equip") };
		for (const TCHAR* Name : Names)
		{
			if (Command == Name)
			{
				return true;
			}
		}
		return false;
	}

	void Run(UWorld* World, const FString& Command, const TArray<FString>& Args)
	{
		UGrimBuilderComponent* Builder = FindLocalBuilder(World);
		if (!Builder)
		{
			UE_LOG(LogGrimBase, Warning, TEXT("Grim.Base.%s: 로컬 플레이어의 건축 컴포넌트가 아직 없다"), *Command);
			return;
		}
		const int32 First = TakesName(Command) ? 1 : 0;
		const FString Name = First > 0 && Args.IsValidIndex(0) ? Args[0] : FString();
		TArray<int32> Numbers;
		for (int32 Index = First; Index < Args.Num(); ++Index)
		{
			Numbers.Add(FCString::Atoi(*Args[Index]));
		}
		Builder->ServerDebugCommand(Command, Name, Numbers);
	}

	FConsoleCommandWithWorldAndArgsDelegate Make(const TCHAR* Command)
	{
		const FString CommandName(Command);
		return FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([CommandName](const TArray<FString>& Args, UWorld* World)
		{
			Run(World, CommandName, Args);
		});
	}

	/** 이 프로세스의 게임 월드에서 본 거점 · 조각 · 시설 · 플레이어를 한 줄씩(헤드리스 확인용) */
	void Dump(UWorld* World)
	{
		const TCHAR* Side = World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
		const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(World);
		const UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>();
		const UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>();
		if (!Subsystem)
		{
			return;
		}
		UE_LOG(LogGrimBase, Display, TEXT("[BaseDump %s] time=%.3f day=%d night=%d cores=%d pieces=%d structures=%d"), Side,
			TimeOfDay ? TimeOfDay->GetTimeOfDay() : -1.f, TimeOfDay ? TimeOfDay->GetDayIndex() : -1, TimeOfDay && TimeOfDay->IsNight() ? 1 : 0,
			Subsystem->GetCores().Num(), Subsystem->GetPieces().Num(), Subsystem->GetStructures().Num());
		for (const AGrimBaseCore* Core : Subsystem->GetCores())
		{
			UE_LOG(LogGrimBase, Display, TEXT("[BaseDump %s] core %s owner=%s(%s) pos=%s yaw=%.0f"), Side, *Core->GetName(), *Core->GetOwnerName(), *Core->GetOwnerId(),
				*Core->GetActorLocation().ToCompactString(), Core->GetActorRotation().Yaw);
		}
		for (const UGrimStructureComponent* Structure : Subsystem->GetStructures())
		{
			const AActor* Owner = Structure->GetOwner();
			FString Extra;
			if (const AGrimBuildPiece* Piece = Cast<AGrimBuildPiece>(Owner))
			{
				Extra = Piece->GetSlot().ToString() + (Piece->IsDoor() ? FString::Printf(TEXT(" door=%d"), Piece->IsDoorOpen() ? 1 : 0) : FString());
			}
			else if (const AGrimStorageChest* Chest = Cast<AGrimStorageChest>(Owner))
			{
				Extra = TEXT("chest=[");
				for (const FGrimItemInstance& Item : Chest->GetContainer()->GetSlots())
				{
					if (!Item.IsEmpty())
					{
						Extra += FString::Printf(TEXT("%s×%d "), *Item.ItemId.ToString(), Item.Count);
					}
				}
				Extra += TEXT("]");
			}
			else if (const AGrimFarmPlot* Plot = Cast<AGrimFarmPlot>(Owner))
			{
				Extra = FString::Printf(TEXT("crop=%s growth=%.2f moisture=%.0f ash=%d wet=%.1f"), *Plot->GetCropId().ToString(), Plot->GetGrowth(), Plot->GetMoisture(),
					Plot->HasAsh() ? 1 : 0, UGrimElementalStatics::GetStateIntensity(Plot, GrimTags::State_Wet));
			}
			const float Burning = UGrimElementalStatics::GetStateIntensity(Owner, GrimTags::State_Burning);
			UE_LOG(LogGrimBase, Display, TEXT("[BaseDump %s] built %s piece=%s hp=%.1f/%.1f inBase=%d sheltered=%d burning=%.1f %s"), Side, *GetNameSafe(Owner),
				*Structure->GetPieceId().ToString(), Structure->GetHealth(), Structure->GetMaxHealth(), Structure->FindBaseCore() ? 1 : 0,
				Weather && Owner && Weather->IsActorSheltered(Owner) ? 1 : 0, Burning, *Extra);
		}
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
				const AGrimPlayerState* GrimPlayerState = Cast<AGrimPlayerState>(PlayerState);
				const APawn* Pawn = PlayerState->GetPawn();
				const UAbilitySystemComponent* ASC = GrimPlayerState ? GrimPlayerState->GetAbilitySystemComponent() : nullptr;
				const UGrimSurvivalComponent* Survival = GrimPlayerState ? GrimPlayerState->GetSurvival() : nullptr;
				const UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(PlayerState);
				FString Gear;
				if (Equipment)
				{
					for (int32 Index = 0; Index < Equipment->GetSlots().Num(); ++Index)
					{
						const FGrimItemInstance& Item = Equipment->GetSlots()[Index];
						if (!Item.IsEmpty())
						{
							Gear += FString::Printf(TEXT("%d:%s(%.0f/%.0f r%d) "), Index, *Item.ItemId.ToString(), UGrimEquipmentComponent::GetDurability(Item),
								UGrimEquipmentComponent::GetMaxDurability(Item), Item.RepairCount);
						}
					}
				}
				const bool bBuildMode = ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.BuildMode"), false));
				UE_LOG(LogGrimBase, Display, TEXT("[BaseDump %s] player %s pos=%s inBase=%d sheltered=%d wet=%.1f sleeping=%d weather=%d buildMode=%d wandBroken=%d wandMissing=%d ColdResist=%.1f gear=[%s]"), Side,
					*PlayerState->GetPlayerName(), Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"),
					Pawn && Subsystem->FindBaseAt(Pawn->GetActorLocation()) ? 1 : 0,
					Pawn && Weather && Weather->IsActorSheltered(Pawn) ? 1 : 0,
					Pawn ? UGrimElementalStatics::GetStateIntensity(Pawn, GrimTags::State_Wet) : 0.f,
					Survival && Survival->IsSleeping() ? 1 : 0,
					Pawn && Weather ? static_cast<int32>(Weather->GetWeatherAt(Pawn->GetActorLocation())) : -1,
					bBuildMode ? 1 : 0,
					ASC && ASC->HasMatchingGameplayTag(GrimBaseTags::Status_WandBroken) ? 1 : 0,
					ASC && ASC->HasMatchingGameplayTag(GrimBaseTags::Status_WandMissing) ? 1 : 0,
					ASC ? ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()) : -1.f, *Gear);
			}
		}
	}

	/** Grim.Base.At <초> <명령> [인자…] — 이 프로세스의 게임 월드마다 몇 초 뒤에. 명령은 Dump 또는 아래 명령의 끝 이름 */
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

	/**
	 * Grim.Base.AfterJoin <사람 수> <초> <명령 …> — 이 프로세스의 게임 월드에 접속자가 <사람 수> 이상이 된 뒤 <초> 뒤에 콘솔 명령을 실행한다.
	 * 헤드리스 2인 확인에서 서버 · 클라이언트의 시계를 「둘이 다 들어온 때」로 맞추려고 쓴다(기계가 바쁘면 부팅이 1분 넘게 걸린다).
	 */
	void AfterJoinCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogGrimBase, Warning, TEXT("사용: Grim.Base.AfterJoin <사람 수> <초> <명령 …>"));
			return;
		}
		const int32 Players = FMath::Max(1, FCString::Atoi(*Args[0]));
		const double Delay = FMath::Max(0.0, FCString::Atod(*Args[1]));
		const FString Command = FString::Join(TArrayView<const FString>(Args).RightChop(2), TEXT(" "));
		double ArmedAt = -1.0;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Players, Delay, Command, ArmedAt](float) mutable
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				const AGameStateBase* GameState = World && World->IsGameWorld() ? World->GetGameState() : nullptr;
				if (!GameState)
				{
					continue;
				}
				const double Now = FPlatformTime::Seconds();
				if (ArmedAt < 0.0)
				{
					if (GameState->PlayerArray.Num() >= Players)
					{
						ArmedAt = Now;
					}
					return true;
				}
				if (Now - ArmedAt < Delay)
				{
					return true;
				}
				UE_LOG(LogGrimBase, Display, TEXT("[Grim.Base.AfterJoin] %s"), *Command);
				if (APlayerController* PlayerController = World->GetFirstPlayerController())
				{
					PlayerController->ConsoleCommand(Command);
				}
				else
				{
					GEngine->Exec(World, *Command);
				}
				return false;
			}
			return true;
		}), 0.1f);
	}
}

namespace GrimBaseConsoleLocal
{
	/** 로컬: 소지품 화면을 그 탭으로 연다. 시설 탭(Storage · Workbench · Base)이면 가장 가까운 그 시설을 F 로 연 셈. Close 면 닫는다(화면 찍기 · 사람 확인용) */
	void Bag(const TArray<FString>& Args, UWorld* World)
	{
		UGrimBuilderComponent* Builder = FindLocalBuilder(World);
		UGrimInventoryComponent* Inventory = Builder ? UGrimInventoryComponent::FindForActor(Builder->GetOwner()) : nullptr;
		UGrimInventoryWidget* Widget = Inventory ? Inventory->GetWidget() : nullptr;
		if (!Widget)
		{
			UE_LOG(LogGrimBase, Warning, TEXT("Grim.Base.Bag: 로컬 플레이어의 소지품 화면이 아직 없다"));
			return;
		}
		const FName TabId = Args.IsValidIndex(0) ? FName(*Args[0]) : NAME_None;
		if (TabId == TEXT("Close"))
		{
			Widget->SetBagOpen(false);
			return;
		}
		const APawn* Pawn = Builder->GetPawn();
		AActor* Nearest = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();
		for (TActorIterator<AActor> It(World); It && Pawn && !TabId.IsNone(); ++It)
		{
			const IGrimBaseInteractable* Station = Cast<IGrimBaseInteractable>(*It);
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (Station && Station->GetStationTabId() == TabId && DistSq < NearestDistSq)
			{
				Nearest = *It;
				NearestDistSq = DistSq;
			}
		}
		if (Nearest)
		{
			Builder->OpenStation(Nearest);
		}
		Widget->OpenTab(TabId);
		UE_LOG(LogGrimBase, Display, TEXT("Grim.Base.Bag: 탭 %s (시설 %s) → 고른 탭 %s"), *TabId.ToString(), *GetNameSafe(Nearest), *Widget->GetActiveTab().ToString());
	}
}

static FAutoConsoleCommandWithWorldAndArgs GGrimBaseBagCommand(TEXT("Grim.Base.Bag"),
	TEXT("로컬: 소지품 화면을 그 탭으로 연다(시설 탭이면 가장 가까운 그 시설을 연 셈). <Equipment|Creatures|Storage|Workbench|Base|Close>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GrimBaseConsoleLocal::Bag));

static FAutoConsoleCommand GGrimBaseAfterJoinCommand(
	TEXT("Grim.Base.AfterJoin"),
	TEXT("접속자가 <사람 수> 이상이 된 뒤 <초> 뒤에 콘솔 명령. 예: Grim.Base.AfterJoin 2 10 Grim.Base.Dump"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimBaseConsoleLocal::AfterJoinCommand));

static FAutoConsoleCommand GGrimBaseAtCommand(
	TEXT("Grim.Base.At"),
	TEXT("몇 초 뒤 거점 명령(헤드리스 확인용). 예: Grim.Base.At 20 Core / Grim.Base.At 30 Dump"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimBaseConsoleLocal::AtCommand));

static FAutoConsoleCommandWithWorld GGrimBaseDumpCommand(
	TEXT("Grim.Base.Dump"),
	TEXT("이 월드에서 본 거점 · 지은 것 · 플레이어(지붕 · 젖음 · 잠 · 장비)를 로그로."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&GrimBaseConsoleLocal::Dump));

static FAutoConsoleCommandWithWorldAndArgs GGrimBaseCoreCommand(TEXT("Grim.Base.Core"),
	TEXT("폰 앞 [거리 cm, 기본 400] 땅에 거점 핵을 놓는다(재료 필요)."), GrimBaseConsoleLocal::Make(TEXT("Core")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBasePlaceCommand(TEXT("Grim.Base.Place"),
	TEXT("가장 가까운 거점 격자에 구조 조각. <조각> <X> <Y> <층> <방향>. 예: Grim.Base.Place WoodWall 1 0 0 0"), GrimBaseConsoleLocal::Make(TEXT("Place")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBasePlaceAtCommand(TEXT("Grim.Base.PlaceAt"),
	TEXT("시설을 그 칸 바닥에. <시설> <X> <Y> [로컬 X cm] [로컬 Y cm] [90° 방향]. 예: Grim.Base.PlaceAt Bed 2 0 0 -80 1"), GrimBaseConsoleLocal::Make(TEXT("PlaceAt")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseTeleportCommand(TEXT("Grim.Base.Teleport"),
	TEXT("가장 가까운 거점의 칸 바닥 위로. <X> <Y> [층]"), GrimBaseConsoleLocal::Make(TEXT("Teleport")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseDemolishCommand(TEXT("Grim.Base.Demolish"),
	TEXT("그 이름의 지은 것을 헌다(재료 절반 환급). <액터 이름>"), GrimBaseConsoleLocal::Make(TEXT("Demolish")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseDoorCommand(TEXT("Grim.Base.Door"),
	TEXT("문을 열고 닫는다. <액터 이름>"), GrimBaseConsoleLocal::Make(TEXT("Door")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseRepairCommand(TEXT("Grim.Base.Repair"),
	TEXT("「수리」를 건 셈(마나 · 가방 재료). <액터 이름>"), GrimBaseConsoleLocal::Make(TEXT("Repair")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseRepairNearestCommand(TEXT("Grim.Base.RepairNearest"),
	TEXT("가장 가까운 작업대(장비) · 거점 핵(거점 수리)에 「수리」. <Workbench|Core>"), GrimBaseConsoleLocal::Make(TEXT("RepairNearest")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseDamageCommand(TEXT("Grim.Base.Damage"),
	TEXT("지은 것에 피해. <액터 이름> <양>"), GrimBaseConsoleLocal::Make(TEXT("Damage")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseSetHealthCommand(TEXT("Grim.Base.SetHealth"),
	TEXT("지은 것의 내구도를 값으로. <액터 이름> <값>"), GrimBaseConsoleLocal::Make(TEXT("SetHealth")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseLearnRepairCommand(TEXT("Grim.Base.LearnRepair"),
	TEXT("「레스티투오」(수리 마법 DA_Spell_Repair, 2서클 — 모자라면 서클을 올림)를 배운다."), GrimBaseConsoleLocal::Make(TEXT("LearnRepair")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseStoreCommand(TEXT("Grim.Base.Store"),
	TEXT("가장 가까운 저장고에 넣는다. <아이템> [개수]"), GrimBaseConsoleLocal::Make(TEXT("Store")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseTakeCommand(TEXT("Grim.Base.Take"),
	TEXT("가장 가까운 저장고에서 꺼낸다. <아이템> [개수]"), GrimBaseConsoleLocal::Make(TEXT("Take")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseCraftCommand(TEXT("Grim.Base.Craft"),
	TEXT("가장 가까운 작업대에서 만든다. <레시피 ID>"), GrimBaseConsoleLocal::Make(TEXT("Craft")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBasePlotCommand(TEXT("Grim.Base.Plot"),
	TEXT("가장 가까운 밭: 심기(가방의 첫 씨앗) · 거두기."), GrimBaseConsoleLocal::Make(TEXT("Plot")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseGrowCommand(TEXT("Grim.Base.Grow"),
	TEXT("모든 밭을 지금 조건으로 <초> 만큼 키운다."), GrimBaseConsoleLocal::Make(TEXT("Grow")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseLearnBaseRepairCommand(TEXT("Grim.Base.LearnBaseRepair"),
	TEXT("「레스티투오 막시마」(거점 핵 전체 수리 — Q121)를 배운다(서클이 모자라면 올림)."), GrimBaseConsoleLocal::Make(TEXT("LearnBaseRepair")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseEquipCommand(TEXT("Grim.Base.Equip"),
	TEXT("가방의 장비를 입는다. <아이템>"), GrimBaseConsoleLocal::Make(TEXT("Equip")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseUnequipCommand(TEXT("Grim.Base.Unequip"),
	TEXT("칸의 장비를 벗는다. <칸 0~7>"), GrimBaseConsoleLocal::Make(TEXT("Unequip")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseWearCommand(TEXT("Grim.Base.Wear"),
	TEXT("칸의 장비를 닳게 한다. <칸 0~7> <양>"), GrimBaseConsoleLocal::Make(TEXT("Wear")));
static FAutoConsoleCommandWithWorldAndArgs GGrimBaseCollapseCommand(TEXT("Grim.Base.Collapse"),
	TEXT("받침 없는 조각을 지금 무너뜨린다."), GrimBaseConsoleLocal::Make(TEXT("Collapse")));

#endif
