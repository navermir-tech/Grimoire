#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/GrimSaveTypes.h"
#include "GrimSaveGame.generated.h"

/**
 * 참여자 하나의 몫(docs/systems/world-multiplayer 「세이브」). 바이트는 참여자가 FArchive 로 직접 쓴 것 — 세이브 시스템은 뜻을 모른다.
 * 이 코드가 모르는 SaveId(다른 버전 · 아직 없는 시스템 · 스트리밍으로 안 올라온 액터)의 레코드도 다시 쓸 때 그대로 남긴다.
 */
USTRUCT()
struct GRIMOIRE_API FGrimSaveRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName SaveId;

	/** 쓸 때의 IGrimSaveParticipant::GetSaveVersion. 0 이하 = 잘못된 레코드 */
	UPROPERTY(SaveGame)
	int32 Version = 0;

	UPROPERTY(SaveGame)
	TArray<uint8> Bytes;
};

/**
 * 게임 중에 스폰된 저장 대상 액터(침대 · 스킬북 …). 불러오면 같은 이름 · 클래스 · 위치로 다시 스폰하고, 그 액터의 레코드를 FinishSpawning 전에 읽힌다.
 * 레코드를 이 안에 두므로 부서진 액터는 다음 저장에서 레코드와 함께 사라진다.
 */
USTRUCT()
struct GRIMOIRE_API FGrimSpawnedActorRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName ActorName;

	UPROPERTY(SaveGame)
	FSoftClassPath ActorClass;

	UPROPERTY(SaveGame)
	FTransform Transform;

	/** 그 액터와 컴포넌트의 레코드 */
	UPROPERTY(SaveGame)
	TArray<FGrimSaveRecord> Records;
};

/**
 * 세이브 파일 하나: 세계 WorldSave_<슬롯> 또는 플레이어 Player_<PlayerId>.
 * 파일은 UGameplayStatics::SaveGameToMemory 결과를 FGrimSaveFiles 의 봉투(매직 · 크기 · CRC)로 싸서 쓴다.
 */
UCLASS()
class GRIMOIRE_API UGrimSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 이 클래스 자체의 형식 버전(레코드 버전과 따로) */
	static constexpr int32 CurrentFormatVersion = 1;

	UPROPERTY(SaveGame)
	int32 FormatVersion = CurrentFormatVersion;

	UPROPERTY(SaveGame)
	EGrimSaveScope Scope = EGrimSaveScope::World;

	UPROPERTY(SaveGame)
	FString Slot;

	/** 플레이어 세이브만 */
	UPROPERTY(SaveGame)
	FString PlayerId;

	/** 플레이어 세이브만 — 사람이 알아보기 위한 마지막 이름(식별에는 쓰지 않음) */
	UPROPERTY(SaveGame)
	FString PlayerName;

	UPROPERTY(SaveGame)
	FDateTime SavedAtUtc;

	/** 저장한 게임 버전(GeneralProjectSettings.ProjectVersion) — 참고용 */
	UPROPERTY(SaveGame)
	FString ProjectVersion;

	/** 저장한 맵(참고용) */
	UPROPERTY(SaveGame)
	FString MapName;

	/** 이 파일이 몇 번째 저장인가 */
	UPROPERTY(SaveGame)
	int32 SaveCount = 0;

	UPROPERTY(SaveGame)
	TArray<FGrimSaveRecord> Records;

	/** 세계만: 레벨에 놓였다가 게임 중에 없어진 액터 이름(집은 스킬북 …) — 불러오면 다시 없앤다 */
	UPROPERTY(SaveGame)
	TArray<FName> RemovedActors;

	/** 세계만: 게임 중에 스폰된 저장 대상 액터 */
	UPROPERTY(SaveGame)
	TArray<FGrimSpawnedActorRecord> SpawnedActors;
};
