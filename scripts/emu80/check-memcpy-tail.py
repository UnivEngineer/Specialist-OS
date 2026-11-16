"""Локальные BIOS/DOS memcpy в видимом Emu80 и возврат в NC через Reset."""
from pathlib import Path
import re
import runpy

R = Path(__file__).resolve().parents[2]
O = R/'docs/bench/memcpy-tail-2026-11-15/emulator'
O.mkdir(parents=True, exist_ok=True)
sy = lambda p: {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', p.read_text('utf-8'), re.M)}
b, k = sy(R/'bin/BIOS.sym'), sy(R/'bin/banked/KERNEL.sym')
h = runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O, RUNTIME=R/'.local/monitor2-001')
x = h['Matrix'](); m = x.m
word = lambda n: n.to_bytes(2, 'little')
def page(n):
    m.write(0xfffd if n else 0xfffc, bytes((n-1 if n else 0,)))
try:
    fixture = runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
    n = sy(fixture['nc_symbols'])
    x.start('memcpy tail MAIN and DOS', **fixture)
    m.call('debug_break'); m.call('bp_clear')
    font = m.read(b['FONT_ADDR'], b['FONT_BYTES'])
    cases = 0
    for bank, entry, stub, stack, start in [(0,b['bios_memcpy_bc_hl'],0x8000,0x8f00,0x3000), (1,k['memcpy_bc_hl'],0xc000,k['DOS_BANK_STACK_TOP'],0xc200)]:
        page(bank)
        if bank:
            assert m.read(k['diActive'],1)==b'\0'
        for count in [0,2,14,16,20,128,254,255,256,272,511]:
            for delta in [-512,-1,0,1,512]:
                src, dst = start+512, start+512+delta
                pattern = bytes((i*37+(i>>8)*13)&255 for i in range(1536))
                expected = bytearray(pattern)
                expected[dst-start:dst-start+count] = pattern[src-start:src-start+count]
                m.write(start,pattern)
                end = stub+0x80
                m.write(stub,b'\xcd'+word(entry)+b'\xc3'+word(end))
                m.stop_at(end)
                m.call('regs_set', pc=stub, sp=stack, hl=src, de=src+count, bc=dst)
                z=m.until(end,10)['registers']
                assert z['sp']['dec']==stack and z['hl']['dec']==src+count and z['bc']['dec']==dst+count and z['de']['dec']==src+count
                assert m.read(start,len(pattern))==expected, (bank,count,delta)
                cases+=1
        print('PASS bank',bank,flush=True)
    page(0)
    assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==font
    m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120,page=0)
    assert m.read(0,32)==Path(fixture['nc_binary']).read_bytes()[:32]
    m.screenshot(O/'nc-after-reset.png')
    print('PASS',cases,'BIOS/DOS copies, font and NC after Reset',flush=True)
finally:
    x.stop()
