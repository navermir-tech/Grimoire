#include "MagicFX/GrimMagicFXEditorLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraEffectType.h"

bool UGrimMagicFXEditorLibrary::CompileNiagaraSystemAndWait(UNiagaraSystem* System, bool bForce)
{
#if WITH_EDITORONLY_DATA
	if (!System)
	{
		return false;
	}
	System->RequestCompile(bForce);
	System->WaitForCompilationComplete(false, false);
	return true;
#else
	return false;
#endif
}

bool UGrimMagicFXEditorLibrary::SetDistanceSignificance(UNiagaraEffectType* EffectType)
{
	if (!EffectType)
	{
		return false;
	}
	if (!Cast<UNiagaraSignificanceHandlerDistance>(EffectType->SignificanceHandler))
	{
		EffectType->Modify();
		EffectType->SignificanceHandler = NewObject<UNiagaraSignificanceHandlerDistance>(EffectType);
	}
	return true;
}
