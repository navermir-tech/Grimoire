# M1.5 통합: 마법 정의(DA_Spell_*)의 VisualSet 에 연출 묶음(VS_*)을 잇는다.
# 마법 값은 Scripts/Magic/make_spells.py, 연출 값은 Scripts/MagicFX/make_magicfx_assets.py 가 정본이고, 이 스크립트는 둘을 잇기만 한다.
# 여러 번 돌려도 같은 결과.
#
#   UnrealEditor-Cmd.exe Grimoire.uproject -run=pythonscript -script="<경로>\Scripts\Integration\link_visual_sets.py" -unattended -nop4 -nosplash -nullrhi
import unreal

LINKS = {
    "/Game/Grimoire/Magic/Spells/DA_Spell_Ember": "/Game/Grimoire/MagicFX/VisualSets/VS_Ember",
    "/Game/Grimoire/Magic/Spells/DA_Spell_WaterJet": "/Game/Grimoire/MagicFX/VisualSets/VS_WaterJet",
    "/Game/Grimoire/Magic/Spells/DA_Spell_Breeze": "/Game/Grimoire/MagicFX/VisualSets/VS_Breeze",
}

for spell_path, visual_path in LINKS.items():
    spell = unreal.EditorAssetLibrary.load_asset(spell_path)
    visual = unreal.EditorAssetLibrary.load_asset(visual_path)
    if spell is None or visual is None:
        raise RuntimeError(f"에셋 없음: {spell_path} 또는 {visual_path}")
    spell.set_editor_property("visual_set", visual)
    if not unreal.EditorAssetLibrary.save_loaded_asset(spell, only_if_is_dirty=False):
        raise RuntimeError(f"저장 실패: {spell_path}")
    unreal.log(f"link_visual_sets: {spell_path} -> {visual_path}")
