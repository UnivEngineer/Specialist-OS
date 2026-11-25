"""Measure one group within an assembled pixel-row loop on the 8080."""
import argparse
import itertools
import json
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
font = root/'bin/fonts'
engine_check = (root/'source-apps/fontbench/check-engines.py').read_text(encoding='utf-8')
ticks = re.search(r"^TICKS='([^']+)'", engine_check, re.M)[1]
word = lambda x: x.to_bytes(2,'little')
modules = (('RotRows',3),('FontPack',3),('FontFull',3),
           ('Font4',1),('Font4Fst',1),('Font4Cmp',1))
results = {}
with tempfile.TemporaryDirectory(prefix='font-row-core-') as work:
    path = Path(work)/'program.bin'
    start = 0x8000
    for name, size in modules:
        labels = {key:int(value,16) for key,value in re.findall(
            r'^([\w.]+): EQU 0x([0-9a-fA-F]+)', (font/f'{name}.sym').read_text(), re.M)}
        module = (font/f'{name}.i80').read_bytes()
        results[name] = {}
        for mode in ('Normal','Inverse'):
            limit = labels[f'rowStream.stream{mode}Limit']
            end = limit+5
            memory = bytearray(65536)
            memory[0x2000:0x2000+len(module)] = module
            assert memory[limit] == 0xFE and memory[limit+2] == 0xC2
            group = int.from_bytes(memory[limit+3:end],'little')
            memory[limit+1] = 0x90+size
            setup = bytes.fromhex('310086010040110090210040c3')+word(group)
            memory[start:start+len(setup)] = setup
            times = []
            for left,right in itertools.product((0x20,0x21), repeat=2):
                memory[0x8600:0x8604] = bytes([left,right,left,right])
                path.write_bytes(memory)
                time = int(subprocess.check_output([ticks,'-m8080','-pc',f'{start:x}',
                           '-end',f'{end:x}','-counter','50000',str(path)]))
                times.append(time-50)  # four LXI and one JMP
            results[name][mode.lower()] = {'bytes':end-group,'min':min(times),'max':max(times)}
        results[name]['screen_writes'] = size
        results[name]['kernel_bytes'] = labels['rowStream.streamNext']+2-labels['rowStream.tryGroup']
Path(__file__).with_name('kernel-timing.json').write_text(json.dumps(results,indent=2)+'\n')
for module, data in results.items():
    print(module, data)

