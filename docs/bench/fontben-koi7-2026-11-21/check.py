"""Measure KOI-7R rendering and FONT installation on private Emu80 slots."""
import base64
import http.client
from pathlib import Path
from urllib.parse import urlsplit

report = Path(__file__).resolve().parent
base_harness = report.parent/'fontben-alternating-2026-11-21/check.py'
transport_harness = report.parent/'remap32-modern-2026-11-21/check.py'
shared, scenarios = base_harness.read_text(encoding='utf-8').split('resident = (', 1)
shared = shared.replace("parent / f'worker-{args.group}'", "parent / 'packed-unroll' / f'worker-{args.group}'")
shared = shared.replace("out.mkdir(exist_ok=True)", "out.mkdir(parents=True, exist_ok=True)")
transport = transport_harness.read_text(encoding='utf-8').split("transport = '''", 1)[1].split("'''", 1)[0]
shared = shared.replace("m = Bench(f'http://127.0.0.1:{args.port}/mcp')", transport)
shared = shared[:shared.index('def font(width, name):')] + '''
def font(width, name):
    assert width == 6
    raw = (root/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
    return bytes(b&63 for b in raw[:0x300]+raw[0x700:0x7F8]+bytes(8)+raw[0x400:])

def koi7(code):
    if code >= 0xC0:
        return 0x60 | (code & 31)
    if code in (0xA3, 0xB3):
        return 0x65
    if code >= 0x80:
        return 0x20
    if 0x60 <= code < 0x7F:
        return code & 0xDF
    return code
''' + shared[shared.index('def oracle('):]
shared = shared.replace('    current = command(line, bench_stops, tag)', '''
    normalized = {bench['gridEngineNext']:'FONTCOM', **bench_stops}
    current = command(line, normalized, tag)
    assert current == bench['gridEngineNext'], (tag, current)
    # The shell restores its encoding when a COM exits. Exercise the
    # public encoding entry inside this disposable FontBen invocation.
    state = m.call('cpu_state')['registers']
    return_to = state['pc']['dec']
    stack = state['sp']['dec']-2
    saved = m.read(stack,2)
    m.write(stack,return_to.to_bytes(2,'little'))
    returns = {return_to:'FONTCOM'}
    breaks(returns)
    m.call('regs_set',pc=bios['bios_setFontEncoding'],sp=stack,af=0x0100)
    m.call('emu_run')
    assert await_stop(returns)==return_to
    m.write(stack,saved)
    m.call('regs_set',af=state['af']['dec'])
    assert m.read(bios['bios_upper.koi7'],1)==b'\\xff'
    text_addr = bench['gridText']
    text = bytes(koi7(code) for code in m.read(text_addr,924))
    m.write(text_addr, text)
    breaks(bench_stops)
    current = onwards(bench_stops)''')
boot = 'assert not owner()' + scenarios.split('assert not owner()', 1)[1]
boot = boot.split('    for name, path in resident[args.group]:', 1)[0]
body = '''
    def tick_scale():
        saved_pc = m.call('cpu_state')['registers']['pc']['dec']
        address = 0x3100
        saved = m.read(address,1)
        m.write(address,b'\\0')
        m.call('regs_set',pc=address)
        start = m.call('cpu_state')['cpu_ticks']
        m.call('debug_step',mode='into')
        state = m.call('cpu_state')
        assert state['registers']['pc']['dec']==address+1
        m.write(address,saved)
        m.call('regs_set',pc=saved_pc)
        return (state['cpu_ticks']-start)/4

    def timed_install(path, prepared=False):
        entry = font_utility['start']
        command_line = 'FONT 6x8/' + path + (' -FILE' if prepared else '')
        current = command(command_line, {entry:'FONT',**cli_stop}, 'load-'+path)
        assert current == entry, (command_line, current)
        t0 = m.call('cpu_state')['cpu_ticks']
        committed = font_utility['committed']
        endpoints = {committed:'FONT',**cli_stop}
        breaks(endpoints)
        assert onwards(endpoints)==committed, command_line
        elapsed = m.call('cpu_state')['cpu_ticks']-t0
        assert not m.read(font_utility['preparedError'],1)[0], command_line
        breaks(cli_stop)
        onwards(cli_stop)
        return elapsed

    loads = []
    scale = tick_scale()
    profiles = (('Rotate','K7Rotate'),('Packed','K7Packed'),('Fast','K7Hybrid'))
    if args.group < 3:
        profile,replacement = profiles[args.group]
        old = timed_install(profile,profile!='Rotate')
        benchmark('',f'6x8/{profile} KOI-7R')
        new = timed_install(replacement)
        assert m.read(bios['bios_upper.koi7'],1)==b'\\xff'
        benchmark('',f'6x8/{replacement}')
        loads.append(dict(profile=profile,old_ticks=old,new_ticks=new,ticks_per_tstate=scale))
        assert len(results)==36 and not errors, (len(results),errors)
        assert {r['pixels'] for r in results}=={'oracle'}
    else:
        for profile,replacement in profiles:
            for _ in range(2):
                old = timed_install(profile,profile!='Rotate')
                new = timed_install(replacement)
                assert m.read(bios['bios_upper.koi7'],1)==b'\\xff'
                loads.append(dict(profile=profile,old_ticks=old,new_ticks=new,ticks_per_tstate=scale))
    (out/'loads.json').write_text(json.dumps(loads,indent=2)+'\\n')
    print('COMPLETE',args.group,len(results),loads,flush=True)
finally:
    m.http.close()
    if pid and process_path(pid) == exe:
        ps(f'Stop-Process -Id {pid}')
'''
exec(compile(shared + boot + body, str(__file__), 'exec'))
