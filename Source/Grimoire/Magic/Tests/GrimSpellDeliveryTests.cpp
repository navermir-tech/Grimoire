#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimSpellDelivery.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimMagicSettings.h"
#include "Core/GrimTargetingTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

namespace
{
	constexpr EAutomationTestFlags GrimDeliveryTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const UGrimSpellDefinition* LoadStartingSpell(const TCHAR* AssetName)
	{
		for (const TSoftObjectPtr<UGrimSpellDefinition>& Ref : UGrimMagicSettings::Get()->StartingSpells)
		{
			if (Ref.ToSoftObjectPath().GetAssetName() == AssetName)
			{
				return Ref.LoadSynchronous();
			}
		}
		return nullptr;
	}
}

// 시차 보정: 멀면 완드 끝 → 조준점, 2m 안이면 가슴 → 조준점, 조준점이 뒤면 시선 방향.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryParallaxTest, "Grimoire.Magic.Delivery.Parallax", GrimDeliveryTestFlags)
bool FGrimDeliveryParallaxTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	const FVector Muzzle(30.f, 25.f, 120.f);   // 오른손 완드 끝
	const FVector Chest(0.f, 0.f, 130.f);
	const FVector View(1.f, 0.f, 0.f);

	{
		const FVector Aim(2000.f, 0.f, 100.f);
		const FLaunch Launch = ComputeLaunch(Muzzle, Chest, Aim, View, 200.f);
		TestFalse(TEXT("먼 대상: 완드 끝에서"), Launch.bFromChest);
		TestTrue(TEXT("먼 대상: 시작 = 완드 끝"), Launch.Start.Equals(Muzzle));
		TestTrue(TEXT("먼 대상: 방향 = 완드 끝 → 조준점"), Launch.Direction.Equals((Aim - Muzzle).GetSafeNormal(), 1e-4f));
		// 시차: 카메라 방향(+X)이 아니라 조준점을 향해 안쪽(−Y)으로 꺾인다.
		TestTrue(TEXT("먼 대상: 화면 중앙 쪽으로 꺾임"), Launch.Direction.Y < 0.f);
	}
	{
		const FVector Aim(150.f, 0.f, 120.f);
		const FLaunch Launch = ComputeLaunch(Muzzle, Chest, Aim, View, 200.f);
		TestTrue(TEXT("2m 안: 가슴에서"), Launch.bFromChest);
		TestTrue(TEXT("2m 안: 시작 = 가슴"), Launch.Start.Equals(Chest));
		TestTrue(TEXT("2m 안: 방향 = 가슴 → 조준점"), Launch.Direction.Equals((Aim - Chest).GetSafeNormal(), 1e-4f));
	}
	{
		// 카메라와 완드 사이에 맞은 점(완드 끝 뒤)
		const FVector Aim(10.f, 25.f, 120.f);
		const FLaunch Launch = ComputeLaunch(Muzzle, Chest, Aim, View, 0.f);
		TestTrue(TEXT("조준점이 뒤면 시선 방향"), Launch.Direction.Equals(View, 1e-4f));
	}
	return true;
}

// 유도: 초당 회전 상한 · 속력 유지 · 크게 빗나가면 휘지 않음.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryHomingTest, "Grimoire.Magic.Delivery.Homing", GrimDeliveryTestFlags)
bool FGrimDeliveryHomingTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	const FVector Velocity(7500.f, 0.f, 0.f);
	const FVector Position = FVector::ZeroVector;

	{
		// 대상이 30° 옆. 60°/s × 0.1s = 6° 만 돈다.
		const FVector Target = FVector(1000.f, 0.f, 0.f).RotateAngleAxis(30.f, FVector::UpVector);
		const FVector NewVelocity = SteerTowards(Velocity, Position, Target, 60.f, 60.f, 0.1f);
		const float Turned = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(NewVelocity.GetSafeNormal(), Velocity.GetSafeNormal())));
		TestEqual(TEXT("회전 = 상한 × 시간"), Turned, 6.f, 0.05f);
		TestEqual(TEXT("속력 유지"), static_cast<float>(NewVelocity.Size()), 7500.f, 0.5f);
		TestTrue(TEXT("대상 쪽으로"), NewVelocity.Y > 0.f);
	}
	{
		// 남은 각이 상한보다 작으면 정확히 대상 쪽
		const FVector Target = FVector(1000.f, 0.f, 0.f).RotateAngleAxis(3.f, FVector::UpVector);
		const FVector NewVelocity = SteerTowards(Velocity, Position, Target, 60.f, 60.f, 0.1f);
		TestTrue(TEXT("넘어 돌지 않는다"), NewVelocity.GetSafeNormal().Equals(Target.GetSafeNormal(), 1e-3f));
	}
	{
		const FVector Behind(-1000.f, 100.f, 0.f);
		TestTrue(TEXT("뒤 대상은 휘지 않음(유턴 없음)"), SteerTowards(Velocity, Position, Behind, 60.f, 60.f, 0.1f).Equals(Velocity));
		TestTrue(TEXT("세기 0 = 유도 없음"), SteerTowards(Velocity, Position, FVector(1000.f, 500.f, 0.f), 0.f, 60.f, 0.1f).Equals(Velocity));
	}
	return true;
}

// 포물선: 점 개수 · 끝점 · 조준점을 지나는 낮은 호 · 닿지 않는 거리.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryStreamArcTest, "Grimoire.Magic.Delivery.StreamArc", GrimDeliveryTestFlags)
bool FGrimDeliveryStreamArcTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	const float GravityZ = -980.f;
	const float Speed = 1400.f;
	const FVector Start(0.f, 0.f, 120.f);

	TArray<FVector> Points;
	ComputeArcPoints(Start, FVector(Speed, 0.f, 0.f), GravityZ, 1.f, 8, Points);
	TestEqual(TEXT("구간 8 → 점 9"), Points.Num(), 9);
	TestTrue(TEXT("첫 점 = 시작"), Points[0].Equals(Start));
	TestTrue(TEXT("끝점 = 해석해"), Points.Last().Equals(Start + FVector(Speed, 0.f, 0.5f * GravityZ), 0.01f));

	const FVector Target(800.f, 300.f, 0.f);
	FVector Direction;
	TestTrue(TEXT("닿는 거리는 풀린다"), SolveLaunchDirection(Start, Target, Speed, GravityZ, Direction));
	TestTrue(TEXT("단위 벡터"), FMath::IsNearlyEqual(static_cast<float>(Direction.Size()), 1.f, 1e-4f));
	// 수평 거리를 갈 때의 높이가 대상 높이와 같아야 한다.
	const FVector Delta = Target - Start;
	const float Horizontal = FVector2D(Delta.X, Delta.Y).Size();
	const float FlatSpeed = Speed * FVector2D(Direction.X, Direction.Y).Size();
	const float T = Horizontal / FlatSpeed;
	const FVector Reached = Start + Direction * Speed * T + FVector(0.f, 0.f, 0.5f * GravityZ * T * T);
	TestTrue(*FString::Printf(TEXT("호가 조준점을 지난다 (%s)"), *Reached.ToCompactString()), Reached.Equals(Target, 1.f));
	TestTrue(TEXT("낮은 호: 45° 아래"), Direction.Z < FMath::Sin(UE_PI / 4.f));

	const FVector TooFar(10000.f, 0.f, 0.f);
	TestFalse(TEXT("닿지 않는 거리"), SolveLaunchDirection(Start, TooFar, Speed, GravityZ, Direction));
	TestEqual(TEXT("닿지 않으면 45°"), static_cast<float>(Direction.Z), FMath::Sin(UE_PI / 4.f), 1e-3f);

	TestTrue(TEXT("중력 0 = 직선"), SolveLaunchDirection(Start, Target, Speed, 0.f, Direction) && Direction.Equals((Target - Start).GetSafeNormal(), 1e-4f));

	// 줄기 연출: 호 위의 점까지 걸린 시간(수평 거리 ÷ 수평 속력), 수직이면 Fallback, 상한 Fallback.
	TestEqual(TEXT("호 시간 0.5초"), CalcArcTimeToPoint(Start, FVector(Speed, 0.f, 0.f), Points[4], 2.f), 0.5f, 1e-4f);
	TestEqual(TEXT("수직이면 Fallback"), CalcArcTimeToPoint(Start, FVector(0.f, 0.f, Speed), Target, 0.7f), 0.7f, 1e-4f);
	TestEqual(TEXT("Fallback 이 상한"), CalcArcTimeToPoint(Start, FVector(Speed, 0.f, 0.f), Points.Last(), 0.6f), 0.6f, 1e-4f);
	return true;
}

// 볼트 발사 빛줄기(Q85): 완드 끝부터 볼트 속력 × 시간, 당긴 만큼은 다 덮고, 맞을 곳에서 멈춘다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryLaunchStreakTest, "Grimoire.Magic.Delivery.LaunchStreak", GrimDeliveryTestFlags)
bool FGrimDeliveryLaunchStreakTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	TestEqual(TEXT("먼 표적: 7500 × 0.1 = 750"), CalcLaunchStreakLength(0.f, 7500.f, 0.1f, 0.f), 750.f, 1e-3f);
	TestEqual(TEXT("가까운 표적(4 m)에서 멈춤"), CalcLaunchStreakLength(0.f, 7500.f, 0.1f, 400.f), 400.f, 1e-3f);
	TestEqual(TEXT("원격 지연으로 당긴 11 m 는 다 덮는다"), CalcLaunchStreakLength(1100.f, 7500.f, 0.1f, 0.f), 1100.f, 1e-3f);
	TestEqual(TEXT("당겼어도 맞은 곳까지만"), CalcLaunchStreakLength(1100.f, 7500.f, 0.1f, 600.f), 600.f, 1e-3f);
	TestEqual(TEXT("시간 0 · 당김 0 = 긋지 않음"), CalcLaunchStreakLength(0.f, 7500.f, 0.f, 0.f), 0.f, 1e-6f);
	return true;
}

// 원뿔 판정: 각도 · 거리 경계.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryConeTest, "Grimoire.Magic.Delivery.Cone", GrimDeliveryTestFlags)
bool FGrimDeliveryConeTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	const FVector Origin = FVector::ZeroVector;
	const FVector Dir(1.f, 0.f, 0.f);

	TestTrue(TEXT("정면"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(500.f, 0.f, 0.f)));
	TestTrue(TEXT("29° 안"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(500.f, 0.f, 0.f).RotateAngleAxis(29.f, FVector::UpVector)));
	TestFalse(TEXT("31° 밖"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(500.f, 0.f, 0.f).RotateAngleAxis(31.f, FVector::UpVector)));
	TestFalse(TEXT("위로 31° 밖"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(500.f, 0.f, 0.f).RotateAngleAxis(-31.f, FVector::RightVector)));
	TestFalse(TEXT("사거리 밖"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(1001.f, 0.f, 0.f)));
	TestFalse(TEXT("뒤"), IsInCone(Origin, Dir, 30.f, 1000.f, FVector(-100.f, 0.f, 0.f)));
	TestTrue(TEXT("원점"), IsInCone(Origin, Dir, 30.f, 1000.f, Origin));
	return true;
}

// 거리별 세기: 곡선 + 미풍 에셋 값이 양초 규칙(바람 0.3~2.5 키움, 2.5 이상 꺼짐)에 맞는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryFalloffTest, "Grimoire.Magic.Delivery.Falloff", GrimDeliveryTestFlags)
bool FGrimDeliveryFalloffTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;
	FGrimConeFalloff Falloff;
	Falloff.NearDistance = 100.f;
	Falloff.NearIntensity = 3.f;
	Falloff.FarIntensity = 1.f;
	Falloff.Exponent = 1.f;

	TestEqual(TEXT("가까이 = Near"), ConeIntensityAt(Falloff, 1100.f, 50.f), 3.f);
	TestEqual(TEXT("NearDistance = Near"), ConeIntensityAt(Falloff, 1100.f, 100.f), 3.f);
	TestEqual(TEXT("가운데 = 평균"), ConeIntensityAt(Falloff, 1100.f, 600.f), 2.f, 1e-4f);
	TestEqual(TEXT("끝 = Far"), ConeIntensityAt(Falloff, 1100.f, 1100.f), 1.f, 1e-4f);
	TestEqual(TEXT("사거리 너머도 Far"), ConeIntensityAt(Falloff, 1100.f, 5000.f), 1.f, 1e-4f);
	Falloff.Exponent = 2.f;
	TestEqual(TEXT("지수 2: 가운데는 더 세다"), ConeIntensityAt(Falloff, 1100.f, 600.f), 2.5f, 1e-4f);

	const UGrimSpellDefinition* Breeze = LoadStartingSpell(TEXT("DA_Spell_Breeze"));
	if (TestNotNull(TEXT("DA_Spell_Breeze"), Breeze))
	{
		TestEqual(TEXT("미풍 = Cone"), Breeze->Delivery, EGrimSpellDelivery::Cone);
		const float Near = ConeIntensityAt(Breeze->ConeFalloff, Breeze->ConeRange, 140.f);
		const float Far = ConeIntensityAt(Breeze->ConeFalloff, Breeze->ConeRange, 600.f);
		const float Edge = ConeIntensityAt(Breeze->ConeFalloff, Breeze->ConeRange, Breeze->ConeRange);
		TestTrue(*FString::Printf(TEXT("바로 앞(1.4m) = 돌풍 ≥ 2.5 (%.2f)"), Near), Near >= 2.5f);
		TestTrue(*FString::Printf(TEXT("6m = 키움 0.3~2.5 (%.2f)"), Far), Far >= 0.3f && Far < 2.5f);
		TestTrue(*FString::Printf(TEXT("끝 = 키움 ≥ 0.3 (%.2f)"), Edge), Edge >= 0.3f);
	}
	return true;
}

// Stream 대상별 자극 주기 · 지연 보정 나누기 · TargetData 검증.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryNetworkTest, "Grimoire.Magic.Delivery.Network", GrimDeliveryTestFlags)
bool FGrimDeliveryNetworkTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellDelivery;

	// 자극 주기
	TestTrue(TEXT("처음 닿으면 바로"), IsStimulusDue(nullptr, 5.0, 1.f));
	const double Last = 5.0;
	TestFalse(TEXT("주기 전"), IsStimulusDue(&Last, 5.5, 1.f));
	TestTrue(TEXT("주기 지남"), IsStimulusDue(&Last, 6.0, 1.f));

	// 지연 보정: Windup 0.1
	{
		const FLagSplit Split = SplitLag(0.03f, 0.1f, 0.15f);
		TestEqual(TEXT("지연 < Windup: 남은 만큼 기다림"), Split.WaitSeconds, 0.07f, 1e-5f);
		TestEqual(TEXT("지연 < Windup: 당김 없음"), Split.AdvanceSeconds, 0.f);
	}
	{
		const FLagSplit Split = SplitLag(0.18f, 0.1f, 0.15f);
		TestEqual(TEXT("지연 > Windup: 기다림 없음"), Split.WaitSeconds, 0.f);
		TestEqual(TEXT("지연 > Windup: 넘은 만큼 당김"), Split.AdvanceSeconds, 0.08f, 1e-5f);
	}
	{
		const FLagSplit Split = SplitLag(0.5f, 0.f, 0.15f);
		TestEqual(TEXT("당김 상한 0.15"), Split.AdvanceSeconds, 0.15f, 1e-5f);
	}

	// TargetData 검증
	const FVector Pawn(0.f, 0.f, 90.f);
	const FVector Forward(1.f, 0.f, 0.f);
	const FVector Camera(-300.f, 50.f, 160.f);
	TestTrue(TEXT("정상 조준"), IsClientAimPlausible(Camera, FVector(1500.f, 0.f, 50.f), Pawn, Forward, 800.f, 60.f, 2800.f));
	TestFalse(TEXT("시선 시작이 너무 멂"), IsClientAimPlausible(FVector(5000.f, 0.f, 0.f), FVector(6000.f, 0.f, 0.f), Pawn, Forward, 800.f, 60.f, 9000.f));
	TestFalse(TEXT("조준점이 사거리 밖"), IsClientAimPlausible(Camera, FVector(5000.f, 0.f, 50.f), Pawn, Forward, 800.f, 60.f, 2800.f));
	TestFalse(TEXT("등 뒤 조준"), IsClientAimPlausible(Camera, FVector(-1500.f, 0.f, 50.f), Pawn, Forward, 800.f, 60.f, 2800.f));

	// TargetData 왕복(직렬화 없이 담기 · 꺼내기)
	FGrimAimResult Aim;
	Aim.Mode = EGrimAimMode::Soft;
	Aim.ViewOrigin = Camera;
	Aim.AimPoint = FVector(1200.f, 30.f, 40.f);
	FGrimAimResult Read;
	TestTrue(TEXT("TargetData 꺼내기"), UGrimSpellAbility::ReadAimTargetData(UGrimSpellAbility::MakeAimTargetData(Aim), Read));
	TestTrue(TEXT("시선 시작"), Read.ViewOrigin.Equals(Aim.ViewOrigin));
	TestTrue(TEXT("조준점"), Read.AimPoint.Equals(Aim.AimPoint));
	TestEqual(TEXT("모드"), Read.Mode, EGrimAimMode::Soft);
	return true;
}

// 시작 마법 3개의 전달 방식 · 필드(make_spells.py 가 정본).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryStartingSpellsTest, "Grimoire.Magic.Delivery.StartingSpells", GrimDeliveryTestFlags)
bool FGrimDeliveryStartingSpellsTest::RunTest(const FString& Parameters)
{
	const UGrimSpellDefinition* Ember = LoadStartingSpell(TEXT("DA_Spell_Ember"));
	const UGrimSpellDefinition* Water = LoadStartingSpell(TEXT("DA_Spell_WaterJet"));
	const UGrimSpellDefinition* Breeze = LoadStartingSpell(TEXT("DA_Spell_Breeze"));
	if (!TestNotNull(TEXT("불씨"), Ember) || !TestNotNull(TEXT("물줄기"), Water) || !TestNotNull(TEXT("미풍"), Breeze))
	{
		return false;
	}

	TestEqual(TEXT("불씨 = Bolt"), Ember->Delivery, EGrimSpellDelivery::Bolt);
	TestTrue(TEXT("불씨 속도 6000~9000"), Ember->BoltSpeed >= 6000.f && Ember->BoltSpeed <= 9000.f);
	TestTrue(TEXT("불씨 유도 > 0"), Ember->HomingStrength > 0.f);
	TestFalse(TEXT("불씨는 한 번"), Ember->bChanneled);
	TestTrue(TEXT("불씨 Windup 0.08~0.12"), Ember->CastWindup >= 0.08f && Ember->CastWindup <= 0.12f);

	TestEqual(TEXT("물줄기 = Stream"), Water->Delivery, EGrimSpellDelivery::Stream);
	TestTrue(TEXT("물줄기는 누르는 동안"), Water->bChanneled);
	TestTrue(TEXT("물줄기 초당 마나 > 0"), Water->ChannelManaPerSecond > 0.f);
	TestTrue(TEXT("물줄기 속도 > 0"), Water->StreamLaunchSpeed > 0.f);
	TestEqual(TEXT("물줄기 판정 주기 0.1"), Water->StreamTickInterval, 0.1f, 1e-4f);
	TestTrue(TEXT("물줄기 구간 ≥ 4"), Water->StreamSegments >= 4);

	TestEqual(TEXT("미풍 = Cone"), Breeze->Delivery, EGrimSpellDelivery::Cone);
	TestTrue(TEXT("미풍 원뿔 거리 > 0"), Breeze->ConeRange > 0.f);
	TestTrue(TEXT("미풍 반각 10~60"), Breeze->ConeHalfAngle >= 10.f && Breeze->ConeHalfAngle <= 60.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDeliveryZoomAimTest, "Grimoire.Magic.Delivery.ZoomAim", GrimDeliveryTestFlags)
bool FGrimDeliveryZoomAimTest::RunTest(const FString& Parameters)
{
	// 줌인 조준(R24)은 대상이 있어도 따라가지 않는다 — 겨눈 지점으로 곧게.
	AActor* Dummy = NewObject<AActor>();
	FGrimAimResult Aim;
	Aim.Target = Dummy;
	Aim.Mode = EGrimAimMode::Soft;
	TestTrue(TEXT("소프트 락온은 대상을 따라간다"), UGrimSpellAbility::FollowsTarget(Aim));
	Aim.Mode = EGrimAimMode::Locked;
	TestTrue(TEXT("고정 락온은 대상을 따라간다"), UGrimSpellAbility::FollowsTarget(Aim));
	Aim.Mode = EGrimAimMode::Zoom;
	TestFalse(TEXT("줌인 조준은 따라가지 않는다"), UGrimSpellAbility::FollowsTarget(Aim));
	Aim.Target = nullptr;
	Aim.Mode = EGrimAimMode::Free;
	TestFalse(TEXT("대상이 없으면 따라가지 않는다"), UGrimSpellAbility::FollowsTarget(Aim));
	return true;
}

#endif
