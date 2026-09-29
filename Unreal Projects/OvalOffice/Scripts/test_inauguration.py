# Run in Unreal Editor while playing an existing presidency. Does not reset or save the term.
import unreal as u, json, time, hashlib, traceback
from pathlib import Path

intro_project = Path(u.Paths.project_dir())
intro_save = intro_project/'Saved/FourYears/Term.json'
intro_original = intro_save.read_bytes()
intro_world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if not intro_world:
    raise RuntimeError('Press Play before running inauguration validation')
intro_pc = u.GameplayStatics.get_player_controller(intro_world, 0)
intro_gi = u.GameplayStatics.get_game_instance(intro_world)
intro_game = next(o for o in u.ObjectIterator(u.FourYearsSubsystem) if o.get_outer() == intro_gi)
intro_checks = []

def intro_check(name, passed):
    intro_checks.append({'check': name, 'passed': bool(passed)})
    if not passed: raise AssertionError(name)

intro_check('Existing presidency does not need the prologue', not intro_game.needs_inauguration())
intro_check('No automatic intro over an existing save', intro_pc.get_inauguration_stage() == -1)
for i in range(3):
    intro_check('Inauguration image %d loaded' % i, intro_game.get_inauguration_image(i) is not None)
intro_check('Invalid image indices safely rejected', intro_game.get_inauguration_image(-1) is None and intro_game.get_inauguration_image(3) is None)
intro_pc.open_inauguration()
intro_check('Replay starts with the montage', intro_pc.get_inauguration_stage() == 0)
intro_check('Intro holds the screen lock', intro_pc.is_screen_open())
intro_pc.open_policies()
intro_pc.close_screen()
intro_check('Policy and screen commands cannot dismiss intro', intro_pc.get_inauguration_stage() == 0)
intro_phase = 0
intro_started = time.monotonic()
intro_briefing_at = None

def intro_validation_tick(dt):
    global intro_phase, intro_briefing_at
    try:
        elapsed = time.monotonic() - intro_started
        if elapsed > 100: raise RuntimeError('Montage validation timed out')
        stage = intro_pc.get_inauguration_stage()
        if intro_phase == 0 and stage == 1:
            intro_check('Montage advances naturally to the welcome briefing', elapsed >= 17)
            intro_briefing_at = time.monotonic()
            intro_phase = 1
        elif intro_phase == 1 and time.monotonic() - intro_briefing_at > 3:
            intro_check('Briefing waits for player input', stage == 1)
            intro_pc.finish_inauguration()
            intro_check('Continue restores gameplay screen state', not intro_pc.is_screen_open() and intro_pc.get_inauguration_stage() == -1)
            intro_pc.open_policies()
            intro_check('Policies work after the intro', intro_pc.is_screen_open())
            intro_pc.close_screen()
            intro_pc.open_inauguration()
            intro_pc.skip_inauguration()
            intro_check('Skip jumps straight to welcome briefing', intro_pc.get_inauguration_stage() == 1)
            intro_pc.finish_inauguration()
            intro_check('Repeat completion is harmless', intro_pc.get_inauguration_stage() == -1)
            intro_pc.finish_inauguration()
            intro_check('Saved presidency is byte-for-byte unchanged', intro_save.read_bytes() == intro_original)
            (intro_project/'Saved/InaugurationValidation.json').write_text(json.dumps({'status': 'passed', 'checks': intro_checks, 'save_sha256': hashlib.sha256(intro_original).hexdigest()}, indent=2))
            u.unregister_slate_post_tick_callback(intro_validation_handle)
            u.log('INAUGURATION VALIDATION PASSED: %d checks' % len(intro_checks))
    except Exception:
        (intro_project/'Saved/InaugurationValidation.json').write_text(json.dumps({'status': 'failed', 'checks': intro_checks, 'error': traceback.format_exc()}, indent=2))
        u.unregister_slate_post_tick_callback(intro_validation_handle)
        raise

intro_validation_handle = u.register_slate_post_tick_callback(intro_validation_tick)
