from pathlib import Path
import tempfile

root=Path(__file__).resolve().parents[3]
script=root/'source-apps/fontbench/check-engines.py'
ns={'__file__':str(script)}
exec(compile(script.read_text(encoding='utf8').split('cases=')[0],str(script),'exec'),ns)
s,bs,run,base,w=tuple(ns[k] for k in ('s','bs','run','base','w'))

font6=bytes(x & 63 for x in (root/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes())
font8=(root/'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()

def setup(name):
    data=(root/f'bin/fonts/{name}.i80').read_bytes()
    m=bytearray(base)
    m[0x2000:0x2000+len(data)]=data
    m,regs,_=run(m,0x2000)
    assert not regs[0]&1
    descriptor=int.from_bytes(regs[4:6],'little')
    m,_,_=run(m,s['engineSave'])
    m,_,_=run(m,s['directOpen'],hl=descriptor)
    m[s['engineHistoricalMode']]=0x80
    m,_,_=run(m,s['engineInstall'])
    return m

def check(m,font,width,phase,inverse,count,api,payload,background):
    m=bytearray(m)
    pos=bs['bios_vars.cursorY']
    m[pos:pos+2]=bytes([7,phase])
    m[bs['bios_vars.inverse']:bs['bios_vars.inverse']+2]=bytes([255*inverse])*2
    m[0x8500:0x8500+len(payload)+1]=payload+b'\0'
    m[0x9000:0xc000]=bytes([background])*12288
    result,regs,cycles=run(m,s[api],hl=0x8500,bc=count)
    expected=bytearray([background])*12288
    xp=phase*2
    for code in payload:
        for y in range(8):
            bits=font[code*8+y]^(((1<<width)-1) if inverse else 0)
            for bit in range(width):
                pixel=xp+bit
                addr=(pixel//8)*256+y
                mask=128>>(pixel%8)
                expected[addr]=(expected[addr]&~mask)|(mask if bits&(1<<(width-1-bit)) else 0)
        xp+=width
    assert result[0x9000:0xc000]==expected,(width,phase,inverse,count,api)
    assert result[pos:pos+2]==bytes([7,xp//2]),(width,phase,inverse,count,api,'cursor')
    assert regs[4:6]==w(0x8500+len(payload)),(width,phase,inverse,count,api,'input pointer')
    return cycles

with tempfile.TemporaryDirectory(prefix='fontben-') as tmp:
    ns['O']=Path(tmp)
    for name,width,font in [('FontRot',6,font6),('RotRows',6,font6),('Rot8Quad',8,font8),('Rot8Strm',8,font8)]:
        m=setup(name)
        for phase in range(4):
            for inverse in (False,True):
                for count in (0,1,2,3,4,5,7,8,19,20,21,46):
                    payload=bytes((c*37)&255 for c in range(count))
                    check(m,font,width,phase,inverse,count,'bios_printStringRawLim',payload,0xA5)
                payload=bytes(range(32,78))
                check(m,font,width,phase,inverse,46,'bios_printStringRawZ',payload,0xA5)
        print(name,'edge pixels, both directions of inversion, both Raw APIs PASS',flush=True)
