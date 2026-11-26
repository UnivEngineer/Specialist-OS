"""8080: регистровый диспетчер, вложенные вызовы и BDOS; реальный BIOS/DOS."""
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/banked/syscall-main-001';OUT.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M)}
W=lambda n:n.to_bytes(2,'little')
base=bytearray(65536)
for name,addr in [('BIOS.sys',0xC000),('IO.main.bin',S['BIOS_IO_ADDR']),('DOS.native.sys',S['dosCoreStart'])]:
    data=(ROOT/'bin'/name).read_bytes();base[addr:addr+len(data)]=data
base[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']]=base[S['initVars']:S['initVarsEnd']]
font=(ROOT/'bin/fonts/Compact.pf').read_bytes()
base[S['FONT_ADDR']:S['FONT_ADDR']+len(font)]=font
__import__('runpy').run_path(str(Path(__file__).with_name('mem-image.py')))['install'](base, S)
base[S['v_dosModuleReady']]=1
rows=[]

def run(body,memory=None):
    m=bytearray(base if memory is None else memory)
    src=OUT/'case.asm'
    src.write_text(f' INCLUDE "{(ROOT/"source/common/system.inc").as_posix()}"\n ORG 02000h\n ld sp,STACK_ADDR\n'+body+'''
 push af
 push bc
 push de
 push hl
 ld hl,0
 add hl,sp
 ld de,03B00h
 ld b,8
save:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec b
 jp nz,save
 ld hl,0
 add hl,sp
 ld (03B08h),hl
 jp 03C00h
''',encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(OUT/'case.bin'),str(src)],check=True,capture_output=True)
    code=(OUT/'case.bin').read_bytes();m[0x2000:0x2000+len(code)]=code
    (OUT/'input.bin').write_bytes(m)
    r=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','2000','-end','3C00','-counter','2000000','-output',str(OUT/'output.bin'),str(OUT/'input.bin')],check=True,capture_output=True)
    result=(OUT/'output.bin').read_bytes()
    assert int.from_bytes(result[65542:65544],'little')==0x3C00,'Не вернулись из диспетчера'
    assert int.from_bytes(result[0x3B08:0x3B0A],'little')==S['STACK_ADDR']-8,'Повреждён SP'
    return result[:65536],int(r.stdout.strip())

registers=''' ld hl,01234h
 ld de,05678h
 ld bc,09ABCh
 ld a,0DEh
 scf
'''
# Подставной исполнитель возвращает все входные регистры/флаги без изменений.
for group in json.loads((ROOT/'source/bios/syscalls.json').read_text()):
    for svc in group['services']:
        m=bytearray(base);at=S['sys'+group['name']+'Table']+svc['id']*2
        m[at:at+2]=W(0x3100);m[0x3100]=0xC9
        op='MXOS_'+group['name']+'_'+svc['name']
        result,cycles=run(registers+' MXOS_CALL_REGS '+op,m)
        assert result[0x3B00:0x3B06]==bytes.fromhex('34 12 78 56 BC 9A'),op
        assert result[0x3B07]==0xDE and result[0x3B06]&1,op
        rows.append(dict(case=op,cycles=cycles))

# Retired file-data operation IDs are holes, not compatibility aliases.
for op in (0x202,0x203,0x207,0x20b,0x20d,0x20e,0x20f,0x21f,0x220):
    result,_=run(f' MXOS_CALL_REGS {op}')
    assert result[0x3b07]==S['DOS_ERR_UNSUPPORTED'] and result[0x3b06]&1

# Реальные функции: прямой вход и новый адаптер имеют одинаковые результаты.
for legacy,op,setup in [
    ('bios_upperCase','MXOS_BIOS_UPPERCASE',' ld a,061h'),
    ('dosGetError','MXOS_DOS_GET_ERROR',registers),
]:
    direct,dc=run(setup+f'\n call 0{S[legacy]:04X}h')
    native,nc=run(setup+'\n MXOS_CALL_REGS '+op)
    assert direct[0x3B00:0x3B08]==native[0x3B00:0x3B08],legacy
    rows.append(dict(case=legacy,directCycles=dc,nativeCycles=nc,overhead=nc-dc))

# Вложенность: исполнитель вызывает другую службу через тот же вход.
m=bytearray(base);at=S['sysBIOSTable'];m[at:at+2]=W(0x3100)
# Служба 0: MVI A,'a' / CALL mx_callRegs / DW uppercase / RET.
m[0x3100:0x3109]=bytes([0x3E,0x61,0xCD])+W(S['mx_callRegs'])+W(S['MXOS_BIOS_UPPERCASE'])+b'\xC9\0'
r,_=run(registers+' MXOS_CALL_REGS 0100h',m)
assert r[0x3B07]==0x41

# Первый номер за таблицей, FFh и неизвестные группы через оба публичных входа.
rejected_ids = [0x100 + S['sysBIOSCount'], 0x1ff, 0x200 + S['sysDOSCount'], 0x2ff,
                0x300, 0xff00]
for op in rejected_ids:
    m = bytearray(base)
    m[0x3000:0x3008] = bytes.fromhex('21 43 65 87 ba dc fe 19')
    for call in (f' ld bc,{op}\n ld de,03000h\n call mx_callPacket', f' call mx_callRegs\n dw {op}'):
        r,_=run(call,m)
        assert r[0x3B07]==S['DOS_ERR_UNSUPPORTED'] and r[0x3B06]&1, (op,call)
        assert r[0x3000:0x3008]==m[0x3000:0x3008], (op,call)

m=bytearray(base);m[S['v_dosModuleReady']]=0
r,_=run(registers+' MXOS_CALL_REGS MXOS_DOS_GET_ERROR',m)
assert r[0x3B07]==S['DOS_ERR_UNSUPPORTED'] and r[0x3B06]&1
r,_=run(' ld a,061h\n MXOS_CALL_REGS MXOS_BIOS_UPPERCASE',m)
assert r[0x3B07]==0x41,'BIOS не должен требовать DOS'

# Преждевременный адаптер CP/M удалён: вход и группа зарезервированы.
for call in (' ld bc,0A50Ch\n call mx_callCPM', ' ld bc,0Ch\n call mx_callPacket'):
 r,_=run(call)
 assert r[0x3B07]==S['DOS_ERR_UNSUPPORTED'] and r[0x3B06]&1
(OUT/'result.json').write_text(json.dumps(dict(registerCases=rows,nested=True,bdosUnsupported=True),indent=2)+'\n')
print(f'System dispatcher PASS: {len(rows)} register cases, nested call, unavailable DOS, BDOS rejected')
