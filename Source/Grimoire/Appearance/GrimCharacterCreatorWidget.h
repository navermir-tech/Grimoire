#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "GrimCharacterCreatorWidget.generated.h"

class APlayerController;
class USlider;
class UTextBlock;
class UGrimAppearanceComponent;

/**
 * 최소 캐릭터 생성 화면(docs/spec/customization 「캐릭터 생성」) — C++ 로 짠 UMG.
 * 왼쪽 판: 칸마다 ◀ 값 ▶(체형 · 얼굴 · 머리 · 색 · 옷 조각 · 안감 속성), 키 · 얼굴 슬라이더, 무작위 · 확인 · 취소.
 * 고르는 동안은 내 폰에 미리보기(UGrimAppearanceComponent::PreviewAppearance — 내 화면만), 확인하면 서버로.
 * 여는 법: 치트 GrimCreator, 또는 새 캐릭터(설정 bOpenCreatorForNewCharacter). Esc = 취소.
 */
UCLASS()
class GRIMOIRE_API UGrimCharacterCreatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 컨트롤러 화면에 연다(이미 열려 있으면 그대로) */
	static UGrimCharacterCreatorWidget* Open(APlayerController* Controller);

	void Close(bool bConfirm);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** ◀ ▶ 한 줄이 바꾸는 것 */
	enum class ERow : uint8
	{
		Body, Slot, Skin, Eye, HairColor, ClothPrimary, ClothSecondary, Element
	};

	struct FCycleRow
	{
		ERow Kind = ERow::Body;
		EGrimAppearanceSlot Slot = EGrimAppearanceSlot::Face;
	};

	void BuildTree();
	void HandleButton(int32 Id);
	void Cycle(int32 RowIndex, int32 Step);
	void Randomize();
	void ApplyWorking(bool bResetFaceMorphs = false);
	void RefreshTexts();
	FText DescribeRow(const FCycleRow& Row) const;

	UFUNCTION()
	void HandleSliderChanged(float Value);

	UGrimAppearanceComponent* GetPawnAppearance() const;

	FGrimAppearance Working;
	TArray<FCycleRow> Rows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RowValues;

	UPROPERTY(Transient)
	TObjectPtr<USlider> HeightSlider;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USlider>> MorphSliders;

	bool bSyncingSliders = false;
	bool bClosed = false;
};
