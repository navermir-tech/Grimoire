#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GrimPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UGrimAbilitySystemComponent;
struct FInputActionValue;

/** 입력 액션 하나를 어빌리티 입력 태그 하나에 잇는다. */
USTRUCT()
struct FGrimAbilityInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> Action;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * 플레이어 폰. 3인칭 이동 · 카메라 · 점프와 마법 입력 전달만 한다.
 * ASC 는 AGrimPlayerState 가 가지고, 이 폰은 아바타일 뿐이다(탑승 · 사망으로 폰이 바뀌어도 상태 유지).
 * 마법 입력은 어떤 어빌리티가 있는지 모른 채 Input.Spell.* 태그로 ASC 에 보낸다.
 */
UCLASS()
class GRIMOIRE_API AGrimPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGrimPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UGrimAbilitySystemComponent* GetGrimAbilitySystemComponent() const;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** 완드 등 손에 드는 것을 붙이는 메시 소켓(본) 이름. */
	static const FName HandSocketName;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SpellInputPressed(FGameplayTag InputTag);
	void SpellInputReleased(FGameplayTag InputTag);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grimoire|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 속성 상태(젖음 · 불붙음) — 사물과 같은 규칙표를 쓴다 (R16). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grimoire|Elemental")
	TObjectPtr<class UGrimElementalStateComponent> ElementalState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grimoire|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// 입력 액션. 매핑(키)은 컨트롤러가 붙이는 매핑 컨텍스트에 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	/** 입력 액션 → 어빌리티 입력 태그. 눌림 = Started, 뗌 = Completed. */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<FGrimAbilityInputBinding> AbilityInputBindings;

private:
	void InitAbilityActorInfo();
};
