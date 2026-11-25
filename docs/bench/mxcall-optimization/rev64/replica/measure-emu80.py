"""Проверить выделенные мосты на реальном банковом маппере Emu80 MCP."""
import json,hashlib,os,re,runpy,struct,subprocess,time
from pathlib import Path
from tempfile import TemporaryDirectory
out=Path(__file__).resolve().parent
root=out.parents[4]
runtime=Path(os.environ['LOCALAPPDATA'])/'Specialist-OS/emu80/19781/replica-hidden'
exe=runtime/'Emu80qt-mcp.exe'
h=runpy.run_path(str(root/'scripts/emu80/benchmark-disks.py'))
sym=runpy.run_path(str(root/'scripts/emu80/benchmark-mxcall.py'))['symbols']
t=sym(root/'bin/banked/TRANSPORT.sym')
b=sym(root/'bin/BIOS.sym')
word=struct.Struct('<H').pack
image=root/'images/C-Flash-512k-MMX-System-Emu80.bin'
m=h['Bench']('http://127.0.0.1:19781/mcp')
def shell(text):
    return subprocess.run(['powershell.exe','-NoProfile','-Command',text],check=True,capture_output=True,text=True,
                          creationflags=subprocess.CREATE_NO_WINDOW).stdout.strip()
def owner():
    return shell("$l=Get-NetTCPConnection -State Listen -LocalPort 19781 -ErrorAction SilentlyContinue; if($l){$l[0].OwningProcess};exit 0")
def page(n):
    m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))
def init_assembly():
    origin=0xfd80
    with TemporaryDirectory() as folder:
        tmp=Path(folder)
        src,bin_,sym_=tmp/'replica.asm',tmp/'replica.bin',tmp/'replica.sym'
        args=["    ORG 0FD80h"]+[f"{n} = 0{t[n]:04X}h" for n in
               ('BANK_IO_MAIN','BANK_IO_RAM','bankPage','bankServiceSP','bios_getCursorPos')]
        args+=[f"cursorAddress = 0{b['bios_vars.cursorY']:04X}h"]
        src.write_text('\n'.join(args)+'\n'+(out/'replica.asm').read_text(encoding='utf-8-sig'),encoding='utf8')
        subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080',f'--raw={bin_}',f'--sym={sym_}',str(src)],
                       check=True,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        s=sym(sym_)
        code=bin_.read_bytes()
    starts=[s[n] for n in ('cursorReplica','cursorReplicaRead','cursorReplicaReadBound')]+[origin+len(code)]
    return dict((name,(start,code[start-origin:end-origin])) for name,start,end in
                zip(('replica','read','read_bound'),starts[:-1],starts[1:],strict=True))
def start():
    assert not owner(),'MCP 19781 already owned'
    os.environ['QT_QPA_PLATFORM']='offscreen'
    launch=root/'scripts/emu80/run.ps1'
    stdout=shell(f"$p=& {h['ps'](launch)} -EmulatorDirectory {h['ps'](runtime)} -Hidden -Flash -ResetFlash -Mcp -PassThru; $p.Id")
    return int(stdout.splitlines()[-1])
def run(bank,target,regs,watch=False):
    page(bank)
    code=b'\xcd'+word(target)
    stub=0x8000
    stop=stub+len(code)
    m.write(stub,code)
    m.stop_at(stop)
    if watch:
        for address in (*range(0xffc0,0xffe0),0xfffc,0xfffd):
            m.call('bp_set',addr=address,type='access',skipCount=1000000)
    m.call('regs_set',pc=stub,sp=0x8650,**regs)
    start=m.call('cpu_state')['cpu_ticks']
    state=m.until(stop,6,page=bank)
    if watch:
        counts={int(p['addr'],16):p['hitCount'] for p in m.call('bp_list')['breakpoints'] if p['type']=='access'}
        assert len(counts)==34 and all(not counts[at] for at in range(0xffc0,0xffe0)),counts
        assert (counts[0xfffc],counts[0xfffd])==((1,1) if bank else (0,0)),counts
    else:
        counts={}
    m.call('bp_clear')
    result={n:state['registers'][n]['dec'] for n in ('af','bc','de','hl','sp','pc')}
    assert result.pop('pc')==stop and result['sp']==0x8650,result
    return state['cpu_ticks']-start,result,counts

assert t['MEM_RUNTIME_REVISION']==64
variants=init_assembly()
rows=[]
pid=start()
try:
    for _ in range(30):
        if owner()==str(pid):
            break
        time.sleep(.1)
    else:
        raise RuntimeError('The owned process did not acquire port 19781')
    who=shell(f"Get-Process -Id {pid} | Select-Object -ExpandProperty Path")
    assert Path(who).resolve()==exe.resolve()
    assert shell(f"(Get-Process -Id {pid}).MainWindowHandle.ToInt64()")=='0'
    info=m.call('sys_info')
    assert info['cpu']['speed_factor']==32 and not info['cpu']['full_throttle'],info['cpu']
    launch=(runtime/'specialist-os/launch.conf').read_text()
    assert 'pitSoundSource.muted = yes' in launch and 'tapeMonitor.muted = yes' in launch
    assert (runtime/'specialist-os/system-flash.bin').read_bytes()==image.read_bytes()
    m.call('debug_break')
    m.stop_at(b['biosBootExec'])
    m.call('emu_reset')
    m.until(b['biosBootExec'],30,page=0)
    print('hidden 32x boot confirmed',flush=True)
    # Действующий MEM, до намеренной порчи участков RAM.
    data=bytes((i*37+11)&255 for i in range(258))
    page(3);m.write(0x3fff,b'\xcc'+data+b'\xdd')
    page(0);m.write(0x8100,struct.pack('<BHBHH',3,0x4000,3,0x4001,257))
    elapsed,result,_=run(0,t['mx_memory'],dict(af=0x0002,bc=0x3456,de=0x5678,hl=0x8100))
    assert not result['af']&1,result
    assert m.read(0xffdf,1)==b'\x02'
    page(3);assert m.read(0x3fff,260)==b'\xcc'+data[:1]+data[:257]+b'\xdd'
    move=dict(ticks=elapsed,profile=2,result=result,bytes=257)
    print('LOCAL MOVE PASS',move,flush=True)

    ff=bytes((x*29+17)&255 for x in range(32))
    cursor=b['bios_vars.cursorY']
    markers={bank:bytes([0xa5,bank,bank^0xff,0x5a]) for bank in (0,2,6,16)}
    for bank,marker in markers.items():
        page(bank)
        m.write(0x5ff0,marker)
    for bank,marker in markers.items():
        page(bank)
        assert m.read(0x5ff0,4)==marker
    for bank in (2,6,16):
        for af,xy,top in ((0x7193,0x1234,0xed80),(0x5546,0x0123,0xed40),(0xa5d7,0x4567,0xed60)):
            regs=dict(af=af,bc=0x3456,de=0x5678,hl=0x9876)
            for name in ('direct','replica','read','read_bound'):
                page(0)
                m.write(t['bankServiceSP'],word(top))
                m.write(cursor,word(xy))
                m.write(0xffc0,ff)
                m.write(top-16,b'\xa5'*20)
                initial=0 if name=='direct' else bank
                if name!='direct':
                    address,blob=variants[name]
                    if name=='read_bound':
                        assert blob[1]==0x3e
                        blob=blob[:2]+bytes([bank-1])+blob[3:]
                    # У каждой стороны есть код только для своего участка траектории.
                    enter=blob.index(bytes.fromhex('32fcff'))+3
                    leave=blob.index(bytes.fromhex('32fdff'))+3
                    halt=b'\x76'
                    m.write(address,halt*enter+blob[enter:leave]+halt*(len(blob)-leave))
                    m.write(0x8000,halt*4)
                    page(bank)
                    m.write(t['bankPage'],bytes([bank]))
                    m.write(t['bankServiceSP'],word(0x6100))
                    m.write(0x60f0,b'\xb6'*20)
                    m.write(cursor,word(xy^0xffff))
                    m.write(b['bios_getCursorPos'],halt*3)
                    m.write(address,blob[:enter]+halt*(leave-enter)+blob[leave:])
                    target=address
                else:
                    target=b['bios_getCursorPos']
                page(initial)
                m.write(0x8640,b'\x5a'*20)
                elapsed,result,counts=run(initial,target,regs,watch=True)
                assert m.read(0x5ff0,4)==markers[initial]
                assert result==dict(regs,hl=xy,sp=0x8650),(name,bank,result)
                assert m.read(0xffc0,32)==ff
                assert m.read(0x8640,10)==b'\x5a'*10 and m.read(0x8650,4)==b'\x5a'*4
                if initial:
                    assert m.read(t['bankPage'],1)==bytes([bank])
                    assert m.read(t['bankServiceSP'],2)==word(0x6100)
                    assert m.read(0x60f0,20)==b'\xb6'*20
                    assert m.read(cursor,2)==word(xy^0xffff)
                page(0)
                stack=m.read(top-16,20)
                if name=='replica':
                    inner_return=address+blob.index(b'\xcd'+word(b['bios_getCursorPos']))+3
                    assert stack==b'\xa5'*14+word(inner_return)+b'\xa5'*4
                else:
                    assert stack==b'\xa5'*20
                assert m.read(t['bankServiceSP'],2)==word(top)
                assert m.read(cursor,2)==word(xy)
                assert elapsed==dict(direct=53,replica=224,read=108,read_bound=97)[name]
                rows.append(dict(case=name,test_bank=bank,caller=initial,af=af,cursor=xy,main_sp=top,ticks=elapsed,result=result,
                                 mapper=[counts[0xfffc],counts[0xfffd]],window_accesses=0))
        print(f'bank {bank} PASS',flush=True)
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    (out/'emu-result.json').write_text(json.dumps(dict(runtime_sha256=sha(exe),system_sha256=sha(image),
        emu80=info['server'],cpu=info['cpu'],hidden=True,qpa='offscreen',port=19781,
        physical_markers={n:v.hex() for n,v in markers.items()},local_move=move,rows=rows),indent=2)+'\n',encoding='utf8')
finally:
    shell(f"$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; if ($p -and $p.Path -eq {h['ps'](exe)}) {{ Stop-Process -InputObject $p; $p.WaitForExit() }}")
