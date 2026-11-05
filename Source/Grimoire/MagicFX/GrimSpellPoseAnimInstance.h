#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "MagicFX/GrimMagicFXTypes.h"
#include "GrimSpellPoseAnimInstance.generated.h"

/**
 * 시전 동작을 뼈에 입히는 프록시. 애님 그래프(이동 · 점프)를 먼저 평가한 뒤
 * 컴포넌트 공간에서 오른팔(완드) · 왼팔(책 받치기) · 상체를 조준 방향으로 돌린다 — 다리는 그래프 그대로라 움직이며 시전한다.
 * 뼈 축을 모르는 채로 「이 뼈 → 자식 뼈」 방향을 목표 방향으로 돌리므로 마네킹이 바뀌어도 뼈 이름만 맞으면 된다.
 */
struct GRIMOIRE_API FGrimSpellPoseAnimInstanceProxy : public FAnimInstanceProxy
{
	FGrimSpellPoseAnimInstanceProxy() = default;
	explicit FGrimSpellPoseAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:
	void ApplySpellPose(FPoseContext& Output) const;

	FGrimSpellPoseFrame Frame;
	FName RightUpperArm, RightForearm, RightHand, LeftUpperArm, LeftForearm, LeftHand, Spine;
	bool bEnabled = true;
};

/**
 * 플레이어 애님 인스턴스의 C++ 부모 (R21 시전 동작 · 책 받치기).
 * 애님 BP(ABP_GrimPlayer — ABP_Unarmed 를 복제해 이 클래스로 부모를 바꾼 것)가 이동 그래프를 그대로 갖고,
 * 이 클래스가 그 위에 상체 시전 동작을 덧입힌다. 정식 몽타주가 생기면 bProceduralSpellPose 를 끄고 그래프에서 슬롯 · 레이어로 바꾼다.
 * 트리거는 시전 GameplayCue(MagicFX 큐 처리기) — 시전자는 예측으로, 다른 사람은 큐 복제로 같은 동작을 본다.
 */
UCLASS(Transient, Blueprintable)
class GRIMOIRE_API UGrimSpellPoseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** 한 번 하는 시전 동작(튕기기 · 쓸기 · 짧은 뻗기). */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|SpellPose")
	void PlayCastMotion(EGrimCastMotion Motion);

	/** 누르는 동안 뻗고 유지(Channel 큐 Add). */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|SpellPose")
	void BeginHoldMotion(EGrimCastMotion Motion);

	/** 놓음(Channel 큐 Remove). */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|SpellPose")
	void EndHoldMotion();

	const FGrimSpellPoseFrame& GetPoseFrame() const { return PoseFrame; }

	/** 그래프에서도 쓸 수 있게 내놓는 값 */
	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|SpellPose")
	EGrimCastMotion CurrentMotion = EGrimCastMotion::Auto;

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|SpellPose")
	float RightArmWeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Grimoire|SpellPose")
	float BookHoldWeight = 0.f;

	/** 코드로 뼈를 돌리는 임시 시전 동작을 쓸지. 몽타주로 바꾸면 끈다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grimoire|SpellPose")
	bool bProceduralSpellPose = true;

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName RightUpperArmBone = TEXT("upperarm_r");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName RightForearmBone = TEXT("lowerarm_r");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName RightHandBone = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName LeftUpperArmBone = TEXT("upperarm_l");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName LeftForearmBone = TEXT("lowerarm_l");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName LeftHandBone = TEXT("hand_l");

	UPROPERTY(EditDefaultsOnly, Category = "Grimoire|SpellPose|Bones")
	FName SpineBone = TEXT("spine_03");

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	friend struct FGrimSpellPoseAnimInstanceProxy;

	float MotionElapsed = 0.f;
	float ReleaseElapsed = -1.f;
	bool bHolding = false;
	FGrimSpellPoseFrame PoseFrame;
};
