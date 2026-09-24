# Air Force One — native Unreal level

Open **Play Air Force One.cmd** to enter the aircraft, or **Edit Air Force One.cmd** to open its level in Unreal Engine 5.8.

Click inside the game window. Use **W/A/S/D** to walk and the **mouse** to look around. **Escape** exits the game. The presidential office, hallway and conference room are connected through open doorways. Walking speed is a comfortable 1.8 metres per second.

The editable map is `/Game/AirForceOne/Maps/AirForceOne`. Its assets are under `Content/AirForceOne`: nine grouped static meshes, twenty cabin materials with authored image textures, rebuilt interior lighting, a first-person Character Blueprint and its GameMode. The character has a 34 cm capsule radius and 90 cm half-height. Static surfaces use triangle collision so open doors and furniture gaps remain usable.

This level was imported from the widened Air Force One Blender model. It is a playable exploration level; presidency decisions, desk interactions and travel transitions to the Oval Office are not implemented here. The original Oval Office map and browser game remain available separately.

## Graphics

The level uses the project's DirectX 11 settings for this computer's integrated GPU. Lighting uses broad movable lights and fill lights. This is a real-time interpretation of the Blender materials and lighting; it does not reproduce the Cycles renders exactly.

## Rebuild and validation

The Blender exporter is `../../air-force-one/export_unreal.py`. Exported source geometry and textures are in `SourceAssets/AirForceOne`. `Scripts/build_air_force_one.py` builds native assets and the map in Unreal. `Scripts/LaunchAirForceOne.ps1 -Review` runs the opt-in runtime checks and writes results and screenshots to `Saved/AirForceOne`. Normal launches do not run the tests.

The runtime test moves the real Character through seven paths using CharacterMovement, checks floor support, and pushes against the hallway wall to check blocking collision. The native Blueprints provide keyboard movement and mouse look independently of Python; Python is used only for authoring and validation.

**Verified:** all seven routes passed in the running Unreal level, including the office lounge and both conference-table aisles. Floor support and wall blocking also passed. The full result is `Saved/AirForceOne/runtime_test.json`.
