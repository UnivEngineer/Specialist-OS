"""NC index sorting on an owned visible, muted Emu80; timings from PIT only.

The generated Shell implementation comes from the current NC sort.inc.
No production NC files are modified. Results and the report live in docs/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import runpy
import shutil
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'), re.M)}


def upper(c):
    # Exact current BIOS KOI8 conversion, including the historical yo range.
    return c if c < 0x61 else c - 32 if c < 0x7b else (
        c + 16 if c < 0xa3 else c if c < 0xc0 or c >= 0xe0 else c + 32)


def rank(row):
    if not row[11] & 16:
        return 3
    return 0 if row[:2] == b'\\ ' else 1 if row[:3] == b'.. ' else 2


def key(row, mode):
    name, ext = bytes(map(upper, row[:8])), bytes(map(upper, row[8:11]))
    return (rank(row), name, ext) if mode == 1 else (rank(row), ext, name)


def record(name, ext='', directory=False):
    return (name.encode('koi8-r').ljust(8, b' ') + ext.encode('koi8-r').ljust(3, b' ') +
            bytes([16 if directory else 0]) + bytes(4))


def build(out, profile=False):
    source = (ROOT/'source-apps/sortbench/comparison.inc').read_text(encoding='utf-8-sig')
    fixture = (ROOT/'source-apps/sortbench/kernels.inc').read_text(encoding='utf-8')
    fixture = fixture.replace('@CORE@', 'sort-under-test.inc')
    fixture = fixture.replace('@TIMING@', (ROOT/'source/include/timing-pit.inc').as_posix())
    if profile:
        def count(label, counter, text):
            code = ('    push hl\n    ld hl,(' + counter + ')\n    inc hl\n'
                    '    ld (' + counter + '),hl\n    pop hl\n')
            return text.replace(label+':\n', label+':\n'+code)
        source = count('ncCompareNameFiles', 'profileComparisons', source)
        source = count('ncCompareExtFiles', 'profileComparisons', source)
        source = count('ncComparePart', 'profileCharacters', source)
        fixture = count('benchSwapIJ', 'profileQuickSwaps', fixture)
        fixture = count('benchQuickRange', 'profileQuickCalls', fixture)
        fixture += '\nprofileComparisons: DW 0\nprofileCharacters: DW 0\nprofileQuickSwaps: DW 0\nprofileQuickCalls: DW 0\n'
    result = {}
    for prepared in (0, 1):
        folder = out/str(prepared)
        folder.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT/'source-apps/sortbench/sort-radix.inc', folder/'sort-radix.inc')
        (folder/'sort-under-test.inc').write_text(
            source.replace('    call bios_upperCase', '    ; normalized key') if prepared else source,
            encoding='utf-8')
        assembly = ('    INCLUDE "' + (ROOT/'source/include/mxos.inc').as_posix() + '"\n'
                    '    ORG 0100h\nPREUPPER = ' + str(prepared) + '\n' + fixture)
        (folder/'bench.asm').write_text(assembly, encoding='utf-8')
        run = subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                              '--lst=bench.lst', '--sym=bench.sym', '--raw=bench.bin', 'bench.asm'],
                             cwd=folder, capture_output=True, text=True)
        if run.returncode:
            raise RuntimeError(run.stdout + run.stderr)
        result[prepared] = (folder/'bench.bin').read_bytes(), symbols(folder/'bench.sym')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT/'docs/nc/sort-optimized-2026-11-11')
    p.add_argument('--runtime-source', type=Path,
                   default=Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test'))
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--prepare-only', action='store_true')
    p.add_argument('--algorithms', nargs='+', default=['shell', 'insertion', 'merge', 'quick', 'radix-opt'],
                   choices=['shell', 'insertion', 'merge', 'radix', 'quick', 'radix-columns', 'radix-opt'])
    p.add_argument('--append', action='store_true')
    p.add_argument('--baseline-only', action='store_true',
                   help='Time the unchanged buildFileIndex from the booted NC on the actual H: root')
    p.add_argument('--fill-h', action='store_true', help='Fill test H: to 250 entries, including 12 empty directories')
    p.add_argument('--h-only', action='store_true', help='Measure only the actual test H: root')
    p.add_argument('--sizes', nargs='+', type=int, choices=range(2,256),
                   help='Measure only independently seeded random sets of these sizes')
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    builds = build(args.output/'build')
    if args.prepare_only:
        print('Both 8080 benchmark variants assembled.')
        return
    helpers = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    runtime = ROOT/'.local/nc-sort-benchmark-runtime'
    if helpers['mcp_owner']():
        raise RuntimeError('MCP port already occupied; no emulator started')
    if not runtime.exists():
        shutil.copytree(args.runtime_source, runtime,
                        ignore=shutil.ignore_patterns('specialist-os', '_settings'))
    matrix_class = helpers['Matrix']
    matrix_class.__init__.__globals__.update(RUNTIME=runtime, OUT=args.output)
    matrix = matrix_class()
    results = json.loads((args.output/'results.json').read_text())['results'] if args.append else []
    report = dict(cpu_hz=2000000, pit_hz=2000000, pit_tick_ms=0.128,
                  repeats=args.repeats, results=results,
                  source_sha256=hashlib.sha256((ROOT/'source-apps/sortbench/comparison.inc').read_bytes()).hexdigest())
    try:
        disk = ROOT/'images/FlashDisk_emu80_seed.bin'
        if args.fill_h:
            disk = runpy.run_path(str(ROOT/'scripts/emu80/nc-sort-image.py'))['make_full_h'](args.output/'H-sort250.bin')
        matrix.start('sort', disk=disk)
        m = matrix.m
        m.call('debug_break')
        nc = symbols(ROOT/'bin/NC.sym')
        if 'NC_FONT_END' not in nc:
            bios_symbols = symbols(ROOT/'bin/BIOS.sym')
            assert m.read(bios_symbols['fast6.preBase'], 2) == b'\0\0', 'NC enabled prerender'
            (args.output/'nc-memory.json').write_text(json.dumps(dict(
                nc_end=nc['NC_END'], memory_end=nc['APP_MEMORY_END'],
                free_bytes=nc['APP_MEMORY_END']-nc['NC_END'],
                prerender_base=0, nc_sha256=hashlib.sha256((ROOT/'bin/NC.i80').read_bytes()).hexdigest()), indent=2)+'\n')
        meta = m.read(nc['ncCatalogs'], 24)
        suites = {}
        h_name = 'H-root-250' if args.fill_h else 'H-root'
        for slot in range(3):
            entry = meta[slot*8:slot*8+8]
            if entry[0] and entry[1] == 7 and entry[2:4] == b'\0\0':
                rows = m.read(nc[f'FILE_CACHE_{"ABC"[slot]}'], entry[4]*16)
                suites[h_name] = [rows[i:i+16] for i in range(0, len(rows), 16)]
        if h_name not in suites:
            raise RuntimeError('NC did not cache the actual H: root')
        if args.fill_h:
            assert len(suites[h_name]) == 250
            assert sum(bool(row[11]&16) for row in suites[h_name]) >= 12
        if args.fill_h and not args.baseline_only:
            # Verify exotic directory names through the real NC keyboard path.
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)
            m.tap('esc')
            if m.read(nc['state.activePanel'], 1) == b'\0':
                m.tap('tab')
            assert m.read(nc['state.activePanel'], 1) == b'\x01'
            slot = m.read(nc['vars.cacheSlotB'], 1)[0]
            raw_list = m.read(nc[f'FILE_CACHE_{"ABC"[slot]}'], 250*16)
            indices = m.read(nc['FILE_INDEX_B'], 250)
            position = next(i for i,original in enumerate(indices)
                            if raw_list[original*16:original*16+11] == b'SRTDIR00ASM')
            current = m.read(nc['state.panelB_curFile'], 1)[0]
            for _ in range(abs(position-current)):
                m.tap('down' if position > current else 'up')
                m.call('emu_run_for', ms=250)
            assert m.read(nc['state.panelB_curFile'], 1)[0] == position
            chosen = indices[position]
            entry = raw_list[chosen*16:chosen*16+16]
            assert entry[:11] == b'SRTDIR00ASM' and entry[11]&16, entry.hex()
            m.stop_at(nc['mainLoop'])
            m.tap('enter')
            m.until(nc['mainLoop'])
            cluster = int.from_bytes(m.read(nc['state.panelB_dirLow'], 1)+
                                     m.read(nc['state.panelB_dirHigh'], 1), 'little')
            assert cluster != 0
            assert m.read(nc['state.panelB_filesCnt'], 1) == b'\x02'
            m.screenshot(args.output/'empty-directory-with-extension.png')
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)
            m.stop_at(nc['mainLoop'])
            m.tap('enter')
            m.until(nc['mainLoop'])
            assert m.read(nc['state.panelB_dirLow'], 1) == b'\0'
            assert m.read(nc['state.panelB_dirHigh'], 1) == b'\0'
            assert m.read(nc['state.panelB_filesCnt'], 1) == bytes([250])
            m.screenshot(args.output/'H-root-250.png')
            (args.output/'directory-ui.json').write_text(json.dumps(dict(
                name='SRTDIR00.ASM', cluster=cluster, opened_by_keyboard=True,
                empty_directory_entries=2, returned_to_root=True, root_entries=250), indent=2)+'\n')
            m.call('debug_break')
        if args.baseline_only:
            folder = args.output/'build/actual'
            folder.mkdir(parents=True, exist_ok=True)
            assembly = (args.output/'build/0/bench.asm').read_text()
            assembly = assembly.replace('ORG 0100h', 'ORG 07800h')
            assembly = assembly.replace('    call benchFill', '    ; original NC fills its own index')
            assembly = assembly.replace('    call ncSortPass', f"    call {nc['buildFileIndex']}", 1)
            assembly = assembly.replace('    ASSERT $ < benchRaw', '    ASSERT $ < 08A00h')
            (folder/'bench.asm').write_text(assembly)
            shutil.copyfile(args.output/'build/0/sort-under-test.inc', folder/'sort-under-test.inc')
            shutil.copyfile(ROOT/'source-apps/sortbench/sort-radix.inc', folder/'sort-radix.inc')
            run = subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                                  '--sym=bench.sym', '--raw=bench.bin', 'bench.asm'], cwd=folder,
                                 capture_output=True, text=True)
            if run.returncode:
                raise RuntimeError(run.stdout+run.stderr)
            s = symbols(folder/'bench.sym')
            m.write(0x7800, (folder/'bench.bin').read_bytes())
            rows = suites[h_name]
            data = b''.join(rows)+b'\xff'
            m.write(nc['FILE_CACHE_A'], data)
            m.write(nc['vars.cacheSlotA'], b'\0')
            m.write(nc['state.activePanel'], b'\0')
            m.write(nc['state.panelA_filesCnt'], bytes([len(rows)]))
            actual = []
            for mode in (1, 2):
                m.write(nc['state.panelA_sort'], bytes([mode]))
                ticks = []
                for _ in range(args.repeats):
                    m.stop_at(s['benchDone'])
                    m.call('regs_set', pc=s['benchRun'], sp=0x8B00)
                    m.until(s['benchDone'], 60)
                    ticks.append(int.from_bytes(m.read(s['benchElapsed'], 2), 'little'))
                    expected = bytes(sorted(range(len(rows)), key=lambda i:key(rows[i], mode)))+b'\xff'
                    assert m.read(nc['FILE_INDEX_A'], len(expected)) == expected
                    assert m.read(nc['FILE_CACHE_A'], len(data)) == data
                actual.append(dict(mode=mode, count=len(rows), ticks=ticks,
                                   median_ms=statistics.median(ticks)*0.128))
            (args.output/('actual-nc-250.json' if args.fill_h else 'actual-nc.json')).write_text(json.dumps(actual, indent=2)+'\n')
            print('Original NC buildFileIndex:', actual, flush=True)
            return
        rng = random.Random(8080)
        for count in (38, 128, 255):
            rows = [record(f'File{i:04}', ('ASM', 'TXT', 'COM', '')[i % 4], i % 9 == 0)
                    for i in range(count)]
            rng.shuffle(rows)
            suites[f'random-{count}'] = rows
        rows = [record(f'File{i:04}', 'TXT', i % 9 == 0) for i in range(255)]
        suites['sorted-255'] = sorted(rows, key=lambda r: key(r, 2))
        suites['reverse-255'] = list(reversed(suites['sorted-255']))
        suites['ties-koi8'] = [record('\\', directory=True), record('..', directory=True),
                              record('я', 'TXT'), record('Я', 'ASM'), record('same', 'TXT'),
                              record('SAME', 'TXT'), record('Dir', 'ASM', True),
                              record('dir', 'TXT', True), record('ё', 'txt'), record('Ё', 'TXT')]
        if args.h_only:
            suites = {h_name:suites[h_name]}
        if args.sizes:
            suites = {}
            for count in args.sizes:
                rows = [record(f'File{i:04}', ('ASM', 'TXT', 'COM', '')[i % 4], i % 9 == 0)
                        for i in range(count)]
                random.Random(8080).shuffle(rows)
                suites[f'small-random-{count}'] = rows
        report['datasets'] = json.loads((args.output/'results.json').read_text()).get('datasets', {}) if args.append else {}
        report['datasets'].update({name: {'count': len(rows), 'records_hex': b''.join(rows).hex()}
                                  for name, rows in suites.items()})

        def invoke(address, s):
            m.stop_at(s['benchDone'])
            m.call('regs_set', pc=address, sp=0x8B00)
            m.until(s['benchDone'], 60)
            return int.from_bytes(m.read(s['benchElapsed'], 2), 'little')

        # Snapshot before MAIN is replaced by the isolated harness.
        m.screenshot(args.output/'nc-before.png')
        for prepared, (binary, s) in builds.items():
            m.write(0x100, binary)
            for name, rows in suites.items():
                data = b''.join(rows)
                m.write(s['benchRaw'], data)
                m.write(s['state.panelA_filesCnt'], bytes([len(rows)]))
                prep_ticks = []
                if prepared:
                    m.write(s['benchDivisorHigh'], b'\x01')
                    for _ in range(args.repeats):
                        prep_ticks.append(invoke(s['benchPrepareRun'], s))
                    normalized = m.read(s['benchNormalized'], len(data))
                    for i, row in enumerate(rows):
                        assert normalized[i*16:i*16+11] == bytes(map(upper, row[:11]))
                        assert normalized[i*16+12] == rank(row)
                for mode in (1, 2):
                    m.write(s['state.panelA_sort'], bytes([mode]))
                    expected = bytes(sorted(range(len(rows)), key=lambda i: key(rows[i], mode)))
                    for algorithm, entry in [('shell', 'ncSortPass'), ('insertion', 'benchInsertion'),
                                             ('merge', 'benchMerge'), ('radix', 'benchRadix'), ('quick', 'benchQuick'),
                                             ('radix-columns', 'benchRadix'), ('radix-opt', 'ncRadixSort')]:
                        if algorithm not in args.algorithms:
                            continue
                        if algorithm == 'radix-columns' and not prepared:
                            continue
                        if algorithm == 'radix-opt' and prepared:
                            continue
                        result_prepared = bool(prepared) or algorithm == 'radix-opt'
                        if any(r['dataset'] == name and r['prepared'] == result_prepared
                               and r['mode'] == mode and r['algorithm'] == algorithm for r in results):
                            continue
                        divisor_high = 16 if algorithm == 'insertion' else 1
                        m.write(s['benchDivisorHigh'], bytes([divisor_high]))
                        current_prep = prep_ticks
                        if algorithm == 'radix-opt':
                            m.write(s['benchPrepareInvoke']+1, s['benchOptimizedPrepare'].to_bytes(2, 'little'))
                            current_prep = [invoke(s['benchPrepareRun'], s) for _ in range(args.repeats)]
                            m.write(s['benchPrepareInvoke']+1, s['benchPrepare'].to_bytes(2, 'little'))
                        if algorithm == 'radix-columns':
                            m.write(s['benchPrepareInvoke']+1, s['benchPreparePlanes'].to_bytes(2, 'little'))
                            current_prep = [invoke(s['benchPrepareRun'], s) for _ in range(args.repeats)]
                            m.write(s['benchPrepareInvoke']+1, s['benchPrepare'].to_bytes(2, 'little'))
                            m.write(s['benchRadixKey'], b'\xc3'+s['benchPlaneKey'].to_bytes(2, 'little'))
                        else:
                            m.write(s['benchRadixKey'], b'\x3a'+s['benchI'].to_bytes(2, 'little'))
                        m.write(s['benchInvoke']+1, s[entry].to_bytes(2, 'little'))
                        ticks = []
                        for _ in range(args.repeats):
                            ticks.append(invoke(s['benchRun'], s))
                            got = m.read(s['FILE_INDEX_A'], len(rows)+1)
                            assert got == expected+b'\xff', (prepared, name, mode, algorithm, got.hex(), expected.hex())
                            assert m.read(s['benchRaw'], len(data)) == data, 'source changed'
                        results.append(dict(dataset=name, count=len(rows), prepared=result_prepared,
                                            mode=mode, algorithm=algorithm, ticks=ticks,
                                            tick_ms=divisor_high*0.128,
                                            median_ms=statistics.median(ticks)*divisor_high*0.128,
                                            preparation_ticks=current_prep,
                                            preparation_ms=statistics.median(current_prep)*0.128 if current_prep else 0))
                        if algorithm == 'radix-columns':
                            m.write(s['benchNormalized'], normalized)
                        (args.output/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
                (args.output/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
                print('PASS', 'normalized' if prepared else 'raw', name, len(rows), flush=True)
        print('PIT benchmark complete:', len(results), 'configurations', flush=True)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
