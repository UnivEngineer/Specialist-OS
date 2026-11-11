"""Launch SORTBEN through NC keyboard and check its menus, report and return."""
import json
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/nc/sortbench-2026-11-12/emulator'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    sym = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))['symbols']
    runtime = ROOT/'.local/nc-sort-benchmark-runtime'
    h['Matrix'].__init__.__globals__.update(RUNTIME=runtime,OUT=OUT)
    matrix = h['Matrix']()
    sb,nc = sym(ROOT/'bin/SORTBEN.sym'),sym(ROOT/'bin/NC.sym')
    try:
        disk = ROOT/'docs/nc/sort-optimized-2026-11-11/H-sort250.bin'
        matrix.start('standalone-sortbench',disk=disk)
        m = matrix.m
        for source in ('s','h'):
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.tap('esc')
            m.call('emu_run_for',ms=250)
            command = 'A:/BENCH/SORTBEN/SORTBEN.COM'
            for char in command:
                m.tap({':':'colon','/':'slash','.':'period'}.get(char,char.lower()))
            assert m.read(nc['vars.cmdLine'],len(command)).decode().upper()==command
            m.screenshot(OUT/f'{source}-command.png')
            m.stop_at(0)
            m.tap('enter')
            m.until(0,120)
            assert m.read(0,64)==(ROOT/'bin/SORTBEN.i80').read_bytes()[:64]
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.call('debug_break')
            m.screenshot(OUT/f'{source}-menu.png')
            assert m.read(0,3)==(ROOT/'bin/SORTBEN.i80').read_bytes()[:3],m.call('cpu_state')
            m.call('emu_run_for',ms=100)
            m.tap(source)
            m.call('emu_run_for',ms=250)
            m.stop_at(sb['waitBetweenTables'])
            m.tap('1')
            m.until(sb['waitBetweenTables'],120)
            m.screenshot(OUT/f'{source}-first-table.png')
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.stop_at(sb['waitForExit'])
            m.tap('enter')
            m.until(sb['waitForExit'],120)
            m.screenshot(OUT/f'{source}-stopped.png')
            (OUT/f'{source}-state.json').write_text(json.dumps(m.call('cpu_state'),indent=2))
            # The kernel patches its CALL operands within the first 64 bytes.
            assert m.read(0,3)==(ROOT/'bin/SORTBEN.i80').read_bytes()[:3],m.call('cpu_state')
            end = int.from_bytes(m.read(sb['reportEnd'],2),'little')
            text = m.read(sb['REPORT'],end-sb['REPORT'])
            (OUT/f'{source}-report.txt').write_bytes(text)
            assert m.read(sb['failed'],1)==b'\0',text
            assert b'All indices match.' in text and b'Source checksum OK.' in text,text
            assert text.count(b' OK\r\n')==18,text
            actual = int.from_bytes(m.read(sb['saveIO']+sb['FILE_PATH_IO.actual'],2),'little')
            assert actual==len(text),(actual,len(text),text)
            assert m.read(sb['count'],1)==bytes([32])
            m.screenshot(OUT/f'{source}-results.png')
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.stop_at(nc['mainLoop'])
            m.tap('space')
            m.until(nc['mainLoop'],120)
            assert m.read(0,64)==(ROOT/'bin/NC.i80').read_bytes()[:64]
            print('SORTBEN UI PASS:',source,'32 records, 18 checked sorts, report saved, returned to NC',flush=True)
        # Separate ABI check after all keyboard scenarios: read back the actual
        # saved file. This does not substitute for entering/running through NC.
        io = bytearray(sb['FILE_PATH_IO'])
        struct.pack_into('<HHHH',io,0,0x7540,0x7600,len(text)+1,0)
        m.write(0x7500,io)
        m.write(0x7540,b'B:\\SORTBEN.TXT\0'+bytes(113))
        word = lambda x:x.to_bytes(2,'little')
        code = (b'\x21\x00\x75\xcd'+word(sb['mx_callRegs'])+
                word(sb['MXOS_DOS_FILE_LOAD_PATH'])+b'\xc3\xff\x81')
        m.write(0x8100,code)
        m.stop_at(0x81ff)
        m.call('regs_set',pc=0x8100,sp=0x8b00)
        state = m.until(0x81ff,120)
        assert not state['flags']['C'],state
        assert int.from_bytes(m.read(0x7506,2),'little')==len(text)
        assert m.read(0x7600,len(text))==text
        (OUT/'result.json').write_text(json.dumps(dict(synthetic=True,hRoot=True,
            records=32,sortsPerRun=18,savedReport=True,readBackMatches=True,
            returnedToNC=True),indent=2)+'\n')
    finally:
        matrix.stop()


if __name__=='__main__':
    main()
