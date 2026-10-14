# Presidential staff

Eight fictional human characters replace the original sphere-and-box placeholders:
Maya Chen, Daniel Brooks, Agent Morgan, Elena Ruiz, Samira Bell, Evelyn Mercer,
Luis Ortega and Jordan Ellis. Each has a distinct head shape, stature, skin texture,
hair and clothing palette. The same identity is used in every room.

The Blender sources contain the fitted original meshes, a posing rig, and baked
standing/seated game parts. The head is a separate textured mesh with its pivot
under the collar; Unreal supplies restrained head tracking and idle movement.
These are lightweight posed characters, not fully animated skeletal NPCs: they
do not walk, lip-sync, or play facial expressions yet. Existing dialogue and
interaction logic is retained.

## Rebuild

1. Run Blender in background mode with `Scripts/build_presidential_staff.py`.
2. Run `Scripts/import_presidential_staff.py` through Unreal's Python editor.
3. Rebuild the native module when the generated `PresidentialStaffCatalog.inl`
   changes. The catalog records the exact standing and seated head pivots.

`Meshes/` contains centimetre-scale OBJ exports (+X forward). `Textures/` is shared
by the editable `.blend` files through relative paths. `Renders/` contains actual
Blender renders for visual review. Runtime assets are in `/Game/PresidentialStaff`.
The cast uses masked hair, UV-mapped face/hand details, normal-mapped hair where
available, matte fabric and shinier eyes/shoes. Chair poses preserve a 54 cm hip
height and plant the shoe soles at floor level.

## Asset credits

Human topology, shape targets, rig landmarks and weights: MakeHuman Community / MPFB.
Skin, hair, eyes, brows, clothing and shoes: MakeHuman Community system assets.
These sources are **CC0 1.0**. Original file headers and the complete license are
retained in `Vendor/`; source URLs and the MPFB revision are in `provenance.json`.

- [MakeHuman system asset license catalog](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html)
- [MPFB asset license](https://github.com/makehumancommunity/mpfb2/blob/master/LICENSE.ASSETS.md)

Game-specific adaptations include eight character profiles, tailored garment
fitting, restrained poses, material variations, floor contact,
head seams, game exports and runtime integration. No MakeHuman add-on is installed
or required to play; the build script reads asset data directly.


Validation: all 24 exported mesh parts passed bounds, floor-contact and texture checks. The Unreal build succeeded, and 99 native checks passed for asset loading, NPC conversations and travel through the Oval Office, Cabinet Room and aircraft. The campaign save was unchanged.
