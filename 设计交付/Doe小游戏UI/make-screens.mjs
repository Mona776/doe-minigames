import fs from 'node:fs';
const dir='/Users/chenningxin/小游戏制作模拟器/设计交付/Doe小游戏UI';
const data=JSON.parse(fs.readFileSync(dir+'/design-data.json','utf8'));
const receipt=JSON.parse(fs.readFileSync(dir+'/01-components-result.json','utf8'));
const map=JSON.parse(receipt.result.content.find(c=>c.type==='text').text).components;
const utility=fs.readFileSync(dir+'/pixso-utility.txt','utf8');
for(let i=0;i<data.groups.length;i++){
 const g=data.groups[i];
 const script=utility+`
 const group=${JSON.stringify(g)}, ids=${JSON.stringify(map)};
 if(pixso.root.children.some(p=>p.name===group.name))throw Error('该组已存在，禁止重复创建');
 const page=pixso.createPage();page.name=group.name;
 const cols=4,rows=Math.ceil(group.screens.length/cols);
 const board=frame(page,group.name+' / 全状态概览',0,0,1180,176+rows*318,'#EDF4F1');
 note(board,'Doe 小游戏 UI / '+group.name,28,24,28);
 note(board,group.description,28,68,14);
 note(board,'所有小画板为240×240。浅色虚线区域只是游戏画面占位；设计内容仅为UI覆盖层。',28,96,14);
 const screens=[];
 for(let i=0;i<group.screens.length;i++){
  const s=group.screens[i],x=28+(i%cols)*282,y=176+Math.floor(i/cols)*318;
  note(board,s.name,x,y-30,14,250);
  const root=frame(board,s.name+' · 240×240',x,y,240,240,'#FFFFFF');root.clipsContent=true;
  for(const l of s.layers){
   if(l.kind==='instance'){
    const master=pixso.getNodeById(ids[l.key]);if(!master||master.type!=='COMPONENT')throw Error('缺组件 '+l.key);
    const n=master.createInstance();root.appendChild(n);n.x=l.x;n.y=l.y;n.name='实例/'+l.key;
   }else svgNode(root,l.svg,l.name,l.x,l.y);
  }
  note(board,s.description,x,y+250,11,250,'#668D87');
  screens.push({id:root.id,name:s.name});
 }
 return {pageId:page.id,boardId:board.id,screens};`;
 fs.writeFileSync(dir+`/0${i+2}-screens-request.json`,JSON.stringify({name:'eval_script',arguments:{script}}));
}
console.log('Four screen groups prepared');
