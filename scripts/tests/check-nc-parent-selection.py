"""Actual 8080 parent navigation: identify the leaf after sorting, reveal it."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-parent-cpu'
W = lambda value: value.to_bytes(2, 'little')


def symbols(path):
    return {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'), re.M)}


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    n = symbols(ROOT / 'bin/NC.sym')
    b = symbols(ROOT / 'bin/BIOS.sym')

    def call(memory, entry):
        memory = bytearray(memory)
        code = b'\x31' + W(0x8b00) + b'\xcd' + W(n[entry]) + b'\xc3' + W(0x81ff)
        memory[0x8100:0x8100+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', '8100', '-end', '81ff', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'output.bin').read_bytes()
        assert result[65542:65544] == W(0x81ff), (entry, result[65542:65544].hex())
        assert result[65544:65546] == W(0x8b00), (entry, 'stack imbalance')
        return bytearray(result[:65536])

    def seed(old, new, panel=0):
        memory = bytearray(65536)
        for address, file in [(0, 'NC.i80'), (0xc000, 'BIOS.sys'), (0xf000, 'IO.sys')]:
            data = (ROOT / 'bin' / file).read_bytes()
            memory[address:address+len(data)] = data
        memory[n['getPanelPath']:n['getPanelPath']+5] = b'\x21' + W(0x8600) + b'\xb7\xc9'
        memory[n['fileCacheBase']:n['fileCacheBase']+4] = b'\x21' + W(0x7000) + b'\xc9'
        for address, value in [(n['vars.path'], old), (0x8600, new)]:
            data = value.encode('koi8-r') + b'\0'
            memory[address:address+len(data)] = data
        memory[n['state.activePanel']] = panel
        memory[n['vars.cmdLineEnd']:n['vars.cmdLineEnd']+2] = W(n['vars.cmdLine'])
        return memory

    def record(leaf, directory=True):
        name, _, ext = leaf.partition('.')
        return name.encode('koi8-r').ljust(8, b' ') + ext.encode('koi8-r').ljust(3, b' ') + bytes([16 if directory else 0]) + bytes(4)

    # No selection request for entering a child, the same folder, another drive,
    # an ancestor two levels up, or a same-prefix sibling (FOO vs FOOBAR).
    for old, new in [('A:\\', 'A:\\SUB'), ('A:\\SUB', 'A:\\SUB'),
                     ('A:\\SUB', 'H:\\'), ('A:\\FOOBAR\\SUB', 'A:\\FOO'),
                     ('A:\\ONE\\TWO', 'A:\\')]:
        memory = call(seed(old, new), 'ncParentPrepare')
        assert memory[n['ncParentPending']] == 0, (old, new)

    cases = 0
    for old, new, leaf in [('H:\\MiXeD.ext', 'H:\\', 'MIXED.EXT'),
                            ('A:\\TOP\\MiXeD.ext', 'A:\\TOP', 'MIXED.EXT'),
                            ('A:\\TOP\\12345678.abc', 'A:\\top', '12345678.ABC')]:
        for panel in (0, 1):
            for mode in (0, 1, 2):
                for visible in (19, 38, 57):
                    memory = seed(old, new, panel)
                    # Includes a same-named file first: attribute must be checked.
                    rows = [record(leaf, False)]
                    rows += [record(f'D{i:07d}.{"ZIP" if i%2 else "AAA"}') for i in range(100)]
                    rows += [record(leaf), record('ZZZZZZZZ.ZZZ')]
                    original = b''.join(rows)
                    memory[0x7000:0x7000+len(original)] = original
                    memory[n['state.panelA_filesCnt']+panel] = len(rows)
                    memory[n['state.panelA_sort']+panel] = mode
                    memory[n['ncVisibleCount']] = visible
                    memory[n['state.panelA_curFile']+(panel^1)] = 17
                    memory[n['state.panelA_firstFile']+(panel^1)] = 3
                    memory = call(memory, 'ncParentPrepare')
                    assert memory[n['ncParentPending']] == 1, (old, new)
                    expected_name = leaf.encode('koi8-r') + b'\0'
                    assert memory[n['ncParentName']:n['ncParentName']+len(expected_name)] == expected_name
                    memory = call(memory, 'directoryResetSelection')
                    memory = call(memory, 'buildFileIndex')
                    index = n['FILE_INDEX_A'] if panel == 0 else n['FILE_INDEX_B']
                    order = list(memory[index:index+len(rows)])
                    selected = order.index(101)
                    memory = call(memory, 'lf_Cached')
                    assert memory[n['state.panelA_curFile']+panel] == selected, (panel, mode, visible, selected)
                    first = memory[n['state.panelA_firstFile']+panel]
                    assert first <= selected < first+visible, (selected, first, visible)
                    assert int.from_bytes(memory[n['vars.fileListPtr']:n['vars.fileListPtr']+2], 'little') == index+first
                    assert memory[n['ncParentPending']] == 0
                    assert memory[0x7000:0x7000+len(original)] == original
                    assert memory[n['state.panelA_curFile']+(panel^1)] == 17
                    assert memory[n['state.panelA_firstFile']+(panel^1)] == 3
                    cases += 1

    # Missing/deleted folder and an empty parent consume the request safely.
    for count in (0, 1):
        memory = seed('A:\\GONE', 'A:\\')
        memory[0x7000:0x7010] = record('OTHER')
        memory[n['state.panelA_filesCnt']] = count
        memory[n['ncVisibleCount']] = 19
        memory = call(memory, 'ncParentPrepare')
        memory = call(memory, 'directoryResetSelection')
        memory = call(memory, 'buildFileIndex')
        memory = call(memory, 'lf_Cached')
        assert memory[n['state.panelA_curFile']] == 0
        assert memory[n['state.panelA_firstFile']] == 0
        assert memory[n['ncParentPending']] == 0
    print(f'Parent navigation PASS: {cases} actual 8080 cases, both panels, all sort modes and viewport sizes; missing/empty parent; immutable records')


if __name__ == '__main__':
    main()
