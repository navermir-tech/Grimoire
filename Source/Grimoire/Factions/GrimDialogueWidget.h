#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimDialogueWidget.generated.h"

class AGrimTowerNPC;
class UBorder;
class UGrimQuestComponent;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * 마법탑 NPC 대화 창(C++ 위젯 트리, WBP 없음) — 아래 가운데 패널: 이름 · 직함, 대사 한 줄씩, 서비스 메뉴.
 *  - 열려 있는 동안 입력은 UI 전용(이동 · 시전이 멈춘다), 마우스 커서를 보인다.
 *  - 키: F · 스페이스 · Enter = 다음 / 메뉴에서 닫기, 1~9 = 메뉴 고르기, Esc · Tab = 닫기. 메뉴는 마우스로도 누른다.
 *  - 다가가면 대화 안내 한 줄(프롬프트)도 이 위젯이 보인다.
 */
UCLASS()
class GRIMOIRE_API UGrimDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPrompt(const FText& Text);
	void HidePrompt();

	bool IsConversationOpen() const { return Phase != EPhase::Closed; }
	AGrimTowerNPC* GetConversationNpc() const { return Npc.Get(); }

	void OpenConversation(AGrimTowerNPC* InNpc);
	void CloseConversation();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	enum class EPhase : uint8
	{
		Closed,
		Lines,
		Menu,
		Waiting
	};

	/** 줄을 다 읽은 뒤 */
	enum class EAfterLines : uint8
	{
		/** 대화를 마쳤다 → 서버에 Talk, 서비스가 있으면 메뉴 */
		FinishTalk,
		/** 메뉴로 돌아간다 */
		BackToMenu,
		/** 닫는다(시험 시작) */
		Close
	};

	void BuildTree();
	void ShowLines(const TArray<FString>& Lines, EAfterLines After);
	void ShowCurrentLine();
	void Advance();
	void ShowMenu();
	void ChooseService(int32 Index);
	void HandleNpcResponse(UGrimQuestComponent* Quest, AActor* ResponseNpc, FName Context);
	void SetFocusToSelf();
	UGrimQuestComponent* GetQuest() const;

private:
	EPhase Phase = EPhase::Closed;
	EAfterLines AfterLines = EAfterLines::FinishTalk;
	TArray<FString> PendingLines;
	int32 LineIndex = 0;
	TArray<FName> MenuServices;

	TWeakObjectPtr<AGrimTowerNPC> Npc;
	TWeakObjectPtr<UGrimQuestComponent> BoundQuest;
	FDelegateHandle ResponseHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LineText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MenuBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;
};
