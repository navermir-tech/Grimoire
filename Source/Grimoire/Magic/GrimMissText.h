#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimMissText.generated.h"

class UTextRenderComponent;

/**
 * 지침 빗나감 「MISS」 피해 숫자(Q132). 시전자 화면에만 로컬로 띄우는 연출 액터 — 복제하지 않는다.
 * 맞은 자리 위에서 떠올라 카메라를 보고, 수명 끝에 줄어들며 사라진다. 수치는 UGrimMagicSettings::MissText.
 * 서버가 빗나감을 정하고 UGrimSpellbookComponent::ClientShowMiss 로 자리만 보낸다.
 */
UCLASS(NotBlueprintable)
class GRIMOIRE_API AGrimMissText : public AActor
{
	GENERATED_BODY()

public:
	AGrimMissText();

	/** 이 기기에 「MISS」 하나를 띄운다(전용 서버에서는 아무것도 안 한다). */
	static AGrimMissText* Spawn(UWorld* World, const FVector& Location);

	virtual void Tick(float DeltaSeconds) override;

	UTextRenderComponent* GetText() const { return Text; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Miss")
	TObjectPtr<UTextRenderComponent> Text;

	float Age = 0.f;
	float Lifetime = 1.f;
	float RiseSpeed = 80.f;
};
