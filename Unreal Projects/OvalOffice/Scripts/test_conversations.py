"""Run in Play in Editor. Exercises native meetings, policies, portraits and save/reload.

The player's save is backed up and restored byte-for-byte, even on assertion failure.
The temporary play session is stopped afterward so its test state cannot be saved later.
"""
import json
from datetime import datetime
from pathlib import Path
import unreal as u

editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
world = editor.get_game_world()
if not world:
    raise RuntimeError('Start Play in Editor before running this script.')
instance = u.GameplayStatics.get_game_instance(world)
games = [obj for obj in u.ObjectIterator(u.FourYearsSubsystem) if obj.get_outer() == instance]
assert len(games) == 1, 'Could not find the active game subsystem'
game = games[0]
assert game.is_ready(), game.get_data_error()
save = Path(game.get_save_path())
original = save.read_bytes() if save.exists() else None
backup = save.with_name('Term.test-backup-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.json')
if original is not None:
    backup.write_bytes(original)
checks = []
report = {'status': 'running', 'checks': checks, 'backup': str(backup)}

def check(condition, description):
    assert condition, description
    checks.append(description)

try:
    game.start_new_term()
    game.set_location('oval')
    advisers = list(game.get_advisers())
    check(len(advisers) == 4, 'Four advisers load')
    check(len(game.get_policies()) == 26, 'All 26 policies load')
    greetings = set()
    for person in advisers:
        check(game.get_portrait(person.name) is not None, 'Portrait loads: ' + person.name)
        check(bool(person.requested_policy_id), 'Policy link resolves: ' + person.name)
        greetings.add(game.get_dialogue_line(person.id, 'greeting'))
    check(len(greetings) == 4, 'Advisers have four distinct greeting voices')
    check(bool(game.get_dialogue_line('acting-replacement', 'greeting')), 'Acting replacements have fallback dialogue')
    check(bool(game.get_dialogue_line('chen', 'closed')), 'Missing character cues use shared fallback dialogue')

    first = advisers[0]
    result = game.meet_adviser(first.id, 'promise')
    check(result.ok, 'Promise meeting succeeds: ' + result.message)
    check(game.get_status().appointments_left == 1, 'A meeting spends exactly one appointment')
    check(not game.meet_adviser(first.id, 'promise').ok, 'Duplicate meeting is refused')
    state = json.loads(save.read_text(encoding='utf-8'))
    promise = state['promises'][-1]
    check(promise['policy'] == first.requested_policy_id, 'Conversation link matches the promised policy')
    capital_before = game.get_status().capital
    result = game.set_policy_level(promise['policy'], promise['target'])
    check(result.ok, 'Requested policy can be raised to its target')
    check(game.get_status().capital < capital_before, 'Policy changes charge political capital')
    check(game.meet_adviser(advisers[1].id, 'listen').ok, 'Second adviser meeting succeeds')
    check(not game.meet_adviser(advisers[2].id, 'listen').ok, 'Third meeting in a quarter is refused')
    check(len(game.get_congress().available) > 0, 'Congress offers legislation')

    check(game.choose_response(0), 'Quarterly briefing choice succeeds')
    check(game.get_status().decision_filed, 'Filing the decision closes the quarter to new actions')
    check(not game.set_policy_level(promise['policy'], 1).ok, 'Policy change after decision is refused')
    quarter_report = game.advance_quarter()
    check(any('Promise kept' in line for line in quarter_report.changes), 'Next-quarter report includes Promise kept')
    check(any(line.startswith('Kept') for line in game.get_promise_record()), 'Promise record retains the completed commitment')
    check(game.get_status().appointments_left == 2, 'New quarter restores appointments')
    saved_state = json.loads(save.read_text(encoding='utf-8'))
    check(saved_state['quarter'] == 1 and saved_state['promises'][-1]['status'] == 'kept', 'Saved term contains the updated quarter and kept promise')
    # Stop/restart PIE in the UI after this test to verify restoration of the player's term.
    report['status'] = 'passed'
except Exception as error:
    report['status'] = 'failed'
    report['error'] = str(error)
    raise
finally:
    if original is not None:
        save.write_bytes(original)
        report['player_save_restored'] = save.read_bytes() == original
    elif save.exists():
        save.unlink()
        report['player_save_restored'] = not save.exists()
    levels.editor_request_end_play()
    output = Path(u.Paths.project_saved_dir()) / 'ConversationValidation.json'
    output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log('FOUR_YEARS_CONVERSATION_TEST ' + json.dumps(report))
