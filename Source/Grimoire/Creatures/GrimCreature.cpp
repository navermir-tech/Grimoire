#include "Creatures/GrimCreature.h"
#include "Items/GrimItemStatics.h"
#include "Creatures/GrimCreatureAIController.h"
#include "Creatures/GrimCreatureAttackAbility.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTags.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "Quest/GrimQuestComponent.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace GrimCreatureLocal // 유니티 빌드에서 다른 파일의 도우미와 부딪히지 않게
{
	const TCHAR* RoleName(const AActor* Actor)
	{
		if (!Actor || !Actor->GetWorld())
		{
			return TEXT("?");
		}
		switch (Actor->GetWorld()->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("Server");
		case NM_DedicatedServer: return TEXT("Server");
		default: return TEXT("Standalone");
		}
	}

	UStaticMesh* LoadShape(EGrimCreatureShape Shape)
	{
		const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
		switch (Shape)
		{
		case EGrimCreatureShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
		case EGrimCreatureShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
		case EGrimCreatureShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
		default: break;
		}
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	FString DropsToString(const TArray<FGrimCreatureLootDrop>& Drops)
	{
		FString Out;
		for (const FGrimCreatureLootDrop& Drop : Drops)
		{
			Out += FString::Printf(TEXT("%s%s x%d"), Out.IsEmpty() ? TEXT("") : TEXT(", "), *Drop.ItemId.ToString(), Drop.Count);
		}
		return Out.IsEmpty() ? TEXT("없음") : Out;
	}

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

	const FName ColorParameter(TEXT("Color"));
}

AGrimCreature::AGrimCreature(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);

	AIControllerClass = AGrimCreatureAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->bUseControllerDesiredRotation = false;
		Move->RotationRate = FRotator(0.f, 540.f, 0.f);
		Move->MaxWalkSpeed = 200.f;
		Move->GetNavAgentPropertiesRef().bCanSwim = true;
		// 내비 메시 없이 곧게 달리므로 낭떠러지(발판 끝)에서 멈춘다.
		Move->bCanWalkOffLedges = false;
		Move->bCanWalkOffLedgesWhenCrouching = false;
	}
	GetCapsuleComponent()->InitCapsuleSize(30.f, 30.f);
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		// 스켈레탈 메시는 아직 없다 — 기본 도형 부위(BodyRoot)가 몸이다.
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMesh->SetVisibility(false);
	}

	AbilitySystem = CreateDefaultSubobject<UGrimAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	VitalAttributes = CreateDefaultSubobject<UGrimVitalAttributeSet>(TEXT("VitalAttributes"));

	ElementalState = CreateDefaultSubobject<UGrimElementalStateComponent>(TEXT("ElementalState"));

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(GetCapsuleComponent());
}

AGrimCreature* AGrimCreature::SpawnCreature(UWorld* World, UGrimCreatureSpecies* InSpecies, const FGrimCreatureInstance& InInstance, const FVector& FootLocation, float Yaw, float HealthRatio, AGrimHabitatVolume* InHabitat, int32 InHerdId)
{
	if (!World || !InSpecies || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	const float HalfHeight = InSpecies->CapsuleHalfHeight * FMath::Max(0.1f, InInstance.SizeScale);
	const FTransform Transform(FRotator(0.f, Yaw, 0.f), FootLocation + FVector(0.f, 0.f, HalfHeight + 2.f));
	AGrimCreature* Creature = World->SpawnActorDeferred<AGrimCreature>(AGrimCreature::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Creature)
	{
		return nullptr;
	}
	Creature->InitCreature(InSpecies, InInstance, HealthRatio, InHabitat, InHerdId);
	Creature->FinishSpawning(Transform);
	return Creature;
}

void AGrimCreature::InitCreature(UGrimCreatureSpecies* InSpecies, const FGrimCreatureInstance& InInstance, float InHealthRatio, AGrimHabitatVolume* InHabitat, int32 InHerdId)
{
	Species = InSpecies;
	Instance = InInstance;
	InitialHealthRatio = FMath::Clamp(InHealthRatio, 0.01f, 1.f);
	Habitat = InHabitat;
	HerdId = InHerdId;
	if (Species && ElementalState)
	{
		// 두 속성 생물은 두 속성의 재료 태그를 함께 가진다(R35 — 불 · 풀이면 타기도 쉽고 물도 빨아들인다).
		ElementalState->Material = UGrimCreatureSettings::Get()->GetMaterialsForElements(Species->PrimaryElement, Species->SecondaryElement);
	}
	// 모양 · 태그는 BeginPlay 에서(컴포넌트 등록 뒤).
}

// ───────────────────────── 생명 주기 ─────────────────────────

void AGrimCreature::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	ApplySpeciesLocal();

	if (HasAuthority())
	{
		HomeLocation = GetActorLocation();
		ApplySpeciesServer();
		ElementalState->OnStateChangedNative.AddUObject(this, &AGrimCreature::HandleElementalStateChanged);
		if (UGrimEcosystemSubsystem* Ecosystem = GetWorld()->GetSubsystem<UGrimEcosystemSubsystem>())
		{
			Ecosystem->RegisterCreature(this);
		}
	}
}

void AGrimCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
		if (UGrimEcosystemSubsystem* Ecosystem = World->GetSubsystem<UGrimEcosystemSubsystem>())
		{
			Ecosystem->UnregisterCreature(this);
		}
	}
	if (AbilitySystem && HealthChangedHandle.IsValid())
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimCreature::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimCreature, Species);
	DOREPLIFETIME(AGrimCreature, Instance);
	DOREPLIFETIME(AGrimCreature, AIState);
	DOREPLIFETIME(AGrimCreature, bShadowHidden);
	DOREPLIFETIME(AGrimCreature, Death);
}

UAbilitySystemComponent* AGrimCreature::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

// ───────────────────────── 종 · 개체 적용 ─────────────────────────

void AGrimCreature::ApplySpeciesLocal()
{
	// 복제가 BeginPlay 보다 먼저 오면 BeginPlay 가 다시 부른다.
	if (!Species || !Instance.IsValid() || !(HasActorBegunPlay() || IsActorBeginningPlay()))
	{
		return;
	}
	const float Size = FMath::Max(0.1f, Instance.SizeScale);
	GetCapsuleComponent()->SetCapsuleSize(Species->CapsuleRadius * Size, Species->CapsuleHalfHeight * Size);
	BodyRoot->SetRelativeScale3D(FVector(Size));

	if (!bLocalSpeciesApplied && AbilitySystem)
	{
		bLocalSpeciesApplied = true;
		// 속성 태그(서버 · 클라이언트 각자 — 종에서 정해지므로 복제하지 않는다). 부속성은 SubElement 표지를 함께 단다(Q35 주 · 부 구분).
		if (Species->PrimaryElement.IsValid())
		{
			AbilitySystem->AddLooseGameplayTag(Species->PrimaryElement);
		}
		if (Species->SecondaryElement.IsValid())
		{
			AbilitySystem->AddLooseGameplayTag(Species->SecondaryElement);
			const FGameplayTag Marker = UGrimDamageExecution::GetSecondaryMarkerTag(Species->SecondaryElement);
			if (Marker.IsValid())
			{
				AbilitySystem->AddLooseGameplayTag(Marker);
			}
		}
		AbilitySystem->AddLooseGameplayTags(Species->AbilityTags);
		if (Instance.bGiant)
		{
			AbilitySystem->AddLooseGameplayTag(GrimCreatureTags::Creature_Ability_GiantRoar);
		}
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (Species->WaterMovement == EGrimCreatureWaterMovement::RunOnSurface)
		{
			// 물 위 달리기 자리: 물 볼륨 안에서 수면으로 떠올라 달리기 속도로 헤엄친다(탑승 · 진짜 수면 이동 모드는 다음 묶음).
			const FGrimCreatureAbilityTuning& Abilities = UGrimCreatureSettings::Get()->Abilities;
			Move->Buoyancy = Abilities.WaterRunBuoyancy;
			Move->MaxSwimSpeed = Species->RunSpeed * Abilities.WaterRunSpeedScale;
		}
	}

	BuildBodyParts();
	RefreshBodyVisibility();
	RefreshPose();

	if (!bLoggedAppear)
	{
		bLoggedAppear = true;
		FString Colors;
		for (const FGrimCreatureRegionColor& Color : Instance.Colors)
		{
			Colors += FString::Printf(TEXT("%s%s%s"), Colors.IsEmpty() ? TEXT("") : TEXT(","), *Color.Color.ToFColor(true).ToHex(), Color.bMutant ? TEXT("*") : TEXT(""));
		}
		UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 생물 나타남 %s seed=%d %s 크기=%.2f%s%s 색=%s"),
			GrimCreatureLocal::RoleName(this), *Instance.SpeciesId.ToString(), Instance.Seed,
			Instance.Sex == EGrimCreatureSex::Male ? TEXT("수") : TEXT("암"), Instance.SizeScale,
			Instance.bGiant ? TEXT(" 거대") : TEXT(""), Instance.bMutant ? TEXT(" 돌연변이") : TEXT(""), *Colors);
	}
}

void AGrimCreature::BuildBodyParts()
{
	if (!Species || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (BuiltForSpecies == Species->GetFName() && BuiltForSeed == Instance.Seed)
	{
		return;
	}
	for (UStaticMeshComponent* PartMesh : BodyPartMeshes)
	{
		if (PartMesh)
		{
			PartMesh->DestroyComponent();
		}
	}
	BodyPartMeshes.Reset();
	BodyPartMaterials.Reset();
	BodyPartIndices.Reset();

	for (int32 PartIndex = 0; PartIndex < Species->BodyParts.Num(); ++PartIndex)
	{
		const FGrimCreatureBodyPart& Part = Species->BodyParts[PartIndex];
		// 성별 부위(뿔 · 갈기)는 그 성별에만 만든다.
		if (!GrimCreatureMath::IsPartForSex(Part.Sex, Instance.Sex))
		{
			continue;
		}
		UStaticMesh* Shape = GrimCreatureLocal::LoadShape(Part.Shape);
		if (!Shape)
		{
			continue;
		}
		UStaticMeshComponent* PartMesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Part_%s_%d"), *Part.PartName.ToString(), PartIndex));
		PartMesh->SetupAttachment(BodyRoot);
		PartMesh->SetStaticMesh(Shape);
		PartMesh->SetRelativeLocation(Part.Location);
		PartMesh->SetRelativeRotation(Part.Rotation);
		PartMesh->SetRelativeScale3D(Part.Scale);
		PartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PartMesh->SetGenerateOverlapEvents(false);
		PartMesh->SetCanEverAffectNavigation(false);
		PartMesh->RegisterComponent();

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(PartMesh->GetMaterial(0), this);
		if (Material)
		{
			const FLinearColor Color = Instance.Colors.IsValidIndex(Part.ColorRegion) ? Instance.Colors[Part.ColorRegion].Color : FLinearColor(0.5f, 0.5f, 0.5f);
			Material->SetVectorParameterValue(GrimCreatureLocal::ColorParameter, Color);
			PartMesh->SetMaterial(0, Material);
		}
		BodyPartMeshes.Add(PartMesh);
		BodyPartMaterials.Add(Material);
		BodyPartIndices.Add(PartIndex);
	}
	BuiltForSpecies = Species->GetFName();
	BuiltForSeed = Instance.Seed;
}

void AGrimCreature::RefreshBodyVisibility()
{
	if (!Species)
	{
		return;
	}
	const bool bHide = bShadowHidden && !Death.bDead;
	for (int32 i = 0; i < BodyPartMeshes.Num(); ++i)
	{
		UStaticMeshComponent* PartMesh = BodyPartMeshes[i];
		if (!PartMesh)
		{
			continue;
		}
		const int32 PartIndex = BodyPartIndices.IsValidIndex(i) ? BodyPartIndices[i] : INDEX_NONE;
		const bool bKeep = Species->BodyParts.IsValidIndex(PartIndex) && Species->BodyParts[PartIndex].bVisibleWhenHidden;
		PartMesh->SetVisibility(!bHide || bKeep);
	}
}

void AGrimCreature::RefreshPose()
{
	if (!BodyRoot)
	{
		return;
	}
	const float Size = Instance.IsValid() ? FMath::Max(0.1f, Instance.SizeScale) : 1.f;
	if (Death.bDead)
	{
		// 옆으로 눕는다.
		BodyRoot->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		BodyRoot->SetRelativeScale3D(FVector(Size));
	}
	else if (AIState == EGrimCreatureAIState::Sleep)
	{
		// 웅크린다.
		BodyRoot->SetRelativeRotation(FRotator::ZeroRotator);
		BodyRoot->SetRelativeScale3D(FVector(Size, Size, Size * 0.7f));
	}
	else
	{
		BodyRoot->SetRelativeRotation(FRotator::ZeroRotator);
		BodyRoot->SetRelativeScale3D(FVector(Size));
	}
}

void AGrimCreature::ApplySpeciesServer()
{
	if (bServerInitialized || !Species || !AbilitySystem)
	{
		return;
	}
	bServerInitialized = true;
	const UGrimCreatureSettings* Settings = UGrimCreatureSettings::Get();

	const float MaxHealth = FMath::Max(1.f, Species->MaxHealth * Instance.StatScale);
	AbilitySystem->SetNumericAttributeBase(UGrimVitalAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), MaxHealth * InitialHealthRatio);
	// 접혔다 펼친 약한 개체가 「약해짐」을 다시 알리지 않게.
	bWeakenedNotified = InitialHealthRatio <= Settings->WeakenedHealthRatio;
	if (bWeakenedNotified)
	{
		AbilitySystem->AddLooseGameplayTag(GrimCreatureTags::Creature_State_Weakened);
	}
	HealthChangedHandle = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UGrimVitalAttributeSet::GetHealthAttribute()).AddUObject(this, &AGrimCreature::HandleHealthChanged);

	// 공격 어빌리티 — 속성 하나에 하나(Spec 의 동적 소스 태그 = 그 속성).
	for (const FGrimCreatureAttack& Attack : Species->Attacks)
	{
		if (!Attack.Element.IsValid() || AttackHandles.Contains(Attack.Element))
		{
			continue;
		}
		FGameplayAbilitySpec Spec(UGrimCreatureAttackAbility::StaticClass(), 1, INDEX_NONE, this);
		Spec.GetDynamicSpecSourceTags().AddTag(Attack.Element);
		AttackHandles.Add(Attack.Element, AbilitySystem->GiveAbility(Spec));
	}

	SetRunning(false);

	if (Species->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_SeedSpread))
	{
		const float Interval = FMath::Max(0.2f, Settings->Abilities.SeedSpreadInterval);
		GetWorldTimerManager().SetTimer(SeedTimer, this, &AGrimCreature::TickSeedSpread, Interval, true, FMath::FRandRange(0.f, Interval));
	}
}

// ───────────────────────── 조회 ─────────────────────────

float AGrimCreature::GetHealth() const
{
	return AbilitySystem ? AbilitySystem->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()) : 0.f;
}

float AGrimCreature::GetMaxHealth() const
{
	return AbilitySystem ? AbilitySystem->GetNumericAttribute(UGrimVitalAttributeSet::GetMaxHealthAttribute()) : 0.f;
}

float AGrimCreature::GetHealthRatio() const
{
	const float Max = GetMaxHealth();
	return Max > 0.f ? FMath::Clamp(GetHealth() / Max, 0.f, 1.f) : 0.f;
}

FGrimCreatureTemperamentInfo AGrimCreature::GetTemperamentInfo() const
{
	FGrimCreatureTemperamentInfo Info;
	if (Species)
	{
		Info.Temperament = Species->Temperament;
		Info.Diet = Species->Diet;
		Info.FleeHealthRatio = Species->GetFleeHealthRatio(UGrimCreatureSettings::Get()->AI.FleeHealthRatio);
		Info.bAvoidsLight = Species->bAvoidsLight;
		Info.bCanAttack = !Species->Attacks.IsEmpty();
	}
	return Info;
}

FVector AGrimCreature::GetTargetPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.3f);
}

EGrimTargetKind AGrimCreature::GetTargetKind() const
{
	return Species && Species->Temperament == EGrimCreatureTemperament::Aggressive ? EGrimTargetKind::Enemy : EGrimTargetKind::Creature;
}

bool AGrimCreature::CanBeTargetedBy(const AActor* Requester) const
{
	return !Death.bDead && !bShadowHidden && !IsHidden();
}

AActor* AGrimCreature::GetProvoker() const
{
	AActor* Attacker = LastAttacker.Get();
	if (!Attacker || !GetWorld())
	{
		return nullptr;
	}
	if (const AGrimCreature* Other = Cast<AGrimCreature>(Attacker); Other && Other->IsDead())
	{
		return nullptr;
	}
	return GetWorld()->GetTimeSeconds() - LastAttackedTime <= UGrimCreatureSettings::Get()->AI.ProvokedMemorySeconds ? Attacker : nullptr;
}

float AGrimCreature::GetSurfaceDistanceTo(const AActor* Other) const
{
	if (!Other)
	{
		return TNumericLimits<float>::Max();
	}
	float OtherRadius = 0.f;
	float OtherHalfHeight = 0.f;
	Other->GetSimpleCollisionCylinder(OtherRadius, OtherHalfHeight);
	const float Center = FVector::Dist2D(GetActorLocation(), Other->GetActorLocation());
	return FMath::Max(0.f, Center - GetCapsuleComponent()->GetScaledCapsuleRadius() - OtherRadius);
}

float AGrimCreature::GetMaxAttackRange() const
{
	float Range = 0.f;
	if (Species)
	{
		for (const FGrimCreatureAttack& Attack : Species->Attacks)
		{
			Range = FMath::Max(Range, Attack.Range);
		}
	}
	return Range;
}

float AGrimCreature::GetReadyAttackRange() const
{
	if (!Species || Species->Attacks.IsEmpty() || !GetWorld())
	{
		return 0.f;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	float ReadyMax = -1.f;
	float MinRange = TNumericLimits<float>::Max();
	for (const FGrimCreatureAttack& Attack : Species->Attacks)
	{
		MinRange = FMath::Min(MinRange, Attack.Range);
		const double* Next = NextAttackTime.Find(Attack.Element);
		if (!Next || Now >= *Next)
		{
			ReadyMax = FMath::Max(ReadyMax, Attack.Range);
		}
	}
	return ReadyMax >= 0.f ? ReadyMax : MinRange;
}

FGrimFoldedCreature AGrimCreature::MakeFoldedRecord() const
{
	FGrimFoldedCreature Record;
	Record.Instance = Instance;
	Record.Location = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Record.HealthRatio = GetHealthRatio();
	Record.HerdId = HerdId;
	return Record;
}

// ───────────────────────── 서버: AI 가 부르는 것 ─────────────────────────

void AGrimCreature::SetAIState(EGrimCreatureAIState NewState)
{
	if (!HasAuthority() || AIState == NewState)
	{
		return;
	}
	AIState = NewState;
	RefreshPose();
}

void AGrimCreature::SetRunning(bool bRun)
{
	if (!Species)
	{
		return;
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = bRun ? Species->RunSpeed : Species->WalkSpeed;
	}
}

bool AGrimCreature::TryAttack(AActor* Target)
{
	if (!HasAuthority() || Death.bDead || !Target || !Species || !AbilitySystem)
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const float Distance = GetSurfaceDistanceTo(Target);

	TArray<const FGrimCreatureAttack*, TInlineAllocator<4>> Ready;
	float TotalWeight = 0.f;
	for (const FGrimCreatureAttack& Attack : Species->Attacks)
	{
		const double* Next = NextAttackTime.Find(Attack.Element);
		if (AttackHandles.Contains(Attack.Element) && (!Next || Now >= *Next) && Distance <= Attack.Range)
		{
			Ready.Add(&Attack);
			TotalWeight += FMath::Max(0.01f, Attack.Weight);
		}
	}
	if (Ready.IsEmpty())
	{
		return false;
	}
	float Pick = FMath::FRand() * TotalWeight;
	const FGrimCreatureAttack* Chosen = Ready.Last();
	for (const FGrimCreatureAttack* Attack : Ready)
	{
		Pick -= FMath::Max(0.01f, Attack->Weight);
		if (Pick <= 0.f)
		{
			Chosen = Attack;
			break;
		}
	}

	// 덮칠 때 그림자에서 나온다.
	EndShadowStep(true);
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}

	NextAttackTime.Add(Chosen->Element, Now + Chosen->Cooldown);
	PendingAttackTarget = Target;
	const bool bActivated = AbilitySystem->TryActivateAbility(AttackHandles[Chosen->Element]);
	PendingAttackTarget = nullptr;
	return bActivated;
}

bool AGrimCreature::CanShadowStep() const
{
	return HasAuthority() && Species && !Death.bDead && !bShadowHidden
		&& Species->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_ShadowStep)
		&& GetWorld() && GetWorld()->GetTimeSeconds() >= NextShadowStepTime;
}

void AGrimCreature::StartShadowStep()
{
	if (!CanShadowStep())
	{
		return;
	}
	bShadowHidden = true;
	AbilitySystem->AddLooseGameplayTag(GrimCreatureTags::Creature_State_ShadowHidden);
	GetWorldTimerManager().SetTimer(ShadowStepTimer, FTimerDelegate::CreateUObject(this, &AGrimCreature::EndShadowStep, true), UGrimCreatureSettings::Get()->Abilities.ShadowStepSeconds, false);
	RefreshBodyVisibility();
	ForceNetUpdate();
	UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 그림자 걸음 %s seed=%d"), GrimCreatureLocal::RoleName(this), *Instance.SpeciesId.ToString(), Instance.Seed);
}

void AGrimCreature::EndShadowStep(bool bStartCooldown)
{
	if (!HasAuthority() || !bShadowHidden)
	{
		return;
	}
	bShadowHidden = false;
	AbilitySystem->RemoveLooseGameplayTag(GrimCreatureTags::Creature_State_ShadowHidden);
	GetWorldTimerManager().ClearTimer(ShadowStepTimer);
	if (bStartCooldown && GetWorld())
	{
		NextShadowStepTime = GetWorld()->GetTimeSeconds() + UGrimCreatureSettings::Get()->Abilities.ShadowStepCooldown;
	}
	RefreshBodyVisibility();
	ForceNetUpdate();
	UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 그림자 걸음 풀림 %s seed=%d"), GrimCreatureLocal::RoleName(this), *Instance.SpeciesId.ToString(), Instance.Seed);
}

// ───────────────────────── 속성 자극 ─────────────────────────

void AGrimCreature::ApplyElementalStimulus(const FGrimElementalStimulus& Stimulus, AActor* StimulusInstigator)
{
	if (!HasAuthority())
	{
		return;
	}
	if (ElementalState)
	{
		ElementalState->ApplyElementalStimulus(Stimulus, StimulusInstigator);
	}
	if (Death.bDead)
	{
		return;
	}
	// 빛 · 불빛에 드러난다(그림자 걸음).
	if (bShadowHidden && Stimulus.Element.IsValid() && Stimulus.Element.MatchesAny(UGrimCreatureSettings::Get()->Abilities.ShadowRevealElements))
	{
		EndShadowStep(true);
	}
	// 마법을 맞으면 쏜 이에게 반응한다(피해가 없는 마법이어도).
	if (Stimulus.Source == EGrimStimulusSource::Spell && StimulusInstigator && StimulusInstigator != this)
	{
		NoteAttacker(nullptr, StimulusInstigator);
	}
}

float AGrimCreature::GetElementalStateIntensity(const FGameplayTag& StateTag) const
{
	return ElementalState ? ElementalState->GetElementalStateIntensity(StateTag) : 0.f;
}

void AGrimCreature::HandleElementalStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity)
{
	if (!HasAuthority() || Death.bDead || StateTag != GrimTags::State_Burning)
	{
		return;
	}
	if (NewIntensity > 0.f && OldIntensity <= 0.f)
	{
		BurningSince = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(BurnTimer, this, &AGrimCreature::TickBurning, 1.f, true);
		UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 불붙음 %s seed=%d — 도망"), *Instance.SpeciesId.ToString(), Instance.Seed);
	}
	else if (NewIntensity <= 0.f)
	{
		BurningSince = -1.0;
		GetWorldTimerManager().ClearTimer(BurnTimer);
	}
}

void AGrimCreature::TickBurning()
{
	const float Intensity = GetElementalStateIntensity(GrimTags::State_Burning);
	if (Death.bDead || Intensity <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(BurnTimer);
		return;
	}
	const FGrimCreatureAbilityTuning& Tuning = UGrimCreatureSettings::Get()->Abilities;
	// 불을 붙인 이(마지막으로 때린 이)의 ASC 로 — 타 죽어도 사냥한 플레이어의 몫이다.
	UAbilitySystemComponent* SourceASC = LastAttacker.IsValid() ? UGrimSpellAbility::FindTargetASC(LastAttacker.Get()) : nullptr;
	ApplyDebugDamage(Tuning.BurnDamagePerSecond * Intensity, GrimTags::Element_Fire, SourceASC);

	if (!Death.bDead && BurningSince >= 0.0 && GetWorld()->GetTimeSeconds() - BurningSince >= Tuning.BurnSelfExtinguishSeconds)
	{
		// 흙에 뒹굴어 끈다 — 반응 규칙(Earth_Smother)과 같은 길.
		FGrimElementalStimulus Smother;
		Smother.Element = GrimTags::Element_Earth;
		Smother.Intensity = 1.f;
		Smother.Source = EGrimStimulusSource::Creature;
		ElementalState->ApplyElementalStimulus(Smother, this);
		UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 뒹굴어 불을 끔 %s seed=%d"), *Instance.SpeciesId.ToString(), Instance.Seed);
	}
}

void AGrimCreature::TickSeedSpread()
{
	if (Death.bDead || AIState == EGrimCreatureAIState::Sleep || AIState == EGrimCreatureAIState::Fight)
	{
		return;
	}
	const FGrimCreatureAbilityTuning& Tuning = UGrimCreatureSettings::Get()->Abilities;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSeedSpread), false, this);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Feet, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), FCollisionShape::MakeSphere(Tuning.SeedSpreadRadius), Params);

	TSet<AActor*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == this || Seen.Contains(Actor))
		{
			continue;
		}
		Seen.Add(Actor);
		const UGrimElementalStateComponent* State = Actor->FindComponentByClass<UGrimElementalStateComponent>();
		if (!State || (Tuning.SeedSpreadMaterial.IsValid() && !State->Material.HasTag(Tuning.SeedSpreadMaterial)))
		{
			continue;
		}
		// 지나간 자리 마른 풀에 풀 자극 — 「다시 난다」는 반응 규칙표 몫(행이 없으면 아무 일 없다).
		FGrimElementalStimulus Seed;
		Seed.Element = GrimCreatureTags::Element_Earth_Plant;
		Seed.Intensity = Tuning.SeedSpreadIntensity;
		Seed.Source = EGrimStimulusSource::Creature;
		if (UGrimElementalStatics::ApplyStimulusToActor(Actor, Seed, this))
		{
			++SeedSpreadCount;
			UE_LOG(LogGrimCreatures, Verbose, TEXT("[Server] 씨앗 퍼뜨리기 %s → %s"), *Instance.SpeciesId.ToString(), *Actor->GetName());
		}
	}
}

// ───────────────────────── 피해 · 죽음 ─────────────────────────

void AGrimCreature::NoteAttacker(AActor* InstigatorActor, AActor* CauserActor)
{
	AActor* Attacker = nullptr;
	if (APawn* CauserPawn = Cast<APawn>(CauserActor))
	{
		Attacker = CauserPawn;
	}
	else if (const APlayerState* AttackerState = Cast<APlayerState>(InstigatorActor))
	{
		Attacker = AttackerState->GetPawn();
	}
	else if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		Attacker = InstigatorPawn;
	}
	if (Attacker && Attacker != this && GetWorld())
	{
		LastAttacker = Attacker;
		LastAttackedTime = GetWorld()->GetTimeSeconds();
		if (APlayerState* AttackerState = GrimCreatureLocal::FindPlayerState(Attacker))
		{
			LastDamagerForQuest = AttackerState;
		}
	}
}

void AGrimCreature::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority() || Death.bDead)
	{
		return;
	}
	AActor* InstigatorActor = nullptr;
	AActor* CauserActor = nullptr;
	if (Data.GEModData)
	{
		const FGameplayEffectContextHandle& Context = Data.GEModData->EffectSpec.GetEffectContext();
		InstigatorActor = Context.GetOriginalInstigator();
		CauserActor = Context.GetEffectCauser();
	}
	if (Data.NewValue < Data.OldValue)
	{
		NoteAttacker(InstigatorActor, CauserActor);
		// 맞으면 그림자에서 드러난다.
		EndShadowStep(true);
	}

	const UGrimCreatureSettings* Settings = UGrimCreatureSettings::Get();
	const float Max = GetMaxHealth();
	const float Ratio = Max > 0.f ? Data.NewValue / Max : 0.f;
	if (Data.NewValue > 0.f && Ratio <= Settings->WeakenedHealthRatio && !bWeakenedNotified)
	{
		bWeakenedNotified = true;
		AbilitySystem->AddLooseGameplayTag(GrimCreatureTags::Creature_State_Weakened);
		UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 약해짐 %s seed=%d 체력 %.1f/%.1f"), *Instance.SpeciesId.ToString(), Instance.Seed, Data.NewValue, Max);
		NotifyQuestEvent(LastDamagerForQuest.Get(), Settings->WeakenedEventPrefix);
	}
	if (Data.NewValue <= 0.f)
	{
		Die(InstigatorActor ? InstigatorActor : LastDamagerForQuest.Get(), CauserActor);
	}
}

void AGrimCreature::DebugKill(AActor* Killer)
{
	if (!HasAuthority() || Death.bDead)
	{
		return;
	}
	NoteAttacker(Killer, Killer);
	Die(Killer, Cast<APawn>(Killer));
	AbilitySystem->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), 0.f);
}

void AGrimCreature::ApplyDebugDamage(float Amount, const FGameplayTag& Element, UAbilitySystemComponent* SourceASC)
{
	if (!HasAuthority() || !AbilitySystem || Amount <= 0.f)
	{
		return;
	}
	UAbilitySystemComponent* Source = SourceASC ? SourceASC : AbilitySystem.Get();
	FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UGrimGE_SpellDamage::StaticClass(), 1.f, Source->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, Amount);
	if (Element.IsValid())
	{
		Spec.Data->AddDynamicAssetTag(Element);
	}
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), AbilitySystem);
}

void AGrimCreature::Die(AActor* Killer, AActor* KillerPawn)
{
	if (!HasAuthority() || Death.bDead)
	{
		return;
	}
	const UGrimCreatureSettings* Settings = UGrimCreatureSettings::Get();
	APlayerState* KillerPlayer = GrimCreatureLocal::FindPlayerState(Killer);
	if (!KillerPlayer)
	{
		KillerPlayer = GrimCreatureLocal::FindPlayerState(KillerPawn);
	}
	Death.bDead = true;
	Death.KillerPlayer = KillerPlayer;
	Death.KillerName = KillerPlayer ? KillerPlayer->GetPlayerName() : (Killer ? Killer->GetName() : FString(TEXT("환경")));
	if (const AGrimCreature* KillerCreature = Cast<AGrimCreature>(Killer))
	{
		Death.KillerName = KillerCreature->GetInstance().SpeciesId.ToString();
	}
	FRandomStream Rng(Instance.Seed ^ static_cast<int32>(GetWorld()->GetTimeSeconds() * 1000.0));
	Death.Drops = Species ? GrimCreatureMath::RollLoot(Species->Loot, Instance.bGiant, Settings->Roll.GiantLootMultiplier, Rng) : TArray<FGrimCreatureLootDrop>();

	EndShadowStep(false);
	SetAIState(EGrimCreatureAIState::Dead);
	AbilitySystem->AddLooseGameplayTag(GrimCreatureTags::Creature_State_Dead);
	AbilitySystem->CancelAllAbilities();
	GetWorldTimerManager().ClearTimer(SeedTimer);
	GetWorldTimerManager().ClearTimer(BurnTimer);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 생물 죽음 %s seed=%d 죽인 이=%s 전리품=%s"),
		*Instance.SpeciesId.ToString(), Instance.Seed, *Death.KillerName, *GrimCreatureLocal::DropsToString(Death.Drops));

	OnCreatureDiedNative.Broadcast(this, Killer);
	OnCreatureDied.Broadcast(this, Killer);
	SpawnLootPickups(Death.Drops);

	if (KillerPlayer)
	{
		NotifyQuestEvent(KillerPlayer, Settings->KilledEventPrefix);
	}
	if (UGrimEcosystemSubsystem* Ecosystem = GetWorld()->GetSubsystem<UGrimEcosystemSubsystem>())
	{
		Ecosystem->HandleCreatureDied(this);
	}

	bLoggedDeath = true;
	RefreshPose();
	RefreshBodyVisibility();
	ForceNetUpdate();
	SetLifeSpan(FMath::Max(0.5f, Settings->Ecosystem.CorpseSeconds));
}

void AGrimCreature::SpawnLootPickups(const TArray<FGrimCreatureLootDrop>& Drops)
{
	const FVector Location = GetActorLocation();
	for (int32 Index = 0; Index < Drops.Num(); ++Index)
	{
		const FGrimCreatureLootDrop& Drop = Drops[Index];
		// 전리품을 몸 둘레에 조금씩 흩어 놓는다(겹쳐 하나로 보이지 않게).
		const FVector Offset = FRotator(0.f, 360.f * Index / FMath::Max(1, Drops.Num()), 0.f).Vector() * 40.f;
		UGrimItemStatics::SpawnPickup(GetWorld(), Drop.ItemId, Drop.Count, Location + Offset);
		UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 전리품 %s x%d (%s seed=%d)"), *Drop.ItemId.ToString(), Drop.Count, *Instance.SpeciesId.ToString(), Instance.Seed);
		OnLootDroppedNative.Broadcast(this, Drop.ItemId, Drop.Count, Location);
		OnLootDropped.Broadcast(this, Drop.ItemId, Drop.Count, Location);
	}
}

void AGrimCreature::NotifyQuestEvent(AActor* PlayerActor, const FString& Prefix) const
{
	if (!PlayerActor || !Species)
	{
		return;
	}
	const FName EventId = UGrimCreatureSettings::GetQuestEventName(Prefix, Species->SizeClass);
	UGrimQuestComponent::NotifyQuestEvent(PlayerActor, EventId);
	UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 퀘스트 이벤트 %s → %s"), *EventId.ToString(), *PlayerActor->GetName());
}

// ───────────────────────── 복제 콜백 ─────────────────────────

void AGrimCreature::OnRep_Instance()
{
	ApplySpeciesLocal();
}

void AGrimCreature::OnRep_AIState()
{
	RefreshPose();
}

void AGrimCreature::OnRep_ShadowHidden()
{
	RefreshBodyVisibility();
	UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 그림자 걸음 %s %s seed=%d"), GrimCreatureLocal::RoleName(this),
		bShadowHidden ? TEXT("숨음") : TEXT("풀림"), *Instance.SpeciesId.ToString(), Instance.Seed);
}

void AGrimCreature::OnRep_Death()
{
	RefreshPose();
	RefreshBodyVisibility();
	if (Death.bDead && !bLoggedDeath)
	{
		bLoggedDeath = true;
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 생물 죽음 %s seed=%d 죽인 이=%s 전리품=%s"), GrimCreatureLocal::RoleName(this),
			*Instance.SpeciesId.ToString(), Instance.Seed, *Death.KillerName, *GrimCreatureLocal::DropsToString(Death.Drops));
	}
}
