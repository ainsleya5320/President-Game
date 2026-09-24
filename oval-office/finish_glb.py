from pathlib import Path
import struct,json
P=Path(__file__).parent
palette=json.loads((P/'material_palette.json').read_text())
def read(path):
 raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];return json.loads(raw[20:20+n]),raw[20+n:]
def write(path,doc,tail):
 js=json.dumps(doc,separators=(',',':')).encode();js+=b' '*((-len(js))%4);path.write_bytes(struct.pack('<III',0x46546c67,2,20+len(js)+len(tail))+struct.pack('<II',len(js),0x4e4f534a)+js+tail)
doc,tail=read(P/'Oval_Office.glb')
for m in doc.get('materials',[]):
 pbr=m.setdefault('pbrMetallicRoughness',{});col=palette.get(m.get('name'))
 if col and 'baseColorTexture' not in pbr:pbr['baseColorFactor']=col
write(P/'Oval_Office.glb',doc,tail)
preview,_=read(P/'Oval_Office_Preview.glb')
hide=set()
for n in preview['nodes']:
 name=n.get('name','')
 for prefix in ['FRONTWALL__','ROOF__','UPPER__']:
  if name.startswith(prefix):hide.add(name[len(prefix):])
for s in doc['scenes']:s['nodes']=[i for i in s.get('nodes',[]) if doc['nodes'][i].get('name') not in hide]
write(P/'Oval_Office_Cutaway.glb',doc,tail)
print('Validated full and cutaway GLB files:',len(doc.get('meshes',[])),'meshes;',len(hide),'architectural elements omitted from cutaway')
