"""CPU 8080: фазы сдвига, соседние пиксели, сырой буфер и сохранение ABI."""
import argparse
import json
from pathlib import Path
import runpy
import subprocess

ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-shifted-font.py'))
symbols,shifted_tables,font8=h['symbols'],h['shifted_tables'],h['font8']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--out',type=Path,default=ROOT/'docs/bench/shifted-font-001/cpu')
    args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    dos=(ROOT/'bin/DOS.sys').read_bytes();ds=symbols('DOS')
    init=(ROOT/'bin/DOS4000.memory.bin').read_bytes()[0x8FDF:0x9000]
    font=(ROOT/'assets/system-rom/FONT.FNT').read_bytes()
    def run(mode,batch,x,y,bank=False,length=1,inverse=False,codepoint=65,koi7=False,alternate=False):
        m=bytearray((i*37+(i>>8)*13)&255 for i in range(65536))
        m[0xC000:0xC000+len(dos)]=dos
        m[0x8FDF:0x9000]=init;m[0x8FE7:0x8FE9]=(0xE300//8).to_bytes(2,'little')
        m[0x8FEA:0x8FEC]=bytes([255 if koi7 else 0,255]);m[0x8FFA:0x8FFC]=b'\xff\xff' if inverse else b'\0\0'
        m[0xE300:0xEB00]=font;m[0x3000:0x6000]=shifted_tables();m[0x6000:0x6800]=font8()
        active_font=font
        if alternate:
            active_font=(ROOT/'assets/system-rom/FONT_IT.FNT').read_bytes()
            m[0x7100:0x7900]=active_font
            m[0x8FE7:0x8FE9]=(0x7100//8).to_bytes(2,'little')
        renderer=(ROOT/f'bin/SHIFT_M{mode}_B{int(batch)}.i80').read_bytes()
        m[0xEB00:0xEB00+len(renderer)]=renderer
        if mode==3:
            optimized=(ROOT/'bin/OPT_DRAW.i80').read_bytes();m[0xED00:0xED00+len(optimized)]=optimized
            at=ds['dos_piece_glyph_1_start'];m[at:at+3]=bytes.fromhex('C3 00 ED')
        dispatch=(ROOT/'bin/SHIFTDISP.i80').read_bytes();m[0xF880:0xF880+len(dispatch)]=dispatch
        m[0xFFC0:0xFFE0]=(ROOT/'bin/BUFFERGATE.i80').read_bytes();m[0xFFCC:0xFFE0]=b'A'*20
        expected=bytearray(m);count=length if batch else 1
        if count>20 and batch:
            count=0
        if koi7 and codepoint>=0xC0 and not batch:
            count=0
        effective=65 if batch else codepoint
        if koi7 and 0x60<=effective<0x80:
            effective+=0x80
        for k in range(count):
            if mode==2:
                glyph=font8()[effective*8:effective*8+8]
                for row,v in enumerate(glyph):
                    expected[((x+k*8)//8)*256+0x9000+y+row]=v
            else:
                for row,v in enumerate(active_font[effective*8:effective*8+8]):
                    if inverse:
                        v^=0x3F
                    for bit in range(6):
                        xx=x+k*6+bit;at=0x9000+(xx//8)*256+y+row;mask=1<<(7-xx%8)
                        expected[at]=(expected[at]&(~mask&255))|(mask if v&(1<<(5-bit)) else 0)
        flags=0x0097 if bank else 0xA597
        bc=(0x0100|length) if batch else codepoint
        entry=0xFFC0 if bank else (0xEB03 if batch else 0xEB00)
        code=b'\xCD'+ds['normalVideo'].to_bytes(2,'little')
        code+=bytes.fromhex('31 00 80 21')+bytes([y+7,x//2])+bytes.fromhex('22 FC 8F 21')+flags.to_bytes(2,'little')
        code+=bytes.fromhex('E5 F1 01')+bc.to_bytes(2,'little')+bytes.fromhex('11 78 56 21 BC 9A CD')+entry.to_bytes(2,'little')
        code+=bytes.fromhex('22 00 70 60 69 22 02 70 EB 22 04 70 F5 E1 22 06 70 C3 00 02')
        m[0x100:0x100+len(code)]=code
        inp,out=args.out/'input.bin',args.out/'output.bin';inp.write_bytes(m)
        result=subprocess.run([str(args.ticks),'-m8080','-pc','100','-end','200','-counter','200000',
                               '-output',str(out),str(inp)],check=True,capture_output=True,text=True,timeout=15)
        data=out.read_bytes()
        assert data[0x9000:0xC000]==expected[0x9000:0xC000],(mode,batch,x,y,bank,length)
        assert data[0x7000:0x7008]==bytes.fromhex('BC 9A')+bc.to_bytes(2,'little')+bytes.fromhex('78 56')+flags.to_bytes(2,'little')
        assert data[65542:65546]==bytes.fromhex('00 02 00 80')
        assert data[0x8FFC:0x8FFE]==bytes([y+7,x//2+count*(4 if mode==2 else 3)])
        return int((result.stdout+result.stderr).strip())
    phases=[];tests=0
    for mode in range(4):
        positions=[0,2,4,6,374,376,378] if mode!=2 else [0,8,376]
        for x in positions:
            for y in [0,248]:
                a=run(mode,False,x,y);b=run(mode,False,x,y,True);tests+=2
                if x<8 and y==0:
                    phases.append(dict(mode=mode,x=x,main_ticks=a,page0_ticks=b))
        for n in [0,1,19,20,21]:
            run(mode,True,0,0,length=n);run(mode,True,0,0,True,length=n);tests+=2
    for mode in [0,3]:
        for x in [0,2,4,6]:
            for bank in [False,True]:
                run(mode,False,x,248,bank,inverse=True);tests+=1
        for c in [0x61,0x7F,0xC0]:
            run(mode,False,0,0,True,codepoint=c,koi7=True);tests+=1
        for x in [0,2,4,6]:
            run(mode,False,x,0,True,alternate=True,inverse=True);tests+=1
    # Все печатные коды проверяем для нового штатного бэкенда; эталон
    # вычисляется по пикселям, а не с помощью старого рисовальщика.
    for c in range(32,256):
        for inv in [False,True]:
            run(3,False,(c%4)*2,0,codepoint=c,inverse=inv);tests+=1
    decode=runpy.run_path(str(ROOT/'scripts/monitor2/disassemble.py'))['decode']
    for mode in [1,2]:
        sy=symbols(f'SHIFT_M{mode}_B1');binary=(ROOT/f'bin/SHIFT_M{mode}_B1.i80').read_bytes()
        at=sy['readChar']-0xEB00;end=sy['restore']-0xEB00
        while at<end:
            size,mnemonic=decode(binary,at)
            assert mnemonic.split()[0].lower() not in ['call','push','pop'],(mode,mnemonic)
            at+=size
        assert at==end
    kernels=[]
    for mode,label,source,masks,branch_ticks in [(1,'oneByte',0x3000+65*8,3,10),
                                               (1,'twoBytes',0x4000+65*16,0xF03F,0),
                                               (2,'eightBytes',0x6000+65*8,0,10)]:
        sy=symbols(f'SHIFT_M{mode}_B0');memory=bytearray(65536)
        memory[0x3000:0x6000]=shifted_tables();memory[0x6000:0x6800]=font8()
        image=(ROOT/f'bin/SHIFT_M{mode}_B0.i80').read_bytes();memory[0xEB00:0xEB00+len(image)]=image
        code=b'\x21'+source.to_bytes(2,'little')+bytes.fromhex('11 00 90 01')+masks.to_bytes(2,'little')
        code+=b'\xC3'+sy[label].to_bytes(2,'little');memory[0x100:0x100+len(code)]=code
        path=args.out/f'{label}.bin';path.write_bytes(memory)
        measured=subprocess.run([str(args.ticks),'-m8080','-pc','100','-end',f'{sy["glyphDone"]:x}',
                                 '-counter','10000',str(path)],capture_output=True,text=True,check=True,timeout=15)
        pixels=int((measured.stdout+measured.stderr).strip())-40-branch_ticks
        kernels.append(dict(kernel=label,pixel_ticks=pixels))
    assert [v['pixel_ticks'] for v in kernels]==[270,585,182],kernels
    report=dict(model='z88dk 8080 flat RAM; real page mapping checked in Emu80',tests=tests,
                background='nonzero pseudo-random bytes; untouched pixels verified',phases=phases,
                kernels=kernels,raw_inner_loop_has_no_call_push_pop=True)
    (args.out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':
    main()
