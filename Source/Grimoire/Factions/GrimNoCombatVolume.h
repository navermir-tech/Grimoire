#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"
#include "GrimNoCombatVolume.generated.h"

class APawn;
class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;

namespace GrimFactionTags
{
	/** 전투 금지 구역 안(마법탑 실내 — 중립 지대, R08). 서버가 폰 ASC 에 복제 루스 태그로 붙인다. */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Zone_NoCombat);
}

/**
 * 전투 금지 볼륨 — 마법탑 실내(중립 지대). 폰이 하나라도 이 볼륨 안에 있으면 그 ASC 에 Zone.NoCombat(복제 루스 태그, 수 1)을, 모두 벗어나면 뗀다.
 * 태그만 붙인다: 시전 · 피해를 막는 것은 마법 쪽이 이 태그로 한다(TODO(통합)). 실내에서도 양초 · 아궁이 같은 상호작용 마법은 써야 하니
 * 「피해 · 적대 시전」만 막는 쪽이 맞다. 튜토리얼 수련실(과녁)과 앞마당(사냥 · 길들이기)은 볼륨 밖에 둔다.
 */
UCLASS()
class GRIMOIRE_API AGrimNoCombatVolume : public AActor
{
	GENERATED_BODY()

public:
	AGrimNoCombatVolume();

	virtual void BeginPlay() override;

	UBoxComponent* GetBounds() const { return Bounds; }

	/** 서버: 폰이 금지 구역 안인지 다시 보고 태그를 맞춘다 */
	static void RefreshPawn(APawn* Pawn);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UBoxComponent> Bounds;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
