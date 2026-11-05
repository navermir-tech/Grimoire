#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Creatures/GrimCreatureStorageTypes.h"
#include "GrimCreatureStorageWidget.generated.h"

class AGrimCreatureBox;
class UGrimCreatureStorageComponent;
class UGrimTamerComponent;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** 목록 한 줄이 보여 주는 값(테스트 · 덤프도 쓴다) */
struct GRIMOIRE_API FGrimCreatureRowView
{
	EGrimCreaturePlace Place = EGrimCreaturePlace::Party;
	FGrimCreatureInstance Instance;
	float HealthRatio = 1.f;
	bool bKnockedOut = false;
};

/**
 * 생물 목록 화면(C++ 위젯 트리, R38) — 두 가지로 쓴다.
 *  - 가방 「생물」 탭(UGrimInventoryWidget 이 안에 넣는다): 데리고 다님 · 보관함 두 칸. 줄마다 이름 · 종 · 성별 · 색 칸 · 크기 · 충성도 · 체력 + 넣기 · 꺼내기 단추.
 *  - 거점 생물 상자 관리 화면(SetStandaloneScreen — UGrimCreatureStorageComponent::OpenBoxScreen): 세 칸(+ 상자) · 옮기기 · 풀어놓기 · 거두기. F · Esc 로 닫는다.
 * 값은 복제된 것(내 생물 액터 · 보관함 기록 · 상자 칸)을 읽기만 하고, 바꾸는 것은 서버 RPC(RequestTransfer).
 */
UCLASS()
class GRIMOIRE_API UGrimCreatureStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 화면 가운데 패널로(상자 화면). 위젯을 화면에 붙이기 전에 */
	void SetStandaloneScreen(bool bInStandalone) { bStandalone = bInStandalone; }

	void SetStorage(UGrimCreatureStorageComponent* InStorage);

	/** 상자 화면 열기 · 닫기 */
	void OpenScreen(AGrimCreatureBox* InBox);
	void CloseScreen();
	bool IsScreenOpen() const { return bOpen; }

	/** 가방 탭이 보이게 · 숨게 될 때 */
	void SetTabActive(bool bActive);

	/** 이 플레이어가 보는 줄(데리고 다님 → 보관함 → 상자). Box 가 없으면 상자 줄 없음 */
	static TArray<FGrimCreatureRowView> GatherRows(const UGrimCreatureStorageComponent* Storage, const AGrimCreatureBox* Box);

	/** 줄 설명 두 줄: 「이름 (종) · 수」 / 「크기 ×1.05 거대 · 충성도 72 충직 · 체력 80%」 */
	static FString DescribeRow(const FGrimCreatureRowView& Row);

	/** 그 줄에서 갈 수 있는 곳(단추) */
	static TArray<EGrimCreaturePlace> GetTargets(EGrimCreaturePlace From, bool bHasBox);

	/** 단추 글 */
	static FString TargetLabel(EGrimCreaturePlace From, EGrimCreaturePlace To);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void BuildTree();
	void Refresh();
	void RebuildRows(const TArray<FGrimCreatureRowView>& Views);
	void HandleRowButton(int32 Id);
	void HandleNotice(UGrimTamerComponent* Tamer, const FString& Text);
	UGrimCreatureStorageComponent* ResolveStorage();
	void BindTamer();

	TWeakObjectPtr<UGrimCreatureStorageComponent> Storage;
	TWeakObjectPtr<AGrimCreatureBox> Box;
	TWeakObjectPtr<UGrimTamerComponent> BoundTamer;
	FDelegateHandle NoticeHandle;

	bool bStandalone = false;
	bool bOpen = false;
	bool bTabActive = false;
	double OpenedAt = 0.0;
	float RefreshAccum = 0.f;
	float NoticeRemaining = 0.f;
	FString LastSignature;

	struct FRowKey
	{
		EGrimCreaturePlace Place = EGrimCreaturePlace::Party;
		FName SpeciesId;
		int32 Seed = 0;
		TArray<EGrimCreaturePlace> Targets;
	};
	TArray<FRowKey> RowKeys;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> PanelRoot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StorageHint;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PartyList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> StorageList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BoxList;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BoxColumn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RowTexts;
};
