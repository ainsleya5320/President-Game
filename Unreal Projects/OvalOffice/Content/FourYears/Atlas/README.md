# Four Years world atlas

An offline geographic atlas for the native Unreal diplomacy screen. Political
and terrain imagery share a Robinson projection with the selection polygons,
country labels, graticules, and city points. The shipped atlas contains 258
Natural Earth country/territory units and 556 named cities; these are not 258
sovereign states. Raster size is 6144 × 3116. Vector outlines become visible
when zoomed in, and selection is based on polygon interiors, including holes.

## Data and license

Natural Earth data is public domain, including commercial reuse and modification:
https://www.naturalearthdata.com/about/terms-of-use/

- Admin 0 countries, 1:10m, version 5.1.1:
  https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/
- Populated places, 1:10m, version 5.1.2:
  https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-populated-places/
- Natural Earth II shaded relief / land cover, 1:50m:
  https://www.naturalearthdata.com/downloads/50m-raster-data/50m-natural-earth-2/
- Lakes and rivers, 1:50m:
  https://www.naturalearthdata.com/downloads/50m-physical-vectors/

The map follows the source's **de facto boundary convention**, not a live
boundary service or a statement of universally recognized territorial claims.
The game diplomacy, influence, and alliances are fictional. National borders
remain separate when negotiations operate at EU or Gulf group level. Countries
without implemented diplomacy display geographic information only.

All point symbols represent named settlements at source coordinates. Gold rings
with centers denote capitals. Country names are area labels with no point symbol.
Trade links run from Washington, D.C. to actual capital-city negotiating hubs;
the curves are symbolic diplomatic links, not navigable shipping routes.

## Controls

M opens the atlas from the walk scene. Click territory to inspect it; double-click
to focus. Drag to pan; scroll or use + / - to zoom. Search accepts country names,
three-letter geographic codes, and city names; press Enter. Whole World resets
the camera. Expand Map hides the inspector. Political, Terrain, and Influence
views share the same camera and selection. Cities, Grid, and Trade Links toggle
independently. M or Escape returns to the office outside the search field.

## Rebuilding and validation

Run `Scripts/fetch_world_atlas.py`, install Pillow, numpy, pyshp, shapely, pyproj,
and mapbox-earcut, then run `Scripts/build_world_atlas.py`. Local dependencies may
be installed in `Saved/AtlasTools`. Source ZIPs and extraction remain under
ignored `Saved/AtlasSources`; download hashes and URLs are recorded in world.json.
No network is needed to play. Runtime dependencies stage PNG and JSON files for
packaging. No Blender/level assets or presidency rules are changed by the atlas.

The builder verifies every triangulated polygon against its source area.
`Scripts/test_world_atlas.py` checks selection locations, enclaves, detached
territories, islands, actual city hubs, independent national borders, projection
alignment, mesh index limits, and image dimensions. Results go to ignored Saved.
The Wau city record's SSD code is normalized to the boundary dataset's SDS code.
