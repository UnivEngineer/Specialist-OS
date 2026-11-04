"""Независимая проверка 8x8-глифов, ABI и стоимости вызова на CPU z88dk 8080."""
import argparse
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-font8.py'))
symbols, font_bytes = h['symbols'], h['font_bytes']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--out',type=Path,default=ROOT/'docs/bench/font8-001/cpu')
    args = p.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    renderer = (ROOT/'bin/FONT8.i80').read_bytes()
    dispatch = (ROOT/'bin/F8DISP.i80').read_bytes()
    gate = (ROOT/'bin/BUFFERGATE.i80').read_bytes()
    sy = symbols('FONT8')
    font = font_bytes()
    def run(entry, codepoint, address, bank=False, function=0, length=None):
        m = bytearray((i*37+(i>>8)*13)&255 for i in range(65536))
        m[0x6000:0x6800] = font
        m[0xEB00:0xEB00+len(renderer)] = renderer
        m[0xF880:0xF880+len(dispatch)] = dispatch
        m[0xFFC0:0xFFE0] = gate
        m[0xF870] = 0xC9
        flags = 0x0097 if bank else 0xA597
        bc = (function<<8)|(codepoint if length is None else length)
        if length is not None:
            m[0xFFCC:0xFFE0] = bytes(range(65,85))
        expected = bytearray(m)
        if entry != 0xF870:
            chars = [codepoint] if length is None else ([] if length>20 else list(range(65,65+length)))
            for x,c in enumerate(chars):
                at = address+x*256
                expected[at:at+8] = font[c*8:c*8+8]
        body = bytes.fromhex('31 00 80 21')+flags.to_bytes(2,'little')+bytes.fromhex('E5 F1 01')
        body += bc.to_bytes(2,'little')+bytes.fromhex('11 78 56 21')+address.to_bytes(2,'little')
        body += b'\xCD'+entry.to_bytes(2,'little')
        body += bytes.fromhex('22 00 70 60 69 22 02 70 EB 22 04 70 F5 E1 22 06 70 C3 00 02')
        m[0x100:0x100+len(body)] = body
        inp, out = args.out/'input.bin',args.out/'output.bin'
        inp.write_bytes(m)
        result = subprocess.run([str(args.ticks),'-m8080','-pc','100','-end','200','-counter','100000',
                                 '-output',str(out),str(inp)],check=True,capture_output=True,text=True,timeout=15)
        data = out.read_bytes()
        assert data[0x9000:0xC000] == expected[0x9000:0xC000],(entry,codepoint,address,bank,length)
        assert data[0x6000:0x6800] == font
        assert data[0x7000:0x7008] == address.to_bytes(2,'little')+bc.to_bytes(2,'little')+bytes.fromhex('78 56')+flags.to_bytes(2,'little')
        assert data[65542:65546] == bytes.fromhex('00 02 00 80')
        return int((result.stdout+result.stderr).strip())
    ticks=[]
    for c in range(256):
        address = 0x9000+(c%48)*256+(c//48)*8
        ticks.append(run(sy['draw8'],c,address))
    assert len(set(ticks)) == 1
    for c in [0,65,255]:
        for address in [0x9000,0x90F8,0xBF00,0xBFF8]:
            run(sy['draw8'],c,address)
            run(0xFFC0,c,address,bank=True)
    for length in [0,1,19,20,21]:
        run(sy['buffer8'],0,0x9000,function=1,length=length)
        run(0xFFC0,0,0x9000,bank=True,function=1,length=length)
    baseline = run(0xF870,65,0x9000)
    direct = run(sy['draw8'],65,0x9000)
    bank = run(0xFFC0,65,0x9000,bank=True)
    # Измеряем сам развёрнутый перенос отдельно: два LXI и завершающий JMP
    # дают 30 тактов обвязки. INR r на 8080 требует 5 тактов.
    micro=bytearray(65536)
    micro[0x6000:0x6800]=font
    code=bytes.fromhex('21 00 60 11 00 90')+bytes.fromhex('7E 12 2C 1C')*7+bytes.fromhex('7E 12 C3 00 02')
    micro[0x100:0x100+len(code)]=code
    src=args.out/'pixels.bin';src.write_bytes(micro)
    measured=subprocess.run([str(args.ticks),'-m8080','-pc','100','-end','200','-counter','1000',str(src)],
                            check=True,capture_output=True,text=True,timeout=15)
    copy_ticks=int((measured.stdout+measured.stderr).strip())-30
    assert copy_ticks==182
    report = dict(model='z88dk 8080 flat RAM; mapping checked separately in Emu80',
                  glyphs=256,edge_cases=24,buffer_cases=10,registers_preserved=['AF','BC','DE','HL','SP'],
                  draw8_bytes=sy['buffer8']-sy['draw8'],glyph_copy_ticks=copy_ticks,
                  direct_call_ticks=direct-baseline+27,
                  gate_extra_ticks=bank-direct,buffer20_native_ticks=run(sy['buffer8'],0,0x9000,function=1,length=20)-baseline+27)
    (args.out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':
    main()
