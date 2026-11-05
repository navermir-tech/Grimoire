#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Creatures/GrimTamerComponent.h"
#include "GrimCreatureMenuWidget.generated.h"

class AGrimCreature;
class UEditableTextBox;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * 길들인 생물 메뉴(C++ 위젯 트리, WBP 없음 — 마법탑 NPC 대화 창과 같은 방식). 아래 가운데 패널:
 *  이름 · 종 · 성격 · 충성도 막대와 단계 · 명령 · 「최근에 오르내린 까닭」 세 줄(스펙 「충성도」 화면), 그 아래 메뉴.
 *  - 메뉴: 1 따라오기 · 2 기다리기 · 3 타기(탈것) · 4 먹이 주기 · 5 이름 짓기 · 6 놓아주기(두 번 눌러 확인) · Esc/F 닫기.
 *  - 열려 있는 동안 입력은 UI 전용(이동 · 시전이 멈춘다). 이름 짓기는 글 상자에 쓰고 Enter.
 *  - 다가가면 안내 한 줄(프롬프트) · 알림 한 줄도 이 위젯이 보인다.
 */
UCLASS()
class GRIMOIRE_API UGrimCreatureMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTamer(UGrimTamerComponent* InTamer) { Tamer = InTamer; }

	void ShowPrompt(const FString& Text);
	void HidePrompt();
	void ShowNotice(const FString& Text);

	void OpenMenu(AGrimCreature* Creature);
	void CloseMenu();
	bool IsMenuOpen() const { return bOpen; }
	AGrimCreature* GetMenuCreature() const { return Creature.Get(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum class EEntry : uint8
	{
		Follow,
		Wait,
		Mount,
		Feed,
		Rename,
		Release,
		Close
	};

	void BuildTree();
	void RebuildMenu();
	void RefreshInfo();
	void Choose(int32 Index);
	void BeginRename();
	void ConfirmRename();
	void FocusSelf();

	UFUNCTION()
	void HandleRenameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	TWeakObjectPtr<UGrimTamerComponent> Tamer;
	TWeakObjectPtr<AGrimCreature> Creature;
	TArray<EEntry> Entries;
	bool bOpen = false;
	bool bRenaming = false;
	bool bReleaseArmed = false;
	float NoticeRemaining = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> LoyaltyBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LoyaltyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReasonsText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MenuBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RenameBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintText;
};
