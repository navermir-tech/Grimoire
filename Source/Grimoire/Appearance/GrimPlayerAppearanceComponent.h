#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "Core/GrimSaveTypes.h"
#include "GrimPlayerAppearanceComponent.generated.h"

class APawn;
class APlayerController;
class APlayerState;
struct FOnAttributeChangeData;

/**
 * 플레이어 외형의 정본 — AGrimPlayerState 에 붙는다(폰이 바뀌어도 유지, docs/systems/customization 「흐름」).
 * 서버: 처음엔 무작위 견습생 한 벌(설정 PlayerRule), 세이브가 있으면 덮는다. 서클이 바뀌면 고리 수, 폰이 바뀌면 새 폰의 UGrimAppearanceComponent 에 넣는다.
 * 클라이언트: 생성 화면 결과를 Server_SubmitAppearance 로 보낸다(서버가 Sanitize — 플레이어가 고를 수 있는 조각 · 색만).
 * 세이브: Player 「Player.Appearance」 v1 = bool 고침 여부 + FGrimAppearance::Serialize(이름 형식).
 */
UCLASS(ClassGroup = (Grimoire))
class GRIMOIRE_API UGrimPlayerAppearanceComponent : public UActorComponent, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	UGrimPlayerAppearanceComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// IGrimSaveParticipant
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::Player; }
	virtual FName GetSaveId() const override { return TEXT("Player.Appearance"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	/** 서버의 정본(클라이언트에서는 비어 있다 — 폰의 외형 컴포넌트를 본다) */
	const FGrimAppearance& GetAppearance() const { return Appearance; }

	/** 생성 화면에서 확인한 적이 있나(소유 클라이언트에도 복제) */
	bool HasCustomized() const { return bCustomized; }

	/** 서버: 외형을 정하고 폰에 넣는다. 플레이어 규칙으로 Sanitize, 서클은 지금 서클로 */
	void SetAppearanceAuthority(const FGrimAppearance& NewAppearance, bool bMarkCustomized);

	/** 서버: 플레이어 규칙 무작위 한 벌(Seed 0 = 아무거나) */
	void RandomizeAuthority(int32 Seed);

	/** 로컬: 생성 화면 결과를 서버로(서버면 바로) */
	void SubmitAppearance(const FGrimAppearance& Requested);

	/** 로컬: 치트 무작위(개발 빌드) */
	void RequestRandomize(int32 Seed);

	static UGrimPlayerAppearanceComponent* Find(const APlayerState* PlayerState);
	static UGrimPlayerAppearanceComponent* FindForController(const APlayerController* Controller);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SubmitAppearance(const FGrimAppearance& Requested);

	UFUNCTION(Server, Reliable)
	void Server_Randomize(int32 Seed);

	UFUNCTION()
	void OnRep_Customized();

private:
	UFUNCTION()
	void HandlePawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	void HandleCircleChanged(const FOnAttributeChangeData& Data);

	/** 서버: 서클 어트리뷰트 → 고리 수 */
	int32 ReadCircle() const;

	/** 서버: 지금 폰의 외형 컴포넌트에 넣는다 */
	void PushToPawn();

	/** 로컬: 새 캐릭터면 생성 화면을 연다(설정 bOpenCreatorForNewCharacter) */
	void MaybeOpenCreator();

	FGrimAppearance Appearance;

	UPROPERTY(ReplicatedUsing = OnRep_Customized)
	bool bCustomized = false;

	FDelegateHandle CircleHandle;
	bool bCreatorOffered = false;
};
