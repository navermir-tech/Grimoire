#include "Creatures/GrimTamingSubsystem.h"
#include "Magic/GrimCastFailure.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimWand.h"
#include "Quest/GrimQuestComponent.h"
#include "Save/GrimPlayerIdentity.h"
#include "Save/GrimSaveSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

namespace GrimTamingSubsystemLocal // 유니티 빌드 이름 충돌 방지
{
	TAutoConsoleVariable<float> CVarForceRoll(TEXT("Grim.Taming.ForceRoll"), -1.f,
		TEXT("개발용: 포획 굴림을 이 값(0~1)으로 고정한다. 음수 = 끔. 0 = 늘 성공, 0.999 = 거의 늘 실패"), ECVF_Default);

	APlayerState* FindPlayerState(AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			return PlayerState;
		}
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetPlayerState();
		}
		if (const AController* Controller = Cast<AController>(Actor))
		{
			return Controller->PlayerState;
		}
		return nullptr;
	}
}

bool UGrimTamingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimTamingSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

UGrimTamingSubsystem* UGrimTamingSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGrimTamingSubsystem>() : nullptr;
}

float UGrimTamingSubsystem::GetForcedRoll()
{
#if UE_BUILD_SHIPPING
	return -1.f;
#else
	return GrimTamingSubsystemLocal::CVarForceRoll.GetValueOnGameThread();
#endif
}

void UGrimTamingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsServer())
	{
		return;
	}
	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UGrimTamingSubsystem::HandleActorSpawned));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsureTamer(PlayerState);
		}
	}
	for (TActorIterator<AGrimCreature> It(&InWorld); It; ++It)
	{
		HandleActorSpawned(*It);
	}
}

void UGrimTamingSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

void UGrimTamingSubsystem::HandleActorSpawned(AActor* Actor)
{
	if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		EnsureTamer(PlayerState);
	}
	else if (AGrimCreature* Creature = Cast<AGrimCreature>(Actor))
	{
		Creature->OnCreatureDiedNative.AddUObject(this, &UGrimTamingSubsystem::HandleCreatureDied);
	}
}

UGrimTamerComponent* UGrimTamingSubsystem::EnsureTamer(APlayerState* PlayerState)
{
	if (!PlayerState || !IsServer() || !Cast<AGrimPlayerState>(PlayerState))
	{
		return nullptr;
	}
	UGrimTamerComponent* Tamer = PlayerState->FindComponentByClass<UGrimTamerComponent>();
	if (!Tamer)
	{
		Tamer = NewObject<UGrimTamerComponent>(PlayerState, UGrimTamerComponent::StaticClass(), UGrimTamerComponent::ComponentName);
		PlayerState->AddInstanceComponent(Tamer);
		Tamer->RegisterComponent();
		UE_LOG(LogGrimTaming, Log, TEXT("[Server] %s 에 길들이기 컴포넌트를 붙였다"), *GetNameSafe(PlayerState));
	}
	// 데리고 다니기 · 생물 보관함(R38) — 같은 방식으로 함께 붙인다.
	if (!PlayerState->FindComponentByClass<UGrimCreatureStorageComponent>())
	{
		UGrimCreatureStorageComponent* Storage = NewObject<UGrimCreatureStorageComponent>(PlayerState, UGrimCreatureStorageComponent::StaticClass(), UGrimCreatureStorageComponent::ComponentName);
		PlayerState->AddInstanceComponent(Storage);
		Storage->RegisterComponent();
	}
	return Tamer;
}

FString UGrimTamingSubsystem::ResolvePlayerId(const APlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return FString();
	}
	if (const UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(PlayerState->GetWorld()))
	{
		const FString Id = Save->GetPlayerId(PlayerState);
		if (!Id.IsEmpty())
		{
			return Id;
		}
	}
	FString Id = FGrimPlayerIdentity::ResolveServerSide(PlayerState);
	if (Id.IsEmpty())
	{
		Id = FString::Printf(TEXT("PS-%d"), PlayerState->GetPlayerId());
	}
	return Id;
}

// ───────────────────────── 포획 ─────────────────────────

bool UGrimTamingSubsystem::IsPactElementMatch(const FGameplayTag& PactElement, const AGrimCreature* Creature)
{
	const UGrimCreatureSpecies* Species = Creature ? Creature->GetSpecies() : nullptr;
	if (!Species || !PactElement.IsValid() || PactElement.MatchesTagExact(GrimMagicTags::Element_None))
	{
		return false;
	}
	auto Matches = [&PactElement](const FGameplayTag& Element) { return Element.IsValid() && Element.MatchesTag(PactElement); };
	return Matches(Species->PreferredPactElement) || Matches(Species->PrimaryElement) || Matches(Species->SecondaryElement);
}

FGrimCaptureInput UGrimTamingSubsystem::GatherCaptureInput(const UGrimSpellDefinition* Spell, const UAbilitySystemComponent* CasterASC, const AGrimCreature* Creature)
{
	FGrimCaptureInput Input;
	const UGrimCreatureSpecies* Species = Creature ? Creature->GetSpecies() : nullptr;
	if (!Species)
	{
		return Input;
	}
	const UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	const APlayerState* Caster = CasterASC ? Cast<APlayerState>(CasterASC->GetOwnerActor()) : nullptr;
	const UAbilitySystemComponent* CreatureASC = Creature->GetAbilitySystemComponent();

	Input.HealthRatio = Creature->GetHealthRatio();
	Input.bAsleep = Creature->GetAIState() == EGrimCreatureAIState::Sleep;
	Input.bExhausted = Taming && Taming->IsExhausted();
	bool bFavorite = false;
	Input.bFed = Taming && Taming->WasFedBy(Caster, bFavorite);
	Input.bFedFavorite = Input.bFed && bFavorite;
	Input.bElementMatch = Spell && IsPactElementMatch(Spell->Element, Creature);
	Input.RequiredCircle = Species->TamingCircle;
	Input.SizeClass = Species->SizeClass;
	Input.bGiant = Creature->GetInstance().bGiant;
	(void)CreatureASC;

	if (CasterASC)
	{
		Input.CasterCircle = FMath::RoundToInt(CasterASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()));
	}
	if (const AGrimPlayerState* GrimCaster = Cast<AGrimPlayerState>(Caster))
	{
		if (const UGrimSpellbookComponent* Spellbook = GrimCaster->GetSpellbook())
		{
			if (const AGrimWand* Wand = Spellbook->GetWand())
			{
				Input.WandElementPower = Wand->GetElementPower(Species->PrimaryElement);
			}
			const FGameplayTag BonusTag = UGrimTamingSettings::Get()->CaptureBonusEffectTag;
			if (Spell && (!BonusTag.IsValid() || Spell->ProficiencyExtraEffect.EffectTag.MatchesTagExact(BonusTag)))
			{
				Input.ProficiencyBonus = Spellbook->GetProficiencyExtraEffect(Spell);
			}
		}
	}
	return Input;
}

EGrimTamingResult UGrimTamingSubsystem::TryTame(const UGrimSpellDefinition* Spell, UAbilitySystemComponent* CasterASC, AGrimCreature* Creature, FGrimCaptureChance* OutChance)
{
	if (!IsServer() || !Creature || !CasterASC)
	{
		return EGrimTamingResult::Invalid;
	}
	APlayerState* Caster = Cast<APlayerState>(CasterASC->GetOwnerActor());
	UGrimCreatureTamingComponent* Taming = Creature->GetTaming();
	const UGrimCreatureSpecies* Species = Creature->GetSpecies();
	UGrimTamerComponent* Tamer = EnsureTamer(Caster);
	if (!Caster || !Taming || !Species || !Tamer)
	{
		return EGrimTamingResult::Invalid;
	}
	if (!Species->bTameable || Creature->IsDead() || Taming->IsTamed())
	{
		const FVector At = Creature->GetActorLocation();
		GrimCastFailure::Report(CasterASC, EGrimCastFailReason::TameRefused, 0, &At);
		return EGrimTamingResult::NotTameable;
	}

	// 데리고 다니는 수(R38): 갈 곳(파티 → 보관함 → 거점 상자)이 없으면 굴리지 않는다 — 생물도 화내지 않는다.
	UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(Caster);
	const EGrimNewTameDestination Destination = Storage ? Storage->DecideNewTame() : EGrimNewTameDestination::Party;
	if (Destination == EGrimNewTameDestination::Refused)
	{
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 계약 → %s seed=%d: 갈 곳 없음(데리고 다님 %d/%d · 보관함 %s)"), *Species->SpeciesId.ToString(), Creature->GetInstance().Seed,
			Storage->GetPartyCount(), Storage->GetPartyLimit(), *FString::Printf(TEXT("%d/%d%s"), Storage->GetStored().Num(), Storage->GetStorageCapacity(), Storage->IsStorageLearned() ? TEXT("") : TEXT(" 쿠스토디아 안 배움")));
		// Q64: 왜 안 걸렸는지 — 서버 판정 → 소유 클라이언트 알림 + 생물 앞에서 피식 꺼지는 연출.
		const FVector At = Creature->GetActorLocation();
		GrimCastFailure::Report(CasterASC, EGrimCastFailReason::TameNoRoom, 0, &At);
		return EGrimTamingResult::NoRoom;
	}

	const FGrimCaptureInput Input = GatherCaptureInput(Spell, CasterASC, Creature);
	const FGrimCaptureChance Chance = GrimTamingMath::CalcCaptureChance(Input, UGrimTamingSettings::Get()->Capture, UGrimCreatureSettings::Get()->WeakenedHealthRatio);
	if (OutChance)
	{
		*OutChance = Chance;
	}
	const FString SpellName = Spell ? Spell->GetName() : FString(TEXT("?"));
	if (Chance.bCircleTooLow)
	{
		UE_LOG(LogGrimTaming, Display, TEXT("[Server] 계약 %s → %s seed=%d: 서클 모자람(%d < %d)"), *SpellName, *Species->SpeciesId.ToString(), Creature->GetInstance().Seed, Input.CasterCircle, Input.RequiredCircle);
		const FVector At = Creature->GetActorLocation();
		GrimCastFailure::Report(CasterASC, EGrimCastFailReason::TameCircleTooLow, Input.RequiredCircle, &At);
		return EGrimTamingResult::CircleTooLow;
	}

	const float Forced = GetForcedRoll();
	const float Roll = Forced >= 0.f ? FMath::Clamp(Forced, 0.f, 1.f) : FMath::FRand();
	const bool bSuccess = GrimTamingMath::RollCapture(Chance.Total, Roll);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 계약 %s → %s seed=%d 확률 %s 굴림 %.3f → %s"), *SpellName, *Species->SpeciesId.ToString(), Creature->GetInstance().Seed,
		*Chance.ToString(), Roll, bSuccess ? TEXT("성공") : TEXT("실패"));

	if (!bSuccess)
	{
		// 저항: 화내 덤빈다(겁 많은 종도).
		Taming->Enrage(Caster->GetPawn());
		Tamer->SendNotice(FString::Printf(TEXT("%s 이(가) 계약에 저항했다! (확률 %.0f%%)"), *Taming->GetDisplayName(), Chance.Total * 100.f));
		return EGrimTamingResult::Failed;
	}

	const FString PlayerId = Tamer->GetPlayerId();
	Taming->Claim(Caster, PlayerId);
	if (UGrimEcosystemSubsystem* Ecosystem = GetWorld()->GetSubsystem<UGrimEcosystemSubsystem>())
	{
		Ecosystem->HandleCreatureTamed(Creature);
	}
	Tamer->AddOrUpdateRecord(Taming->MakeRecord());
	Tamer->SendNotice(FString::Printf(TEXT("%s 을(를) 길들였다! (확률 %.0f%%)%s"), *Taming->GetDisplayName(), Chance.Total * 100.f,
		Destination == EGrimNewTameDestination::Party ? TEXT(" — 가까이 가서 [F]") : TEXT("")));
	if (Storage && Destination != EGrimNewTameDestination::Party)
	{
		// 파티가 가득 — 보관함 · 거점 상자로(다음 틱, 투사체 명중 처리 뒤).
		Storage->PlaceNewlyTamed(Creature, Destination);
	}

	const FName EventId = UGrimCreatureSettings::GetQuestEventName(UGrimTamingSettings::Get()->TamedEventPrefix, Species->SizeClass);
	UGrimQuestComponent::NotifyQuestEvent(Caster, EventId);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 길들임 %s seed=%d 주인=%s 충성도=%.0f 퀘스트 이벤트 %s"), *Species->SpeciesId.ToString(), Creature->GetInstance().Seed,
		*PlayerId, Taming->GetLoyalty(), *EventId.ToString());
	return EGrimTamingResult::Success;
}

// ───────────────────────── 주인 기록 ─────────────────────────

TArray<AGrimCreature*> UGrimTamingSubsystem::GetOwnedCreatures(const FString& OwnerId, bool bIncludeAtBase) const
{
	TArray<AGrimCreature*> Out;
	if (OwnerId.IsEmpty() || !GetWorld())
	{
		return Out;
	}
	for (TActorIterator<AGrimCreature> It(GetWorld()); It; ++It)
	{
		const UGrimCreatureTamingComponent* Taming = It->GetTaming();
		if (!bIncludeAtBase && Taming && Taming->IsAtBase())
		{
			continue; // 거점에 풀어놓은 생물은 상자가 맡는다(R38)
		}
		if (!It->IsDead() && !It->IsActorBeingDestroyed() && It->GetInstance().OwnerId == OwnerId)
		{
			Out.Add(*It);
		}
	}
	return Out;
}

UGrimTamerComponent* UGrimTamingSubsystem::FindTamerById(const FString& PlayerId) const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || PlayerId.IsEmpty())
	{
		return nullptr;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		UGrimTamerComponent* Tamer = PlayerState ? PlayerState->FindComponentByClass<UGrimTamerComponent>() : nullptr;
		if (Tamer && Tamer->GetPlayerId() == PlayerId)
		{
			return Tamer;
		}
	}
	return nullptr;
}

void UGrimTamingSubsystem::NotifyReleased(AGrimCreature* Creature, const FString& FormerOwnerId, bool bLeftByLoyalty)
{
	if (!Creature)
	{
		return;
	}
	if (UGrimTamerComponent* Tamer = FindTamerById(FormerOwnerId))
	{
		Tamer->RemoveRecord(Creature->GetInstance());
		const FString Name = Creature->GetTaming() ? Creature->GetTaming()->GetDisplayName() : Creature->GetInstance().SpeciesId.ToString();
		Tamer->SendNotice(bLeftByLoyalty ? FString::Printf(TEXT("%s 이(가) 떠나 야생으로 돌아갔다"), *Name) : FString::Printf(TEXT("%s 을(를) 놓아주었다"), *Name));
	}
}

void UGrimTamingSubsystem::HandleCreatureDied(AGrimCreature* Creature, AActor* Killer)
{
	using namespace GrimTamingSubsystemLocal;
	if (!Creature || !GetWorld())
	{
		return;
	}
	// 길들인 개체가 정말 죽었다(디버그 처치 등) — 주인 기록에서 지운다.
	if (Creature->GetInstance().IsTamed())
	{
		if (UGrimTamerComponent* Tamer = FindTamerById(Creature->GetInstance().OwnerId))
		{
			Tamer->RemoveRecord(Creature->GetInstance());
		}
		// 거점에 풀어놓은 생물이면 상자 칸에서도 지운다(R38).
		if (UGrimCreatureTamingComponent* Taming = Creature->GetTaming(); Taming && Taming->GetHomeBox())
		{
			Taming->GetHomeBox()->ForgetCreature(Creature->GetInstance().SpeciesId, Creature->GetInstance().Seed);
		}
	}
	// 함께 싸워 이김: 죽인 이가 주인이거나 그 생물 자신이고, 곁(WinTogetherRadius)에 있었다.
	const APlayerState* KillerPlayer = FindPlayerState(Killer);
	const float Radius = UGrimTamingSettings::Get()->Loyalty.WinTogetherRadius;
	for (TActorIterator<AGrimCreature> It(GetWorld()); It; ++It)
	{
		AGrimCreature* Ally = *It;
		UGrimCreatureTamingComponent* Taming = Ally ? Ally->GetTaming() : nullptr;
		if (!Ally || Ally == Creature || Ally->IsDead() || !Taming || !Taming->IsTamed())
		{
			continue;
		}
		const bool bByOwner = KillerPlayer && Taming->GetOwnerPlayerState() == KillerPlayer;
		if ((bByOwner || Killer == Ally) && FVector::Dist(Ally->GetActorLocation(), Creature->GetActorLocation()) <= Radius)
		{
			Taming->NotifyWonTogether();
		}
	}
}
