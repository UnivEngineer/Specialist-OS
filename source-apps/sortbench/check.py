"""8080 regression of the actual SORTBEN binary, including both core variants."""
from pathlib import Path
import random
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/nc/sortbench-2026-11-12/cpu'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    symbols = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',
               (ROOT/'bin/SORTBEN.sym').read_text(),re.M)}
    helper = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))
    record, key = helper['record'],helper['key']
    suites = [[],[record('ONLY')],[record('A','Z'),record('A','A'),record('AA'),
              record('A B','TXT'),record('A C','TXT'),record('SAME','A B'),
              record('SAME','A C'),record('я','TXT'),record('Я','ASM'),
              record('..',directory=True),record('\\',directory=True),
              record('Dir','TXT',True),record('Dir','ASM',True),record('same','a b')]]
    for n in (32,64,128,250,255):
        rows = [record(f'FILE{i:04}',('TXT','ASM','COM','')[i%4],i%9==0) for i in range(n)]
        random.Random(8080).shuffle(rows)
        suites.append(rows)
    image = (ROOT/'bin/SORTBEN.i80').read_bytes()
    bios = (ROOT/'bin/BIOS.sys').read_bytes()
    cases = 0
    for rows in suites:
        raw = b''.join(rows)
        for mode in (1,2):
            for module in ('raw','upper'):
                algorithms = ['ncSortPass','benchInsertion','benchMerge','benchQuick']
                if module=='raw':
                    algorithms += ['ncRadixSort']
                for algorithm in algorithms:
                    memory = bytearray(65536)
                    memory[:len(image)] = image
                    memory[0xc000:0xc000+len(bios)] = bios
                    memory[0x4000:0x4000+len(raw)] = raw
                    for ns in ('raw','upper'):
                        memory[symbols[f'{ns}.state.panelA_filesCnt']] = len(rows)
                        memory[symbols[f'{ns}.state.panelA_sort']] = mode
                    at = symbols[f'{module}.benchInvoke']+1
                    memory[at:at+2] = symbols[f'{module}.{algorithm}'].to_bytes(2,'little')
                    prepare = symbols['raw.benchPrepare'] if module=='upper' else symbols['raw.benchOptimizedPrepare']
                    word = lambda value:value.to_bytes(2,'little')
                    code = (b'\x31\x00\x8b\xcd'+word(prepare)+b'\xcd'+
                            word(symbols[f'{module}.benchRun'])+b'\xc3\xff\x80')
                    memory[0x8000:0x8000+len(code)] = code
                    source,destination = OUT/'input.bin',OUT/'output.bin'
                    source.write_bytes(memory)
                    subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                                    '-m8080','-pc','8000','-end','80ff','-counter','200000000',
                                    '-output',str(destination),str(source)],check=True,
                                   capture_output=True,timeout=30)
                    result = destination.read_bytes()
                    assert int.from_bytes(result[65542:65544],'little')==0x80ff
                    expected = bytes(sorted(range(len(rows)),key=lambda i:key(rows[i],mode)))+b'\xff'
                    assert result[0x6000:0x6000+len(expected)]==expected,(len(rows),mode,module,algorithm)
                    assert result[0x4000:0x4000+len(raw)]==raw
                    cases += 1
    print('SORTBEN CPU PASS:',cases,'cases; actual COM payload; both modes, all algorithms, source immutable.')


if __name__=='__main__':
    main()
