#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Grimoire.h"
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
	static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (BodyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(BodyMesh.Object);
	}
	if (BodyAnim.Succeeded())
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

void AGrimPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버(리슨 서버 호스트 포함).
	InitAbilityActorInfo();
}

void AGrimPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트 — 자기 폰과 남의 폰 모두. 남의 폰도 아바타가 있어야 태그 · 큐가 보인다.
	InitAbilityActorInfo();
}

void AGrimPlayerCharacter::InitAbilityActorInfo()
{
	if (AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>())
	{
		GrimPlayerState->InitAbilityActorInfoForPawn(this);
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
