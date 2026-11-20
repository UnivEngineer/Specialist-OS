"""Command queue: full direct generation/transfer/execute cost and flush ordering on 8080."""
from pathlib import Path
import argparse,json,re,runpy,struct,subprocess,os
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782');p.add_argument('--port',type=int,choices=range(19781,19785),default=19782);p.add_argument('--out',type=Path,default=R/'docs/bench/mxcall-2026-11-17/queue');a=p.parse_args();O=a.out.resolve();O.mkdir(parents=True,exist_ok=True)
sy=lambda f:{n:int(v,16) for n,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',Path(f).read_text('utf-8'),re.M)}
k=sy(R/'bin/banked/LOCALBLOCK.sym');t=sy(R/'bin/banked/TRANSPORT.sym');b=sy(R/'bin/BIOS.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=a.runtime.resolve());g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue;if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp';W=lambda n:struct.pack('<H',n)
def page(n):m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))
def asm(n,custom=False):
 text=f' INCLUDE "{(R/"source/include/mxos.inc").as_posix()}"\n ORG 02000h\n'
 if custom:text+=' DEFINE COMMAND_QUEUE_SEND customSend\n'
 def section(name,body):
  nonlocal text
  text+=name+':\n'+body+'\n jp 07000h\n'
 section('init',' ld hl,04000h\n ld bc,'+('1024' if custom else '256')+'\n ld de,32\n call commandQueueInit')
 section('small',' ld hl,04000h\n ld bc,32\n ld de,8\n call commandQueueInit')
 section('off',' ld hl,0\n call mx_queueBind')
 section('direct','\n'.join(f' ld hl,{0x100a+i*256}\n MXOS_CALL MXOS_BIOS_SET_CURSOR_POS' for i in range(n)))
 def emit(i):
  return f' ld hl,(cqNext)\n ld (hl),MXOS_BIOS_SET_CURSOR_POS & 255\n inc hl\n ld (hl),5\n inc hl\n ld (hl),1\n inc hl\n ld (hl),10\n inc hl\n ld (hl),{16+i}\n inc hl\n call commandQueueCommit'
 section('enqueue','\n'.join(emit(i) for i in range(n)))
 section('flush',' call mx_queueFlush')
 section('queued',' call enqueueBody\n call mx_queueFlush')
 text+='enqueueBody:\n'+'\n'.join(emit(i) for i in range(n))+'\n ret\n'
 section('copyAppend',' ld hl,(cqNext)\n ld (hl),MXOS_BIOS_MEMCPY & 255\n inc hl\n ld (hl),9\n inc hl\n ld (hl),7\n inc hl\n ld (hl),0\n inc hl\n ld (hl),061h\n inc hl\n ld (hl),1\n inc hl\n ld (hl),061h\n inc hl\n ld (hl),0\n inc hl\n ld (hl),060h\n inc hl\n call commandQueueCommit')
 section('copy',' ld hl,copyReq\n call mx_memcpyFar')
 section('oversize',' ld bc,257\n call commandQueueReserve')
 section('emptyBatch',' ld a,6\n ld hl,04000h\n ld bc,0\n MXOS_CALL MXOS_BIOS_COMMAND_BATCH')
 section('dosBatch',' MXOS_CALL MXOS_DOS_COMMAND_BATCH')
 section('badBatch',' ld a,6\n ld hl,04000h\n ld bc,257\n MXOS_CALL MXOS_BIOS_COMMAND_BATCH')
 section('wide',' ld hl,04000h\n ld bc,1024\n ld de,32\n call commandQueueInit')
 section('loadBank',' ld hl,dllPath\n ld a,MODULE_TARGET_BANK\n MXOS_CALL MXOS_DOS_MODULE_LOAD\n ld (handle),hl')
 section('loadDll',' ld hl,dllPath\n xor a\n MXOS_CALL MXOS_DOS_MODULE_LOAD\n ld (handle),hl')
 section('scope',' ld hl,(handle)\n ld de,controller\n MXOS_CALL MXOS_DOS_MODULE_SCOPE')
 section('coldCall',' ld hl,(handle)\n ld de,inputRegs\n MXOS_CALL MXOS_DOS_MODULE_CALL')
 section('scopePerf',' ld hl,(handle)\n ld de,runDirect\n MXOS_CALL MXOS_DOS_MODULE_SCOPE')
 section('legacyPerf',' call runLegacy')
 for label,op in (('runLegacy','MXOS_CALL MXOS_DOS_MODULE_CALL'),('runDirect','call mx_routeCall')):
  text+=label+':\n'+(' ld hl,(handle)\n ld de,inputRegs\n '+op+'\n')*16+' ret\n'
 section('expired',' ld hl,(handle)\n ld de,inputRegs\n call mx_routeCall')
 section('unload',' ld hl,(handle)\n xor a\n MXOS_CALL MXOS_DOS_MODULE_UNLOAD')
 text+='''controller:
 ld hl,(handle)
 ld de,inputRegs
 call mx_routeCall
 ld (scalarResult),hl
 ld a,MX_QUEUE_DLL
 ld hl,(handle)
 call commandQueueSelect
 ld bc,320
 call commandQueueReserve
 ret c
 ld (hl),77
 ld de,320
 add hl,de
 call commandQueueCommit
 ld a,MX_QUEUE_DOS
 call commandQueueSelect
 ld hl,(cqNext)
 ld (hl),MXOS_DOS_FLUSH_VOLUME & 255
 inc hl
 ld (hl),3
 inc hl
 ld (hl),0
 inc hl
 call commandQueueCommit
 xor a
 call commandQueueSelect
 ld hl,(handle)
 MXOS_CALL MXOS_DOS_MODULE_UNLOAD
 ld (pinnedError),a
 ret
handle: DW 0
scalarResult: DW 0
pinnedError: DB 0
inputRegs: DW 01234h,0,0,0
dllPath: DB "B:/QROUTE.DLL",0
'''
 section('get',' ld hl,06000h\n ld b,0\n ld a,MEM_OP_GET8\n call mx_memory')
 section('clear',' ld bc,01Fh\n MXOS_CALL MXOS_BIOS_PRINT_CHAR')
 section('text',' ld bc,1+textEnd-textBytes+2\n call commandQueueReserve\n ld (hl),MXOS_BIOS_PRINT_STRING_Z & 255\n inc hl\n ld (hl),1+textEnd-textBytes+2\n inc hl\n ld (hl),16\n inc hl\n ex de,hl\n ld hl,textBytes\n ld bc,textEnd-textBytes\n call copyLocal\n ex de,hl\n call commandQueueCommit\n call mx_queueFlush')
 text+='copyLocal:\n ld a,(hl)\n ld (de),a\n inc hl\n inc de\n dec bc\n ld a,b\n or c\n jp nz,copyLocal\n ret\n'
 for i in range(n):text+=f'packet{i}: DB 1\n DW {0x100a+i*256}\n'
 text+='copyArgs: DB 7\n DW 06100h,06101h,06000h\ncopyReq: DB 6\n DW 05000h\n DB 0\n DW 06000h,1\ntextArgs: DB 16\ntextBytes: DB "Command queue: inline text, COPY flush, registers OK",0\ntextEnd:\n'
 text+='customSend:\n ld h,b\n ld l,c\n ld (customBytes),hl\n ret\ncustomBytes: DW 0\n'
 text+=f' INCLUDE "{(R/"source/include/command-queue.inc").as_posix()}"\n ASSERT $<04000h\n'
 src=O/'fixture.asm';src.write_text(text,encoding='utf-8');subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'fixture.bin'),'--sym='+str(O/'fixture.sym'),str(src)],check=True,capture_output=True)
 return sy(O/'fixture.sym'),(O/'fixture.bin').read_bytes()
def run(s,label,bank=6,**regs):
 page(bank);m.stop_at(0x7000);m.call('regs_set',pc=s[label],sp=0x7000,**regs);start=m.call('cpu_state')['cpu_ticks'];z=m.until(0x7000,40,page=bank);elapsed=m.call('cpu_state')['cpu_ticks']-start
 r={n:v['dec'] for n,v in z['registers'].items()};assert r['sp']==0x7000,r
 return elapsed,r
def cursor():
 page(0);v=int.from_bytes(m.read(b['bios_vars.cursorY'],2),'little');page(6);return v
rows=[]
try:
 x.start('command queue',system=R/'images/SystemRom_emu80_writable.bin');m.call('debug_break');m.call('bp_clear')
 cli=sy(R/'bin/CLI.sym');m.stop_at(cli['cliPrompt']);m.tap('f10');m.until(cli['cliPrompt'],90,page=0);m.call('bp_clear')
 page(6)
 for addr,name in ((k['MEM_IMAGE_LOCAL_START'],'LOCALBLOCK'),(k['mx_call'],'CLIENT'),(t['BANK_LANDING'],'TRANSPORT')):m.write(addr,(R/f'bin/banked/{name}.bin').read_bytes())
 m.write(k['MEM_BANK_KIND'],bytes([k['MEM_BANK_EXEC']]))
 for bank in (0,6):
  page(bank);m.write(t['bankPage'],bytes([bank]));m.write(t['bankOwner'],bytes([7+bank,bank]));m.write(t['bankDepth'],b'\0');m.write(t['bankServiceSP'],W(k['DOS_BANK_STACK_TOP'] if bank else b['BIOS_BANK_STACK_TOP']));m.write(t['bankStackFloor'],W((k['DOS_BANK_STACK_BOTTOM'] if bank else b['BIOS_BANK_STACK_BOTTOM'])+64))
 page(0);m.write(0xffc0,(R/'bin/banked/WINDOW.bin').read_bytes())
 for n in (1,4,16,64):
  s,body=asm(n);page(6);m.write(0x2000,body);run(s,'off');direct,_=run(s,'direct');assert cursor()==0x100a+(n-1)*256
  run(s,'init');queued,_=run(s,'queued');assert cursor()==0x100a+(n-1)*256
  assert m.read(s['cqUsed'],2)==W(0)
  rows.append(dict(commands=n,direct=direct,queued=queued,speedup=direct/queued));print('PERF',rows[-1],flush=True)
 # Explicit flush preserves all CPU registers/flags with pending commands.
 run(s,'init');run(s,'enqueue');values=dict(af=0xa503,bc=0x3456,de=0x2345,hl=0x1234);_,r=run(s,'flush',**values)
 assert all(r[v]==z for v,z in values.items()),r
 print('PASS explicit flush preserves AF/BC/DE/HL/SP',flush=True)
 run(s,'init');run(s,'enqueue');before=m.read(0x4000,256);used=m.read(s['cqUsed'],2);_,r=run(s,'oversize');assert r['af']&1 and r['af']>>8==k['MEM_ERROR_RANGE'];assert m.read(0x4000,256)==before and m.read(s['cqUsed'],2)==used
 print('PASS oversized command rejected without flush or buffer writes',flush=True)
 run(s,'flush')
 # A queued MAIN memcpy writes 1; the following FAR copy must overwrite it with 2.
 run(s,'init');page(0);m.write(0x6100,b'\x01');m.write(0x6000,b'\0');page(6);m.write(0x5000,b'\x02');run(s,'copyAppend');run(s,'copy');page(0);assert m.read(0x6000,1)==b'\x02';page(6);assert m.read(s['cqUsed'],2)==W(0)
 print('PASS pending commands execute before COPY',flush=True)
 run(s,'enqueue');_,r=run(s,'get');assert r['de']&255==2,r;assert m.read(s['cqUsed'],2)==W(0)
 print('PASS pending commands flush before scalar memory protocol',flush=True)
 run(s,'clear');run(s,'text');assert m.read(s['cqUsed'],2)==W(0);m.screenshot(O/'inline-text.png');print('PASS inline string payload',flush=True)
 run(s,'off');assert m.read(k['MEM_QUEUE_HOOK'],2)==W(0);print('PASS queue disabled',flush=True)
 page(0);guard=m.read(b['BIOS_BANK_IO_BUFFER'],272);page(6);_,r=run(s,'badBatch');assert r['af']&1 and r['af']>>8==k['DOS_ERR_ARGUMENT'];page(0);assert m.read(b['BIOS_BANK_IO_BUFFER'],272)==guard;page(6)
 _,r=run(s,'emptyBatch');assert r['af']&1 and r['af']>>8==k['DOS_ERR_ARGUMENT']
 print('PASS system receive-buffer bound: 257-byte batch rejected without copy',flush=True)
 # A 32-byte allocation keeps its guard intact while commit repeatedly flushes.
 run(s,'small');page(6);m.write(0x4020,b'\xa5'*8);run(s,'enqueue');assert m.read(0x4020,8)==b'\xa5'*8;assert int.from_bytes(m.read(s['cqUsed'],2),'little')==20
 run(s,'flush');run(s,'off');print('PASS post-commit reserve and 32-byte buffer guard',flush=True)
 # The same manager supports a larger service-owned protocol, without padding.
 s,body=asm(64,custom=True);page(6);m.write(0x2000,body);m.write(0x4400,b'\x5a'*8);run(s,'init');run(s,'enqueue');assert m.read(s['cqUsed'],2)==W(320);run(s,'flush');assert m.read(s['customBytes'],2)==W(320);assert m.read(0x4400,8)==b'\x5a'*8;run(s,'off')
 print('PASS custom service: 1024-byte buffer, direct generation, exact 320-byte send',flush=True)
 # Load a real MAIN DLL and enter a pinned scope from EXEC. Use FILEHANDLE on B:.
 page(0)
 fileapi=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
 imageapi=runpy.run_path(str(R/'scripts/images/executable.py'))
 module_src=R/'scripts/emu80/fixtures/queue-module.asm'
 subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'leaf.bin'),'--sym='+str(O/'leaf.sym'),str(module_src)],check=True,capture_output=True)
 leaf=sy(O/'leaf.sym')
 image=imageapi['encode'](imageapi['Image']((O/'leaf.bin').read_bytes(),0x6000,0x6000,4),'dll')
 dos=sy(R/'bin/banked/KERNEL.sym')
 def allocated():
  page(1)
  table=m.read(dos['MEM_ALLOC_TABLE'],6*dos['MEM_ALLOC_RECORDS'])
  return [table[i:i+6] for i in range(0,len(table),6) if table[i]!=255]
 baseline=allocated();page(0)
 m.write(0x3000,b'B:/QROUTE.DLL\0'+bytes(114));m.write(0x4800,image)
 m.write(0x3200,W(0x3000)+W(0x4800)+W(len(image))+bytes(34))
 fileapi['path_io'](m,lambda op,hl=0,a=0:fileapi['main_call'](m,k,op,hl,a),k,0x3200,True)
 s,body=asm(4);page(6);m.write(0x2000,body);run(s,'wide')
 for step in ('loadDll','scope'):
  _,r=run(s,step);assert not r['af']&1,(step,r)
 assert m.read(s['scalarResult'],2)==W(0x1235)
 assert m.read(s['pinnedError'],1)==bytes([k['MEM_ERROR_ACTIVE']])
 assert m.read(s['cqError'],1)==bytes(1)
 page(0);assert m.read(leaf['receivedBytes'],3)==W(320)+bytes([77]);page(6)
 regular,r=run(s,'legacyPerf');assert not r['af']&1
 scoped,r=run(s,'scopePerf');assert not r['af']&1
 route_perf=dict(calls=16,module_call=regular,module_scope=scoped,speedup=regular/scoped)
 (O/'route.json').write_text(json.dumps(route_perf,indent=2)+'\n')
 print('PERF DLL',route_perf,flush=True)
 _,r=run(s,'expired');assert r['af']>>8==k['MEM_ERROR_HANDLE'] and r['af']&1
 _,r=run(s,'unload');assert not r['af']&1
 assert sorted(allocated())==sorted(baseline),'DLL-owned FAR allocation leaked'
 run(s,'off')
 print('PASS EXEC -> MAIN scoped DLL, scalar and exact payload, DLL -> DOS -> BIOS target changes, pin and stale route',flush=True)
 # Compile and execute the C SDK writer on the same isolated EXEC fixture.
 z=Path(os.environ.get('SPECIALIST_Z88DK_DIR','D:/Projects/RetroPC/Specialist/MySoft/z88dk'));env=os.environ.copy();env['ZCCCFG']=str(z/'lib/config');env['PATH']=str(z/'bin')+os.pathsep+env['PATH']
 subprocess.run([str(z/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80','-crt0='+str(R/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(R/'sdk/include'),str(R/'sdk/src/mxos_z88dk.asm'),str(R/'sdk/src/mxos_queue.c'),str(R/'sdk/src/mxos_module.c'),str(R/'sdk/tests/command-queue.c'),'-o',str(O/'c-queue.bin'),'-m'],env=env,check=True,capture_output=True)
 code=(O/'c-queue_code_crt_init.bin').read_bytes();assert len(code)<0x3000
 for bank in (6,0):
  page(bank);m.write(0x2000,code);m.stop_at(0x6100);m.call('regs_set',pc=0x2000,sp=0x8fdf);m.until(0x6100,90,page=bank);assert m.read(0x5e80,2)==W(0),m.read(0x5e80,2).hex()
  assert m.read(k['MEM_QUEUE_HOOK'],2)==W(0)
  print('PASS C SDK direct writer, automatic flush, oversize refusal and close; bank',bank,flush=True)
 # Reverse direction through the same public entry. A live scope in MAIN calls EXEC.
 page(0);s,body=asm(4);m.write(0x2000,body)
 image=imageapi['encode'](imageapi['Image']((O/'leaf.bin').read_bytes(),0x6000,0x6000,k['IMAGE_FLAG_BANK']),'dll')
 m.write(0x3000,b'B:/QROUTE.DLL\0'+bytes(114));m.write(0x4800,image)
 m.write(0x3200,W(0x3000)+W(0x4800)+W(len(image))+bytes(34))
 fileapi['path_io'](m,lambda op,hl=0,a=0:fileapi['main_call'](m,k,op,hl,a),k,0x3200,True)
 # FILEHANDLE fixture uses MAIN scratch at 2800h. Refresh the assembled code afterwards.
 m.write(0x2000,body);run(s,'wide',bank=0)
 # EXEC images clone DOS's MEM code. Its already bound flush callback stays local to DOS.
 page(1);m.write(0xed,b'\xc9');m.write(k['MEM_QUEUE_HOOK'],W(0xed));m.write(t['bankQueueBefore'],b'\xc3')
 _,r=run(s,'loadBank',bank=0);assert not r['af']&1,r
 handle=m.read(s['handle'],2)
 page(1);record=m.read(dos['MODULE_TABLE'],dos['MODULE_RECORD_SIZE']*dos['MODULE_MAX'])
 target=next(record[i+3] for i in range(0,len(record),dos['MODULE_RECORD_SIZE']) if record[i:i+2]==handle)
 assert target>=2 and m.read(k['MEM_QUEUE_HOOK'],2)==W(0xed)
 page(target);assert m.read(k['MEM_QUEUE_HOOK'],2)==W(0) and m.read(t['bankQueueBefore'],1)==b'\xc9'
 page(1);m.write(t['bankQueueBefore'],b'\xc9');m.write(k['MEM_QUEUE_HOOK'],W(0))
 print('PASS new EXEC discards inherited DOS queue registration',flush=True)
 for step in ('scope','unload'):
  _,r=run(s,step,bank=0);assert not r['af']&1,(step,r)
 assert m.read(s['scalarResult'],2)==W(0x1235)
 assert m.read(s['cqError'],1)==bytes(1) and m.read(s['pinnedError'],1)==bytes([k['MEM_ERROR_ACTIVE']])
 assert sorted(allocated())==sorted(baseline)
 run(s,'off',bank=0);print('PASS MAIN -> EXEC -> DOS, DLL allocation ownership and unload',flush=True)
 # Only the disposable DLL created on this run's B: is used as a delete target.
 select_b=bytes([k['MXOS_DOS_FILE_SELECT_DRIVE']&255,5,8,0,1])
 remove=bytes([k['MXOS_DOS_FILE_DELETE']&255,14,16])+b'QROUTE  DLL'
 unknown=bytes([0xfe,3,0])
 def dos_batch(records):
  page(0);m.write(0x4000,records)
  return run(s,'dosBatch',bank=0,hl=0x4000,bc=len(records))[1]['af']
 error=dos_batch(select_b+unknown+remove)
 assert error&1 and error>>8==k['DOS_ERR_UNSUPPORTED'],hex(error)
 assert not dos_batch(remove)&1,'failed batch executed its suffix'
 error=dos_batch(remove)
 assert error&1 and error>>8,'second delete must report a file error'
 print('PASS DOS batch: select drive, stop on unsupported opcode, delete inline 8.3, report missing file',flush=True)
 # A Reset abandons a bound controller. Count any accidental call into its old code.
 page(1);m.write(0xeb,b'\x00');m.write(0xed,b'\x21\xeb\x00\x34\xc9')
 m.write(k['MEM_QUEUE_HOOK'],W(0xed));m.write(t['bankQueueBefore'],b'\xc3')
 page(0);m.call('bp_clear');m.stop_at(b['biosTryShell']);m.call('emu_reset');m.until(b['biosTryShell'],120,page=0)
 page(1);assert m.read(0xeb,1)==b'\x00' and m.read(k['MEM_QUEUE_HOOK'],2)==W(0)
 page(0);assert m.read(k['MEM_QUEUE_HOOK'],2)==W(0)
 print('PASS warm Reset discards bound queues without dispatching them',flush=True)
 (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
