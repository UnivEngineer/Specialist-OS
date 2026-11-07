"""Проверка графического ABI BIOS на CPU 8080: окна, инверсия и memmove битмапа."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--image', type=Path, default=ROOT/'bin/BIOS.sys')
    p.add_argument('--memory-image',type=Path)
    p.add_argument('--report-directory', type=Path, default=ROOT/'docs/nc/ui-002/cpu')
    args = p.parse_args()
    out = args.report_directory.resolve()
    out.mkdir(parents=True, exist_ok=True)
    dos = args.image.read_bytes()
    symbols = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', args.image.with_suffix('.sym').read_text(), re.M)}
    screen = bytes((i*37+(i>>8)*13)&255 for i in range(0x3000))
    def memory():
        m = bytearray(65536)
        if args.memory_image:
            data=args.memory_image.read_bytes()
            assert len(data)==65536
            m[:]=data
        else:
            m[0xC000:0xC000+len(dos)] = dos
        m[0x9000:0xC000] = screen
        return m
    def run(m, entry, hl=0, bc=0, de=0, a=0):
        code = b'\x31\x00\x8F\x21'+hl.to_bytes(2,'little')
        code += b'\x01'+bc.to_bytes(2,'little')+b'\x11'+de.to_bytes(2,'little')
        code += bytes((0x3E,a,0xCD))+entry.to_bytes(2,'little')
        code += bytes.fromhex('22 00 7E 60 69 22 02 7E EB 22 04 7E C3 00 63')
        m[0x6200:0x6200+len(code)] = code
        inp, target = out/'input.bin', out/'output.bin'
        inp.write_bytes(m)
        result = subprocess.run([str(args.ticks),'-m8080','-pc','6200','-end','6300','-counter','2000000','-output',str(target),str(inp)],capture_output=True,text=True,check=True,timeout=15)
        b=target.read_bytes()
        assert int.from_bytes(b[65542:65544],'little') == 0x6300, result.stdout
        assert int.from_bytes(b[65544:65546],'little') == 0x8F00
        return b, result.stdout+result.stderr
    copies=[]
    for src,dst,width,height in [(0x9122,0x9118,10,170),(0x9118,0x9122,10,170),(0x9D18,0x91C2,10,10),(0x91C2,0x9D18,10,10),(0xA918,0x9D18,10,180),(0x9000,0x9000,48,255),(0x9010,0xB01A,1,246)]:
        m=memory();expected=bytearray(m)
        for x in range(width):
            expected[dst+x*256:dst+x*256+height] = m[src+x*256:src+x*256+height]
        b,log=run(m,symbols['bios_uiCopyRect'],src,dst,width*256+height)
        assert b[0x9000:0xC000] == expected[0x9000:0xC000], (hex(src),hex(dst),width,height)
        assert b[0x7E00:0x7E06] == (src+width*256).to_bytes(2,'little')+(dst+width*256).to_bytes(2,'little')+height.to_bytes(2,'little')
        copies.append(dict(source=src,destination=dst,width=width,height=height,model_log=log.strip()))
    inversions=0
    for addr,width,height in [(0x9118,10,10),(0xAB18,1,180),(0x9000,48,255)]:
        m=memory();expected=bytearray(m)
        for x in range(width):
            for y in range(height):expected[addr+x*256+y]^=255
        b,_=run(m,symbols['bios_uiInverseRect'],addr,width*256+height,0x5678)
        assert b[0x9000:0xC000] == expected[0x9000:0xC000]
        assert b[0x7E04:0x7E06] == b'\x78\x56'
        inversions+=1
    windows=0
    # Окно 40x20 пикселей, линия с масками и вертикальное заполнение.
    records=bytes([2,30,0x92,14,3,1,60,0x92,3,0x0F,0xF0,3,70,0x94,5,0x5A,0])
    for offset in (0,24):
        for absolute in (False,True):
            m=memory();objects=bytearray(records)
            if absolute:
                for i in (0,5,11):objects[i]|=0x80
            m[0x7400:0x7400+len(objects)]=objects
            expected=bytearray(m);shift=0 if absolute else offset
            cols=[[0x7F,0x40,0x5F]+[0x50]*14+[0x5F,0x40,0x7F]]
            cols += [[0xFF,0,0xFF]+[0]*14+[0xFF,0,0xFF]]*3
            cols += [[0xFE,2,0xFA]+[0x0A]*14+[0xFA,2,0xFE]]
            for x,data in enumerate(cols):
                at=(0x92+shift+x)*256+30;expected[at:at+20]=bytes(data)
            expected[(0x92+shift)*256+60]|=0x0F
            for x in range(0x93,0x96):expected[(x+shift)*256+60]=255
            expected[(0x96+shift)*256+60]|=0xF0
            at=(0x94+shift)*256+70;expected[at:at+5]=b'\x5A'*5
            b,_=run(m,symbols['bios_uiDraw'],hl=0x7400,a=offset)
            assert b[0x9000:0xC000] == expected[0x9000:0xC000], (offset,absolute)
            windows+=1
    report=dict(cpu='8080 model; no color hardware',copyRect=copies,inverseRectCases=inversions,windowCases=windows)
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':
    main()
