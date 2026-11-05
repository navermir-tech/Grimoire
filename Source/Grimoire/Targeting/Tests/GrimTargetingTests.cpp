#include "Misc/AutomationTest.h"
#include "Targeting/GrimTargetingMath.h"
#include "Targeting/GrimTargetingSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimTargetingTestsPrivate
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FGrimTargetCandidate MakeCandidate(EGrimTargetCategory Category, float Angle, float Distance, bool bCurrent, const FGrimTargetScoreWeights& Weights)
	{
		FGrimTargetCandidate Candidate;
		Candidate.Category = Category;
		Candidate.AngleDegrees = Angle;
		Candidate.Distance = Distance;
		Candidate.bIsCurrent = bCurrent;
		FGrimTargetingMath::ScoreCandidate(Candidate, 25.f, 2500.f, Weights);
		return Candidate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingConeTest, "Grimoire.Targeting.Cone", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingConeTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const FVector Axis = FVector::ForwardVector;

	TestEqual(TEXT("정면은 0도"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(1000.f, 0.f, 0.f)), 0.f, 1e-3f);
	TestEqual(TEXT("옆은 90도"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(0.f, 500.f, 0.f)), 90.f, 1e-3f);
	TestEqual(TEXT("뒤는 180도"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(-10.f, 0.f, 0.f)), 180.f, 1e-3f);
	TestEqual(TEXT("45도"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(100.f, 100.f, 0.f)), 45.f, 1e-3f);
	TestEqual(TEXT("축 길이는 상관없다"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis * 37.f, FVector(100.f, 100.f, 0.f)), 45.f, 1e-3f);
	TestEqual(TEXT("같은 점은 0"), FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, Origin), 0.f);

	// 반각 25°: tan(25°) ≈ 0.466 → 1000cm 앞 · 옆 400cm(21.8°)는 안, 옆 500cm(26.6°)는 밖.
	const float Inside = FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(1000.f, 400.f, 0.f));
	const float Outside = FGrimTargetingMath::AngleFromAxisDegrees(Origin, Axis, FVector(1000.f, 500.f, 0.f));
	TestTrue(TEXT("21.8°는 원뿔 안"), FGrimTargetingMath::IsInsideCone(Inside, 1000.f, 25.f, 2500.f));
	TestFalse(TEXT("26.6°는 원뿔 밖"), FGrimTargetingMath::IsInsideCone(Outside, 1000.f, 25.f, 2500.f));
	TestFalse(TEXT("사거리 밖"), FGrimTargetingMath::IsInsideCone(0.f, 2600.f, 25.f, 2500.f));
	TestTrue(TEXT("경계는 안"), FGrimTargetingMath::IsInsideCone(25.f, 2500.f, 25.f, 2500.f));
	// 지금 대상 여유 반각(+5°)이면 26.6° 도 유지된다.
	TestTrue(TEXT("지금 대상 여유"), FGrimTargetingMath::IsInsideCone(Outside, 1000.f, 30.f, 2500.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingCategoryTest, "Grimoire.Targeting.Priority.Category", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingCategoryTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("적"), FGrimTargetingMath::Categorize(EGrimTargetKind::Enemy, false), EGrimTargetCategory::Enemy);
	TestEqual(TEXT("반응 사물"), FGrimTargetingMath::Categorize(EGrimTargetKind::Object, true), EGrimTargetCategory::ReactiveObject);
	TestEqual(TEXT("그 밖의 사물"), FGrimTargetingMath::Categorize(EGrimTargetKind::Object, false), EGrimTargetCategory::Object);
	TestEqual(TEXT("생물"), FGrimTargetingMath::Categorize(EGrimTargetKind::Creature, true), EGrimTargetCategory::Creature);
	TestEqual(TEXT("아군은 잡지 않는다"), FGrimTargetingMath::Categorize(EGrimTargetKind::Ally, false), EGrimTargetCategory::None);
	TestEqual(TEXT("종류 없음"), FGrimTargetingMath::Categorize(EGrimTargetKind::None, true), EGrimTargetCategory::None);

	const FGrimTargetScoreWeights Weights;
	TestTrue(TEXT("적 > 반응 사물"), FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory::Enemy, Weights) > FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory::ReactiveObject, Weights));
	TestTrue(TEXT("반응 사물 > 그 밖의 사물"), FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory::ReactiveObject, Weights) > FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory::Object, Weights));
	TestEqual(TEXT("None 은 0"), FGrimTargetingMath::GetCategoryWeight(EGrimTargetCategory::None, Weights), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingScoreTest, "Grimoire.Targeting.Score", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingScoreTest::RunTest(const FString& Parameters)
{
	FGrimTargetScoreWeights Weights;
	Weights.Center = 1.f;
	Weights.Distance = 0.5f;
	Weights.Kind = 1.f;
	Weights.Keep = 0.3f;
	Weights.EnemyKind = 1.f;
	Weights.ReactiveObjectKind = 0.5f;
	Weights.ObjectKind = 0.1f;

	// 정중앙 · 거리 0 · 적 · 지금 대상 = 1 + 0.5 + 1 + 0.3
	TestEqual(TEXT("최대 점수"), FGrimTargetingMath::Score(0.f, 25.f, 0.f, 2500.f, EGrimTargetCategory::Enemy, true, Weights), 2.8f, 1e-4f);
	// 반각 끝 · 사거리 끝 · 그 밖의 사물 = 종류 가중만
	TestEqual(TEXT("가장자리"), FGrimTargetingMath::Score(25.f, 25.f, 2500.f, 2500.f, EGrimTargetCategory::Object, false, Weights), 0.1f, 1e-4f);
	// 반 각 · 반 거리 · 반응 사물 = 0.5 + 0.25 + 0.5
	TestEqual(TEXT("가운데"), FGrimTargetingMath::Score(12.5f, 25.f, 1250.f, 2500.f, EGrimTargetCategory::ReactiveObject, false, Weights), 1.25f, 1e-4f);
	// 여유 반각 밖(30°)이어도 중앙 항은 0 에서 멈춘다(음수 아님)
	TestEqual(TEXT("반각 밖은 0 으로 자름"), FGrimTargetingMath::Score(30.f, 25.f, 2500.f, 2500.f, EGrimTargetCategory::Object, true, Weights), 0.4f, 1e-4f);

	// 화면 중앙에 가까울수록 · 가까울수록 높다
	TestTrue(TEXT("중앙 우선"), FGrimTargetingMath::Score(2.f, 25.f, 1000.f, 2500.f, EGrimTargetCategory::Object, false, Weights)
		> FGrimTargetingMath::Score(20.f, 25.f, 1000.f, 2500.f, EGrimTargetCategory::Object, false, Weights));
	TestTrue(TEXT("가까운 쪽 우선"), FGrimTargetingMath::Score(10.f, 25.f, 300.f, 2500.f, EGrimTargetCategory::Object, false, Weights)
		> FGrimTargetingMath::Score(10.f, 25.f, 2000.f, 2500.f, EGrimTargetCategory::Object, false, Weights));

	// 유지 가산: 거의 같은 두 후보면 지금 대상이 이긴다(자주 바뀌지 않게)
	TestTrue(TEXT("지금 대상 유지"), FGrimTargetingMath::Score(6.f, 25.f, 1000.f, 2500.f, EGrimTargetCategory::Object, true, Weights)
		> FGrimTargetingMath::Score(4.f, 25.f, 1000.f, 2500.f, EGrimTargetCategory::Object, false, Weights));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingSortTest, "Grimoire.Targeting.Priority.SortAndCap", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingSortTest::RunTest(const FString& Parameters)
{
	using namespace GrimTargetingTestsPrivate;
	const FGrimTargetScoreWeights Weights;

	// 반응 사물(꺼진 양초)이 정중앙 · 가까이, 적은 원뿔 가장자리 · 멀리
	TArray<FGrimTargetCandidate> Candidates;
	Candidates.Add(MakeCandidate(EGrimTargetCategory::Object, 3.f, 500.f, false, Weights));
	Candidates.Add(MakeCandidate(EGrimTargetCategory::ReactiveObject, 1.f, 400.f, false, Weights));
	Candidates.Add(MakeCandidate(EGrimTargetCategory::Enemy, 24.f, 2400.f, false, Weights));

	TArray<FGrimTargetCandidate> ScoreOnly = Candidates;
	FGrimTargetingMath::SortAndCap(ScoreOnly, false, 32);
	TestEqual(TEXT("점수만: 반응 사물이 1등"), ScoreOnly[0].Category, EGrimTargetCategory::ReactiveObject);
	TestEqual(TEXT("점수만: 반응 사물 > 그 밖의 사물"), ScoreOnly[1].Category, EGrimTargetCategory::Object);

	TArray<FGrimTargetCandidate> EnemiesFirst = Candidates;
	FGrimTargetingMath::SortAndCap(EnemiesFirst, true, 32);
	TestEqual(TEXT("적 우선: 적이 1등"), EnemiesFirst[0].Category, EGrimTargetCategory::Enemy);
	TestEqual(TEXT("적 우선: 그다음은 점수 순"), EnemiesFirst[1].Category, EGrimTargetCategory::ReactiveObject);

	// 상한 32
	TArray<FGrimTargetCandidate> Many;
	for (int32 Index = 0; Index < 50; ++Index)
	{
		Many.Add(MakeCandidate(EGrimTargetCategory::Object, Index * 0.4f, 1000.f, false, Weights));
	}
	FGrimTargetingMath::SortAndCap(Many, true, 32);
	TestEqual(TEXT("상한 32"), Many.Num(), 32);
	TestEqual(TEXT("가장 중앙이 1등"), Many[0].AngleDegrees, 0.f);
	bool bSorted = true;
	for (int32 Index = 1; Index < Many.Num(); ++Index)
	{
		bSorted &= Many[Index - 1].Score >= Many[Index].Score;
	}
	TestTrue(TEXT("점수 내림차순"), bSorted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingOcclusionTest, "Grimoire.Targeting.Occlusion", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingOcclusionTest::RunTest(const FString& Parameters)
{
	// 0.05초 간격 선정, 허용 0.3초
	FGrimOcclusionTimer Timer;
	TestTrue(TEXT("보이면 유지"), Timer.Update(true, 0.05f, 0.3f));
	bool bKept = true;
	for (int32 Step = 0; Step < 5; ++Step) // 0.25초 가림
	{
		bKept = Timer.Update(false, 0.05f, 0.3f);
	}
	TestTrue(TEXT("0.3초 안은 유지"), bKept);
	TestFalse(TEXT("0.3초를 넘으면 풀림"), Timer.Update(false, 0.1f, 0.3f));

	// 잠깐 가렸다 다시 보이면 처음부터
	FGrimOcclusionTimer Flicker;
	Flicker.Update(false, 0.2f, 0.3f);
	Flicker.Update(true, 0.05f, 0.3f);
	TestEqual(TEXT("보이면 0 으로"), Flicker.OccludedSeconds, 0.f);
	TestTrue(TEXT("다시 0.2초 가려도 유지"), Flicker.Update(false, 0.2f, 0.3f));

	Flicker.Reset();
	TestEqual(TEXT("Reset"), Flicker.OccludedSeconds, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingSwitchTest, "Grimoire.Targeting.Lock.Switch", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingSwitchTest::RunTest(const FString& Parameters)
{
	const FVector2D From(960.f, 540.f);
	// 0: 오른쪽 가까이, 1: 오른쪽 멀리, 2: 왼쪽, 3: 위(화면 y 작음), 4: 지금 대상과 같은 자리
	const TArray<FVector2D> Screen = {
		FVector2D(1160.f, 560.f),
		FVector2D(1700.f, 540.f),
		FVector2D(600.f, 540.f),
		FVector2D(960.f, 200.f),
		FVector2D(960.f, 540.f)
	};

	TestEqual(TEXT("오른쪽 → 가까운 오른쪽"), FGrimTargetingMath::FindSwitchTarget(From, FVector2D(1.f, 0.f), Screen, 60.f), 0);
	TestEqual(TEXT("왼쪽"), FGrimTargetingMath::FindSwitchTarget(From, FVector2D(-1.f, 0.f), Screen, 60.f), 2);
	TestEqual(TEXT("위"), FGrimTargetingMath::FindSwitchTarget(From, FVector2D(0.f, -1.f), Screen, 60.f), 3);
	TestEqual(TEXT("아래엔 없음"), FGrimTargetingMath::FindSwitchTarget(From, FVector2D(0.f, 1.f), Screen, 30.f), INDEX_NONE);
	TestEqual(TEXT("방향 0 이면 없음"), FGrimTargetingMath::FindSwitchTarget(From, FVector2D::ZeroVector, Screen, 60.f), INDEX_NONE);

	// 카메라 평면 투영: 정면 +X 카메라에서 오른쪽(+Y)은 x +, 위(+Z)는 y −, 뒤는 실패
	FVector2D Plane;
	TestTrue(TEXT("앞 오른쪽 투영"), FGrimTargetingMath::ProjectToViewPlane(FVector::ZeroVector, FRotator::ZeroRotator, FVector(1000.f, 500.f, 0.f), Plane));
	TestEqual(TEXT("오른쪽 = x +"), static_cast<float>(Plane.X), 500.f, 1e-3f);
	FGrimTargetingMath::ProjectToViewPlane(FVector::ZeroVector, FRotator::ZeroRotator, FVector(1000.f, 0.f, 200.f), Plane);
	TestEqual(TEXT("위 = y −"), static_cast<float>(Plane.Y), -200.f, 1e-3f);
	TestFalse(TEXT("카메라 뒤"), FGrimTargetingMath::ProjectToViewPlane(FVector::ZeroVector, FRotator::ZeroRotator, FVector(-100.f, 0.f, 0.f), Plane));
	// 카메라가 돌아 있어도(요 90°, +Y 를 봄) 오른쪽은 −X
	FGrimTargetingMath::ProjectToViewPlane(FVector::ZeroVector, FRotator(0.f, 90.f, 0.f), FVector(-300.f, 1000.f, 0.f), Plane);
	TestEqual(TEXT("돈 카메라의 오른쪽"), static_cast<float>(Plane.X), 300.f, 1e-2f);

	// 시점 입력 → 화면 방향: 요 + = 오른쪽, 피치 + (위로 봄) = 화면 위(y −)
	TestEqual(TEXT("요 입력"), FGrimTargetingMath::LookInputToScreenDirection(FVector2D(1.f, 0.f)), FVector2D(1.f, 0.f));
	TestEqual(TEXT("피치 입력"), FGrimTargetingMath::LookInputToScreenDirection(FVector2D(0.f, 1.f)), FVector2D(0.f, -1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTargetingFlickTest, "Grimoire.Targeting.Lock.Flick", GrimTargetingTestsPrivate::TestFlags)
bool FGrimTargetingFlickTest::RunTest(const FString& Parameters)
{
	FGrimLookFlickDetector Flick;
	FVector2D Direction;

	// 문턱 6: 2 + 2 로는 아직, + 3 에서 넘김(오른쪽)
	TestFalse(TEXT("모자람"), Flick.AddInput(FVector2D(2.f, 0.f), 6.f, Direction));
	TestFalse(TEXT("아직"), Flick.AddInput(FVector2D(2.f, 0.f), 6.f, Direction));
	TestTrue(TEXT("문턱 넘음"), Flick.AddInput(FVector2D(3.f, 0.f), 6.f, Direction));
	TestEqual(TEXT("방향 = 오른쪽"), Direction, FVector2D(1.f, 0.f));
	TestEqual(TEXT("넘긴 뒤 비움"), Flick.Accumulated, FVector2D::ZeroVector);

	// 쿨다운 동안은 무시
	Flick.StartCooldown(0.35f);
	TestFalse(TEXT("쿨다운 중"), Flick.AddInput(FVector2D(10.f, 0.f), 6.f, Direction));
	Flick.Tick(0.4f, 0.2f);
	TestTrue(TEXT("쿨다운 뒤"), Flick.AddInput(FVector2D(-10.f, 0.f), 6.f, Direction));
	TestEqual(TEXT("방향 = 왼쪽"), Direction, FVector2D(-1.f, 0.f));

	// 천천히 흘린 입력은 넘김이 아니다(쉬는 동안 버림)
	Flick.Reset();
	Flick.AddInput(FVector2D(4.f, 0.f), 6.f, Direction);
	Flick.Tick(0.25f, 0.2f);
	TestFalse(TEXT("쉰 뒤 처음부터"), Flick.AddInput(FVector2D(4.f, 0.f), 6.f, Direction));

	// 요 · 피치가 따로 들어와도 합쳐서 본다
	Flick.Reset();
	Flick.AddInput(FVector2D(0.f, 4.5f), 6.f, Direction);
	TestTrue(TEXT("대각선 합"), Flick.AddInput(FVector2D(4.5f, 0.f), 6.f, Direction));
	TestEqual(TEXT("대각선 방향"), static_cast<float>(Direction.X), static_cast<float>(Direction.Y), 1e-4f);
	return true;
}

#endif
