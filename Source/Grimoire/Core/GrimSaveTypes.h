#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GrimSaveTypes.generated.h"

/** 저장되는 몫이 어디에 붙나 (docs/systems/world-multiplayer 「세이브」). */
UENUM(BlueprintType)
enum class EGrimSaveScope : uint8
{
	/** 플레이어 세이브 Player_<PlayerId> — 서클 · 마법 · 숙련도 · 레벨 · 성장 트리 · 생존 · 부활 지점 … (PlayerState 와 그 컴포넌트) */
	Player,
	/** 세계 세이브 WorldSave_<슬롯> — 시간 · 날씨 · 사물 상태 · 놓인 액터 … (월드의 액터 · 서브시스템) */
	World
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGrimSaveParticipant : public UInterface
{
	GENERATED_BODY()
};

/**
 * 세이브에 참여하는 쪽. 액터 · 컴포넌트 · 서브시스템이 구현한다.
 * 세이브 시스템(Save 폴더)은 구현체를 몰라도 이 인터페이스로만 모으고 되돌린다 — 시스템마다 자기 몫을 자기가 쓴다.
 *
 * 규칙:
 *  - 서버에서만 불린다.
 *  - GetSaveId 는 그 범위 안에서 겹치지 않는 고정 이름(예: "Magic.Spellbook"). 바꾸면 옛 세이브를 못 읽는다.
 *  - 쓰는 형식을 바꾸면 GetSaveVersion 을 올리고 ReadSaveData 에서 옛 버전을 읽어 준다.
 *  - 월드의 놓인 액터는 GetSaveId 에 액터 이름(레벨 안에서 고정)을 섞어 쓴다.
 */
class GRIMOIRE_API IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	virtual EGrimSaveScope GetSaveScope() const = 0;

	virtual FName GetSaveId() const = 0;

	virtual int32 GetSaveVersion() const { return 1; }

	/** 자기 몫을 Ar 에 쓴다(Ar.IsSaving()). */
	virtual void WriteSaveData(FArchive& Ar) = 0;

	/** 자기 몫을 Ar 에서 읽어 되돌린다(Ar.IsLoading()). Version = 쓸 때의 GetSaveVersion. */
	virtual void ReadSaveData(FArchive& Ar, int32 Version) = 0;
};
