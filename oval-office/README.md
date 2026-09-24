# Oval Office — Obama-era Blender reconstruction

An editable, individually modeled Blender scene inspired by the room after the 2010 redesign. Made in Blender 5.2.2, with Cycles lighting and physically based materials.

Open **Oval_Office.blend**. The eight numbered collections organize architecture, seating, the Resolute desk, antiques, flags, other furnishings, landscaping, and cameras. The scene contains three composed cameras. The primary camera is the architectural view; press Numpad 0 to view it and F12 to render.

## Included

- Elliptical architectural shell, layered cornices, wainscoting, parquet floor, three glazed sash windows, and a fireplace.
- Custom modeled Resolute-style desk, with mouldings, work accessories, and a photograph-based front carving texture and bump surface. An earlier, simplified geometric relief is retained as hidden editable objects.
- Upholstered sofas with individual cushions and seams; guest chairs with modeled open cane weaving.
- Geometric silk curtains and flags; porcelain lamps, drawer hardware, a credenza and small frames.
- Procedural oak, mahogany, linen, leather, silk, marble, and mica materials.
- Garden geometry and a Poly Haven outdoor environment for daylight and window reflections.
- Rebuild scripts and texture assets.

## Fidelity

This is an artistic reconstruction from photographs, not a measured replica or a scanned asset. Room dimensions and furniture placement are approximate. The fine desk carving is represented by a photograph-based surface over the modeled carcass, rather than a complete sculpt of the original relief. Minor furnishings, small photographs, the bronze sculpture, outdoor landscaping, flag draping, the carpet seal treatment and the arrangement of border quotations are approximations. The fireplace and side doors are simplified. No claim of historical or photographic identity is made.

The delivered images are actual renders of the Blender scene. No generative-image replacement or post-render AI enhancement is used.

## References and image assets

- White House archived description of the 2010 refurbishment: https://obamawhitehouse.archives.gov/interactive-tour/oval-office
- Main room reference photograph: https://dcmp.org/images/learning_center/420/420-2.jpg (reference only; not mapped onto the room)
- Childe Hassam, *The Avenue in the Rain* (1917), public-domain reproduction: https://commons.wikimedia.org/wiki/File:The_Avenue_in_the_Rain_Frederick_Childe_Hassam_1917.jpeg
- Norman Rockwell, *Working on the Statue of Liberty* (1946), small image used as a historical painting reference within the reconstruction: https://en.wikipedia.org/wiki/Working_on_the_Statue_of_Liberty. This painting is not represented as a CC0 asset; underlying rights remain with its rights holder.
- Presidential seal, public-domain government artwork: https://commons.wikimedia.org/wiki/File:Seal_of_the_President_of_the_United_States.svg
- Poly Haven, *Spruit Sunrise* HDRI, CC0: https://polyhaven.com/a/spruit_sunrise
- Desk surface reference: the room photograph accompanying HISTORY, *A History of the Presidential Farewell Address*: https://www.history.com/articles/a-history-of-the-presidential-farewell-address. The photographic reference is not represented as a CC0 asset; underlying image rights remain with its rights holder.

Textures are packed into **Oval_Office.blend**. The two principal renders are in the **renders** folder. **scene_report.json** records the scene inventory and confirms that the source images were present when the project was packed.

The project is an independent visualization, with no government affiliation or endorsement.

## Interactive 3D exports

**Oval_Office.glb** is a portable export of the room and furniture. **Oval_Office_Cutaway.glb** exposes the interior by omitting the ceiling, upper cornice and front wall sections. The original Blender scene remains unchanged. Procedural Blender materials are reduced to their base colors in these exports; texture images are embedded. The interactive conversation preview uses reduced geometry and image resolution for responsiveness, a simplified elliptical floor, and brighter real-time lighting. The original Blender project contains the fuller materials and geometry used for the rendered images.
