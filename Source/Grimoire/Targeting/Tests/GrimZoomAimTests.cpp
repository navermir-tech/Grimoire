#include "Misc/AutomationTest.h"
#include "Targeting/GrimZoomAim.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimZoomAimTestsPrivate
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoomModeTest, "Grimoire.Targeting.Zoom.Mode", GrimZoomAimTestsPrivate::TestFlags)
bool FGrimZoomModeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("줌 · 대상 없음 = Zoom"), FGrimZoomAimMath::DecideAimMode(true, false, false), EGrimAimMode::Zoom);
	TestEqual(TEXT("줌 · 조준점 아래 대상 = 여전히 Zoom"), FGrimZoomAimMath::DecideAimMode(true, true, false), EGrimAimMode::Zoom);
	TestEqual(TEXT("줌이 고정보다 먼저"), FGrimZoomAimMath::DecideAimMode(true, true, true), EGrimAimMode::Zoom);
	TestEqual(TEXT("고정"), FGrimZoomAimMath::DecideAimMode(false, true, true), EGrimAimMode::Locked);
	TestEqual(TEXT("소프트"), FGrimZoomAimMath::DecideAimMode(false, true, false), EGrimAimMode::Soft);
	TestEqual(TEXT("대상 없음"), FGrimZoomAimMath::DecideAimMode(false, false, false), EGrimAimMode::Free);
	TestEqual(TEXT("대상 없는 고정은 Free"), FGrimZoomAimMath::DecideAimMode(false, false, true), EGrimAimMode::Free);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoomInputTest, "Grimoire.Targeting.Zoom.Input", GrimZoomAimTestsPrivate::TestFlags)
bool FGrimZoomInputTest::RunTest(const FString& Parameters)
{
	// 누르는 동안
	{
		FGrimZoomInputState Input;
		TestTrue(TEXT("누르면 바뀜"), Input.Press(false));
		TestTrue(TEXT("누르는 동안 켜짐"), Input.bActive);
		TestFalse(TEXT("다시 눌림(반복 Started)은 그대로"), Input.Press(false));
		TestTrue(TEXT("여전히 켜짐"), Input.bActive);
		TestTrue(TEXT("떼면 바뀜"), Input.Release(false));
		TestFalse(TEXT("떼면 꺼짐"), Input.bActive);
		TestFalse(TEXT("꺼진 채 떼면 그대로"), Input.Release(false));
	}
	// 토글
	{
		FGrimZoomInputState Input;
		TestTrue(TEXT("토글 눌림 → 켬"), Input.Press(true));
		TestTrue(TEXT("켜짐"), Input.bActive);
		TestFalse(TEXT("토글은 떼도 그대로"), Input.Release(true));
		TestTrue(TEXT("여전히 켜짐"), Input.bActive);
		TestTrue(TEXT("다시 눌림 → 끔"), Input.Press(true));
		TestFalse(TEXT("꺼짐"), Input.bActive);
	}
	// 취소(폰 잃음)
	{
		FGrimZoomInputState Input;
		Input.Press(true);
		TestTrue(TEXT("취소하면 바뀜"), Input.Cancel());
		TestFalse(TEXT("취소 뒤 꺼짐"), Input.bActive);
		TestFalse(TEXT("꺼진 채 취소는 그대로"), Input.Cancel());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoomBlendTest, "Grimoire.Targeting.Zoom.Blend", GrimZoomAimTestsPrivate::TestFlags)
bool FGrimZoomBlendTest::RunTest(const FString& Parameters)
{
	FGrimZoomBlend Blend;
	TestTrue(TEXT("처음은 평소"), Blend.IsFullyOut());

	Blend.Tick(true, 0.1f, 0.2f);
	TestEqual(TEXT("0.1초면 반"), Blend.Alpha, 0.5f, 1e-5f);
	TestEqual(TEXT("반이면 부드럽게 한 값도 반"), Blend.GetEased(), 0.5f, 1e-5f);
	Blend.Tick(true, 0.05f, 0.2f);
	TestTrue(TEXT("SmoothStep 은 3/4 지점에서 선형보다 앞선다"), Blend.GetEased() > Blend.Alpha);
	Blend.Tick(true, 1.f, 0.2f);
	TestTrue(TEXT("넘쳐도 1"), Blend.IsFullyIn());
	TestEqual(TEXT("1"), Blend.Alpha, 1.f);

	Blend.Tick(false, 0.05f, 0.2f);
	TestEqual(TEXT("되돌아가는 중"), Blend.Alpha, 0.75f, 1e-5f);
	// 되돌아가다 다시 누르면 그 자리에서 다시 당긴다(튀지 않음).
	Blend.Tick(true, 0.05f, 0.2f);
	TestEqual(TEXT("되돌리던 자리에서 다시"), Blend.Alpha, 1.f, 1e-5f);
	Blend.Tick(false, 0.5f, 0.2f);
	TestTrue(TEXT("다 돌아옴"), Blend.IsFullyOut());

	Blend.Tick(true, 0.f, 0.f);
	TestTrue(TEXT("시간 0 이면 바로"), Blend.IsFullyIn());
	Blend.Tick(false, -1.f, 0.2f);
	TestTrue(TEXT("음수 시간은 움직이지 않는다"), Blend.IsFullyIn());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoomSpeedOverrideTest, "Grimoire.Targeting.Zoom.Speed", GrimZoomAimTestsPrivate::TestFlags)
bool FGrimZoomSpeedOverrideTest::RunTest(const FString& Parameters)
{
	// 평범한 경우: 500 → 230 → 500
	{
		FGrimZoomSpeedOverride Speed;
		TestEqual(TEXT("줌 시작 = 걷기"), Speed.Begin(500.f, 230.f), 230.f);
		TestEqual(TEXT("줌 중 그대로"), Speed.Update(230.f, 230.f), 230.f);
		TestEqual(TEXT("떼면 되돌림"), Speed.End(230.f), 500.f);
		TestFalse(TEXT("끝나면 비활성"), Speed.bActive);
	}
	// 평소가 이미 느리면(감속 중) 빨라지지 않는다.
	{
		FGrimZoomSpeedOverride Speed;
		TestEqual(TEXT("느린 평소는 그대로"), Speed.Begin(150.f, 230.f), 150.f);
		TestEqual(TEXT("되돌림"), Speed.End(150.f), 150.f);
	}
	// 줌 중에 다른 시스템이 기준을 바꿨다 → 새 기준, 떼면 그 값으로.
	{
		FGrimZoomSpeedOverride Speed;
		Speed.Begin(500.f, 230.f);
		TestEqual(TEXT("다른 시스템이 600 으로 → 여전히 230"), Speed.Update(600.f, 230.f), 230.f);
		TestEqual(TEXT("떼면 새 기준"), Speed.End(230.f), 600.f);
	}
	{
		FGrimZoomSpeedOverride Speed;
		Speed.Begin(500.f, 230.f);
		TestEqual(TEXT("다른 시스템이 100 으로 → 100"), Speed.Update(100.f, 230.f), 100.f);
		TestEqual(TEXT("떼면 100"), Speed.End(100.f), 100.f);
	}
	// 떼는 그 프레임에 누가 바꿨으면 그 값을 둔다.
	{
		FGrimZoomSpeedOverride Speed;
		Speed.Begin(500.f, 230.f);
		TestEqual(TEXT("마지막 순간의 다른 값 유지"), Speed.End(320.f), 320.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimZoomFlagOverrideTest, "Grimoire.Targeting.Zoom.Orient", GrimZoomAimTestsPrivate::TestFlags)
bool FGrimZoomFlagOverrideTest::RunTest(const FString& Parameters)
{
	bool bFix = false;

	// 평범한 경우: 이동 방향 보기 true → 줌 false → 떼면 true. 감시 중 아무 일 없음.
	{
		FGrimZoomFlagOverride Orient;
		TestFalse(TEXT("줌 시작 = 끔"), Orient.Begin(true, false));
		TestFalse(TEXT("줌 중 끔"), Orient.Update(false));
		TestTrue(TEXT("떼면 켬"), Orient.End(false, 0.6f));
		TestTrue(TEXT("감시 중"), Orient.IsGuarding());
		TestFalse(TEXT("감시 중 바로잡을 것 없음"), Orient.TickGuard(true, 0.3f, bFix));
		TestFalse(TEXT("감시 끝, 바로잡을 것 없음"), Orient.TickGuard(true, 0.4f, bFix));
		TestFalse(TEXT("감시 끝남"), Orient.IsGuarding());
	}
	// 줌 중에 시전 → 시전 돌리기가 줌 값(false)을 기억 → 줌을 먼저 뗌 → 시전 돌리기가 끝나며 false 를 넣음 → 감시 끝에 true 로.
	{
		FGrimZoomFlagOverride Orient;
		Orient.Begin(true, false);
		Orient.Update(false);
		TestTrue(TEXT("떼면 켬"), Orient.End(false, 0.6f));
		TestFalse(TEXT("감시 중에는 기다린다"), Orient.TickGuard(false, 0.3f, bFix));
		TestTrue(TEXT("감시 끝에 줌 값이 남아 있으면 바로잡는다"), Orient.TickGuard(false, 0.4f, bFix));
		TestTrue(TEXT("되돌릴 값 = 켬"), bFix);
	}
	// 시전 돌리기 도중(false)에 줌 시작 → 줌 중에 시전 돌리기가 끝나며 true 를 넣음 → 그것을 되돌릴 값으로 기억.
	{
		FGrimZoomFlagOverride Orient;
		TestFalse(TEXT("줌 시작"), Orient.Begin(false, false));
		TestFalse(TEXT("누가 켜도 줌 중엔 끔"), Orient.Update(true));
		TestTrue(TEXT("떼면 그 사이 켜진 값"), Orient.End(false, 0.6f));
	}
	// 시전 돌리기 도중에 줌 시작 → 시전 돌리기가 끝나기 전에 뗌(되돌릴 값 false) → 시전 돌리기가 끝나며 true(정당한 값) → 그대로 둔다.
	{
		FGrimZoomFlagOverride Orient;
		Orient.Begin(false, false);
		TestFalse(TEXT("떼면 끔(기억한 값)"), Orient.End(false, 0.6f));
		TestFalse(TEXT("줌 값이 아닌 값은 다른 쪽 몫"), Orient.TickGuard(true, 1.f, bFix));
	}
	// 감시 시간 0 이면 감시하지 않는다.
	{
		FGrimZoomFlagOverride Orient;
		Orient.Begin(true, false);
		Orient.End(false, 0.f);
		TestFalse(TEXT("감시 없음"), Orient.IsGuarding());
		TestFalse(TEXT("바로잡지 않음"), Orient.TickGuard(false, 1.f, bFix));
	}
	// 컨트롤 방향 보기(false → 줌 true → false)
	{
		FGrimZoomFlagOverride Desired;
		TestTrue(TEXT("줌 = 켬"), Desired.Begin(false, true));
		TestTrue(TEXT("줌 중 켬"), Desired.Update(true));
		TestFalse(TEXT("떼면 끔"), Desired.End(true, 0.f));
	}
	return true;
}

#endif
