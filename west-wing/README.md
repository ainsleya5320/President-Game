# West Wing exploration expansion

An editable Blender scene and playable expansion for Four Years. Enter from **Explore West Wing** in the browser toolbar, or approach the west door of the Oval Office and press **E**. The **Wing map** and room selector provide directions and shortcuts. W/A/S/D or arrow keys walk; drag the scene to look.

The main floor includes connected galleries, the Roosevelt Room, Cabinet Room, presidential study, reception, a 49-seat press briefing room, the colonnade and a landscaped Rose Garden. Closed staff-office doors, an upper-storey exterior and a curved Oval exterior complete the building mass. Staff offices, the upper storey and basement are not enterable. Return to the existing Oval Office through its marked doorway; Air Force One remains available. Travel does not advance the term or spend resources, and existing browser saves are retained.

The Cabinet, Roosevelt, study, reception and press rooms offer **E** shortcuts to the existing strategy screens. This is an exploration expansion; it does not add new political rules or animate staff.

## Files

- `West_Wing.blend`: editable scene, packed textures, cameras, lights, player starts and a preserved copy of the original Oval Office.
- `West_Wing.glb`: portable expansion geometry for the browser. The existing polished Oval loads separately in the game.
- `navigation.json`: metre-based floor, obstacle, door and destination data exported by the scene builder. Player collision radius is 0.28 m; eye height in the game is 1.65 m.
- `renders`: actual Blender Cycles renders of the gallery, Cabinet Room and colonnade/garden.
- `build_west_wing.py`: repeatable Blender authoring script. Run with Blender 5.2: `blender -b --python west-wing/build_west_wing.py -- --render`. Rebuild the browser page afterward with `Unreal Projects/OvalOffice/Prototype/build_prototype.py`.

The browser batches the static meshes by material, downsizes embedded textures and loads the new scene when first entered. The generated HTML contains its model and navigation data. Blender uses Cycles; the browser uses lighter real-time lighting, so their images differ. The native Unreal project is unchanged by this expansion.

## Reference and interpretation

Public-room selection and architectural cues are based on the [archived White House West Wing Tour booklet](https://obamawhitehouse.archives.gov/sites/default/files/docs/west-wing-tour-booklet.pdf). This is an artistic, public-tour-inspired game layout, not a measured or current floor plan. Routes and room dimensions are adapted for comfortable walking and to preserve the existing Oval Office.

Wood, leather and the presidential emblem reuse the project's existing Air Force One assets; framed artwork reuses the Oval Office paintings and original fictional photo assets. See `air-force-one/README.md` and `Unreal Projects/OvalOffice/Prototype/assets/ASSET_NOTES.md` for those assets' provenance. No reference photos of people are newly incorporated.

## Validation

`test_west_wing_navigation.cjs` checks that all eight destinations and the connecting doorways are reachable from the Oval approach, checks the entire press center aisle, and verifies walls, tables, stage and garden obstacles block movement. `test_browser_west_wing.cjs` exercises the production 3D page in an isolated browser, including WASD, walls, all destinations, map, Cabinet interaction, dashboard, save/resume, the Oval doorway and Air Force One travel. Browser test screenshots are saved in the ignored `Saved/BrowserPrototype` folder.
