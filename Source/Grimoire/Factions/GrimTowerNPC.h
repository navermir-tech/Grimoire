#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimTowerNPC.generated.h"

class APawn;
class APlayerController;
class UCapsuleComponent;
class UGrimDialogueWidget;
class UStaticMeshComponent;
struct FGrimNpcDef;
struct FGrimQuestLog;

/**
 * 마법탑 NPC (R08) — 탑의 스승 등. 이름 · 서비스는 TowerNpcs.csv, 대사는 TowerDialogue.csv(NpcId 로 찾는다).
 * 침대처럼 「다가가 키」(UGrimQuestSettings::TalkKey, 기본 F)로 말을 건다 — 마법 상호작용의 예외(사람과의 대화).
 * 키는 이 액터가 로컬 컨트롤러에서 직접 읽는다(캐릭터 입력을 고치지 않으려고). 대화 창은 UGrimDialogueWidget.
 * 대화를 마치면 서버에 TalkToNpc 신호, 서비스(승급 시험 · 정화 의식 · 대리 수리 · 지우기)는 UGrimQuestComponent 가 서버에서 처리한다.
 * 모양: 엔진 기본 도형(원통 몸 · 구 머리 · 원뿔 모자) 그레이박스. 탑은 안전 지대 — 조준 대상이 아니다.
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimTowerNPC : public AActor
{
	GENERATED_BODY()

public:
	AGrimTowerNPC();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	FName GetNpcId() const { return NpcId; }

	/** 표의 NPC 정의(이름 · 서비스). 표에 없으면 nullptr. */
	const FGrimNpcDef* GetNpcDef() const;

	FText GetDisplayName() const;

	bool OffersService(FName Service) const;

	/** 폰이 말을 걸 수 있는 거리인가(서버 검증은 Slack 을 더 준다) */
	bool IsWithinTalkRange(const APawn* Pawn, float Slack = 0.f) const;

	/** 맥락(Talk · 서비스 결과)의 대사 줄 — 진행에 맞는 행을 고른다. 게시판은 진행 목록을 덧붙인다. */
	virtual void BuildLines(FName Context, const FGrimQuestLog& Log, TArray<FString>& OutLines) const;

	/** NpcId 로 월드의 NPC 를 찾는다 */
	static AGrimTowerNPC* FindById(const UWorld* World, FName InNpcId);

	/** 대화 창이 닫힌 직후 같은 키로 다시 열리지 않게 */
	void BlockReopenFor(float Seconds);

protected:
	/** TowerNpcs.csv 의 행 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NpcId = TEXT("Master");

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> Hat;

	/** 몸 색(BasicShapeMaterial 의 Color) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FLinearColor RobeColor = FLinearColor(0.18f, 0.22f, 0.55f);

	void ApplyColors();
	void HandleLocalPlayer(APlayerController* PlayerController);

	/** 로컬 화면의 대화 창(화면이 있는 쪽에서만 만든다) */
	UPROPERTY(Transient)
	TObjectPtr<UGrimDialogueWidget> Dialogue;

	double ReopenBlockedUntil = 0.0;
	bool bPromptShown = false;
};

/** 퀘스트 게시판 — 말을 걸면(F) 진행 중인 퀘스트와 단계를 읽어 준다. 서비스 없음. 모양은 벽에 붙은 판. */
UCLASS()
class GRIMOIRE_API AGrimQuestBoard : public AGrimTowerNPC
{
	GENERATED_BODY()

public:
	AGrimQuestBoard();

	virtual void BuildLines(FName Context, const FGrimQuestLog& Log, TArray<FString>& OutLines) const override;
};
