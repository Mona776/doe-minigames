import fs from 'node:fs';
const dir='/Users/chenningxin/小游戏制作模拟器/设计交付/Doe小游戏UI';
const d=JSON.parse(fs.readFileSync(dir+'/design-data.json','utf8'));
const utility=`
if(pixso.root.name!=='Doe 小游戏 UI')throw Error('当前不是目标新文档，停止写入');
const fonts=await pixso.listAvailableFontsAsync();
const match=fonts.map(f=>f.fontName).find(f=>f.family==='Alibaba PuHuiTi'&&f.style==='Regular');
if(!match)throw Error('说明文字字体不可用');
const FONT={family:match.family,style:match.style};
await pixso.loadFontAsync(FONT);
function rgb(hex){return {r:parseInt(hex.slice(1,3),16)/255,g:parseInt(hex.slice(3,5),16)/255,b:parseInt(hex.slice(5,7),16)/255};}
function frame(p,name,x,y,w,h,color){const n=pixso.createFrame();n.name=name;n.resize(w,h);n.fills=color?[{type:'SOLID',color:rgb(color)}]:[];n.clipsContent=false;p.appendChild(n);n.x=x;n.y=y;return n;}
function note(p,str,x,y,size=14,width=1100,color='#175B52'){const n=pixso.createText();n.fontName=FONT;n.fontSize=size;n.characters=str;n.fills=[{type:'SOLID',color:rgb(color)}];n.textAutoResize='HEIGHT';n.resize(width,Math.max(size+6,24));p.appendChild(n);n.x=x;n.y=y;n.name='设计说明/'+str.slice(0,20);return n;}
function svgNode(p,src,name,x,y){const n=pixso.createNodeFromSvg(src);p.appendChild(n);n.name=name;n.x=x;n.y=y;return n;}
`;
const assetScript=utility+`
const assets=${JSON.stringify(d.assets)};
const existing=pixso.currentPage.children;
if(existing.length)throw Error('新文档不再为空，停止首次创建以免覆盖');
const page=pixso.currentPage;page.name='00-UI规范与新增组件';
const board=frame(page,'UI规范与组件 · 仅在新文档创建',0,0,1180,1450,'#F5FAF8');
note(board,'Doe 小游戏 UI / 组件与规范',28,24,28);
note(board,'独立设计文档 · 所有状态稿240×240 · UI覆盖层设计，不包含角色、场景、道具和游戏逻辑',28,66,14);
note(board,'来源规范：弹丸机UI / 青绿主色、浅青底板、阶梯形边框。原文件只读，未接入跨文件组件库。',28,92,14);
note(board,'本页组件为新建替代组件；description标注原库对应项，库可用后替换成引用实例。没有修改任何原组件。',28,118,14);
note(board,'像素文字：采用模拟器同款Fusion Pixel 12px/24px，转为矢量轮廓保证无字体替代；说明文字保持可编辑。',28,144,14);
const palette=${JSON.stringify(d.palette)};
let i=0;for(const [name,hex]of Object.entries(palette)){const x=28+(i%9)*124,y=190+Math.floor(i/9)*70;const swatch=pixso.createRectangle();swatch.resize(104,28);swatch.fills=[{type:'SOLID',color:rgb(hex)}];board.appendChild(swatch);swatch.x=x;swatch.y=y;note(board,name+' '+hex,x,y+32,11,122);i++;}
const map={};i=0;
for(const [key,a]of Object.entries(assets)){const x=28+(i%4)*282,y=362+Math.floor(i/4)*92;const comp=pixso.createComponent();comp.name='game-ui/'+key;comp.resize(a.w,a.h);comp.fills=[];comp.clipsContent=false;comp.description=a.source?'按原规范新建 / 对应原库 '+a.source+' / 待替换为库引用':'小游戏专用新增控件';board.appendChild(comp);comp.x=x;comp.y=y;svgNode(comp,a.svg,'像素图形与文字',0,0);map[key]=comp.id;note(board,key,x,y+Math.min(a.h,60)+5,10,266);i++;}
// 大面板组件单独移到末尾，避免压住相邻组件。
for(const [key,offset]of [['dialog.large',0],['dialog.small',1]]){const c=pixso.getNodeById(map[key]);c.x=28+offset*282;c.y=1450;}
board.resize(1180,1636);
return {pageId:page.id,boardId:board.id,components:map};`;
fs.writeFileSync(dir+'/01-components-request.json',JSON.stringify({name:'eval_script',arguments:{script:assetScript}}));
fs.writeFileSync(dir+'/pixso-utility.txt',utility);
console.log('Component request prepared');
