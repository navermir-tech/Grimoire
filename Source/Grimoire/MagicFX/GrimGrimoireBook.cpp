#include "MagicFX/GrimGrimoireBook.h"
#include "MagicFX/GrimMagicFXMath.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "MagicFX/GrimMagicFXTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 책 크기(cm). 한쪽 면 너비 14 · 높이 20 — 손바닥 위에 올라가는 작은 책.
	constexpr float HalfWidth = 14.f;
	constexpr float PageHeight = 20.f;
	constexpr float CoverThickness = 1.2f;
	constexpr float PagesThickness = 1.5f;
	constexpr float PagesZ = CoverThickness * 0.5f + PagesThickness * 0.5f;
	constexpr float PagesTop = PagesZ + PagesThickness * 0.5f;

	UStaticMeshComponent* MakeBox(AActor* Owner, const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Cube, const FVector& Location, const FVector& SizeCm)
	{
		UStaticMeshComponent* Box = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Box->SetupAttachment(Parent);
		Box->SetStaticMesh(Cube);
		Box->SetRelativeLocation(Location);
		Box->SetRelativeScale3D(SizeCm / 100.f); // 엔진 기본 Cube 는 100cm.
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCastShadow(false);
		Box->bReceivesDecals = false;
		return Box;
	}

	UMaterialInstanceDynamic* MakeBookMaterial(UStaticMeshComponent* Mesh, UMaterialInterface* Base, const FLinearColor& Color, const FLinearColor& Emissive, float Glow)
	{
		UMaterialInstanceDynamic* Material = Base ? Mesh->CreateDynamicMaterialInstance(0, Base) : nullptr;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
			Material->SetScalarParameterValue(TEXT("Glow"), Glow);
		}
		return Material;
	}
}

AGrimGrimoireBook::AGrimGrimoireBook()
{
	PrimaryActorTick.bCanEverTick = true;
	// 손 뼈가 이번 프레임 자세로 움직인 뒤에 따라간다.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	bReplicates = true;
	// 부착 정보(AttachmentReplication)는 움직임 복제가 켜져 있어야 클라이언트로 간다(완드와 같음).
	SetReplicatingMovement(true);
	bNetUseOwnerRelevancy = true;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	FloatPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FloatPivot"));
	FloatPivot->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

	// 펼친 책: 표지 두 장 + 그 위 책장 덩어리 두 개. 책등은 X 축(Y = 0).
	CoverLeft = MakeBox(this, TEXT("CoverLeft"), FloatPivot, Cube, FVector(0.f, -HalfWidth * 0.5f - 0.3f, 0.f), FVector(PageHeight + 1.f, HalfWidth + 0.6f, CoverThickness));
	CoverRight = MakeBox(this, TEXT("CoverRight"), FloatPivot, Cube, FVector(0.f, HalfWidth * 0.5f + 0.3f, 0.f), FVector(PageHeight + 1.f, HalfWidth + 0.6f, CoverThickness));
	PagesLeft = MakeBox(this, TEXT("PagesLeft"), FloatPivot, Cube, FVector(0.f, -HalfWidth * 0.5f, PagesZ), FVector(PageHeight, HalfWidth - 0.6f, PagesThickness));
	PagesRight = MakeBox(this, TEXT("PagesRight"), FloatPivot, Cube, FVector(0.f, HalfWidth * 0.5f, PagesZ), FVector(PageHeight, HalfWidth - 0.6f, PagesThickness));

	// 넘기는 판: 책등에 붙은 축을 돌려 오른쪽 면에서 왼쪽 면으로 넘긴다.
	FlipPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FlipPivot"));
	FlipPivot->SetupAttachment(FloatPivot);
	FlipPivot->SetRelativeLocation(FVector(0.f, 0.f, PagesTop + 0.15f));
	FlipPageMesh = MakeBox(this, TEXT("FlipPage"), FlipPivot, Cube, FVector(0.f, HalfWidth * 0.5f - 0.2f, 0.f), FVector(PageHeight - 0.5f, HalfWidth - 0.8f, 0.2f));

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(FloatPivot);
	GlowLight->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
	GlowLight->SetCastShadows(false);
	GlowLight->SetIntensityUnits(ELightUnits::Unitless);
	GlowLight->SetAttenuationRadius(90.f);
	GlowLight->SetIntensity(IdleLightIntensity);
	GlowLight->SetLightColor(IdleGlowColor);
}

void AGrimGrimoireBook::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Base = UGrimMagicFXStatics::GetBookMaterial();
	MakeBookMaterial(CoverLeft, Base, CoverColor, FLinearColor::Black, 0.f);
	MakeBookMaterial(CoverRight, Base, CoverColor, FLinearColor::Black, 0.f);
	PagesMaterial = MakeBookMaterial(PagesLeft, Base, PageColor, IdleGlowColor, IdleGlow);
	if (PagesMaterial)
	{
		PagesRight->SetMaterial(0, PagesMaterial);
	}
	FlipPageMaterial = MakeBookMaterial(FlipPageMesh, Base, PageColor, IdleGlowColor, IdleGlow);

	// 시작 위상을 책마다 달리해 두 사람의 책이 똑같이 오르내리지 않게 한다.
	LocalTime = FMath::FRandRange(0.f, Float.BobPeriod);
}

USkeletalMeshComponent* AGrimGrimoireBook::GetOwnerMesh() const
{
	return Cast<USkeletalMeshComponent>(Root ? Root->GetAttachParent() : nullptr);
}

FName AGrimGrimoireBook::GetFloatAttachName(const USkeletalMeshComponent* Mesh) const
{
	if (Mesh && !FloatSocketName.IsNone() && Mesh->DoesSocketExist(FloatSocketName))
	{
		return FloatSocketName;
	}
	return FloatBoneName;
}

void AGrimGrimoireBook::AttachToCharacterMesh(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}
	// Snap: 루트를 손에 그대로 붙인다. 손바닥 위 높이 · 흔들림은 FloatPivot 이 틱에서 맡는다.
	AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetFloatAttachName(Mesh));
}

void AGrimGrimoireBook::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LocalTime += DeltaSeconds;
	TickStow(DeltaSeconds);
	TickFlip(DeltaSeconds);
	TickPlacement();
}

// ---- 넘김 ----

void AGrimGrimoireBook::FlipPage(FLinearColor Color)
{
	NotifyCombatActivity();
	FlipColor = Color;
	if (bFlipping)
	{
		// 앞으로 넘기기만 쌓는다(촤르륵 중이면 그 뒤에).
		QueuedFlips = FMath::Clamp(FMath::Max(QueuedFlips, 0) + 1, 0, MaxQueuedFlips);
		QueuedFlipDuration = FlipDuration;
		return;
	}
	QueuedFlips = 1;
	QueuedFlipDuration = FlipDuration;
	StartNextFlip();
}

void AGrimGrimoireBook::FlipTo(int32 SlotIndex, FLinearColor Color)
{
	if (CurrentSlot == INDEX_NONE)
	{
		CurrentSlot = SlotIndex;
		return;
	}
	const int32 Count = GrimMagicFXMath::CalcFlipToPageCount(CurrentSlot, SlotIndex, PagesPerSpellSlot, MaxFlipToPages);
	CurrentSlot = SlotIndex;
	if (Count == 0)
	{
		return;
	}
	FlipColor = Color;
	QueuedFlips = Count;
	QueuedFlipDuration = GrimMagicFXMath::CalcRapidFlipDuration(Count, FlipToDuration, 0.04f, FlipDuration);
	if (!bFlipping)
	{
		StartNextFlip();
	}
}

void AGrimGrimoireBook::BeginChannelFlip(FLinearColor Color)
{
	NotifyCombatActivity();
	FlipColor = Color;
	bChannelFlip = true;
	// 첫 장은 시전 큐(GC_Spell_Cast)가 넘긴다 — 다음 장부터 FlipRate 로.
	ChannelFlipTimer = GrimMagicFXMath::CalcChannelFlipInterval(FlipRate);
}

void AGrimGrimoireBook::EndChannelFlip()
{
	bChannelFlip = false;
}

void AGrimGrimoireBook::NotifyCombatActivity()
{
	if (const UWorld* World = GetWorld())
	{
		LastCombatTime = World->GetTimeSeconds();
	}
}

void AGrimGrimoireBook::StartNextFlip()
{
	if (QueuedFlips == 0)
	{
		return;
	}
	const int32 Direction = QueuedFlips > 0 ? 1 : -1;
	QueuedFlips -= Direction;
	CurrentFlipDirection = Direction;
	CurrentFlipDuration = QueuedFlipDuration;
	FlipElapsed = 0.f;
	SinceGlowStart = 0.f;
	bFlipping = true;
	FlipPivot->SetRelativeRotation(GrimMagicFXMath::CalcFlipPageRotation(0.f, Direction));
}

void AGrimGrimoireBook::TickFlip(float DeltaSeconds)
{
	SinceGlowStart += DeltaSeconds;

	if (bChannelFlip)
	{
		const float Interval = GrimMagicFXMath::CalcChannelFlipInterval(FlipRate);
		if (Interval > 0.f)
		{
			ChannelFlipTimer -= DeltaSeconds;
			if (ChannelFlipTimer <= 0.f)
			{
				ChannelFlipTimer += Interval;
				FlipPage(FlipColor);
			}
		}
	}

	if (bFlipping)
	{
		FlipElapsed += DeltaSeconds;
		const float Progress = GrimMagicFXMath::CalcFlipProgress(FlipElapsed, CurrentFlipDuration);
		FlipPivot->SetRelativeRotation(GrimMagicFXMath::CalcFlipPageRotation(Progress, CurrentFlipDirection));
		if (FlipElapsed >= CurrentFlipDuration)
		{
			// 다 넘긴 판은 오른쪽 면으로 돌아가 다음 장이 된다(두 면이 같은 모양이라 티가 나지 않는다).
			bFlipping = false;
			FlipPivot->SetRelativeRotation(FQuat::Identity);
			StartNextFlip();
		}
	}

	const float Glow = GrimMagicFXMath::CalcFlipGlow(SinceGlowStart, FlipGlowDuration, FlipGlowPeak);
	ApplyGlow(Glow, FlipColor);
}

void AGrimGrimoireBook::ApplyGlow(float Glow, const FLinearColor& Color)
{
	const float Strength = FlipGlowPeak > 0.f ? FMath::Clamp(Glow / FlipGlowPeak, 0.f, 1.f) : 0.f;
	const FLinearColor Emissive = FMath::Lerp(IdleGlowColor, Color, Strength);
	if (FlipPageMaterial)
	{
		FlipPageMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
		FlipPageMaterial->SetScalarParameterValue(TEXT("Glow"), IdleGlow + Glow);
	}
	const float Visible = 1.f - StowAlpha;
	GlowLight->SetLightColor(Emissive);
	GlowLight->SetIntensity((IdleLightIntensity + FlipLightIntensity * Strength) * Visible);
}

// ---- 거두기 (Q25) ----

FGrimGrimoireStowQuery AGrimGrimoireBook::GatherStowQuery() const
{
	FGrimGrimoireStowQuery Query;
	Query.MountedCombatHoldSeconds = MountedCombatHoldSeconds;

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		Pawn = Cast<APawn>(GetAttachParentActor());
	}
	if (!Pawn)
	{
		// 든 사람이 없다 — 펼칠 일이 없으니 거둔 모양으로.
		Query.bHasWand = false;
		return Query;
	}

	Query.bHasWand = UGrimMagicFXStatics::FindWand(Pawn) != nullptr;

	if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
	{
		Query.bSwimming = ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Swimming);
		Query.bBuildMode = ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_BuildMode);
		Query.bCarrying = ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Carrying);
		Query.bMounted = ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Mounted);
		Query.bAiming = ASC->HasMatchingGameplayTag(GrimMagicFXTags::State_Aiming);
	}
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Query.bSwimming |= Movement->IsSwimming();
		}
	}
	if (const UWorld* World = GetWorld())
	{
		Query.SecondsSinceCombat = static_cast<float>(World->GetTimeSeconds() - LastCombatTime);
	}
	return Query;
}

void AGrimGrimoireBook::TickStow(float DeltaSeconds)
{
	const bool bStow = GrimMagicFXMath::ShouldStowGrimoire(GatherStowQuery());
	StowAlpha = GrimMagicFXMath::StepStowAlpha(StowAlpha, bStow, DeltaSeconds, StowBlendTime);
}

void AGrimGrimoireBook::TickPlacement()
{
	const FVector Bob = GrimMagicFXMath::CalcBookBobOffset(LocalTime, Float);
	const FQuat Tilt = GrimMagicFXMath::CalcBookBobTilt(LocalTime, Float).Quaternion();

	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	const AActor* OwnerActor = GetAttachParentActor();
	if (!Mesh || !OwnerActor)
	{
		// 아직 붙지 않았다(클라이언트에서 부착 복제 전) — 제자리에서 흔들리기만.
		FloatPivot->SetRelativeLocationAndRotation(Bob, Tilt);
		return;
	}

	const FQuat BodyYaw = FRotator(0.f, OwnerActor->GetActorRotation().Yaw, 0.f).Quaternion();

	// 손 위: GrimoireFloat 소켓이 있으면 그 자세, 없으면 hand_l 위 FloatHeight 에 몸 방향으로 수평.
	FTransform FloatTransform;
	if (!FloatSocketName.IsNone() && Mesh->DoesSocketExist(FloatSocketName))
	{
		FloatTransform = Mesh->GetSocketTransform(FloatSocketName);
		FloatTransform.SetRotation(FloatTransform.GetRotation() * Tilt);
		FloatTransform.AddToTranslation(FloatTransform.GetRotation().RotateVector(Bob));
	}
	else
	{
		const FVector Hand = Mesh->GetSocketLocation(FloatBoneName);
		const FQuat Rotation = BodyYaw * FRotator(FloatPitch, 0.f, 0.f).Quaternion() * Tilt;
		FloatTransform = FTransform(Rotation, Hand + FVector(0.f, 0.f, FloatHeight) + Bob);
	}

	// 허리춤: GrimoireBelt 소켓이 있으면 그것, 없으면 pelvis 에서 몸 기준 BeltOffset.
	FTransform BeltTransform;
	if (!BeltSocketName.IsNone() && Mesh->DoesSocketExist(BeltSocketName))
	{
		BeltTransform = Mesh->GetSocketTransform(BeltSocketName);
	}
	else
	{
		const FVector Pelvis = Mesh->GetSocketLocation(BeltBoneName);
		BeltTransform = FTransform(BodyYaw * BeltRotation.Quaternion(), Pelvis + BodyYaw.RotateVector(BeltOffset));
	}

	const float Blend = FMath::SmoothStep(0.f, 1.f, StowAlpha);
	const FVector Location = FMath::Lerp(FloatTransform.GetLocation(), BeltTransform.GetLocation(), Blend);
	const FQuat Rotation = FQuat::Slerp(FloatTransform.GetRotation(), BeltTransform.GetRotation(), Blend);
	FloatPivot->SetWorldLocationAndRotation(Location, Rotation);
	FloatPivot->SetWorldScale3D(FVector(FMath::Lerp(1.f, BeltScale, Blend)));
}
