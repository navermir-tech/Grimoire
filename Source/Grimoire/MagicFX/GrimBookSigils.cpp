#include "MagicFX/GrimBookSigils.h"

const FGrimBookSigil* UGrimBookSigilSettings::FindSigil(FName ElementTagName) const
{
	const int32 Index = GrimBookSigilMath::FindSigilIndex(Sigils, ElementTagName, FallbackElement);
	return Sigils.IsValidIndex(Index) ? &Sigils[Index] : nullptr;
}

namespace GrimBookSigilMath
{
	int32 FindSigilIndex(TConstArrayView<FGrimBookSigil> Sigils, FName ElementTagName, FName FallbackName)
	{
		const FString Name = ElementTagName.ToString();
		int32 Best = INDEX_NONE;
		int32 BestLength = -1;
		for (int32 Index = 0; Index < Sigils.Num(); ++Index)
		{
			const FString Candidate = Sigils[Index].Element.ToString();
			if (Candidate.IsEmpty() || Sigils[Index].Element.IsNone())
			{
				continue;
			}
			const bool bExact = Name.Equals(Candidate, ESearchCase::IgnoreCase);
			const bool bParent = Name.Len() > Candidate.Len() && Name.StartsWith(Candidate + TEXT("."), ESearchCase::IgnoreCase);
			if ((bExact || bParent) && Candidate.Len() > BestLength)
			{
				Best = Index;
				BestLength = Candidate.Len();
			}
		}
		if (Best == INDEX_NONE && !FallbackName.IsNone() && FallbackName != ElementTagName)
		{
			return FindSigilIndex(Sigils, FallbackName, NAME_None);
		}
		return Best;
	}

	FTransform MakePieceTransform(const FGrimSigilPiece& Piece, float Thickness)
	{
		const float T = FMath::Max(Thickness, 0.01f);
		const FVector Location(Piece.Offset.X, Piece.Offset.Y, T * 0.5f);
		const FQuat Yaw = FRotator(0.f, Piece.Yaw, 0.f).Quaternion();
		switch (Piece.Shape)
		{
		case EGrimSigilShape::Triangle:
		{
			// Cone: 높이 = 로컬 Z, 밑면 지름 = 로컬 X · Y. 피치 −90° 로 눕히면 로컬 Z → +X(꼭짓점), 로컬 X → 두께(Z).
			const FQuat LieDown = FRotator(-90.f, 0.f, 0.f).Quaternion();
			return FTransform(Yaw * LieDown, Location, FVector(T, Piece.Size.Y, Piece.Size.X) / 100.f);
		}
		case EGrimSigilShape::Disc:
		case EGrimSigilShape::Square:
		default:
			return FTransform(Yaw, Location, FVector(Piece.Size.X, Piece.Size.Y, T) / 100.f);
		}
	}
}
