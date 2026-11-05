#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"
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
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

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

	AbilityInputBindings = {
		{ TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IA_SpellCast.IA_SpellCast"))), GrimTags::Input_Spell_Cast },
		{ TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IA_SpellNext.IA_SpellNext"))), GrimTags::Input_Spell_Next },
		{ TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IA_SpellPrev.IA_SpellPrev"))), GrimTags::Input_Spell_Prev },
	};
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
	// 책이 선택 슬롯보다 늦게 왔을 수 있다 — 지금 고른 쪽을 기억시킨다.
	if (UGrimSpellbookComponent* Spellbook = BoundSpellbook.Get())
	{
		HandleSelectedSpellChanged(Spellbook, Spellbook->GetSelectedSpell());
	}
}

void AGrimPlayerCharacter::BindSpellbookEvents()
{
	const AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>();
	UGrimSpellbookComponent* Spellbook = GrimPlayerState ? GrimPlayerState->GetSpellbook() : nullptr;
	if (Spellbook == BoundSpellbook.Get())
	{
		return;
	}
	if (UGrimSpellbookComponent* Old = BoundSpellbook.Get())
	{
		Old->OnSelectedSpellChanged.RemoveDynamic(this, &ThisClass::HandleSelectedSpellChanged);
	}
	BoundSpellbook = Spellbook;
	if (Spellbook)
	{
		Spellbook->OnSelectedSpellChanged.AddUniqueDynamic(this, &ThisClass::HandleSelectedSpellChanged);
		// 지금 고른 쪽을 책이 기억하게 한다(처음 한 번은 넘기지 않는다).
		HandleSelectedSpellChanged(Spellbook, Spellbook->GetSelectedSpell());
	}
}

void AGrimPlayerCharacter::HandleSelectedSpellChanged(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* SelectedSpell)
{
	if (!GrimoireBook || !Spellbook || Spellbook->GetSelectedIndex() == INDEX_NONE)
	{
		return;
	}
	FGameplayCueParameters Parameters;
	Parameters.SourceObject = SelectedSpell;
	const FLinearColor Color = UGrimMagicFXStatics::ResolveColor(UGrimMagicFXStatics::ResolveVisualSet(Parameters, this), Parameters);
	GrimoireBook->FlipTo(Spellbook->GetSelectedIndex(), Color);
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
	if (Ability && Ability->GetAvatarActorFromActorInfo() == this && Ability->IsA<UGrimSpellAbility>())
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
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
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
