#!/bin/bash
# 同时支持 git clone 与下载源码 ZIP；不依赖原电脑目录。
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PYDEPS'
from pathlib import Path
import subprocess,json
for d in json.loads(Path('handoff/firmware-dependencies.json').read_text()):
 p=Path(d['path'])
 if not p.exists() or not any(p.iterdir()):
  subprocess.run(['git','clone','--no-checkout',d['url'],str(p)],check=True)
 elif not (p/'.git').exists():
  raise SystemExit(f'{p} 已存在且不是 Git 仓库，停止以免覆盖')
 head=subprocess.run(['git','-C',str(p),'rev-parse','HEAD'],capture_output=True,text=True)
 if head.returncode or head.stdout.strip()!=d['commit']:
  if subprocess.check_output(['git','-C',str(p),'status','--porcelain'],text=True).strip():
   raise SystemExit(f'{p} 有本地修改，停止以免覆盖')
  subprocess.run(['git','-C',str(p),'checkout','--detach',d['commit']],check=True)
 if d['recursive']:
  subprocess.run(['git','-C',str(p),'submodule','update','--init','--recursive'],check=True)
 print(f'{p}: {d["commit"]}')
PYDEPS
