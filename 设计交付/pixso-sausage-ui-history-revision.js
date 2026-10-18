
if(pixso.fileKey!=="rOoE_8K4uh0UTQReGuZukw")throw Error("Wrong file");
await pixso.loadFontAsync({family:"Noto Sans SC",style:"Black"});
await pixso.loadFontAsync({family:"JetBrains Mono",style:"ExtraBold"});
const C={bg:"#081923",a:"#0D2633",b:"#123241",jade:"#66C7B5",edge:"#244D58",gold:"#F3C04F",ivory:"#F4EDD6",dim:"#8DA5AD",brown:"#C17D3E",hi:"#E9A45C",dark:"#87502D",ink:"#10202B",red:"#D78D7B"};
const rgb=h=>({r:parseInt(h.slice(1,3),16)/255,g:parseInt(h.slice(3,5),16)/255,b:parseInt(h.slice(5,7),16)/255});
const fills=c=>[{type:"SOLID",color:rgb(c)}];
function box(p,x,y,w,h,c,name="像素块"){const n=pixso.createRectangle();n.name=name;n.resize(w,h);n.fills=fills(c);p.appendChild(n);n.x=x;n.y=y;return n;}
function fr(p,name,x,y,w,h,c=C.bg){const n=pixso.createFrame();n.name=name;n.resize(w,h);n.fills=fills(c);n.clipsContent=true;p.appendChild(n);n.x=x;n.y=y;return n;}
function tx(p,s,x,y,size=12,c=C.ivory,w,align="LEFT"){const n=pixso.createText();n.name=s;n.fontName={family:/[^\x00-\x7f]/.test(s)?"Noto Sans SC":"JetBrains Mono",style:/[^\x00-\x7f]/.test(s)?"Black":"ExtraBold"};n.fontSize=size;n.characters=s;n.fills=fills(c);if(w){n.resize(w,n.height);n.textAlignHorizontal=align;}p.appendChild(n);n.x=x;n.y=y;return n;}
function ct(p,s,y,size=14,c=C.ivory){return tx(p,s,8,y,size,c,224,"CENTER");}
function edge(p,x,y,w,h,c=C.jade){box(p,x+4,y,w-8,1,C.edge);box(p,x+4,y+1,w-8,1,c);box(p,x+4,y+h-2,w-8,1,c);box(p,x+4,y+h-1,w-8,1,C.edge);box(p,x,y+4,1,h-8,C.edge);box(p,x+1,y+4,1,h-8,c);box(p,x+w-2,y+4,1,h-8,c);box(p,x+w-1,y+4,1,h-8,C.edge);[[2,2],[w-4,2],[2,h-4],[w-4,h-4]].forEach(([a,b])=>box(p,x+a,y+b,2,2,c));}
function cp(p,name,w,h){const n=pixso.createComponent();n.name=name;n.resize(w,h);n.fills=[];p.appendChild(n);return n;}
function put(p,c,x,y,scale=1){const n=c.createInstance();p.appendChild(n);if(scale!==1)n.rescale(scale);n.x=x;n.y=y;return n;}


const page=pixso.getNodeById("3:14048"),board=pixso.getNodeById("3:14049");
const before=board.children.filter(n=>n.type==="FRAME");
const removed=[];
for(const n of before.filter(n=>n.name.startsWith("04 /")||n.name.startsWith("05 /"))){
 const caption=board.children.find(c=>c.type==="TEXT"&&c.x===n.x&&c.y===n.y+248);
 if(caption)caption.remove();removed.push(n.name);n.remove();
}
const oldCaptions=new Map();
for(const n of board.children.filter(n=>n.type==="FRAME")){
 const c=board.children.find(c=>c.type==="TEXT"&&c.x===n.x&&c.y===n.y+248);
 if(c)oldCaptions.set(n.id,c);
}
const bone=pixso.getNodeById("3:5212"),star=pixso.getNodeById("3:5264"),empty=pixso.getNodeById("3:5277");
const recordSource=pixso.getNodeById("3:26069"),newRecord=recordSource.clone();board.appendChild(newRecord);newRecord.name="09 / 结算 3星-新纪录";
for(const t of newRecord.children.filter(n=>n.type==="TEXT"))if(t.characters==="26"){t.characters="77";t.name="77";}
const newCaption=tx(board,"09 3星 · 新纪录 77 根",32,0,12);oldCaptions.set(newRecord.id,newCaption);
function best(p,s,y=135,newBest=false){return tx(p,s,8,y,10,newBest?C.gold:C.dim,224,"CENTER");}
const entry=pixso.getNodeById("3:14052");
for(const n of entry.children){
 if(n.type==="RECTANGLE"&&n.y>=21&&n.y<90)n.y-=8;
 if(n.type==="INSTANCE"&&n.name==="骨头 / 高对比"&&n.y===21)n.y=13;
 if(n.type==="INSTANCE"&&n.name==="Doe / 侧面四脚")n.y=99;
 if(n.type==="TEXT"&&n.characters==="ENDLESS")n.y=84;
}
best(entry,"最高纪录 76",130);
let headers=0,results=0;
for(const n of board.children.filter(n=>n.type==="FRAME")){
 const lv=n.children.find(c=>c.type==="TEXT"&&/^LV \d/.test(c.characters));
 if(lv){
  lv.fontSize=10;lv.x=20;lv.y=9;
  const score=n.children.find(c=>c.type==="TEXT"&&/^\d+$/.test(c.characters));
  if(score){score.fontSize=12;score.x=75;score.y=8;}
  const icon=n.children.find(c=>c.type==="INSTANCE"&&c.name==="骨头 / 高对比"&&c.y<30);
  if(icon){icon.x=55;icon.y=12;icon.rescale(.85);}
  const stars=n.children.filter(c=>c.type==="INSTANCE"&&["星 / 亮","星 / 空"].includes(c.name)&&c.y<30);
  stars.forEach((s,i)=>{s.rescale(.5);s.x=111+13*i;s.y=11;});
  const speed=n.name.startsWith("06 /");
  if(speed){
   for(const c of n.children.slice()){
    if(c.type==="TEXT"&&c.characters==="加速！")c.remove();
    else if(c.type==="RECTANGLE"&&c.x>=87&&c.y>=36&&c.x+c.width<=153&&c.y+c.height<=59)c.remove();
   }
  }
  const high=tx(n,speed?"加速!":"最高 76",158,8,10,speed?C.gold:C.dim,62,"RIGHT");high.name="历史最高 / 升档提示";
  const dim=n.children.find(c=>c.type==="RECTANGLE"&&c.name==="场景压暗");
  if(dim)n.insertChild(n.children.indexOf(dim),high);
  headers++;
 }
 if(n.name.startsWith("09 /")){best(n,n.id===newRecord.id?"新纪录！77":"最高纪录 76",135,n.id===newRecord.id);results++;}
}
const remap={"06":"04","07":"05","08":"06","09":"07"};
const frames=board.children.filter(n=>n.type==="FRAME");
frames.forEach((n,i)=>{
 const caption=oldCaptions.get(n.id),prefix=n.name.slice(0,2),mapped=remap[prefix];
 if(mapped)n.name=mapped+n.name.slice(2);
 n.x=32+(i%5)*280;n.y=112+Math.floor(i/5)*300;
 if(caption){caption.x=n.x;caption.y=n.y+248;if(mapped)caption.characters=mapped+caption.characters.slice(2);}
});
board.resize(1432,1040);
const subtitle=board.children.find(n=>n.type==="TEXT"&&n.y===61);
if(subtitle)subtitle.characters="无尽版 UI · 保留历史高分 · 不含局内档位与升星过程示意";
const notes=pixso.getNodeById("3:26399");notes.y=1080;
const replacements={
"04":"04  HUD：LV档位 / 骨头累计数 / 三个星位 / 历史最高；不显示 x/5。右侧最高分升档时短暂显示“加速!”。",
"07":"07  身体变化、障碍摆放、局内档位示例属于玩法，本套UI不绘制，交由程序与美术处理。",
"08":"08  不绘制五根骨头飞入星位的过程；UI只提供星位的空、亮与高光状态。",
"13":"13  无尽版保留历史最高：入口显示纪录；局内超纪录时高亮跟随；结算提供普通和新纪录两种状态。"
};
for(const t of notes.children.filter(n=>n.type==="TEXT")){const key=t.characters.slice(0,2);if(replacements[key])t.characters=replacements[key];}
const kit=fr(page,"无尽版 / 顶栏 UI 组件",1480,0,640,650,"#0B1722");
tx(kit,"顶栏 / 历史高分与星位",24,22,23);
tx(kit,"仅绘制UI组件；不包含角色、障碍或升星飞行过程",24,64,12,C.dim);
const variants=[{label:"常规 · 最高纪录常驻",lv:2,score:8,stars:1,high:"最高 76"},{label:"刷新纪录 · 数字高亮跟随本局",lv:8,score:77,stars:3,high:"最高 77",gold:true},{label:"升档 · 暂时替换最高分，结束后恢复",lv:5,score:25,stars:3,high:"加速!",gold:true},{label:"开局 · 保留历史纪录，星位清空",lv:1,score:0,stars:0,high:"最高 76"}];
const hudIds=[];
variants.forEach((v,i)=>{
 tx(kit,v.label,24,110+i*110,12,C.dim);
 const c=pixso.createComponent();c.name="无尽顶栏 / "+v.label;c.resize(240,30);c.fills=fills(C.bg);kit.appendChild(c);c.x=24;c.y=140+i*110;
 tx(c,"LV "+v.lv,20,9,10,C.gold);put(c,bone,55,12,.5);tx(c,String(v.score).padStart(2,"0"),75,8,12);
 for(let j=0;j<3;j++)put(c,j<v.stars?star:empty,111+j*13,11,.5);
 tx(c,v.high,158,8,10,v.gold?C.gold:C.dim,62,"RIGHT");
 hudIds.push(c.id);
});
pixso.currentPage=page;pixso.currentPage.selection=[board];pixso.viewport.scrollAndZoomIntoView([board]);
return {removed,remainingScreens:frames.length,historyHeaders:headers,historyResults:results,newRecord:newRecord.id,kit:kit.id,hudComponents:hudIds};

