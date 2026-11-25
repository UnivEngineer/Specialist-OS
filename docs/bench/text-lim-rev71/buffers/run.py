"""Far256 и две опытные ветки TEXT в частном скрытом Emu80 на одном образе rev71."""
import argparse
import hashlib
import json
import runpy
import shutil
import struct
import subprocess
import tempfile
import time
from pathlib import Path

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[3]
lib = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-mem-bank.py'))
symbols, shell, ps, word = (lib[n] for n in ('symbols','shell','ps','WORD'))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', required=True, type=Path)
parser.add_argument('--port', default=19782, type=int)
parser.add_argument('--counted-extension',action='store_true',help='Опытный обычный LIM через общий буфер')
parser.add_argument('--raw-z',action='store_true',help='Отдельная серия штатного RawZ')
args = parser.parse_args()
runtime = args.runtime.resolve()
image = ROOT/'images'/lib['SYSTEM']
digest = lib['digest'](image)
assert digest == 'b87ba3033b0dbc6cdd4fce4d0ecb9317c0e1d12f986190ec64f70c550c25dcee'
b,k,t = (symbols(ROOT/'bin'/name) for name in ('BIOS.sym','banked/LOCALBLOCK.sym','banked/TRANSPORT.sym'))
defs = dict(cursorX=b['bios_vars.cursorX'], printWindow=b['bankText.printWindow'],
            operation=k['memOperation'], printLoop=k['memPrintLoop'], doneHL=k['memPrintHaveHL'],
            callRegs=k['mx_callRegs'], textPrint=k['MXOS_BIOS_TEXT_PRINT'])
with tempfile.TemporaryDirectory() as tmp:
    temp = Path(tmp)
    asm = temp/'bench.asm'
    asm.write_text(''.join(f'{name} = {value}\n' for name,value in defs.items())
                   + f' INCLUDE "{(OUT/"probe.asm").as_posix()}"\n',encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw=probe.bin','--sym=probe.sym',asm.name],
                   cwd=temp,check=True)
    probe=(temp/'probe.bin').read_bytes()
    p=symbols(temp/'probe.sym')

def next_quota(x):
    # Прогноз для печатных 6x8 с реального начального X. Управляющие коды не сканируем.
    choices=[]
    for n in range(1,33):
        if x>=190: x=0
        x+=3
        if x>=192: x=0
        if n>=29 and x%4==0: choices.append(n)
    return max(choices,default=32)

def owner():
    return shell(f'$l=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($l){{$l[0].OwningProcess}}; exit 0')

assert not owner()
lib['hidden_qt'](runtime)
shutil.copyfile(image,runtime/'specialist-os/system-flash.bin')
exe=runtime/'Emu80qt-mcp.exe'
pid=int(shell(f'$p=& {ps(ROOT/"scripts/emu80/run.ps1")} -EmulatorDirectory {ps(runtime)} -Hidden -Flash -Mcp -PassThru; $p.Id').splitlines()[-1])
try:
    for _ in range(30):
        if owner()==str(pid): break
        time.sleep(.1)
    assert owner()==str(pid) and Path(shell(f'(Get-Process -Id {pid}).Path')).resolve()==exe
    assert shell(f'(Get-Process -Id {pid}).MainWindowHandle.ToInt64()')=='0'
    Bench=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['Bench']
    m=Bench(f'http://127.0.0.1:{args.port}/mcp')
    cpu=m.call('sys_info')['cpu']
    assert cpu['speed_factor']==32 and not cpu['full_throttle']
    m.call('debug_break')
    m.stop_at(b['biosBootExec'])
    m.call('emu_reset')
    m.until(b['biosBootExec'],30)

    def page(n):
        m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))

    def patch(bank,address,old,new):
        page(bank)
        assert len(old)==len(new) and m.read(address,len(old))==old,hex(address)
        m.write(address,new)

    def site(bank,start,end,pattern):
        page(bank)
        code=m.read(start,end-start)
        assert code.count(pattern)==1,(hex(start),pattern.hex())
        return start+code.index(pattern)

    mem=(ROOT/'bin/MEM.sys').read_bytes()
    page(6)
    for i in range(mem[5]):
        addr,n,offset=struct.unpack_from('<HHH',mem,6+6*i)
        if addr!=0xffc0: m.write(addr,mem[offset:offset+n])
    for field,data in [('bankPage',b'\x06'),('bankOwner',b'\x0d\x06'),('bankDepth',b'\0'),
                       ('bankPageCount',b'\x10'),('bankServiceSP',word(t['DOS_BANK_STACK_TOP'])),
                       ('bankStackFloor',word(t['DOS_BANK_STACK_BOTTOM']+64))]:
        m.write(t[field],data)
    m.write(k['MEM_BANK_KIND'],bytes([k['MEM_BANK_EXEC']]))
    m.write(k['MEM_EXEC_READY'],bytes([k['MEM_EXEC_READY_VALUE']]))
    m.write(k['MEM_NEAR_COPY'],word(k['memExecCopy']))
    for bank in (0,6):
        page(bank)
        m.write(p['aligned'],probe)
    page(0)
    m.write(p['quota'],bytes(map(next_quota,range(256))))
    rotate=symbols(ROOT/'bin/fonts/Rotate.sym')
    assert m.read(b['FONT_ENGINE_TEXT_LIM'],3)==b'\xc3'+word(rotate['printStringLim'])
    initial_esc=m.read(b['bios_upper.escMode'],1)
    initial_inverse=m.read(b['bios_vars.inverse'],2)
    call_print=b'\xcd'+word(defs['printWindow'])
    advance=bytes.fromhex('06 00 09 79 fe 20')
    loop=b'\xc3'+word(defs['printLoop'])
    start=k['memPrintTyped']
    page(6)
    original=m.read(start,3)
    assert original==bytes.fromhex('c5 d5 3a')
    changes={
        'aligned':[(0,site(0,b['biosMemPrintMain'],b['memPrintBatch'],call_print),
                    call_print,b'\xcd'+word(p['aligned'])),
                   (6,site(6,k['memPrintReturn'],k['memPrintShort'],advance),
                    advance,bytes.fromhex('78 06 00 09 fe 20'))],
        'after64':[(6,start,original,b'\xc3'+word(p['hybridStart'])),
                   (6,site(6,k['memPrintReturn'],k['memPrintShort'],loop),
                    loop,b'\xc3'+word(p['afterWindow']))]}
    if args.counted_extension:
        raw_call=b'\xcc'+word(b['bios_printStringRawLim'])
        target=site(0,b['bankText.print'],b['bankText.done'],raw_call)
        patch_render=(0,target,raw_call,b'\xcc'+word(b['bios_printStringLim']))
        bank,addr,old,_=changes['after64'][1]
        changes['after64']=[changes['after64'][0],(bank,addr,old,b'\xc3'+word(p['afterWindowLim'])),patch_render]
        changes['far256']=[patch_render]

    source,request,stub,sp=0x7000,0x8200,0x8100,0x8650
    def measure(route,kind,payload,x):
        n=len(payload)
        for bank,addr,old,new in changes.get(route,[]): patch(bank,addr,old,new)
        try:
            page(0)
            m.write(b['bios_vars.cursorY'],bytes([10,x]))
            m.write(b['bios_upper.escMode'],initial_esc)
            m.write(b['bios_vars.inverse'],initial_inverse)
            m.write(0x9000,bytes(0x3000))
            caller=0 if route=='main' else 6
            page(caller)
            m.write(source,payload+b'\0POISON')
            name,entry={'z':('Z','bios_printStringZ'),'raw-z':('RAW_Z','bios_printStringRawZ'),'raw':('RAW_LIM','bios_printStringRawLim'),
                        'lim':('LIM','bios_printStringLim')}[kind]
            ident=k['MXOS_BIOS_PRINT_STRING_'+name]
            hl=source
            if route=='far256':
                ident=k['MXOS_BIOS_TEXT_PRINT']
                style={'z':0,'raw-z':1,'raw':2,'lim':2}[kind]
                m.write(request,struct.pack('<BB4H',style,1,source,n+(style<2),0,0))
                hl=request
            code=b'\xcd'+word(b[entry] if caller==0 else k['mx_callRegs'])
            if caller: code+=word(ident)
            m.write(stub,code)
            end=stub+len(code)
            m.stop_at(end)
            m.call('regs_set',pc=stub,sp=sp,af=0x5a92,bc=n,de=0x1234,hl=hl)
            before=m.call('cpu_state')['cpu_ticks']
            state=m.until(end,10)
            ticks=state['cpu_ticks']-before
            regs={key:state['registers'][key]['dec'] for key in ('hl','af','de','bc','sp')}
            assert regs['hl']==source+n and regs['sp']==sp and not regs['af']&1,(route,kind,n,regs)
            if route!='far256':
                assert (regs['bc'],regs['de'],regs['af'])==(n,0x1234,0x5a92)
            assert m.read(t['bankPage'],1)==bytes([caller]) and m.read(source,n+7)==payload+b'\0POISON'
            page(0)
            screen=m.read(0x9000,0x3000)+m.read(b['bios_vars.cursorY'],2)
            screen+=m.read(b['bios_upper.escMode'],1)+m.read(b['bios_vars.inverse'],2)
            return ticks,hashlib.sha256(screen).hexdigest()
        finally:
            for bank,addr,old,new in reversed(changes.get(route,[])): patch(bank,addr,new,old)

    rows=[]
    for kind,controls,lengths in (
            ('z',False,(32,63,64,65,96,127,128,160,192,224,255,256,257,384,511)),
            ('raw',False,(64,96,128,192,255,256,257,384,511)),
            ('raw-z',False,(64,96,128,192,255,256,257,511)),
            ('lim',False,(64,96,128,192,256,384)),
            ('z',True,(64,128,256)),('lim',True,(64,128,256))):
        if args.counted_extension and kind!='lim': continue
        if (kind=='raw-z')!=args.raw_z: continue
        for n in lengths:
            payload=bytearray((b'Abc123! '*((n+7)//8))[:n])
            if controls:
                for offset in range(23,n,37): payload[offset]=13
                if n>32: payload[31:33]=b'\x1b)'
                if args.counted_extension: payload[17]=0
            for x in (16,0):
                reference=None
                for route in ('main','win32','far256','after64','aligned'):
                    if kind=='lim' and route in ('far256','after64') and not args.counted_extension: continue
                    ticks,screen=measure(route,kind,bytes(payload),x)
                    if reference is None: reference=screen
                    assert screen==reference,(kind,n,x,route)
                    rows.append(dict(kind=kind,controls=controls,length=n,x=x,route=route,ticks=ticks,screen=screen))
            print(kind,controls,n,'PASS',flush=True)
    result_name='counted-prototype.json' if args.counted_extension else 'raw-z.json' if args.raw_z else 'result.json'
    (OUT/result_name).write_text(json.dumps(dict(image_sha256=digest,emu_sha256=lib['digest'](exe),
        cpu=cpu,hidden=True,rows=rows),indent=2)+'\n',encoding='utf-8')
finally:
    shell(f'$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; if($p -and $p.Path -eq {ps(exe)}) '
          '{Stop-Process -InputObject $p; $p.WaitForExit()}; exit 0')
