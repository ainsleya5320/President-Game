# Air Force One — compact presidential suite

An editable Blender environment containing a presidential office, connecting passage, and conference room. Built for the Four Years game's future aircraft level.

## Open the scene

- **Air_Force_One.blend** — full scene, packed image textures, Cycles lighting, four cameras, separate room collections, and named player/interaction markers.
- **Air_Force_One.glb** — portable model with embedded material images for later import into Unreal or the browser game.
- **renders/** — office, conference, hallway, and cutaway views rendered from the actual Blender geometry.
- **level_manifest.json** — dimensions, units, room names, and marker positions.

The Blender scene is the visual master. A native Unreal Engine 5.8 exploration level now lives in `../Unreal Projects/OvalOffice/Content/AirForceOne`. Open `../Play Air Force One.cmd` to walk through it, or `Edit Air Force One.cmd` to edit the level. Native first-person movement, collision, materials and lighting are implemented; all seven runtime routes, floor support and wall blocking passed. Campaign transitions and the browser game's integration remain separate.

## Design

Approximately 7.2 × 13.1 metres after the walking-clearance revision. Both door openings provide approximately 1.26 metres of clear width, and the hall provides approximately 1.27 metres between handrails. Furniture dimensions are preserved; architecture and cabinet placement were expanded to create circulation space. The office includes an angled walnut desk, two high-backed leather seats, a tufted sofa, built-in cabinets, stationery, telephones, lamps, and recessed aircraft windows. The conference room has eight seats, a walnut table, writing pads, a briefing display, and side cabinetry. A blue-grey corridor runner distinguishes the connecting hallway from the star-patterned room carpets.

The layout is an artistic reconstruction adapted for a compact game level, not a surveyed replica. The office's warm material palette and furniture vocabulary follow the user's supplied photograph and the office shown in the ABC News tour. The conference room and hallway arrangement are interpretive.

## References

- User-supplied presidential aircraft office photograph, retained locally in `references/office-reference.png`.
- [ABC News: Air Force One — Inside the Oval Office in the Sky](https://www.youtube.com/watch?v=NvyCqgrzOYc), supplied by the user and inspected in the browser.
- [White House archive: Air Force One](https://trumpwhitehouse.archives.gov/about-the-white-house/air-force-one/) — public context describing the presidential office and conference facilities.

## Rebuild

Run `make_materials.py` with Python/Pillow/NumPy, then run `build_air_force_one.py` in Blender's background mode. The latter saves the packed Blender file. Next run `ensure_walkable.py` with the saved Blender file loaded to apply the circulation revision and run the geometry-based clearance test. With that file loaded, `render_and_export.py` accepts `office`, `conference`, `hallway`, `cutaway`, or `export` as its final command-line argument.

All renders are produced by Blender Cycles. Material images are locally authored procedural textures; the small presidential emblem reuses the existing Oval Office project's seal asset. No generated photograph is being presented as a 3D render.


## Walking-clearance verification

A 0.68 metre wide, 1.80 metre tall player, with an additional 2 cm radial margin, can reach all seven tested destinations from the hallway. These include the office lounge, desk-side aisle, both sides of the conference table and its far end. All three player starts are clear and connected. Tests use evaluated Blender geometry, conservatively projected into convex solid footprints on a 5 cm grid. This is a geometric clearance check; it does not replace game-engine collision or navigation testing. See Walking_Clearance.svg and walkability_report.json.
