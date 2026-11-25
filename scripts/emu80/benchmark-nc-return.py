"""Normal keyboard application return, with font/GFX restoration separated."""
from pathlib import Path
import argparse,hashlib,json,os,runpy,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/nc/application-return-2026-11-17'
SNAP=OUT/'build'
RUNTIME=ROOT/'.local/rom-pio-19782'
W=lambda n:int(n).to_bytes(2,'little')
def prepare(h):
 OUT.mkdir(parents=True,exist_ok=True);SNAP.mkdir(exist_ok=True)
 h['command'](['powershell.exe','-NoProfile','-File','scripts/build/build-project.ps1','OS'])
 shutil.copytree(ROOT/'bin',SNAP/'bin',dirs_exist_ok=True,ignore=shutil.ignore_patterns('*.lst','*.log','__pycache__'))
 executable=runpy.run_path(str(ROOT/'scripts/images/executable.py'))
 fixtures={}
 for name,bank,end,policy in [('TINY',False,0,'MANAGED'),('BIG',False,0x6000,'MANAGED'),('ALL',False,0x8c00,'EXCLUSIVE'),('BNK',True,0,'MANAGED')]:
  folder=OUT/'fixtures';folder.mkdir(exist_ok=True)
  origin=0x100 if bank else 0
  source=f'''    INCLUDE "{(ROOT/'source/include/mxos.inc').as_posix()}"
    ORG {origin}
    jp start
    DB 'RETURNBENCH-{name}',0
start:
    ld hl,message
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
ready:
    MXOS_CALL_REGS MXOS_BIOS_GETCH
finish:
    xor a
    ret
message: DB 'Return benchmark: press Enter',13,10,0
    END
'''
  (folder/f'{name}.asm').write_text(source,encoding='utf-8')
  subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={folder/name}.i80',f'--sym={folder/name}.sym',str(folder/f'{name}.asm')],check=True,capture_output=True)
  body=(folder/f'{name}.i80').read_bytes();end=end or origin+len(body)
  image=executable['Image'](body,origin,origin,2 if bank else 4,bss=end-origin-len(body),memory_begin=origin,memory_end=end,policy=0 if policy=='EXCLUSIVE' else 2,api=2)
  (folder/f'{name}.COM').write_bytes(executable['encode'](image,'com'))
  fixtures[name]=dict(origin=origin,end=end,bank=bank)
 manifest=(ROOT/'assets/C-Flash-512k-MMX-System.txt').read_text(encoding='utf-8-sig')
 manifest=manifest.replace('../bin/',os.path.relpath(SNAP/'bin',ROOT/'assets').replace(chr(92),'/')+'/')
 for name in fixtures:manifest+=f"\n{name}.COM {os.path.relpath(OUT/'fixtures'/f'{name}.COM',ROOT/'assets').replace(chr(92),'/')}\n"
 (OUT/'A-test.txt').write_text(manifest,encoding='utf-8')
 script=(ROOT/'scripts/images/build-image.ps1').read_text(encoding='utf-8-sig')
 ps=h['ps']
 script=script.replace('$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)','$repositoryRoot = '+ps(ROOT))
 script=script.replace('$imageDirectory = Join-Path $repositoryRoot "images"','$imageDirectory = '+ps(SNAP))
 script=script.replace(". (Join-Path $PSScriptRoot 'executable.ps1')",'. '+ps(ROOT/'scripts/images/executable.ps1'))
 script=script.replace('Join-Path $assetDirectory "C-Flash-512k-MMX-System.txt"',ps(OUT/'A-test.txt'))
 script=script.replace("Join-Path $PSScriptRoot 'nest-fat-image.py'",ps(ROOT/'scripts/images/nest-fat-image.py'))
 (OUT/'build-image.ps1').write_text(script,encoding='utf-8-sig')
 h['command'](['powershell.exe','-NoProfile','-File',OUT/'build-image.ps1','SystemEmuWritable'])
 shutil.copyfile(SNAP/'C-Flash-512k-MMX-System-Emu80.bin',SNAP/'A.bin')
 image=bytearray((ROOT/'images/H-Flash-2M-Games.bin').read_bytes())
 word=lambda at:int.from_bytes(image[at:at+2],'little')
 root=(word(14)+image[16]*word(22))*word(11);kept=[]
 for at in range(root,root+word(17)*32,32):
  row=image[at:at+32]
  if row[0] not in (0,229) and not row[11]&8:kept.append(bytes(row))
 label=bytearray(32);label[:11]=image[43:54];label[11]=8
 image[root:root+word(17)*32]=(bytes(label)+b''.join(kept[:32])).ljust(word(17)*32,b'\0')
 (SNAP/'H128.bin').write_bytes(image)
 (OUT/'fixtures.json').write_text(json.dumps(fixtures,indent=2),encoding='utf-8')

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--prepare',action='store_true');parser.add_argument('--prepare-only',action='store_true');parser.add_argument('--resume',action='store_true');args=parser.parse_args()
 h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
 if args.prepare or args.prepare_only or not (SNAP/'A.bin').exists():prepare(h)
 if args.prepare_only:return
 module=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-profiles.py'))
 g=module['symbols'].__globals__;g.update(SNAP=SNAP,OUT=OUT)
 Campaign=module['Campaign'];sy=module['symbols']
 module['verify_snapshot']()
 h['Matrix'].__init__.__globals__.update(RUNTIME=RUNTIME,OUT=OUT,mcp_owner=lambda:h['shell']("$listener=Get-NetTCPConnection -State Listen -LocalPort 19782 -ErrorAction SilentlyContinue; if ($listener) { $listener[0].OwningProcess }; exit 0"))
 x=h['Matrix']();m=h['Bench']('http://127.0.0.1:19782/mcp');x.m=m
 fixtures=json.loads((OUT/'fixtures.json').read_text())
 report={'cpu_hz':2000000,'rows':[],'hardware_measured':False}
 if args.resume and (OUT/'results.json').exists():report=json.loads((OUT/'results.json').read_text(encoding='utf-8'))
 report['artifacts']={n:hashlib.sha256((SNAP/n).read_bytes()).hexdigest() for n in ['A.bin','bin/BIOS.sys','bin/DOS.sys','bin/NC.i80','bin/NC_CORE.i80']}
 def save():(OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 try:
  for font in ['Rotate','Packed6K','Fast12K']:
   for bank in [False,True]:
    if font=='Fast12K' and not bank:continue
    if all(any(q['profile']==('BANK' if bank else 'MAIN') and q['font']==font and q['application']==name for q in report['rows']) for name in ['TINY','BIG','ALL','BNK','EDIT']):continue
    x.start(('BANK-' if bank else 'MAIN-')+font,system=SNAP/'A.bin',disk=SNAP/'H128.bin',nc_binary=SNAP/'bin/NC.i80',nc_symbols=SNAP/'bin/NC.sym',bios=SNAP/'bin/BIOS.sys',dos=SNAP/'bin/DOS.sys')
    c=Campaign(m)
    if font=='Fast12K':
     c.launch(bank);c.command('FONT 6x8/'+{'Packed6K':'Packed','Fast12K':'Fast'}.get(font,font))
    else:
     c.command('FONT 6x8/'+{'Packed6K':'Packed','Fast12K':'Fast'}.get(font,font));c.launch(bank)
    for name in ['TINY','BIG','ALL','BNK','EDIT']:
     if any(q['profile']==('BANK' if bank else 'MAIN') and q['font']==font and q['application']==name for q in report['rows']):continue
     meta=fixtures.get(name,dict(origin=0,bank=False))
     app=sy('EDIT') if name=='EDIT' else {k:int(v,16) for k,v in __import__('re').findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(OUT/'fixtures'/f'{name}.sym').read_text(),__import__('re').M)}
     c.type('A:'+name)
     ready=app['editorStatusLoop'] if name=='EDIT' else app['ready']
     m.call('bp_clear');m.call('bp_set',addr=ready,type='exec');c.tap('enter')
     prefix=(SNAP/'bin/EDIT.i80').read_bytes()[app['keyInput']:app['keyInput']+3] if name=='EDIT' else (OUT/'fixtures'/f'{name}.i80').read_bytes()[:12]
     for _ in range(60):
      state=c.state();app_page=c.page_now()
      if state['registers']['pc']['dec']==ready:
       if (not meta['bank'] and app_page==0 or meta['bank'] and app_page not in [0,1,c.page]) and c.read(app_page,app['keyInput'] if name=='EDIT' else meta['origin'],len(prefix))==prefix:break
       m.call('debug_step',mode='into')
      else:m.call('emu_run_for',ms=1000)
     else:raise RuntimeError(('application not reached',font,bank,name,c.state()))
     m.call('bp_clear');m.call('emu_run_for',ms=250)
     finish=app['editorExitNow'] if name=='EDIT' else app['finish']
     m.call('bp_clear');m.call('bp_set',addr=finish,type='exec');c.tap('clear' if name=='EDIT' else 'enter')
     start=c.wait(finish,app_page)['cpu_ticks']
     pending=c.read(1,c.k['fontConfigPending'],1)[0]
     before=c.wait(c.b['biosRestoreFont'],0)['cpu_ticks']
     return_at=int.from_bytes(m.read(c.state()['registers']['sp']['dec'],2),'little')
     after=c.wait(return_at,0)['cpu_ticks']
     gfx_ticks=0
     if bank:
      keys=c.read(1,c.k['residentKeys'],16);slot=next(i for i in range(8) if keys[2*i:2*i+2]==W(0x434e))
      # If GFX handle has disappeared, count its reload separately.
      handle=c.read(c.page,c.n['ncGfxHandle'],2)
      records=c.read(1,c.k['MODULE_TABLE'],c.k['MODULE_RECORD_SIZE']*8)
      live=any(records[i:i+2]==handle for i in range(0,len(records),c.k['MODULE_RECORD_SIZE']))
      if not live:
       a=c.wait(c.n['ncBankLoadGraphics'],c.page)['cpu_ticks']
       b=c.wait(c.n['ncBankGraphicsReady'],c.page)['cpu_ticks'];gfx_ticks=b-a
     done=c.wait(c.b['getch2'],0)['cpu_ticks']
     expected={'Rotate':0,'Packed6K':1,'Fast12K':2}[font]
     assert c.read(0,c.b['FONT_ENGINE_HEADER']+7,1)==bytes([expected]),(font,name,'font not restored')
     row=dict(profile='BANK' if bank else 'MAIN',font=font,application=name,total_ticks=done-start,exit_to_font_ticks=before-start,font_ticks=after-before,font_pending=pending,gfx_load_ticks=gfx_ticks,after_font_ticks=done-after)
     report['rows'].append(row);save();print(row,flush=True)
 finally:x.stop()
if __name__=='__main__':main()
