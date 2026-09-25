# Oval Office — Unreal Engine project

Open `OvalOffice.uproject` with Unreal Engine 5.8. The startup map is the furnished interior.

**Air Force One is also available as a native first-person level.** Open `Play Air Force One.cmd` to walk through the aircraft, or `Edit Air Force One.cmd` to edit it. See `AIR FORCE ONE.md` for controls and details.

For the playable presidency game, run the repository's **Play Four Years.cmd**, or serve `Prototype/Four_Years_Prototype.html` in a modern browser. It contains the walkable Oval Office, Air Force One's office/hallway/conference suite and a full four-year political simulation with policies, budgets, voter groups, quarterly dilemmas and elections. Use **Board Air Force One** and **Return to Oval Office** to switch locations while keeping the same term. `GAME_CONCEPT.md` explains the gameplay systems and what remains to be built natively in Unreal.

The playable prototype now includes a Rose Garden-inspired exterior view, framed fireplace artwork, mantel greenery, and clearer door panels and hardware. These additions are in the browser prototype; the original native Unreal level remains an editable room model. Generated image source files and prompts are documented in `Prototype/assets/ASSET_NOTES.md`.

## Explore

- Hold the right mouse button in the level viewport and use W/A/S/D to move, Q/E to descend/ascend. Use the mouse wheel while moving to adjust speed.
- Open `Content/OvalOffice/Maps/OvalOffice_Cutaway` for the open architectural view.
- Cameras are grouped under **Cameras** in the Outliner. Right-click a camera and choose **Pilot** to view from it.
- Architecture and furniture are grouped in the Outliner under **Oval Office**. Their material instances are in `Content/OvalOffice/Materials`.

## Contents

Two editable levels, 31 static mesh assets, 62 material instances, image textures, daylight and lamp lighting, and interior/desk/cutaway cameras. Geometry was transferred from the original Blender model at centimetre scale. Source meshes, textures and import scripts are included for reproducibility.

The original native Oval Office is a visual design scene with free camera navigation; its character controls and decision gameplay have not been built. The native Air Force One level has a walking character and collision; the browser game supplies the shared decision system and location switching. The models are artistic approximations, not exact architectural surveys or photoreal replicas. Blender procedural surfaces have been translated into simpler Unreal materials; lighting is rebuilt for Unreal.

## Graphics

The project uses DirectX 11, standard shadows and movable lighting for compatibility with this computer's integrated Adreno GPU. Lumen, ray tracing and Nanite are disabled. A project-local derived-data cache is configured. More powerful hardware can support a later lighting and material refinement pass.

## Provenance

Source model: `../../oval-office/Oval_Office.blend`. Original reference and texture attribution notes are in `../../oval-office/README.md`. Those attribution and reuse limitations continue to apply to the imported textures, including reference artwork and the desk image.
