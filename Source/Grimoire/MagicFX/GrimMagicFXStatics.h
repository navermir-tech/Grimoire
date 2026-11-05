#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "MagicFX/GrimMagicFXTypes.h"
#include "GrimMagicFXStatics.generated.h"

class UGrimSpellVisualSet;
class UGrimMagicFXVisualSet;
class UFXSystemAsset;
class UFXSystemComponent;
class USoundBase;
class UAudioComponent;
class UMaterialInterface;
class UCameraShakeBase;
class AGrimWand;
class AGrimGrimoireBook;
class UGrimSpellPoseAnimInstance;

/**
 * 큐 처리기 공용 — 큐 파라미터에서 연출 묶음 · 색 · 동작을 풀고, 이펙트 · 소리 · 자국을 띄운다.
 * 큐 파라미터 약속은 Core/GrimSpellFXTypes.h (SourceObject = UGrimSpellDefinition, Element.* = AggregatedSourceTags).
 */
UCLASS()
class GRIMOIRE_API UGrimMagicFXStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---- 풀기 ----

	/**
	 * 연출 묶음: ① SourceObject 가 VisualSet 그 자체 ② SourceObject 의 VisualSet 필드(이름으로 찾음 — 갈래 A 가 만든다)
	 * ③ 속성 태그 → 설정의 DefaultVisualSets ④ FallbackVisualSet. 시전자가 있으면 ⑤ 선택된 마법의 정의도 본다.
	 */
	static UGrimSpellVisualSet* ResolveVisualSet(const FGameplayCueParameters& Parameters, const AActor* CueTarget = nullptr);

	/** 객체의 VisualSet 필드(하드 · 소프트 참조 모두)를 리플렉션으로 읽는다. 객체가 VisualSet 이면 그대로. */
	static UGrimSpellVisualSet* FindVisualSetOnObject(const UObject* Object);

	/** 큐 파라미터의 Element.* 태그. 없으면 SourceObject(정의)의 Element. */
	static FGameplayTag FindElementTag(const FGameplayCueParameters& Parameters);

	/** 태그 묶음에서 Element.* 하나. */
	static FGameplayTag FindElementTagInContainer(const FGameplayTagContainer& Tags);

	/** 속성색: VisualSet → 정의의 Color → 흰색. */
	static FLinearColor ResolveColor(const UGrimSpellVisualSet* VisualSet, const FGameplayCueParameters& Parameters);

	/** 시전 동작: VisualSet 지정 → 정의의 Delivery 이름 → 속성. */
	static EGrimCastMotion ResolveCastMotion(const UGrimSpellVisualSet* VisualSet, const FGameplayCueParameters& Parameters);

	/** 정의(또는 아무 객체)의 float 필드를 이름으로. 없으면 Default. */
	static float ReadFloatProperty(const UObject* Object, FName PropertyName, float Default);

	/** 시전 폰: Instigator → 큐 대상 → 큐 대상의 Instigator. */
	static APawn* ResolveInstigatorPawn(AActor* CueTarget, const FGameplayCueParameters& Parameters);

	static AGrimWand* FindWand(const APawn* Pawn);
	static AGrimGrimoireBook* FindGrimoireBook(const APawn* Pawn);
	static UGrimSpellPoseAnimInstance* FindPoseAnimInstance(const APawn* Pawn);

	/** 완드 끝: 큐 Location 이 있으면 그것, 없으면 시전자 완드의 Muzzle. Normal 이 없으면 폰 조준 방향. */
	static void ResolveMuzzle(const APawn* Pawn, const FGameplayCueParameters& Parameters, FVector& OutLocation, FVector& OutDirection);

	/** 조준 방향: 폰의 GetBaseAimRotation(원격 폰도 복제된 피치가 있다). */
	static FVector GetAimDirection(const APawn* Pawn);

	// ---- 띄우기 (각 기기 로컬) ----

	/**
	 * Niagara · Cascade 어느 쪽이든 띄운다. 색은 Niagara 는 User.Color, Cascade 는 Color 파라미터로 넘긴다
	 * (엔진 기본 템플릿은 이 파라미터가 없어 무시되고, 나중에 바꿔 낄 이펙트가 받으면 바로 색이 입혀진다).
	 * StopAfterSeconds > 0 이면 그 뒤 멈춘다(반복 템플릿을 한 번만 쓰기 위해).
	 */
	static UFXSystemComponent* SpawnFX(UWorld* World, UFXSystemAsset* Asset, const FVector& Location, const FRotator& Rotation, float Scale, const FLinearColor& Color, bool bAutoDestroy, float StopAfterSeconds);

	/** 컴포넌트를 멈추고 다 사라지면 지운다. */
	static void StopFX(UFXSystemComponent* Component);

	static UAudioComponent* PlaySound(UWorld* World, USoundBase* Sound, const FVector& Location, float Volume, float Pitch, float StopAfterSeconds);

	/** 맞은 곳 자국. Normal 쪽으로 투영한다. */
	static void SpawnImpactDecal(UWorld* World, const FGrimImpactDecalSpec& Spec, const FVector& Location, const FVector& Normal);

	/** 시전자 본인 화면만 흔든다. */
	static void PlayCasterCameraShake(const APawn* Pawn, TSubclassOf<UCameraShakeBase> ShakeClass, float Scale);

	/** 색 펄스 · 줄기 구슬 머티리얼(설정 GlowMaterial, 없으면 엔진 BasicShapeMaterial). */
	static UMaterialInterface* GetGlowMaterial();

	/** 책 머티리얼(설정 BookMaterial, 없으면 엔진 BasicShapeMaterial). */
	static UMaterialInterface* GetBookMaterial();
};
