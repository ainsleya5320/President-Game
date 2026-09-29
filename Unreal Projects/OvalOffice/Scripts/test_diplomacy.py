"""Native gameplay smoke test, run in Play in Editor.
Backs up and restores the player's save byte-for-byte; always stops the temporary play session.
"""
import unreal as u, json, hashlib, traceback
from pathlib import Path
from datetime import datetime

p = Path(u.Paths.project_dir())
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if not world: raise RuntimeError('Press Play first')
gi = u.GameplayStatics.get_game_instance(world)
game = next(o for o in u.ObjectIterator(u.FourYearsSubsystem) if o.get_outer() == gi)
path = Path(game.get_save_path())
original = path.read_bytes()
backup = path.with_name('Term.pre-diplomacy-test-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.json')
backup.write_bytes(original)
checks = []
def check(name, condition):
    checks.append({'check': name, 'passed': bool(condition)})
    if not condition: raise AssertionError(name)
def view(): return json.loads(game.get_diplomacy_json())
report = {'status': 'running', 'checks': checks, 'backup': str(backup)}
try:
    check('Existing save acquires twelve diplomatic partners', len(view()['regions']) == 12)
    check('Loading world state does not write the existing save', path.read_bytes() == original)
    game.start_new_term()
    check('New presidency requests inauguration', game.needs_inauguration())
    game.complete_inauguration()
    check('Inauguration completion persists', not game.needs_inauguration() and json.loads(path.read_text())['inaugurationSeen'])
    check('Doctrine can be chosen before taking action', game.choose_diplomacy_doctrine('peace').ok)
    check('Trade agreement accepted', game.diplomacy_action('canada', 'trade').ok)
    check('Trade agreement saved', json.loads(path.read_text())['diplomacy']['regions']['canada']['trade'])
    check('Duplicate bilateral action refused', not game.diplomacy_action('canada', 'envoy').ok)
    check('First mediation round accepted', game.respond_diplomacy_crisis(0, 'mediate').ok)
    check('Strategy is now committed', not game.choose_diplomacy_doctrine('commerce').ok)
    check('Third diplomatic action accepted', game.diplomacy_action('gulf', 'envoy').ok)
    check('Three actions exhausted', view()['actions'] == 0)
    check('Further action refused', not game.diplomacy_action('japan', 'envoy').ok)
    check('Domestic quarterly decision still works', game.choose_response(0))
    check('World locks after filing', not game.diplomacy_action('japan', 'envoy').ok)
    result = game.advance_quarter()
    check('Domestic quarter advances', result.quarter == 1)
    check('Diplomatic actions replenish', view()['actions'] == 3)
    check('Diplomacy results appear in the quarter report', any('Trade network' in s for s in result.changes))
    check('Rival action appears in the quarter report', any('Rival diplomatic' in s for s in result.changes))
    check('Second mediation round succeeds', game.respond_diplomacy_crisis(0, 'mediate').ok)
    check('Crisis resolution advances the victory goal', view()['resolved'] == 1)
    check('Resolved crisis is closed', not game.respond_diplomacy_crisis(0, 'fund').ok)
    check('Strategic state survives serialization', json.loads(path.read_text())['diplomacy']['resolved'] == 1)
    report['status'] = 'passed'
except Exception:
    report['status'] = 'failed'
    report['error'] = traceback.format_exc()
finally:
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    path.write_bytes(original)
    report['save_restored'] = path.read_bytes() == original
    report['save_sha256'] = hashlib.sha256(original).hexdigest()
    (p/'Saved/DiplomacyValidation.json').write_text(json.dumps(report, indent=2))
    u.log('DIPLOMACY VALIDATION: ' + report['status'] + ', %d checks' % len(checks))
if report['status'] != 'passed': raise RuntimeError(report['error'])
