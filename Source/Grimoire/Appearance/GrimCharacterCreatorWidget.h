#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "GrimCharacterCreatorWidget.generated.h"

class ACameraActor;
class APlayerController;
class USlider;
class UTextBlock;
class UGrimAppearanceComponent;
struct FGrimCreatorCameraTuning;

/** 생성 화면 카메라가 무엇을 비추나(Q102) */
enum class EGrimCreatorFocus : uint8
{
	/** 전신 — 몸 · 옷 칸 */
	Body,
	/** 얼굴 가까이 — 얼굴 · 머리 · 수염 · 피부 · 눈 · 얼굴 슬라이더 */
	Face
};

/** 생성 화면 카메라 순수 계산(테스트 Grimoire.Appearance.CreatorCamera) */
struct GRIMOIRE_API FGrimCreatorCameraMath
{
	/** 칸 → 비출 곳: 얼굴 · 머리 · 수염은 얼굴, 나머지(모자 포함 옷)는 전신 */
	static EGrimCreatorFocus FocusForSlot(EGrimAppearanceSlot Slot);

	/**
	 * 카메라 자리: 캐릭터 정면(FrontYaw = 캐릭터가 보는 방향)에서 OrbitYaw 만큼 돌고 Pitch 로 내려다보며 Pivot 을 Distance 만큼 떨어져 본다.
	 * Lateral 만큼 카메라 기준 왼쪽을 겨눠 캐릭터가 화면 오른쪽에 온다(왼쪽 판에 가리지 않게).
	 */
	static FTransform ComputePose(const FVector& Pivot, float FrontYaw, float OrbitYaw, float Pitch, float Distance, float Lateral);
};

/**
 * 최소 캐릭터 생성 화면(docs/spec/customization 「캐릭터 생성」) — C++ 로 짠 UMG.
 * 왼쪽 판: 칸마다 ◀ 값 ▶(체형 · 얼굴 · 머리 · 색 · 옷 조각 · 안감 속성), 키 · 얼굴 슬라이더, 무작위 · 확인 · 취소.
 * 고르는 동안은 내 폰에 미리보기(UGrimAppearanceComponent::PreviewAppearance — 내 화면만), 확인하면 서버로.
 * 여는 법: 치트 GrimCreator, 또는 새 캐릭터(설정 bOpenCreatorForNewCharacter). Esc = 취소. 마법탑 거울(뒤)도 이 화면을 연다.
 * 카메라(Q102): 열면 캐릭터 앞 정면 카메라(이 기기만의 ACameraActor)로 바꾼다 — 얼굴 칸을 만지면 얼굴 가까이, 몸 · 옷 칸이면 전신,
 * 판 밖 빈 곳을 마우스로 끌면 돌려 본다. 닫으면 평소 카메라로 돌아간다. 수치는 설정 CreatorCamera.
 */
UCLASS()
class GRIMOIRE_API UGrimCharacterCreatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 컨트롤러 화면에 연다(이미 열려 있으면 그대로) */
	static UGrimCharacterCreatorWidget* Open(APlayerController* Controller);

	void Close(bool bConfirm);

	/** 지금 열린 생성 화면(없으면 null) — 디버그 콘솔 Grim.Appearance.CreatorFocus · CreatorOrbit */
	static UGrimCharacterCreatorWidget* GetOpen();

	/** 카메라가 비출 곳을 바꾼다(칸을 만지면 저절로) */
	void SetFocus(EGrimCreatorFocus NewFocus);
	EGrimCreatorFocus GetFocus() const { return Focus; }

	/** 정면에서 도는 각 · 내려다보는 각(도). 끌기와 같다 */
	void SetOrbit(float Yaw, float Pitch);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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

	/** 카메라: 열 때 만들고 닫을 때 평소 카메라로 돌린다 */
	void BeginCreatorCamera();
	void EndCreatorCamera();
	/** 지금 비출 곳 · 각으로 원하는 카메라 자리 */
	bool ComputeDesiredPose(FTransform& OutPose) const;

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

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CreatorCamera;

	EGrimCreatorFocus Focus = EGrimCreatorFocus::Body;
	/** 연 순간 캐릭터가 보던 방향(정면) */
	float FrontYaw = 0.f;
	float OrbitYaw = 0.f;
	float OrbitPitch = 0.f;
	bool bDragging = false;
	bool bCameraSnapped = false;
};
