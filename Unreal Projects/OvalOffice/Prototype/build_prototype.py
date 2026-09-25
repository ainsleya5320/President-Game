from pathlib import Path
from io import BytesIO
import base64, gzip, json, struct
from PIL import Image, ImageFilter

here = Path(__file__).resolve().parent
source = here.parents[2] / 'oval-office'
template = (here / 'office-game.template.html').read_text(encoding='utf-8')
# Keep the original smooth furniture, carved trim, sash bars, and woven chair backs.
# Transfer cutaway labels from the lightweight export without simplifying geometry.
raw = (source / 'Oval_Office.glb').read_bytes()
length = struct.unpack_from('<I', raw, 12)[0]
doc = json.loads(raw[20:20+length])
preview = (source / 'Oval_Office_Preview.glb').read_bytes()
preview_doc = json.loads(preview[20:20+struct.unpack_from('<I', preview, 12)[0]])
labels = {}
for node in preview_doc['nodes']:
    name = node.get('name', '')
    for prefix in ('ROOF__', 'UPPER__', 'FRONTWALL__', 'BACKWALL__'):
        if name.startswith(prefix):
            labels[name[len(prefix):]] = name
for node in doc['nodes']:
    node['name'] = labels.get(node.get('name'), node.get('name', ''))
chunk = json.dumps(doc, separators=(',', ':')).encode()
chunk += b' ' * (-len(chunk) % 4)
tail = raw[20+length:]
packed = struct.pack('<III', 0x46546c67, 2, 20+len(chunk)+len(tail))
packed += struct.pack('<II', len(chunk), 0x4e4f534a) + chunk + tail
model = base64.b64encode(gzip.compress(packed, compresslevel=7)).decode('ascii')

def browser_aircraft():
    """Retain the authored meshes while resizing embedded textures for browser use."""
    raw = (here.parents[2] / 'air-force-one' / 'Air_Force_One.glb').read_bytes()
    length = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20+length])
    binary = raw[28+length:]
    images = {im['bufferView']: im for im in doc.get('images', [])}
    rebuilt = bytearray()
    for index, view in enumerate(doc['bufferViews']):
        data = binary[view.get('byteOffset', 0):view.get('byteOffset', 0)+view['byteLength']]
        if index in images:
            image = Image.open(BytesIO(data))
            image.thumbnail((1024, 1024), Image.Resampling.LANCZOS)
            transparent = image.mode == 'RGBA' and image.getextrema()[3][0] < 255
            output = BytesIO()
            if transparent:
                image.save(output, format='PNG', optimize=True)
                images[index]['mimeType'] = 'image/png'
            else:
                image.convert('RGB').save(output, format='JPEG', quality=88, optimize=True)
                images[index]['mimeType'] = 'image/jpeg'
            data = output.getvalue()
        rebuilt.extend(b'\0' * (-len(rebuilt) % 4))
        view.update(byteOffset=len(rebuilt), byteLength=len(data))
        rebuilt.extend(data)
    doc['buffers'][0]['byteLength'] = len(rebuilt)
    rebuilt.extend(b'\0' * (-len(rebuilt) % 4))
    header = json.dumps(doc, separators=(',', ':')).encode()
    header += b' ' * (-len(header) % 4)
    glb = struct.pack('<III', 0x46546c67, 2, 28+len(header)+len(rebuilt))
    glb += struct.pack('<II', len(header), 0x4e4f534a)+header
    glb += struct.pack('<II', len(rebuilt), 0x004e4942)+rebuilt
    print('Browser aircraft:', round(len(glb)/1e6, 2), 'MB before gzip')
    return base64.b64encode(gzip.compress(glb, compresslevel=7)).decode('ascii')

aircraft = browser_aircraft()
footprints = json.loads((here.parents[2] / 'air-force-one' / 'collision_footprints.json').read_text())
collision = json.dumps([p['polygon'] for p in footprints], separators=(',', ':'))
palette = (source / 'material_palette.json').read_text(encoding='utf-8')
output = here / 'Four_Years_Prototype.html'
def jpg_data(name, size, blur=0):
    im = Image.open(here / 'assets' / name).convert('RGB')
    im.thumbnail(size, Image.Resampling.LANCZOS)
    if blur:
        im = im.filter(ImageFilter.GaussianBlur(blur))
    out = BytesIO()
    im.save(out, format='JPEG', quality=84, optimize=True)
    return base64.b64encode(out.getvalue()).decode('ascii')

garden = jpg_data('rose-garden.png', (1400, 850), blur=1.5)
portrait = jpg_data('washington-portrait.png', (550, 760))
landscape = jpg_data('river-landscape.png', (650, 500))
liberty = jpg_data('statue-liberty.png', (500, 650))
markup = template.replace('__MODEL_BASE64__', model).replace('__PALETTE__', palette)
markup = markup.replace('__GARDEN_BASE64__', garden).replace('__PORTRAIT_BASE64__', portrait)
markup = markup.replace('__LANDSCAPE_BASE64__', landscape).replace('__LIBERTY_BASE64__', liberty)
markup = markup.replace('__ROOM_POLISH__', (here / 'room-polish.js').read_text(encoding='utf-8'))
markup = markup.replace('__AIRCRAFT_CODE__', (here / 'aircraft-level.js').read_text(encoding='utf-8'))
markup = markup.replace('__AIRCRAFT_BASE64__', aircraft).replace('__AIRCRAFT_COLLISION__', collision)
output.write_text(markup, encoding='utf-8')
print(output, output.stat().st_size)
