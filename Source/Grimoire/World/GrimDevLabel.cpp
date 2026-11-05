#include "World/GrimDevLabel.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace GrimDevLabelLocal
{
	int32 GShowDevLabels = 1;
	FAutoConsoleVariableRef CVarShowDevLabels(
		TEXT("Grim.DevLabels"),
		GShowDevLabels,
		TEXT("개발 표시용 떠 있는 글씨(랜드마크 · POI 설명, 공사 중 표시). 1 = 보임(기본), 0 = 숨김. 출시 빌드에서는 늘 숨김."),
		ECVF_Default);

	float GMaxDistanceOverride = 0.f;
	FAutoConsoleVariableRef CVarMaxDistance(
		TEXT("Grim.DevLabels.MaxDistance"),
		GMaxDistanceOverride,
		TEXT("개발 표시 글씨를 숨기는 거리(cm). 0 = 액터 값(MaxVisibleDistance, 기본 250 m)"),
		ECVF_Default);

	/** 제목 · 설명 글씨 크기(Slate 포인트) — 위젯 1 픽셀 = 1 cm × WorldScale */
	constexpr int32 TitleSize = 44;
	constexpr int32 BodySize = 30;
}

AGrimDevLabel::AGrimDevLabel()
{
	PrimaryActorTick.bCanEverTick = true;
	// 카메라 쪽으로 돌리기 · 거리 확인만 — 0.1 초면 돈 게 티 나지 않는다
	PrimaryActorTick.TickInterval = 0.1f;
	SetCanBeDamaged(false);
	bReplicates = false;

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetPivot(FVector2D(0.5, 1.0));
	Widget->SetTwoSided(true);
	Widget->SetBlendMode(EWidgetBlendMode::Transparent);
	Widget->SetBackgroundColor(FLinearColor::Transparent);
	Widget->SetWindowFocusable(false);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetCastShadow(false);
	// 글씨는 바뀌지 않는다 — 렌더 타깃에 한 번만 그리고(RequestRenderUpdate) 컴포넌트 틱은 끈다.
	// 예전엔 44개가 매 프레임 틱 · 1초마다 다시 그렸다(docs/systems/performance 「측정 기록」).
	Widget->SetTickWhenOffscreen(false);
	Widget->SetManuallyRedraw(true);
	Widget->SetTickMode(ETickMode::Disabled);
}

bool AGrimDevLabel::AreLabelsEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return GrimDevLabelLocal::GShowDevLabels != 0;
#endif
}

bool AGrimDevLabel::CanShowWidget() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UWorld* World = GetWorld();
	return World && !IsRunningCommandlet() && !IsRunningDedicatedServer() && World->GetNetMode() != NM_DedicatedServer;
#endif
}

void AGrimDevLabel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Widget)
	{
		Widget->SetRelativeScale3D(FVector(FMath::Max(WorldScale, 0.1f)));
	}
	// 에디터 뷰포트에서도 보이게(게임 월드는 BeginPlay 에서)
	const UWorld* World = GetWorld();
	if (World && !World->IsGameWorld() && CanShowWidget())
	{
		BuildWidget();
	}
}

void AGrimDevLabel::BeginPlay()
{
	Super::BeginPlay();
	if (!CanShowWidget())
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
		if (Widget)
		{
			Widget->SetComponentTickEnabled(false);
			Widget->SetVisibility(false);
		}
		return;
	}
	BuildWidget();
	ApplyVisibility(true);
}

void AGrimDevLabel::BuildWidget()
{
	if (!Widget)
	{
		return;
	}
	using namespace GrimDevLabelLocal;
	Widget->SetRelativeScale3D(FVector(FMath::Max(WorldScale, 0.1f)));

	TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", TitleSize))
			.ColorAndOpacity(FSlateColor(TitleColor))
			.WrapTextAt(WrapWidth)
		];
	if (!Body.IsEmpty())
	{
		Lines->AddSlot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Body)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", BodySize))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.92f)))
				.WrapTextAt(WrapWidth)
			];
	}
	TSharedRef<SWidget> Content = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.72f))
		.Padding(FMargin(28.f, 16.f))
		[
			Lines
		];
	Widget->SetSlateWidget(Content);
	Widget->RequestRenderUpdate();
}

void AGrimDevLabel::ApplyVisibility(bool bInRange)
{
	const bool bWant = AreLabelsEnabled() && bInRange;
	if (bWant != bShown || !bVisibilityApplied)
	{
		bVisibilityApplied = true;
		bShown = bWant;
		SetActorHiddenInGame(!bWant);
		if (Widget)
		{
			Widget->SetVisibility(bWant);
			if (bWant)
			{
				// 숨어 있던 동안 렌더 타깃이 비었을 수 있다 — 다시 보일 때 한 번 그린다(컴포넌트 틱이 켜졌다가 그린 뒤 꺼진다)
				Widget->RequestRenderUpdate();
			}
			else
			{
				// 숨은 채로는 그리기 요청이 남아 틱이 꺼지지 않는다 — 직접 끈다
				Widget->SetComponentTickEnabled(false);
			}
		}
	}
}

bool AGrimDevLabel::GetViewLocation(FVector& OutLocation) const
{
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		OutLocation = Camera->GetCameraLocation();
		return true;
	}
	return false;
}

void AGrimDevLabel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FVector ViewLocation;
	const bool bHasView = GetViewLocation(ViewLocation);
	// 카메라가 아직 없으면(첫 프레임) 거리로 숨기지 않는다
	const float MaxDistance = GrimDevLabelLocal::GMaxDistanceOverride > 0.f ? GrimDevLabelLocal::GMaxDistanceOverride : MaxVisibleDistance;
	const bool bInRange = !bHasView || FVector::DistSquared(ViewLocation, GetActorLocation()) <= FMath::Square(MaxDistance);
	ApplyVisibility(bInRange);
	if (!bShown || !bFaceCamera || !bHasView)
	{
		return;
	}
	// 위젯 앞면(+X)이 카메라를 보게 Yaw 만 돌린다
	const FVector ToCamera = ViewLocation - GetActorLocation();
	if (!ToCamera.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
	}
}
