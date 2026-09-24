import bpy,json
from pathlib import Path
P=Path(__file__).parent
(P/'material_palette.json').write_text(json.dumps({m.name:list(m.diffuse_color) for m in bpy.data.materials},separators=(',',':')))
