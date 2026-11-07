#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GrimMagicFXEditorLibrary.generated.h"

class UNiagaraSystem;
class UNiagaraEffectType;

/**
 * 이펙트 자동 제작 스크립트(Scripts/MagicFX/make_fx_niagara.py)가 파이썬에서 부르는 에디터 전용 도우미.
 * 엔진의 Niagara 컴파일 대기 함수가 파이썬에 드러나 있지 않아서 감싼다 — 컴파일 중에 저장하면 커맨드렛이 죽는다.
 */
UCLASS()
class GRIMOIRE_API UGrimMagicFXEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 시스템을 다시 컴파일하고 끝날 때까지 기다린다. 에디터가 아니면 아무것도 하지 않고 false. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|MagicFX")
	static bool CompileNiagaraSystemAndWait(UNiagaraSystem* System, bool bForce = true);

	/** Effect Type 의 중요도 처리기를 「거리」로(가까운 이펙트부터 남긴다 — 성능 규칙 24). 처리기 클래스가 파이썬에 드러나 있지 않아서 감싼다. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|MagicFX")
	static bool SetDistanceSignificance(UNiagaraEffectType* EffectType);
};
