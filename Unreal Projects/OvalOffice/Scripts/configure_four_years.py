"""Set the imported Oval map's explicit game-mode override to the native game."""
import unreal as u
import json
from pathlib import Path

project = Path(u.Paths.project_dir())
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_name() != 'OvalOffice':
    raise RuntimeError('Open /Game/OvalOffice/Maps/OvalOffice before configuring Four Years.')
game_mode = u.load_class(None, '/Script/OvalOffice.FourYearsGameMode')
if not game_mode:
    raise RuntimeError('Build the OvalOffice C++ module first.')
settings = world.get_world_settings()
if settings.get_editor_property('default_game_mode') != game_mode:
    settings.set_editor_property('default_game_mode', game_mode)
    if not levels.save_current_level():
        raise RuntimeError('Could not save the Oval Office game-mode override.')
report = {'map': world.get_path_name(), 'game_mode': settings.get_editor_property('default_game_mode').get_path_name(), 'status': 'ready'}
(project / 'Saved' / 'FourYearsSetup.json').write_text(json.dumps(report, indent=2))
u.log('FOUR_YEARS_SETUP ' + json.dumps(report))
