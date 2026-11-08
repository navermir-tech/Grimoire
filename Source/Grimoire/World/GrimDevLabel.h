#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimDevLabel.generated.h"

class UWidgetComponent;

/**
 * 개발 표시용 떠 있는 글씨(레벨 디자인 그레이박스 — docs/dev/level-design.md 「7」).
 * 랜드마크 · POI 블록 위에 「무엇이 들어갈 자리인가」를, 마탑 구간 경계에 「공사 중」을 적는다.
 *
 * 한글이 보이게: TextRenderComponent 는 오프라인 폰트(로마자만 구운 텍스처)라 한글이 네모로 깨진다.
 * 그래서 월드 공간 위젯 컴포넌트에 Slate STextBlock 을 직접 넣는다 — Slate 기본 폰트(Roboto 합성 폰트)는
 * 없는 글자를 엔진 폴백 폰트(DroidSansFallback, 한글 포함)로 그린다. 위젯 블루프린트 에셋은 필요 없다.
 *
 * 숨기기: 출시(Shipping) 빌드에서는 늘 숨긴다. 개발 빌드는 콘솔 변수 Grim.DevLabels 0 으로 끈다
 * (기본값은 Config/DefaultEngine.ini [SystemSettings] 에서 바꿀 수 있다). 데디케이티드 서버는 위젯을 만들지 않는다.
 */
UCLASS()
class GRIMOIRE_API AGrimDevLabel : public AActor
{
	GENERATED_BODY()

public:
	AGrimDevLabel();

	/** 첫 줄(굵게, 색) — 예: 「거대 고목」 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel")
	FText Title;

	/** 설명(여러 줄) — 예: 「높이 40 m 둥근 초록 덩어리. 오르면 전망」 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel")
	FLinearColor TitleColor = FLinearColor(1.f, 0.82f, 0.25f);

	/** 글씨 크기 배율 — 1 이면 제목 줄 높이 약 0.6 m(위젯 1 픽셀 = 1 cm × 배율) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (ClampMin = "0.1"))
	float WorldScale = 2.f;

	/** 참이면 보는 사람(로컬 카메라) 쪽으로 돈다(Yaw 만) — 어느 쪽에서 와도 읽힌다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel")
	bool bFaceCamera = true;

	/** 보는 사람(로컬 카메라)에게서 이보다 멀면 숨긴다(cm) — 월드 공간 위젯은 반투명 판이라 겹치면 비싸다(docs/systems/performance 규칙 25) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (ClampMin = "1000"))
	float MaxVisibleDistance = 25000.f;

	/** 글씨를 감싸는 줄 폭(위젯 픽셀) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (ClampMin = "200"))
	float WrapWidth = 1400.f;

	/**
	 * 하늘의 구역 이름(레벨 디자인 2차 — 사용자가 구역을 알아보게). 참이면: 콘솔 Grim.DevLabels.Sections 로 따로 끄고 켜며,
	 * Grim.DevLabels.MaxDistance 덮어쓰기를 받지 않고(늘 MaxVisibleDistance), 어두운 판 없이 테두리 글씨로 그리고, 위아래로도 카메라를 본다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel")
	bool bSectionLabel = false;

	/** 제목 · 설명 글씨 크기(Slate 포인트). 0 이면 기본(제목 44 · 설명 30). 크게 하면 렌더 타깃 해상도가 올라 멀리서 덜 뭉개진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (ClampMin = "0"))
	int32 TitleFontSize = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grimoire|DevLabel", meta = (ClampMin = "0"))
	int32 BodyFontSize = 0;

	/** 지금 개발 표시를 보여 주는가(출시 빌드 = 늘 거짓, 개발 빌드 = Grim.DevLabels) */
	static bool AreLabelsEnabled();
	/** 하늘 구역 이름을 보여 주는가(출시 빌드 = 늘 거짓, 개발 빌드 = Grim.DevLabels && Grim.DevLabels.Sections) */
	static bool AreSectionLabelsEnabled();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Grimoire|DevLabel")
	TObjectPtr<UWidgetComponent> Widget;

private:
	/** 위젯을 만들 수 있는 월드인가(커맨드렛 · 데디케이티드 서버 · 출시 빌드 아님) */
	bool CanShowWidget() const;
	void BuildWidget();
	void ApplyVisibility(bool bInRange);
	/** 로컬 카메라 위치(없으면 거짓) */
	bool GetViewLocation(FVector& OutLocation) const;
	bool bShown = false;
	bool bVisibilityApplied = false;
};
