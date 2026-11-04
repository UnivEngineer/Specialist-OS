"""Отдельные диапазоны DOS4000/BIOS, неизменный ABI и независимость BIOS."""
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def symbols(name):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (ROOT/'bin'/name).read_text('utf-8'),re.M)}


def main():
    sy = symbols('DOS4000.sym')
    regular = symbols('DOS.sym')
    layout = json.loads((ROOT/'bin/DOS4000.layout.json').read_text('utf-8'))
    memory = (ROOT/'bin/DOS4000.memory.bin').read_bytes()
    assert len(memory)==65536
    core,bios = layout['regions']
    assert core['start']==0x4000 and core['end']<=sy['bios_vars']
    assert bios['start']==0xC000 and bios['end']<=sy['CMD_LINE']
    assert core['end']<=bios['start']
    for part in [core,bios]:
        data = (ROOT/'bin'/part['file']).read_bytes()
        assert part['end']-part['start']==part['bytes']==len(data)
        assert memory[part['start']:part['end']]==data
    spans=sorted((b['address'],b['address']+b['size']) for b in layout['blocks'] if b['size'])
    spans+= [(w['address'],w['address']+w['size']) for w in layout['windows']]
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
    declarations=re.findall(r'(?m)^(bios_\w+)\s*=\s*0[0-9A-Fa-f]+h',
                            (ROOT/'source/include/mxos.inc').read_text('utf-8'))
    assert {n for n in declarations if n in sy}=={n for n in declarations if n in regular}
    declarations=[n for n in declarations if n in regular]
    assert all(sy[name]==regular[name] for name in declarations)
    for name in ['bios_fileCopy','bios_fileSelectDrive','bios_getError','bios_uiDraw']:
        address=sy[name]
        assert memory[address]==0xC3
        target=int.from_bytes(memory[address+1:address+3],'little')
        assert core['start']<=target<core['end'], (name,target)
    bios_names=json.loads((ROOT/'source/dos/layout4000.json').read_text('utf-8'))['bios_units']
    manifest=json.loads((ROOT/'source/dos/layout.json').read_text('utf-8'))
    seen=set()
    def scan(file):
        path=(ROOT/'source/dos'/file).resolve()
        if path in seen:return
        seen.add(path)
        text=path.read_text('utf-8')
        for number,line in enumerate(text.splitlines(),1):
            code=re.sub(r'"[^"]*"|;.*','',line)
            refs=[token for token in re.findall(r'\b[\w.]+\b',code)
                  if token and token[0].islower() and token in sy and core['start']<=sy[token]<core['end']]
            assert not refs,(file,number,refs)
        for nested in re.findall(r'(?mi)^\s*INCLUDE\s+"([^"]+)"',text):
            scan(path.parent/nested)
    for unit in manifest['units']:
        if unit['name'] not in bios_names: continue
        for file in unit['files']:
            scan(file)
    for name in ['printChar','clearScreen','getch2','memcpy_bc_hl','calcCS','input',
                 'v_memTop','v_char','v_input_start','v_input_end','diskDriver',
                 'romMappedRead','setRAMDDriver','diskWriteChecked','v_drive','v_drives','v_diskInfo']:
        assert bios['start']<=sy[name]<bios['end'], (name,sy[name])
    assert not layout['integratedWithNC'] and not layout['bankSwitching']
    report=dict(regions=layout['regions'],abiEntriesUnchanged=len(declarations),
                biosHasNoReferencesToDos=True,nonoverlapping=True,
                memorySha256=hashlib.sha256(memory).hexdigest())
    out=ROOT/'docs/fat/repair-001/DOS4000-static.json'
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report))


if __name__=='__main__':main()
