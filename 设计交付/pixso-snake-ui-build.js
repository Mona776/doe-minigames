
if(pixso.fileKey!=="rOoE_8K4uh0UTQReGuZukw")throw Error("Wrong document");
await pixso.loadFontAsync({family:"Noto Sans SC",style:"Black"});
await pixso.loadFontAsync({family:"JetBrains Mono",style:"Bold"});
const P={bg:"#0E1621",a:"#14232F",b:"#1B2E3A",jade:"#75B6A3",line:"#355F59",gold:"#E5BB58",text:"#EEE6CE",dim:"#91A7AD",brown:"#BE8249",dark:"#80502E",light:"#D69A58",ink:"#09121A",red:"#D87B66"};
function rgb(hex){return {r:parseInt(hex.slice(1,3),16)/255,g:parseInt(hex.slice(3,5),16)/255,b:parseInt(hex.slice(5,7),16)/255};}
function fill(hex){return [{type:"SOLID",color:rgb(hex)}];}
function frame(parent,name,x,y,w,h,color=P.bg){const n=pixso.createFrame();n.name=name;n.resize(w,h);n.x=x;n.y=y;n.fills=fill(color);n.clipsContent=true;parent.appendChild(n);return n;}
function rect(parent,x,y,w,h,c,name="像素"){const n=pixso.createRectangle();n.name=name;n.resize(w,h);n.x=x;n.y=y;n.fills=fill(c);parent.appendChild(n);return n;}
function text(parent,s,x,y,size=12,c=P.text,w,align="LEFT"){const n=pixso.createText();n.name=s;n.fontName={family:/[^\x00-\x7f]/.test(s)?"Noto Sans SC":"JetBrains Mono",style:/[^\x00-\x7f]/.test(s)?"Black":"Bold"};n.fontSize=size;n.characters=s;n.fills=fill(c);n.x=x;n.y=y;if(w){n.resize(w,n.height);n.textAlignHorizontal=align;}parent.appendChild(n);return n;}
function centered(parent,s,y,size=12,c=P.text){return text(parent,s,8,y,size,c,224,"CENTER");}
function edge(parent,x,y,w,h,c=P.jade){rect(parent,x+3,y,w-6,1,c);rect(parent,x+3,y+h-1,w-6,1,c);rect(parent,x,y+3,1,h-6,c);rect(parent,x+w-1,y+3,1,h-6,c);rect(parent,x+1,y+1,2,2,c);rect(parent,x+w-3,y+1,2,2,c);rect(parent,x+1,y+h-3,2,2,c);rect(parent,x+w-3,y+h-3,2,2,c);}
function comp(page,name,w,h){const n=pixso.createComponent();n.name=name;n.resize(w,h);n.fills=[];page.appendChild(n);return n;}
function inst(parent,c,x,y,w,h){const n=c.createInstance();parent.appendChild(n);n.x=x;n.y=y;if(w&&h)n.resize(w,h);return n;}
const artPage=pixso.createPage();artPage.name="03 · 组件与字体";
const statePage=pixso.createPage();statePage.name="02 · 交互与状态说明";
const page=pixso.getNodeById("0:1");page.name="01 · 完整 UI";
const kit={};
const palettes={K:P.ink,B:P.brown,E:P.dark,H:P.light,W:P.text,Y:P.gold,G:P.line,D:"#29414A",J:P.jade,R:P.red};
function sprite(name,rows,scale=2){const c=comp(artPage,name,rows[0].length*scale,rows.length*scale);rows.forEach((row,y)=>{let i=0;while(i<row.length){const ch=row[i];let j=i+1;while(j<row.length&&row[j]===ch)j++;if(ch!=="."&&palettes[ch])rect(c,i*scale,y*scale,(j-i)*scale,scale,palettes[ch]);i=j;}});kit[name]=c;return c;}
const bone=sprite("图标 / 骨头",["..........",".WW....WW.","WWWWWWWWWW",".WW....WW.",".........."],1);
const shell=sprite("图标 / 贝壳",["..YYYYY..",".YYHYHYY.","YYHYHYHYY","YYHYHYHYY",".YHYHYHY.","..YHYHY..","...YYY...","...YYY..."],2);
const star=sprite("图标 / 星星-获得",["....Y....","....Y....","...YYY...","YYYYYYYYY",".YYYYYYY.","..YYYYY..","..YY.YY..",".YY...YY.",".Y.....Y."],2);
const empty=sprite("图标 / 星星-未获得",["....D....","....D....","...DDD...","DDDDDDDDD",".DDDDDDD.","..DDDDD..","..DD.DD..",".DD...DD.",".D.....D."],2);
const arrow=sprite("图标 / 下一步-右",["..Y..","..YY.","YYYYY","..YY.","..Y.."],1);
const rock=sprite("场景 / 障碍格",[".KKKKKKKK.","KGGGGGGGGK","KGJJGGGGGK","KGJGGGGGGK","KGGGGGGGGK","KGGGGGGDGK","KGGGGGDDGK","KGGGGGGGGK","KDDDDDDDDK",".KKKKKKKK."],2);
const head=sprite("角色 / 头-侧面",["..KKKK....",".KBBBBK...","KBBBBKBKK.","BBEEBBBBBK","BEEEBBBBBR","BEEEBBBBKK","BKEKBBBBK.","KKKKKKKK..","..........",".........."],2);
const tail=sprite("角色 / 尾-侧面",["KK........","KEK.......",".KEKKKKKKK","..KBBBBBBB","..KBBBBBBB","..KBBBBBBB","...KBBBBBB","....KKKKKK","..........",".........."],2);
const body=sprite("角色 / 身体-横",["..........","..........","KKKKKKKKKK","BHHHHHHHHB","BBBBBBBBBB","BBBBBBBBBB","BBBBBBBBBB","KKKKKKKKKK","..........",".........."],2);
const frontLegs=sprite("角色 / 前脚-站立",["............","............","............","............","............","............","............","............","............","..EE.BB.....","..EE.BB.....","..KK.KK....."],2);
const backLegs=sprite("角色 / 后脚-站立",["............","............","............","............","............","............","............","............","............",".....EE.BB..",".....EE.BB..",".....KK.KK.."],2);
const checker=comp(artPage,"场景 / 200×200 棋盘",200,200);
for(let y=0;y<10;y++)for(let x=0;x<10;x++)rect(checker,x*20,y*20,20,20,(x+y)%2?P.b:P.a,"格 "+x+","+y);
const btns={};
for(const selected of [true,false]){const c=comp(artPage,"按钮 / "+(selected?"选中":"默认"),168,28);rect(c,0,0,168,28,P.bg);edge(c,0,0,168,28,selected?P.gold:P.line);if(selected)inst(c,arrow,8,11);btns[selected?"selected":"default"]=c;}
const labelStyles=[["正文 / 12",12],["标题 / 18",18],["提示 / 10",10]];
for(const [name,size]of labelStyles){const s=pixso.createTextStyle();s.name=name;s.fontName={family:"Noto Sans SC",style:"Black"};s.fontSize=size;}
for(const [name,color]of Object.entries(P)){const s=pixso.createPaintStyle();s.name="贪吃腊肠 / "+name;s.paints=fill(color);}
function dog(parent,x,y,length=4){const n=frame(parent,"Doe · 连续身体与四只脚",x,y,length*20,24,P.a);n.fills=[];inst(n,tail,0,0);for(let i=1;i<length-1;i++)inst(n,body,i*20,0);inst(n,head,(length-1)*20,0);inst(n,backLegs,-2,-2);inst(n,frontLegs,(length-1)*20-2,-2);n.clipsContent=false;return n;}
const board=frame(page,"贪吃腊肠 · 全流程 / 240×240 原尺寸",0,0,1432,1336,"#101C29");
text(board,"贪吃腊肠 / 完整 UI",32,20,26);
text(board,"240 × 240 · 8 类界面 · 18 个状态 · 图标与短标签结合",32,58,12,P.dim);
const screens=[];
function screen(name,caption){const i=screens.length,x=32+(i%5)*280,y=112+Math.floor(i/5)*300;const n=frame(board,name,x,y,240,240);edge(n,2,2,236,236);text(board,caption,x,y+248,12,P.text);screens.push(n);return n;}
function field(n,round=1,score=0,hud=true){if(hud){text(n,"ROUND "+round,20,9,11,P.gold);inst(n,bone,168,12);text(n,score+"/5",183,8,12);}inst(n,checker,20,30);edge(n,18,28,204,204);const layouts={1:[],2:[[1,2],[6,7]],3:[[1,2],[6,7],[7,1]]};for(const [x,y]of layouts[round])for(let k=0;k<3;k++)inst(n,rock,20+(x+k)*20,30+y*20);dog(n,80,110,4);inst(n,arrow,164,119);inst(n,bone,61,176,18,9);return n;}
function button(n,label,y,selected=true){inst(n,btns[selected?"selected":"default"],36,y);text(n,label,48,y+5,12,selected?P.text:P.dim,144,"CENTER");}
function hint(n,items){const widths=items.length===3?70:100;let x=(240-items.length*widths)/2;for(const[key,label]of items){edge(n,x,219,16,16,P.dim);text(n,key,x+4,219,10,P.text);text(n,label,x+21,219,10,P.dim);x+=widths;}}
function shade(n){const q=rect(n,3,3,234,234,"#08111B","暂停遮罩");q.opacity=.76;}
function panel(n,x,y,w,h){rect(n,x,y,w,h,P.bg,"弹窗底");edge(n,x,y,w,h);}
const entry=screen("01 / 游戏入口","01 入口 · A 选择 / B 确定");
centered(entry,"贪吃腊肠",23,22,P.gold);centered(entry,"SAUSAGE SNAKE",53,10,P.dim);dog(entry,70,81,5);button(entry,"开始游戏",130,true);button(entry,"玩法说明",166,false);hint(entry,[["A","选择"],["B","确定"],["C","返回"]]);
const help=screen("02 / 玩法说明","02 玩法说明 · 独立保留");
centered(help,"玩法说明",14,18,P.gold);dog(help,30,48,3);text(help,"B ×1",110,48,12,P.gold);text(help,"右转",170,48,12);
text(help,"B 按1次右转，连按3次左转",20,85,12);
text(help,"同一节拍按2次：保持方向",20,110,12);
text(help,"吃骨头变长，每关吃满5根",20,135,12);
text(help,"撞墙、障碍或自己就结束",20,160,12);
hint(help,[["B","开始"],["C","返回"]]);
for(const val of ["3","2","1","GO"]){const n=screen("03 / 倒计时-"+val,"03 倒计时 · "+val+" / 每关复用");field(n,2,0,false);rect(n,86,181,68,40,P.bg,"数字底");text(n,val,86,177,val==="GO"?28:36,P.gold,68,"CENTER");}
for(let round=1;round<=3;round++){const n=screen("04 / ROUND "+round,"04 局内 · ROUND "+round);field(n,round,round===1?0:3,true);}
const pause=screen("05 / 暂停","05 暂停 · A 直接继续 / C 退出");field(pause,2,3);shade(pause);panel(pause,38,81,164,83);centered(pause,"暂停",94,18,P.gold);text(pause,"A 继续",57,132,12);text(pause,"C 退出",132,132,12);
for(const selected of [0,1]){const n=screen("06 / 退出确认-"+selected,"06 退出确认 · "+(selected?"选中退出":"默认继续"));field(n,2,3);shade(n);panel(n,25,51,190,153);centered(n,"退出本局？",64,16,P.gold);centered(n,"退出不获得奖励",93,12,P.dim);inst(n,btns[selected===0?"selected":"default"],36,120);text(n,"继续游戏",48,125,12,P.text,144,"CENTER");inst(n,btns[selected===1?"selected":"default"],36,158);text(n,"确认退出",48,163,12,P.text,144,"CENTER");hint(n,[["A","选择"],["B","确定"],["C","取消"]]);}
for(const round of [2,3]){const n=screen("07 / 换关-"+round,"07 换关 · 自动进入 ROUND "+round);field(n,round-1,5);shade(n);panel(n,30,83,180,76);centered(n,"本关完成",94,16,P.text);centered(n,"ROUND "+round,121,22,P.gold);}
const samples=[{stars:0,bones:3,shells:5},{stars:1,bones:8,shells:10},{stars:2,bones:12,shells:20},{stars:3,bones:15,shells:30}];
for(const s of samples){const n=screen("08 / 结算-"+s.stars+"星","08 "+(s.stars===3?"全部通关":"本局结束")+" · "+s.stars+"星 / "+s.shells+"贝壳");centered(n,s.stars===3?"全部通关":"本局结束",20,18,P.gold);dog(n,70,55,5);inst(n,bone,82,96,20,10);text(n,String(s.bones).padStart(2,"0"),112,87,22);for(let i=0;i<3;i++)inst(n,i<s.stars?star:empty,78+i*32,121);inst(n,shell,85,155);text(n,"+"+s.shells,113,151,20);hint(n,[["B","再来一局"],["C","返回菜单"]]);}
for(let i=0;i<screens.length;i++){screens[i].setPluginData("screenSize","240x240");}
const assets=Object.values(kit).concat([checker,btns.selected,btns.default]);assets.forEach((n,i)=>{n.x=32+(i%5)*230;n.y=220+Math.floor(i/5)*250;});
text(artPage,"组件库 / 原生可编辑",32,22,26);
text(artPage,"中文：Noto Sans SC Black · 英文与数字：JetBrains Mono Bold",32,70,14,P.dim);
text(artPage,"所有场地、图标、角色与按钮均为可复用组件；无历史最高分。",32,102,14,P.dim);
const spec=frame(statePage,"交互与状态 / 已确认范围",0,0,1120,1000,"#101C29");
text(spec,"贪吃腊肠 / 交互说明",32,26,26,P.gold);
const notes=[
"入口：A 切换开始/玩法，B 确定，C 返回小游戏菜单。",
"玩法：独立页面保留；B 开始，C 返回入口。",
"倒计时：每关 3 → 2 → 1 → GO；无开场说明弹窗；最后1秒露出场地。",
"局内：B 按下沿右转；同节拍缓存最多3次；2次保持方向，3次左转。",
"ROUND 1：无障碍；ROUND 2：两段3格障碍；ROUND 3：三段3格障碍。",
"障碍位置由程序随机生成，UI 中展示代表性布局；速度沿用程序配置。",
"暂停：A 暂停/继续；C 进入退出确认；不增加 A 选择 / B 确认。",
"退出确认：默认继续游戏；A 选择，B 确定，C 取消；弹窗期间停止计时。",
"取消退出回到打开前状态；确认退出返回小游戏菜单，不结算奖励。",
"换关：仅显示本关完成及下一关 ROUND；自动进入倒计时，无说明/进度条。",
"结算：共用布局，不展示失败原因；B 再来一局，C 返回小游戏菜单。",
"0–4根：0星/5贝壳；5–9根：1星/10贝壳；10–14根：2星/20贝壳。",
"15根：3星/30贝壳，全部通关；神值仅后台结算。",
"移除历史最高分、奖杯、新纪录及相关占位；保留本局累计骨头。",
"标准：屏幕240×240；场地200×200；10×10网格；每格20×20。",
"中文12px；标题16–22px；底部提示10px；全部使用 Pixso 可加载字体。",
"本稿包含新增入口/玩法/退出确认；这些交互需要后续程序实现。"
];
notes.forEach((s,i)=>text(spec,String(i+1).padStart(2,"0")+"  "+s,32,90+i*47,14,P.text));
pixso.currentPage=page;pixso.currentPage.selection=[board];pixso.viewport.scrollAndZoomIntoView([board]);
pixso.root.setPluginData("snakeUi",JSON.stringify({board:board.id,screens:screens.map(n=>({id:n.id,name:n.name})),assets:assets.map(n=>({id:n.id,name:n.name})),pages:{main:page.id,states:statePage.id,assets:artPage.id},spec:spec.id}));
return {fileKey:pixso.fileKey,board:board.id,screens:screens.map(n=>({id:n.id,name:n.name})),pages:{main:page.id,states:statePage.id,assets:artPage.id},components:assets.length,fonts:["Noto Sans SC Black","JetBrains Mono Bold"]};

