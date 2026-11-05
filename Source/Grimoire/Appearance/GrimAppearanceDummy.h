#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimAppearanceDummy.generated.h"

class UGrimAppearanceComponent;
class USkeletalMeshComponent;

/**
 * 외형만 있는 사람 — 서 있는 애님 + UGrimAppearanceComponent. 콘솔 Grim.Appearance.Lineup 이 규칙대로 줄 세우고,
 * 나중에 군중 NPC(마을 주민 · 사냥꾼 순찰대)의 바탕이 된다. 충돌 · 상호작용 없음. 복제된다(외형은 서버가 정한다).
 */
UCLASS()
class GRIMOIRE_API AGrimAppearanceDummy : public AActor
{
	GENERATED_BODY()

public:
	AGrimAppearanceDummy();

	virtual void BeginPlay() override;

	UGrimAppearanceComponent* GetAppearanceComponent() const { return Appearance; }

	/** 서버: 규칙 + 씨앗으로 외형을 정한다 */
	void GenerateAppearance(FName Rule, int32 Seed);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> Figure;

	UPROPERTY(VisibleAnywhere, Category = "Appearance")
	TObjectPtr<UGrimAppearanceComponent> Appearance;
};
