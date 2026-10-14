# Cabinet Room garden view

The playable room uses a continuous photographic garden beyond all five windows.
The source is the existing project image `Prototype/assets/rose-garden.png`.
It is imported as `T_PhotographicRoseGarden`, resized by Unreal to power-of-two
dimensions for mip generation, and sampled at mip 4 for soft foliage and roses.
Only the scenery is blurred; people, furnishings and window frames stay sharp.

`Scripts/refine_cabinet_garden.py` applies the update without rebuilding the
interior. `import_cabinet_room.py` also calls it after a complete room import.
The old `CR_Rose_Garden` low-poly actor is removed from the playable map; its
source mesh remains available. The Oval Office environment is retained behind
the garden photo and continues to provide the Cabinet Room's ambient lighting.

The garden is scenery seen through windows, not an explorable outdoor level.
