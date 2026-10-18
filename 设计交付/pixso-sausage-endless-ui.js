
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

const page=pixso.createPage();page.name="04 · Sausage Snake 无尽版";
const bone=pixso.getNodeById("3:5212"),shell=pixso.getNodeById("3:5219"),star=pixso.getNodeById("3:5264"),empty=pixso.getNodeById("3:5277"),arrow=pixso.getNodeById("3:5298"),rock=pixso.getNodeById("3:5306"),dog=pixso.getNodeById("3:5331"),f200=pixso.getNodeById("3:5354"),f240=pixso.getNodeById("3:5355");
const A={A:["01110","11011","11011","11111","11011","11011","11011"],S:["01111","11000","11000","01110","00011","00011","11110"],U:["11011","11011","11011","11011","11011","11011","01110"],G:["01111","11000","11000","11011","11011","11011","01111"],E:["11111","11000","11000","11110","11000","11000","11111"],N:["11011","11011","11111","11111","11111","11011","11011"],K:["11011","11011","11110","11100","11110","11011","11011"],O:["01110","11011","11011","11011","11011","11011","01110"],R:["11110","11011","11011","11110","11110","11011","11011"],D:["11110","11011","11011","11011","11011","11011","11110"],"1":["00100","01100","00100","00100","00100","00100","01110"],"2":["01110","11011","00011","00110","01100","11000","11111"],"3":["11110","00011","00011","01110","00011","00011","11110"]};
function bmp(p,s,x,y,scale=2,c=C.gold){for(const ch of s){const rows=A[ch];if(rows)rows.forEach((r,j)=>{for(let i=0;i<r.length;i++)if(r[i]==="1")box(p,x+i*scale,y+j*scale,scale,scale,c);});x+=6*scale;}}
function centerBmp(p,s,y,scale=3,c=C.gold){bmp(p,s,(240-(s.length*6-1)*scale)/2,y,scale,c);}
function key(p,k,x,y,label){edge(p,x,y,18,17,C.ivory);tx(p,k,x+5,y+1,11);if(label)tx(p,label,x+23,y,11);}
function footer(p,items){let x=items.length===3?14:35;for(const item of items){key(p,item[0],x,218,item[1]);x+=items.length===3?72:98;}}
function btn(p,s,y,selected=true){box(p,50,y,140,28,C.ink);edge(p,50,y,140,28,selected?C.gold:C.dim);box(p,53,y+3,134,1,selected?"#786331":"#294753");tx(p,s,56,y+4,14,C.ivory,128,"CENTER");if(selected)put(p,arrow,36,y+7,.75);}

const board=fr(page,"Sausage Snake / 无尽版完整 UI",0,0,1432,1640,"#0B1722");
tx(board,"SAUSAGE SNAKE / 无尽挑战",32,20,26);
tx(board,"复用已确认视觉 · 累计骨头 / 实时三星 · 无换关 / 无通关页 · 主动结束也结算",32,61,12,C.dim);
const screens=[];
function scr(name,label){const i=screens.length,x=32+(i%5)*280,y=112+Math.floor(i/5)*300;const n=fr(board,name,x,y,240,240);put(n,f240,0,0);edge(n,2,2,236,236);tx(board,label,x,y+248,12);screens.push(n);return n;}
function earned(score){return score>=15?3:score>=10?2:score>=5?1:0;}
function tier(score){return Math.min(8,score<15?Math.floor(score/5)+1:4+Math.floor((score-15)/10));}
const rockPlaces=[[0,1],[6,1],[2,3],[6,6],[0,7],[4,8]];
function rockCount(lv){return lv===1?0:Math.min(6,lv===2?2:lv===3?3:lv);}
function field(p,score,opts={}){
 const lv=tier(score),r=rockCount(lv),blocked=new Set();
 const tile=p.children.find(c=>c.type==="INSTANCE"&&c.name==="棋盘 / 全屏240");if(tile)tile.visible=false;
 if(opts.hud!==false){
 tx(p,"LV "+lv,20,8,12,C.gold);
 put(p,bone,63,12,.6);tx(p,String(score).padStart(2,"0"),81,6,14);
 const stars=opts.stars===undefined?earned(score):opts.stars;
 for(let i=0;i<3;i++)put(p,i<stars?star:empty,156+i*21,5);
 }
 put(p,f200,20,30);edge(p,18,28,204,204);
 for(let i=0;i<r;i++){const[x,y]=rockPlaces[i];for(let k=0;k<3;k++){put(p,rock,20+(x+k)*20,30+y*20);blocked.add(y*10+x+k);}}
 const len=Math.min(45,3+Math.floor(score/2)),path=[46,45],used=new Set(path);
 let tries=0;
 function search(){if(path.length>=len)return true;if(++tries>30000)return false;const c=path[path.length-1],x=c%10,y=Math.floor(c/10);const near=[[-1,0],[0,-1],[0,1],[1,0]].map(([dx,dy])=>[x+dx,y+dy]).filter(([xx,yy])=>xx>=0&&xx<10&&yy>=0&&yy<10).map(([xx,yy])=>yy*10+xx).filter(c=>!used.has(c)&&!blocked.has(c));
 for(const c of near){path.push(c);used.add(c);if(search())return true;used.delete(c);path.pop();}return false;}
 search();
 const layer=fr(p,"Doe / 随分数增长的连续身体",20,30,200,200);layer.fills=[];layer.clipsContent=false;
 // Draw the outline first, then the continuous fill, to avoid beaded segments.
 for(const[c,i]of path.map((c,i)=>[c,i])){const x=(c%10)*20,y=Math.floor(c/10)*20;box(layer,x+2,y+3,16,14,C.ink);if(i+1<path.length){const n=path[i+1],xx=(n%10)*20,yy=Math.floor(n/10)*20;if(yy===y)box(layer,Math.min(x,xx)+2,y+3,36,14,C.ink);else box(layer,x+2,Math.min(y,yy)+3,16,34,C.ink);}}
 for(let i=0;i<path.length;i++){const c=path[i],x=(c%10)*20,y=Math.floor(c/10)*20;box(layer,x+4,y+5,12,10,C.brown);if(i+1<path.length){const n=path[i+1],xx=(n%10)*20,yy=Math.floor(n/10)*20;if(yy===y){box(layer,Math.min(x,xx)+4,y+5,32,10,C.brown);box(layer,Math.min(x,xx)+4,y+5,32,2,C.hi);}else{box(layer,x+4,Math.min(y,yy)+5,12,30,C.brown);box(layer,x+4,Math.min(y,yy)+5,2,30,C.hi);}}}
 const h=path[0],hx=h%10*20,hy=Math.floor(h/10)*20;
 const head=fr(layer,"Doe / 头与前脚",hx,hy,24,30);head.fills=[];put(head,dog,-64,-2);
 const t=path[path.length-1],tailX=t%10*20,tailY=Math.floor(t/10)*20;
 box(layer,tailX+3,tailY+2,3,4,C.dark);box(layer,tailX+5,tailY+5,3,4,C.brown);
 box(layer,tailX+4,tailY+17,3,8,C.dark);box(layer,tailX+12,tailY+17,3,8,C.brown);box(layer,tailX+13,tailY+23,4,2,C.hi);
 put(layer,arrow,hx+24,hy+9,.5);
 let food=81;for(let c=80;c<100;c++)if(!blocked.has(c)&&!used.has(c)&&Math.abs(c%10-h%10)+Math.abs(Math.floor(c/10)-Math.floor(h/10))>=3){food=c;break;}
 put(p,bone,20+(food%10)*20,36+Math.floor(food/10)*20,.8);
 return {lv,path: path.length,requested:len};
}
function dim(p){box(p,3,3,234,234,"#06131D","场景压暗").opacity=.74;}
function panel(p,x,y,w,h){box(p,x,y,w,h,C.bg);edge(p,x,y,w,h);}
const entry=scr("01 / 无尽版入口","01 入口 · 无尽挑战");
put(entry,bone,110,21);centerBmp(entry,"SAUSAGE",44,3,C.gold);centerBmp(entry,"SNAKE",69,3,C.ivory);ct(entry,"ENDLESS",94,9,C.dim);put(entry,dog,76,110);btn(entry,"开始游戏",148,true);btn(entry,"玩法说明",183,false);footer(entry,[["A","选择"],["B","确定"],["C","返回"]]);
const help=scr("02 / 无尽版玩法","02 玩法 · 每2根增长 / 15根三星");
ct(help,"玩法说明",12,17);
key(help,"B",24,44);put(help,dog,62,43,.55);tx(help,"按1次右转",127,44,12);
key(help,"B",24,76);put(help,dog,62,75,.55);tx(help,"按3次左转",127,76,12);
ct(help,"同节拍连按，按2次不掉头",106,11,C.dim);
ct(help,"撞墙、障碍或自己就结束",128,12);
ct(help,"每2根骨头，身体长1格",148,12);
ct(help,"5 / 10 / 15 根，各亮1星",168,12,C.gold);
ct(help,"三星后继续 · 结束也结算",190,11,C.dim);
footer(help,[["B","开始"],["C","返回"]]);
for(const v of ["3","2","1","GO"]){const n=scr("03 / 开局 "+v,"03 开局 "+v+" · 仅开局倒计时");field(n,0,{hud:false});centerBmp(n,v,39,v==="GO"?4:5);}
const fieldStats=[];
for(const score of [3,8,12,18,76]){const n=scr("04 / 局内 LV "+tier(score),"04 LV "+tier(score)+" · 累计 "+score+" 根");fieldStats.push(field(n,score));}
for(let i=1;i<=3;i++){
 const score=i*5,n=scr("05 / 获得第 "+i+" 星","05 第"+i+"星 · 骨头依次飞入");
 field(n,score,{stars:i-1});
 const targetX=156+(i-1)*21;
 const glow=box(n,targetX-3,3,24,23,C.gold,"目标星位闪亮");glow.opacity=.18;edge(n,targetX-3,3,24,23,C.gold);
 for(let j=0;j<5;j++){const t=j/4,x=91+(targetX-91)*t,y=17+Math.sin(t*Math.PI)*24;const b=put(n,bone,Math.round(x),Math.round(y),.5);b.opacity=.35+j*.13;}
}
const speed=scr("06 / 升档加速","06 升档提示 · 不中断游戏");field(speed,25);panel(speed,87,36,66,23);tx(speed,"加速！",90,39,12,C.gold,60,"CENTER");
const pause=scr("07 / 暂停","07 暂停 · C 直接结束并结算");field(pause,18);dim(pause);panel(pause,39,82,162,81);ct(pause,"暂停",92,18);key(pause,"A",58,132,"继续");key(pause,"C",132,132,"结束");
for(let selected=0;selected<2;selected++){
 const n=scr("08 / 结束确认 "+selected,"08 结束确认 · "+(selected?"结束本局":"默认继续"));field(n,18);dim(n);panel(n,28,47,184,163);ct(n,"结束本局？",58,16);put(n,shell,109,86,.85);ct(n,"按当前成绩结算",114,12);btn(n,"继续游戏",140,selected===0);btn(n,"结束本局",176,selected===1);footer(n,[["A","选择"],["B","确定"],["C","取消"]]);
}
for(const s of [{n:0,b:3,v:5},{n:1,b:8,v:10},{n:2,b:12,v:20},{n:3,b:26,v:30}]){
 const n=scr("09 / 结算 "+s.n+"星","09 "+s.n+"星 · "+s.b+"根 / "+s.v+"贝壳");
 ct(n,"本局结束",14,18);put(n,dog,76,49);for(let i=0;i<3;i++)put(n,i<s.n?star:empty,79+i*29,86);
 put(n,bone,67,116,.9);tx(n,String(s.b).padStart(2,"0"),92,110,18);
 put(n,shell,135,114,.9);tx(n,"+"+s.v,159,110,18);
 panel(n,44,151,152,27);key(n,"B",55,156,"再来一局");panel(n,44,188,152,27);key(n,"C",55,193,"返回入口");
}
const notes=fr(page,"无尽版 / 状态与交互说明",0,1680,1432,820,"#0B1722");
tx(notes,"无尽版 / 与三关版的差异",32,22,24,C.gold);
const lines=[
"01  同款240×240屏幕、200×200场地；直接复用三关版角色、棋盘、图标、按钮与字体组件。",
"02  无ROUND、无换关提示、无通关页。LV仅表示难度档，身体和累计骨头不重置。",
"03  每吃2根身体增长1格；初始3格。单键转向、2次不掉头、3次左转沿用现有逻辑。",
"04  HUD：LV档位 / 骨头累计数 / 三个星位；不显示 x/5，不显示历史最高纪录。",
"05  0/5/10/15根对应0/1/2/3星；15根后继续游戏，奖励上限30贝壳。",
"06  0–4根LV1，5–9根LV2，10–14根LV3，15–24根LV4；其后每10根升档，LV8封顶。",
"07  升档追加障碍、加快速度，不暂停。空间不足时程序允许少摆障碍；画面仅为代表性布局。",
"08  5/10/15根各播放五根骨头飞入星位的动画；星点亮后弹一下，再周期扫光。",
"09  其他升档显示短暂“加速！”；不能遮住头部或中断移动；顶栏星数始终清楚可读。",
"10  A暂停/继续。运行或倒计时中C打开结束确认；暂停中C直接结束并按当前成绩结算（当前代码）。",
"11  结束确认默认继续：A切换、B确定、C取消；弹窗期间冻结游戏，取消恢复原状态。",
"12  结算统一“本局结束”，不展示死亡原因；B重开，C返回无尽版入口（当前实际路由）。",
"13  历史最高分按此前决定不绘制；代码中的最高分、破纪录显示仍需移除。",
"14  现有代码结算提示“返回菜单”与实际返回入口不一致，本稿用“返回入口”，后续同步文字。",
"15  复用 Pixso 可加载字体 Noto Sans SC Black / JetBrains Mono ExtraBold；英文标题为原生像素图形。"
];
lines.forEach((s,i)=>tx(notes,s,32,78+i*44,13,i===12||i===13?C.gold:C.ivory));
pixso.currentPage=page;pixso.currentPage.selection=[board];pixso.viewport.scrollAndZoomIntoView([board]);
return {page:page.id,board:board.id,notes:notes.id,screens:screens.map(n=>({id:n.id,name:n.name})),fieldStats};

