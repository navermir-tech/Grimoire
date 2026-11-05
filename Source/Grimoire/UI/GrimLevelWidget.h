#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimLevelWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 레벨 · 경험 막대 (R27) — 위젯 트리를 C++ 로 짠다. 「Lv 5」 · 경험 막대 · 「경험 1,230 / 2,820」 · 남은 성장 트리 포인트.
 * 레벨이 오르면 「레벨 업!」을 잠깐 띄운다. HUD 에는 통합 때 끼운다 — 그전에는 콘솔 Grim.UI.Level [0|1] 로 단독으로 띄운다(화면 아래 가운데).
 */
UCLASS()
class GRIMOIRE_API UGrimLevelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 로컬 플레이어마다 단독으로 띄우기 · 내리기(콘솔 Grim.UI.Level). 그리지 못하는 프로세스에서는 아무것도 안 한다. */
	static void SetStandaloneVisible(bool bVisible);
	static bool IsStandaloneVisible();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildTree();
	void Refresh();

private:
	UTextBlock* LevelText = nullptr;
	UProgressBar* XPBar = nullptr;
	UTextBlock* XPText = nullptr;
	UTextBlock* PointsText = nullptr;
	UTextBlock* LevelUpText = nullptr;

	/** 마지막으로 본 레벨(INDEX_NONE = 아직 못 봄 — 첫 값은 알림 없이 받는다) */
	int32 LastLevel = INDEX_NONE;
	float LevelUpRemaining = 0.f;
	/** 처음 이만큼은 레벨이 바뀌어도 알리지 않는다(접속 직후 복제로 레벨이 한꺼번에 들어오는 것) */
	float Grace = 2.f;
	float RefreshTimer = 0.f;
};
