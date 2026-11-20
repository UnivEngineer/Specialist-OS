"""Visible, muted NC->E keyboard integration on an owned MCP runtime."""
from pathlib import Path
import argparse, runpy, re, struct

R=Path(__file__).resolve().parents[2]
O=R/'.local/editor-ui-check';O.mkdir(exist_ok=True)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19785),default=19782)
p.add_argument('--scroll-only',action='store_true',help='Check scroll/header/banner scenarios only')
p.add_argument('--modern-only',action='store_true',help='Check editing keys and unsaved-exit modal')
p.add_argument('--tabs-only',action='store_true',help='Literal Tab, visual movement and disk round trip')
p.add_argument('--line-undo-only',action='store_true',help='Retained line snapshot and F8 return after navigation/dialogs/save')
p.add_argument('--tab-width-only',action='store_true',help='Tab width in Help, menu, idle language and incremental edits')
p.add_argument('--keys-only',action='store_true',help='Latched toolbars, actual RUS/LAT keys, mark, find and file actions')
args=p.parse_args()
sy=lambda name:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
s=sy('EDIT.sym');n=sy('NC.sym');k=sy('DOS.sym');fs=sy('FONT.sym')
images={name:(R/'bin'/name).read_bytes() for name in ('NC.i80','EDIT.i80','FONT.i80')}
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=args.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{args.port}/mcp'
w=lambda v:struct.pack('<H',v)
def val(key):return int.from_bytes(m.read(s[key],2),'little')
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def tap(key):
 result=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not result.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
def text(t):
 for ch in t:
  if ch=='_':
   m.call('kbd_send',keys=['shift'],action='press')
   try:tap('0')
   finally:m.call('kbd_send',keys=['shift'],action='release')
  else:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(ch,ch.lower()))
def command(t,target,font_program=False):
 ready();tap('esc');m.call('emu_run_for',ms=250);text(t)
 m.screenshot(O/'nc-command.png')
 assert m.read(n['vars.cmdLine'],len(t)).upper()==t.upper().encode(),(t,m.read(n['vars.cmdLine'],len(t)))
 first=s['editorLoadingDone'] if target==s['editorStatusLoop'] and args.scroll_only else target
 m.stop_at(first);tap('enter')
 signature=0 if target==n['mainLoop'] or font_program else s['keyInput']
 expected=images['FONT.i80' if font_program else 'NC.i80' if target==n['mainLoop'] else 'EDIT.i80'][signature:signature+3]
 for _ in range(90):
  state=m.call('cpu_state')
  if state['registers']['pc']['dec']==first and m.read(signature,3)==expected:break
  if state['registers']['pc']['dec']==first:m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=500)
 else:raise AssertionError(('program did not arrive',t,state))
 if first!=target:
  m.screenshot(O/'loading-copyright.png')
  m.stop_at(target);m.until(target,90,page=0)
 if not font_program:ready()
def wait_editor():
 m.stop_at(s['editorStatusLoop']);tap('esc');tap('esc');m.until(s['editorStatusLoop'],60,page=0);ready()
def doc():
 lo=val('v_gapLo');hi=val('v_gapHi');end=val('v_textEnd');begin=val('v_textBeginLo')
 return m.read(begin,lo-begin)+m.read(hi,end-lo)

def edit(key,target=None,shift=False):
 target=target or s['editorStatusLoop']
 m.stop_at(target)
 if shift:m.call('kbd_send',keys=['shift'],action='press')
 tap(key)
 m.until(target,60,page=0)
 if shift:m.call('kbd_send',keys=['shift'],action='release')
 m.call('bp_clear')
 m.call('emu_run_for',ms=500)

def chord(modifier,key):
 target=s['editorStatusLoop'];m.stop_at(target)
 m.call('kbd_send',keys=[modifier],action='press')
 tap(key)
 m.call('kbd_send',keys=[modifier],action='release')
 m.until(target,60,page=0);ready()

def action(name,target=None):
 key={'mode':'f1','tab':'f1','help':'f2','load':'f4','save':'f5','add':'f6'}[name]
 if name=='load' and m.read(s['v_documentDirty'],1)[0]:
  edit(key,s['editorSaveWait'],shift=True);edit('d',target);return
 edit(key,target,shift=name not in ('mode','help'))

def replace_input(value):
 for _ in range(m.read(s['lineEditLength'],1)[0]):tap('backspace')
 text(value)

try:
 x.start('editor')
 # Fixture creation uses the public file API; then reload NC before UI input.
 api=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
 def call(op,hl=0,a=0):return api['main_call'](m,k,op,hl,a)
 payload=(b'0123456789'*40+b'\r\nSECOND\nTHIRD\r'+b'\n'.join(f'Line {i:02d}'.encode() for i in range(4,31)))
 if args.keys_only:payload=b'Alpha beta\rSecond line\rThird beta\r'+b'\r'.join(f'Line {i:02d}'.encode() for i in range(4,36))+b'\rTab\tindent\r'
 if args.tab_width_only:payload=b'AB\tC\r1234567\r\t\tQ\r'+b'A'*255+b'\tB\tTAIL\r'+b'  text\tmore\r'*25
 if args.tabs_only:payload=b'A\tB\r123456\r\tQ\r'+b'X'*60+b'\tY\r'+b'Tab\tline\r'*25
 m.write(0x3400,b'B:/EDITTEST.TXT\0');m.write(0x4000,payload)
 m.write(0x3200,w(0x3400)+w(0x4000)+w(len(payload))+bytes(34))
 api['path_io'](m,call,k,0x3200,True)
 call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
 m.stop_at(n['mainLoop']);m.call('regs_set',pc=s['bios_reboot'],sp=s['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
 for font,width in [('Rotate',6),('Fast8',8)]:
  command('A:FONT '+{'Rotate':'6x8/Rotate','Cpm4':'4x8/Fast','Fast8':'8x8/Rotate'}[font],0,font_program=True)
  m.stop_at(n['mainLoop'])
  for label in ['error','usage']:m.call('bp_set',addr=fs[label],type='exec')
  for _ in range(90):
   m.call('emu_run_for',ms=500)
   state=m.call('cpu_state')
   if state['registers']['pc']['dec'] in [fs['error'],fs['usage']] and m.read(0,3)==images['FONT.i80'][:3]:
    raise AssertionError(('FONT stopped',font,state,m.read(fs['path'],80)))
   if state['registers']['pc']['dec']==n['mainLoop'] and m.read(0,3)==images['NC.i80'][:3]:break
   if state['registers']['pc']['dec']==n['mainLoop']:m.call('debug_step',mode='into')
  else:raise AssertionError(('FONT did not return to NC',font,state))
  ready()
  assert m.read(s['FONT_ENGINE_HEADER']+5,1)==bytes([width])
  command('A:EDIT_MN B:/EDITTEST.TXT',s['editorStatusLoop'])
  expected=payload
  assert doc()==expected
  m.screenshot(O/(font+'-loaded.png'))
  if args.keys_only:
   assert m.read(s['editorShiftState'],1)==b'\0'
   edit('f1');assert m.read(s['v_insertMode'],1)==b'\0'
   m.screenshot(O/(font+'-main-keys-ovr.png'))
   edit('f1');assert m.read(s['v_insertMode'],1)==b'\xff'
   chord('rpt','right');assert val('v_column')==6
   chord('rpt','left');assert val('v_column')==0
   edit('lf');chord('rpt','right');assert val('v_lineNumber')==2 and val('v_column')==0
   chord('rpt','left');assert val('v_lineNumber')==1 and val('v_column')==6
   edit('home')
   chord('rpt','down');assert val('v_lineNumber')==22
   chord('rpt','up');assert val('v_lineNumber')==1
   edit('f4');assert m.read(s['editorMarkActive'],1)==b'\1'
   edit('down');edit('down');edit('f4');assert m.read(s['editorMarkActive'],1)==b'\0'
   assert m.read(s['editorToolbarMarkLabel'],4)==b'Unmk'
   edit('down');edit('down');edit('f5')
   rows=expected.split(b'\r');offset=sum(len(row)+1 for row in rows[:4]);block=b'\r'.join(rows[:2])+b'\r'
   copied=expected[:offset]+block+expected[offset:];assert doc()==copied
   edit('f4');assert val('v_selectionBegin')==0
   edit('f8');assert doc()!=copied
   chord('rpt','z');assert doc()==copied
   edit('f4',s['editorSaveWait'],shift=True);edit('esc');assert doc()==copied
   edit('f4',s['editorSaveWait'],shift=True);edit('d',s['lineEditLoop']);edit('enter');assert doc()==expected
   edit('f2',s['uiHelpRead']);m.screenshot(O/(font+'-keyboard-help.png'))
   assert m.read(s['bios_upper.cursorCfg'],1)==b'\0'
   edit('t',s['uiHelpRead']);assert m.read(s['v_tabWidth'],1)==b'\4'
   edit('esc');assert m.read(s['editorLastTab'],1)==b'\4'
   action('tab');assert m.read(s['v_tabWidth'],1)==b'\x06'
   edit('f3',s['lineEditLoop']);replace_input('beta');edit('enter')
   assert val('v_lineNumber')==1 and val('v_column')==6
   edit('f7');assert val('v_lineNumber')==3 and val('v_column')==6
   edit('f7');assert val('v_lineNumber')==1 and val('v_column')==6
   edit('f3',s['lineEditLoop']);replace_input('not-found');edit('enter',s['editorInfoWait']);m.screenshot(O/(font+'-find-information.png'));edit('enter')
   edit('home')
   # Real KOI8 keys: Shift changes case; LANG alone changes alphabet.
   ready();locks=m.read(s['bios_vars.keyLocks'],1)[0];old=locks&1
   def typed_letter(language, uppercase):
    char='а' if language else 'a'
    return (char.upper() if uppercase else char).encode('koi8-r')[0]
   m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=500)
   assert m.read(s['editorLastLanguage'],1)[0]==old
   edit('a');assert doc()[0]==typed_letter(old,bool(locks&128))
   m.screenshot(O/(font+'-shift-case.png'))
   m.call('kbd_send',keys=['shift'],action='release');m.call('emu_run_for',ms=500)
   modified=doc();edit('f9',s['hexWait'],shift=True);edit('f9',shift=True);assert doc()==modified
   chord('rpt','z');assert doc()==expected
   ready();tap('caps');m.call('emu_run_for',ms=500)
   assert m.read(s['bios_vars.keyLocks'],1)[0]==locks^1
   assert m.read(s['editorLastLanguage'],1)[0]==(old^1)
   edit('a');assert doc()[0]==typed_letter(old^1,not locks&128)
   chord('rpt','z');assert doc()==expected
   ready();m.call('kbd_send',keys=['shift'],action='press');tap('caps')
   m.call('kbd_send',keys=['shift'],action='release');m.call('emu_run_for',ms=500)
   assert m.read(s['bios_vars.keyLocks'],1)[0]==locks^0x81
   assert m.read(s['editorLastLanguage'],1)[0]==old^1
   edit('a');assert doc()[0]==typed_letter(old^1,bool(locks&128))
   chord('rpt','z');assert doc()==expected
   ready();m.call('kbd_send',keys=['shift'],action='press');tap('caps')
   m.call('kbd_send',keys=['shift'],action='release');m.call('emu_run_for',ms=500)
   tap('caps');m.call('emu_run_for',ms=500)
   assert m.read(s['bios_vars.keyLocks'],1)[0]==locks
   assert m.read(s['editorLastLanguage'],1)[0]==old
   edit('f3',s['editorSaveWait'],shift=True);edit('esc');assert doc()==expected
   edit('f3',s['editorSaveWait'],shift=True);edit('d');assert doc()==b'' and not m.read(s['v_documentDirty'],1)[0]
   edit('f4',s['lineEditLoop'],shift=True);replace_input('B:/EDITTEST.TXT');edit('enter');assert doc()==expected
   edit('f5',s['lineEditLoop'],shift=True);edit('enter')
   m.stop_at(n['mainLoop']);tap('f10');m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'Shift toolbars, mark/copy/delete, modal find/help, real language, New/Load/Save/F10',flush=True)
   continue
  if args.tab_width_only:
   def visual():return val('v_visualColumn')+(m.read(s['v_visualColumnHi'],1)[0]<<16)
   assert m.read(s['v_tabWidth'],1)==b'\4'
   assert m.read(s['editorLastLanguage'],1)[0]==(m.read(s['bios_vars.keyLocks'],1)[0]&1)
   edit('right');edit('right');edit('right');assert visual()==4
   for tab in [6,8,2,4]:
    action('tab');assert m.read(s['v_tabWidth'],1)==bytes([tab])
    assert m.read(s['editorLastTab'],1)==bytes([tab])
    assert visual()==(2//tab+1)*tab and doc()==expected
   action('mode');assert m.read(s['v_insertMode'],1)==b'\0'
   action('mode');assert m.read(s['v_insertMode'],1)==b'\xff'
   m.screenshot(O/(font+'-menu-tab-width.png'))
   # Read key names from this runtime's own tool schema. If the schema has
   # no key enum, exercise the polling callback with a key-lock fixture.
   import json, urllib.request
   req=urllib.request.Request(m.url,json.dumps({'jsonrpc':'2.0','id':900,'method':'tools/list','params':{}}).encode(),{'Content-Type':'application/json','Accept':'application/json, text/event-stream'})
   ts=json.load(urllib.request.urlopen(req,timeout=30))['result']['tools']
   schema=next(t for t in ts if t['name']=='kbd_send')
   (O/'keyboard-schema.json').write_text(json.dumps(schema,indent=2))
   names=schema.get('inputSchema',{}).get('properties',{}).get('keys',{}).get('items',{}).get('enum',[])
   lang=next((key for key in names if key.lower() in ['rus','ruslat','lang','rc']),None)
   old=m.read(s['bios_vars.keyLocks'],1)[0]
   ready()
   if lang:tap(lang)
   else:m.write(s['bios_vars.keyLocks'],bytes([old^1]))
   m.call('emu_run_for',ms=500)
   assert m.read(s['editorLastLanguage'],1)[0]==((old^1)&1)
   assert doc()==expected
   m.screenshot(O/(font+'-rus-indicator.png'))
   if lang:tap(lang)
   else:m.write(s['bios_vars.keyLocks'],bytes([old]))
   m.call('emu_run_for',ms=500)
   assert m.read(s['editorLastLanguage'],1)[0]==(old&1)
   edit('down');edit('down');edit('down');edit('lf')
   assert visual()==len(expected.split(b'\r')[3].expandtabs(4))
   edit('home')
   for _ in range(11):edit('right')
   edit('x');assert doc()!=expected
   edit('up');chord('rpt','z');assert doc()==expected and val('v_lineNumber')==4 and val('v_column')==11
   m.screenshot(O/(font+'-edit-without-clear.png'))
   action('save',s['lineEditLoop']);edit('enter')
   m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'Tab widths, menu, language polling, edit/undo and save',flush=True)
   continue
  if args.line_undo_only:
   def pending():assert m.read(s['v_lineDirty'],1)==b'\1'
   edit('right');edit('x');first=doc();assert first!=expected
   edit('down');pending()
   action('help',s['uiHelpRead']);edit('esc');pending()
   action('save',s['lineEditLoop']);edit('esc');pending()
   edit('clear',s['editorSaveWait']);edit('esc');pending()
   chord('rpt','z');assert doc()==expected and val('v_lineNumber')==1 and val('v_column')==1
   edit('x');first=doc()
   chord('rpt','lf');pending()
   chord('rpt','z');assert doc()==expected and val('v_lineNumber')==1 and val('v_column')==1
   assert val('v_topLine')==val('v_textBeginLo')
   edit('x');first=doc();edit('down');edit('home');second=val('v_currentLine')
   edit('y');assert val('v_undoLine')==second
   edit('up');chord('rpt','z')
   assert doc()==first and val('v_currentLine')==second and val('v_column')==0
   edit('y');pending()
   action('save',s['lineEditLoop']);edit('enter');pending()
   assert m.read(s['v_documentDirty'],1)==b'\0'
   chord('rpt','z');assert doc()==first and val('v_currentLine')==second
   assert m.read(s['v_documentDirty'],1)==b'\1'
   m.screenshot(O/(font+'-retained-line-undo.png'))
   edit('clear',s['editorSaveWait']);m.stop_at(n['mainLoop']);tap('d');m.until(n['mainLoop'],90,page=0);ready()
   m.write(0x3400,b'B:/EDITTEST.TXT\0');m.write(0x4000,payload)
   m.write(0x3200,w(0x3400)+w(0x4000)+w(len(payload))+bytes(34))
   api['path_io'](m,call,k,0x3200,True)
   call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
   m.stop_at(n['mainLoop']);m.call('regs_set',pc=s['bios_reboot'],sp=s['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'retained snapshot, F8 return, replacement, dialogs and save',flush=True)
   continue
  if args.tabs_only:
   action('tab');action('tab') # Select the historical 8-cell test fixture.
   def visual():return val('v_visualColumn')+(m.read(s['v_visualColumnHi'],1)[0]<<16)
   edit('right');assert val('v_column')==1 and visual()==1
   edit('right');assert val('v_column')==2 and visual()==8
   edit('left');assert val('v_column')==1 and visual()==1
   edit('tab');modified=expected[:1]+b'\t'+expected[1:]
   assert doc()==modified and val('v_column')==2 and visual()==8
   chord('rpt','z');assert doc()==expected and val('v_column')==1
   edit('tab');assert doc()==modified
   gap=(val('v_gapLo'),val('v_gapHi'))
   action('save',s['lineEditLoop']);edit('enter')
   assert (val('v_gapLo'),val('v_gapHi'))==gap
   m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
   command('A:EDIT_MN B:/EDITTEST.TXT',s['editorStatusLoop']);assert doc()==modified
   edit('right');edit('right');edit('right');assert visual()==16 and val('v_column')==3
   edit('down');assert visual()==6 and val('v_column')==6
   edit('down');assert visual()==9 and val('v_column')==2
   edit('up');assert visual()==6 and val('v_column')==6
   edit('home');edit('down');assert visual()==0 and val('v_column')==0
   edit('right');assert visual()==8 and val('v_column')==1
   edit('down');assert visual()==8 and val('v_column')==8
   edit('lf');assert visual()==65 and val('v_column')==62
   assert m.read(s['bios_vars.cursorX'],1)[0]==(visual()-val('v_leftColumn'))*(width//2)
   m.screenshot(O/(font+'-tabs-scroll.png'))
   edit('home');assert val('v_leftColumn')==0
   assert b'\t' not in m.read(s['editorRowBuffer'],96)
   m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
   m.write(0x3400,b'B:/EDITTEST.TXT\0');m.write(0x4000,payload)
   m.write(0x3200,w(0x3400)+w(0x4000)+w(len(payload))+bytes(34))
   api['path_io'](m,call,k,0x3200,True)
   call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
   m.stop_at(n['mainLoop']);m.call('regs_set',pc=s['bios_reboot'],sp=s['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'literal Tab, visual movement, scrolling, undo and save/reopen',flush=True)
   continue
  if args.modern_only:
   assert m.read(s['v_insertMode'],1)==b'\xff'
   edit('lf');assert val('v_column')==400
   edit('right');assert val('v_column')==0 and val('v_lineNumber')==2
   edit('left');assert val('v_column')==400 and val('v_lineNumber')==1
   chord('rpt','lf');assert val('v_lineNumber')==30 and val('v_column')==7
   chord('rpt','home');assert val('v_column')==0 and val('v_lineNumber')==1
   edit('right');edit('enter');assert doc()==expected[:1]+b'\r'+expected[1:]
   edit('backspace');assert doc()==expected
   edit('backspace');assert doc()==expected[1:]
   edit('0');assert doc()==expected
   edit('home');edit('enter');assert doc()==b'\r'+expected
   edit('up');chord('shift','backspace');assert doc()==expected
   action('mode');assert m.read(s['v_insertMode'],1)==b'\0'
   action('mode');assert m.read(s['v_insertMode'],1)==b'\xff'
   action('help',s['uiHelpRead']);m.screenshot(O/(font+'-selection-menu.png'));edit('esc')
   # Cancel the save question and the filename modal; both keep editing.
   edit('clear',s['editorSaveWait']);m.screenshot(O/(font+'-save-changes.png'));edit('esc')
   edit('clear',s['editorSaveWait']);edit('enter',s['lineEditLoop']);edit('esc')
   assert doc()==expected and m.read(s['v_documentDirty'],1)==b'\1'
   # Failed save must not exit or clear the dirty flag.
   edit('clear',s['editorSaveWait']);edit('enter',s['lineEditLoop'])
   replace_input('B:/NO-DIR/ERR.TXT')
   edit('enter',s['editorErrorWait']);m.screenshot(O/(font+'-exit-save-error.png'));edit('enter')
   assert doc()==expected and m.read(s['v_documentDirty'],1)==b'\1'
   # Successful save exits to NC. Reopen and verify the actual disk content.
   edit('clear',s['editorSaveWait']);edit('enter',s['lineEditLoop'])
   replace_input('B:/EDITTEST.TXT')
   m.stop_at(n['mainLoop']);tap('enter');m.until(n['mainLoop'],90,page=0);ready()
   command('A:EDIT_MN B:/EDITTEST.TXT',s['editorStatusLoop']);assert doc()==expected
   edit('x');edit('clear',s['editorSaveWait'])
   m.stop_at(n['mainLoop']);tap('d');m.until(n['mainLoop'],90,page=0);ready()
   command('A:EDIT_MN B:/EDITTEST.TXT',s['editorStatusLoop']);assert doc()==expected
   m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'editing keys, INS default, CTRL navigation, save/cancel/discard/error',flush=True)
   continue
  if args.scroll_only:
   action('load',s['lineEditLoop']);edit('enter')
   # F4 must paint the margins with COLOR_TEXT instead of console black.
   m.screenshot(O/(font+'-f4-open.png'))
   action('add',s['lineEditLoop'])
   m.screenshot(O/(font+'-append-prompt.png'))
   edit('enter')
   assert doc()==expected+expected
   action('load',s['lineEditLoop']);edit('enter');assert doc()==expected
   old_limit=val('v_textLimitLo')
   action('add',s['lineEditLoop'])
   m.write(s['v_textLimitLo'],w(val('v_textEnd')+1))
   edit('enter',s['editorErrorWait'])
   m.screenshot(O/(font+'-append-error.png'))
   edit('enter');assert doc()==expected
   m.write(s['v_textLimitLo'],w(old_limit));m.write(s['v_fullLimit'],w(old_limit))
   chord('rpt','down');m.screenshot(O/(font+'-pagedown.png'));chord('rpt','up')
   # Cross the right edge, then return across the left edge through real keys.
   for _ in range(384//width):edit('right')
   assert val('v_leftColumn')==8
   m.screenshot(O/(font+'-scroll-right.png'))
   edit('home')
   assert val('v_leftColumn')==0
   for _ in range(25):edit('down')
   assert val('v_lineNumber')==26
   assert m.read(s['bios_vars.cursorY'],1)==bytes([s['EDITOR_LAST_Y']])
   m.screenshot(O/(font+'-scroll-down.png'))
   for _ in range(25):edit('up')
   assert val('v_lineNumber')==1 and val('v_topLine')==val('v_textBeginLo')
   m.screenshot(O/(font+'-scroll-up.png'))
   assert doc()==expected
   m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
   print('PASS',font,'keyboard scrolling, header and startup copyright',flush=True)
   continue
  # Last physical column: 64 or 48, no hard-coded 63-column cap.
  last=384//width-1
  for _ in range(last):edit('right')
  assert val('v_column')==384//width-1
  m.screenshot(O/(font+'-last-column.png'))
  edit('lf');assert val('v_column')==400,val('v_column')
  m.screenshot(O/(font+'-long-line.png'))
  edit('z');assert doc() in (expected[:400]+c+expected[400:] for c in (b'Z',b'z'))
  chord('rpt','z');assert doc()==expected
  action('help',s['uiHelpRead']);m.screenshot(O/(font+'-esc-menu.png'));edit('esc')
  edit('home');edit('right');edit('enter')
  assert doc()==expected[:1]+b'\r'+expected[1:]
  edit('backspace');assert doc()==expected
  action('save',s['lineEditLoop']);m.screenshot(O/(font+'-save-prompt.png'));edit('enter')
  print('PASS',font,'editing, undo, Esc menu and save',flush=True)
  # Native Specialist STR (1Fh) is the MCP 'clear' key.
  m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
  command('A:EDIT_MN B:/EDITTEST.TXT',s['editorStatusLoop'])
  assert doc()==expected
  m.stop_at(n['mainLoop']);tap('clear');m.until(n['mainLoop'],90,page=0);ready()
  # Restore fixture for next font.
  m.write(0x3400,b'B:/EDITTEST.TXT\0');m.write(0x4000,payload)
  m.write(0x3200,w(0x3400)+w(0x4000)+w(len(payload))+bytes(34))
  api['path_io'](m,call,k,0x3200,True)
  call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
  m.stop_at(n['mainLoop']);m.call('regs_set',pc=s['bios_reboot'],sp=s['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
  print('PASS',font,'load, columns, long line, undo, Esc, save/reopen',flush=True)
 print('EDITOR UI PASS',flush=True)
except Exception:
 if x.pid:
  m.call('debug_break')
  m.screenshot(O/'failure.png')
  print('FAIL STATE',m.call('cpu_state'),flush=True)
 raise
finally:
 x.stop()
