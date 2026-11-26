"""8080 NC.CFG parser and runtime palette, independent of CLI.EXT/build time."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-config-cpu'
W = lambda value: int(value).to_bytes(2, 'little')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    symbols = lambda path: {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'), re.M)}
    n = symbols(ROOT / 'bin/NC.sym')
    memory = bytearray(65536)
    for address, file in [(0, 'NC.i80'), (0xc000, 'BIOS.sys'), (0xf000, 'IO.main.bin')]:
        data = (ROOT / 'bin' / file).read_bytes()
        memory[address:address+len(data)] = data
    memory[n['vars.driveCount']] = 4
    memory[n['vars.driveList']:n['vars.driveList']+4] = bytes([0,1,2,7])

    def call(base, entry, hl=0):
        result = bytearray(base)
        code = b'\x31'+W(0x8b00)+b'\x21'+W(hl)+b'\xcd'+W(n[entry])+b'\x32'+W(0x83ff)+b'\xc3'+W(0x81ff)
        result[0x8100:0x8100+len(code)] = code
        (OUT / 'input.bin').write_bytes(result)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', '8100', '-end', '81ff', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        data = (OUT / 'output.bin').read_bytes()
        assert data[65542:65544] == W(0x81ff), (entry, data[65542:65544].hex())
        assert data[65544:65546] == W(0x8b00), (entry, 'SP')
        return bytearray(data[:65536])

    def parse(data, previous=None):
        result = call(memory, 'ncConfigInit') if previous is None else bytearray(previous)
        result[0x7000:0x7000+len(data)+1] = data+b'\0'
        return call(result, 'ncConfigParse', 0x7000)

    def rules(result):
        start = n['vars.fileColorRules']
        values = {}
        for i in range(32):
            row = result[start+4*i:start+4*i+4]
            if row[0] == 0:
                break
            values.setdefault(bytes(row[:3]).decode('koi8-r').strip(), row[3])
        return values

    def color(base, extension, attrib=0):
        result = bytearray(base)
        result[0x8500:0x8510] = b'FILE    '+extension.encode('koi8-r').ljust(3, b' ')+bytes([attrib])+bytes(4)
        return call(result, 'ncFileColor', 0x8500)[0x83ff]

    shipped = parse((ROOT / 'assets/system-rom/NC.CFG').read_bytes())
    assert rules(shipped)['FNT'] == 0xa1 and rules(shipped)['BAT'] == 0xc1
    drives = n['state.panelA_drive']
    assert shipped[drives:drives+2] == bytes([2,7])
    previous = bytearray(shipped)
    for field in ('state.panelA_dirLow','state.panelA_dirHigh','vars.pathValidA'):
        start = n[field]
        previous[start:start+2] = b'\1\1'
    result = parse(b'defaultleftdrive b:\r\nDefaultRightDrive a\n', previous)
    assert result[drives:drives+2] == bytes([1,0])
    for field in ('state.panelA_dirLow','state.panelA_dirHigh','vars.pathValidA'):
        start = n[field]
        assert result[start:start+2] == b'\0\0'
    for bad in (b'', b'D:', b'I:', b'B:/', b'BX', b'1:'):
        result = parse(b'DefaultRightDrive '+bad, previous)
        assert result[drives:drives+2] == bytes([2,7])
    no_external = bytearray(memory)
    no_external[n['vars.driveCount']] = 2
    no_external[n['buildDriveList']] = 0xc9  # The fixture already supplies A: and B:.
    fallback = call(no_external, 'initRightDrive')
    assert parse(b'DefaultRightDrive H:', fallback)[drives:drives+2] == bytes([2,1])
    for ext, attr, expected in [('fnt', 4, 0xa1), ('MON', 5, 0xa1), ('BAT', 0, 0xc1),
                                ('cfg', 4, 0xc1), ('txt', 0, 0xb1), ('zip', 0, 0xd1),
                                ('SYS', 4, 0x71), ('ZZZ', 0, 0xf1), ('exe', 16, 0xf1)]:
        assert color(shipped, ext, attr) == expected, (ext, attr)

    for ending in (b'\r', b'\n', b'\r\n'):
        for mode in range(3):
            data = ending.join([b'# comment', b'', b'filecoloruser\tYellow  abc ab a # tail',
                                b'Unknown Bogus stuff', b'FileColorUser LightRed ABC',
                                b'FileColorSys DarkGray', b'FileColorDef LightBlue',
                                f'DefaultSortMode {mode}'.encode(), b'FileColorUser Bogus XYZ'])
            result = parse(data)
            assert {key:rules(result)[key] for key in ('ABC','AB','A')} == {'ABC': 0xc1, 'AB': 0xe1, 'A': 0xe1}
            assert result[n['ncFileSystemColor']] == 0x81
            assert result[n['ncFileDefaultColor']] == 0x91
            assert result[n['state.panelA_sort']:n['state.panelA_sort']+2] == bytes([mode, mode])
            assert color(result, 'ab') == 0xe1
            assert color(result, 'ZZZ', 4) == 0x81
            assert color(result, 'ZZZ') == 0x91
            # A second file is a partial overlay, preserving unspecified keys.
            checked=parse(b'LeftSort 1\nFileColorUser White ABC\nLeftFirst 255\n',result)
            assert checked[n['state.panelA_sort']]==1
            assert checked[n['state.panelB_sort']]==mode
            assert checked[n['state.panelA_firstFile']]==255
            assert rules(checked)['ABC']==0xf1 and rules(checked)['AB']==0xe1
            assert checked[n['ncFileSystemColor']]==0x81

    for bad in (b'', b'9', b'-1', b'22', b'NAME'):
        result = parse(b'DefaultSortMode '+bad+b'\nFileColorUser White '+b'X'*256)
        assert result[n['ncDefaultSort']] == 2
        assert 'XXX' not in rules(result)
    data = b'FileColorUser Green '+b' '.join(f'{i:03}'.encode() for i in range(40))
    result = parse(data)
    assert len(rules(result)) == 32
    assert result[n['vars.fileColorRules']+128] == 0
    result=parse(b'FileColorUser Red COM\n',result)
    assert rules(result)['COM']==0x41,'overrides must still work at capacity'
    for key,maximum in [('ActivePanel',1),('LeftSort',2),('RightSort',2),
                        ('LeftInfo',1),('RightInfo',1),('LeftFirst',255),('RightFirst',255)]:
        field={'ActivePanel':'activePanel','LeftSort':'panelA_sort','RightSort':'panelB_sort',
               'LeftInfo':'panelA_info','RightInfo':'panelB_info','LeftFirst':'panelA_firstFile','RightFirst':'panelB_firstFile'}[key]
        for value in (0,maximum):
            result=parse(f'{key} {value}'.encode())
            assert result[n['state.'+field]]==value,(key,value)
        for value in (str(maximum+1),'999','256','-1','no'):
            result=parse(f'{key} {value}'.encode())
            assert result[n['state.'+field]]==memory[n['state.'+field]],(key,value)
    result=parse(b'LeftSelected Mixed.txt\nRightSelected Another.dir\n')
    assert result[n['vars.savedSelectedA']:n['vars.savedSelectedA']+10]==b'MIXED.TXT\0'
    assert result[n['vars.restoreSelectedA']:n['vars.restoreSelectedA']+2]==b'\1\1'
    result=parse(b'LeftSelected\n',result)
    assert result[n['vars.savedSelectedA']]==0 and result[n['vars.restoreSelectedA']]==0
    assert result[n['vars.restoreSelectedB']]==1
    result=parse(b'LeftSelected "#Mixed.txt"\nRightSelected ""\n')
    assert result[n['vars.savedSelectedA']:n['vars.savedSelectedA']+11]==b'#MIXED.TXT\0'
    assert result[n['vars.restoreSelectedB']]==0
    result=parse(b'LeftSelected "unfinished\n',result)
    assert result[n['vars.savedSelectedA']:n['vars.savedSelectedA']+11]==b'#MIXED.TXT\0'
    fallback = call(memory, 'ncConfigInit')
    path = n['vars.stateFile']
    selected = b'A:\\DOS\\NC-STATE.CFG\0'
    configured = parse(b'StateFile a:\\dos\\nc-state.cfg\n')
    assert configured[path:path+len(selected)] == selected
    for invalid in (b'relative.cfg', b'I:\\OTHER.CFG', b'A:FILE.CFG', b'A:\\'+b'X'*126):
        checked = parse(b'StateFile '+invalid, configured)
        assert checked[path:path+len(selected)] == selected
    configured[n['ncConfigStateLayer']] = 1
    checked = parse(b'StateFile B:\\OTHER.CFG', configured)
    assert checked[path:path+len(selected)] == selected
    assert color(fallback, 'EXE') == n['COLOR_EXECUTABLE']
    assert color(fallback, 'TXT') == n['COLOR_FILE']
    print('NC.CFG PASS: layered last-wins settings/colors, CR/LF/CRLF, comments, 32-rule bound, validation, numeric state and selected names')


if __name__ == '__main__':
    main()
