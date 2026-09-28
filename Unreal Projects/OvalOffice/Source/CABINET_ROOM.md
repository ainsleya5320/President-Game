# Cabinet Room

A Blender-authored, walkable Cabinet Room for the native Four Years Unreal game.
This is a game adaptation, with a 13 × 7.6 metre room and an entrance vestibule;
it is not a surveyed replica of the White House floor plan.

## Play

From the repository root, double-click **Play Cabinet Room in Unreal.cmd**.
Alternatively, play the Oval Office and approach the door on the right-hand side
when facing the Resolute Desk. The CABINET ROOM sign identifies it; press **E**.
The door at the far end of the Cabinet Room's entrance hall returns to the Oval Office.

- **W/A/S/D** walks; the mouse looks around.
- **E** beside Maya Chen, Elena Ruiz, Daniel Brooks or Samira Bell's named seat opens that adviser's portrait conversation.
- **E** at the taller presidential chair opens the quarterly briefing.
- **P** opens policies and **C** opens Congress in either room.
- Travel consumes no time, capital, or appointments. The same game instance keeps the current presidency alive across both maps.

The seats provide interactions with the existing portrait characters; seated,
animated 3D character bodies are not included in this room build.

## Contents

- `SourceAssets/CabinetRoom/Cabinet_Room.blend`: editable Blender scene.
- `SourceAssets/CabinetRoom/Meshes`: evaluated bevelled meshes and their material library.
- `SourceAssets/CabinetRoom/Textures`: project textures used by this room.
- `/Game/CabinetRoom/Maps/CabinetRoom`: native Unreal level.
- `Scripts/author_cabinet_room.py`: repeatable Blender authoring/export script.
- `Scripts/import_cabinet_room.py`: Unreal import, lighting and two-way connection.
- `Scripts/test_cabinet_room.py`: native perimeter, furniture, adviser and return-trip validation.

The model has sixteen leather chairs, an oval mahogany table with brass inlay,
briefing papers, pens, microphones, water tumblers, a mantel portrait, ceremonial
flags, paneled walls, layered cornice, pendant lights and modelled rose beds
outside five tall windows. The president's taller chair is centered on the garden side.

## Rebuild and verify

Build the C++ module with `Scripts/BuildFourYearsUnreal.ps1` while Unreal is closed.
Run Blender in background with `--python Scripts/author_cabinet_room.py` to regenerate
the source. Then run `py <absolute path>/Scripts/import_cabinet_room.py` in Unreal's
command field. Reimport only replaces actors with the `CR_` or `FY_CabinetLink` prefixes.
The OBJ material library must remain beside the meshes. The importer disables
Interchange OBJ for this run because the legacy FBX factory matches the authored
material slots and works synchronously in the Python import workflow.

Press Play in CabinetRoom and run `Scripts/test_cabinet_room.py` from the editor's
Python command field. Results are written to `Saved/CabinetValidation.json`.
The test opens conversations but never selects a response, spends resources,
advances a quarter, or overwrites the user's saved term.

Room travel, collision areas and seat interactions live on `AFourYearsRoom` actors.
Future rooms can author their own bounds, obstacles, arrival position and
interactions without replacing the presidency simulation or the original Oval Office.
Keep new travel destinations in the packaging map list.

## Reference

Architectural and furnishing cues follow the official archived tours:

- https://georgewbush-whitehouse.archives.gov/government/cabinet-room.html
- https://obamawhitehouse.archives.gov/node/11950/

These describe the oval mahogany table, the taller presidential chair on the east
side, brass chair plates, views of the Rose Garden and presidential portrait selections.
Art and flag images are reused from the existing project's source asset collection;
this change does not introduce new external image downloads.

## Verified build

Compiled successfully in Unreal Engine 5.8.3 on 28 September 2026. Native playtest
passed 237 checks covering the perimeter route, solid furniture and walls, all four
adviser conversation screens, return travel, arrival positions and saved-term preservation.
