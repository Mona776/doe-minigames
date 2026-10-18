import fs from 'node:fs';
import {createRequire} from 'node:module';
const require=createRequire(import.meta.url);
const ot=require('/Users/chenningxin/小游戏制作模拟器/tools/node_modules/lv_font_conv/node_modules/opentype.js');
const font=ot.loadSync('/Users/chenningxin/Documents/手搓版电子宠物/tools/fonts/fusion-pixel-12px-proportional-zh_hans.ttf');
const out='/Users/chenningxin/小游戏制作模拟器/设计交付/Doe小游戏UI';
const P={ink:'#175B52',edge:'#19766A',teal:'#A4D8D8',paper:'#E1F8FD',white:'#FFFFFF',quiet:'#668D87',mint:'#E8F7F2',success:'#86B845',lime:'#C2FF4F',gold:'#FCB800',pink:'#FBA0B4',error:'#FF4D4F',lavender:'#B693F8',wash:'#F5FAF8'};
const esc=s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('"','&quot;');
const rect=(x,y,w,h,c)=>`<rect x="${x}" y="${y}" width="${w}" height="${h}" fill="${c}"/>`;
const path=(d,c)=>`<path d="${d}" fill="${c}"/>`;
const glyphMissing=new Set();
function text(s,x,y,size=12,c=P.ink,align='left'){
  for(const ch of s) if(!font.charToGlyphIndex(ch)&&ch!==' ')glyphMissing.add(ch);
  const w=font.getAdvanceWidth(s,size);
  if(align==='center')x-=w/2; if(align==='right')x-=w;
  const d=font.getPath(s,Math.round(x),y+size,size).toPathData(2);
  return path(d,c);
}
function panel(w,h,fill=P.paper,border=P.edge){
  const d=`M4 0H${w-4}V2H${w-2}V4H${w}V${h-4}H${w-2}V${h-2}H${w-4}V${h}H4V${h-2}H2V${h-4}H0V4H2V2H4Z`;
  const inner=`M5 3H${w-5}V5H${w-3}V${h-6}H${w-5}V${h-3}H5V${h-6}H3V5H5Z`;
  return path(d,border)+path(inner,fill)+rect(6,h-6,w-12,3,P.teal)+rect(6,4,w-12,2,P.white);
}
const svg=(w,h,s)=>`<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">${s}</svg>`;
const translate=(s,x,y)=>`<g transform="translate(${x},${y})">${s}</g>`;
function pixels(rows,colorMap,scale=1){let r='';for(let y=0;y<rows.length;y++)for(let x=0;x<rows[y].length;x++){const c=colorMap[rows[y][x]];if(c)r+=rect(x*scale,y*scale,scale,scale,c);}return r;}
const heartRows=['.KK.KK.','KPPKPPK','KPPPPPK','.KPPPK.','..KPK..','...K...'];
const heart=(full=true)=>pixels(heartRows,{K:P.edge,P:full?P.pink:P.paper},2);
const starRows=['....K....','...KYK...','KKKYYYKKK','.KYYYYYK.','..KYYYK..','.KYYYYYK.','.KYKKKYK.','..K...K..'];
const star=(full=true)=>pixels(starRows,{K:full?'#7E0001':P.quiet,Y:full?P.gold:P.paper},2);
const shellRows=['..KKKK..','.KWWWWK.','KWTWTWWK','KTWTWTWK','KWWTWWWK','.KWWWWK.','..KKKK..'];
const shell=()=>pixels(shellRows,{K:P.edge,W:P.white,T:P.teal},2);
const diamond=(c=P.gold)=>pixels(['..K..','.KYK.','KYYYK','.KYK.','..K..'],{K:P.edge,Y:c},2);
const controller=pixels(['..KKKKKK..','.KWWWWWWK.','KWWWWWWWWK','KWKKWWPPWK','KWKKWWPPWK','KWWWWWWWWK','.KKK..KKK.'],{K:P.edge,W:P.white,P:P.pink},2);
const pauseIcon=rect(2,2,4,12,P.ink)+rect(12,2,4,12,P.ink);
function arrow(dir=0,c=P.ink){return `<g transform="translate(8,8) rotate(${dir*90}) translate(-8,-8)">${pixels(['...KK...','..KKKK..','.KKKKKK.','KKKKKKKK','...KK...','...KK...'],{K:c},2)}</g>`;}
function key(k,label){return panel(18,18,P.white)+text(k,9,2,12,P.ink,'center')+text(label,23,3);}
const A={};
function asset(name,w,h,s,source=null){A[name]={name,w,h,svg:svg(w,h,s),source};return name;}
asset('title',220,34,panel(220,34)+text('小游戏',110,3,24,P.ink,'center'),'bg/title');
asset('hint',216,24,panel(216,24)+text('选择:切换  确定:进入',108,5,12,P.ink,'center'),'control/dialog-sm');
asset('row.normal',216,42,panel(216,42,P.white));
asset('row.focus',216,42,panel(216,42,P.teal)+rect(4,6,4,30,P.ink)+translate(arrow(1),190,13),'bg/selected');
asset('dialog.large',216,150,panel(216,150,P.paper),'control/dialog-lg');
asset('dialog.small',192,90,panel(192,90,P.paper),'control/dialog-sm');
asset('button.normal',180,30,panel(180,30,P.white));
asset('button.focus',180,30,panel(180,30,P.teal)+translate(diamond(),8,10));
asset('heart.full',14,12,heart(true),'icon/heart');
asset('heart.empty',14,12,heart(false),'icon/heart');
asset('star.full',18,16,star(true),'decor/stars');
asset('star.empty',18,16,star(false),'decor/stars');
asset('shell',16,14,shell(),'icon/shell-pearl-28');
asset('controller',20,14,controller,'icon/game');
asset('position.marker',10,10,diamond(),'新增/rope-marker');
for(let d=0;d<4;d++)asset('arrow.'+d,16,16,arrow(d),'新增/next-direction');
asset('key.a',64,18,key('A','选择'));
asset('key.b',64,18,key('B','确定'));
asset('key.c',64,18,key('C','返回'));
asset('hud.base',216,28,panel(216,28,P.white));
for(let tier=0;tier<3;tier++){
 let target=[58,72,86][tier],range=[10,8,7][tier],perfect=[4,3,3][tier];
 let s=panel(208,26,P.white);let from=6+target*1.96;
 s+=rect(6,9,196,8,P.teal)+rect(from-range*1.96,9,range*3.92,8,P.success)+rect(from-perfect*1.96,9,perfect*3.92,8,P.lime);
 s+=rect(from-1,3,2,20,P.error)+rect(from-6,1,12,3,P.error)+rect(from-1,22,2,3,P.error);
 s+=rect(from-12,5,4,17,P.ink)+rect(from-15,5,10,3,P.ink);
 asset('meter.tier'+(tier+1),208,26,s,'control/progress');
}
asset('round.progress',160,14,rect(0,6,160,2,P.teal)+[0,1,2,3,4].map(i=>rect(i*38,2,8,10,i<3?P.gold:P.white)+rect(i*38,11,8,2,P.edge)).join(''));
for(const [id,label,color]of [['perfect','完美反拉!','#3B6F25'],['good','反拉成功',P.ink],['weak','力量不足',P.error],['slip','用力过猛',P.error],['nice','Nice!',P.ink],['tennis.perfect','Perfect!','#3B6F25'],['early','跳早了',P.error],['late','跳晚了',P.error],['miss','漏接了',P.error]])asset('feedback.'+id,132,28,panel(132,28,P.white)+text(label,66,6,12,color,'center'));
for(let n=1;n<=3;n++)asset('count.'+n,64,64,text(String(n),32,1,60,P.ink,'center'));
asset('count.go',120,40,text('开始!',60,3,24,P.ink,'center'));
asset('record',92,22,panel(92,22,P.teal)+text('新纪录!',46,4,12,P.ink,'center'));

const groups=[];
function group(name,description){const g={name,description,screens:[]};groups.push(g);return g;}
let current;
function screen(name,description){const s={name,description,layers:[]};current.screens.push(s);return s;}
function use(s,key,x,y){s.layers.push({kind:'instance',key,x,y});}
function art(s,name,x,y,w,h,v){s.layers.push({kind:'svg',name,x,y,w,h,svg:svg(w,h,v)});}
function txt(s,str,x,y,size=12,c=P.ink,align='left'){let w=Math.ceil(font.getAdvanceWidth(str,size)),h=size+4;if(align==='center')x-=w/2;if(align==='right')x-=w;art(s,'文字/'+str,Math.round(x),y,w,h,text(str,0,0,size,c));}
function scene(s,x=12,y=70,w=216,h=132,label='游戏画面占位'){
 art(s,'占位层/不作为UI素材导出',x,y,w,h,rect(0,0,w,h,P.wash)+`<path d="M0 0H${w}V${h}H0Z" fill="none" stroke="${P.teal}" stroke-dasharray="4 4"/>`);
}
function footer(s,label){use(s,'hint',12,204);art(s,'覆盖提示文案',18,210,204,12,rect(0,0,204,12,P.paper)+text(label,102,0,12,P.ink,'center'));}
function miniTitle(s,title){art(s,'标题栏',12,12,216,30,panel(216,30)+text(title,108,3,24,P.ink,'center'));}
function button(s,label,y,selected=true){use(s,selected?'button.focus':'button.normal',30,y);txt(s,label,120,y+7,12,P.ink,'center');}

current=group('01-通用 UI','240×240 · 2px阶梯边框 · 12/24px像素文字 · 场景占位层不导出');
for(let focus=0;focus<3;focus++){
 let s=screen('菜单/'+['拔河','贪吃腊肠','接网球'][focus]+'选中','选择切换条目，确定进入；复用同一菜单项组件');use(s,'title',10,10);use(s,'controller',110,47);
 ['Doe 拔河','贪吃腊肠','Doe 接网球'].forEach((t,i)=>{use(s,i===focus?'row.focus':'row.normal',12,70+i*44);txt(s,String(i+1).padStart(2,'0'),24,84+i*44);txt(s,t,59,76+i*44,24);});footer(s,'选择:切换  确定:进入');
}
const guide=[['Doe 拔河','黄针对准主人拉力时','按确定，成功反拉','每轮只有一次锁定机会'],['贪吃腊肠','按一次右转，连按三次左转','吃满5根骨头进入下一关','选择键暂停，返回键退出'],['Doe 接网球','看准球飞过来的时机','按确定，跳起叼住网球','漏接3次后结束训练']];
for(const [title,a,b,c]of guide){let s=screen('说明/'+title,'游戏开始前的操作说明，不占用局内场景');miniTitle(s,title);use(s,'dialog.large',12,54);txt(s,'怎么玩',120,70,24,P.ink,'center');txt(s,a,120,110,12,P.ink,'center');txt(s,b,120,134,12,P.ink,'center');txt(s,c,120,169,12,P.quiet,'center');footer(s,'确定:开始  返回:菜单');}
for(const n of [3,2,1,0]){let s=screen('倒计时/'+(n||'开始'),'只显示倒计时覆盖层，局内HUD全部隐藏');scene(s,12,12,216,216,'场景占位');use(s,n?'count.'+n:'count.go',n?88:60,n?86:92);}
for(const selected of [0,1]){let s=screen('暂停/'+(selected?'退出选中':'继续选中'),'移动计时冻结，选择切换，确定执行');scene(s,12,12,216,216,'冻结的游戏画面');use(s,'dialog.large',12,43);txt(s,'暂停中',120,55,24,P.ink,'center');button(s,'继续游戏',99,selected===0);button(s,'退出游戏',137,selected===1);footer(s,'选择:切换  确定:执行');}
for(const selected of [0,1]){let s=screen('退出确认/'+(selected?'退出选中':'留下选中'),'退出不给奖励；避免误触直接丢失本局');scene(s,12,12,216,216,'冻结的游戏画面');use(s,'dialog.large',12,43);txt(s,'离开游戏?',120,55,24,P.ink,'center');txt(s,'本局不会获得奖励',120,85,12,P.error,'center');button(s,'继续游戏',110,selected===0);button(s,'确认退出',148,selected===1);footer(s,'选择:切换  确定:执行');}
for(let stars=0;stars<=3;stars++){let s=screen('结算/'+stars+'星','只显示贝壳，神值后台结算；星级规则由各游戏提供');scene(s,12,12,216,216,'结算时的游戏画面');use(s,'dialog.large',12,34);txt(s,stars?'挑战完成':'再试一次',120,47,24,P.ink,'center');for(let i=0;i<3;i++)use(s,i<stars?'star.full':'star.empty',82+i*30,86);txt(s,stars?'本局表现 '+stars+' 星':'本局表现 0 星',120,114,12,P.quiet,'center');use(s,'shell',76,148);txt(s,'贝壳 +'+[5,10,20,30][stars],101,148);footer(s,'确定:再来  返回:菜单');}
{let s=screen('结算/新纪录','附加新纪录标识，不改变奖励档位');scene(s,12,12,216,216,'结算时的游戏画面');use(s,'dialog.large',12,34);txt(s,'挑战完成',120,46,24,P.ink,'center');use(s,'record',74,78);txt(s,'最高分 12',120,110,24,P.ink,'center');use(s,'shell',78,150);txt(s,'贝壳 +30',102,150);footer(s,'确定:再来  返回:菜单');}

current=group('02-拔河 UI','7个绳标只表示位置，不表示回合数；第3回合以后难度保持；无超时');
function tugHUD(s,tier=1){use(s,'hud.base',12,10);txt(s,'第 '+tier+' 回合',120,17,12,P.ink,'center');art(s,'7格绳子位置条',14,42,212,36,text('DOE',0,10)+text('主人',212,10,12,P.ink,'right')+rect(34,17,142,2,P.edge)+Array.from({length:7},(_,i)=>rect(34+i*23, i===3?12:14,2,i===3?12:8,P.edge)).join('')+translate(diamond(),98,12));}
for(let tier=1;tier<=3;tier++){let s=screen('力量匹配/第'+tier+'回合','红色=主人目标；青绿=当前力量；绿区=成功；亮绿=完美');tugHUD(s,tier);scene(s,12,86,216,68);txt(s,'主人拉力 '+[58,72,86][tier-1],120,159,12,P.ink,'center');use(s,'meter.tier'+tier,16,175);footer(s,'确定:锁定力量');}
{let s=screen('主人发力/力量条隐藏','发力动画期间隐藏力量条；无可执行按钮');tugHUD(s,2);scene(s,12,86,216,104,'主人发力动画占位');footer(s,'看主人发力，等待力量条');}
for(const id of ['perfect','good','weak','slip']){let s=screen('回合反馈/'+id,'反馈在游戏画面上方短暂出现，原场景不重排');tugHUD(s,3);scene(s,12,86,216,110);use(s,'feedback.'+id,54,103);footer(s,'本轮结束，自动进入下一轮');}
{let s=screen('拔河结算/2星','胜利零失误3星；1–2次失误2星；更多失误1星；失败0星');scene(s,12,12,216,216,'拔河结束场景');use(s,'dialog.large',12,35);txt(s,'抢到了!',120,48,24,P.ink,'center');use(s,'star.full',82,86);use(s,'star.full',112,86);use(s,'star.empty',142,86);txt(s,'失误 1 次',120,117,12,P.quiet,'center');use(s,'shell',78,149);txt(s,'贝壳 +20',102,149);footer(s,'确定:再来  返回:菜单');}

current=group('03-贪吃腊肠 UI','关卡HUD置于顶部；200×200游戏区域完整保留；不制作身体、食物或障碍');
function snakeHUD(s,level=1,progress=3){art(s,'腊肠HUD',20,7,200,20,text('第'+level+'关',0,2)+text(progress+'/5',100,2,12,P.ink,'center')+text('最高11',200,2,12,P.ink,'right'));scene(s,20,30,200,200,'200×200游戏区域');}
for(let level=1;level<=3;level++){let s=screen('局内/第'+level+'关','每关5分；第二关新增障碍；第三关提速，场景由游戏实现');snakeHUD(s,level);}
{let s=screen('下一步方向/三次输入','箭头表示最终方向；两次反向输入忽略；第三次累计成左转');snakeHUD(s,1);use(s,'arrow.3',112,104);art(s,'输入预览标记',84,138,72,22,panel(72,22)+text('转向 ×3',36,4,12,P.ink,'center'));}
for(const level of [1,2]){let s=screen('换关/第'+level+'关完成','过关停顿1.5秒，进入下一关重新倒计时');snakeHUD(s,level,5);use(s,'dialog.small',24,90);txt(s,'第 '+level+' 关完成!',120,101,24,P.ink,'center');txt(s,level===1?'下一关：小心石头':'下一关：速度变快',120,145,12,P.ink,'center');}
{let s=screen('三关通关/3星','总计15根，通关后结算30贝壳');snakeHUD(s,3,5);use(s,'dialog.large',12,45);txt(s,'全部通关!',120,58,24,P.ink,'center');for(let i=0;i<3;i++)use(s,'star.full',82+i*30,95);txt(s,'吃到 15 根骨头',120,125,12,P.ink,'center');use(s,'shell',78,158);txt(s,'贝壳 +30',102,158);footer(s,'确定:再来  返回:菜单');}
{let s=screen('碰撞结算/1星','撞墙、撞身体、撞障碍使用同一面板，标题可替换');snakeHUD(s,2);use(s,'dialog.large',12,45);txt(s,'撞到石头了',120,58,24,P.ink,'center');use(s,'star.full',82,95);use(s,'star.empty',112,95);use(s,'star.empty',142,95);txt(s,'第2关 · 共吃到8根',120,125,12,P.ink,'center');use(s,'shell',78,158);txt(s,'贝壳 +10',102,158);footer(s,'确定:再来  返回:菜单');}

current=group('04-接网球 UI','保持远机近角色构图的净空；不画轨迹虚线，不画机器、角色或散落球');
function tennisHUD(s,miss=0){use(s,'hud.base',12,10);txt(s,'分数 06',20,17);for(let i=0;i<3;i++)use(s,i<3-miss?'heart.full':'heart.empty',96+i*18,18);txt(s,'最高12',221,17,12,P.ink,'right');scene(s,12,48,216,151,'透视训练场画面占位');footer(s,'确定:跳起  选择:暂停');}
for(let miss=0;miss<3;miss++){let s=screen('局内/'+(3-miss)+'颗爱心','三次失误结束，比分不扣减；不遮挡来球区域');tennisHUD(s,miss);}
for(const id of ['nice','tennis.perfect','early','late','miss']){let s=screen('球反馈/'+id,'居中提示短暂出现，反馈与玩法视觉保持分离');tennisHUD(s,id.includes('nice')||id.includes('perfect')?0:1);use(s,'feedback.'+id,54,58);}
{let s=screen('发球预备/提示气泡','气泡贴近远处发球机位置；音效对应的短提示');tennisHUD(s);art(s,'发球机气泡/UI',38,88,38,25,panel(38,22,P.white)+rect(16,22,4,3,P.edge)+text('滴!',19,3,12,P.ink,'center'));}
{let s=screen('接球结算/2星','失误3次结束；按得分4/7/10分档结算');scene(s,12,12,216,216,'训练结束场景');use(s,'dialog.large',12,35);txt(s,'训练完成',120,48,24,P.ink,'center');use(s,'star.full',82,86);use(s,'star.full',112,86);use(s,'star.empty',142,86);txt(s,'接住 8 球 · 漏接3次',120,116,12,P.quiet,'center');use(s,'shell',78,149);txt(s,'贝壳 +20',102,149);footer(s,'确定:再来  返回:菜单');}

if(glyphMissing.size)throw Error('缺字:'+Array.from(glyphMissing).join(''));
fs.writeFileSync(out+'/design-data.json',JSON.stringify({palette:P,assets:A,groups},null,2));
console.log(JSON.stringify({assets:Object.keys(A).length,screens:groups.map(g=>({name:g.name,count:g.screens.length})),font:font.names.fontFamily.en},null,2));
