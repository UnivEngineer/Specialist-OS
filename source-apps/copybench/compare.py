"""Near/FAR, alignment, 8 KiB vs 32x256 and COPY lifetime; private model tests."""
from pathlib import Path
import argparse, hashlib, json, re, runpy, shutil, statistics, struct, subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(), re.M)}


def assemble(source, stem, defines=()):
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    *['-D'+x for x in defines], '--raw='+str(stem)+'.bin',
                    '--sym='+str(stem)+'.sym', '--lst='+str(stem)+'.lst', str(source)],
                   check=True, capture_output=True)
    return Path(str(stem)+'.bin').read_bytes(), symbols(Path(str(stem)+'.sym'))


def near(out):
    data, sy = assemble(HERE/'near-engines.asm', out/'near-engines')
    bs = symbols(out/'input/BIOS.sym')
    bios = (out/'input/BIOS.sys').read_bytes()
    rows = []
    algorithms = [('byte', sy['byteCopy'], False, False),
                  ('BIOS memmove', bs['bios_memcpy_bc_hl'], False, False),
                  ('BIOS forward', bs['moveBytesForward'], False, False),
                  ('stack256', sy['stack256'], True, False),
                  ('stack256 aligned', sy['alignedStack256'], True, True),
                  ('byte256 aligned x8', sy['alignedByte256'], True, True)]
    for name, entry, fixed, aligned in algorithms:
        for direction, src0, dst0 in [('LOW -> LOW', 0x2000, 0x5000),
                                     ('HIGH -> LOW', 0x9000, 0x3000)]:
            for so, do in [(0, 0), (1, 0), (0, 1), (1, 1), (0xf0, 0xfc)]:
                if aligned and do: continue
                for size, batch in [(256, False), (8192, False), (8192, True)]:
                    if fixed and size > 256 and not batch: continue
                    src, dst = src0+so, dst0+do
                    chunk = 256 if batch else size
                    # Equal parameter setup per call; fully expanded request series,
                    # not a hidden preparation loop outside the timed interval.
                    code = ['    ORG 01000h', '    ld sp,08F00h']
                    for at in range(0, size, chunk):
                        if name == 'BIOS memmove':
                            regs = [('hl', src+at), ('de', src+at+chunk), ('bc', dst+at)]
                        else:
                            regs = [('hl', src+at), ('de', dst+at), ('bc', chunk)]
                        code += [f'    ld {r},{v}' for r, v in regs]
                        code += [f'    call {entry}']
                    code += ['    jp 01800h']
                    (out/'near-case.asm').write_text('\n'.join(code)+'\n')
                    stub, _ = assemble(out/'near-case.asm', out/'near-case')
                    samples = []
                    for _ in range(3):
                        m = bytearray(65536)
                        m[0xc000:0xc000+len(bios)] = bios
                        m[0x8000:0x8000+len(data)] = data
                        payload = bytes((i*37+(i>>8)+19)&255 for i in range(size))
                        m[src:src+size] = payload
                        m[dst-1:dst+size+1] = b'\xa5'+bytes(size)+b'\x5a'
                        m[0x1000:0x1000+len(stub)] = stub
                        (out/'cpu-in.bin').write_bytes(m)
                        result = subprocess.run([
                            r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                            '-m8080', '-pc', '1000', '-end', '1800', '-counter', '4000000',
                            '-output', str(out/'cpu-out.bin'), str(out/'cpu-in.bin')],
                            check=True, capture_output=True, text=True)
                        actual = (out/'cpu-out.bin').read_bytes()
                        assert actual[65542:65544] == b'\x00\x18'
                        assert actual[65544:65546] == b'\x00\x8f'
                        assert actual[dst:dst+size] == payload, (name, so, do, size)
                        assert actual[src:src+size] == payload
                        assert actual[dst-1] == 0xa5 and actual[dst+size] == 0x5a
                        samples.append(int((result.stdout+result.stderr).strip()))
                    assert len(set(samples)) == 1
                    ticks = samples[0]
                    rows.append(dict(algorithm=name, direction=direction, source=src,
                                     destination=dst, size=size, batch256=batch,
                                     ticks=samples, KiB_s=size*2000000/ticks/1024,
                                     verified=True))
    (out/'near.json').write_text(json.dumps(rows, indent=2)+'\n')
    print('NEAR PASS', len(rows), 'rows x3', flush=True)


def far(out, runtime, port, algorithms=None):
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    C = h['Matrix']; g = C.__init__.__globals__
    state = out/'emulator'; state.mkdir(exist_ok=True)
    g.update(OUT=state, RUNTIME=runtime)
    g['mcp_owner'] = lambda: h['shell'](
        f'$p=Get-NetTCPConnection -State Listen -LocalPort {port} '
        '-ErrorAction SilentlyContinue; if($p){$p[0].OwningProcess};exit 0')
    x = C(); m = x.m; m.url = f'http://127.0.0.1:{port}/mcp'
    ts = symbols(out/'input/TRANSPORT.sym')
    variants = {}
    for name, chunk, aligned in [('local16', 16, 0), ('local32', 32, 0),
                                  ('local32 aligned256', 32, 1),
                                  ('local32 aligned batch', 32, 2)]:
        stem = out/name.replace(' ', '-')
        variants[name] = assemble(HERE/'cpm-local.asm', stem,
                                  [f'CHUNK={chunk}', f'ALIGNED={aligned}'])
    build_hashes = {name: hashlib.sha256(raw).hexdigest() for name, (raw, _) in variants.items()}
    build_hashes['MEM'] = hashlib.sha256(b''.join(
        (out/'input'/f'{name}.bin').read_bytes() for name in ['LOCALBLOCK', 'CLIENT', 'TRANSPORT'])).hexdigest()
    build_hashes['CPM original'] = hashlib.sha256((out/'input/CPMCOPY.bin').read_bytes()).hexdigest()
    manifest = out/'far-builds.json'
    if manifest.exists():
        previous = json.loads(manifest.read_text())
        assert all(previous.get(name) == build_hashes[name]
                   for name in (algorithms or build_hashes)), 'Changed binary: use a new report directory'
    else:
        assert not (out/'far.json').exists(), 'Missing binary manifest: use a new report directory'
        manifest.write_text(json.dumps(build_hashes, indent=2)+'\n')
    rows = json.loads((out/'far.json').read_text()) if (out/'far.json').exists() else []

    def select(page):
        m.write(0xfffe if page & 128 else (0xfffd if page else 0xfffc),
                bytes([page & 15 if page & 128 else (page-1 if page else 0)]))

    def install(name):
        if name == 'MEM':
            for page in (0, 1):
                select(page)
                for stem, origin in [('LOCALBLOCK', ts['MEM_IMAGE_LOCAL_START']),
                                     ('CLIENT', 0xf800), ('TRANSPORT', 0xfc00)]:
                    m.write(origin, (out/'input'/f'{stem}.bin').read_bytes())
                m.write(ts['bankPage'], bytes([page]))
                m.write(ts['bankPageCount'], bytes([16]))
            raw = (out/'input/TRANSPORT.bin').read_bytes()
            for page in (2, 3):
                select(page); m.write(0xff00, raw[0x300:ts['bankCopyTailEnd']-0xfc00])
        elif name == 'CPM original':
            for page in (0, 1, 2, 3):
                select(page); m.write(0xff00, (out/'input/CPMCOPY.bin').read_bytes())
        else:
            raw, sy = variants[name]
            for page in (0, 1):
                select(page)
                m.write(0x7000, raw[:sy['tailImage']-0x7000])
                m.write(sy['callerPage'], bytes([page]))
            for page in (0, 1, 2, 3):
                select(page); m.write(0xff00, raw[sy['tailImage']-0x7000:])
        select(0)

    def program(name, caller, spage, src, dpage, dst, size, mode):
        chunk = size if mode == 'whole' else 256
        code = ['    ORG 01000h', '    ld sp,08F00h']
        if name == 'CPM original':
            for at in range(0, size, chunk):
                for address, value in [(0xffd0, src+at), (0xffd2, dst+at),
                                       (0xffd4, chunk//16)]:
                    code += [f'    ld hl,{value}', f'    ld ({address}),hl']
                for address, value in [(0xffd6, spage-1 if spage else 255),
                                       (0xffd7, dpage-1 if dpage else 255),
                                       (0xffd8, caller-1 if caller else 255)]:
                    code += [f'    ld a,{value}', f'    ld ({address}),a']
                code += ['    call 0FF00h']
        else:
            sy = ts if name == 'MEM' else variants[name][1]
            entry = (ts['bankCopyPhysical'] if spage & 128 else ts['bankCopyFar']) if name == 'MEM' else sy['copy']
            restore = ts['bankCommandWindow'] if name == 'MEM' else sy['restoreCommand']
            for at in range(0, size, chunk):
                # All per-request packet construction is inside the measured interval.
                for offset, value in [(1, src+at), (4, dst+at), (6, chunk)]:
                    code += [f'    ld hl,{value}', f'    ld (request+{offset}),hl']
                for offset, value in [(0, spage), (3, dpage)]:
                    code += [f'    ld a,{value}', f'    ld (request+{offset}),a']
                code += ['    ld hl,request', f'    call {entry}', '    jp c,01801h']
                if mode == 'command256': code += [f'    call {restore}']
            if mode != 'command256': code += [f'    call {restore}']
        code += ['    jp 01800h', 'request: DS 8,0']
        (out/'far-case.asm').write_text('\n'.join(code)+'\n')
        return assemble(out/'far-case.asm', out/'far-case')[0]

    try:
        x.start('copyben-compare'); m.call('debug_break'); m.call('bp_clear')
        # The following is an ABI experiment. No OS/file/UI calls after tail replacement.
        directions = [('MAIN -> DATA', 0, 0, 3, 0x4000),
                      ('DATA -> MAIN', 0, 3, 0, 0x4000),
                      ('DATA -> DATA, third caller', 1, 3, 2, 0x4000),
                      ('ROM -> DATA', 1, 0x80, 3, 0x4000),
                      ('ROM -> MAIN HIGH', 0, 0x80, 0, 0x9000)]
        for name in (algorithms or ['MEM', 'CPM original', *variants]):
            install(name)
            for direction, caller, spage, dpage, dst0 in directions:
                if name == 'CPM original' and spage & 128: continue
                for so, do in [(0, 0), (1, 0), (0, 1), (1, 1), (0xf0, 0xfc)]:
                    if 'aligned' in name and so: continue
                    src, dst = 0x2000+so, dst0+do
                    for size, mode in [(256, 'whole'), (8192, 'whole'),
                                       (8192, 'copy256'), (8192, 'command256')]:
                        if name == 'CPM original' and mode == 'command256': continue
                        if 'aligned256' in name and size > 256 and mode == 'whole': continue
                        if any(r['algorithm'] == name and r['caller'] == caller
                               and r['source'] == [spage, src] and r['destination'] == [dpage, dst]
                               and r['size'] == size and r['mode'] == mode for r in rows): continue
                        code = program(name, caller, spage, src, dpage, dst, size, mode)
                        samples = []
                        for _ in range(3):
                            select(spage)
                            payload = (m.read(src, size) if spage & 128 else
                                       bytes((i*19+(i>>8)+37)&255 for i in range(size)))
                            if not spage & 128: m.write(src, payload)
                            select(dpage); m.write(dst-1, b'\xa5'+bytes(size)+b'\x5a')
                            select(caller)
                            if name in variants:
                                assert m.read(0x7000, 6) == variants[name][0][:6], (name, caller, 'front overwritten before call')
                                assert m.read(variants[name][1]['callerPage'], 1) == bytes([caller])
                            initial = bytes(range(31))+b'\0'
                            m.write(0xffc0, initial); m.write(0x1000, code)
                            m.call('bp_clear')
                            for pc in (0x1800, 0x1801): m.call('bp_set', addr=pc, type='exec')
                            m.call('regs_set', pc=0x1000, sp=0x8f00)
                            start = m.call('cpu_state')['cpu_ticks']
                            for _wait in range(20):
                                st = m.call('cpu_state'); pc = st['registers']['pc']['dec']
                                if pc in (0x1800, 0x1801): break
                                m.call('emu_run_for', ms=1000)
                            assert pc == 0x1800, (name, direction, so, do, size, mode, st)
                            samples.append(st['cpu_ticks']-start)
                            assert st['registers']['sp']['dec'] == 0x8f00
                            if name != 'CPM original': assert m.read(0xffc0, 32) == initial
                            select(dpage); assert m.read(dst, size) == payload
                            assert m.read(dst-1, 1) == b'\xa5' and m.read(dst+size, 1) == b'\x5a'
                            select(spage); assert m.read(src, size) == payload
                        assert len(set(samples)) == 1, (name, samples)
                        rows.append(dict(algorithm=name, direction=direction, caller=caller,
                                         source=[spage, src], destination=[dpage, dst], size=size,
                                         mode=mode, ticks=samples, verified=True,
                                         KiB_s=size*2000000/samples[0]/1024))
                        (out/'far.json').write_text(json.dumps(rows, indent=2)+'\n')
            print('FAR PASS', name, len(rows), 'rows total', flush=True)
    except Exception:
        m.call('debug_break')
        for page in (0, 1, 2, 3):
            select(page)
            for address, length in [(0x7000, 0x800), (0x8e00, 0x200), (0xff00, 0x100)]:
                (out/f'error-{page}-{address:04x}.bin').write_bytes(m.read(address, length))
        raise
    finally:
        x.stop()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--runtime', type=Path, default=ROOT/'.local/rom-pio-19782')
    p.add_argument('--port', type=int, default=19782)
    p.add_argument('--stage', choices=['near', 'far', 'all'], default='all')
    p.add_argument('--resume', action='store_true')
    p.add_argument('--algorithms', nargs='+',
                   choices=['MEM', 'CPM original', 'local16', 'local32',
                            'local32 aligned256', 'local32 aligned batch'])
    a = p.parse_args(); out = a.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    frozen = out/'input'; frozen.mkdir(exist_ok=True)
    for name in ['BIOS.sys', 'BIOS.sym', 'DOS.sys', 'NC.i80', 'NC.sym']:
        if not (frozen/name).exists(): shutil.copyfile(ROOT/'bin'/name, frozen/name)
    for name in ['TRANSPORT', 'CLIENT', 'LOCALBLOCK', 'WINDOW']:
        for ext in ['bin', 'sym']:
            if not (frozen/f'{name}.{ext}').exists():
                shutil.copyfile(ROOT/f'bin/banked/{name}.{ext}', frozen/f'{name}.{ext}')
    assemble(HERE/'CPMCOPY.asm', out/'CPMCOPY')
    shutil.copyfile(out/'CPMCOPY.bin', frozen/'CPMCOPY.bin')
    if a.stage in ('near', 'all'):
        if (out/'near.json').exists(): p.error('near.json exists; choose a new report')
        near(out)
    if a.stage in ('far', 'all'):
        if (out/'far.json').exists() and not a.resume: p.error('far.json exists; choose a new report')
        far(out, a.runtime.resolve(), a.port, a.algorithms)
    (out/'hashes.json').write_text(json.dumps({f.name: hashlib.sha256(f.read_bytes()).hexdigest()
                                              for f in frozen.iterdir() if f.is_file()}, indent=2)+'\n')


if __name__ == '__main__': main()
