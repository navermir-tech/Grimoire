#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Base/GrimBuilderComponent.h"
#include "GrimBaseWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 거점 화면(C++ 위젯 트리) — 로컬 플레이어마다 하나(UGrimBuilderComponent 가 띄운다).
 *  - 오른쪽: 건축 모드 패널(조각 목록 · 비용 · 가진 수, 고른 것 ▶, 키 안내, 지금 자리의 놓기 검사 결과).
 *  - 아래 가운데(가방 안내보다 위): 「[F] 문 열기」 같은 거점 상호작용 안내, 그 위 알림 한 줄.
 * 값은 건축 컴포넌트 · 복제된 가방을 읽기만 한다.
 */
UCLASS()
class GRIMOIRE_API UGrimBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetBuilder(UGrimBuilderComponent* InBuilder);

	/** 건축 패널 글(테스트 · 덤프도 쓴다) */
	static FString DescribePieceList(const UGrimBuilderComponent* Builder);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void BuildTree();
	void Refresh();
	void HandleNotice(UGrimBuilderComponent* InBuilder, EGrimBaseNotice Notice, FName Item, int32 CountOrResult);

	TWeakObjectPtr<UGrimBuilderComponent> Builder;
	FDelegateHandle NoticeHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BuildPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PieceListText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> PromptRoot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;

	float NoticeRemaining = 0.f;
	float RefreshAccumulator = 0.f;
};
