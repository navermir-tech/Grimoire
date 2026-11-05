#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

namespace GrimAppearanceComponentLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	/** 색 칸 이름 파라미터 전부 + 추가 파라미터 하나(자리 표시 · 마네킹)에 Channel 색 */
	void TintMaterials(UPrimitiveComponent* Component, const FLinearColor (&Colors)[GrimAppearanceChannelCount], EGrimAppearanceChannel Channel,
		FName ExtraParameter, int32 RingCount)
	{
		const UGrimAppearanceSettings* Settings = UGrimAppearanceSettings::Get();
		for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
		{
			UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(Index);
			if (!Material)
			{
				continue;
			}
			for (int32 ChannelIndex = 1; ChannelIndex < GrimAppearanceChannelCount; ++ChannelIndex)
			{
				Material->SetVectorParameterValue(FGrimAppearanceMath::ChannelParameterName(static_cast<EGrimAppearanceChannel>(ChannelIndex)), Colors[ChannelIndex]);
			}
			if (!ExtraParameter.IsNone())
			{
				Material->SetVectorParameterValue(ExtraParameter, Colors[static_cast<int32>(Channel)]);
			}
			if (!Settings->RingCountParameter.IsNone())
			{
				Material->SetScalarParameterValue(Settings->RingCountParameter, static_cast<float>(RingCount));
			}
		}
	}

	/** 마네킹 기준 자세에서 뼈의 컴포넌트 공간 변환(뼈가 없으면 원점) */
	FTransform RefPoseBoneTransform(const USkeletalMeshComponent* Mesh, FName Bone)
	{
		const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
		if (!Asset || Bone.IsNone())
		{
			return FTransform::Identity;
		}
		const FReferenceSkeleton& RefSkeleton = Asset->GetRefSkeleton();
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(Bone);
		return BoneIndex == INDEX_NONE ? FTransform::Identity : FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, BoneIndex);
	}
}

UGrimAppearanceComponent::UGrimAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrimAppearanceComponent, Appearance);
}

void UGrimAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Appearance.IsSet())
	{
		ApplyVisuals(GetDisplayedAppearance());
	}
}

void UGrimAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSpawned();
	Super::EndPlay(EndPlayReason);
}

void UGrimAppearanceComponent::SetAppearance(const FGrimAppearance& NewAppearance)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		UE_LOG(LogGrimAppearance, Warning, TEXT("%s: 외형은 서버만 바꾼다"), *GetNameSafe(GetOwner()));
		return;
	}
	if (Appearance == NewAppearance && (bBuiltBody || !NewAppearance.IsSet()))
	{
		return;
	}
	Appearance = NewAppearance;
	GetOwner()->ForceNetUpdate();
	if (!bPreviewing && HasBegunPlay())
	{
		ApplyVisuals(Appearance);
	}
}

void UGrimAppearanceComponent::OnRep_Appearance()
{
	if (bEndPreviewOnRep)
	{
		bEndPreviewOnRep = false;
		bPreviewing = false;
		PreviewValue = FGrimAppearance();
	}
	if (!bPreviewing)
	{
		ApplyVisuals(Appearance);
	}
}

void UGrimAppearanceComponent::PreviewAppearance(const FGrimAppearance& Preview)
{
	bPreviewing = true;
	PreviewValue = Preview;
	ApplyVisuals(PreviewValue);
}

void UGrimAppearanceComponent::ClearPreview()
{
	if (!bPreviewing)
	{
		return;
	}
	bPreviewing = false;
	PreviewValue = FGrimAppearance();
	ApplyVisuals(Appearance);
}

void UGrimAppearanceComponent::EndPreviewOnNextReplication()
{
	if (!bPreviewing)
	{
		return;
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClearPreview();
		return;
	}
	bEndPreviewOnRep = true;
}

void UGrimAppearanceComponent::SetLeaderMesh(USkeletalMeshComponent* InLeader)
{
	Leader = InLeader;
}

USkeletalMeshComponent* UGrimAppearanceComponent::GetLeaderMesh() const
{
	if (Leader)
	{
		return Leader;
	}
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}
	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

void UGrimAppearanceComponent::RebuildVisuals()
{
	ApplyVisuals(GetDisplayedAppearance());
}

void UGrimAppearanceComponent::RebuildAllInWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (UGrimAppearanceComponent* Component = It->FindComponentByClass<UGrimAppearanceComponent>())
		{
			Component->RebuildVisuals();
		}
	}
}

void UGrimAppearanceComponent::ClearSpawned()
{
	for (USceneComponent* Component : Spawned)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	Spawned.Reset();
	SkeletalPartCount = 0;
	StaticPieceCount = 0;
	RingCount = 0;
	bBuiltBody = false;
}

void UGrimAppearanceComponent::ApplyVisuals(const FGrimAppearance& Source)
{
	using namespace GrimAppearanceComponentLocal;
	AActor* Owner = GetOwner();
	USkeletalMeshComponent* LeaderMesh = GetLeaderMesh();
	if (!Owner || !LeaderMesh || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	ClearSpawned();
	if (!Source.IsSet())
	{
		OnAppearanceApplied.Broadcast(this);
		return;
	}

	const UGrimAppearanceSettings* Settings = UGrimAppearanceSettings::Get();
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	FLinearColor Colors[GrimAppearanceChannelCount];
	FGrimAppearanceMath::ResolveColors(Source, Catalog, Colors);
	const uint8 BodyType = FMath::Min<uint8>(Source.BodyType, 1);

	// 몸 — 체형 메시 · 키 · 피부색
	if (bDriveBodyMesh && Settings->BodyMeshes.IsValidIndex(BodyType))
	{
		USkeletalMesh* BodyMesh = Settings->BodyMeshes[BodyType].LoadSynchronous();
		if (BodyMesh && LeaderMesh->GetSkeletalMeshAsset() != BodyMesh)
		{
			LeaderMesh->EmptyOverrideMaterials();
			LeaderMesh->SetSkeletalMeshAsset(BodyMesh);
		}
	}
	if (!bCapturedBaseScale)
	{
		BaseLeaderScale = LeaderMesh->GetRelativeScale3D();
		bCapturedBaseScale = true;
	}
	const float HeightAlpha = Source.Height / 255.f;
	LeaderMesh->SetRelativeScale3D(BaseLeaderScale * FMath::Lerp(Settings->HeightRange.X, Settings->HeightRange.Y, HeightAlpha));
	for (int32 Index = 0; Index < LeaderMesh->GetNumMaterials(); ++Index)
	{
		const EGrimAppearanceChannel Channel = Settings->BodyMaterialChannels.IsValidIndex(Index) ? Settings->BodyMaterialChannels[Index] : EGrimAppearanceChannel::Skin;
		if (UMaterialInstanceDynamic* Material = LeaderMesh->CreateDynamicMaterialInstance(Index))
		{
			Material->SetVectorParameterValue(Settings->BodyTintParameter, Colors[static_cast<int32>(Channel)]);
			Material->SetVectorParameterValue(FGrimAppearanceMath::ChannelParameterName(EGrimAppearanceChannel::Skin), Colors[static_cast<int32>(EGrimAppearanceChannel::Skin)]);
		}
	}
	bBuiltBody = true;

	// 얼굴 슬라이더 값 — 모프 타깃(실제 머리 메시) · 자리 표시 크기
	FVector FaceScale = FVector::OneVector;
	for (int32 Index = 0; Index < Catalog.Morphs.Num(); ++Index)
	{
		const float Value = Source.GetMorph(Index);
		FaceScale += Catalog.Morphs[Index].Row.PlaceholderScale * Value;
		if (!Catalog.Morphs[Index].Row.MorphTarget.IsNone())
		{
			LeaderMesh->SetMorphTarget(Catalog.Morphs[Index].Row.MorphTarget, Value);
		}
	}

	const FGrimAppearancePart* Hat = Catalog.FindPart(Source.GetPart(EGrimAppearanceSlot::Hat));
	const bool bHideHair = Hat && Hat->Row.HidesHair;

	for (int32 SlotIndex = 0; SlotIndex < GrimAppearanceSlotCount && SlotIndex < Source.Parts.Num(); ++SlotIndex)
	{
		const EGrimAppearanceSlot Slot = static_cast<EGrimAppearanceSlot>(SlotIndex);
		const FGrimAppearancePart* Part = Catalog.FindPart(Source.Parts[SlotIndex]);
		if (!Part || (Slot == EGrimAppearanceSlot::Hair && bHideHair))
		{
			continue;
		}

		// 스켈레탈 조각 — 리더의 뼈를 그대로 따른다
		const TSoftObjectPtr<USkeletalMesh>& PartMeshRef = BodyType == 0 ? Part->Row.MeshA : Part->Row.MeshB;
		if (USkeletalMesh* PartMesh = PartMeshRef.IsNull() ? nullptr : PartMeshRef.LoadSynchronous())
		{
			USkeletalMeshComponent* PartComponent = NewObject<USkeletalMeshComponent>(Owner, NAME_None, RF_Transient);
			PartComponent->SetupAttachment(LeaderMesh);
			PartComponent->SetSkeletalMeshAsset(PartMesh);
			PartComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PartComponent->bUseBoundsFromLeaderPoseComponent = true;
			PartComponent->RegisterComponent();
			PartComponent->SetLeaderPoseComponent(LeaderMesh);
			TintMaterials(PartComponent, Colors, EGrimAppearanceChannel::Primary, NAME_None, Source.Circle);
			if (Slot == EGrimAppearanceSlot::Face)
			{
				for (int32 Index = 0; Index < Catalog.Morphs.Num(); ++Index)
				{
					if (!Catalog.Morphs[Index].Row.MorphTarget.IsNone())
					{
						PartComponent->SetMorphTarget(Catalog.Morphs[Index].Row.MorphTarget, Source.GetMorph(Index));
					}
				}
			}
			Spawned.Add(PartComponent);
			++SkeletalPartCount;
		}

		// 뼈에 붙는 소품(자리 표시 도형 · 모자 · 고리 · 주머니)
		for (const int32 PieceIndex : Part->Pieces)
		{
			const FGrimAppearancePiece& Piece = Catalog.Pieces[PieceIndex];
			if ((Piece.BodyMask & (1u << BodyType)) == 0)
			{
				continue;
			}
			UStaticMesh* PieceMesh = Piece.Row.Mesh.LoadSynchronous();
			if (!PieceMesh)
			{
				continue;
			}
			const FTransform BoneRef = RefPoseBoneTransform(LeaderMesh, Piece.Row.Bone);
			const bool bHasBone = !Piece.Row.Bone.IsNone() && LeaderMesh->GetBoneIndex(Piece.Row.Bone) != INDEX_NONE;
			const int32 Count = Piece.bRepeatCircle ? Source.Circle : 1;
			for (int32 Repeat = 0; Repeat < Count; ++Repeat)
			{
				const FVector Scale = Piece.Row.Face ? Piece.Row.Scale * FaceScale : Piece.Row.Scale;
				const FTransform Desired(Piece.Row.Rotation, BoneRef.GetLocation() + Piece.Row.Offset + Piece.Row.Step * Repeat, Scale);
				UStaticMeshComponent* PieceComponent = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
				PieceComponent->SetStaticMesh(PieceMesh);
				PieceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				PieceComponent->SetGenerateOverlapEvents(false);
				PieceComponent->SetupAttachment(LeaderMesh, bHasBone ? Piece.Row.Bone : NAME_None);
				PieceComponent->SetRelativeTransform(bHasBone ? Desired.GetRelativeTransform(BoneRef) : Desired);
				if (Piece.Row.Small && Settings->SmallPieceCullDistance > 0.f)
				{
					PieceComponent->SetCullDistance(Settings->SmallPieceCullDistance);
				}
				PieceComponent->RegisterComponent();
				TintMaterials(PieceComponent, Colors, Piece.Row.Channel, Settings->PlaceholderColorParameter, Source.Circle);
				Spawned.Add(PieceComponent);
				++StaticPieceCount;
				RingCount += Piece.bRepeatCircle ? 1 : 0;
			}
		}
	}

	OnAppearanceApplied.Broadcast(this);
}
