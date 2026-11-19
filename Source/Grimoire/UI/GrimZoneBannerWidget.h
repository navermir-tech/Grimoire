#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/GrimZoneSettings.h"
#include "World/GrimZoneTypes.h"
#include "GrimZoneBannerWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/**
 * 구역 이름 배너(C++ 위젯 트리) — 화면 위쪽 가운데에 구역 이름 + 가는 줄 + 한 줄 설명을 크게 띄웠다가 사라진다(젤다 · 엘든 링식).
 * 처음 · 다시 × 큰 구역 · 소구역마다 모양(시간 · 크기 · 설명 보이기)은 UGrimZoneSettings. 여러 개가 오면 차례로 띄운다.
 * UGrimZoneDiscoveryComponent 가 소유 클라이언트에 만든다(입력을 받지 않는다).
 */
UCLASS()
class GRIMOIRE_API UGrimZoneBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 띄울 배너를 줄에 넣는다(지금 「다시」 배너가 떠 있고 이것이 「처음」이면 지금 것을 빨리 끈다) */
	void Enqueue(const FGrimZoneDef& Zone, bool bFirstVisit);

	/** 지금 떠 있는 배너의 구역(없으면 None) */
	FName GetShowingZone() const { return bActive ? Showing.Zone : NAME_None; }

	/** 기다리는 배너 수 */
	int32 GetQueuedCount() const { return Queue.Num(); }

	/** 지난 시간 → 불투명도(0~1) — 테스트도 쓴다 */
	static float ComputeAlpha(const FGrimZoneBannerStyle& Style, float Elapsed);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FEntry
	{
		FName Zone;
		FText Title;
		FText Subtitle;
		EGrimZoneTier Tier = EGrimZoneTier::Region;
		bool bFirstVisit = false;
	};

	void BuildTree();
	void Begin(const FEntry& Entry);
	void ApplyAlpha(float Alpha);

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Column;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> RuleBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	TArray<FEntry> Queue;
	FEntry Showing;
	FGrimZoneBannerStyle Style;
	float Elapsed = 0.f;
	bool bActive = false;
};
