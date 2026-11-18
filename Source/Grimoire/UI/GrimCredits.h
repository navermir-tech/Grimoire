#pragma once

#include "CoreMinimal.h"
#include "GrimCredits.generated.h"

/** CC BY 표기 하나 — Content/Grimoire/Data/Credits.json 의 Attributions 한 칸 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreditEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString Author;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString Asset;
	/** 받은 곳(Fab) */
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString Store;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString AssetUrl;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString License;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString LicenseUrl;
	/** 플레이어에게 보이는 「고친 점」 */
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString ModifiedKo;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString ModifiedEn;
	// DocUsedFor · DocChanges · Checked 는 문서 표에만 쓴다(게임은 읽지 않는다)
};

/** 크레딧 파일 전체 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCredits
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString Game;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString IntroKo;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") FString IntroEn;
	UPROPERTY(BlueprintReadOnly, Category = "Credits") TArray<FGrimCreditEntry> Attributions;
	/** 엔진 표기 등 그대로 옮겨 적는 줄 */
	UPROPERTY(BlueprintReadOnly, Category = "Credits") TArray<FString> Notices;
};

/**
 * 크레딧 읽기(Q137) — 정본은 Content/Grimoire/Data/Credits.json(경로는 UGrimUISettings::CreditsFile).
 * 같은 파일에서 docs/art/credits.md 의 표와 출시본 CREDITS.txt 를 Scripts/Docs/credits.mjs 가 만든다 — 줄 모양이 같아야 한다.
 */
struct GRIMOIRE_API FGrimCreditsData
{
	/** JSON 을 읽는다. 필수 칸(저작자 · 에셋 · 링크 · 라이선스 · 고친 점)이 빈 표기는 OutError 에 적는다(읽은 만큼은 남는다) */
	static bool Parse(const FString& Text, FGrimCredits& Out, FString& OutError);

	/** 한 번 읽어 둔 크레딧(못 읽으면 빈 것 + 로그 Error) */
	static const FGrimCredits& Get();

	/** 다시 읽게 한다 */
	static void Reload();

	/** CC BY 표기 한 줄(영어) — credits.mjs 의 attributionEn 과 같은 모양 */
	static FString MakeAttributionLine(const FGrimCreditEntry& Entry);

	/** 한국어 한 줄 — credits.mjs 의 attributionKo 와 같은 모양 */
	static FString MakeAttributionLineKo(const FGrimCreditEntry& Entry);
};
