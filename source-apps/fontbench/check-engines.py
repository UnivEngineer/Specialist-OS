"""8080 oracle for DLL renderers behind the five public FontBen hooks."""
from pathlib import Path
import re, subprocess, json, sys

R=Path(__file__).resolve().parents[2]
O=R/'docs/bench/engine-grid/cpu';O.mkdir(parents=True,exist_ok=True)
symbols=lambda name:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/f'bin/{name}.sym').read_text(encoding='utf-8'),re.M)}
s=symbols('FONTCOM');bs=symbols('BIOS')
w=lambda n:(n&65535).to_bytes(2,'little')
font=(R/'assets/system-rom/FONT.FNT').read_bytes()
f4=(R/'bin/FONTBEN.font4.bin').read_bytes()
font4=bytes((f4[(c//2)*8+y]>>(0 if c&1 else 4))&15 for c in range(256) for y in range(8))
base=bytearray(65536)
for name,at,ext in [('BIOS',0xC000,'sys'),('IO',0xF000,'sys'),('FONTCOM',0,'i80')]:
    data=(R/f'bin/{name}.{ext}').read_bytes();base[at:at+len(data)]=data
base[bs['bios_vars']:bs['bios_vars']+bs['BIOS_VARIABLES']]=base[bs['initVars']:bs['initVarsEnd']]
base[bs['bios_upper.charGen']:bs['bios_upper.charGen']+9]=base[bs['initTextVars']:bs['initTextVarsEnd']]
base[bs['FONT_ADDR']:bs['FONT_ADDR']+2048]=bytes(font[c*8+y] for y in range(8) for c in range(256))
base[bs['bios_upper.escMode']]=255
base[bs['bios_upper.koi7']]=0
base[bs['bios_vars.inverse']]=0
base[bs['bios_upper.charGen']:bs['bios_upper.charGen']+2]=w(bs['FONT_ADDR'])
TICKS='D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
def run(mem,entry,af=0xA597,bc=0x1357,hl=0x8600,de=0x2468):
    m=bytearray(mem)
    code=b'\x31'+w(0x8F00)+b'\x21'+w(af)+b'\xe5\xf1\x01'+w(bc)+b'\x11'+w(de)+b'\x21'+w(hl)+b'\xcd'+w(entry)+b'\xc3'+w(0x8B80)
    m[0x8000:0x8000+len(code)]=code
    (O/'input.bin').write_bytes(m)
    r=subprocess.run([TICKS,'-m8080','-pc','8000','-end','8b80','-counter','30000000','-output',str(O/'output.bin'),str(O/'input.bin')],capture_output=True,check=True,timeout=20)
    out=(O/'output.bin').read_bytes()
    assert out[65544:65546]==w(0x8F00),('SP',out[65544:65546].hex())
    return bytearray(out[:65536]),out[65536:],int(r.stdout.strip())
def put(mem,key,value):mem[s[key]:s[key]+2]=w(value)
def init(module,variant,pre,width):
    mem=bytearray(base);data=(R/f'bin/{module}.i80').read_bytes();mem[0x2000:0x2000+len(data)]=data
    mem,regs,_=run(mem,0x2000,af=variant<<8,bc=pre<<8)
    assert not regs[0]&1,('init',module,variant,pre)
    desc=int.from_bytes(regs[4:6],'little')
    assert mem[desc]==width,(module,variant,'width')
    mem,_,_=run(mem,s['engineSave'])
    for offset,key in [(2,'engineScalar'),(4,'engineBatch'),(6,'engineResetStage')]:
        mem[s[key]+1:s[key]+3]=mem[desc+offset:desc+offset+2]
    mem[s['hookRoundMask']+1]=(-mem[desc+1])&255
    mem[s['hookBounds']+1:s['hookBounds']+3]=w(s[f'bounds{width}'])
    mem,_,_=run(mem,s['engineResetStage'])
    mem,_,_=run(mem,s['engineInstall'])
    return mem
def oracle(text,raw,width,x=0,background=0):
    out=bytearray([background])*12288;xp=x*2;y=7;ft=font4 if width==4 else font
    for code in text:
        if not raw and code==13:xp=0;continue
        if not raw and code==10:xp=0;y+=10;continue
        for row,b in enumerate(ft[code*8:code*8+8]):
            for bit in range(width):
                pixel=xp+bit;at=(pixel//8)*256+y-7+row;mask=128>>(pixel%8)
                out[at]=(out[at]&~mask)|(mask if b&(1<<(width-1-bit)) else 0)
        xp+=width
        if xp>=384:xp=0;y+=10
    return out,bytes([y,xp//2])
cases=[('ENGINE6',i,6) for i in range(10)]+[('ENGINEP',0,6),('ENGINEP',1,8),('ENGINEM',0,6),('ENGINEB',0,6)]+[('ENGINE4',i,4) for i in range(3)]
cases += [('ENGINES',i,6) for i in range(5)]
results=[];checks=0
if len(sys.argv)>1: cases=[c for c in cases if c[0] in sys.argv[1:]]
for module,variant,width in cases:
    for pre in (0,1):
        mem=init(module,variant,pre,width)
        for api in ('bios_printChar','bios_printCharRaw','bios_printStringZ','bios_printStringRawZ','bios_printStringRawLim'):
            raw=api not in ('bios_printChar','bios_printStringZ')
            payload=(bytes(range(256)) if api=='bios_printStringRawLim' else bytes(range(1,256)) if raw else b'Test ABC\r\n01234\r\nabc')
            if api.endswith('CharRaw'):payload=bytes([0,13,10,32,65,127,128,192,255])*2
            if api.endswith('Char'):payload=b'ABC\r\nxyz'
            m=bytearray(mem);m[0x8600:0x8600+len(payload)+1]=payload+b'\0'
            m[bs['bios_vars.cursorY']:bs['bios_vars.cursorY']+2]=bytes([7,0])
            m[0x9000:0xC000]=bytes(12288)
            entry=s[api]
            if 'String' in api:
                m,regs,cycles=run(m,entry,bc=len(payload))
                assert regs[:2]==w(0xA597) and regs[2:4]==w(len(payload)) and regs[12:14]==w(0x2468),(module,variant,pre,api,'registers',regs.hex())
                assert regs[4:6]==w(0x8600+len(payload)),(module,variant,api,'HL',regs.hex())
            else:
                cycles=0
                for code in payload:
                    af=(code<<8)|0x97 if raw else 0xA597
                    bc=0x1300|code
                    m,regs,cy=run(m,entry,af=af,bc=bc)
                    assert regs[:6]==w(af)+w(bc)+w(0x8600) and regs[12:14]==w(0x2468),(module,variant,pre,api,'registers',regs.hex())
                    cycles+=cy
            expected,cursor=oracle(payload,raw,width)
            diffs=[i for i in range(12288) if m[0x9000+i]!=expected[i]]
            assert not diffs,(module,variant,pre,api,'pixels',diffs[:16],[(m[0x9000+i],expected[i]) for i in diffs[:8]])
            assert m[bs['bios_vars.cursorY']:bs['bios_vars.cursorY']+2]==cursor,(module,variant,pre,api,'cursor')
            checks+=1
            m,_,_=run(m,s['engineRestore'])
            for i in range(13):
                slot=int.from_bytes(mem[s['hookSlots']+2*i:s['hookSlots']+2*i+2],'little')
                assert m[slot:slot+2]==base[slot:slot+2], ('restore',i)
        # Packet heads/tails retain adjacent pixels. Exercise every allowed
        # initial phase, empty count, and both sides of the 20-byte window.
        for x in ((0,1,2,3) if width==6 else (0,2) if width==4 else (0,)):
            for count in (0,1,2,3,4,5,19,20,21):
                payload=bytes((c*37)&255 for c in range(count))
                m=bytearray(mem);m[0x8600:0x8600+count]=payload
                m[bs['bios_vars.cursorY']:bs['bios_vars.cursorY']+2]=bytes([7,x])
                m[0x9000:0xC000]=bytes([0xA5])*12288
                m,regs,_=run(m,s['bios_printStringRawLim'],bc=count)
                expected,cursor=oracle(payload,True,width,x,0xA5)
                assert m[0x9000:0xC000]==expected,(module,variant,pre,x,count,'tail pixels')
                assert m[bs['bios_vars.cursorY']:bs['bios_vars.cursorY']+2]==cursor,(module,variant,pre,x,count,'tail cursor')
                assert regs[:6]==w(0xA597)+w(count)+w(0x8600+count) and regs[12:14]==w(0x2468),'tail ABI'
                checks+=1
        print(module,variant,pre,'PASS',flush=True)
# The actual BIOS Raw wrapper must retain the caller loop registers too.
for code in range(256):
    m=bytearray(base);m[bs['bios_vars.cursorY']:bs['bios_vars.cursorY']+2]=bytes([7,0])
    m,regs,_=run(m,s['bios_printCharRaw'],af=(code<<8)|0x97,bc=0x1357)
    assert regs[:6]==w((code<<8)|0x97)+w(0x1357)+w(0x8600) and regs[12:14]==w(0x2468),('native Raw ABI',code)
    checks+=1
(O/'results.json').write_text(json.dumps(dict(checks=checks,bitmap='PASS',registers='PASS',slotsRestored=13,engines=len(cases)),indent=2)+'\n')
print('ENGINE GRID CPU PASS',checks)
