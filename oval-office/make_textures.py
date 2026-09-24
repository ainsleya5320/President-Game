from PIL import Image, ImageDraw, ImageFont
import math, random
from pathlib import Path
P=Path(__file__).parent/'textures'
random.seed(23)
# Vector-like textile artwork is generated at high resolution for real UV-mapped cloth.
im=Image.new('RGB',(1900,1000),'#ede8d9');d=ImageDraw.Draw(im)
for i in range(13):
 if i%2==0:d.rectangle((0,i*1000/13,1900,(i+1)*1000/13),fill='#921e29')
d.rectangle((0,0,760,1000*7/13),fill='#172946')
def star(cx,cy,r):
 return [(cx+math.sin(i*math.pi/5)*r*(1 if i%2==0 else .4),cy-math.cos(i*math.pi/5)*r*(1 if i%2==0 else .4)) for i in range(10)]
for row in range(9):
 for c in range(6 if row%2==0 else 5):d.polygon(star(63+c*126+(63 if row%2 else 0),30+row*59,22),fill='#eeeadd')
im.save(P/'us_flag.png')
im=Image.new('RGB',(1400,1800),'#101e32');s=Image.open(P/'seal.png').convert('RGBA');s.thumbnail((950,950));im.paste(s,((1400-s.width)//2,(1800-s.height)//2),s)
d=ImageDraw.Draw(im)
for x,y in [(170,180),(1230,180),(170,1620),(1230,1620)]:d.polygon(star(x,y,58),fill='#e6d7af')
im.save(P/'presidential_flag.png')
# Muted woven seal, on the cream field of the Obama-era carpet.
N=3072
im=Image.new('RGB',(N,N),'#c6b78f');d=ImageDraw.Draw(im)
for inset,color,width in [(33,'#80704f',9),(55,'#e5d9b7',15),(85,'#766649',8),(120,'#b7a881',95),(180,'#8b7c58',6),(200,'#ded2ae',13)]:d.ellipse((inset,inset,N-inset,N-inset),outline=color,width=width)
font=ImageFont.truetype('C:/Windows/Fonts/times.ttf',43)
quote='THE ONLY THING WE HAVE TO FEAR IS FEAR ITSELF  •  THE ARC OF THE MORAL UNIVERSE IS LONG, BUT IT BENDS TOWARD JUSTICE  •  GOVERNMENT OF THE PEOPLE, BY THE PEOPLE, FOR THE PEOPLE  •  '
for i,c in enumerate(quote):
 a=2*math.pi*i/len(quote); tile=Image.new('RGBA',(80,80));dt=ImageDraw.Draw(tile);dt.text((40,40),c,font=font,anchor='mm',fill='#554c3b');tile=tile.rotate(-math.degrees(a),resample=Image.Resampling.BICUBIC);im.paste(tile,(int(N/2+1395*math.sin(a)-40),int(N/2-1395*math.cos(a)-40)),tile)
s=Image.open(P/'seal.png').convert('RGBA');s.thumbnail((930,930))
# The carpet insignia is deliberately rendered in a low-contrast wool palette.
px=s.load()
for y in range(s.height):
 for x in range(s.width):
  r,g,b,a=px[x,y]; lum=(r+g+b)/765;px[x,y]=(int(99+lum*75),int(92+lum*69),int(70+lum*54),a)
im.paste(s,((N-s.width)//2,(N-s.height)//2),s);im.save(P/'rug.png')
print('Textile textures ready')
