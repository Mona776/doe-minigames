
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
const page=pixso.createPage();page.name="01 · 按确认稿还原";
pixso.getNodeById("0:1").name="归档 · 前版还原偏差";
const assetPage=pixso.getNodeById("2:1");
const assetBoard=fr(assetPage,"确认风格组件 / V3",1400,0,1000,1120,"#101E2B");
tx(assetBoard,"确认风格 / 粗像素图形与字体",24,20,22);
tx(assetBoard,"字体：Noto Sans SC Black + JetBrains Mono ExtraBold",24,56,12,C.dim);
const comps=[];
function spr(name,rows,scale=2){const n=cp(assetBoard,name,rows[0].length*scale,rows.length*scale);const pal={K:C.ink,B:C.brown,H:C.hi,E:C.dark,W:C.ivory,Y:C.gold,J:C.jade,D:C.edge,R:C.red};rows.forEach((r,y)=>{let x=0;while(x<r.length){let end=x+1;while(end<r.length&&r[end]===r[x])end++;if(pal[r[x]])box(n,x*scale,y*scale,(end-x)*scale,scale,pal[r[x]]);x=end;}});comps.push(n);return n;}
const bone=spr("骨头 / 高对比",[".WW....WW.","WWWWWWWWWW","WWWWWWWWWW",".WW....WW."],2);
const shell=spr("贝壳 / 扇形",["...YYYYY...",".YYHYHYHYY.","YYHYHYHYHYY","YYHYHYHYHYY",".YHYHYHYHY.","..YHYHYHY..","...YYYYY...","....YYY...."],2);
const star=spr("星 / 亮",["....Y....","....Y....","...YYY...","YYYYYYYYY",".YYYYYYY.","..YYYYY..","..YY.YY..",".YY...YY.",".Y.....Y."],2);
const empty=spr("星 / 空",["....D....","...D.D...","...D.D...","DDD...DDD",".D.....D.","..D...D..","..D.D.D..",".D.D.D.D.",".D.....D."],2);
const arrow=spr("选择指针",["Y....","YY...","YYY..","YYYY.","YYY..","YY...","Y...."],2);
const rock=spr("石头 / 块面高光",[".DDDDDDDD.","DJJJJJJJJD","DJJJJJJJJD","DJJJJJJJJD","DJJJJJJJDD","DJJJJJJDDD","DJJJJJDDDD","DJJJJDDDDD","DDDDDDDDDD",".DDDDDDDD."],2);
const dog=cp(assetBoard,"Doe / 侧面四脚",88,30);comps.push(dog);
box(dog,4,7,63,16,C.ink);box(dog,6,9,62,11,C.brown);box(dog,6,9,62,3,C.hi);
box(dog,0,1,3,3,C.dark);box(dog,2,4,4,5,C.brown);box(dog,6,8,3,3,C.brown);
box(dog,64,5,13,17,C.brown);box(dog,66,2,10,3,C.hi);box(dog,76,8,9,10,C.brown);box(dog,83,9,5,4,C.ink);
box(dog,67,9,5,12,C.dark);box(dog,69,7,4,3,C.brown);box(dog,77,5,3,3,C.ink);box(dog,84,14,3,3,C.red);
box(dog,11,21,4,7,C.dark);box(dog,15,24,5,4,C.dark);box(dog,22,21,4,7,C.brown);box(dog,24,25,5,3,C.hi);
box(dog,65,21,4,7,C.dark);box(dog,68,25,4,3,C.dark);box(dog,75,21,4,7,C.brown);box(dog,77,25,4,3,C.hi);
const f200=cp(assetBoard,"棋盘 / 场地200",200,200),f240=cp(assetBoard,"棋盘 / 全屏240",240,240);comps.push(f200,f240);
for(const c of [f200,f240])for(let y=0;y<c.height;y+=20)for(let x=0;x<c.width;x+=20)box(c,x,y,20,20,((x+y)/20)%2?C.b:C.a,"棋盘格");
const A={A:["01110","11011","11011","11111","11011","11011","11011"],S:["01111","11000","11000","01110","00011","00011","11110"],U:["11011","11011","11011","11011","11011","11011","01110"],G:["01111","11000","11000","11011","11011","11011","01111"],E:["11111","11000","11000","11110","11000","11000","11111"],N:["11011","11011","11111","11111","11111","11011","11011"],K:["11011","11011","11110","11100","11110","11011","11011"],O:["01110","11011","11011","11011","11011","11011","01110"],R:["11110","11011","11011","11110","11110","11011","11011"],D:["11110","11011","11011","11011","11011","11011","11110"],"1":["00100","01100","00100","00100","00100","00100","01110"],"2":["01110","11011","00011","00110","01100","11000","11111"],"3":["11110","00011","00011","01110","00011","00011","11110"]};
function bmp(p,s,x,y,scale=2,c=C.gold){for(const ch of s){const rows=A[ch];if(rows)rows.forEach((r,j)=>{for(let i=0;i<r.length;i++)if(r[i]==="1")box(p,x+i*scale,y+j*scale,scale,scale,c);});x+=6*scale;}}
function centerBmp(p,s,y,scale=3,c=C.gold){bmp(p,s,(240-(s.length*6-1)*scale)/2,y,scale,c);}
function key(p,k,x,y,label){edge(p,x,y,18,17,C.ivory);tx(p,k,x+5,y+1,11);if(label)tx(p,label,x+23,y,11);}
function footer(p,items){let x=items.length===3?14:35;for(const item of items){key(p,item[0],x,218,item[1]);x+=items.length===3?72:98;}}
function btn(p,s,y,selected=true){box(p,50,y,140,28,C.ink);edge(p,50,y,140,28,selected?C.gold:C.dim);box(p,53,y+3,134,1,selected?"#786331":"#294753");tx(p,s,56,y+4,14,C.ivory,128,"CENTER");if(selected)put(p,arrow,36,y+7,.75);}
const board=fr(page,"贪吃腊肠 / 确认风格 · 全流程",0,0,1432,1336,"#0B1722");
tx(board,"贪吃腊肠 / 按确认稿还原",32,20,26);
tx(board,"保留粗像素标题、棋盘背景、角色层次与按钮；功能按最终确认说明",32,61,12,C.dim);
const screens=[];
function scr(name,label){const i=screens.length,x=32+(i%5)*280,y=112+Math.floor(i/5)*300;const n=fr(board,name,x,y,240,240);put(n,f240,0,0);edge(n,2,2,236,236);tx(board,label,x,y+248,12);screens.push(n);return n;}
function scene(p,r=1,score=0,hud=true){
 p.children.find(c=>c.type==="INSTANCE"&&c.name==="棋盘 / 全屏240").visible=false;
 if(hud){tx(p,"ROUND "+r,20,8,12);put(p,bone,164,12,.6);tx(p,score+"/5",183,8,12);}
 put(p,f200,20,30);edge(p,18,28,204,204);
 const layouts={1:[],2:[[1,2],[6,7]],3:[[1,2],[6,7],[7,1]]};
 for(const[x,y]of layouts[r])for(let k=0;k<3;k++)put(p,rock,20+(x+k)*20,30+y*20);
 put(p,bone,60,175,.8);
 if(r===1){put(p,dog,78,109);}
 else{
  box(p,80,100,15,30,C.ink);box(p,82,100,11,31,C.brown);box(p,82,100,3,31,C.hi);
  box(p,80,96,5,5,C.dark);put(p,dog,80,119);
 }
 put(p,arrow,170,r===1?120:130,.5);
}
function dim(p){box(p,3,3,234,234,"#06131D","场景压暗").opacity=.74;}
function panel(p,x,y,w,h){box(p,x,y,w,h,C.bg);edge(p,x,y,w,h);}
const entry=scr("01 / 入口","01 入口 · 保留游戏首页");
put(entry,bone,110,21);centerBmp(entry,"SAUSAGE",44,3,C.gold);centerBmp(entry,"SNAKE",69,3,C.ivory);put(entry,dog,76,108);
btn(entry,"开始游戏",148,true);btn(entry,"玩法说明",183,false);footer(entry,[["A","选择"],["B","确定"],["C","返回"]]);
const help=scr("02 / 玩法说明","02 玩法说明 · 图示与短句");
ct(help,"玩法说明",12,17);
key(help,"B",25,49);put(help,dog,68,48,.6);tx(help,"↷",139,37,26,C.gold);tx(help,"按1次：右转",64,77,12);
key(help,"B",25,104);put(help,dog,68,103,.6);tx(help,"↶",139,92,26,C.gold);tx(help,"按3次：左转",64,131,12);
put(help,dog,20,160,.55);put(help,bone,95,167,.8);put(help,dog,136,160,.8);
ct(help,"吃骨头，身体变长",186,12);
footer(help,[["B","开始"],["C","返回"]]);
for(const v of ["3","2","1","GO"]){const n=scr("03 / 倒计时 "+v,"03 "+v+" · 每关复用");scene(n,2,0,false);centerBmp(n,v,39,v==="GO"?4:5);}
for(let r=1;r<=3;r++){const n=scr("04 / ROUND "+r,"04 ROUND "+r+" · "+(r===1?"无障碍":r===2?"两段障碍":"三段障碍"));scene(n,r,r===1?0:3);}
const pause=scr("05 / 暂停","05 暂停 · A 直接继续");scene(pause,2,3);dim(pause);panel(pause,39,82,162,81);ct(pause,"暂停",92,18);key(pause,"A",58,132,"继续");key(pause,"C",132,132,"退出");
for(let selected=0;selected<2;selected++){
 const n=scr("06 / 退出确认 "+selected,"06 退出确认 · "+(selected?"确认退出":"默认继续"));scene(n,2,3);dim(n);panel(n,28,47,184,163);ct(n,"退出本局？",58,16);
 put(n,shell,109,86,.85);box(n,105,91,29,3,C.red).rotation=32;ct(n,"退出不获得奖励",114,12);
 btn(n,"继续游戏",140,selected===0);btn(n,"确认退出",176,selected===1);footer(n,[["A","选择"],["B","确定"],["C","取消"]]);
}
for(const r of [2,3]){const n=scr("07 / 换关 "+r,"07 换关 · 自动进入 ROUND "+r);scene(n,r-1,5);dim(n);panel(n,34,85,172,75);ct(n,"本关完成",96,16);centerBmp(n,"ROUND"+r,126,2,C.gold);}
for(const s of [{n:0,b:3,v:5},{n:1,b:8,v:10},{n:2,b:12,v:20},{n:3,b:15,v:30}]){
 const n=scr("08 / 结算 "+s.n+"星","08 "+s.n+"星 · "+s.b+"根 / "+s.v+"贝壳");
 ct(n,s.n===3?"全部通关":"本局结束",14,18);
 put(n,dog,76,49);
 for(let i=0;i<3;i++)put(n,i<s.n?star:empty,79+i*29,86);
 put(n,bone,67,116,.9);tx(n,String(s.b).padStart(2,"0"),92,110,18);
 put(n,shell,135,114,.9);tx(n,"+"+s.v,159,110,18);
 // These are direct physical-key commands, not a selectable menu.
 panel(n,44,151,152,27);key(n,"B",55,156,"再来一局");
 panel(n,44,188,152,27);key(n,"C",55,193,"返回菜单");
}
comps.forEach((n,i)=>{n.x=24+(i%4)*240;n.y=124+Math.floor(i/4)*300;tx(assetBoard,n.name,n.x,n.y+248,12,C.dim);});
pixso.currentPage=page;pixso.currentPage.selection=[entry];pixso.viewport.scrollAndZoomIntoView([entry]);pixso.viewport.zoom=3;
return {page:page.id,board:board.id,entry:entry.id,gameplay:screens.find(n=>n.name==="04 / ROUND 2").id,screens:screens.map(n=>({id:n.id,name:n.name})),assets:assetBoard.id};

