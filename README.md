# President Game / Four Years

A presidency-game prototype combining turn-based decisions with explorable 3D locations. The current work includes an Oval Office browser prototype and a native Unreal Engine Air Force One exploration level.

## Get the files

Install Git LFS before cloning so the Blender and Unreal assets download correctly:

```sh
git lfs install
git clone https://github.com/ainsleya5320/President-Game.git
cd President-Game
git lfs pull
```

Use a Git clone with LFS rather than relying on GitHub's source ZIP for the large 3D assets.

## Play

### Browser prototype

Serve the repository with a local web server, for example `python -m http.server 8781`, then open:

<http://localhost:8781/Unreal%20Projects/OvalOffice/Prototype/Four_Years_Prototype.html>

This prototype includes the explorable Oval Office, decorative improvements, a garden view and twelve monthly decisions. It loads Three.js from a CDN, so an internet connection is required.

### Air Force One in Unreal

Requires Unreal Engine 5.8 on Windows. Run **Play Air Force One.cmd** at the repository root, or **Edit Air Force One.cmd** to open the native level in the editor. The launcher currently expects Unreal at `C:\Program Files\Epic Games\UE_5.8`; adjust `Scripts/LaunchAirForceOne.ps1` if installed elsewhere.

Click in the game window. **W/A/S/D** walks, the **mouse** looks around, and **Escape** exits. The presidential office, connecting hallway and conference room have working first-person controls and collision. All seven tested walking routes, floor support and wall blocking passed in the running engine. Results are saved under [`validation`](validation).

The project file is [`Unreal Projects/OvalOffice/OvalOffice.uproject`](Unreal%20Projects/OvalOffice/OvalOffice.uproject). Air Force One is `/Game/AirForceOne/Maps/AirForceOne`; the original Oval Office scene is `/Game/OvalOffice/Maps/OvalOffice`. The project uses DirectX 11 settings for integrated-GPU compatibility.

Air Force One is currently an exploration level. Campaign events, desk interactions and travel transitions between the native levels are not yet implemented.

## Project files

| Folder | Contents |
| --- | --- |
| `Unreal Projects/OvalOffice` | Unreal project, native maps, materials, source meshes and authoring scripts |
| `Unreal Projects/OvalOffice/Prototype` | Browser game, source templates and artwork |
| `oval-office` | Original Blender scene, portable models, textures, renders and scripts |
| `air-force-one` | Widened Blender scene, portable model, textures, renders and clearance plan |
| `validation` | Saved Unreal build and runtime acceptance results |

The source scenes were authored with Blender 5.2. Some historical authoring scripts use local paths and may need adjustment on another computer; the committed game assets do not require rebuilding to open.

## References and scope

These are artistic reconstructions based on public references, with room dimensions adapted for gameplay. The Blender renders and real-time Unreal views use different lighting systems. Reference and artwork provenance is documented in the individual scene READMEs and `Prototype/assets/ASSET_NOTES.md`; no blanket license is granted to third-party reference material.

Generated engine caches, temporary logs, local backups and duplicate ZIP packages are excluded from version control. Large editable 3D files use Git LFS.
