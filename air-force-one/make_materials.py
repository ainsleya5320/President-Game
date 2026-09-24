from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
P=Path(__file__).parent/'textures';P.mkdir(exist_ok=True)
rng=np.random.default_rng(947)
N=1024
def noise(n):
 a=rng.random((n,n))*255
 return np.array(Image.fromarray(a.astype('uint8')).resize((N,N),Image.Resampling.BICUBIC))/255
def save(name,rgb,height):
 Image.fromarray(np.uint8(np.clip(rgb,0,255))).save(P/(name+'.jpg'),quality=95)
 gy,gx=np.gradient(height);v=np.dstack((-gx*3,-gy*3,np.ones_like(gx)));v/=np.linalg.norm(v,axis=2,keepdims=True)
 Image.fromarray(np.uint8((v*.5+.5)*255)).save(P/(name+'-normal.png'))
cloud=noise(12)*.45+noise(48)*.25+noise(140)*.15+noise(512)*.15
leather=np.array([117,87,59])[None,None,:]*(.68+cloud[:,:,None]*.72)
save('cognac-leather',leather,noise(512)*.25+noise(200)*.35)
y,x=np.mgrid[0:N,0:N]/N
grain=np.array(Image.fromarray(np.uint8(rng.random((24,512))*255)).resize((N,N),Image.Resampling.BICUBIC))/255
gr=(grain-.5)*.35+(noise(9)-.5)*.12+(noise(512)-.5)*.025
wood=(np.array([90,62,40])[None,None,:]*(1+gr[:,:,None]))
save('walnut',wood,gr*.11)
fiber=noise(512)*.65+noise(96)*.35
save('ivory-panels',np.array([206,199,180])[None,None,:]*(.93+fiber[:,:,None]*.12),fiber*.08)
base=np.array([166,150,119])[None,None,:]*(.9+fiber[:,:,None]*.2)
im=Image.fromarray(np.uint8(base));d=ImageDraw.Draw(im)
for cx,cy in [(256,256),(768,768)]:
 pts=[(cx+np.sin(i*np.pi/5)*(62 if i%2==0 else 26),cy-np.cos(i*np.pi/5)*(62 if i%2==0 else 26)) for i in range(10)]
 d.polygon(pts,fill=(213,207,181));d.line(pts+[pts[0]],fill=(185,178,150),width=3)
 for i in range(0,10,2):d.line([(cx,cy),pts[i]],fill=(230,224,198),width=2)
save('star-carpet',np.array(im),fiber*.65)
save('hall-carpet',np.array([87,96,99])[None,None,:]*(.9+fiber[:,:,None]*.2),fiber*.6)
# A soft cloud deck seen only through the small cabin windows.
W,H=2048,1024
yy=np.linspace(0,1,H)[:,None,None];top=np.array([67,139,190]);bottom=np.array([214,229,233]);sky=top*(1-yy)+bottom*yy;sky=np.tile(sky,(1,W,1))
cl=Image.fromarray(np.uint8(noise(16)*255)).resize((W,H));cl=np.array(cl)/255
alpha=np.clip((cl-.35)*2.5,0,.9)*np.clip((np.linspace(0,1,H)[:,None]-.32)*3,0,1)
sky=sky*(1-alpha[:,:,None])+np.array([248,244,231])*alpha[:,:,None]
Image.fromarray(np.uint8(sky)).filter(ImageFilter.GaussianBlur(2)).save(P/'cloud-deck.jpg',quality=93)
print('Portable material maps created')
