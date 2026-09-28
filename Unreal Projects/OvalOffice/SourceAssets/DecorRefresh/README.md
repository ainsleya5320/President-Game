# Room decor revision

This pass updates the existing native Unreal Oval Office, Air Force One and Cabinet Room. Their layouts, travel links and presidency simulation remain in place.

- Four ceremonial standards use gathered, still-air cloth with long vertical folds, low fly corners, pole attachments and individual bullion fringe. Reference: [Gerald R. Ford Presidential Museum Oval Office exhibit photograph](https://www.fordlibrarymuseum.gov/sites/default/files/styles/larger/public/externals/4564e7183eaaea1619477523ad07be78.png?itok=pfG292cX).
- Eight credenza frames contain five fictional personal photographs and three small landscape/travel images. The four earlier generated memories are carried over from `Prototype/assets/memories`, with their original provenance retained there.
- Three framed works return to the Oval Office fireplace wall: the existing generated Washington portrait, Albert Bierstadt's *Mountain Brook*, and Thomas Cole's *Distant View of Niagara Falls*.
- Air Force One has fifteen individually assigned paintings, rather than cycling the same three. Cabinet Room uses a separate selection, including a historical Lincoln photograph above the mantel. These are game decoration choices, not a claim to reproduce a particular administration's collection.
- All three desk telephones (Oval Office, aircraft office, aircraft conference room) have rebuilt consoles, inclined displays, connected receivers, buttons with legends and coiled cords. They face their users; the conference telephone sits beside the seated position rather than in the middle of the table.

## Artwork and generated image provenance

Museum images are preserved as downloaded. `art_provenance.json` records titles, artists, source pages, exact image URLs and the Art Institute of Chicago's public-domain designation. Individual artwork cards crop their UVs proportionally; source image pixels are unchanged.

`Textures/family-beach.png` was generated using Codex's **built-in image generation tool**, not the CLI. It depicts fictional people, not the player or a real presidential family. Final prompt:

> Create one photorealistic personal snapshot texture for a small tabletop photograph inside a fictional president's office in a videogame. Landscape 4:3 composition. A warmly smiling fictional middle aged couple and their two adult children sitting together on a beach dune, candid natural interaction, summer afternoon, believable 1990s 35mm film snapshot, gentle grain, ordinary clothing, faces large enough to read in a tiny frame. No famous people, no politicians, no frame, no border, no text. Full bleed photograph only.

## Editable source and installation

`Oval_Decor.blend`, `AirForceOne_Decor.blend`, and `CabinetRoom_Decor.blend` are full editable scenes with packed images. `Meshes/` contains only the changed groups. `manifest.json` records every material assignment and placement.

`Scripts/refresh_decor_blender.py` authors the revisions from the original scene sources. Pass `--original-root` pointing to the checkout containing `oval-office/Oval_Office.blend` and `air-force-one/Air_Force_One.blend`; Cabinet Room uses its existing packed source. The saved decor .blend files can also be edited directly in Blender.

Run `Scripts/import_decor_refresh.py` in the Unreal editor to install the meshes. It swaps only their matching existing actors, adds the fireplace painting group, preserves gameplay and collision settings, and saves the three existing maps. Reimport clears stale OBJ material-slot names so changing a picture cannot silently retain an earlier image.

The Oval Office's legacy coordinate conversion reverses the apparent handedness of the scene. The phone export compensates locally, preserving the position and ensuring its receiver and text face the seated player correctly.

## Verification — September 28, 2026

- 98 native asset checks passed: meshes, material references, textures, distinct room collections, all eight photographs, four flags and three phones.
- 237 existing Cabinet Room walkthrough checks passed, including perimeter clearance, four adviser conversations, and travel to the Oval Office and back. The saved presidency was unchanged byte for byte.
- Native Unreal captures in `Renders/` cover the Oval desk, fireplace, photographs and phone; the aircraft office, corridor and phone; and the Cabinet Room artwork and flags. `Flag_Detail.png` is a separate Blender cloth detail render.
