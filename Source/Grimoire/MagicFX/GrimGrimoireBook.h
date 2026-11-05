#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MagicFX/GrimMagicFXTypes.h"
#include "GrimGrimoireBook.generated.h"

class UStaticMesh;

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

/**
 * 왼손 위에 떠 있는 그리모어 (R21 · docs/systems/spell-feel 「왼손 그리모어」).
 * 서버가 스폰해 캐릭터 메시의 hand_l 에 Snap 으로 붙인다(부착 · 소유는 복제).
 * 떠다니기 · 책장 넘김 · 거두기 보간은 각 기기가 틱에서 로컬로 그린다 — 복제하지 않는다.
 * 넘김은 시전 GameplayCue(GC_Spell_Cast · GC_Spell_Channel)가 부른다: 시전자는 예측으로 바로, 다른 사람은 큐 복제로 본다.
 * M1.5 임시 모양은 엔진 기본 도형(Cube)이다. 수치는 기본값(UPROPERTY)이라 블루프린트 자식에서 바꾼다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimGrimoireBook : public AActor
{
	GENERATED_BODY()

public:
	AGrimGrimoireBook();

	virtual void Tick(float DeltaSeconds) override;

	/** 한 장 넘긴다. 넘기는 면의 글자가 Color 로 번쩍인다. 넘기는 중이면 이어서 넘긴다(최대 몇 장 쌓임). 문양은 마지막 시전의 것. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void FlipPage(FLinearColor Color);

	/**
	 * 시전 한 번에 한 장(R21 — 기본 공격 포함 모든 시전). 넘어간 면에 Element 의 속성 문양(UGrimBookSigilSettings)이 Color 로 번쩍인다.
	 * FlipDurationOverride > 0 이면 이 장은 그 시간에 넘긴다(기본 공격 0.15초 — VisualSet 의 PageFlipDuration).
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void FlipPageForSpell(FLinearColor Color, FGameplayTag Element, float FlipDurationOverride);

	/** 지금 펼친(왼쪽) 면에 보이는 문양 번호(UGrimBookSigilSettings::Sigils). 아직 없으면 INDEX_NONE */
	int32 GetShownSigilIndex() const { return LeftSigilIndex; }

	/** 넘기는 판 뒷면에 실린 문양 번호(넘기는 중일 때). 없으면 INDEX_NONE */
	int32 GetFlippingSigilIndex() const { return bFlipping ? CurrentFlipSigil : INDEX_NONE; }

	/** 지금 넘기는 장의 시간 */
	float GetCurrentFlipDuration() const { return CurrentFlipDuration; }

	/** 마법 선택이 SlotIndex 로 바뀌었다 — 그 쪽까지 촤르륵 넘긴다(처음 한 번은 넘기지 않고 기억만). */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void FlipTo(int32 SlotIndex, FLinearColor Color);

	/** 누르고 있는 마법(Stream): FlipRate 로 계속 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void BeginChannelFlip(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void EndChannelFlip();

	/** 조준 · 시전이 있었다(탈것 위에서 책을 띄워 두는 시간을 새로 센다). */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Book")
	void NotifyCombatActivity();

	/** 0 = 손 위에 떠 있음, 1 = 허리춤. 왼팔 「책 받치기」 무게가 1 − 이 값이다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Book")
	float GetStowAlpha() const { return StowAlpha; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Book")
	bool IsFlipping() const { return bFlipping; }

	/** 지금 펼친 마법 슬롯(FlipTo 로 마지막에 간 곳). */
	int32 GetCurrentSlot() const { return CurrentSlot; }

	/** 캐릭터가 서버에서 스폰한 뒤 부른다: 메시의 손에 Snap 으로 붙인다(복제됨). */
	void AttachToCharacterMesh(USkeletalMeshComponent* Mesh);

	/** 왼손 붙는 곳: 메시에 GrimoireFloat 소켓이 있으면 그것, 없으면 hand_l 뼈. */
	FName GetFloatAttachName(const USkeletalMeshComponent* Mesh) const;

protected:
	virtual void BeginPlay() override;

	void StartNextFlip();
	void QueueFlip(const FLinearColor& Color, float Duration);
	/** 문양 조각을 SigilIndex 의 모양으로 놓는다. bFlipSide = 넘기는 판 뒷면(다 넘기면 왼쪽 면 자리와 겹친다) */
	void ApplySigil(bool bFlipSide, int32 SigilIndex);
	void TickFlip(float DeltaSeconds);
	void TickStow(float DeltaSeconds);
	void TickPlacement();
	void ApplyGlow(float Glow, const FLinearColor& Color);
	FGrimGrimoireStowQuery GatherStowQuery() const;
	USkeletalMeshComponent* GetOwnerMesh() const;

	// ---- 모양 (엔진 기본 도형) ----

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<USceneComponent> Root;

	/** 떠다니기 · 거두기 위치가 실리는 곳. 아래 모든 모양이 여기 붙는다. 책 기준: +X = 책등 방향(앞), +Z = 펼친 면 */
	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<USceneComponent> FloatPivot;

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UStaticMeshComponent> CoverLeft;

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UStaticMeshComponent> CoverRight;

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UStaticMeshComponent> PagesLeft;

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UStaticMeshComponent> PagesRight;

	/** 넘기는 판이 도는 축(책등) */
	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<USceneComponent> FlipPivot;

	/** 넘기는 판 — 글자 면 발광이 실린다 */
	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UStaticMeshComponent> FlipPageMesh;

	/** 펼친 면의 은은한 빛 */
	UPROPERTY(VisibleAnywhere, Category = "Book")
	TObjectPtr<UPointLightComponent> GlowLight;

	/** 속성 문양 조각 — 왼쪽(펼친) 면 · 넘기는 판 뒷면. 엔진 기본 도형을 납작하게(UGrimBookSigilSettings) */
	UPROPERTY(VisibleAnywhere, Category = "Book")
	TArray<TObjectPtr<UStaticMeshComponent>> SigilLeftPieces;

	UPROPERTY(VisibleAnywhere, Category = "Book")
	TArray<TObjectPtr<UStaticMeshComponent>> SigilFlipPieces;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SigilCubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SigilDiscMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SigilTriangleMesh;

	// ---- 수치 (블루프린트 자식에서 조정) ----

	/** 붙는 뼈 · 소켓 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FName FloatSocketName = TEXT("GrimoireFloat");

	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FName FloatBoneName = TEXT("hand_l");

	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FName BeltSocketName = TEXT("GrimoireBelt");

	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FName BeltBoneName = TEXT("pelvis");

	/** 손바닥 위 높이(월드 위쪽, cm) — 소켓이 없을 때 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach", meta = (Units = "cm"))
	float FloatHeight = 12.f;

	/** 떠 있을 때 책을 읽는 쪽으로 기울이는 각(도). 앞 가장자리가 들린다 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach", meta = (Units = "deg"))
	float FloatPitch = 25.f;

	/** 허리춤 위치(캐릭터 기준: X 앞 · Y 오른쪽 · Z 위, pelvis 뼈에서) — 소켓이 없을 때 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FVector BeltOffset = FVector(2.f, -20.f, -6.f);

	/** 허리춤에서의 회전(캐릭터 기준) — 책을 세워 옆구리에 붙인다 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach")
	FRotator BeltRotation = FRotator(0.f, 0.f, -80.f);

	UPROPERTY(EditDefaultsOnly, Category = "Book|Attach", meta = (ClampMin = "0.1"))
	float BeltScale = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Float")
	FGrimBookFloatParams Float;

	/** 한 장 넘기는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "0.05", Units = "s"))
	float FlipDuration = 0.25f;

	/** 넘길 때 글자 면이 빛나는 시간 · 세기 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "0", Units = "s"))
	float FlipGlowDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "0"))
	float FlipGlowPeak = 8.f;

	/** 누르고 있는 마법에서 초당 넘기는 장 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "0"))
	float FlipRate = 2.f;

	/** 시전이 몰려도 쌓아 두는 최대 장 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "1"))
	int32 MaxQueuedFlips = 3;

	/** 마법 바꾸기 촤르륵: 슬롯 하나에 몇 장, 최대 몇 장, 전체 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "1"))
	int32 PagesPerSpellSlot = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "1"))
	int32 MaxFlipToPages = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Flip", meta = (ClampMin = "0.05", Units = "s"))
	float FlipToDuration = 0.5f;

	/** 거두기 · 꺼내기 보간 시간 (Q25) */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Stow", meta = (ClampMin = "0", Units = "s"))
	float StowBlendTime = 0.3f;

	/** 탈것 위에서 시전 뒤 책을 띄워 두는 시간 (Q25, MountedCombatHoldSeconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Stow", meta = (ClampMin = "0", Units = "s"))
	float MountedCombatHoldSeconds = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Look")
	FLinearColor CoverColor = FLinearColor(0.12f, 0.05f, 0.03f);

	UPROPERTY(EditDefaultsOnly, Category = "Book|Look")
	FLinearColor PageColor = FLinearColor(0.85f, 0.76f, 0.55f);

	/** 평소 펼친 면 빛 */
	UPROPERTY(EditDefaultsOnly, Category = "Book|Look")
	FLinearColor IdleGlowColor = FLinearColor(1.f, 0.8f, 0.45f);

	UPROPERTY(EditDefaultsOnly, Category = "Book|Look", meta = (ClampMin = "0"))
	float IdleGlow = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Look", meta = (ClampMin = "0"))
	float IdleLightIntensity = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Book|Look", meta = (ClampMin = "0"))
	float FlipLightIntensity = 900.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlipPageMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PagesMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SigilLeftMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SigilFlipMaterials;

	// 문양: 다음에 넘길 장 · 지금 넘기는 장 · 펼친 면
	int32 PendingFlipSigil = INDEX_NONE;
	int32 CurrentFlipSigil = INDEX_NONE;
	int32 LeftSigilIndex = INDEX_NONE;
	FLinearColor CurrentFlipColor = FLinearColor::White;
	FLinearColor LeftSigilColor = FLinearColor::White;

	float LocalTime = 0.f;

	// 넘김
	bool bFlipping = false;
	float FlipElapsed = 0.f;
	float CurrentFlipDuration = 0.25f;
	int32 CurrentFlipDirection = 1;
	int32 QueuedFlips = 0;          // 부호 = 방향
	float QueuedFlipDuration = 0.25f;
	FLinearColor FlipColor = FLinearColor::White;
	float SinceGlowStart = 1000.f;

	// 누르는 동안
	bool bChannelFlip = false;
	float ChannelFlipTimer = 0.f;

	// 마법 슬롯
	int32 CurrentSlot = INDEX_NONE;

	// 거두기
	float StowAlpha = 0.f;
	double LastCombatTime = -1000.0;
};
