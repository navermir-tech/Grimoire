#include "MagicFX/GrimMagicFXEditorLibrary.h"
#include "NiagaraSystem.h"

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
