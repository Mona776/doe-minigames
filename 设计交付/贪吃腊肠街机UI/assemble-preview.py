from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json, shutil

root = Path(__file__).parent
paths = json.loads((root / '图像索引.json').read_text())
labels = [('entry','入口'),('countdown','倒计时'),('gameplay','局内'),('pause','暂停'),('exit','退出确认'),('stage','换关'),('loss','失败结算'),('partial','部分过关'),('clear','全部通关')]
font_path = '/Users/chenningxin/Documents/手搓版电子宠物/tools/fonts/fusion-pixel-12px-proportional-zh_hans.ttf'
font = ImageFont.truetype(font_path, 18)
board = Image.new('RGB', (996,1118), '#FFF8E9')
draw = ImageDraw.Draw(board)
draw.text((24,16), '贪吃腊肠 / 暖色街机 UI', font=ImageFont.truetype(font_path,24), fill='#283454')
draw.text((24,48), '单屏 1:1 · 等比例展示', font=font, fill='#283454')
report=[]
for i,(key,label) in enumerate(labels):
    image = Image.open(paths[key]).convert('RGB')
    assert image.width == image.height, (key, 'not square')
    shutil.copyfile(paths[key], root/'单屏'/f'{i+1:02d}-{label}.png')
    x=24+(i%3)*324
    y=90+(i//3)*342
    # Only assemble the presentation; source screen artwork remains untouched.
    board.paste(image.resize((300,300), Image.Resampling.NEAREST),(x,y))
    draw.text((x,y+306),f'{i+1:02d}  {label}', font=font, fill='#283454')
    report.append({'screen':label,'sourceSize':list(image.size),'displaySize':[300,300],'aspectRatio':'1:1'})
board.save(root/'贪吃腊肠-暖色街机UI-总览.png')
components=Image.open(paths['components'])
assert components.width == components.height
shutil.copyfile(paths['components'],root/'贪吃腊肠-暖色街机UI-控件图.png')
(root/'尺寸检查.json').write_text(json.dumps(report, ensure_ascii=False, indent=2))
print('Verified: 9 square screens, no aspect distortion; components square.')
