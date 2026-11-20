"""Visible/muted owned Emu80: NC MAIN tail, CLI handoff and return."""
from pathlib import Path
import argparse,runpy
ROOT=Path(__file__).resolve().parents[2]

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--snapshot',type=Path,required=True)
 p.add_argument('--runtime',type=Path,default=ROOT/'.local/rom-pio-19782')
 p.add_argument('--port',type=int,default=19782,choices=range(19781,19785))
 args=p.parse_args();snap=args.snapshot.resolve();out=ROOT/'.local/main-buffer-live';out.mkdir(exist_ok=True)
 h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
 g=h['Matrix'].__init__.__globals__
 g.update(RUNTIME=args.runtime.resolve(),OUT=out,mcp_owner=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0'))
 x=h['Matrix']();x.m.url=f'http://127.0.0.1:{args.port}/mcp';m=x.m
 module=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-profiles.py'))
 module['symbols'].__globals__.update(SNAP=snap,OUT=out)
 c=module['Campaign'](m)
 def allocations():
  raw=c.read(1,c.k['MEM_ALLOC_TABLE'],c.k['MEM_ALLOC_RECORDS']*6)
  return [dict(bank=raw[i],base=int.from_bytes(raw[i+1:i+3],'little'),size=int.from_bytes(raw[i+3:i+5],'little'),owner=raw[i+5]) for i in range(0,len(raw),6) if raw[i]!=255]
 def nc_tail():
  rows=allocations();owner=c.read(1,c.k['appOwner'],1)[0]
  end=int.from_bytes(c.read(0,c.n['ncMemoryEnd'],2),'little')
  image=next(q for q in rows if q['bank']==0 and q['base']==0 and q['owner']==owner)
  tail=next(q for q in rows if q['bank']==0 and q['base']==c.n['FILE_CACHE_A'])
  assert image['size']==c.n['FILE_CACHE_A'],image
  assert tail['size']==end-c.n['FILE_CACHE_A'] and tail['owner']==owner,(tail,end,owner)
  print('NC image/BSS and allocated catalogs:',image,tail,flush=True)
 try:
  x.start('main-buffers',system=snap/'A.bin',disk=ROOT/'docs/nc/application-return-2026-11-17/build/H128.bin',nc_binary=snap/'bin/NC.i80',nc_symbols=snap/'bin/NC.sym',bios=snap/'bin/BIOS.sys',dos=snap/'bin/DOS.sys')
  nc_tail()
  cli=module['symbols']('CLI')
  c.type('A:CLI')
  m.call('bp_clear');m.call('bp_set',addr=cli['cliPrompt'],type='exec');c.tap('enter')
  c.wait(cli['cliPrompt'],0)
  assert not any(q['bank']==0 and q['base']==c.n['FILE_CACHE_A'] for q in allocations()),'old NC catalogs survived replacement'
  c.free()
  for key in 'exit':c.tap(key)
  c.tap('enter');c.wait(c.b['getch2'],0)
  assert c.read(0,0,12)==(snap/'bin/NC.i80').read_bytes()[:12]
  nc_tail();m.screenshot(out/'returned.png')
  c.launch(True)
  assert not any(q['bank']==0 and q['base']==module['symbols']('NC')['FILE_CACHE_A'] for q in allocations()),'MAIN catalogs survived BANK loader'
  print('MAIN buffers live PASS: boot, CLI replacement/return, BANK resident launch',flush=True)
 finally:x.stop()

if __name__=='__main__':main()
