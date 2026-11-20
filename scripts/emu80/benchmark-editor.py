"""Owned visible/muted Emu80: NC keyboard launches, Elite and CPU-cycle UI timings."""
from pathlib import Path
import argparse,hashlib,json,re,runpy,struct

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--variant',required=True)
p.add_argument('--binary-dir',type=Path,default=R/'bin')
p.add_argument('--editor-base',default='EDIT',help='E selects the historical before fixture')
p.add_argument('--shell-dir',type=Path,default=R/'bin',help='Symbols/images matching the shell in the selected test image')
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19785),default=19782)
p.add_argument('--report',type=Path,default=R/'docs/editor/performance-2026-11-17')
p.add_argument('--system-image',type=Path,default=R/'images/SystemRom_emu80_writable.bin')
p.add_argument('--font',choices=['Rotate','Packed6K','Fast12K'])
a=p.parse_args();O=a.report.resolve()/('emu-'+a.variant);O.mkdir(parents=True,exist_ok=True)
sy=lambda path:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(),re.M)}
s=sy(a.binary_dir/(a.editor_base+'.sym'));n=sy(a.shell_dir/'NC.sym');k=sy(a.shell_dir/'DOS.sym')
c=sy(a.shell_dir/'CLI.sym')
editor_image=a.editor_base+'.i80'
images={name:(a.binary_dir/name if name==editor_image else a.shell_dir/name).read_bytes() for name in [editor_image,'NC.i80','CLI.i80']}
elite=(R/'assets/games/Elite.txt').read_bytes();short=elite[:min(16000,0x6400-s['EDITOR_TEXT_BEGIN']-129)].rsplit(b'\r',1)[0]+b'\r'
tree=runpy.run_path(str(R/'scripts/tests/check-nested-image.py'))['read_tree'](a.system_image.read_bytes())[0]
assert tree[a.editor_base+'.COM'][s['IMAGE_HEADER_SIZE']:]==images[editor_image],'stale editor in image'
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=a.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
W=lambda v:struct.pack('<H',v)
shell='NC'
def shell_pc():return c['cliPrompt'] if shell=='CLI' else n['mainLoop']
def val(name):return int.from_bytes(m.read(s[name],2),'little')
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def tap(key):
 result=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not result.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
 return result
def command(text,target,nc=False):
 ready();tap('esc');m.call('emu_run_for',ms=250)
 for ch in text:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(ch,ch.lower()))
 cmd_address=c['CMD_LINE'] if shell=='CLI' else n['vars.cmdLine']
 assert m.read(cmd_address,len(text)).upper()==text.upper().encode()
 m.screenshot(O/'nc-command.png');m.stop_at(target);tap('enter')
 image='CLI.i80' if target==c['cliPrompt'] else 'NC.i80' if target==n['mainLoop'] else editor_image
 signature=s['keyInput'] if image==editor_image else 0;expected=images[image][signature:signature+3]
 for _ in range(120):
  state=m.call('cpu_state')
  if state['registers']['pc']['dec']==target and m.read(signature,3)==expected:break
  if state['registers']['pc']['dec']==target:m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=500)
 else:raise AssertionError(('command timeout',text,state))
 ready()
def edit(key,target=None):
 target=target or s['editorStatusLoop'];m.stop_at(target);tap(key);m.until(target,90,page=0);ready()
rows=[]
def address(entry):
 if entry in s:return s[entry]
 return s[{'editorPageUp':'keyHandler_F2','editorPageDown':'keyHandler_F3','editorUndoLine':'keyHandler_F8'}[entry]]

def measure(font,key,entry,modifier=None):
 ready();at=address(entry);m.stop_at(at)
 assert m.read(at,16)==images[editor_image][at:at+16],(entry,'wrong loaded code')
 if modifier:m.call('kbd_send',keys=[modifier],action='press')
 result=tap(key)
 if modifier:m.call('kbd_send',keys=[modifier],action='release')
 state=m.until(at,10,page=0);start=state['cpu_ticks']
 m.stop_at(s['keyInput']);m.call('emu_run_for',ms=1000)
 state=m.until(s['keyInput'],90,page=0);cycles=state['cpu_ticks']-start
 row=dict(font=font,operation=entry,cycles=cycles,ms=round(cycles/2000,3),line=val('v_lineNumber'),column=val('v_column'))
 rows.append(row);print(row,flush=True);ready()
def doc():
 if 'v_gapLo' not in s:return m.read(val('v_textBeginLo'),val('v_textEnd')-val('v_textBeginLo'))
 lo=val('v_gapLo');hi=val('v_gapHi');end=val('v_textEnd');begin=val('v_textBeginLo')
 return m.read(begin,lo-begin)+m.read(hi,end-lo)

try:
 x.start('editor-perf',system=a.system_image.resolve(),nc_binary=a.shell_dir/'NC.i80',nc_symbols=a.shell_dir/'NC.sym',dos=a.shell_dir/'DOS.sys')
 api=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
 call=lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a)
 m.write(0x3400,b'B:/ELITE16.TXT\0');m.write(0x4000,short)
 m.write(0x3200,W(0x3400)+W(0x4000)+W(len(short))+bytes(34))
 api['path_io'](m,call,k,0x3200,True)
 # The isolated ABI fixture bypasses NC's input epilogue. Remove its
 # temporary idle callback before replacing/restarting MAIN shell code.
 call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
 m.stop_at(n['mainLoop']);m.call('regs_set',pc=s['bios_reboot'],sp=s['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
 for font in ([a.font] if a.font else ['Rotate','Packed6K','Fast12K']):
  if font=='Fast12K':
   # The current large MAIN NC exceeds Fast12K's memtop. CLI is itself a
   # normal keyboard-launched MAIN shell and fits under all three engines.
   command('A:CLI',c['cliPrompt']);shell='CLI'
  command('A:FONT 6x8/'+{'Packed6K':'Packed','Fast12K':'Fast'}.get(font,font),shell_pc(),True)
  if font!='Rotate':
   # Document the actual refusal, rather than overrun reserved font tables.
   command('A:'+a.editor_base+' H:/ELITE.TXT',s.get('editorStartupErrorWait',s['editorErrorWait']))
   rows.append(dict(font=font,operation='full_file_limit',fileBytes=len(elite),textBegin=val('v_textBeginLo'),limit=val('v_textLimitLo'),capacity=val('v_textLimitLo')-val('v_textBeginLo')-1))
   m.screenshot(O/(font+'-full-file-limit.png'))
   m.stop_at(shell_pc());tap('enter');m.until(shell_pc(),90,page=0);ready()
  payload=elite if font=='Rotate' else short
  command('A:'+a.editor_base+' '+('H:/ELITE.TXT' if font=='Rotate' else 'B:/ELITE16.TXT'),s['editorStatusLoop'])
  assert doc()==payload,(font,'file content')
  assert int.from_bytes(m.read(s['bios_printStringRawLim']+1,2),'little')>=0xd800
  m.screenshot(O/(font+'-loaded.png'))
  modern='editorMainKeys' in s
  measure(font,'down' if modern else 'f3','editorPageDown',modifier='rpt' if modern else None);m.screenshot(O/(font+'-pagedown.png'))
  measure(font,'up' if modern else 'f2','editorPageUp',modifier='rpt' if modern else None)
  assert val('v_column')==0 and val('v_lineNumber')==1
  measure(font,'x','keyHandler_Space');assert doc()[1:]==payload
  if modern:
   ready();m.call('kbd_send',keys=['rpt'],action='press');edit('z');m.call('kbd_send',keys=['rpt'],action='release')
  else:edit('f8')
  assert doc()==payload
  measure(font,'backspace','keyHandler_Del',modifier='shift');assert doc()==payload[1:]
  if modern:
   ready();m.call('kbd_send',keys=['rpt'],action='press');edit('z');m.call('kbd_send',keys=['rpt'],action='release')
  else:edit('f8')
  assert doc()==payload
  m.stop_at(s['editorSaveWait']);tap('clear');m.until(s['editorSaveWait'],90,page=0);ready()
  m.stop_at(shell_pc());tap('d');m.until(shell_pc(),90,page=0);ready()
 report=dict(cpu='Emu80 MX2 Intel 8080, 2 MHz',variant=a.variant,editorSha256=hashlib.sha256(images[editor_image]).hexdigest(),eliteSha256=hashlib.sha256(elite).hexdigest(),results=rows,
             scope='CPU ticks from key handler entry to keyInput: document work, rendering, cursor positioning and status fields; excludes keyboard scan/wait, file load and NC startup. Full file on Rotate; Packed/Fast: identical complete-line prefix from Elite on test RAM disk, limited by current Fast12K capacity.')
 (O/'results.json').write_text(json.dumps(report,indent=2)+'\n')
 print('EDITOR EMULATOR BENCH PASS',flush=True)
except Exception:
 if x.pid:
  m.call('debug_break');m.screenshot(O/'failure.png');print('FAIL',m.call('cpu_state'),flush=True)
 raise
finally:x.stop()
