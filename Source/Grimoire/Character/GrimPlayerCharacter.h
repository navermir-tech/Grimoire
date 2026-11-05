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
class AGrimGrimoireBook;
class UGrimSpellbookComponent;
class UGrimSpellDefinition;
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
 * 왼손 그리모어(R21)를 서버에서 스폰해 붙이고, 애님은 시전 동작을 덧입히는 ABP_GrimPlayer(부모 UGrimSpellPoseAnimInstance)를 쓴다.
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

	/** 왼손 위에 떠 있는 그리모어 (R21). 서버가 스폰하고 포인터는 복제된다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Grimoire")
	AGrimGrimoireBook* GetGrimoireBook() const { return GrimoireBook; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 시전 순간 몸을 조준 방향(컨트롤 요)으로 CastFaceTurnTime 안에 돌리고 CastFaceHoldTime 동안 그쪽을 본다(spell-feel 「시전 동작」).
	 * 서버(어빌리티 발동 콜백)와 소유 클라이언트(예측 발동 · 시전 큐)에서 불린다 — 다른 사람에게는 이동 복제로 간다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Cast")
	void FaceAimForCast();

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	/** 서버에서 스폰되는 그리모어. 클래스는 Grimoire Magic FX 설정의 GrimoireBookClass. */
	UPROPERTY(ReplicatedUsing = OnRep_GrimoireBook, VisibleInstanceOnly, BlueprintReadOnly, Category = "Grimoire|Grimoire")
	TObjectPtr<AGrimGrimoireBook> GrimoireBook;

	UFUNCTION()
	void OnRep_GrimoireBook();

	/** 시전 때 몸을 조준 쪽으로 돌리는 시간 · 그쪽을 보고 있는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Cast", meta = (ClampMin = "0.01", Units = "s"))
	float CastFaceTurnTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Cast", meta = (ClampMin = "0", Units = "s"))
	float CastFaceHoldTime = 0.4f;

private:
	void InitAbilityActorInfo();

	/** 서버: 그리모어를 (없으면 만들어) 왼손에 붙인다. */
	void SpawnGrimoireBook();

	/** 마법 선택이 바뀌면 그리모어를 그 쪽까지 넘긴다(서버 · 클라이언트 모두 — 선택 슬롯은 복제된다). */
	void BindSpellbookEvents();

	UFUNCTION()
	void HandleSelectedSpellChanged(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* SelectedSpell);

	TWeakObjectPtr<UGrimSpellbookComponent> BoundSpellbook;

	/** 어빌리티 발동(서버 · 예측 클라이언트) — 마법이면 몸을 조준 쪽으로. */
	void HandleAbilityActivated(class UGameplayAbility* Ability);

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
	FDelegateHandle AbilityActivatedHandle;

	float CastFaceElapsed = -1.f;
	float CastFaceTargetYaw = 0.f;
	float CastFaceStartYaw = 0.f;
	bool bCastFaceSavedOrient = true;
};
