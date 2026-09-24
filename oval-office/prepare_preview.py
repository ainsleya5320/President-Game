from pathlib import Path
import json,struct,gzip,base64
import numpy as np
P=Path(__file__).parent
raw=(P/'Oval_Office_Preview.glb').read_bytes()
jl=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+jl]);offset=20+jl
bl=struct.unpack_from('<I',raw,offset)[0];binary=bytearray(raw[offset+8:offset+8+bl])
for ac in doc.get('accessors',[]):
 if ac.get('componentType')!=5126 or 'bufferView' not in ac:continue
 bv=doc['bufferViews'][ac['bufferView']];n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[ac['type']];start=bv.get('byteOffset',0)+ac.get('byteOffset',0)
 if bv.get('byteStride',n*4)!=n*4:continue
 a=np.frombuffer(binary,dtype='<f4',count=ac['count']*n,offset=start);np.round(a,3,out=a)
def rounding(v):
 if isinstance(v,float):return round(v,5)
 if isinstance(v,list):return [rounding(x) for x in v]
 if isinstance(v,dict):return {k:rounding(x) for k,x in v.items()}
 return v
js=json.dumps(rounding(doc),separators=(',',':')).encode();js+=b' '*((-len(js))%4)
out=struct.pack('<III',0x46546c67,2,28+len(js)+len(binary))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(binary),0x004e4942)+binary
(P/'Oval_Office_Preview.glb').write_bytes(out);compressed=gzip.compress(out,compresslevel=9);(P/'Oval_Office_Preview.glb.gz').write_bytes(compressed)
print('Compressed bytes',len(compressed),'Base64 bytes',len(base64.b64encode(compressed)))
