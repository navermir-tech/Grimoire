#include "Misc/AutomationTest.h"
#include "MagicFX/GrimBookSigils.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "Magic/GrimMagicTags.h"
#include "MagicFX/GrimMagicFXVisualSet.h"
#include "NiagaraSystem.h"
#include "NiagaraEffectType.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "Core/GrimGameplayTags.h"
#include "Magic/GrimSpellDefinition.h"
#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimMagicFXSettings.h"
#include "MagicFX/GrimSpellChannelCue.h"
#include "MagicFX/GrimSpellCueNotifies.h"
#include "MagicFX/GrimSpellPoseAnimInstance.h"
#include "Character/GrimPlayerCharacter.h"
#include "Core/GrimSpellFXTypes.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags GrimMagicFXTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookFloatTest, "Grimoire.MagicFX.Book.Float", GrimMagicFXTestFlags)
bool FGrimBookFloatTest::RunTest(const FString& Parameters)
{
	FGrimBookFloatParams Params;
	Params.BobAmplitude = 1.5f;
	Params.BobPeriod = 2.f;
	Params.TiltDegrees = 3.f;

	// 시작은 가운데, 1/4 주기에 꼭대기, 한 주기 뒤 제자리.
	TestEqual(TEXT("0초 = 가운데"), GrimMagicFXMath::CalcBookBobOffset(0.f, Params).Z, 0.0, 1e-4);
	TestEqual(TEXT("0.5초 = 꼭대기"), GrimMagicFXMath::CalcBookBobOffset(0.5f, Params).Z, 1.5, 1e-4);
	TestEqual(TEXT("주기 반복"), GrimMagicFXMath::CalcBookBobOffset(2.3f, Params).Z, GrimMagicFXMath::CalcBookBobOffset(0.3f, Params).Z, 1e-3);
	TestTrue(TEXT("위아래로만"), GrimMagicFXMath::CalcBookBobOffset(0.7f, Params).X == 0.f && GrimMagicFXMath::CalcBookBobOffset(0.7f, Params).Y == 0.f);

	// 기울기는 TiltDegrees 안에서, 요(yaw)는 건드리지 않는다.
	float MaxPitch = 0.f;
	for (float T = 0.f; T < 10.f; T += 0.05f)
	{
		const FRotator Tilt = GrimMagicFXMath::CalcBookBobTilt(T, Params);
		MaxPitch = FMath::Max(MaxPitch, FMath::Abs(Tilt.Pitch));
		if (FMath::Abs(Tilt.Pitch) > 3.f + 1e-3f || FMath::Abs(Tilt.Roll) > 3.f + 1e-3f || Tilt.Yaw != 0.f)
		{
			AddError(FString::Printf(TEXT("기울기 범위 밖: t=%.2f %s"), T, *Tilt.ToString()));
			break;
		}
	}
	TestTrue(TEXT("실제로 기운다"), MaxPitch > 2.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookFlipTest, "Grimoire.MagicFX.Book.Flip", GrimMagicFXTestFlags)
bool FGrimBookFlipTest::RunTest(const FString& Parameters)
{
	// 진행: 0 → 1, 가운데는 절반, 0 이하 시간은 바로 끝.
	TestEqual(TEXT("시작"), GrimMagicFXMath::CalcFlipProgress(0.f, 0.25f), 0.f, 1e-5f);
	TestEqual(TEXT("가운데"), GrimMagicFXMath::CalcFlipProgress(0.125f, 0.25f), 0.5f, 1e-5f);
	TestEqual(TEXT("끝"), GrimMagicFXMath::CalcFlipProgress(0.3f, 0.25f), 1.f, 1e-5f);
	TestEqual(TEXT("시간 0"), GrimMagicFXMath::CalcFlipProgress(0.f, 0.f), 1.f, 1e-5f);

	// 판 가운데(오른쪽 면 +Y)가 책등을 축으로 위를 지나 왼쪽 면으로 간다.
	const FVector PageCenter(0.f, 7.f, 0.f);
	const FVector Start = GrimMagicFXMath::CalcFlipPageRotation(0.f, 1).RotateVector(PageCenter);
	const FVector Middle = GrimMagicFXMath::CalcFlipPageRotation(0.5f, 1).RotateVector(PageCenter);
	const FVector End = GrimMagicFXMath::CalcFlipPageRotation(1.f, 1).RotateVector(PageCenter);
	TestTrue(TEXT("시작은 오른쪽 면"), Start.Equals(PageCenter, 1e-3f));
	TestTrue(TEXT("가운데는 위로 들림"), Middle.Z > 6.9f && FMath::Abs(Middle.Y) < 1e-2f);
	TestTrue(TEXT("끝은 왼쪽 면"), End.Equals(FVector(0.f, -7.f, 0.f), 1e-3f));
	TestTrue(TEXT("거꾸로 넘기면 왼쪽에서 시작"), GrimMagicFXMath::CalcFlipPageRotation(0.f, -1).RotateVector(PageCenter).Equals(FVector(0.f, -7.f, 0.f), 1e-3f));
	TestTrue(TEXT("거꾸로 넘기면 오른쪽에서 끝"), GrimMagicFXMath::CalcFlipPageRotation(1.f, -1).RotateVector(PageCenter).Equals(PageCenter, 1e-3f));
	TestTrue(TEXT("거꾸로도 위로 들림"), GrimMagicFXMath::CalcFlipPageRotation(0.5f, -1).RotateVector(PageCenter).Z > 6.9f);

	// 글자 면 발광: 0 에서 올라 꼭대기 근처 Peak, 시간이 지나면 0.
	const float Duration = 0.45f;
	const float Peak = 8.f;
	TestEqual(TEXT("발광 시작"), GrimMagicFXMath::CalcFlipGlow(0.f, Duration, Peak), 0.f, 1e-4f);
	TestEqual(TEXT("발광 꼭대기"), GrimMagicFXMath::CalcFlipGlow(Duration * 0.15f, Duration, Peak), Peak, 1e-3f);
	TestEqual(TEXT("발광 끝"), GrimMagicFXMath::CalcFlipGlow(Duration, Duration, Peak), 0.f, 1e-4f);
	TestTrue(TEXT("꼭대기 뒤 줄어든다"), GrimMagicFXMath::CalcFlipGlow(0.2f, Duration, Peak) > GrimMagicFXMath::CalcFlipGlow(0.35f, Duration, Peak));
	TestEqual(TEXT("넘기기 전(음수)은 0"), GrimMagicFXMath::CalcFlipGlow(-0.1f, Duration, Peak), 0.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookFlipToTest, "Grimoire.MagicFX.Book.FlipTo", GrimMagicFXTestFlags)
bool FGrimBookFlipToTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("처음 고르면 넘기지 않음"), GrimMagicFXMath::CalcFlipToPageCount(INDEX_NONE, 2, 3, 12), 0);
	TestEqual(TEXT("같은 슬롯"), GrimMagicFXMath::CalcFlipToPageCount(1, 1, 3, 12), 0);
	TestEqual(TEXT("앞으로 두 칸 = 6장"), GrimMagicFXMath::CalcFlipToPageCount(0, 2, 3, 12), 6);
	TestEqual(TEXT("뒤로 두 칸 = −6장"), GrimMagicFXMath::CalcFlipToPageCount(2, 0, 3, 12), -6);
	TestEqual(TEXT("최대 장 수"), GrimMagicFXMath::CalcFlipToPageCount(0, 9, 3, 12), 12);
	TestEqual(TEXT("최대 장 수(뒤)"), GrimMagicFXMath::CalcFlipToPageCount(9, 0, 3, 12), -12);

	// 촤르륵: 전체 0.5초를 장 수로 나누되 한 장 넘김 시간을 넘지 않는다.
	TestEqual(TEXT("6장 = 장당 0.083초"), GrimMagicFXMath::CalcRapidFlipDuration(6, 0.5f, 0.04f, 0.25f), 0.5f / 6.f, 1e-4f);
	TestEqual(TEXT("1장은 한 장 시간까지만"), GrimMagicFXMath::CalcRapidFlipDuration(1, 0.5f, 0.04f, 0.25f), 0.25f, 1e-4f);
	TestEqual(TEXT("많으면 최소 시간"), GrimMagicFXMath::CalcRapidFlipDuration(-40, 0.5f, 0.04f, 0.25f), 0.04f, 1e-4f);

	// 누르는 동안 초당 2장.
	TestEqual(TEXT("초당 2장 = 0.5초 간격"), GrimMagicFXMath::CalcChannelFlipInterval(2.f), 0.5f, 1e-5f);
	TestEqual(TEXT("0 = 넘기지 않음"), GrimMagicFXMath::CalcChannelFlipInterval(0.f), 0.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookStowTest, "Grimoire.MagicFX.Book.Stow", GrimMagicFXTestFlags)
bool FGrimBookStowTest::RunTest(const FString& Parameters)
{
	FGrimGrimoireStowQuery Base;
	Base.MountedCombatHoldSeconds = 4.f;
	TestFalse(TEXT("평소에는 띄운다"), GrimMagicFXMath::ShouldStowGrimoire(Base));

	FGrimGrimoireStowQuery Query = Base;
	Query.bHasWand = false;
	TestTrue(TEXT("완드 없음 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	Query = Base;
	Query.bSwimming = true;
	TestTrue(TEXT("헤엄 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	Query = Base;
	Query.bBuildMode = true;
	TestTrue(TEXT("건축 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	Query = Base;
	Query.bCarrying = true;
	TestTrue(TEXT("나르기 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	// 탈것: 그냥 달리면 거두고, 조준 중이거나 최근 4초 안에 시전했으면 띄운다.
	Query = Base;
	Query.bMounted = true;
	Query.SecondsSinceCombat = 30.f;
	TestTrue(TEXT("탈것 · 전투 없음 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));
	Query.SecondsSinceCombat = 2.f;
	TestFalse(TEXT("탈것 · 2초 전 시전 → 띄움"), GrimMagicFXMath::ShouldStowGrimoire(Query));
	Query.SecondsSinceCombat = 4.5f;
	TestTrue(TEXT("탈것 · 4.5초 전 시전 → 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));
	Query.bAiming = true;
	TestFalse(TEXT("탈것 · 조준 중 → 띄움"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	// 탈것이어도 헤엄 · 완드 없음이 먼저.
	Query.bSwimming = true;
	TestTrue(TEXT("탈것 · 조준 중이어도 헤엄이면 거둠"), GrimMagicFXMath::ShouldStowGrimoire(Query));

	// 보간: 0.3초에 걸쳐 0 → 1, 넘치지 않는다.
	float Alpha = 0.f;
	for (int32 Step = 0; Step < 3; ++Step)
	{
		Alpha = GrimMagicFXMath::StepStowAlpha(Alpha, true, 0.1f, 0.3f);
	}
	TestEqual(TEXT("0.3초 뒤 허리"), Alpha, 1.f, 1e-4f);
	TestEqual(TEXT("넘치지 않음"), GrimMagicFXMath::StepStowAlpha(Alpha, true, 0.1f, 0.3f), 1.f, 1e-5f);
	TestEqual(TEXT("되돌리기 한 걸음"), GrimMagicFXMath::StepStowAlpha(1.f, false, 0.15f, 0.3f), 0.5f, 1e-4f);
	TestEqual(TEXT("보간 시간 0 = 바로"), GrimMagicFXMath::StepStowAlpha(0.f, true, 0.01f, 0.f), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCastMotionResolveTest, "Grimoire.MagicFX.CastMotion.Resolve", GrimMagicFXTestFlags)
bool FGrimCastMotionResolveTest::RunTest(const FString& Parameters)
{
	// 전달 방식 이름 → 동작 (갈래 A 의 새 이름 · M1 이름 모두).
	TestEqual(TEXT("Bolt = 튕기기"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Bolt")), EGrimCastMotion::Flick);
	TestEqual(TEXT("Projectile = 튕기기"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Projectile")), EGrimCastMotion::Flick);
	TestEqual(TEXT("Stream = 뻗고 유지"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Stream")), EGrimCastMotion::Extend);
	TestEqual(TEXT("Cone = 쓸기"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Cone")), EGrimCastMotion::Sweep);
	TestEqual(TEXT("Beam = 쓸기"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Beam")), EGrimCastMotion::Sweep);
	TestEqual(TEXT("열거형 접두 무시"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("EGrimSpellDelivery::Stream")), EGrimCastMotion::Extend);
	TestEqual(TEXT("모르는 이름 = Auto"), GrimMagicFXMath::CastMotionFromDeliveryName(TEXT("Teleport")), EGrimCastMotion::Auto);

	TestEqual(TEXT("물 = 뻗기"), GrimMagicFXMath::CastMotionFromElementName(TEXT("Element.Water")), EGrimCastMotion::Extend);
	TestEqual(TEXT("바람 = 쓸기"), GrimMagicFXMath::CastMotionFromElementName(TEXT("Element.Wind")), EGrimCastMotion::Sweep);
	TestEqual(TEXT("불 = 튕기기"), GrimMagicFXMath::CastMotionFromElementName(TEXT("Element.Fire")), EGrimCastMotion::Flick);
	TestEqual(TEXT("없음 = 튕기기"), GrimMagicFXMath::CastMotionFromElementName(NAME_None), EGrimCastMotion::Flick);

	// 큐 파라미터에서: VisualSet 지정이 먼저, 없으면 정의의 전달 방식.
	UGrimMagicFXVisualSet* VisualSet = NewObject<UGrimMagicFXVisualSet>();
	VisualSet->ElementColor = FLinearColor(0.1f, 0.4f, 0.9f);
	VisualSet->CastMotion = EGrimCastMotion::Sweep;

	FGameplayCueParameters CueParams;
	CueParams.SourceObject = VisualSet;
	TestEqual(TEXT("SourceObject 가 VisualSet 이면 그대로"), UGrimMagicFXStatics::FindVisualSetOnObject(VisualSet), static_cast<UGrimSpellVisualSet*>(VisualSet));
	TestEqual(TEXT("VisualSet 지정 동작"), UGrimMagicFXStatics::ResolveCastMotion(VisualSet, CueParams), EGrimCastMotion::Sweep);
	TestTrue(TEXT("VisualSet 색"), UGrimMagicFXStatics::ResolveColor(VisualSet, CueParams).Equals(FLinearColor(0.1f, 0.4f, 0.9f)));

	UGrimSpellDefinition* Definition = NewObject<UGrimSpellDefinition>();
	Definition->Element = GrimTags::Element_Water;
	Definition->Color = FLinearColor(0.2f, 0.3f, 0.4f);
	FGameplayCueParameters DefinitionParams;
	DefinitionParams.SourceObject = Definition;
	// 기본 전달 방식(M1 Projectile · M1.5 Bolt)은 튕기기 — 속성(물)보다 전달 방식이 먼저.
	TestEqual(TEXT("정의의 기본 전달 방식 = 튕기기"), UGrimMagicFXStatics::ResolveCastMotion(nullptr, DefinitionParams), EGrimCastMotion::Flick);
	TestTrue(TEXT("VisualSet 없으면 정의 색"), UGrimMagicFXStatics::ResolveColor(nullptr, DefinitionParams).Equals(FLinearColor(0.2f, 0.3f, 0.4f)));
	TestEqual(TEXT("정의의 속성"), UGrimMagicFXStatics::FindElementTag(DefinitionParams), GrimTags::Element_Water.GetTag());
	TestNull(TEXT("빈 정의의 VisualSet 은 없음"), UGrimMagicFXStatics::FindVisualSetOnObject(Definition));

	// 큐 태그 묶음의 Element.* 가 정의보다 먼저.
	DefinitionParams.AggregatedSourceTags.AddTag(GrimTags::Element_Fire);
	TestEqual(TEXT("태그 묶음의 속성"), UGrimMagicFXStatics::FindElementTag(DefinitionParams), GrimTags::Element_Fire.GetTag());

	// 정의의 float 필드를 이름으로(갈래 A 의 Stream · Cone 필드를 읽는 방법).
	Definition->Range = 1234.f;
	TestEqual(TEXT("이름으로 읽기"), UGrimMagicFXStatics::ReadFloatProperty(Definition, TEXT("Range"), 0.f), 1234.f, 1e-3f);
	TestEqual(TEXT("없는 필드는 기본값"), UGrimMagicFXStatics::ReadFloatProperty(Definition, TEXT("NoSuchField"), 7.f), 7.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCastPoseTest, "Grimoire.MagicFX.CastMotion.Pose", GrimMagicFXTestFlags)
bool FGrimCastPoseTest::RunTest(const FString& Parameters)
{
	// 축: 앞 +Y · 위 +Z · 오른쪽 −X (마네킹 컴포넌트 공간).
	TestTrue(TEXT("조준 90° 위 = +Z"), GrimMagicFXMath::ApplyAimPitch(FVector(0.f, 1.f, 0.f), 90.f).Equals(FVector(0.f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("몸 90° 오른쪽 = −X"), GrimMagicFXMath::ApplyBodyYaw(FVector(0.f, 1.f, 0.f), 90.f).Equals(FVector(-1.f, 0.f, 0.f), 1e-4f));

	// 튕기기: 0 에서 시작해 금방 다 들고, 0.45초에 끝.
	TestEqual(TEXT("튕기기 시작 무게"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.f, -1.f, 0.f).RightArmWeight, 0.f, 1e-4f);
	TestEqual(TEXT("튕기기 0.1초 무게"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.1f, -1.f, 0.f).RightArmWeight, 1.f, 1e-4f);
	TestEqual(TEXT("튕기기 끝 무게"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.45f, -1.f, 0.f).RightArmWeight, 0.f, 1e-4f);
	TestTrue(TEXT("튕기기 끝남"), GrimMagicFXMath::IsCastMotionFinished(EGrimCastMotion::Flick, 0.46f, -1.f));
	const FGrimSpellPoseFrame Flick = GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.15f, -1.f, 0.f);
	TestTrue(TEXT("튕긴 아래팔은 앞으로"), Flick.RightForearmDir.Y > 0.9f);
	TestTrue(TEXT("방향은 단위 벡터"), FMath::IsNearlyEqual(Flick.RightUpperArmDir.Size(), 1.f, 1e-3f) && FMath::IsNearlyEqual(Flick.RightForearmDir.Size(), 1.f, 1e-3f));
	TestTrue(TEXT("젖힌 손목은 위로"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.02f, -1.f, 0.f).RightForearmDir.Z > 0.5f);

	// 위를 겨누면 팔도 든다.
	TestTrue(TEXT("조준 높이 반영"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Flick, 0.15f, -1.f, 30.f).RightForearmDir.Z > Flick.RightForearmDir.Z + 0.3f);

	// 뻗고 유지: 놓기 전엔 계속 1, 놓으면 0.25초에 0.
	TestEqual(TEXT("뻗고 유지 2초"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Extend, 2.f, -1.f, 0.f).RightArmWeight, 1.f, 1e-4f);
	TestFalse(TEXT("놓기 전엔 안 끝남"), GrimMagicFXMath::IsCastMotionFinished(EGrimCastMotion::Extend, 5.f, -1.f));
	TestEqual(TEXT("놓고 0.25초"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Extend, 2.25f, 0.25f, 0.f).RightArmWeight, 0.f, 1e-4f);
	TestTrue(TEXT("놓으면 끝남"), GrimMagicFXMath::IsCastMotionFinished(EGrimCastMotion::Extend, 2.25f, 0.25f));
	TestTrue(TEXT("뻗은 팔은 앞으로"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Extend, 1.f, -1.f, 0.f).RightUpperArmDir.Y > 0.95f);

	// 쓸기: 오른쪽(−X)에서 시작해 왼쪽(+X)으로.
	const FGrimSpellPoseFrame SweepStart = GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Sweep, 0.04f, -1.f, 0.f);
	const FGrimSpellPoseFrame SweepEndPose = GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Sweep, 0.32f, -1.f, 0.f);
	TestTrue(TEXT("쓸기 시작은 오른쪽"), SweepStart.RightForearmDir.X < -0.5f);
	TestTrue(TEXT("쓸기 끝은 왼쪽"), SweepEndPose.RightForearmDir.X > 0.5f);
	TestEqual(TEXT("쓸기 끝 무게"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Sweep, 0.55f, -1.f, 0.f).RightArmWeight, 0.f, 1e-4f);

	// 동작 없음 = 아무것도 안 돌림.
	TestEqual(TEXT("Auto 무게 0"), GrimMagicFXMath::EvaluateCastPose(EGrimCastMotion::Auto, 0.1f, -1.f, 0.f).RightArmWeight, 0.f, 1e-5f);

	// 책 받치기: 아래팔이 앞으로, 무게는 0~1.
	FGrimSpellPoseFrame Hold;
	GrimMagicFXMath::ApplyBookHoldPose(Hold, 1.5f);
	TestEqual(TEXT("받치기 무게 자름"), Hold.LeftHoldWeight, 1.f, 1e-5f);
	TestTrue(TEXT("받치는 아래팔은 앞으로"), Hold.LeftForearmDir.Y > 0.8f);
	TestTrue(TEXT("위팔은 아래로"), Hold.LeftUpperArmDir.Z < -0.7f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimStreamShapeTest, "Grimoire.MagicFX.Stream.Shape", GrimMagicFXTestFlags)
bool FGrimStreamShapeTest::RunTest(const FString& Parameters)
{
	// 포물선: 수평으로 1000cm/s, 중력 −980 → 1초 뒤 앞 1000 · 아래 490.
	const FVector Point = GrimMagicFXMath::CalcStreamPoint(FVector::ZeroVector, FVector(1.f, 0.f, 0.f), 1000.f, -980.f, 1.f);
	TestTrue(TEXT("포물선 1초"), Point.Equals(FVector(1000.f, 0.f, -490.f), 1e-2f));
	TestTrue(TEXT("0초 = 시작점"), GrimMagicFXMath::CalcStreamPoint(FVector(5.f), FVector(0.f, 1.f, 0.f), 1000.f, -980.f, 0.f).Equals(FVector(5.f), 1e-4f));

	// 원뿔 파동: 사거리 600 · 반각 45° → 끝 지름 1200, 길이 600.
	const FVector Cone = GrimMagicFXMath::CalcConeWaveSize(600.f, 45.f);
	TestEqual(TEXT("원뿔 지름"), Cone.X, 1200.0, 0.5);
	TestEqual(TEXT("원뿔 길이"), Cone.Z, 600.0, 1e-3);

	// 상태 태그 이름(거두기)이 등록돼 있다.
	TestEqual(TEXT("헤엄 태그 이름"), GrimMagicFXTags::State_Swimming.GetTag().GetTagName(), FName(TEXT("State.Swimming")));
	TestEqual(TEXT("탈것 태그 이름"), GrimMagicFXTags::State_Mounted.GetTag().GetTagName(), FName(TEXT("State.Mounted")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimMagicFXAssetsTest, "Grimoire.MagicFX.Assets", GrimMagicFXTestFlags)
bool FGrimMagicFXAssetsTest::RunTest(const FString& Parameters)
{
	// 설정(DefaultGame.ini)이 속성마다 연출 묶음을 가리키고, 셋의 색이 멀리서도 구분된다.
	const UGrimMagicFXSettings* Settings = UGrimMagicFXSettings::Get();
	TArray<FLinearColor> Colors;
	for (const FGameplayTag Element : { GrimTags::Element_Fire.GetTag(), GrimTags::Element_Water.GetTag(), GrimTags::Element_Wind.GetTag() })
	{
		const TSoftObjectPtr<UGrimSpellVisualSet>* Entry = Settings->DefaultVisualSets.Find(Element);
		const UGrimMagicFXVisualSet* VisualSet = Entry ? Cast<UGrimMagicFXVisualSet>(Entry->LoadSynchronous()) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s 연출 묶음"), *Element.ToString()), VisualSet))
		{
			continue;
		}
		TestNotNull(*FString::Printf(TEXT("%s 섬광"), *Element.ToString()), VisualSet->MuzzleFlash.Get());
		TestNotNull(*FString::Printf(TEXT("%s 터짐"), *Element.ToString()), VisualSet->Impact.Get());
		TestNotNull(*FString::Printf(TEXT("%s 흔들림"), *Element.ToString()), VisualSet->CameraShake.Get());
		Colors.Add(VisualSet->ElementColor);
	}
	for (int32 A = 0; A < Colors.Num(); ++A)
	{
		for (int32 B = A + 1; B < Colors.Num(); ++B)
		{
			const float Distance = FVector(Colors[A].R - Colors[B].R, Colors[A].G - Colors[B].G, Colors[A].B - Colors[B].B).Size();
			TestTrue(*FString::Printf(TEXT("속성색 %d · %d 구분"), A, B), Distance > 0.3f);
		}
	}
	// 연출 묶음의 Niagara 는 모두 마법 Effect Type 을 단다(성능 규칙 24 · BUG-006). 볼트 묶음은 발사 빛줄기(Q85)가 있다.
	for (const TCHAR* Name : { TEXT("VS_Ember"), TEXT("VS_WaterJet"), TEXT("VS_Breeze"), TEXT("VS_BasicAttack") })
	{
		const UGrimMagicFXVisualSet* VisualSet = LoadObject<UGrimMagicFXVisualSet>(nullptr, *FString::Printf(TEXT("/Game/Grimoire/MagicFX/VisualSets/%s.%s"), Name, Name));
		if (!TestNotNull(Name, VisualSet))
		{
			continue;
		}
		for (const UFXSystemAsset* Asset : { VisualSet->MuzzleFlash.Get(), VisualSet->Trail.Get(), VisualSet->Impact.Get(), VisualSet->LaunchStreak.Get() })
		{
			if (const UNiagaraSystem* System = Cast<UNiagaraSystem>(Asset))
			{
				TestEqual(*FString::Printf(TEXT("%s · %s Effect Type"), Name, *System->GetName()), GetNameSafe(System->GetEffectType()), FString(TEXT("EFT_Grim_Spell")));
			}
		}
		if (FCString::Strcmp(Name, TEXT("VS_Ember")) == 0 || FCString::Strcmp(Name, TEXT("VS_BasicAttack")) == 0)
		{
			TestNotNull(*FString::Printf(TEXT("%s 발사 빛줄기"), Name), VisualSet->LaunchStreak.Get());
			TestTrue(*FString::Printf(TEXT("%s 발사 빛줄기 시간"), Name), VisualSet->LaunchStreakSeconds > 0.f);
		}
	}

	TestNotNull(TEXT("펄스 머티리얼"), Settings->GlowMaterial.LoadSynchronous());
	TestNotNull(TEXT("책 머티리얼"), Settings->BookMaterial.LoadSynchronous());

	// 큐 처리기 에셋: 큐 관리자가 찾는 경로에 있고 태그가 맞다.
	TestTrue(TEXT("큐 경로에 /Game/Grimoire"), UAbilitySystemGlobals::Get().GetGameplayCueNotifyPaths().Contains(TEXT("/Game/Grimoire")));
	const UClass* CastClass = LoadClass<UGrimCueNotify_SpellCast>(nullptr, TEXT("/Game/Grimoire/MagicFX/Cues/GC_Spell_Cast.GC_Spell_Cast_C"));
	const UClass* ImpactClass = LoadClass<UGrimCueNotify_SpellImpact>(nullptr, TEXT("/Game/Grimoire/MagicFX/Cues/GC_Spell_Impact.GC_Spell_Impact_C"));
	const UClass* ChannelClass = LoadClass<AGrimCueNotify_SpellChannel>(nullptr, TEXT("/Game/Grimoire/MagicFX/Cues/GC_Spell_Channel.GC_Spell_Channel_C"));
	if (TestNotNull(TEXT("GC_Spell_Cast"), CastClass))
	{
		TestEqual(TEXT("Cast 태그"), CastClass->GetDefaultObject<UGrimCueNotify_SpellCast>()->GameplayCueTag, GrimCueTags::Spell_Cast.GetTag());
	}
	if (TestNotNull(TEXT("GC_Spell_Impact"), ImpactClass))
	{
		TestEqual(TEXT("Impact 태그"), ImpactClass->GetDefaultObject<UGrimCueNotify_SpellImpact>()->GameplayCueTag, GrimCueTags::Spell_Impact.GetTag());
	}
	if (TestNotNull(TEXT("GC_Spell_Channel"), ChannelClass))
	{
		TestEqual(TEXT("Channel 태그"), ChannelClass->GetDefaultObject<AGrimCueNotify_SpellChannel>()->GameplayCueTag, GrimCueTags::Spell_Channel.GetTag());
	}

	// 애님 BP: 시전 동작을 덧입히는 부모, 캐릭터 기본값이 그것을 쓴다.
	const UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Grimoire/MagicFX/Anim/ABP_GrimPlayer.ABP_GrimPlayer_C"));
	if (TestNotNull(TEXT("ABP_GrimPlayer"), AnimClass))
	{
		TestTrue(TEXT("부모 = UGrimSpellPoseAnimInstance"), AnimClass->IsChildOf(UGrimSpellPoseAnimInstance::StaticClass()));
		TestEqual(TEXT("캐릭터 애님 클래스"), GetDefault<AGrimPlayerCharacter>()->GetMesh()->GetAnimClass(), const_cast<UClass*>(AnimClass));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookActorTest, "Grimoire.MagicFX.Book.Actor", GrimMagicFXTestFlags)
bool FGrimBookActorTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimMagicFXBookTest"));
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	AGrimGrimoireBook* Book = World->SpawnActor<AGrimGrimoireBook>();
	if (TestNotNull(TEXT("책 스폰"), Book))
	{
		TestFalse(TEXT("처음엔 넘기지 않음"), Book->IsFlipping());

		Book->FlipPage(FLinearColor::Red);
		TestTrue(TEXT("넘기기 시작"), Book->IsFlipping());
		Book->Tick(0.1f);
		TestTrue(TEXT("0.1초엔 넘기는 중"), Book->IsFlipping());
		Book->Tick(0.2f);
		TestFalse(TEXT("0.3초엔 다 넘김"), Book->IsFlipping());

		// 촤르륵: 처음 고른 슬롯은 기억만, 다음부터 넘긴다.
		Book->FlipTo(0, FLinearColor::Blue);
		TestFalse(TEXT("처음 슬롯은 넘기지 않음"), Book->IsFlipping());
		Book->FlipTo(2, FLinearColor::Blue);
		TestTrue(TEXT("다른 슬롯이면 넘김"), Book->IsFlipping());
		TestEqual(TEXT("지금 슬롯"), Book->GetCurrentSlot(), 2);
		for (int32 Step = 0; Step < 20; ++Step)
		{
			Book->Tick(0.05f);
		}
		TestFalse(TEXT("1초 안에 다 넘김"), Book->IsFlipping());

		// 주인(완드)이 없으면 거둔다.
		TestTrue(TEXT("완드 없음 → 허리춤"), Book->GetStowAlpha() > 0.99f);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// 속성 문양 11개(R21): 이름 찾기(하위 태그 → 위 이름 → 무속성), ini 표, 조각 변환.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookSigilLookupTest, "Grimoire.MagicFX.Sigil.Lookup", GrimMagicFXTestFlags)
bool FGrimBookSigilLookupTest::RunTest(const FString& Parameters)
{
	TArray<FGrimBookSigil> Sigils;
	for (const TCHAR* Name : { TEXT("Element.None"), TEXT("Element.Water"), TEXT("Element.Water.Ice"), TEXT("Element.Light") })
	{
		FGrimBookSigil& Sigil = Sigils.AddDefaulted_GetRef();
		Sigil.Element = Name;
	}
	TestEqual(TEXT("같은 이름"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Water"), TEXT("Element.None")), 1);
	TestEqual(TEXT("하위가 따로 있으면 그것"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Water.Ice"), TEXT("Element.None")), 2);
	TestEqual(TEXT("하위가 없으면 위 이름"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Water.Liquid"), TEXT("Element.None")), 1);
	TestEqual(TEXT("빛 하위(번개류)"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Light.Radiance"), TEXT("Element.None")), 3);
	TestEqual(TEXT("점 경계가 아니면 안 맞음 → 무속성"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.WaterX"), TEXT("Element.None")), 0);
	TestEqual(TEXT("모르는 속성 → 무속성"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Fire"), TEXT("Element.None")), 0);
	TestEqual(TEXT("빈 태그 → 무속성"), GrimBookSigilMath::FindSigilIndex(Sigils, NAME_None, TEXT("Element.None")), 0);
	TestEqual(TEXT("대신할 것도 없으면 없음"), GrimBookSigilMath::FindSigilIndex(Sigils, TEXT("Element.Fire"), NAME_None), (int32)INDEX_NONE);

	// ini: 스펙의 11 속성이 모두 있고 서로 다른 모양, 조각은 칸 수 안.
	const UGrimBookSigilSettings* Settings = UGrimBookSigilSettings::Get();
	TestEqual(TEXT("문양 11개"), Settings->Sigils.Num(), 11);
	for (const TCHAR* Name : { TEXT("Element.None"), TEXT("Element.Water"), TEXT("Element.Water.Ice"), TEXT("Element.Fire"), TEXT("Element.Wind"),
		TEXT("Element.Earth"), TEXT("Element.Earth.Plant"), TEXT("Element.Light.Holy"), TEXT("Element.Light"), TEXT("Element.Dark"), TEXT("Element.Dark.Shadow") })
	{
		const FGrimBookSigil* Sigil = Settings->FindSigil(Name);
		if (TestNotNull(*FString::Printf(TEXT("문양 %s"), Name), Sigil))
		{
			TestEqual(*FString::Printf(TEXT("%s 정확히"), Name), Sigil->Element, FName(Name));
			TestTrue(*FString::Printf(TEXT("%s 조각 1~%d"), Name, GrimBookSigilMath::MaxPieces), Sigil->Pieces.Num() >= 1 && Sigil->Pieces.Num() <= GrimBookSigilMath::MaxPieces);
		}
	}
	TestEqual(TEXT("기본 공격(무속성) 문양"), Settings->FindSigil(GrimMagicTags::Element_None.GetTag().GetTagName()), Settings->FindSigil(TEXT("Element.None")));
	TestEqual(TEXT("흑마법(Hex) → 흑마법 문양"), Settings->FindSigil(TEXT("Element.Dark.Hex")), Settings->FindSigil(TEXT("Element.Dark")));

	// 조각 변환: 세모(Cone) 꼭짓점이 +X, 두께는 위로, 바닥이 면(0)에 닿는다.
	FGrimSigilPiece Triangle;
	Triangle.Shape = EGrimSigilShape::Triangle;
	Triangle.Size = FVector2D(4.f, 3.f);
	const FTransform T = GrimBookSigilMath::MakePieceTransform(Triangle, 0.2f);
	const FVector Tip = T.TransformPosition(FVector(0.f, 0.f, 50.f));
	TestEqual(TEXT("꼭짓점 +X 로 높이의 절반"), Tip.X, 2.0, 0.01);
	TestEqual(TEXT("가로 반지름 = 밑변 절반"), T.TransformPosition(FVector(0.f, 50.f, 0.f)).Y, 1.5, 0.01);
	TestEqual(TEXT("두께 가운데 높이"), T.GetLocation().Z, 0.1, 0.001);
	FGrimSigilPiece Bar;
	Bar.Size = FVector2D(1.f, 6.f);
	Bar.Yaw = 90.f;
	const FVector BarEnd = GrimBookSigilMath::MakePieceTransform(Bar, 0.2f).TransformPosition(FVector(0.f, 50.f, 0.f));
	TestEqual(TEXT("막대 90° 돌림 → 가로가 −X 쪽으로"), BarEnd.X, -3.0, 0.01);
	return true;
}

// 책: 시전 한 장 — 기본 공격 0.15초, 넘기는 판 뒷면 → 다 넘기면 펼친 면에 그 속성 문양.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBookSigilFlipTest, "Grimoire.MagicFX.Sigil.Flip", GrimMagicFXTestFlags)
bool FGrimBookSigilFlipTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimMagicFXSigilTest"));
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	const UGrimBookSigilSettings* Settings = UGrimBookSigilSettings::Get();
	const int32 Fire = GrimBookSigilMath::FindSigilIndex(Settings->Sigils, TEXT("Element.Fire"), Settings->FallbackElement);
	const int32 None = GrimBookSigilMath::FindSigilIndex(Settings->Sigils, TEXT("Element.None"), Settings->FallbackElement);

	AGrimGrimoireBook* Book = World->SpawnActor<AGrimGrimoireBook>();
	if (TestNotNull(TEXT("책 스폰"), Book))
	{
		TestEqual(TEXT("처음엔 문양 없음"), Book->GetShownSigilIndex(), (int32)INDEX_NONE);
		Book->FlipPageForSpell(FLinearColor::Red, GrimTags::Element_Fire, 0.f);
		TestEqual(TEXT("보통 마법 = 책 기본 넘김 시간"), Book->GetCurrentFlipDuration(), 0.25f, 0.001f);
		TestEqual(TEXT("넘기는 판에 불 문양"), Book->GetFlippingSigilIndex(), Fire);
		Book->Tick(0.3f);
		TestEqual(TEXT("다 넘기면 펼친 면에 불 문양"), Book->GetShownSigilIndex(), Fire);
		TestEqual(TEXT("넘기는 판은 비었다"), Book->GetFlippingSigilIndex(), (int32)INDEX_NONE);

		// 기본 공격: 무속성 문양, 0.15초(VisualSet PageFlipDuration)
		Book->FlipPageForSpell(FLinearColor::White, GrimMagicTags::Element_None, 0.15f);
		TestEqual(TEXT("기본 공격 넘김 0.15초"), Book->GetCurrentFlipDuration(), 0.15f, 0.001f);
		Book->Tick(0.1f);
		TestEqual(TEXT("넘기는 중엔 펼친 면은 앞 문양"), Book->GetShownSigilIndex(), Fire);
		Book->Tick(0.06f);
		TestFalse(TEXT("0.16초엔 다 넘김"), Book->IsFlipping());
		TestEqual(TEXT("펼친 면 = 무속성 문양"), Book->GetShownSigilIndex(), None);

		// 세트 바꾸기 촤르륵은 문양을 바꾸지 않는다.
		Book->FlipTo(0, FLinearColor::Blue);
		Book->FlipTo(2, FLinearColor::Blue);
		for (int32 Step = 0; Step < 20; ++Step)
		{
			Book->Tick(0.05f);
		}
		TestEqual(TEXT("촤르륵 뒤에도 무속성 문양"), Book->GetShownSigilIndex(), None);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
