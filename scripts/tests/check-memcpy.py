"""Сравнение memmove и прокрутки BIOS со снимком прежней реализации, CPU 8080."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--baseline', type=Path, default=ROOT/'docs/bench/memcpy-002/before/DOS.sys')
    p.add_argument('--image', type=Path, default=ROOT/'bin/BIOS.sys')
    p.add_argument('--out', type=Path, default=ROOT/'docs/bench/memcpy-002/cpu')
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    def syms(path):
        return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.with_suffix('.sym').read_text(),re.M)}
    def run(image, entry, src=0, dst=0, count=0, inverse=0):
        sy = syms(image)
        memory = bytearray((i*37+(i>>8)*13)&255 for i in range(65536))
        binary = image.read_bytes()
        memory[0xC000:0xC000+len(binary)] = binary
        memory[0x8FFA:0x8FFC] = inverse.to_bytes(2,'little')
        code = bytes.fromhex('31 00 8F 21')+src.to_bytes(2,'little')
        code += b'\x11'+((src+count)&65535).to_bytes(2,'little')+b'\x01'+dst.to_bytes(2,'little')
        address = sy[entry] if entry == 'bios_memcpy_bc_hl' else sy['dos_piece_scroll_1_start' if entry == 'up' else 'dos_piece_scrollDown_1_start']
        code += b'\xCD'+address.to_bytes(2,'little')
        code += bytes.fromhex('22 00 87 60 69 22 02 87 EB 22 04 87 C3 00 83')
        memory[0x8200:0x8200+len(code)] = code
        expected = bytearray(memory)
        if entry == 'bios_memcpy_bc_hl':
            expected[dst:dst+count] = memory[src:src+count]
        else:
            down = entry == 'down'
            fill = inverse.to_bytes(2,'little')*5
            for x in range(48):
                at = 0x9000+x*256
                if down:
                    expected[at:at+256] = fill+memory[at:at+246]
                else:
                    expected[at:at+256] = memory[at+10:at+256]+fill
        inp, out = args.out/'input.bin', args.out/'output.bin'
        inp.write_bytes(memory)
        result = subprocess.run([str(args.ticks),'-m8080','-pc','8200','-end','8300',
                                 '-counter','4000000','-output',str(out),str(inp)],
                                capture_output=True,text=True,check=True,timeout=15)
        data = out.read_bytes()
        assert data[65542:65544] == bytes.fromhex('00 83'), result.stdout
        assert data[65544:65546] == bytes.fromhex('00 8F')
        if entry == 'bios_memcpy_bc_hl':
            assert data[dst:dst+count] == expected[dst:dst+count], (image,src,dst,count)
            assert data[0x8700:0x8706] == ((src+count)&65535).to_bytes(2,'little')+((dst+count)&65535).to_bytes(2,'little')+((src+count)&65535).to_bytes(2,'little')
            # Проверяем соседние байты вне стека и служебной области.
            if dst and dst-1 < 0x8000:
                assert data[dst-1] == expected[dst-1]
            if dst+count < 0x8000:
                assert data[dst+count] == expected[dst+count]
        else:
            assert data[0x9000:0xC000] == expected[0x9000:0xC000], (image,entry,inverse)
            assert data[0x8702:0x8704] == dst.to_bytes(2,'little'), 'BC changed'
        return int((result.stdout+result.stderr).strip())
    cases = []
    lengths = list(range(34))+[63,64,127,128,245,246,247,254,255,256,257,258,511,512,513,1023,1024,4096,8192]
    for n in lengths:
        for src,dst in [(0x2000,0x5000),(0x3000,0x2000),(0x2000,0x2001),(0x2001,0x2000),(0x2000,0x2000)]:
            ticks = run(args.image,'bios_memcpy_bc_hl',src,dst,n)
            cases.append(dict(source=src,destination=dst,count=n,ticks=ticks))
    for src,dst,n in [(0xF000,0x3000,4096),(0x3000,0xF000,4096)]:
        cases.append(dict(source=src,destination=dst,count=n,ticks=run(args.image,'bios_memcpy_bc_hl',src,dst,n)))
    comparisons=[]
    for n in [2,20,246,256,512,4096,12278]:
        for src,dst in [(0x2000,0x1000),(0x1000,0x2000)]:
            new=run(args.image,'bios_memcpy_bc_hl',src,dst,n)
            if not args.baseline.exists():
                continue
            old=run(args.baseline,'bios_memcpy_bc_hl',src,dst,n)
            comparisons.append(dict(count=n,backward=dst>src,old_ticks=old,new_ticks=new,percent=100*(new/old-1)))
    scroll=[]
    for entry in ['up','down']:
        for inverse in [0,0xFFFF,0x5AA5]:
            new=run(args.image,entry,inverse=inverse)
            if not args.baseline.exists():
                continue
            old=run(args.baseline,entry,inverse=inverse)
            scroll.append(dict(entry=entry,inverse=inverse,old_ticks=old,new_ticks=new,percent=100*(new/old-1)))
    report=dict(copy_cases=len(cases),cases=cases,comparisons=comparisons,scroll=scroll)
    (args.out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='cases'}))


if __name__=='__main__':
    main()
