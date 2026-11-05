#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimFistAbility.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellSlotInput.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimMagicFXSettings.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "Grimoire.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"

const FName AGrimPlayerCharacter::HandSocketName(TEXT("hand_r"));

AGrimPlayerCharacter::AGrimPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 몸은 이동 방향을 보고, 컨트롤러 회전은 카메라만 돌린다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 아래 이동 수치는 기본값이다. 조정은 BP 자식이나 에디터 디테일에서 한다.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
	Movement->JumpZVelocity = 500.f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = 500.f;
	Movement->MinAnalogWalkSpeed = 20.f;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// 호그와트 레거시처럼 어깨 너머 카메라(2026-11-05 플레이 테스트) — 오른쪽 어깨 위, 가깝게, 살짝 늦게 따라온다.
	// 줌인 조준(Targeting)은 시작할 때 이 값을 기억했다가 되돌린다.
	CameraBoom->TargetArmLength = 270.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(85.f);

	ElementalState = CreateDefaultSubobject<UGrimElementalStateComponent>(TEXT("ElementalState"));

	// 템플릿 마네킹(Content/Characters). 다른 몸은 BP 자식에서 바꾼다.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (BodyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(BodyMesh.Object);
	}
	// 애님: ABP_GrimPlayer = 템플릿 ABP_Unarmed 를 복제해 부모를 UGrimSpellPoseAnimInstance 로 바꾼 것(시전 동작 · 책 받치기).
	// 아직 없으면(에셋 스크립트 전) 템플릿 그대로.
	static ConstructorHelpers::FClassFinder<UAnimInstance> SpellAnim(TEXT("/Game/Grimoire/MagicFX/Anim/ABP_GrimPlayer"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (SpellAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(SpellAnim.Class);
	}
	else if (BodyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(BodyAnim.Class);
	}
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	// 입력 에셋은 소프트 참조 — 파이썬으로 만드는 에셋이 없어도 CDO 생성이 실패하지 않는다.
	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Move.IA_Move")));
	LookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Look.IA_Look")));
	MouseLookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook")));
	JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump")));

	// 호그와트 레거시 배치(Q29). 키는 Content/Grimoire/Input 의 IMC_Spell(Scripts/Character/make_input.py).
	auto InputAsset = [](const TCHAR* Name)
	{
		return TSoftObjectPtr<UInputAction>(FSoftObjectPath(FString::Printf(TEXT("/Game/Grimoire/Input/%s.%s"), Name, Name)));
	};
	AbilityInputBindings = {
		{ InputAsset(TEXT("IA_BasicAttack")), GrimInputTags::Input_Spell_BasicAttack },
		{ InputAsset(TEXT("IA_SpellSlot1")), GrimInputTags::Input_Spell_Slot1 },
		{ InputAsset(TEXT("IA_SpellSlot2")), GrimInputTags::Input_Spell_Slot2 },
		{ InputAsset(TEXT("IA_SpellSlot3")), GrimInputTags::Input_Spell_Slot3 },
		{ InputAsset(TEXT("IA_SpellSlot4")), GrimInputTags::Input_Spell_Slot4 },
		{ InputAsset(TEXT("IA_SpellSlotPad1")), GrimInputTags::Input_Spell_Slot1 },
		{ InputAsset(TEXT("IA_SpellSlotPad2")), GrimInputTags::Input_Spell_Slot2 },
		{ InputAsset(TEXT("IA_SpellSlotPad3")), GrimInputTags::Input_Spell_Slot3 },
		{ InputAsset(TEXT("IA_SpellSlotPad4")), GrimInputTags::Input_Spell_Slot4 },
		{ InputAsset(TEXT("IA_Protego")), GrimInputTags::Input_Action_Protego },
		{ InputAsset(TEXT("IA_Dodge")), GrimInputTags::Input_Action_Dodge },
	};
	BasicAttackPadAction = InputAsset(TEXT("IA_BasicAttackPad"));
	SpellSetHoldAction = InputAsset(TEXT("IA_SpellSetHold"));
	SpellSetPadActions = { InputAsset(TEXT("IA_SpellSetPad1")), InputAsset(TEXT("IA_SpellSetPad2")), InputAsset(TEXT("IA_SpellSetPad3")), InputAsset(TEXT("IA_SpellSetPad4")) };
	SpellSetNextAction = InputAsset(TEXT("IA_SpellSetNext"));
	SpellSetPrevAction = InputAsset(TEXT("IA_SpellSetPrev"));
	SprintAction = InputAsset(TEXT("IA_Sprint"));
	SprintToggleAction = InputAsset(TEXT("IA_SprintToggle"));
}

UAbilitySystemComponent* AGrimPlayerCharacter::GetAbilitySystemComponent() const
{
	return GetGrimAbilitySystemComponent();
}

UGrimAbilitySystemComponent* AGrimPlayerCharacter::GetGrimAbilitySystemComponent() const
{
	const AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>();
	return GrimPlayerState ? GrimPlayerState->GetGrimAbilitySystemComponent() : nullptr;
}

void AGrimPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGrimPlayerCharacter, GrimoireBook);
}

void AGrimPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버(리슨 서버 호스트 포함).
	InitAbilityActorInfo();
	SpawnGrimoireBook();
	BindSpellbookEvents();
}

void AGrimPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트 — 자기 폰과 남의 폰 모두. 남의 폰도 아바타가 있어야 태그 · 큐가 보인다.
	InitAbilityActorInfo();
	BindSpellbookEvents();
}

void AGrimPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGrimSpellbookComponent* Spellbook = BoundSpellbook.Get())
	{
		Spellbook->OnSelectedSpellChanged.RemoveDynamic(this, &ThisClass::HandleSelectedSpellChanged);
	}
	BoundSpellbook.Reset();

	if (UAbilitySystemComponent* ASC = BoundAbilitySystem.Get())
	{
		ASC->AbilityActivatedCallbacks.Remove(AbilityActivatedHandle);
	}
	BoundAbilitySystem.Reset();

	if (HasAuthority() && GrimoireBook)
	{
		GrimoireBook->Destroy();
		GrimoireBook = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimPlayerCharacter::SpawnGrimoireBook()
{
	if (!HasAuthority())
	{
		return;
	}
	if (!GrimoireBook)
	{
		UClass* BookClass = UGrimMagicFXSettings::Get()->GrimoireBookClass.LoadSynchronous();
		if (!BookClass)
		{
			BookClass = AGrimGrimoireBook::StaticClass();
		}

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GrimoireBook = GetWorld()->SpawnActor<AGrimGrimoireBook>(BookClass, GetActorTransform(), Params);
		if (!GrimoireBook)
		{
			UE_LOG(LogGrimoire, Warning, TEXT("%s: 그리모어를 스폰하지 못했다."), *GetNameSafe(this));
			return;
		}
	}
	GrimoireBook->SetOwner(this);
	GrimoireBook->AttachToCharacterMesh(GetMesh());
}

void AGrimPlayerCharacter::OnRep_GrimoireBook()
{
	// 책이 세트 복제보다 늦게 왔을 수 있다 — 지금 세트를 기억시킨다(처음 한 번은 넘기지 않는다).
	if (GrimoireBook && WatchedSlotSet != INDEX_NONE)
	{
		GrimoireBook->FlipTo(WatchedSlotSet, FLinearColor::White);
	}
}

UGrimSpellbookComponent* AGrimPlayerCharacter::GetSpellbook() const
{
	const AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>();
	return GrimPlayerState ? GrimPlayerState->GetSpellbook() : nullptr;
}

void AGrimPlayerCharacter::BindSpellbookEvents()
{
	UGrimSpellbookComponent* Spellbook = GetSpellbook();
	if (Spellbook == BoundSpellbook.Get())
	{
		return;
	}
	if (UGrimSpellbookComponent* Old = BoundSpellbook.Get())
	{
		Old->OnSelectedSpellChanged.RemoveDynamic(this, &ThisClass::HandleSelectedSpellChanged);
	}
	BoundSpellbook = Spellbook;
	WatchedSlotSet = INDEX_NONE;
	if (Spellbook)
	{
		Spellbook->OnSelectedSpellChanged.AddUniqueDynamic(this, &ThisClass::HandleSelectedSpellChanged);
		HandleSelectedSpellChanged(Spellbook, Spellbook->GetSelectedSpell());
	}
}

void AGrimPlayerCharacter::HandleSelectedSpellChanged(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* SelectedSpell)
{
	// 배움 · 슬롯 배치가 바뀌었다 — 서버가 슬롯 입력 태그를 다시 붙인다(바뀐 Spec 만 복제).
	if (HasAuthority() && Spellbook)
	{
		GrimSpellSlotInput::RefreshSlotInputTags(GetAbilitySystemComponent(), Spellbook);
	}
}

void AGrimPlayerCharacter::TickSpellbookWatch()
{
	UGrimSpellbookComponent* Spellbook = BoundSpellbook.Get();
	if (!Spellbook)
	{
		return;
	}

	// 서버: 주먹질(기본 공격 입력의 대신 — 기본 공격을 배우기 전 · 완드 없음)은 마법 정의가 없어 마법서가 아니라 여기서 한 번 준다.
	// 기본 공격 자체는 튜토리얼의 「기본 공격 스킬북」으로 배운다(시작 지급 없음).
	if (HasAuthority() && !bFistGranted)
	{
		bFistGranted = UGrimFistAbility::EnsureGranted(GetAbilitySystemComponent());
	}

	const int32 Set = Spellbook->GetActiveSlotSet();
	if (Set == WatchedSlotSet)
	{
		return;
	}
	WatchedSlotSet = Set;
	if (HasAuthority())
	{
		GrimSpellSlotInput::RefreshSlotInputTags(GetAbilitySystemComponent(), Spellbook);
	}
	if (GrimoireBook)
	{
		// 세트를 바꾸면 책장이 그 세트 쪽까지 촤르륵(R21). 빛은 그 세트 첫 마법의 색.
		const UGrimSpellDefinition* First = nullptr;
		const TArray<TObjectPtr<UGrimSpellDefinition>>& SlotSpells = Spellbook->GetSlotSpells();
		const int32 PerSet = UGrimSpellbookSettings::Get()->SpellSlotsPerSet;
		for (int32 Index = Set * PerSet; Index < (Set + 1) * PerSet && Index < SlotSpells.Num() && !First; ++Index)
		{
			First = SlotSpells[Index];
		}
		FGameplayCueParameters Parameters;
		Parameters.SourceObject = First;
		const FLinearColor Color = First ? UGrimMagicFXStatics::ResolveColor(UGrimMagicFXStatics::ResolveVisualSet(Parameters, this), Parameters) : FLinearColor::White;
		GrimoireBook->FlipTo(Set, Color);
	}
}

void AGrimPlayerCharacter::RequestSlotSet(int32 SetIndex)
{
	UGrimSpellbookComponent* Spellbook = GetSpellbook();
	if (!Spellbook)
	{
		return;
	}
	if (HasAuthority())
	{
		Spellbook->SetActiveSlotSet(SetIndex);
	}
	else
	{
		Spellbook->Server_SetActiveSlotSet(SetIndex);
	}
}

void AGrimPlayerCharacter::CycleSlotSet(int32 Step)
{
	if (const UGrimSpellbookComponent* Spellbook = GetSpellbook())
	{
		RequestSlotSet(FGrimSpellSlotInputMath::CycleSet(Spellbook->GetActiveSlotSet(), Step, UGrimSpellbookSettings::Get()->SpellSlotSets));
	}
}

void AGrimPlayerCharacter::InitAbilityActorInfo()
{
	if (AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>())
	{
		GrimPlayerState->InitAbilityActorInfoForPawn(this);
	}

	// 마법이 발동하면(서버 · 예측하는 소유 클라이언트) 몸을 조준 쪽으로 돌린다.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC != BoundAbilitySystem.Get())
	{
		if (UAbilitySystemComponent* Old = BoundAbilitySystem.Get())
		{
			Old->AbilityActivatedCallbacks.Remove(AbilityActivatedHandle);
		}
		BoundAbilitySystem = ASC;
		AbilityActivatedHandle = ASC ? ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated) : FDelegateHandle();
	}
}

void AGrimPlayerCharacter::HandleAbilityActivated(UGameplayAbility* Ability)
{
	if (Ability && Ability->GetAvatarActorFromActorInfo() == this && (Ability->IsA<UGrimSpellAbility>() || Ability->IsA<UGrimFistAbility>()))
	{
		FaceAimForCast();
	}
}

void AGrimPlayerCharacter::FaceAimForCast()
{
	// 회전은 서버가 정하고 이동 복제로 다른 사람에게 간다. 소유 클라이언트는 예측으로 같이 돌린다.
	if (!Controller || !(HasAuthority() || IsLocallyControlled()))
	{
		return;
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (CastFaceElapsed < 0.f)
	{
		bCastFaceSavedOrient = Movement->bOrientRotationToMovement;
	}
	Movement->bOrientRotationToMovement = false;
	CastFaceStartYaw = GetActorRotation().Yaw;
	CastFaceTargetYaw = Controller->GetControlRotation().Yaw;
	CastFaceElapsed = 0.f;
}

void AGrimPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickSpellbookWatch();

	// 달리는 중에 탈진하거나 줌 조준을 시작하면 달리기를 멈춘다. 패드 토글은 멈추면(입력 없음) 풀린다.
	if (bSprinting && (IsSprintBlocked() || (bSprintToggled && GetVelocity().Size2D() < 10.f)))
	{
		if (IsLocallyControlled())
		{
			SetSprinting(false);
		}
		else if (HasAuthority() && IsSprintBlocked())
		{
			bSprinting = false;
			ApplySprintSpeed(false);
		}
	}

	if (CastFaceElapsed < 0.f)
	{
		return;
	}
	CastFaceElapsed += DeltaSeconds;
	if (Controller)
	{
		// 보고 있는 동안 조준을 따라간다(움직이며 시전).
		CastFaceTargetYaw = Controller->GetControlRotation().Yaw;
	}
	const float Alpha = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(CastFaceElapsed / CastFaceTurnTime, 0.f, 1.f));
	const float Yaw = CastFaceStartYaw + FMath::FindDeltaAngleDegrees(CastFaceStartYaw, CastFaceTargetYaw) * Alpha;
	SetActorRotation(FRotator(0.f, Yaw, 0.f));

	if (CastFaceElapsed >= CastFaceTurnTime + CastFaceHoldTime)
	{
		GetCharacterMovement()->bOrientRotationToMovement = bCastFaceSavedOrient;
		CastFaceElapsed = -1.f;
	}
}

void AGrimPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogGrimoire, Error, TEXT("%s: Enhanced Input 컴포넌트가 아니다 — DefaultInput.ini 를 확인한다."), *GetNameSafe(this));
		return;
	}

	if (const UInputAction* Action = JumpAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleJumpPressed);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::HandleJumpReleased);
	}
	if (const UInputAction* Action = MoveAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}
	if (const UInputAction* Action = LookAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
	if (const UInputAction* Action = MouseLookAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}

	for (const FGrimAbilityInputBinding& Binding : AbilityInputBindings)
	{
		const UInputAction* Action = Binding.Action.LoadSynchronous();
		if (!Action || !Binding.InputTag.IsValid())
		{
			UE_LOG(LogGrimoire, Warning, TEXT("%s: 어빌리티 입력 %s 를 찾지 못했다."), *GetNameSafe(this), *Binding.Action.ToString());
			continue;
		}
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::SpellInputPressed, Binding.InputTag);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::SpellInputReleased, Binding.InputTag);
		EnhancedInput->BindAction(Action, ETriggerEvent::Canceled, this, &ThisClass::SpellInputReleased, Binding.InputTag);
	}

	// 패드: R2/RT 짧게 = 기본 공격, 누르는 동안 = 세트 활성(얼굴 버튼 · 십자키가 Chord 로 슬롯 · 세트가 된다).
	if (const UInputAction* Action = BasicAttackPadAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ThisClass::HandleBasicAttackPadTap);
	}
	if (const UInputAction* Action = SpellSetHoldAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSpellSetHoldStarted);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::HandleSpellSetHoldEnded);
		EnhancedInput->BindAction(Action, ETriggerEvent::Canceled, this, &ThisClass::HandleSpellSetHoldEnded);
	}
	for (int32 SetIndex = 0; SetIndex < SpellSetPadActions.Num(); ++SetIndex)
	{
		if (const UInputAction* Action = SpellSetPadActions[SetIndex].LoadSynchronous())
		{
			EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSpellSetPadInput, SetIndex);
		}
	}
	if (const UInputAction* Action = SpellSetNextAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSpellSetWheel, 1);
	}
	if (const UInputAction* Action = SpellSetPrevAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSpellSetWheel, -1);
	}
	if (const UInputAction* Action = SprintAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSprintPressed);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::HandleSprintReleased);
	}
	if (const UInputAction* Action = SprintToggleAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleSprintToggle);
	}
}

void AGrimPlayerCharacter::HandleSprintPressed()
{
	bSprintToggled = false;
	SetSprinting(true);
}

void AGrimPlayerCharacter::HandleSprintReleased()
{
	SetSprinting(false);
}

void AGrimPlayerCharacter::HandleSprintToggle()
{
	bSprintToggled = !bSprinting;
	SetSprinting(!bSprinting);
}

bool AGrimPlayerCharacter::IsSprintBlocked() const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPlayerState());
	if (!ASC)
	{
		return false;
	}
	// 생존(Status.Exhausted)과 조준(State.Aiming) 태그는 이름으로 — 그쪽 헤더에 묶이지 않게.
	static const FGameplayTag Exhausted = FGameplayTag::RequestGameplayTag(TEXT("Status.Exhausted"), false);
	static const FGameplayTag Aiming = FGameplayTag::RequestGameplayTag(TEXT("State.Aiming"), false);
	return (Exhausted.IsValid() && ASC->HasMatchingGameplayTag(Exhausted)) || (Aiming.IsValid() && ASC->HasMatchingGameplayTag(Aiming));
}

void AGrimPlayerCharacter::SetSprinting(bool bNewSprinting)
{
	if (bNewSprinting && IsSprintBlocked())
	{
		bNewSprinting = false;
	}
	if (bNewSprinting == bSprinting)
	{
		return;
	}
	bSprinting = bNewSprinting;
	ApplySprintSpeed(bSprinting);
	if (!HasAuthority())
	{
		Server_SetSprinting(bSprinting);
	}
}

void AGrimPlayerCharacter::Server_SetSprinting_Implementation(bool bNewSprinting)
{
	bSprinting = bNewSprinting && !IsSprintBlocked();
	ApplySprintSpeed(bSprinting);
}

void AGrimPlayerCharacter::ApplySprintSpeed(bool bNewSprinting)
{
	if (bNewSprinting == bSprintSpeedApplied)
	{
		return;
	}
	bSprintSpeedApplied = bNewSprinting;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bNewSprinting ? Movement->MaxWalkSpeed * SprintSpeedScale : Movement->MaxWalkSpeed / SprintSpeedScale;
	}
}

void AGrimPlayerCharacter::HandleJumpPressed()
{
	// 패드 세트 활성 중의 A/✕ 는 슬롯 3 이다.
	if (!bSpellSetHeld)
	{
		Jump();
	}
}

void AGrimPlayerCharacter::HandleJumpReleased()
{
	StopJumping();
}

void AGrimPlayerCharacter::HandleBasicAttackPadTap()
{
	// 짧게 누른 사이 슬롯을 썼으면(빠른 R2 + 얼굴 버튼) 기본 공격은 아니다.
	if (bSlotUsedDuringSetHold)
	{
		bSlotUsedDuringSetHold = false;
		return;
	}
	SpellInputPressed(GrimInputTags::Input_Spell_BasicAttack);
	SpellInputReleased(GrimInputTags::Input_Spell_BasicAttack);
}

void AGrimPlayerCharacter::HandleSpellSetHoldStarted()
{
	bSpellSetHeld = true;
	bSlotUsedDuringSetHold = false;
}

void AGrimPlayerCharacter::HandleSpellSetHoldEnded()
{
	// bSlotUsedDuringSetHold 는 남겨 둔다 — 같은 프레임에 오는 Tap(기본 공격)이 본다.
	bSpellSetHeld = false;
}

void AGrimPlayerCharacter::HandleSpellSetPadInput(int32 SetIndex)
{
	bSlotUsedDuringSetHold = true;
	RequestSlotSet(SetIndex);
}

void AGrimPlayerCharacter::HandleSpellSetWheel(int32 Step)
{
	if (UGrimSpellInputSettings::Get()->bMouseWheelCyclesSets)
	{
		CycleSlotSet(Step);
	}
}

void AGrimPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void AGrimPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AGrimPlayerCharacter::SpellInputPressed(FGameplayTag InputTag)
{
	const bool bSlotInput = FGrimSpellSlotInputMath::GetSlotInSet(InputTag) != INDEX_NONE;
	if (bSpellSetHeld)
	{
		// 패드 세트 활성 중의 얼굴 버튼은 슬롯 시전이다(Y = 방어, B = 회피 자리로 가지 않는다).
		if (InputTag == GrimInputTags::Input_Action_Protego || InputTag == GrimInputTags::Input_Action_Dodge)
		{
			return;
		}
		if (bSlotInput)
		{
			bSlotUsedDuringSetHold = true;
		}
	}
	if (InputTag == GrimInputTags::Input_Action_Protego || InputTag == GrimInputTags::Input_Action_Dodge)
	{
		UE_LOG(LogGrimoire, Verbose, TEXT("%s: %s 입력(자리만 — 어빌리티 없음)"), *GetNameSafe(this), *InputTag.ToString());
	}
	if (UGrimAbilitySystemComponent* ASC = GetGrimAbilitySystemComponent())
	{
		ASC->AbilityInputTagPressed(InputTag);
	}
}

void AGrimPlayerCharacter::SpellInputReleased(FGameplayTag InputTag)
{
	if (UGrimAbilitySystemComponent* ASC = GetGrimAbilitySystemComponent())
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}
