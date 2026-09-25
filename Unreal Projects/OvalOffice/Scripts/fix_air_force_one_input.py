"""Repair invalid FKey pin defaults without rebuilding the level's geometry."""
import unreal as u,json
from pathlib import Path
P=Path(u.Paths.project_dir());bp=u.load_asset('/Game/AirForceOne/Blueprints/BP_AF1_Walker')
e=u.BlueprintGraphEditor.get_graph_editor(u.BlueprintEditorLibrary.find_event_graph(bp));pins=u.BlueprintGraphPinLibrary
def key_values(graph):
 return [str(pins.get_pin_value(node.find_input_pin('Key'))) for node in graph.list_all_nodes() if 'isinputkeydown' in str(node.get_node_title()).lower().replace(' ','')]
before=key_values(e)
# Rebuild only the controls from the corrected, reproducible authoring function.
source=(P/'Scripts'/'build_air_force_one.py').read_text().split('\ntry:\n',1)[0]
namespace={'__name__':'af1_controls_repair'};exec(source,namespace);namespace['controls']()
e=u.BlueprintGraphEditor.get_graph_editor(u.BlueprintEditorLibrary.find_event_graph(bp));keys=key_values(e)
changes=[{'before':a,'after':b} for a,b in zip(before,keys)]
assert sorted(keys)==['A','D','S','W'],keys
assert u.BlueprintEditorLibrary.compile_blueprint(bp)
assert not e.list_nodes_with_errors()
assert u.get_editor_subsystem(u.EditorAssetSubsystem).save_loaded_asset(bp,False)
report={'key_pin_defaults':keys,'repaired':changes,'compiled':True,'explanation':'FKey::ImportTextItem reads a bare key name. Struct syntax was parsed as an invalid key.'}
(P/'Saved'/'air_force_one_input_fix.json').write_text(json.dumps(report,indent=2));u.log('AF1_INPUT_FIXED '+str(report))
