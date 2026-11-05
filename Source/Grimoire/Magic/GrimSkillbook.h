#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimSkillbook.generated.h"

class UGrimSpellDefinition;
class UStaticMeshComponent;
class URotatingMovementComponent;
class USphereComponent;
class UPrimitiveComponent;
struct FHitResult;

/**
 * 스킬북 (R05) — 월드에 놓인 책. 스킬북은 「아이템을 사용」해서 배우는데 인벤토리는 M4 라,
 * 지금은 플레이어가 책에 닿으면(겹침, 서버) 집자마자 쓰인다 → UGrimSpellbookComponent::UseSkillbook(정의).
 * 쓰였으면(배움 · 숙련도 경험) 책이 사라지고, 못 쓰면(서클 모자람 · 이미 S) 남는다.
 * 인벤토리가 생기면 이 액터는 「줍기 → 아이템(ItemType = Skillbook, TeachesSpell)」이 되고, 사용이 UseSkillbook 을 부른다.
 * 모양: 엔진 기본 상자(책 크기) + 속성색 표지, 천천히 돈다. 콘솔: Grim.Magic.ReadSkillbook · Grim.Magic.SpawnSkillbook.
 */
UCLASS()
class GRIMOIRE_API AGrimSkillbook : public AActor
{
	GENERATED_BODY()

public:
	AGrimSkillbook();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Skillbook")
	UGrimSpellDefinition* GetTeachesSpell() const { return TeachesSpell; }

	/** 서버 · 스폰 직후에만: 가르칠 마법을 바꾼다. */
	void SetTeachesSpell(UGrimSpellDefinition* Spell);

	/** 폰이 읽을 수 있는 거리 안인가(ReadRadius + Slack). */
	bool IsWithinReadRange(const APawn* Reader, float Slack = 0.f) const;

	float GetReadRadius() const { return ReadRadius; }

	/** 서버 검증 여유(클라이언트 위치 오차) */
	float GetServerReadSlack() const { return ServerReadSlack; }

	bool IsConsumed() const { return bConsumed; }

	/** 서버: 배웠다 → 책이 사라진다. */
	void ConsumeBook();

	/** 이 폰이 읽을 수 있는 가장 가까운 책. 없으면 nullptr. */
	static AGrimSkillbook* FindNearestReadable(const APawn* Reader);

protected:
	void RefreshCoverColor();

	/** 가르칠 마법 정의. 맵에 놓을 때 지정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TeachesSpell, Category = "Skillbook")
	TObjectPtr<UGrimSpellDefinition> TeachesSpell;

	/** 콘솔로 집거나 HUD 가 안내하는 거리(폰 위치 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skillbook", meta = (ClampMin = "0", Units = "cm"))
	float ReadRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skillbook", meta = (ClampMin = "0", Units = "cm"))
	float ServerReadSlack = 150.f;

	/** 정의가 없을 때 표지 색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skillbook")
	FLinearColor DefaultCoverColor = FLinearColor(0.35f, 0.22f, 0.12f);

	UFUNCTION()
	void OnRep_TeachesSpell();

	/** 서버: 폰이 닿으면 집는다 */
	UFUNCTION()
	void HandlePickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 폰이 닿으면 집는 반지름(겹침). 0 이면 겹침으로 집지 않는다(콘솔만). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skillbook", meta = (ClampMin = "0", Units = "cm"))
	float PickupRadius = 90.f;

	UPROPERTY(VisibleAnywhere, Category = "Skillbook")
	TObjectPtr<USphereComponent> PickupZone;

	UPROPERTY(VisibleAnywhere, Category = "Skillbook")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Skillbook")
	TObjectPtr<UStaticMeshComponent> BookMesh;

	UPROPERTY(VisibleAnywhere, Category = "Skillbook")
	TObjectPtr<URotatingMovementComponent> Spin;

	bool bConsumed = false;
};
