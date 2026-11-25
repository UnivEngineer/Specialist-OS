"""Цена настоящего BIOS-опроса и блокирующего GETCH при удержании стрелки."""
import argparse
import json
import runpy
import struct
from pathlib import Path

root = Path(__file__).resolve().parents[4]
out = Path(__file__).resolve().parent
word = struct.Struct('<H').pack
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('version', choices=('before','after'))
parser.add_argument('--artifacts', required=True, type=Path)
parser.add_argument('--runtime', required=True, type=Path)
parser.add_argument('--port', required=True, type=int)
args = parser.parse_args()
build = args.artifacts.resolve()
report_dir = out / f'poll-{args.version}'
report_dir.mkdir(exist_ok=True)
bench = runpy.run_path(str(root/'scripts/emu80/benchmark-disks.py'))
sym = runpy.run_path(str(root/'scripts/emu80/benchmark-mxcall.py'))['symbols']
code = build/'bin/banked'
t = sym(code/'TRANSPORT.sym')
cl = sym(code/'CLIENT.sym')
bios = sym(build/'bin/BIOS.sym')
env = bench['Matrix'].__init__.__globals__
env.update(ROOT=build, RUNTIME=args.runtime,OUT=report_dir,
           Bench=runpy.run_path(str(build/'source-apps/diskbench/check-emulator.py'))['Bench'])
env['mcp_owner'] = lambda: bench['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
matrix = bench['Matrix']()
m = matrix.m
m.url = f'http://127.0.0.1:{args.port}/mcp'
stub, top = 0x8000,0x8600
def page(n):
    m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))
def ticks():
    return m.call('cpu_state')['cpu_ticks']
def svc(op):
    return b'\xcd'+word(cl['mx_callRegs'])+word(cl['MXOS_BIOS_'+op])
def call(n, instructions):
    page(n)
    m.write(stub,instructions)
    m.stop_at(stub+len(instructions))
    m.call('regs_set',pc=stub,sp=top,bc=0x3456,de=0x2345,hl=0x8060,af=0x7193)
    start=ticks()
    state=m.until(stub+len(instructions),20,page=n)
    assert state['registers']['sp']['dec']==top
    return dict(ticks=state['cpu_ticks']-start,a=state['registers']['af']['dec']>>8)
def press(action):
    m.call('kbd_send',keys=['down'],action=action)
result = {}
try:
    matrix.start('poll-'+args.version,system=build/'images/C-Flash-512k-MMX-System-Emu80.bin')
    m.call('debug_break')
    result['cpu']=m.call('sys_info')['cpu']
    assert result['cpu']['speed_factor']==32 and not result['cpu']['full_throttle']
    page(0)
    m.write(bios['inputHook'],word(0))
    page(6)
    for addr,name in [(cl['IO_BANK_KEYS'],'KEYBANK'),(cl['MEM_IMAGE_LOCAL_START'],'LOCALBLOCK'),(cl['mx_call'],'CLIENT'),(t['BANK_LANDING'],'TRANSPORT')]:
        m.write(addr,(code/(name+'.bin')).read_bytes())
    m.write(t['bankPage'],bytes([6]))
    m.write(t['bankDepth'],b'\0')
    m.write(t['bankServiceSP'],word(cl['DOS_BANK_STACK_TOP']))
    m.write(t['bankStackFloor'],word(cl['DOS_BANK_STACK_BOTTOM']+64))
    m.write(t['BANK_WINDOW'],(code/'WINDOW.bin').read_bytes())
    # Исключаем очередь приложения из синтетического сравнения входов.
    call(6,b'\x21\0\0\xcd'+word(cl['mx_queueBind']))
    result['raw']={}
    for key_action in ('release','press'):
        press(key_action)
        values={}
        for name,n,ins in [('F806_KEY_CHECK',6,svc('KEY_CHECK')),('F806_KEY_SCAN',6,svc('KEY_SCAN')),
                           ('MAIN_keyScan',0,b'\xcd'+word(bios['bios_keyScan'])),
                           ('EXEC_io_bankKeyScan',6,b'\xcd'+word(cl['io_bankKeyScan']))]:
            values[name]=[call(n,ins) for _ in range(3)]
        assert values['F806_KEY_CHECK'][0]['a']==values['MAIN_keyScan'][0]['a']==values['F806_KEY_SCAN'][0]['a']
        assert (values['F806_KEY_SCAN'][0]['a']==255)==(key_action=='release')
        result['raw'][key_action]=values
    press('release')
    # Каждый вариант исполняет шесть подряд CALL при одном удержании.
    result['getch']={}
    for name,n,one in [('EXEC_F806_GETCH',6,svc('GETCH')),('MAIN_GETCH',0,b'\xcd'+word(bios['bios_getch']))]:
        page(0)
        m.write(bios['bios_vars.lastLastKey'],b'\xff\xff')
        page(n)
        m.write(stub,one*6)
        m.stop_at(bios['loc_C23C'])
        m.call('regs_set',pc=stub,sp=top)
        m.until(bios['loc_C23C'],20,page=0)
        press('press')
        try:
            previous=ticks()
            samples=[]
            for i in range(1,7):
                stop=stub+i*len(one)
                m.stop_at(stop)
                state=m.until(stop,25,page=n)
                assert state['registers']['af']['dec']>>8==26 and state['registers']['sp']['dec']==top
                samples.append(state['cpu_ticks']-previous)
                previous=state['cpu_ticks']
            result['getch'][name]=samples
        finally:
            press('release')
    (report_dir/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result,flush=True)
finally:
    if matrix.pid:
        try: press('release')
        finally: matrix.stop()
