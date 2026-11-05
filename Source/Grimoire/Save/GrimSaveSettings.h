#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimSaveSettings.generated.h"

/**
 * 세이브 v1 설정 (docs/systems/world-multiplayer 「세이브」). 값은 DefaultGame.ini 의 [/Script/Grimoire.GrimSaveSettings].
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Grimoire Save"))
class GRIMOIRE_API UGrimSaveSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimSaveSettings* Get() { return GetDefault<UGrimSaveSettings>(); }

	/** 끄면 저장 · 불러오기를 하지 않는다(콘솔도). 명령줄 -NoGrimSave 도 같다. */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	bool bEnabled = true;

	/** URL 옵션 ?SaveSlot= 이 없을 때의 세계 슬롯 */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	FString DefaultSlot = TEXT("Default");

	/** PIE 에서 쓸 슬롯. 비우면 PIE 에서는 저장하지 않는다(에디터 시험이 세이브에 섞이지 않게). */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	FString PIESlot;

	/**
	 * 무인 실행(-unattended: 헤드리스 확인 스크립트 · 자동화 테스트)에서는 URL 에 ?SaveSlot= 이 있을 때만 저장한다.
	 * 다른 확인 스크립트가 지난 실행의 세이브를 이어받아 결과가 바뀌지 않게. true 면 무인 실행도 기본 슬롯을 쓴다.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	bool bSaveWhenUnattendedWithoutSlot = false;

	/** Saved/ 아래 폴더. 슬롯마다 하위 폴더 하나(세계 파일 + 그 세계의 플레이어 파일). */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	FString SaveDirectory = TEXT("SaveGames/Grimoire");

	/** 자동 저장 간격(초, 실제 시간). 0 이하면 끈다. */
	UPROPERTY(config, EditAnywhere, Category = "Save", meta = (Units = "s"))
	float AutoSaveIntervalSeconds = 300.f;

	/** 다시 쓸 때 직전 파일을 .bak 으로 하나 남긴다. 본 파일이 깨졌으면 불러올 때 .bak 을 쓴다. */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	bool bKeepBackup = true;

	/** 플레이어 데이터를 읽는 순서 — 여기 있는 SaveId 가 이 순서로 먼저, 나머지는 찾은 순서(예: 서클을 마법서보다 먼저). */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	TArray<FName> PlayerLoadOrder;

	/** 세계 데이터를 읽는 순서(위와 같은 규칙) */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	TArray<FName> WorldLoadOrder;

	/**
	 * Player.Attributes 가 저장하는 어트리뷰트(이름 — "Mana" 또는 "GrimMagicAttributeSet.Mana"). 기본값(Base)을 저장한다.
	 * HP · 배고픔 · 체온 · 피곤함은 생존(Survival.Vitals), 서클은 Magic.Progression 이 맡는다 — 여기 넣지 않는다.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Player")
	TArray<FString> PlayerAttributes;

	/** 다시 접속하면 마지막 위치 · 방향으로(Player.Transform). 끄면 PlayerStart 에서 시작한다. */
	UPROPERTY(config, EditAnywhere, Category = "Player")
	bool bRestorePlayerTransform = true;

	/** 폰이 준비된 뒤 몇 틱 기다렸다 불러오나 — 마법서 · 생존 초기화(시작 마법 · 초기 수치)가 먼저 끝나게. */
	UPROPERTY(config, EditAnywhere, Category = "Player", meta = (ClampMin = "1"))
	int32 PlayerLoadDelayTicks = 2;
};
