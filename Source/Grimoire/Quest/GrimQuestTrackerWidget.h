#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/GrimQuestComponent.h"
#include "GrimQuestTrackerWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * 퀘스트 추적 창(C++ 위젯 트리) — 오른쪽 위: 진행 중인 퀘스트마다 제목 · 단계 설명 · 목표(센 값) · 남은 시간.
 * 그 아래 알림 한 줄(시작 · 단계 · 완료 · 실패, UGrimQuestSettings::NoticeSeconds 동안).
 * 값은 소유 클라이언트에 복제된 진행(UGrimQuestComponent::GetLog)을 읽기만 한다.
 */
UCLASS()
class GRIMOIRE_API UGrimQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetQuestComponent(UGrimQuestComponent* InQuest);

	/** 한 퀘스트의 추적 글(제목 줄 · 본문 줄) — 테스트 · 게시판도 쓴다 */
	static void DescribeProgress(const FGrimQuestDatabase& Database, const FGrimQuestProgress& Progress, float ServerTime, FString& OutTitle, FString& OutBody);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	void BuildTree();
	void Refresh();
	void HandleNotice(UGrimQuestComponent* Quest, EGrimQuestNotice Notice, FName QuestId, int32 Step);

private:
	TWeakObjectPtr<UGrimQuestComponent> Quest;
	FDelegateHandle NoticeHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> PanelRoot;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> List;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> TitleTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> BodyTexts;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;

	float NoticeRemaining = 0.f;
	float RefreshAccumulator = 0.f;
};
