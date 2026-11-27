"""Visible/muted MAIN vs EXEC editor: launch and full UI timings."""
from pathlib import Path
import argparse,runpy,re,json,struct,hashlib
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/monitor2-001')
p.add_argument('--port',type=int,default=19781,choices=range(19781,19797))
p.add_argument('--profiles',nargs='+',choices=['EDIT_MN','EDIT'],default=['EDIT_MN','EDIT'])
p.add_argument('--font',default='Rotate',choices=['Rotate','Packed6K','Fast12K'])
p.add_argument('--cache-only',action='store_true',help='Check cache capacity, idle prefetch and LRU')
p.add_argument('--prefetch-only',action='store_true',help='Measure EXEC idle prefetch without navigating past the cached window')
p.add_argument('--bars-only',action='store_true',help='Check document scrollbar visibility and movement')
p.add_argument('--modal-only',action='store_true',help='Check modal restore, cache ownership and Tab invalidation')
p.add_argument('--interaction-only',action='store_true',help='Keyboard-only character selection and KOI8/Ctrl-Z checks')
p.add_argument('--short',action='store_true',help='Identical complete-line Elite prefix fitting both editors')
p.add_argument('--navigation-only',action='store_true',help='Fixed 8 KiB Elite prefix; open, page and scroll at viewport edges')
p.add_argument('--ui-timings',action='store_true',help='Measure movement, scrolling, dialog, input and return with the short fixture')
p.add_argument('--counters',action='store_true',help='Collect system call counters in a separate UI scenario run')
p.add_argument('--held-down',choices=['timing','wait-trace'],help='Measure or trace eight held Down events inside one viewport')
p.add_argument('--hex-view',action='store_true',help='Exercise byte view with real keyboard input in both profiles')
p.add_argument('--byte-roundtrip',action='store_true',help='Save, reload and append a mixed binary document through the editor UI')
p.add_argument('--undo-only',action='store_true',help='Time Ctrl+Z after two insertions and after Delete')
p.add_argument('--max-document',action='store_true',help='Load and navigate an H: file at each profile capacity')
p.add_argument('--report',type=Path,default=R/'docs/editor/banks-2026-11-17')
a=p.parse_args()
if a.prefetch_only and a.ui_timings:p.error('Choose one UI timing scenario')
if a.held_down and (a.ui_timings or a.prefetch_only or a.counters):p.error('Choose a separate held-key run')
if a.counters and (not (a.ui_timings or a.prefetch_only) or a.navigation_only):p.error('--counters requires --ui-timings or --prefetch-only')
if a.prefetch_only:
 a.short=a.cache_only=True
 a.profiles=['EDIT']
if a.bars_only or a.navigation_only or a.ui_timings or a.held_down or a.byte_roundtrip or a.hex_view:a.short=True
if a.max_document and (a.short or a.cache_only or a.bars_only or a.modal_only or a.interaction_only):
 p.error('--max-document runs its own load and navigation sequence')
if (a.ui_timings or a.prefetch_only or a.held_down) and a.font!='Rotate':p.error('Returning to NC MAIN requires Rotate for this scenario')
if a.max_document and a.font=='Packed6K':
 p.error('--max-document currently covers Rotate and Fast12K')
if 'EDIT_MN' in a.profiles and not (a.short or a.max_document):p.error('Use --short for MAIN because the full Elite fixture exceeds its document region')
O=a.report;O.mkdir(parents=True,exist_ok=True)
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/f).read_text(),re.M)}
n=sy('NC.sym');c=sy('CLI.sym');k=sy('DOS.sym');g=sy('UI_GFX.sym');t=sy('banked/TRANSPORT.sym')
profile_tools=runpy.run_path(str(R/'scripts/emu80/benchmark-nc-profiles.py'))
profile_tools['symbols'].__globals__.update(SNAP=R)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
gl=h['Matrix'].__init__.__globals__;gl.update(OUT=O,RUNTIME=a.runtime.resolve())
gl['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
probe=runpy.run_path(str(R/'scripts/emu80/ui-counters.py'))['UiCounters'](m,t,sy('banked/KERNEL.sym')) if a.counters else None
rows=[];elite=(R/'assets/games/families/elite/manuals/Elite.txt').read_bytes()
shellend=n['mainLoop'];shellenter=n['but_Enter']
short=elite[:0x6400-sy('EDIT.sym')['EDITOR_TEXT_BEGIN']-129].rsplit(b'\r',1)[0]+b'\r'
if a.bars_only:short=b'X'*180+b'\r'+b'Text\r'*28
if a.navigation_only:short=elite[:8192].rsplit(b'\r',1)[0]+b'\r'
if a.byte_roundtrip:short=b'A\0B\r\n'+bytes(range(256))+b'\n\r\xff\0'
if a.hex_view:short=b'A\r\n'+bytes(range(256))*2
capacity_cases={}
def make_capacity_disk():
 # Fill spare FAT16 clusters in a private copy of the H: fixture.
 raw=bytearray((R/'bin/nested-Games2M.bin').read_bytes())
 def u16(at):return struct.unpack_from('<H',raw,at)[0]
 sector,spc,reserved,copies=u16(11),raw[13],u16(14),raw[16]
 fat_sectors,root_entries=u16(22),u16(17)
 assert raw[54:62].startswith(b'FAT16'), raw[54:62]
 fat=reserved*sector
 root=(reserved+copies*fat_sectors)*sector
 data=root+((root_entries*32+sector-1)//sector)*sector
 cluster_size=sector*spc
 free_clusters=iter(c for c in range(2,2+(len(raw)-data)//cluster_size) if u16(fat+2*c)==0)
 free_entries=iter(off for off in range(root,root+root_entries*32,32) if raw[off] in (0,0xe5,0xff))
 for profile in a.profiles:
  sym=sy('EDITCORE.sym' if profile=='EDIT' else 'EDIT.sym')
  limit=0x6400 if profile=='EDIT_MN' and a.font=='Fast12K' else sym['EDITOR_MEMORY_END']
  length=limit-sym['EDITOR_TEXT_BEGIN']-1
  prefix=(elite*((length+len(elite)-1)//len(elite)))[:length-1]
  payload=prefix+b'\r'
  stem='BANKMAX' if profile=='EDIT' else 'MAINMAX'
  clusters=[next(free_clusters) for _ in range((length+cluster_size-1)//cluster_size)]
  for index,cluster in enumerate(clusters):
   successor=clusters[index+1] if index+1<len(clusters) else 0xffff
   for copy in range(copies):
    struct.pack_into('<H',raw,fat+copy*fat_sectors*sector+2*cluster,successor)
   start=data+(cluster-2)*cluster_size
   chunk=payload[index*cluster_size:(index+1)*cluster_size]
   raw[start:start+len(chunk)]=chunk
  entry=bytearray(32)
  entry[:11]=(stem.ljust(8)+'TXT').encode()
  entry[11]=0x20
  struct.pack_into('<HI',entry,26,clusters[0],length)
  offset=next(free_entries)
  raw[offset:offset+32]=entry
  capacity_cases[profile]=('H:/'+stem+'.TXT',payload)
 fixture=O/'capacity-fixture.bin'
 fixture.write_bytes(raw)
 return fixture
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def tap(key):
 r=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not r.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
def text(v):
 for ch in v:
  if ch=='_':
   m.call('kbd_send',keys=['shift'],action='press');tap('0');m.call('kbd_send',keys=['shift'],action='release')
  else:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(ch,ch.lower()))
def select(page):m.write(0xfffd if page else 0xfffc,bytes([page-1 if page else 0]))
coremem=b''
def capture():
 global coremem
 if banked:
  assert m.read(t['bankPage'],1)==bytes([corepage])
  assert m.read(origin,3)==body[:3]
  coremem=m.read(0,s['EDIT_BANK_STATE_END'])
def get(at,size,bank=False):
 return coremem[at:at+size] if bank else m.read(at,size)
def main_memory(at,size,data=None):
 m.call('debug_break');current=m.read(t['bankPage'],1)[0]
 if current:select(0)
 try:
  if data is not None:m.write(at,data)
  else:return m.read(at,size)
 finally:
  if current:select(current)
def allocations():
 m.call('debug_break');current=m.read(t['bankPage'],1)[0];select(1)
 try:raw=m.read(k['MEM_ALLOC_TABLE'],k['MEM_ALLOC_RECORDS']*6)
 finally:select(current)
 return [dict(bank=raw[i],base=int.from_bytes(raw[i+1:i+3],'little'),size=int.from_bytes(raw[i+3:i+5],'little'),owner=raw[i+5]) for i in range(0,len(raw),6) if raw[i+5]]
def val(name):return int.from_bytes(get(s[name],2,banked),'little')
def help_value(name,size=2):
 symbols=g if banked else s
 return int.from_bytes(main_memory(symbols[name],size),'little')
def help_file_position():
 symbols=g if banked else s
 return main_memory(symbols['flHandle']+symbols['FH_POSITION'],4)
def doc():
 lo=val('v_gapLo');hi=val('v_gapHi');end=val('v_textEnd');begin=val('v_textBeginLo')
 return get(begin,lo-begin,banked)+get(hi,end-lo,banked)
def graphics_counters():
 return struct.unpack('<3H',main_memory(g['ngBatches'],6))
def endwait(modal=False,trace=False):
 target=s['ebWaitReady'] if banked else s['bios_getch'] if modal else s['keyInput']
 if trace and probe:
  state,counts=probe.until(target,corepage,graphics_submit=s['cqDeliverDll'] if banked else None)
  state['counters']=counts
 else:
  m.stop_at(target);state=m.until(target,120,page=corepage)
 if banked:
  capture()
 return state
def dismiss_catalog_warning():
 ready()
 if ('ncCatalogWarningModal' in n and
     m.read(n['uiModalCurrent'],2)==struct.pack('<H',n['ncCatalogWarningModal']) and
     m.read(n['ncDialogActive'],1)==b'\x01'):
  tap('enter');ready()
def launch(name,path):
 global g
 command = 'C:/DOS/'+name+'.COM '+path
 dismiss_catalog_warning();tap('esc');text(command)
 source=c['CMD_LINE'] if shellend==c['cliPrompt'] else n['vars.cmdLine']
 assert m.read(source,len(command)).upper()==command.upper().encode()
 m.screenshot(O/(name+'-command.png'))
 # Start before NC's Enter dispatcher: includes container loading/checksum,
 # arena registration, module loading, file read, row counting and full UI.
 m.stop_at(shellenter);tap('enter');start=m.until(shellenter,30,page=0)['cpu_ticks']
 global corepage
 if probe:
  def is_editor(page):
   input_pc=s['keyInput']
   return (page>1 if banked else page==0) and m.read(input_pc,3)==body[input_pc-origin:input_pc-origin+3]
  state,counts=probe.until(s['ebWaitReady'] if banked else s['keyInput'],None,
                           target_test=is_editor,graphics_submit=s['cqDeliverDll'] if banked else None)
  corepage=m.read(t['bankPage'],1)[0]
  if banked:capture()
 else:
  launch_page=0
  if banked:
   m.stop_at(s['editorBankEntry']);m.until(s['editorBankEntry'],60)
   launch_page=m.read(t['bankPage'],1)[0]
   assert launch_page and m.read(origin,3)==body[:3]
   print('CORE ENTRY',m.call('cpu_state'),flush=True)
  # A command-line launch reads the file before painting the first page.
  if not banked:
   m.stop_at(s['openCommandLineFile']);m.until(s['openCommandLineFile'],240,page=0)
   assert m.read(0,10)==body[:10]
  m.stop_at(s['loadTextFile'])
  read_start=m.until(s['loadTextFile'],90,page=launch_page)['cpu_ticks']
  assert m.read(s['editorInterfaceReady'],1)==b'\0'
  assert m.read(s['editorProgressVisible'],1)==b'\0'
  m.stop_at(s['editorPaintDocument'])
  m.until(s['editorPaintDocument'],120,page=launch_page)
  assert m.read(s['editorInterfaceReady'],1)==b'\0'
  m.stop_at(s['editorStatusLoop'])
  if banked:m.call('bp_set',addr=s['ebFailed'],type='exec')
  for _ in range(90):
   state=m.call('cpu_state');pc=state['registers']['pc']['dec']
   if pc==s['editorStatusLoop']:break
   if banked and pc==s['ebFailed']:raise AssertionError(('CORE FAILED',state))
   m.call('emu_run_for',ms=1000)
  else:raise AssertionError(('EDITOR START TIMEOUT',state))
  corepage=m.read(t['bankPage'],1)[0]
  assert bool(corepage)==banked,(name,corepage,state)
  assert m.read(s['keyInput'],3)==body[s['keyInput']-origin:s['keyInput']-origin+3]
  state=endwait()
 elapsed=state['cpu_ticks']-start
 rows.append(dict(profile=name,font=a.font,operation='launch_document',cycles=elapsed,ms=elapsed/2000,fileBytes=len(payload)))
 if probe:rows[-1]['counters']=counts
 if a.navigation_only:rows[-1]['readToReadyCycles']=state['cpu_ticks']-read_start
 if banked:
  handle=val('ebGfxHandle')
  def read_dos(at,size):
   previous=m.read(t['bankPage'],1)[0]
   try:
    select(1)
    return m.read(at,size)
   finally:select(previous)
  g=profile_tools['graphics_symbols'](read_dos,k,handle)
 if banked and 'ngStoredTexts' in g:
  batches,reads,packet_bytes=graphics_counters()
  rows[-1].update(graphicsBatches=batches,rowReads=reads,graphicsBytes=packet_bytes)
 assert doc()==payload,(name,len(doc()),len(payload))
 allocated=allocations()
 if banked:
  assert any(r['bank']==corepage and r['base']<=origin and r['base']+r['size']>=s['EDITOR_MEMORY_END'] for r in allocated),allocated
  assert any(r['bank']==0 and r['base']==g['entry'] and r['size']==g['IMAGE_MEM_END'] for r in allocated),allocated
  slots=get(s['ebCacheRows'],1,True)[0]
  stride=get(s['ebCacheStride'],1,True)[0]
  assert any(r['bank']==0 and r['base']==s['EDIT_MAIN_PACKET'] and r['size']==s['EDIT_MAIN_END']-s['EDIT_MAIN_PACKET']+slots*stride for r in allocated),allocated
  rows[-1].update(cacheRows=slots,cacheStride=stride)
 else:
  assert any(r['bank']==0 and r['base']==val('v_textBeginLo') and r['size']==val('v_textLimitLo')-val('v_textBeginLo') for r in allocated),allocated
 (O/(name+'-'+a.font+'-allocations.json')).write_text(json.dumps(allocated,indent=2))
 m.screenshot(O/(name+'-loaded.png'));print('LOADED',name,{k:v for k,v in rows[-1].items() if k!='counters'},flush=True)
 if not (banked and a.cache_only):ready()
def edit(key,entry,modifier=None):
 measured=banked and 'ngStoredTexts' in g
 if measured:batches_before,reads_before,bytes_before=graphics_counters()
 ready();m.stop_at(s[entry])
 if modifier:m.call('kbd_send',keys=[modifier],action='press')
 tap(key)
 if modifier:m.call('kbd_send',keys=[modifier],action='release')
 state=m.until(s[entry],60,page=corepage);start=state['cpu_ticks']
 assert m.read(t['bankPage'],1)[0]==corepage
 end=endwait(trace=True);elapsed=end['cpu_ticks']-start
 row=dict(profile=name,font=a.font,operation=entry,cycles=elapsed,ms=elapsed/2000,line=val('v_lineNumber'),column=val('v_column'))
 if 'counters' in end:row['counters']=end['counters']
 if measured:
  batches,reads,packet_bytes=graphics_counters()
  row['rowReads']=(reads-reads_before)&65535
  if entry=='editorPageUp' and not a.cache_only:assert row['rowReads']==0,row
  row['graphicsBatches']=(batches-batches_before)&65535
  row['graphicsBytes']=(packet_bytes-bytes_before)&65535
 if not a.max_document and entry in ('editorPageDown','editorPageUp'):
  bitmap=main_memory(0x9000,0x3000)
  suffix=f"-{row['line']}" if a.cache_only else ''
  reference=O/(entry+suffix+'.vram')
  if not banked:reference.write_bytes(bitmap)
  elif reference.exists():assert bitmap==reference.read_bytes(),(entry,'MAIN/BANK viewport mismatch')
 rows.append(row);print({k:v for k,v in row.items() if k!='counters'},flush=True);ready()
def action(key,target='keyInput',wait_ready=True,shift=False,operation=None):
 ready()
 # The absolute overlay PC can also execute in MAIN inside the font engine.
 # Stop after the bank queue flush, where the entire HEX frame is observable.
 destination='ebWaitReady' if banked and target=='hexWait' else target
 if shift:
  m.call('kbd_send',keys=['shift'],action='press')
  m.call('emu_run_for',ms=500)  # let the Shift toolbar finish before tapping the key
 m.stop_at(s[destination]);tap(key);start=m.until(s[destination],90,page=corepage)
 if shift:m.call('kbd_send',keys=['shift'],action='release')
 if banked:
  stop=endwait(trace=operation is not None)
  if target=='hexWait':assert get(s['hexActive'],1,True)==bytes([1])
 else:
  stop=endwait(modal=True,trace=operation is not None)
 if operation:
  elapsed=stop['cpu_ticks']-start['cpu_ticks']
  rows.append(dict(profile=name,font=a.font,operation=operation,cycles=elapsed,ms=elapsed/2000))
  if 'counters' in stop:rows[-1]['counters']=stop['counters']
 if wait_ready:ready()

def held_down():
 bios=sy('BIOS.sym')
 poll_name='ebPollKey' if banked else 'loc_C23C'
 decode_name='ebInputReady' if banked else 'loc_C311'
 render_name='ebWaitReady' if banked else 'keyInput'
 decoder=s if banked else bios
 wait_trace=a.held_down=='wait-trace'

 def repeat_configuration():
  if banked:
   return {field:get(s['ebKeyState.'+field],1,True)[0] for field in ('repeatCount','repeatStart','timed')}
  return {field:m.read(bios['bios_upper.'+field],1)[0] for field in ('antiBsDelay','cursorDelay')}

 def until(target,trace=False):
  m.stop_at(target)
  if trace:
   m.call('bp_set',addr=t['mx_callRegs'],type='exec',skipCount=1_000_000,comment='F806')
  state=m.until(target,90,page=corepage)
  calls=None
  if trace:
   calls=next(bp['hitCount'] for bp in m.call('bp_list')['breakpoints'] if bp['comment']=='F806')
  return state,calls

 ready()
 before,_=until(decoder[poll_name]);capture()
 first_line,top=val('v_lineNumber'),val('v_topLine')
 assert first_line==1 and top==val('v_textBeginLo') and s['EDITOR_ROWS']>9
 initial_repeat=repeat_configuration()
 samples=[]
 m.call('kbd_send',keys=['down'],action='press')
 try:
  for index in range(8):
   decoded,poll_calls=until(decoder[decode_name],wait_trace)
   assert decoded['registers']['af']['dec']>>8==0x1a,decoded
   accepted,_=until(s['keyHandler_Down'])
   rendered=endwait()
   line,new_top=val('v_lineNumber'),val('v_topLine')
   assert line==first_line+index+1 and new_top==top,(name,index,line,new_top,top)
   configuration=repeat_configuration()
   if banked:
    assert configuration['repeatCount']==s['KEY_REPEAT_FIRST' if index==0 else 'KEY_REPEAT_NEXT']
   else:
    assert configuration==initial_repeat
   polling,setup_calls=until(decoder[poll_name],wait_trace)
   samples.append(dict(decoded=decoded['cpu_ticks'],accepted=accepted['cpu_ticks'],
                       rendered=rendered['cpu_ticks'],pollReady=polling['cpu_ticks'],
                       line=line,top=new_top,pollF806=poll_calls,postRenderF806=setup_calls,
                       repeatConfiguration=configuration))
 finally:
  m.call('kbd_send',keys=['down'],action='release')

 assert doc()==payload
 rows.append(dict(profile=name,font=a.font,operation='held_down',initialLine=first_line,
                  initialTop=top,press=before['cpu_ticks'],decodedAt=decode_name,
                  acceptedAt='keyHandler_Down',renderedAt=render_name,pollingAt=poll_name,
                  initialRepeatConfiguration=initial_repeat,samples=samples))
 print(name,'HELD DOWN',[b['decoded']-a['decoded'] for a,b in zip(samples,samples[1:])],flush=True)

try:
 x.start('editor-banks',disk=make_capacity_disk() if a.max_document else None)
 if a.ui_timings or a.prefetch_only or a.held_down:
  speed=m.call('sys_info')['cpu']
 if a.modal_only:
  dismiss_catalog_warning();m.stop_at(n['uiHelpRead']);tap('f9');m.until(n['uiHelpRead'],90,page=0)
  assert int.from_bytes(m.read(n['uiHelpScroll.total'],2),'little')>=20
  m.screenshot(O/'NC-help.png')
  ready();m.stop_at(n['mainLoop_cursorReady']);tap('esc');m.until(n['mainLoop_cursorReady'],90,page=0)
  ready()
 if a.short:
  api=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
  call=lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a)
  m.write(0x3400,b'B:/COMMON.TXT\0');m.write(0x4000,short)
  m.write(0x3200,struct.pack('<3H',0x3400,0x4000,len(short))+bytes(34))
  api['path_io'](m,call,k,0x3200,True)
  call(k['MXOS_BIOS_SET_INPUT_HOOK'],hl=0)
  m.stop_at(n['mainLoop']);m.call('regs_set',pc=k['bios_reboot'],sp=k['STACK_ADDR']);m.until(n['mainLoop'],90,page=0);ready()
 if a.font!='Rotate':
  # The current NC MAIN body does not fit below Fast12K: use native CLI.
  ready();tap('esc');text('C:/DOS/CLI');m.stop_at(c['cliPrompt']);tap('enter');m.until(c['cliPrompt'],90,page=0);ready()
  shellend=c['cliPrompt'];shellenter=c['cliHistoryAdd']
  ready();tap('esc');text('C:/DOS/FONT 6x8/'+{'Packed6K':'Packed','Fast12K':'Fast'}.get(a.font,a.font));m.stop_at(0);tap('enter');m.until(0,90,page=0)
  m.stop_at(shellend)
  for _ in range(120):
   state=m.until(shellend,90,page=0)
   if m.read(0,16)==(R/'bin/CLI.i80').read_bytes()[:16]:break
   m.call('debug_step',mode='into')
  else:raise AssertionError('FONT did not return to CLI')
  ready()
  assert m.read(k['FONT_ENGINE_HEADER']+7,1)==bytes([{'Rotate':0,'Packed6K':1,'Fast12K':2}[a.font]])
 for name in a.profiles:
  banked=name=='EDIT';origin=0x100 if banked else 0;stem='EDITCORE' if banked else 'EDIT'
  s=sy(stem+'.sym');body=(R/'bin'/(stem+'.i80')).read_bytes();corepage=0
  if a.max_document:
   path,payload=capacity_cases[name]
  else:
   payload=short if a.short else elite;path='B:/COMMON.TXT' if a.short else 'H:/ELITE.TXT'
  launch(name,path)
  if a.held_down:
   held_down()
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0)
   assert m.read(0,4)==(R/'bin/NC.i80').read_bytes()[:4]
   assert not banked or not any(r['bank']==corepage for r in allocations())
   continue
  if a.ui_timings:
   edit('right','keyHandler_Right');edit('left','keyHandler_Left')
   edit('down','editorPageDown','rpt');edit('up','editorPageUp','rpt')
   edit('down','editorPageDown','rpt')
   top=val('v_topLine');edit('up','keyHandler_Up')
   assert val('v_topLine')<top
   for _ in range(s['EDITOR_ROWS']-1):action('down')
   top=val('v_topLine');edit('down','keyHandler_Down')
   assert val('v_topLine')>top
   edit('home','keyHandler_DocumentHome','rpt')
   action('f3','editorFind',operation='open_search_dialog')
   action('esc')
   edit('x','keyHandler_Space');edit('y','keyHandler_Space')
   assert doc()==b'XY'+payload or doc()==b'xy'+payload
   action('f10','editorSaveWait')
   m.stop_at(s['editorDiscardFile']);tap('d')
   start=m.until(s['editorDiscardFile'],120,page=corepage)['cpu_ticks']
   if probe:
    state,counts=probe.until(shellend,0,graphics_submit=s['cqDeliverDll'] if banked else None,
                             graphics_page=corepage)
    stop=state['cpu_ticks']
   else:
    m.stop_at(shellend);stop=m.until(shellend,120,page=0)['cpu_ticks']
   assert m.read(0,4)==(R/'bin/NC.i80').read_bytes()[:4]
   assert not banked or not any(r['bank']==corepage for r in allocations())
   ready()
   rows.append(dict(profile=name,font=a.font,operation='return_nc',verified=True,cycles=stop-start,ms=(stop-start)/2000))
   if probe:rows[-1]['counters']=counts
   continue
  if a.hex_view:
   edit('x','keyHandler_Space')
   edited=b'x'+payload
   assert doc()==edited
   ready();m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=500)
   m.stop_at(s['hexToggle']);tap('f9')
   start=m.until(s['hexToggle'],60,page=corepage)['cpu_ticks']
   wait=s['ebWaitReady' if banked else 'bios_getch']
   m.stop_at(wait)
   elapsed=m.until(wait,90,page=corepage)['cpu_ticks']-start
   if banked:capture()
   m.call('kbd_send',keys=['shift'],action='release');ready()
   rows.append(dict(profile=name,font=a.font,operation='enter_hex',cycles=elapsed,ms=elapsed/2000))
   print(rows[-1],flush=True)
   assert val('hexCaret')==1 and doc()==edited
   action('right','hexWait');action('right','hexWait')
   action('f6','hexWait')
   action('f4','hexWait');action('down','hexWait');action('f4','hexWait')
   assert val('hexCaret')==19 and val('v_selectionEnd')-val('v_selectionBegin')==16
   m.screenshot(O/(name+'-hex-raw.png'))
   action('f4','hexWait');action('up','hexWait')
   action('f9','keyInput')
   assert val('v_column')==2 and doc()==edited
   action('f9','hexWait',shift=True)
   assert val('hexCaret')==3
   action('left','hexWait');action('left','hexWait')
   action('f9','keyInput')
   edit('z','editorUndoLine','rpt')
   assert doc()==payload
   action('f9','hexWait',shift=True)
   assert val('hexCaret')==0
   if banked:assert val('cqAllocation')==s['EDIT_HEX_QUEUE_BYTES']
   action('b','hexWait')
   assert doc()==bytes([0xB1])+payload[1:] and val('hexCaret')==0
   action('right','hexWait')
   action('0','hexWait');action('c','hexWait')
   expected=bytes([0xB1,0x0C])+payload[2:]
   assert doc()==expected and val('hexCaret')==2
   output = ('H' if banked else 'B') + ':/ROUND.BIN'
   action('f5','lineEditLoop');text(output);action('enter','hexWait')
   assert doc()==expected and get(s['v_documentDirty'],1,banked)==bytes(1)
   action('f9','keyInput')
   if banked:assert val('cqAllocation')==1024
   action('f4','lineEditLoop',shift=True);text(output);action('enter')
   assert doc()==expected
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   print('HEX VIEW PASS',name,flush=True);continue
  if a.byte_roundtrip:
   action('right');action('right')
   edit('x','keyHandler_Space');assert doc()==payload[:2]+b'x'+payload[2:]
   edit('z','editorUndoLine','rpt');assert doc()==payload
   action('f5','lineEditLoop',shift=True);text('B:/ROUND.BIN');action('enter')
   action('f3',shift=True);assert doc()==b''
   action('f5','lineEditLoop',shift=True);text('B:/EMPTY.BIN');action('enter')
   action('f4','lineEditLoop',shift=True);text('B:/ROUND.BIN');action('enter')
   assert doc()==payload
   action('f6','lineEditLoop',shift=True);text('B:/COMMON.TXT');action('enter')
   assert doc()==payload*2
   action('f4','editorSaveWait',shift=True);action('d','lineEditLoop');text('B:/EMPTY.BIN');action('enter')
   assert doc()==b'' and val('editorBarV.total')==1
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   print('BYTE ROUNDTRIP PASS',name,flush=True);continue
  if a.navigation_only:
   edit('down','editorPageDown','rpt');edit('up','editorPageUp','rpt')
   assert val('v_lineNumber')==1
   edit('down','editorPageDown','rpt')
   top=val('v_topLine')
   edit('up','keyHandler_Up')
   assert val('v_topLine')<top
   for _ in range(s['EDITOR_ROWS']-1):action('down')
   top=val('v_topLine')
   edit('down','keyHandler_Down')
   assert val('v_topLine')>top and doc()==payload
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   continue
  if a.max_document:
   assert val('v_textLimitLo')-val('v_textBeginLo')-1==len(payload)
   edit('down','editorPageDown','rpt')
   edit('up','editorPageUp','rpt')
   edit('lf','keyHandler_DocumentEnd','rpt')
   assert val('v_lineNumber')==payload.count(b'\r')+1 and val('v_column')==0
   edit('home','keyHandler_DocumentHome','rpt')
   assert val('v_lineNumber')==1 and val('v_column')==0 and doc()==payload
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   assert not banked or not any(r['bank']==corepage for r in allocations())
   print('CAPACITY PASS',name,a.font,len(payload),flush=True);continue
  if a.bars_only:
   assert val('editorBarV.total')==30 and get(s['editorCopyWidth'],1,banked)[0]<48
   assert get(s['editorHShown'],1,banked)==b'\x01'
   m.screenshot(O/(name+'-both-document-bars.png'))
   action('lf',wait_ready=False)
   assert val('editorBarH.position')==val('editorBarH.total')-val('editorBarH.visible')
   action('home');action('down',wait_ready=False)
   assert get(s['editorHShown'],1,banked)==b'\x01'
   assert get(s['v_columns'],1,banked)[0]==376//(get(s['v_charStep'],1,banked)[0]*2)
   edit('down','editorPageDown','rpt')
   assert val('editorBarV.position')>0
   assert get(s['editorHShown'],1,banked)==b'\x00'
   action('f3',shift=True,wait_ready=False)
   assert get(s['editorCopyWidth'],1,banked)==b'\x30'
   assert get(s['editorHShown'],1,banked)==b'\x00'
   start=s['EDITOR_TEXT_TOP']
   assert main_memory(0xbf00+start,s['EDITOR_BAR_Y']-start)==bytes(s['EDITOR_BAR_Y']-start)
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   print('DOCUMENT BARS PASS',name,flush=True);continue
  if a.cache_only:
   if banked:
    slots=get(s['ebCacheRows'],1,True)[0]
    stride=get(s['ebCacheStride'],1,True)[0]
    assert slots=={'Rotate':255,'Packed6K':200,'Fast12K':104}[a.font] and stride==64
    assert val('ebPrefetched')==0
    m.stop_at(s['ebPrefetchCopy']);copy_start=m.until(s['ebPrefetchCopy'],90,page=corepage)['cpu_ticks']
    prefetched_at_copy=int.from_bytes(m.read(s['ebPrefetched'],2),'little')
    before=main_memory(0x9000,0x3000)
    if probe:
     copied,copy_counts=probe.until(s['ebPollKey'],corepage)
     copy_stop=copied['cpu_ticks']
    else:
     m.stop_at(s['ebPollKey']);copy_stop=m.until(s['ebPollKey'],90,page=corepage)['cpu_ticks']
    copy_rows=int.from_bytes(m.read(s['ebPrefetched'],2),'little')-prefetched_at_copy
    assert copy_rows>0
    assert main_memory(0x9000,0x3000)==before,'Prefetch changed the screen'
    m.stop_at(s['ebPrefetch']);start=m.until(s['ebPrefetch'],90,page=corepage)['cpu_ticks']
    before_count=int.from_bytes(m.read(s['ebPrefetched'],2),'little')
    m.stop_at(s['ebPollKey']);stop=m.until(s['ebPollKey'],90,page=corepage)['cpu_ticks']
    after_count=int.from_bytes(m.read(s['ebPrefetched'],2),'little')
    assert stop-start<65536,(start,stop)
    ready();m.call('emu_run_for',ms=1200)
    m.stop_at(s['ebPollKey']);m.until(s['ebPollKey'],90,page=corepage);capture()
    assert val('ebPrefetched')>=s['EDITOR_PAGE_STEP'],val('ebPrefetched')
    rows.append(dict(profile=name,font=a.font,operation='idle_prefetch',cycles=stop-start,
                     sliceRows=after_count-before_count,copyToPollCycles=copy_stop-copy_start,
                     copyRows=copy_rows,cachedRows=val('ebPrefetched'),cacheRows=slots,cacheStride=stride))
    if probe:rows[-1]['counters']=copy_counts
    print('PREFETCH',slots,stride,val('ebPrefetched'),stop-start,flush=True)
    if a.prefetch_only:
     m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0)
     assert m.read(0,4)==(R/'bin/NC.i80').read_bytes()[:4]
     assert not any(r['bank']==corepage or r['bank']==0 and r['base']==s['EDIT_MAIN_PACKET'] for r in allocations())
     continue
   def page(key):
    edit(key,'editorPageDown' if key=='down' else 'editorPageUp','rpt')
    return val('v_topLine')
   first=val('v_topLine')
   second=page('down')
   assert second>first and (not banked or rows[-1]['rowReads']==0)
   third=page('down')
   assert third>second and page('up')==second and page('up')==first
   if banked:
    tags=s['EDIT_TAGS']
    expected=get(s['ebValid'],1,True)[0]
    prev=255
    node=get(s['ebOldest'],1,True)[0]
    visited=set()
    while node!=255:
     assert node<expected and node not in visited
     visited.add(node)
     older,newer=get(tags+8*node+5,2,True)
     assert older==prev
     prev,node=node,newer
    assert len(visited)==expected and prev==get(s['ebNewest'],1,True)[0]
    prefetched=val('ebPrefetched')
    edit('lf','keyHandler_DocumentEnd','rpt')
    m.call('emu_run_for',ms=1200)
    m.stop_at(s['ebPollKey']);m.until(s['ebPollKey'],90,page=corepage);capture()
    assert val('ebPrefetched')-prefetched>=s['EDITOR_PAGE_STEP'],val('ebPrefetched')
    page('up')
    assert rows[-1]['rowReads']==0,rows[-1]
   m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   assert not banked or not any(r['bank']==corepage or r['bank']==0 and r['base']==s['EDIT_MAIN_PACKET'] for r in allocations())
   print('DYNAMIC CACHE AND PREFETCH PASS',name,flush=True);continue
  if a.modal_only:
   for _ in range(4):action('down')
   action('f4')
   for _ in range(5):action('right')
   action('f4',wait_ready=False)
   clean=main_memory(0x9000,0x3000)
   marker=0x9000+s['EDITOR_FIRST_Y']-7
   main_memory(marker,1,bytes([clean[marker-0x9000]^0x33]))
   before=main_memory(0x9000,0x3000) if banked else clean
   for pressed in (True,False):
    ready()
    m.call('kbd_send',keys=['shift'],action='press' if pressed else 'release')
    m.call('emu_run_for',ms=500)
    if banked:
     m.stop_at(s['ebPollKey']);m.until(s['ebPollKey'],90,page=corepage)
    assert m.read(s['editorShiftState'],1)==bytes([pressed])
   action('f6','lineEditLoop',shift=True);action('esc',wait_ready=False)
   assert doc()==payload
   action('f2','uiHelpRead',wait_ready=False)
   assert doc()==payload
   if banked:
    assert main_memory(g['ngHelpActive'],1)==b'\x01'
    assert main_memory(g['ngHelpSize']+1,1)==b'\x20'
    height=main_memory(g['ngHelpSize'],1)[0]
    assert height==148
    rect=int.from_bytes(main_memory(g['ngHelpScreen'],2),'little')
    pixels=b''.join(before[rect-0x9000+i*256:rect-0x9000+i*256+height] for i in range(32))
    assert main_memory(s['EDIT_MAIN_PIXELS'],32*height)==pixels
   m.screenshot(O/(name+'-help-open.png'))
   before_page=main_memory(0x9000,0x3000)
   offset=help_file_position()
   action('down','uiHelpRead',wait_ready=False)
   assert help_value('uiHelpScroll.position')==1
   assert help_file_position()==offset,'Buffered step unexpectedly read disk'
   down_page=main_memory(0x9000,0x3000)
   help_symbols=g if banked else s
   y=help_symbols['UI_HELP_Y']
   for col in range(9,9+help_symbols['UI_HELP_COLUMNS']):
    start=col*256+y
    assert down_page[start:start+80]==before_page[start+10:start+90]
   action('up','uiHelpRead',wait_ready=False)
   assert help_value('uiHelpScroll.position')==0 and main_memory(0x9000,0x3000)==before_page
   action('lf','uiHelpRead',wait_ready=False)
   assert help_value('uiHelpScroll.position')==help_value('uiHelpLast')
   m.screenshot(O/(name+'-help-end.png'))
   action('home','uiHelpRead',wait_ready=False)
   assert help_value('uiHelpScroll.position')==0
   action('esc',wait_ready=False)
   assert main_memory(0x9000,0x3000)==before,(name,'Help restore')
   if banked:assert main_memory(g['ngHelpActive'],1)==b'\0'
   action('f4','lineEditLoop',wait_ready=False,shift=True);action('esc',wait_ready=False)
   assert main_memory(0x9000,0x3000)==before,(name,'filename restore')
   action('f9','hexWait',shift=True);action('f9',shift=True)
   action('f9',wait_ready=False)  # unmodified F9 preserves the rebuilt text view
   before=clean  # a full view switch repaints the synthetic modal marker
   assert main_memory(0x9000,0x3000)==before,(name,'return from HEX')
   action('f2','uiHelpRead');action('t','uiHelpRead');action('esc',wait_ready=False)
   assert get(s['v_tabWidth'],1,banked)==b'\x04' and get(s['editorLastTab'],1,banked)==b'\x04'
   assert main_memory(0x9000,0x3000)==before,(name,'T must not change Help')
   action('f1',wait_ready=False,shift=True)
   assert get(s['v_tabWidth'],1,banked)==b'\x06'
   ready();m.stop_at(shellend);tap('f10');m.until(shellend,120,page=0);ready()
   assert not banked or not any(r['bank']==corepage for r in allocations())
   print('MODAL RESTORE PASS',name,a.font,flush=True);continue
  if a.interaction_only:
   # ELITE is a five-byte word inside the first line, not a whole row.
   for _ in range(22):action('right')
   action('f4')
   for _ in range(5):action('right')
   action('f4');assert val('v_selectionEnd')-val('v_selectionBegin')==5
   m.screenshot(O/(name+'-byte-selection.png'))
   action('home');action('down');action('down');action('right')
   point=payload.index(b'\r',payload.index(b'\r')+1)+2
   action('f5');copied=payload[:point]+b'ELITE'+payload[point:];assert doc()==copied
   action('f4');action('home');action('right');action('f4')
   for _ in range(5):action('right')
   action('f4');assert val('v_selectionEnd')-val('v_selectionBegin')==5
   edit('lf','keyHandler_DocumentEnd','rpt');action('f6');assert doc()==payload+b'ELITE'
   action('f8');assert doc()==payload
   edit('home','keyHandler_DocumentHome','rpt')
   locks=get(s['ebKeyLocks'] if banked else s['bios_vars.keyLocks'],1,banked)[0];old=locks&1
   def typed_letter(language, uppercase):
    char='а' if language else 'a'
    return (char.upper() if uppercase else char).encode('koi8-r')[0]
   def undo():
    ready();m.call('kbd_send',keys=['rpt'],action='press');action('z');m.call('kbd_send',keys=['rpt'],action='release');ready()
   ready();m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=500)
   observed=s['editorLastLanguage']
   def read_language():
    if banked:
     m.stop_at(s['ebPollKey']);m.until(s['ebPollKey'],90,page=corepage)
    return m.read(observed,1)[0]
   assert read_language()==old
   action('a');assert doc()[0]==typed_letter(old,bool(locks&128))
   m.call('kbd_send',keys=['shift'],action='release');ready()
   before=doc();action('f9','hexWait',shift=True);action('f9',shift=True);assert doc()==before;undo();assert doc()==payload
   ready();tap('caps');m.call('emu_run_for',ms=500);assert read_language()==old^1
   action('a');assert doc()[0]==typed_letter(old^1,not locks&128);undo();assert doc()==payload
   ready();m.call('kbd_send',keys=['shift'],action='press');tap('caps')
   m.call('kbd_send',keys=['shift'],action='release');m.call('emu_run_for',ms=500)
   assert read_language()==old^1
   action('a');assert doc()[0]==typed_letter(old^1,bool(locks&128));undo();assert doc()==payload
   ready();m.call('kbd_send',keys=['shift'],action='press');tap('caps')
   m.call('kbd_send',keys=['shift'],action='release');m.call('emu_run_for',ms=500);tap('caps');m.call('emu_run_for',ms=500)
   action('f2','uiHelpRead');m.screenshot(O/(name+'-new-help.png'));action('esc')
   action('f10','editorSaveWait');m.stop_at(shellend);tap('d');m.until(shellend,120,page=0);ready()
   assert not banked or not any(r['bank']==corepage for r in allocations())
   print('CHARACTER SELECTION/KOI8/CTRL-Z PASS',name,flush=True);continue
  edit('down','editorPageDown','rpt');edit('up','editorPageUp','rpt');assert val('v_lineNumber')==1
  edit('x','keyHandler_Space');assert doc()==b'x'+payload
  edit('y','keyHandler_Space');assert doc()==b'xy'+payload
  edit('z','editorUndoLine','rpt');assert doc()==payload
  edit('backspace','keyHandler_Del','shift');assert doc()==payload[1:]
  edit('z','editorUndoLine','rpt');assert doc()==payload
  if a.undo_only:
   assert val('editorBarV.total')==payload.count(b'\r')+1
   action('f10','editorSaveWait',wait_ready=False)
   ready();m.stop_at(shellend);tap('d');m.until(shellend,120,page=0);ready()
   assert not banked or not any(r['bank']==corepage for r in allocations())
   print('UNDO PASS',name,a.font,len(payload),flush=True);continue
  edit('down','keyHandler_Down');edit('up','keyHandler_Up')
  edit('lf','keyHandler_DocumentEnd','rpt');edit('home','keyHandler_DocumentHome','rpt')
  # Search punctuation through the normal modal (case/layout independent).
  action('f3','lineEditLoop');text(' - ');action('enter')
  edit('f7','editorFindNext')
  action('f3','lineEditLoop')
  # Clear previous pattern using its visible input editor.
  for _ in range(3):tap('backspace')
  text('999999999')
  m.stop_at(s['editorSearchStart']);tap('enter')
  state=m.until(s['editorSearchStart'],60,page=corepage);start=state['cpu_ticks']
  if banked:end=endwait()
  else:
   m.stop_at(s['bios_getch']);end=m.until(s['bios_getch'],60,page=0)
  elapsed=end['cpu_ticks']-start
  rows.append(dict(profile=name,font=a.font,operation='search_missing_full',cycles=elapsed,ms=elapsed/2000))
  print(rows[-1],flush=True)
  m.screenshot(O/(name+'-information.png'));ready();action('enter')
  # Save to the isolated RAM disk, then reload and compare all logical bytes.
  action('f5','lineEditLoop',shift=True)
  for _ in range(len(path)):tap('backspace')
  text('B:/ROUND.TXT')
  ready();m.stop_at(s['editorWriteGapFile']);tap('enter');m.until(s['editorWriteGapFile'],90,page=corepage)
  assert m.read(s['editorProgressVisible'],1)==b'\x01'
  m.screenshot(O/(name+'-saving.png'))
  endwait();assert doc()==payload;ready()
  action('f4','lineEditLoop',shift=True);action('enter');assert doc()==payload
  action('f2','uiHelpRead');m.screenshot(O/(name+'-help.png'));action('esc')
  action('f1',shift=True);assert get(s['v_tabWidth'],1,banked)==b'\x06'
  edit('x','keyHandler_Space')
  # Exit dirty -> discard, then verify return to the shell and owner cleanup.
  action('f10','editorSaveWait')
  m.stop_at(shellend);tap('d');m.until(shellend,120,page=0);ready()
  assert not banked or not any(r['bank']==corepage for r in allocations()),'EXEC allocation leaked'
  print('PASS',name,flush=True)
 report=dict(cpuHz=2000000,fileSha256=hashlib.sha256(elite).hexdigest(),
             binarySha256={f:hashlib.sha256((R/'bin'/(f+'.i80')).read_bytes()).hexdigest() for f in ['EDIT','EDIT_DOS','EDITCORE','UI_GFX']},
             scope='Handler entry to next key wait, including queued MAIN flush and all rendering. Launch starts at NC Enter dispatch with Rotate or CLI Enter completion with Fast12K. Keyboard scanning and host RPC time excluded.',
             rows=rows)
 if a.max_document:
  report['files']={profile:dict(path=path,bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
                   for profile,(path,data) in capacity_cases.items()}
 else:
  report.update(fileBytes=len(payload),payloadSha256=hashlib.sha256(payload).hexdigest())
 if a.ui_timings or a.prefetch_only or a.held_down:report['cpu']=speed
 suffix='-counters.json' if a.counters else '-results.json'
 if a.held_down:suffix=f'-held-down-{a.held_down}.json'
 (O/(a.font+'-'+'-'.join(a.profiles)+suffix)).write_text(json.dumps(report,indent=2))
 print('EDITOR BANK BENCH PASS',flush=True)
except Exception:
 if x.pid and gl['mcp_owner']()==str(x.pid):
  m.call('debug_break');m.screenshot(O/'failure.png');print('FAIL',m.call('cpu_state'),flush=True)
 raise
finally:x.stop()
