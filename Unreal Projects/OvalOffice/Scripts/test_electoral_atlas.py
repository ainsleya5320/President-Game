"""Run in Unreal Play in Editor; restores the player's save and ends the test session."""
import unreal as u, json, hashlib, traceback
from pathlib import Path
from datetime import datetime

p = Path(u.Paths.project_dir())
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if not world:
    raise RuntimeError('Start Play in Editor before running this test')
gi = u.GameplayStatics.get_game_instance(world)
game = next(o for o in u.ObjectIterator(u.FourYearsSubsystem) if o.get_outer() == gi)
path = Path(game.get_save_path())
original = path.read_bytes()
backup = path.with_name('Term.pre-electorate-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.json')
backup.write_bytes(original)
checks = []
report = {'status': 'running', 'checks': checks, 'backup': str(backup)}

def check(name, condition):
    checks.append({'check': name, 'passed': bool(condition)})
    if not condition:
        raise AssertionError(name)

def view():
    return json.loads(game.get_voter_atlas_json())

def saved():
    return json.loads(path.read_text(encoding='utf-8-sig'))

try:
    v = view()
    check('All counties, states and district electors loaded', len(v['counties']) == 3143 and len(v['states']) == 51 and len(v['districts']) == 5)
    check('Viewing electorate does not alter the save', path.read_bytes() == original)
    for party in ('republican', 'democrat'):
        game.start_new_term()
        check(party + ': new presidency has no party selected', not view()['party'])
        game.complete_inauguration()
        before = saved()
        check(party + ': selection accepted', game.choose_electoral_party(party).ok)
        after = saved()
        check(party + ': selection preserves progress', all(before.get(k) == after.get(k) for k in before if k != 'electoral'))
        check(party + ': selection saved', after['electoral']['party'] == party)
        check(party + ': cannot switch midterm', not game.choose_electoral_party('democrat' if party == 'republican' else 'republican').ok)
        baseline = view()
        check(party + ': all electors accounted for', baseline['points'] + baseline['opponent'] + baseline['tossup'] == 538)
        local = next(c for c in baseline['counties'] if c['id'] == '17031')
        check(party + ': county has eight joint cohorts', len(local['cohorts']) == 8)
        check(party + ': organizing accepted', game.organize_county('17031').ok)
        organized = next(c for c in view()['counties'] if c['id'] == '17031')
        check(party + ': field operation improves support and turnout', organized['support'] > local['support'] and organized['turnout'] > local['turnout'])
        check(party + ': duplicate operation refused', not game.organize_county('06075').ok)
        check(party + ': operation costs two capital', saved()['capital'] == after['capital'] - 2)
    for quarter in range(16):
        check('Quarter %d decision accepted' % quarter, game.choose_response(0))
        check('Quarter %d disallows late organizing' % quarter, not game.organize_county('17031').ok)
        game.advance_quarter()
    final = saved()['executive']['finalElection']
    check('Complete term uses 538 electors and 270 threshold', final['total'] == 538 and final['needed'] == 270 and len(final['regions']) == 56)
    check('Final victory agrees with electors', final['won'] == (final['points'] >= 270))
    check('Completed term cannot organize', not game.organize_county('17031').ok)
    report['status'] = 'passed'
except Exception:
    report['status'] = 'failed'
    report['error'] = traceback.format_exc()
finally:
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    path.write_bytes(original)
    report['save_restored'] = path.read_bytes() == original
    report['save_sha256'] = hashlib.sha256(original).hexdigest()
    (p / 'Saved/ElectoralValidation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log('ELECTORAL VALIDATION: %s, %d checks' % (report['status'], len(checks)))
if report['status'] != 'passed':
    raise RuntimeError(report['error'])
