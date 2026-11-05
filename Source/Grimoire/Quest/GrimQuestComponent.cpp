#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestSubsystem.h"
#include "Quest/GrimQuestTrackerWidget.h"
#include "Factions/GrimFactionTypes.h"
#include "Factions/GrimShopSettings.h"
#include "Factions/GrimTowerNPC.h"
#include "Items/GrimInventoryComponent.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimProgressionComponent.h"
#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionTags.h"
#include "Progression/GrimSkillTreeComponent.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Survival/GrimSurvivalComponent.h"
#include "UI/GrimUIData.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FName UGrimQuestComponent::ComponentName(TEXT("GrimQuest"));

namespace GrimQuestComponentLocal
{
	/** 서버에서 세계를 보는 조회형 목표 */
	class FWorldFacts final : public IGrimQuestFacts
	{
	public:
		FWorldFacts(const UWorld* InWorld, const AGrimPlayerState* InPlayerState, const APawn* InPawn)
			: World(InWorld), PlayerState(InPlayerState), Pawn(InPawn)
		{
		}

		virtual int32 QueryObjective(const FGrimQuestObjectiveDef& Objective) const override
		{
			switch (Objective.Type)
			{
			case EGrimQuestObjectiveType::ElementalState: return CountElementalState(Objective);
			case EGrimQuestObjectiveType::KnowSpell: return FindKnownSpell(Objective.Target) ? 1 : 0;
			case EGrimQuestObjectiveType::ProficiencyGrade: return QueryProficiency(Objective);
			case EGrimQuestObjectiveType::ReachCircle:
			{
				const UGrimProgressionComponent* Progression = PlayerState ? PlayerState->GetProgression() : nullptr;
				return Progression && Progression->GetCircle() >= FMath::RoundToInt(Objective.Value) ? 1 : 0;
			}
			case EGrimQuestObjectiveType::ReachLocation: return QueryLocation(Objective);
			case EGrimQuestObjectiveType::Attribute: return QueryAttribute(Objective);
			default: return 0;
			}
		}

	private:
		int32 CountElementalState(const FGrimQuestObjectiveDef& Objective) const
		{
			const FGameplayTag State = UGameplayTagsManager::Get().RequestGameplayTag(Objective.State, false);
			if (!World || !State.IsValid())
			{
				return 0;
			}
			int32 Count = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(Objective.Target) || It->IsActorBeingDestroyed())
				{
					continue;
				}
				const UGrimElementalStateComponent* Elemental = It->FindComponentByClass<UGrimElementalStateComponent>();
				if (Elemental && FGrimQuestModel::Compare(Objective.Compare, Elemental->GetStateIntensity(State), Objective.Value))
				{
					++Count;
				}
			}
			return Count;
		}

		UGrimSpellDefinition* FindKnownSpell(FName SpellId) const
		{
			const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
			if (!Spellbook)
			{
				return nullptr;
			}
			for (UGrimSpellDefinition* Spell : Spellbook->GetKnownSpells())
			{
				if (UGrimQuestSettings::SpellMatchesId(Spell, SpellId))
				{
					return Spell;
				}
			}
			return nullptr;
		}

		int32 QueryProficiency(const FGrimQuestObjectiveDef& Objective) const
		{
			const UGrimSpellDefinition* Spell = FindKnownSpell(Objective.Target);
			const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
			if (!Spell || !Spellbook)
			{
				return 0;
			}
			const FGrimProficiencyView View = Spellbook->GetProficiency(Spell);
			return View.bTracked && View.GradeIndex >= FMath::RoundToInt(Objective.Value) ? 1 : 0;
		}

		int32 QueryLocation(const FGrimQuestObjectiveDef& Objective) const
		{
			if (!World || !Pawn)
			{
				return 0;
			}
			const float Radius = Objective.Value > 0.f ? Objective.Value : UGrimQuestSettings::Get()->DefaultLocationRadius;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(Objective.Target) && FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation()) <= FMath::Square(Radius))
				{
					return 1;
				}
			}
			return 0;
		}

		int32 QueryAttribute(const FGrimQuestObjectiveDef& Objective) const
		{
			const UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
			FGameplayAttribute Attribute;
			if (!ASC || !GrimUIData::FindAttribute(ASC, Objective.Target.ToString(), Attribute))
			{
				return 0;
			}
			return FGrimQuestModel::Compare(Objective.Compare, ASC->GetNumericAttribute(Attribute), Objective.Value) ? 1 : 0;
		}

		const UWorld* World;
		const AGrimPlayerState* PlayerState;
		const APawn* Pawn;
	};

	const TCHAR* StartResultName(EGrimQuestStartResult Result)
	{
		switch (Result)
		{
		case EGrimQuestStartResult::Started: return TEXT("시작");
		case EGrimQuestStartResult::UnknownQuest: return TEXT("모르는 퀘스트");
		case EGrimQuestStartResult::AlreadyActive: return TEXT("이미 진행 중");
		case EGrimQuestStartResult::AlreadyCompleted: return TEXT("이미 끝냄");
		default: return TEXT("시작 조건 모자람");
		}
	}
}

UGrimQuestComponent::UGrimQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimQuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimQuestComponent, Log, COND_OwnerOnly);
}

UGrimQuestComponent* UGrimQuestComponent::FindForPlayerState(const APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UGrimQuestComponent>() : nullptr;
}

UGrimQuestComponent* UGrimQuestComponent::FindForController(const AController* Controller)
{
	return Controller ? FindForPlayerState(Controller->PlayerState) : nullptr;
}

UGrimQuestComponent* UGrimQuestComponent::FindForActor(const AActor* Actor)
{
	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		return FindForPlayerState(PlayerState);
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return FindForPlayerState(Pawn->GetPlayerState());
	}
	if (const AController* Controller = Cast<AController>(Actor))
	{
		return FindForController(Controller);
	}
	return nullptr;
}

float UGrimQuestComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : World->GetTimeSeconds();
}

APawn* UGrimQuestComponent::GetPawn() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

void UGrimQuestComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer && UGrimQuestSettings::Get()->bShowTracker)
	{
		// 클라이언트에서는 컨트롤러의 PlayerState 가 늦게 잇기도 한다 — 만들 수 있을 때까지 가끔 본다.
		if (!EnsureTracker())
		{
			GetWorld()->GetTimerManager().SetTimer(TrackerTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (EnsureTracker())
				{
					GetWorld()->GetTimerManager().ClearTimer(TrackerTimer);
				}
			}), 0.5f, true);
		}
	}
}

void UGrimQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TrackerTimer);
	}
	if (Tracker)
	{
		Tracker->RemoveFromParent();
		Tracker = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* UGrimQuestComponent::GetLocalOwningController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController() && PlayerController->PlayerState == GetOwner())
		{
			return PlayerController;
		}
	}
	return nullptr;
}

bool UGrimQuestComponent::EnsureTracker()
{
	if (Tracker)
	{
		return true;
	}
	APlayerController* PlayerController = GetLocalOwningController();
	if (!PlayerController)
	{
		return false;
	}
	Tracker = CreateWidget<UGrimQuestTrackerWidget>(PlayerController, UGrimQuestTrackerWidget::StaticClass());
	if (!Tracker)
	{
		return false;
	}
	Tracker->SetQuestComponent(this);
	Tracker->AddToViewport(5);
	return true;
}

void UGrimQuestComponent::OnRep_Log()
{
	OnLogChanged.Broadcast(this);
}

void UGrimQuestComponent::MarkLogChanged()
{
	// 서버(리슨 호스트 화면)는 복제 콜백이 없으니 직접 알린다.
	OnLogChanged.Broadcast(this);
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

// ───── 서버: 진행 ─────

EGrimQuestStartResult UGrimQuestComponent::StartQuest(FName QuestId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return EGrimQuestStartResult::UnknownQuest;
	}
	TArray<FGrimQuestChange> Changes;
	const EGrimQuestStartResult Result = FGrimQuestModel::StartQuest(UGrimQuestSettings::GetQuestDatabase(), Log, QuestId, GetServerTime(), Changes);
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 %s 시작 요청: %s"), *GetNameSafe(GetOwner()), *QuestId.ToString(), GrimQuestComponentLocal::StartResultName(Result));
	if (Result == EGrimQuestStartResult::Started)
	{
		HandleChanges(Changes);
		ServerPoll();
	}
	return Result;
}

void UGrimQuestComponent::ReceiveSignal(const FGrimQuestSignal& Signal)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (FGrimQuestModel::ApplySignal(UGrimQuestSettings::GetQuestDatabase(), Log, Signal))
	{
		UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 신호 %s %s +%d"), *GetNameSafe(GetOwner()),
			*StaticEnum<EGrimQuestObjectiveType>()->GetNameStringByValue(static_cast<int64>(Signal.Type)), *Signal.Target.ToString(), Signal.Amount);
		MarkLogChanged();
		ServerPoll();
	}
}

void UGrimQuestComponent::NotifyQuestEvent(AActor* PlayerActor, FName EventId, int32 Amount)
{
	if (UGrimQuestComponent* Quest = FindForActor(PlayerActor))
	{
		Quest->NotifyEvent(EventId, Amount);
	}
}

void UGrimQuestComponent::ServerPoll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	const GrimQuestComponentLocal::FWorldFacts Facts(GetWorld(), PlayerState, GetPawn());
	TArray<FGrimQuestChange> Changes;
	if (FGrimQuestModel::Advance(UGrimQuestSettings::GetQuestDatabase(), Log, &Facts, GetServerTime(), Changes))
	{
		HandleChanges(Changes);
		MarkLogChanged();
	}
}

void UGrimQuestComponent::FailQuest(FName QuestId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	TArray<FGrimQuestChange> Changes;
	if (FGrimQuestModel::FailQuest(UGrimQuestSettings::GetQuestDatabase(), Log, QuestId, Changes))
	{
		HandleChanges(Changes);
		MarkLogChanged();
	}
}

void UGrimQuestComponent::TryAutoStart()
{
	if (bAutoStartTried || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	bAutoStartTried = true;
	const UGrimQuestSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGrimQuestSubsystem>() : nullptr;
	if (!Subsystem || !Subsystem->IsAutoStartMap())
	{
		return;
	}
	for (const FGrimQuestDef& Def : UGrimQuestSettings::GetQuestDatabase().Quests)
	{
		if (Def.bAutoStart && FGrimQuestModel::CanStart(UGrimQuestSettings::GetQuestDatabase(), Log, Def.Id) == EGrimQuestStartResult::Started)
		{
			StartQuest(Def.Id);
		}
	}
}

void UGrimQuestComponent::DebugCompleteStep(FName QuestId)
{
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();
	for (FGrimQuestProgress& Progress : Log.Active)
	{
		if (!QuestId.IsNone() && Progress.QuestId != QuestId)
		{
			continue;
		}
		const FGrimQuestDef* Def = Database.Find(Progress.QuestId);
		if (Def && Def->Steps.IsValidIndex(Progress.Step))
		{
			const TArray<FGrimQuestObjectiveDef>& Objectives = Def->Steps[Progress.Step].Objectives;
			Progress.Counts.SetNum(Objectives.Num());
			for (int32 Index = 0; Index < Objectives.Num(); ++Index)
			{
				Progress.Counts[Index] = FMath::Max(1, Objectives[Index].Count);
			}
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] 개발: %s 단계 %d 를 채운 것으로"), *Progress.QuestId.ToString(), Progress.Step);
		}
		break;
	}
	ServerPoll();
}

void UGrimQuestComponent::NotifyNightSkipBegin()
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	const UGrimSurvivalComponent* Survival = PlayerState ? PlayerState->GetSurvival() : nullptr;
	bNightCreditPending = Survival && Survival->IsSleeping();
}

void UGrimQuestComponent::NotifyNightSkipped()
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	const UGrimSurvivalComponent* Survival = PlayerState ? PlayerState->GetSurvival() : nullptr;
	const bool bCredit = bNightCreditPending || (Survival && Survival->IsSleeping());
	bNightCreditPending = false;
	if (bCredit)
	{
		ReceiveSignal({ EGrimQuestObjectiveType::SleepThroughNight, NAME_None, 1 });
	}
}

void UGrimQuestComponent::HandleChanges(const TArray<FGrimQuestChange>& Changes)
{
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();
	UGrimQuestSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGrimQuestSubsystem>() : nullptr;
	for (const FGrimQuestChange& Change : Changes)
	{
		const FGrimQuestDef* Def = Database.Find(Change.QuestId);
		if (!Def)
		{
			continue;
		}
		const FGrimQuestStepDef* Step = Def->Steps.IsValidIndex(Change.Step) ? &Def->Steps[Change.Step] : nullptr;
		switch (Change.Type)
		{
		case EGrimQuestChangeType::QuestStarted:
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 시작: %s 「%s」"), *GetNameSafe(GetOwner()), *Def->Id.ToString(), *Def->Title);
			SendNotice(EGrimQuestNotice::QuestStarted, Change.QuestId, Change.Step);
			break;
		case EGrimQuestChangeType::StepStarted:
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 단계 시작: %s %d 「%s」"), *GetNameSafe(GetOwner()), *Def->Id.ToString(), Change.Step, Step ? *Step->Text : TEXT(""));
			if (Step && Subsystem)
			{
				Subsystem->ApplyStepStart(*Step, this);
			}
			if (Change.Step > 0)
			{
				SendNotice(EGrimQuestNotice::StepStarted, Change.QuestId, Change.Step);
			}
			break;
		case EGrimQuestChangeType::StepEnded:
			if (Step && Subsystem)
			{
				Subsystem->ApplyStepEnd(*Step);
			}
			break;
		case EGrimQuestChangeType::QuestCompleted:
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 완료: %s"), *GetNameSafe(GetOwner()), *Def->Id.ToString());
			GrantRewards(*Def);
			SendNotice(EGrimQuestNotice::QuestCompleted, Change.QuestId, Change.Step);
			break;
		case EGrimQuestChangeType::QuestFailed:
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 실패: %s (단계 %d)"), *GetNameSafe(GetOwner()), *Def->Id.ToString(), Change.Step);
			SendNotice(EGrimQuestNotice::QuestFailed, Change.QuestId, Change.Step);
			break;
		}
	}
}

void UGrimQuestComponent::GrantRewards(const FGrimQuestDef& Def)
{
	AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	if (!PlayerState)
	{
		return;
	}

	if (Def.IsTrial())
	{
		// 시험 통과 = 승급. 조건(RequireCircleConditions)은 시험을 시작할 때 이미 봤으므로 여기서는 강제로 올린다.
		if (UGrimProgressionComponent* Progression = PlayerState->GetProgression())
		{
			if (Progression->GetCircle() + 1 == Def.TrialCircle)
			{
				Progression->TryCircleUp(true);
			}
			else
			{
				UE_LOG(LogGrimQuest, Warning, TEXT("[Server] 시험 %s 통과했지만 서클이 %d 라 %d 로 올리지 않는다"), *Def.Id.ToString(), Progression->GetCircle(), Def.TrialCircle);
			}
		}
	}

	if (!Def.RewardSpell.IsNone())
	{
		UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
		UGrimSpellDefinition* Spell = UGrimQuestSettings::LoadSpell(Def.RewardSpell);
		if (Spellbook && Spell)
		{
			Spellbook->UseSkillbook(Spell);
		}
		else
		{
			UE_LOG(LogGrimQuest, Warning, TEXT("[Server] 보상 스킬북 %s 를 줄 수 없다(마법 정의 없음)"), *Def.RewardSpell.ToString());
		}
	}

	if (Def.RewardXP > 0)
	{
		if (UGrimLevelComponent* Level = PlayerState->GetCharacterLevel())
		{
			const float Gained = Level->AwardExperience(static_cast<float>(Def.RewardXP), GrimProgressionTags::XPSource_Quest);
			UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 경험치 %d → %.0f 받음(레벨 %d)"), *GetNameSafe(PlayerState), Def.RewardXP, Gained, Level->GetLevel());
		}
	}
}

void UGrimQuestComponent::SendNotice(EGrimQuestNotice Notice, FName QuestId, int32 Step)
{
	OnNotice.Broadcast(this, Notice, QuestId, Step);
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PlayerController = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PlayerController && !PlayerController->IsLocalController())
	{
		Client_Notice(Notice, QuestId, Step);
	}
}

void UGrimQuestComponent::Client_Notice_Implementation(EGrimQuestNotice Notice, FName QuestId, int32 Step)
{
	OnNotice.Broadcast(this, Notice, QuestId, Step);
}

// ───── NPC ─────

void UGrimQuestComponent::RequestNpc(AActor* Npc, FName Request)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Server_NpcRequest_Implementation(Npc, Request);
	}
	else
	{
		Server_NpcRequest(Npc, Request);
	}
}

void UGrimQuestComponent::Server_NpcRequest_Implementation(AActor* Npc, FName Request)
{
	AGrimTowerNPC* TowerNpc = Cast<AGrimTowerNPC>(Npc);
	if (!TowerNpc || !TowerNpc->IsWithinTalkRange(GetPawn(), UGrimQuestSettings::Get()->ServerTalkSlack))
	{
		UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s NPC 요청 %s 거절: NPC 없음 · 너무 멂"), *GetNameSafe(GetOwner()), *Request.ToString());
		if (Request != GrimDialogueContexts::Talk)
		{
			SendNpcResponse(Npc, GrimDialogueContexts::Refused);
		}
		return;
	}

	if (Request == GrimDialogueContexts::Talk)
	{
		UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 가 %s 와 대화를 마쳤다"), *GetNameSafe(GetOwner()), *TowerNpc->GetNpcId().ToString());
		ReceiveSignal({ EGrimQuestObjectiveType::TalkToNpc, TowerNpc->GetNpcId(), 1 });
		return;
	}

	const FName Context = TowerNpc->OffersService(Request) ? ExecuteService(TowerNpc, Request) : GrimDialogueContexts::Refused;
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 서비스 %s → %s"), *GetNameSafe(GetOwner()), *Request.ToString(), *Context.ToString());
	SendNpcResponse(TowerNpc, Context);
}

FName UGrimQuestComponent::ExecuteService(AActor* Npc, FName Service)
{
	AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(GetOwner());
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();

	if (Service == GrimTowerServices::Trial)
	{
		UGrimProgressionComponent* Progression = PlayerState ? PlayerState->GetProgression() : nullptr;
		if (!Progression)
		{
			return GrimDialogueContexts::Refused;
		}
		for (const FGrimQuestProgress& Progress : Log.Active)
		{
			const FGrimQuestDef* Active = Database.Find(Progress.QuestId);
			if (Active && Active->IsTrial())
			{
				return GrimDialogueContexts::TrialBusy;
			}
		}
		const FGrimQuestDef* Trial = Database.FindTrialForCircle(Progression->GetCircle() + 1);
		if (!Trial || Log.IsCompleted(Trial->Id))
		{
			return GrimDialogueContexts::TrialNone;
		}
		if (FGrimQuestModel::CanStart(Database, Log, Trial->Id) == EGrimQuestStartResult::PrerequisiteMissing)
		{
			return GrimDialogueContexts::TrialLocked;
		}
		if (Trial->bRequireCircleConditions && !Progression->EvaluateNextCircle().bMet)
		{
			return GrimDialogueContexts::TrialNotReady;
		}
		return StartQuest(Trial->Id) == EGrimQuestStartResult::Started ? GrimDialogueContexts::TrialStarted : GrimDialogueContexts::TrialNone;
	}

	FName TreeId;
	if (GrimTowerServices::ParsePurify(Service, TreeId))
	{
		// 정화 의식(Q31): 트리 하나를 비우고 포인트를 돌려준다. TODO(재화): 의식 비용이 정해지면 CostCheck 로 묻는다(지금은 무료).
		UGrimSkillTreeComponent* SkillTree = PlayerState ? PlayerState->GetSkillTree() : nullptr;
		if (!SkillTree || SkillTree->GetSpentPoints(TreeId) <= 0)
		{
			return GrimDialogueContexts::PurifyEmpty;
		}
		return SkillTree->ResetTree(TreeId) ? GrimDialogueContexts::Purify : GrimDialogueContexts::PurifyEmpty;
	}
	if (Service == GrimTowerServices::Repair)
	{
		// 인벤토리 · 내구도(M4) 전 — 대사 자리만.
		return GrimDialogueContexts::Repair;
	}
	if (Service == GrimTowerServices::Erase)
	{
		// 인벤토리 · 인챈트(M4) 전 — 대사 자리만.
		return GrimDialogueContexts::Erase;
	}
	FName ItemId;
	if (GrimTowerServices::ParseBuy(Service, ItemId))
	{
		// 상점: 솔리두스(Q38)로 산다 — 값은 UGrimShopSettings(첫 물건: 스킬북 생물 보관함, R38).
		return GrimTowerShop::TryBuy(UGrimInventoryComponent::FindForActor(GetOwner()), ItemId);
	}
	return GrimDialogueContexts::Refused;
}

void UGrimQuestComponent::SendNpcResponse(AActor* Npc, FName Context)
{
	OnNpcResponse.Broadcast(this, Npc, Context);
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* PlayerController = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PlayerController && !PlayerController->IsLocalController())
	{
		Client_NpcResponse(Npc, Context);
	}
}

void UGrimQuestComponent::Client_NpcResponse_Implementation(AActor* Npc, FName Context)
{
	OnNpcResponse.Broadcast(this, Npc, Context);
}

// ───── 세이브 ─────

void UGrimQuestComponent::WriteSaveData(FArchive& Ar)
{
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();

	int32 NumCompleted = Log.Completed.Num();
	Ar << NumCompleted;
	for (const FName QuestId : Log.Completed)
	{
		FString Id = QuestId.ToString();
		Ar << Id;
	}

	// 시험(시간 제한)은 저장하지 않는다 — 불러오면 다시 청한다.
	TArray<const FGrimQuestProgress*> Saved;
	for (const FGrimQuestProgress& Progress : Log.Active)
	{
		const FGrimQuestDef* Def = Database.Find(Progress.QuestId);
		if (Def && !Def->IsTrial())
		{
			Saved.Add(&Progress);
		}
	}
	int32 NumActive = Saved.Num();
	Ar << NumActive;
	for (const FGrimQuestProgress* Progress : Saved)
	{
		FString Id = Progress->QuestId.ToString();
		int32 Step = Progress->Step;
		TArray<int32> Counts = Progress->Counts;
		Ar << Id << Step << Counts;
	}
}

void UGrimQuestComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	FGrimQuestLog Loaded;
	int32 NumCompleted = 0;
	Ar << NumCompleted;
	for (int32 Index = 0; Index < NumCompleted && !Ar.IsError(); ++Index)
	{
		FString Id;
		Ar << Id;
		Loaded.Completed.AddUnique(FName(*Id));
	}
	int32 NumActive = 0;
	Ar << NumActive;
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();
	for (int32 Index = 0; Index < NumActive && !Ar.IsError(); ++Index)
	{
		FString Id;
		int32 Step = 0;
		TArray<int32> Counts;
		Ar << Id << Step << Counts;
		const FGrimQuestDef* Def = Database.Find(FName(*Id));
		if (!Def || !Def->Steps.IsValidIndex(Step))
		{
			continue; // 표에서 사라졌거나 단계가 줄었다
		}
		FGrimQuestProgress& Progress = Loaded.Active.AddDefaulted_GetRef();
		Progress.QuestId = Def->Id;
		Progress.Step = Step;
		Progress.Counts = Counts;
		Progress.Counts.SetNumZeroed(Def->Steps[Step].Objectives.Num());
	}
	if (Ar.IsError())
	{
		UE_LOG(LogGrimQuest, Warning, TEXT("[Server] %s 퀘스트 세이브를 읽지 못했다(버전 %d)"), *GetNameSafe(GetOwner()), Version);
		return;
	}
	Log = MoveTemp(Loaded);
	bAutoStartTried = true;
	UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s 퀘스트 세이브: 진행 %d · 끝냄 %d"), *GetNameSafe(GetOwner()), Log.Active.Num(), Log.Completed.Num());
	MarkLogChanged();
}
