import json
from pathlib import Path
from PIL import ImageFont
root=Path(__file__).parent
data=json.loads((root/'pixso-text-snapshot.json').read_text())
font=ImageFont.truetype('/Users/chenningxin/小游戏制作模拟器/tools/fonts/fusion-pixel-12px-proportional-zh_hans.ttf',12)
result=[]
for screen in data:
 for t in screen['text']:
  mask=font.getmask(t['s'],mode='1');w,h=mask.size
  parts=[]
  for y in range(h):
   x=0
   while x<w:
    if mask[y*w+x]:
     end=x+1
     while end<w and mask[y*w+end]:end+=1
     parts.append(f'M{x} {y}h{end-x}v1h{x-end}z')
     x=end
    else:x+=1
  scale=3 if t['size']>=28 else 2 if t['size']>=18 else 1
  if t['s'] in ['3','2','1','GO'] and screen['name'].startswith('03 /'):
   scale=3
  width=w*scale;height=h*scale
  x=t['x']+(t['w']-width)/2 if t['align']=='CENTER' else t['x']
  if t['size']==10 and t['s'] in ['A','B','C']:x=t['x']
  result.append(dict(id=t['id'],parent=screen['id'],name=t['s'],x=x,y=t['y']+(t['h']-height)/2,width=width,height=height,path=''.join(parts),scale=scale,color=t['color']))
(root/'pixso-pixel-labels.json').write_text(json.dumps(result,ensure_ascii=False))
print(json.dumps(result,ensure_ascii=False))
