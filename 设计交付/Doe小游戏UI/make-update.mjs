import fs from 'node:fs';
const dir='/Users/chenningxin/小游戏制作模拟器/设计交付/Doe小游戏UI';
const data=JSON.parse(fs.readFileSync(dir+'/design-data.json','utf8'));
const parse=p=>JSON.parse(JSON.parse(fs.readFileSync(p,'utf8')).result.content.find(c=>c.type==='text').text);
const comps=parse(dir+'/01-components-result.json');
const groups=data.groups.map((g,i)=>({...g,...parse(dir+`/0${i+2}-screens-result.json`),definitions:g.screens}));
const utility=fs.readFileSync(dir+'/pixso-utility.txt','utf8');
const script=utility+`
const data=${JSON.stringify(data)}, comps=${JSON.stringify(comps)}, groups=${JSON.stringify(groups)};
// 修改的每个节点必须在当前新文档中的已知创建结果里。
for(const [key,id]of Object.entries(comps.components)){
 const c=pixso.getNodeById(id);if(!c||c.type!=='COMPONENT')throw Error('缺组件 '+key);
 const old=c.children.filter(n=>n.name==='像素图形与文字');
 for(const n of old)n.remove();
 svgNode(c,data.assets[key].svg,'像素图形与文字',0,0);
}
// 组件画板按尺寸重新整理，清除因面板移位而遗留的空洞。
const board=pixso.getNodeById(comps.boardId);
for(const n of board.children.filter(n=>n.type==='TEXT'&&data.assets[n.characters]!==undefined))n.remove();
let i=0;
for(const [key,id]of Object.entries(comps.components)){
 if(key.startsWith('dialog.'))continue;
 const a=data.assets[key],x=28+(i%4)*282,y=362+Math.floor(i/4)*98;
 const c=pixso.getNodeById(id);c.x=x;c.y=y;
 note(board,key,x,y+Math.max(a.h+6,66),10,266);i++;
}
const base=362+Math.ceil(i/4)*98+24;
for(const [key,j]of [['dialog.large',0],['dialog.small',1]]){
 const c=pixso.getNodeById(comps.components[key]);c.x=28+j*282;c.y=base;
 note(board,key,c.x,c.y+data.assets[key].h+8,10,266);
}
board.resize(1180,base+182);
for(const g of groups)for(let i=0;i<g.definitions.length;i++){
 const def=g.definitions[i],root=pixso.getNodeById(g.screens[i].id);
 if(!root)throw Error('缺状态稿');
 // 仅替换新生成的SVG层；组件实例继续保留，保持与主组件关联。
 for(const n of [...root.children])if(!n.name.startsWith('实例/'))n.remove();
 const instances=root.children.filter(n=>n.type==='INSTANCE');let index=0;
 for(const l of def.layers){
  if(l.kind==='instance'){const inst=instances[index++];if(!inst)throw Error('实例缺失');root.appendChild(inst);inst.x=l.x;inst.y=l.y;}
  else svgNode(root,l.svg,l.name,l.x,l.y);
 }
}
return {updatedComponents:Object.keys(comps.components).length,updatedScreens:groups.reduce((n,g)=>n+g.screens.length,0),file:pixso.root.name};`;
fs.writeFileSync(dir+'/qa-update-request.json',JSON.stringify({name:'eval_script',arguments:{script}}));
console.log('QA update prepared');
