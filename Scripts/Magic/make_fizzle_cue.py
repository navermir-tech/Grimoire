# 마법 실패 연출 큐 에셋 GC_Magic_Fizzle 을 만든다(Q64 — docs/systems/magic 「마법 실패 알림」).
#  - C++ 처리기 UGrimCueNotify_Fizzle 의 블루프린트 자식. 큐 관리자는 GameplayCueNotifyPaths(/Game/Grimoire)의 블루프린트만 찾는다.
#  - 연출 값(이펙트 · 몽타주)은 에셋이 아니라 ini [/Script/Grimoire.GrimCastFailureSettings] 에 있다.
#
# 실행(에디터를 닫고, 저장소 루트에서 — 빌드 뒤):
#   & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\Grimoire.uproject" `
#     -run=pythonscript -script="$PWD\Scripts\Magic\make_fizzle_cue.py" -unattended -nop4 -nosplash -nullrhi

import unreal

CUE_DIR = "/Game/Grimoire/Magic/Cues"
NAME = "GC_Magic_Fizzle"

EAL = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def main():
    path = CUE_DIR + "/" + NAME
    if EAL.does_asset_exist(path):
        bp = EAL.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.GrimCueNotify_Fizzle.static_class())
        bp = asset_tools.create_asset(NAME, CUE_DIR, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)
    cdo = unreal.get_default_object(EAL.load_blueprint_class(path))
    unreal.log("[make_fizzle_cue] %s → 태그 %s" % (path, cdo.get_editor_property("gameplay_cue_tag")))


main()
