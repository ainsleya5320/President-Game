# Oval Office — Unreal Engine project

Open `OvalOffice.uproject` with Unreal Engine 5.8. The startup map is the furnished interior.

**Air Force One is also available as a native first-person level.** Open `Play Air Force One.cmd` to walk through the aircraft, or `Edit Air Force One.cmd` to edit it. See `AIR FORCE ONE.md` for controls and details.

For the complete native presidency campaign, run **Play Four Years in Unreal.cmd** at the repository root. Walk with **WASD**, look with the mouse and press **J** for the Presidency journal. It connects your 16-quarter campaign, travel, congressional relationships, crises, personal chapters and final legacy. Staff inhabit the rooms and respond through conversation screens. Existing saves continue at their current quarter; the Record page offers a new term with a confirmation and backup.

The native campaign connects the Oval Office, Cabinet Room and Air Force One, plus a residence sitting room and a secure limo cabin. **P** opens policies, **C** Congress, **M** diplomacy and **V** the county electorate. See [native gameplay documentation](Source/README.md) and [campaign guide](Source/PRESIDENCY.md).

The earlier browser game remains available through **Play Four Years.cmd** and `Prototype/Four_Years_Prototype.html`, including its West Wing exploration. It does not contain the new native campaign presentation.

## Explore

- Hold the right mouse button in the level viewport and use W/A/S/D to move, Q/E to descend/ascend. Use the mouse wheel while moving to adjust speed.
- Open `Content/OvalOffice/Maps/OvalOffice_Cutaway` for the open architectural view.
- Cameras are grouped under **Cameras** in the Outliner. Right-click a camera and choose **Pilot** to view from it.
- Architecture and furniture are grouped in the Outliner under **Oval Office**. Their material instances are in `Content/OvalOffice/Materials`.

## Contents

Two editable levels, 31 static mesh assets, 62 material instances, image textures, daylight and lamp lighting, and interior/desk/cutaway cameras. Geometry was transferred from the original Blender model at centimetre scale. Source meshes, textures and import scripts are included for reproducibility.

The imported architectural models are artistic approximations, not exact surveys or photoreal replicas. The new NPCs deliberately use basic stylized geometry. The residence and limo are authored in C++ and generated when the game starts; the existing room assets remain editable in Unreal.

## Graphics

The project uses DirectX 11, standard shadows and movable lighting for compatibility with this computer's integrated Adreno GPU. Lumen, ray tracing and Nanite are disabled. A project-local derived-data cache is configured. More powerful hardware can support a later lighting and material refinement pass.

## Provenance

Source model: `../../oval-office/Oval_Office.blend`. Original reference and texture attribution notes are in `../../oval-office/README.md`. Those attribution and reuse limitations continue to apply to the imported textures, including reference artwork and the desk image.
