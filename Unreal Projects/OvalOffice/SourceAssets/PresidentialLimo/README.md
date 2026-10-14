# Presidential limousine cabin

An explorable luxury cabin inspired by the reference photograph supplied in the
chat. This is an artistic game interior, not a measured reconstruction of a
particular presidential vehicle.

The Blender model includes a curved suede headliner, cream leather rear bench
and side seating, stitched cushions, seat belts, walnut cabinet and fold-down
tray, hollow crystal tumblers, water bottles, circular vents, reading lights,
cove lighting, door releases, a privacy partition, inset displays, a presidential
seal and a secure telephone. The cabin remains stationary; this update does not
add vehicle driving or live video to the displays.

`Presidential_Limo.blend` is editable and uses the adjacent `Textures` folder.
`Meshes/Interior.obj` uses centimetres with X along the cabin and Z up. The
exporter counter-reflects Y and triangle winding for Unreal's legacy OBJ import,
so the in-game furniture matches the native walking and interaction coordinates.
`Renders/Limo_Interior.png` is a Blender render, not a screenshot of Unreal.
The runtime asset is `/Game/PresidentialLimo/Meshes/SM_Interior`.

Rebuild with Blender in background mode and `Scripts/build_presidential_limo.py`.
Append `-- --export-only` to skip the beauty render. Run
`Scripts/import_presidential_limo.py` inside Unreal Editor to import the source.
The importer shares the staff import pipeline and supports the limo's metal,
emissive, translucent and softened photographic materials.

The native presence system instantiates the cabin at the existing limo location.
Maya sits on the rear bench, the centre aisle remains clear, the drinks tray and
rear bench are protected by walking boundaries, and E still reaches destinations,
the secure phone and Maya's conversation. The phone's interaction point is near
the side console; the reduced adviser radius prevents Maya from intercepting it.

Texture sources are existing project assets from Air Force One and the garden.
The user's reference photograph has not been copied into the repository.

Validation: the native Unreal build succeeded. All 19 limo checks passed,
including authored mesh/material loading, seven clear aisle positions, furniture
boundaries, secure-phone access, Maya's conversation, return travel and an
unchanged campaign save. The exported cabin has 69,014 triangles and 14 finishes.
