#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Progression/GrimProgressionTypes.h"
#include "GrimSkillTreeWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UGrimIdButton;
class UGrimSkillTreeComponent;
class UTextBlock;

/**
 * 그리모어 「성장 트리」 탭 (R27) — 위젯 트리를 C++ 로 짠다.
 *  - 위: 트리 탭 4개(노말 · 4원소 · 빛 · 어둠 — UGrimSkillTreeSettings::Trees 순서) · 남은 포인트.
 *  - 가운데: 노드 격자(트리 표의 GridX · GridY), 선행 노드를 잇는 선, 갈래 이름. 노드 색 = 찍음(갈래색 꽉) · 찍을 수 있음(밝은 테) · 포인트 모자람(흐린 테) · 잠김(회색).
 *  - 아래: 고른 노드의 이름 · 설명 · 비용 · 서클 조건 · 선행, 찍기 버튼(서버 RPC — UGrimSkillTreeComponent::RequestUnlock).
 */
UCLASS()
class GRIMOIRE_API UGrimSkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 보여 줄 트리(표시 순서 번호) */
	void SetTree(int32 TreeIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	void BuildTree();
	void RebuildCanvas();
	void Refresh();
	void RefreshDetail(const UGrimSkillTreeComponent* Tree);

	void HandleTreeTabClicked(int32 TreeIndex);
	void HandleNodeClicked(int32 NodeIndex);

	UFUNCTION()
	void HandleUnlockClicked();

	UFUNCTION()
	void HandleUnlockResult(UGrimSkillTreeComponent* SkillTree, FName NodeId, EGrimSkillUnlockResult Result);

	UGrimSkillTreeComponent* GetSkillTree() const;

private:
	struct FNodeWidgets
	{
		FName Id;
		FName Branch;
		UGrimIdButton* Button = nullptr;
		UBorder* Frame = nullptr;
		UBorder* Fill = nullptr;
		UTextBlock* Name = nullptr;
	};

	struct FLineWidgets
	{
		FName From;
		FName To;
		UBorder* Line = nullptr;
	};

	TArray<FName> TreeOrder;
	TArray<UGrimIdButton*> TreeButtons;
	int32 TreeIndex = 0;

	UTextBlock* PointsText = nullptr;
	UCanvasPanel* Canvas = nullptr;
	TArray<FNodeWidgets> Nodes;
	TArray<FLineWidgets> Lines;

	UTextBlock* DetailName = nullptr;
	UTextBlock* DetailInfo = nullptr;
	UTextBlock* DetailBody = nullptr;
	UTextBlock* DetailState = nullptr;
	UButton* UnlockButton = nullptr;
	UTextBlock* ResultText = nullptr;

	FName SelectedNode;
	float RefreshTimer = 0.f;
	float ResultRemaining = 0.f;

	TWeakObjectPtr<UGrimSkillTreeComponent> BoundTree;
};
