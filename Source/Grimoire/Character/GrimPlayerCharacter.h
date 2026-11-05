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
 * 마법 입력은 어떤 어빌리티가 있는지 모른 채 Input.Spell.* · Input.Action.* 태그로 ASC 에 보낸다.
 * 조작은 호그와트 레거시 배치(Q29, docs/spec/references 「조작」):
 *  - 기본 공격 = 좌클릭 · 패드 R2/RT 짧게(Input.Spell.BasicAttack)
 *  - 지금 세트 슬롯 1~4 바로 시전 = 숫자키 1~4 · 패드 R2/RT 누른 채 얼굴 버튼(Input.Spell.Slot1~4). 어느 마법이 어느 태그인지는 서버가 정한다.
 *  - 세트 바꾸기 = (F1~F4 는 UI 갈래) · 패드 R2/RT 누른 채 십자키 · 마우스 휠(보조)
 *  - 방어 = Q · 패드 Y/△, 회피 = Ctrl · 패드 B/○ — 지금은 입력 자리만(Input.Action.*)
 * 패드에서 R2 를 누른 동안(세트 활성)에는 얼굴 버튼이 슬롯 시전이 되고 점프 · 방어 · 회피로 가지 않는다.
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

	/** 어빌리티 입력 태그를 누름 · 뗌으로 ASC 에 보낸다(로컬). 디버그 콘솔(Grim.Input.Press)도 이 길로 누른다. */
	void SpellInputPressed(FGameplayTag InputTag);
	void SpellInputReleased(FGameplayTag InputTag);

	/** 패드: 기본 공격 버튼(R2/RT)을 누르고 있어 슬롯 세트가 활성인가(로컬). HUD 가 슬롯을 밝힌다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Input")
	bool IsSpellSetHeld() const { return bSpellSetHeld; }

	/** 보이는 슬롯 세트를 바꾼다(0부터). 서버면 바로, 아니면 RPC. */
	void RequestSlotSet(int32 SetIndex);

	/** 다음 · 이전 슬롯 세트(끝에서 돈다) */
	void CycleSlotSet(int32 Step);

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void HandleJumpPressed();
	void HandleJumpReleased();

	/** 패드 R2/RT: 짧게 눌렀다 떼면(Tap) 기본 공격. 누른 동안 슬롯을 썼으면 기본 공격으로 치지 않는다. */
	void HandleBasicAttackPadTap();
	void HandleSpellSetHoldStarted();
	void HandleSpellSetHoldEnded();
	void HandleSpellSetPadInput(int32 SetIndex);
	void HandleSpellSetWheel(int32 Step);

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

	/** 입력 액션 → 어빌리티 입력 태그. 눌림 = Started, 뗌 = Completed(취소 포함). 기본 공격(마우스) · 슬롯 1~4(숫자키 · 패드) · 방어 · 회피. */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<FGrimAbilityInputBinding> AbilityInputBindings;

	/** 패드 기본 공격: Tap 트리거(짧게 눌렀다 뗌)가 걸린 R2/RT */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> BasicAttackPadAction;

	/** 패드 슬롯 세트 활성: R2/RT 누르는 동안. 얼굴 버튼 · 십자키 입력은 이 액션을 Chord 로 건다 */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> SpellSetHoldAction;

	/** 패드 세트 고르기(R2/RT + 십자키) — 0번이 세트 1 */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TArray<TSoftObjectPtr<UInputAction>> SpellSetPadActions;

	/** 다음 · 이전 세트(마우스 휠, 보조) */
	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> SpellSetNextAction;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|Input")
	TSoftObjectPtr<UInputAction> SpellSetPrevAction;

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

	/** 마법서가 바뀌면(배움 · 슬롯 배치) 서버가 슬롯 입력 태그를 다시 붙인다. */
	void BindSpellbookEvents();

	UFUNCTION()
	void HandleSelectedSpellChanged(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* SelectedSpell);

	/**
	 * 매 틱: 보이는 세트가 바뀌었으면 그리모어를 그 쪽까지 넘기고(모든 기기 — 세트는 복제된다), 서버는 슬롯 입력 태그를 다시 붙인다.
	 * 서버는 주먹질 어빌리티(UGrimFistAbility — 기본 공격을 배우기 전 · 완드 없음)를 한 번 준다.
	 */
	void TickSpellbookWatch();

	UGrimSpellbookComponent* GetSpellbook() const;

	TWeakObjectPtr<UGrimSpellbookComponent> BoundSpellbook;

	/** 마지막으로 본 세트(INDEX_NONE = 아직) */
	int32 WatchedSlotSet = INDEX_NONE;

	/** 서버: 주먹질을 줬다 */
	bool bFistGranted = false;

	// 패드 세트 활성(로컬)
	bool bSpellSetHeld = false;
	bool bSlotUsedDuringSetHold = false;

	/** 어빌리티 발동(서버 · 예측 클라이언트) — 마법이면 몸을 조준 쪽으로. */
	void HandleAbilityActivated(class UGameplayAbility* Ability);

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
	FDelegateHandle AbilityActivatedHandle;

	float CastFaceElapsed = -1.f;
	float CastFaceTargetYaw = 0.f;
	float CastFaceStartYaw = 0.f;
	bool bCastFaceSavedOrient = true;
};
