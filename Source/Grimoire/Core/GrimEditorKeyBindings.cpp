// 에디터 단축키 — PIE 종료를 Esc 대신 Shift+Esc(보조 Ctrl+Esc)로. 게임 안에서 Esc 는 메뉴(GrimMenuSubsystem)가 쓴다.
// 처음엔 Config/DefaultEditorSettings.ini 의 ProjectDefinedChords 로 했지만, 값(JSON)이 ini 해석에서 깨져
// 조용히 무시되고 Esc 가 그대로였다(2026-11-05 플레이 테스트). 그래서 엔진 초기화가 끝난 뒤 코드로 직접 건다.
#if WITH_EDITOR

#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Misc/DelayedAutoRegister.h"

static FDelayedAutoRegisterHelper GGrimStopPlayChordRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
{
	const TSharedPtr<FUICommandInfo> StopPlay = FInputBindingManager::Get().FindCommandInContext(TEXT("PlayWorld"), TEXT("StopPlaySession"));
	if (!StopPlay.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PIE 종료 단축키: PlayWorld.StopPlaySession 명령을 찾지 못했다 — Esc 가 그대로다"));
		return;
	}
	StopPlay->SetActiveChord(FInputChord(EKeys::Escape, EModifierKey::Shift), EMultipleKeyBindingIndex::Primary);
	StopPlay->SetActiveChord(FInputChord(EKeys::Escape, EModifierKey::Control), EMultipleKeyBindingIndex::Secondary);
	UE_LOG(LogTemp, Display, TEXT("PIE 종료 단축키: Shift+Esc (보조 Ctrl+Esc)"));
});

#endif
