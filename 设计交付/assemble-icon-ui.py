from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json, shutil

root=Path(__file__).parent/'贪吃腊肠-图标版UI'
root.mkdir(exist_ok=True)
paths=json.loads((root/'图像索引.json').read_text())
labels=['入口','图示引导','倒计时','局内','暂停','退出确认','换关','失败结算','通关结算']
font_path='/Users/chenningxin/Documents/手搓版电子宠物/tools/fonts/fusion-pixel-12px-proportional-zh_hans.ttf'
font=ImageFont.truetype(font_path,18)
board=Image.new('RGB',(996,1118),'#0E1621')
draw=ImageDraw.Draw(board)
draw.text((24,16),'贪吃腊肠 / 图标版 UI',font=ImageFont.truetype(font_path,24),fill='#EEE6CE')
draw.text((24,48),'单屏 1:1 · 页面名称仅用于图外标注',font=font,fill='#91A7AD')
report=[]
for i,((key,path),label) in enumerate(zip([(k,v) for k,v in paths.items() if k!='components'],labels)):
    im=Image.open(path).convert('RGB')
    assert im.width==im.height,(key,im.size)
    shutil.copyfile(path,root/(key+'.png'))
    x=24+(i%3)*324;y=90+(i//3)*342
    board.paste(im.resize((300,300),Image.Resampling.NEAREST),(x,y))
    draw.text((x,y+306),f'{i+1:02d} {label}',font=font,fill='#EEE6CE')
    report.append({'screen':key,'size':list(im.size),'aspect':'1:1'})
board.save(root/'贪吃腊肠-图标版UI-总览.png')
shutil.copyfile(paths['components'],root/'贪吃腊肠-图标版UI-控件图.png')
(root/'尺寸核对.json').write_text(json.dumps(report,ensure_ascii=False,indent=2))
print('Verified 9 square screens; overview assembled without aspect distortion.')
