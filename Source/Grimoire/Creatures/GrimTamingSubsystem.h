#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Creatures/GrimTamingTypes.h"
#include "GrimTamingSubsystem.generated.h"

class AGrimCreature;
class APlayerState;
class UAbilitySystemComponent;
class UGrimSpellDefinition;
class UGrimTamerComponent;

/**
 * 길들이기(서버 월드 서브시스템, 갈래 T).
 *  - 플레이어 PlayerState 에 UGrimTamerComponent(가진 개체 · 명령 RPC · 세이브)를 런타임에 붙인다(가방과 같은 방식).
 *  - 파크툼이 맞으면 TryTame: 포획 확률(GrimTamingMath::CalcCaptureChance) 굴림 → 성공이면 주인 · 충성도 · 개체군에서 빼기 · 퀘스트 Creature.Tamed.<분류>,
 *    실패면 저항(화내 덤빈다).
 *  - 생물 죽음을 듣고 「함께 싸워 이김」 · 길들인 개체가 죽으면 주인 기록에서 지운다.
 */
UCLASS()
class GRIMOIRE_API UGrimTamingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	static UGrimTamingSubsystem* Get(const UObject* WorldContext);

	/** 서버: PlayerState 에 길들이기 컴포넌트를 (없으면) 붙인다 */
	UGrimTamerComponent* EnsureTamer(APlayerState* PlayerState);

	/** 서버: 그 플레이어의 PlayerId(세이브와 같은 값 → 없으면 로그인 옵션 · 이름) */
	static FString ResolvePlayerId(const APlayerState* PlayerState);

	/** 서버: 파크툼 명중 → 포획 판정. OutChance 에 몫별 확률 */
	EGrimTamingResult TryTame(const UGrimSpellDefinition* Spell, UAbilitySystemComponent* CasterASC, AGrimCreature* Creature, FGrimCaptureChance* OutChance = nullptr);

	/** 포획 확률 입력을 모은다(서버) */
	static FGrimCaptureInput GatherCaptureInput(const UGrimSpellDefinition* Spell, const UAbilitySystemComponent* CasterASC, const AGrimCreature* Creature);

	/** 파크툼 속성이 생물과 맞나(잘 듣는 파크툼 · 주 · 부 — 무속성 · 빈 태그는 안 맞음) */
	static bool IsPactElementMatch(const FGameplayTag& PactElement, const AGrimCreature* Creature);

	/** 서버: 놓아줌 · 떠남 — 옛 주인의 기록을 지운다 */
	void NotifyReleased(AGrimCreature* Creature, const FString& FormerOwnerId, bool bLeftByLoyalty);

	/** 서버: 이 PlayerId 가 가진, 세계에 있는 생물 */
	TArray<AGrimCreature*> GetOwnedCreatures(const FString& OwnerId) const;

	UGrimTamerComponent* FindTamerById(const FString& PlayerId) const;

	/** 개발용: 굴림을 고정(0~1, 음수 = 끔). 콘솔 Grim.Taming.ForceRoll */
	static float GetForcedRoll();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;
	void HandleActorSpawned(AActor* Actor);
	void HandleCreatureDied(AGrimCreature* Creature, AActor* Killer);

	FDelegateHandle ActorSpawnedHandle;
};
