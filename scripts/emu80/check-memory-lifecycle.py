"""Emu80: фиксированный резидент B:, владельцы MAIN, FILL и C-куча в двух банках."""
from pathlib import Path
import re
import runpy
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bin/tests/memory-live'
OUT.mkdir(parents=True, exist_ok=True)
subprocess.run(['python', str(ROOT / 'scripts/tests/build-memory-runtime.py')], check=True)
def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)', path.read_text('utf-8'), re.M)}
k = symbols(ROOT / 'bin/banked/KERNEL.sym')
n = symbols(ROOT / 'bin/NC.sym')
h = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=OUT, RUNTIME=ROOT / '.local/monitor2-001')
x = h['Matrix']()
m = x.m
e = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
word = lambda v: struct.pack('<H', v)

def page(bank):
    m.write(0xfffd if bank else 0xfffc, bytes([bank-1 if bank else 0]))

def call(op, hl=0, de=0, bc=0, af=0, error=None):
    page(0)
    m.write(0x3000, struct.pack('<4H', hl, de, bc, af))
    m.write(0x2000, bytes.fromhex('cd00f8c30070'))
    m.stop_at(0x7000)
    m.call('regs_set', pc=0x2000, sp=0x8fdf, bc=op, de=0x3000)
    z=m.until(0x7000, 50)
    r={key: value['dec'] for key,value in z['registers'].items()}
    assert r['sp']==0x8fdf, r
    assert bool(r['af']&1)==(error is not None), (hex(op),r)
    if error is not None: assert r['af']>>8==error, r
    return r

def snapshot():
    page(1)
    data=m.read(k['MEM_ALLOC_TABLE'],32*6)
    page(0)
    return [(data[i],int.from_bytes(data[i+1:i+3],'little'),int.from_bytes(data[i+3:i+5],'little'),data[i+5]) for i in range(0,len(data),6) if data[i]!=255]

def save(name,data):
    page(0)
    m.write(0x3400,('B:\\'+name+'\0').encode()+bytes(100))
    m.write(0x4000,data)
    m.write(0x3200,word(0x3400)+word(0x4000)+word(len(data))+bytes(34))
    call(k['MXOS_DOS_FILE_SAVE_PATH'],hl=0x3200)

try:
    x.start('Memory lifecycle / DOS-owned RAM disk')
    m.call('debug_break');m.call('bp_clear')
    status=call(k['MXOS_DOS_MEMORY'],bc=0,af=k['MEM_OP_RAMDISK']<<8)
    assert status['bc']==0x401,status
    baseline=snapshot(); assert len(baseline)==0,baseline
    call(k['MXOS_DOS_MEMORY'],bc=8<<8,af=k['MEM_OP_RAMLIMIT']<<8,error=k['MEM_ERROR_OWNER'])
    save('KEEPMEM.TXT',b'Resident RAM disk survives warm reset\r\n')
    page(2);disk=m.read(0,4096);page(0)
    m.call('bp_clear');m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120)
    assert snapshot()==baseline
    status=call(k['MXOS_DOS_MEMORY'],bc=0,af=k['MEM_OP_RAMDISK']<<8);assert status['bc']==0x401,status
    page(2);assert m.read(0,4096)==disk;page(0)
    print('PASS: DOS-owned bank pool and B: survive Reset without a resident',flush=True)
    # Дальний блок и заполнение: границы проверяются до первой записи.
    p=call(k['MXOS_DOS_MEMORY'],hl=513,bc=0xff00,af=k['MEM_OP_ALLOC']<<8)
    bank=p['bc']>>8;address=p['hl']
    call(k['MXOS_DOS_MEMORY'],hl=address,de=513,bc=(bank<<8)|0xa5,af=k['MEM_OP_FILL']<<8)
    page(bank);assert m.read(address,513)==bytes([0xa5])*513;guard=m.read(0xfefe,2);page(0)
    call(k['MXOS_DOS_MEMORY'],hl=0xfefe,de=3,bc=(bank<<8)|0x99,af=k['MEM_OP_FILL']<<8,error=k['MEM_ERROR_RANGE'])
    page(bank);assert m.read(0xfefe,2)==guard;page(0)
    q=call(k['MXOS_DOS_MEMORY'],bc=bank<<8,af=k['MEM_OP_QUERY']<<8)
    assert q['hl']==0xff00-514 and q['de']==q['hl'],q
    call(k['MXOS_DOS_MEMORY'],hl=address,bc=bank<<8,af=k['MEM_OP_FREE']<<8)
    print('PASS: remote FILL, atomic range error and QUERY',flush=True)
    # Приложение намеренно оставляет MAIN-блок: завершение обязано его убрать.
    asm=f' INCLUDE "{(ROOT/"source/include/mxos.inc").as_posix()}"\n ORG 00100h\n'+'''
    ld hl,06000h
    ld de,01000h
    ld b,0
    ld a,MEM_OP_ARENA
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld hl,64
    ld b,0
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret
 END
'''
    (OUT/'leak.asm').write_text(asm,encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={OUT/"leak.bin"}',str(OUT/'leak.asm')],check=True,capture_output=True)
    save('LEAK.COM',e['encode'](e['Image']((OUT/'leak.bin').read_bytes(),0x100,0x100,4),'com'))
    for _ in range(2):
        page(0);m.write(0x3400,b'B:\\LEAK.COM\0'+bytes(110));call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400)
        assert snapshot()==baseline
    print('PASS: MAIN RET releases owned blocks; repeated arena registration',flush=True)
    for name in ('CMAIN','CBANK'):
        save(name+'.COM',(ROOT/f'bin/tests/near-heap/{name}.COM').read_bytes())
    maps=(ROOT/'bin/tests/near-heap/runtime.map').read_text('utf-8')
    result_address=int(re.search(r'^_runtime_result\s*= \$([\dA-Fa-f]+)',maps,re.M)[1],16)
    for name in ('CMAIN','CBANK'):
        if name=='CMAIN':
            page(0);m.write(0x3400,b'B:\\CMAIN.COM\0'+bytes(110))
            call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400)
            assert m.read(result_address,2)==word(0x55aa)
        else:
            page(0);m.write(0x3400,b'B:\\CBANK.COM\0'+bytes(110))
            mod=call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,af=0x100)['hl']
            page(0);m.write(0x3300,bytes(8))
            call(k['MXOS_DOS_MODULE_CALL'],hl=mod,de=0x3300)
            assert m.read(0x3300,2)==word(0),m.read(0x3300,8).hex()
            call(k['MXOS_DOS_MODULE_UNLOAD'],hl=mod)
        assert snapshot()==baseline
        print('PASS: C startup, BSS, NEAR stdlib, FAR calloc and console in '+name,flush=True)
finally:
    x.stop()
