"""8080: итоговый битмап/цвет каждого байта тулбара без промежуточного стирания."""
from pathlib import Path
import json,re,struct,subprocess

R=Path(__file__).resolve().parents[2]
O=R/'docs/nc/toolbar-2026-11-05/cpu';O.mkdir(parents=True,exist_ok=True)
def symbols(name):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
s=symbols('DOS.sym');d=symbols('BIOS.sym');n=symbols('NC.sym');W=lambda x:struct.pack('<H',x)
# В тестовой памяти единственная экранная запись заменена на RST 5. Обвязка
# выполняет её, записывает адрес/байт/цвет и сохраняет регистры и флаги.
asm=O/'trace.asm';asm.write_text(''' ORG 05000h
 ld (hl),a
 push af
 push bc
 push de
 push hl
 ld b,a
 ld d,h
 ld e,l
 ld hl,(05100h)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld (hl),b
 inc hl
 ld a,(0FFF8h)
 ld (hl),a
 inc hl
 ld (05100h),hl
 pop hl
 pop de
 pop bc
 pop af
 ret
''',encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'trace.bin'),str(asm)],check=True,capture_output=True)
m=bytearray(65536)
for path,at in [(R/'bin/NC.i80',0),(R/'bin/BIOS.sys',0xC000),(R/'bin/IO.sys',0xF000),
                (R/'bin/DOS.native.sys',s['DOS_CODE_START']),
                (R/'assets/system-rom/FONTKOI.FNT',s['FONT_ADDR']),
                (O/'trace.bin',0x5000)]:
    b=path.read_bytes()
    m[at:at+len(b)]=b
m[s['v_dosModuleReady']]=1
assert m[d['uiToolbarWrite']]==0x77
m[d['uiToolbarWrite']]=0xEF;m[0x28:0x2B]=b'\xC3'+W(0x5000)
m[0x9000:0xC000]=bytes((i*37+(i>>8)*13)&255 for i in range(0x3000))
m[s['bios_vars.cursorY']:s['bios_vars.cursorY']+2]=W(0x3543)
m[s['bios_vars.inverse']]=0xA5
font=(R/'assets/system-rom/FONT.FNT').read_bytes()

def expected(at):
    result={};column=0
    while m[at]:
        flags=m[at];at+=1;width=flags&7;inverse=bool(flags&0x80)
        end=m.index(0,at);text=m[at:end];at=end+1
        assert 1<=width<=4 and len(text)*6<=width*8
        pixels=[[inverse]*(8*width) for _ in range(9)]
        origin=(8*width-6*len(text))//2
        for i,char in enumerate(text):
            for y,row in enumerate(font[char*8:char*8+8]):
                for x in range(5):
                    if row&(16>>x):pixels[y+1][origin+i*6+x]=not inverse
        for x in range(width):
            for y in range(9):
                value=sum(128>>bit for bit in range(8) if pixels[y][x*8+bit])
                result[0x9000+256*(column+x)+247+y]=(value,n['COLOR_HELP_TEXT'] if inverse else n['COLOR_HELP_F'])
        column+=width
    assert column==48
    return result

results=[]
for name in ['aF1LeftF2RighF3','aShiftToolbar','aF1LeftF2RighF3']:
    before=bytes(m);want=expected(n[name]);m[0x5100:0x5102]=W(0x2000)
    code=b'\x31\x00\x8F\x21'+W(n[name])+b'\x01'+W((n['COLOR_HELP_F']<<8)|n['COLOR_HELP_TEXT'])
    code+=b'\xCD'+W(s['bios_uiToolbar'])+b'\xC3'+W(0x6100)
    m[0x6000:0x6000+len(code)]=code;(O/'in.bin').write_bytes(m)
    run=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end','6100','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,text=True)
    out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x6100
    m=bytearray(out[:65536]);end=int.from_bytes(m[0x5100:0x5102],'little')
    records=list(struct.iter_unpack('<HBB',m[0x2000:end]))
    assert len(records)==432 and len({r[0] for r in records})==432
    for addr,value,color in records:assert want[addr]==(value,color),(name,hex(addr),want[addr],(value,color))
    for addr in range(0x9000,0xC000):
        assert m[addr]==(want[addr][0] if addr in want else before[addr]),(name,hex(addr))
    assert m[s['bios_vars.cursorY']:s['bios_vars.cursorY']+2]==W(0x3543)
    assert m[s['bios_vars.inverse']]==0xA5
    results.append(dict(layout=name,writes=len(records),uniqueAddresses=True,finalPixelsAndColors=True,modelLog=run.stdout.strip()))
(O/'result.json').write_text(json.dumps(results,indent=2)+'\n')
print('Toolbar CPU PASS: normal -> Shift -> normal; 432 final writes per frame; height/colors/BIOS state')
