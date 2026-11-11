"""Count NC comparisons on an instrumented CPU model, outside PIT measurements."""
import argparse
import json
from pathlib import Path
import random
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'docs/nc/sort-benchmark-2026-11-11')
    parser.add_argument('--data', type=Path)
    parser.add_argument('--ticks', type=Path,
                        default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    helpers = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))
    builds = helpers['build'](args.output/'profile-build', profile=True)
    data = json.loads((args.data or args.output/'results.json').read_text())
    datasets = {name:bytes.fromhex(data['datasets'][name]['records_hex'])
                for name in ('H-root','H-root-250','random-255')}
    for count in (32,64,128):
        rows = [helpers['record'](f'File{i:04}', ('ASM','TXT','COM','')[i%4], i%9==0)
                for i in range(count)]
        random.Random(8080).shuffle(rows)
        datasets[f'small-random-{count}'] = b''.join(rows)
    binary, symbols = builds[0]
    bios = (ROOT/'bin/BIOS.sys').read_bytes()
    result = []
    for name, raw in datasets.items():
        rows = [raw[i:i+16] for i in range(0,len(raw),16)]
        for mode in (1,2):
            for algorithm, entry in (('shell','ncSortPass'),('merge','benchMerge'),('quick','benchQuick')):
                memory = bytearray(65536)
                memory[0x100:0x100+len(binary)] = binary
                memory[0xc000:0xc000+len(bios)] = bios
                memory[0x4000:0x4000+len(raw)] = raw
                memory[symbols['state.panelA_filesCnt']] = len(rows)
                memory[symbols['state.panelA_sort']] = mode
                at = symbols['benchInvoke']+1
                memory[at:at+2] = symbols[entry].to_bytes(2,'little')
                source, destination = args.output/'profile.in.bin', args.output/'profile.out.bin'
                source.write_bytes(memory)
                run = subprocess.run([str(args.ticks),'-m8080','-pc',f"{symbols['benchRun']:x}",
                                      '-end',f"{symbols['benchDone']:x}",'-counter','100000000',
                                      '-output',str(destination),str(source)], capture_output=True,
                                     text=True,check=True,timeout=30)
                output = destination.read_bytes()
                assert int.from_bytes(output[65542:65544],'little') == symbols['benchDone'], run.stdout
                expected = bytes(sorted(range(len(rows)), key=lambda i:helpers['key'](rows[i],mode)))+b'\xff'
                assert output[0x6000:0x6000+len(expected)] == expected,(name,mode,algorithm)
                assert output[0x4000:0x4000+len(raw)] == raw
                def counter(label):
                    at = symbols[label]
                    return int.from_bytes(output[at:at+2],'little')
                result.append(dict(dataset=name,count=len(rows),mode=mode,algorithm=algorithm,
                                   comparisons=counter('profileComparisons'),
                                   character_pairs=counter('profileCharacters'),
                                   quick_swap_calls=counter('profileQuickSwaps'),
                                   quick_range_calls=counter('profileQuickCalls')))
    (args.output/'profile.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Profile counters checked:',len(result),'cases. Timings remain from the uninstrumented PIT builds.')


if __name__ == '__main__':
    main()
