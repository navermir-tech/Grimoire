#include "MagicFX/GrimSpellPoseAnimInstance.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "Animation/AnimNodeBase.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "GameFramework/Pawn.h"

// ---- 애님 인스턴스 (게임 스레드) ----

FAnimInstanceProxy* UGrimSpellPoseAnimInstance::CreateAnimInstanceProxy()
{
	return new FGrimSpellPoseAnimInstanceProxy(this);
}

void UGrimSpellPoseAnimInstance::PlayCastMotion(EGrimCastMotion Motion)
{
	if (Motion == EGrimCastMotion::Auto)
	{
		return;
	}
	// 줄기를 이미 누르고 있으면(Channel 큐가 먼저 왔으면) 뻗은 자세를 다시 시작하지 않는다.
	if (Motion == EGrimCastMotion::Extend && CurrentMotion == EGrimCastMotion::Extend && bHolding)
	{
		return;
	}
	CurrentMotion = Motion;
	MotionElapsed = 0.f;
	ReleaseElapsed = -1.f;
	bHolding = false;
}

void UGrimSpellPoseAnimInstance::BeginHoldMotion(EGrimCastMotion Motion)
{
	const EGrimCastMotion HoldMotion = Motion == EGrimCastMotion::Auto ? EGrimCastMotion::Extend : Motion;
	if (CurrentMotion != HoldMotion)
	{
		CurrentMotion = HoldMotion;
		MotionElapsed = 0.f;
	}
	ReleaseElapsed = -1.f;
	bHolding = true;
}

void UGrimSpellPoseAnimInstance::EndHoldMotion()
{
	bHolding = false;
	if (CurrentMotion == EGrimCastMotion::Extend && ReleaseElapsed < 0.f && MotionElapsed >= GrimMagicFXMath::ExtendMinHoldSeconds)
	{
		ReleaseElapsed = 0.f;
	}
}

void UGrimSpellPoseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (CurrentMotion != EGrimCastMotion::Auto)
	{
		MotionElapsed += DeltaSeconds;
		if (ReleaseElapsed >= 0.f)
		{
			ReleaseElapsed += DeltaSeconds;
		}
		// 줄기 큐 없이 뻗기만 한 경우(짧은 시전)는 최소 시간 뒤 스스로 놓는다.
		if (CurrentMotion == EGrimCastMotion::Extend && !bHolding && ReleaseElapsed < 0.f && MotionElapsed >= GrimMagicFXMath::ExtendMinHoldSeconds)
		{
			ReleaseElapsed = 0.f;
		}
		if (GrimMagicFXMath::IsCastMotionFinished(CurrentMotion, MotionElapsed, ReleaseElapsed))
		{
			CurrentMotion = EGrimCastMotion::Auto;
		}
	}

	const APawn* Pawn = TryGetPawnOwner();
	const float AimPitch = Pawn ? FRotator::NormalizeAxis(Pawn->GetBaseAimRotation().Pitch) : 0.f;
	PoseFrame = CurrentMotion != EGrimCastMotion::Auto
		? GrimMagicFXMath::EvaluateCastPose(CurrentMotion, MotionElapsed, ReleaseElapsed, AimPitch)
		: FGrimSpellPoseFrame();

	// 책이 손 위에 떠 있는 만큼 왼팔이 받친다.
	const AGrimGrimoireBook* Book = UGrimMagicFXStatics::FindGrimoireBook(Pawn);
	BookHoldWeight = Book ? 1.f - Book->GetStowAlpha() : 0.f;
	GrimMagicFXMath::ApplyBookHoldPose(PoseFrame, BookHoldWeight);
	RightArmWeight = PoseFrame.RightArmWeight;
}

// ---- 프록시 (애님 워커 스레드) ----

void FGrimSpellPoseAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	if (const UGrimSpellPoseAnimInstance* Instance = Cast<UGrimSpellPoseAnimInstance>(InAnimInstance))
	{
		Frame = Instance->PoseFrame;
		bEnabled = Instance->bProceduralSpellPose;
		RightUpperArm = Instance->RightUpperArmBone;
		RightForearm = Instance->RightForearmBone;
		RightHand = Instance->RightHandBone;
		LeftUpperArm = Instance->LeftUpperArmBone;
		LeftForearm = Instance->LeftForearmBone;
		LeftHand = Instance->LeftHandBone;
		Spine = Instance->SpineBone;
	}
}

bool FGrimSpellPoseAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	// 그래프(이동 · 점프)를 먼저 평가한다. 메인 그래프일 때만 시전 자세를 덧입힌다(레이어 그래프는 그대로).
	EvaluateAnimationNode_WithRoot(Output, InRootNode);
	if (bEnabled && InRootNode == GetRootNode())
	{
		ApplySpellPose(Output);
	}
	return true;
}

namespace
{
	constexpr float MinWeight = 0.001f;
}

void FGrimSpellPoseAnimInstanceProxy::ApplySpellPose(FPoseContext& Output) const
{
	const bool bSpine = FMath::Abs(Frame.SpineYawDegrees) > 0.01f || FMath::Abs(Frame.SpinePitchDegrees) > 0.01f;
	if (Frame.RightArmWeight < MinWeight && Frame.LeftHoldWeight < MinWeight && !bSpine)
	{
		return;
	}

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	auto Find = [&Bones](FName Name) -> FCompactPoseBoneIndex
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.IsValidToEvaluate(Bones) ? Reference.GetCompactPoseIndex(Bones) : FCompactPoseBoneIndex(INDEX_NONE);
	};

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);

	auto Rotate = [&Pose](FCompactPoseBoneIndex Bone, const FQuat& Delta)
	{
		if (Bone == INDEX_NONE)
		{
			return;
		}
		FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
		Transform.SetRotation((Delta * Transform.GetRotation()).GetNormalized());
		const FBoneTransform Result(Bone, Transform);
		Pose.SafeSetCSBoneTransforms(MakeArrayView(&Result, 1));
	};

	// 뼈 → 자식 뼈 방향을 목표 방향 쪽으로 Weight 만큼 돌린다(뼈 축을 몰라도 된다).
	auto Aim = [&Pose, &Rotate](FCompactPoseBoneIndex Bone, FCompactPoseBoneIndex Child, const FVector& Target, float Weight)
	{
		if (Bone == INDEX_NONE || Child == INDEX_NONE || Weight < MinWeight)
		{
			return;
		}
		const FVector From = Pose.GetComponentSpaceTransform(Bone).GetLocation();
		FVector Current = Pose.GetComponentSpaceTransform(Child).GetLocation() - From;
		const FVector Desired = Target.GetSafeNormal();
		if (!Current.Normalize() || Desired.IsNearlyZero())
		{
			return;
		}
		const FQuat Delta = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Current, Desired), FMath::Clamp(Weight, 0.f, 1.f));
		Rotate(Bone, Delta);
	};

	// 부모 → 자식 순서: 상체 → 왼팔 → 오른팔.
	if (bSpine)
	{
		// 비틀기는 위(+Z) 축, 숙이기는 왼쪽(+X) 축으로 앞(+Y)을 아래로.
		const FQuat Twist(FVector(0.f, 0.f, 1.f), FMath::DegreesToRadians(Frame.SpineYawDegrees));
		const FQuat Lean(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(-Frame.SpinePitchDegrees));
		Rotate(Find(Spine), Twist * Lean);
	}

	if (Frame.LeftHoldWeight >= MinWeight)
	{
		const FCompactPoseBoneIndex Upper = Find(LeftUpperArm);
		const FCompactPoseBoneIndex Fore = Find(LeftForearm);
		Aim(Upper, Fore, Frame.LeftUpperArmDir, Frame.LeftHoldWeight);
		Aim(Fore, Find(LeftHand), Frame.LeftForearmDir, Frame.LeftHoldWeight);
	}

	if (Frame.RightArmWeight >= MinWeight)
	{
		const FCompactPoseBoneIndex Upper = Find(RightUpperArm);
		const FCompactPoseBoneIndex Fore = Find(RightForearm);
		Aim(Upper, Fore, Frame.RightUpperArmDir, Frame.RightArmWeight);
		Aim(Fore, Find(RightHand), Frame.RightForearmDir, Frame.RightArmWeight);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
}
