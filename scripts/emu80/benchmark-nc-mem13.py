"""Measure a cold panel draw and navigation across a viewport boundary."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'docs/bench/mxcall-2026-11-17/nc'
RUNTIME = ROOT / '.local/rom-pio-19782'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=['before', 'after'])
    parser.add_argument('--reuse-build', action='store_true')
    parser.add_argument('--bank-only', action='store_true')
    args = parser.parse_args()
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    profile = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-nc-profiles.py'))
    folder = OUT / args.phase
    snap = folder / 'build'
    snap.mkdir(parents=True, exist_ok=True)
    disk = OUT / 'H.bin'
    if not args.reuse_build:
        if args.phase == 'before' and (snap / 'A.bin').exists():
            parser.error('baseline already frozen, use --reuse-build')
        shutil.copytree(ROOT / 'bin', snap / 'bin', dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('*.lst', '*.log', '__pycache__'))
        shutil.copyfile(ROOT / 'images/C-Flash-512k-MMX-System-Emu80.bin', snap / 'A.bin')
        if args.phase == 'before':
            shutil.copyfile(ROOT / 'images/H-Flash-2M-Games.bin', disk)
        shutil.copyfile(disk, snap / 'H128.bin')
    profile['symbols'].__globals__.update(SNAP=snap, OUT=folder)
    profile['verify_snapshot']()
    helpers['Matrix'].__init__.__globals__.update(
        RUNTIME=RUNTIME, OUT=folder,
        mcp_owner=lambda: helpers['shell'](
            "$p=Get-NetTCPConnection -LocalPort 19782 -State Listen -ErrorAction SilentlyContinue; "
            "if($p){$p[0].OwningProcess};exit 0"))
    matrix = helpers['Matrix']()
    matrix.m.url = 'http://127.0.0.1:19782/mcp'
    report = {'hashes': {name: hashlib.sha256((snap / name).read_bytes()).hexdigest()
                         for name in ['A.bin', 'H128.bin', 'bin/NC.i80', 'bin/NC_CORE.i80',
                                      f"bin/{profile['graphics_name']()}.i80", 'bin/DOS.sys', 'bin/MEM.sys', 'bin/BIOS.sys']},
              'rows': []}
    if args.bank_only:
        previous = json.loads((folder/'results.json').read_text(encoding='utf-8'))
        assert previous['hashes'] == report['hashes']
        report['rows'] = [row for row in previous['rows'] if row['profile'] == 'MAIN']
    try:
        for bank in ((True,) if args.bank_only else (False, True)):
            name = 'BANK' if bank else 'MAIN'
            matrix.start(name, system=snap/'A.bin', disk=disk, nc_binary=snap/'bin/NC.i80',
                         nc_symbols=snap/'bin/NC.sym', bios=snap/'bin/BIOS.sys', dos=snap/'bin/DOS.sys')
            campaign = profile['Campaign'](matrix.m)
            getch = campaign.b['getch2']
            gfx = profile['symbols'](profile['graphics_name']()) if bank else {}
            # The two banks share numeric addresses. In the new build the
            # entry address also lands on a hot CORE loop during startup.
            batch = gfx.get('ngBatchSizeOK', gfx.get('ngBatch'))
            m = matrix.m
            startup_sends = 0
            wait = campaign.wait

            def wait_for_frame(address, page):
                nonlocal startup_sends
                if not bank or address != getch:
                    return wait(address, page)
                m.call('bp_clear')
                for target in (getch, batch):
                    m.call('bp_set', addr=target, type='exec')
                for _ in range(1000):
                    m.call('emu_run_for', ms=500)
                    state = campaign.state()
                    pc = state['registers']['pc']['dec']
                    if campaign.page_now() == 0:
                        if pc == getch:
                            return state
                        if pc == batch:
                            startup_sends += 1
                    if pc in (getch, batch):
                        m.call('debug_step', mode='into')
                m.screenshot(folder / 'startup-timeout.png')
                raise RuntimeError(('startup frame timeout', startup_sends, campaign.page_now(), state))

            campaign.wait = wait_for_frame
            ticks = campaign.launch(bank)
            campaign.wait = wait
            if campaign.panel() == 0:
                campaign.free()
                campaign.tap('tab')
            campaign.free()
            campaign.tap('home')
            start = campaign.wait(campaign.b['loc_C23C'], 0)['cpu_ticks']
            first = campaign.cursor()
            visible = campaign.read(campaign.page, campaign.n['ncVisibleCount'], 1)[0]
            m.call('kbd_send', keys=['down'], action='press')
            events = []
            try:
                for i in range(visible + 8):
                    decoded = campaign.wait(campaign.b['loc_C311'], 0)
                    assert decoded['registers']['af']['dec'] >> 8 == 0x1a
                    m.call('bp_clear')
                    for address in [getch] + ([batch] if bank else []):
                        m.call('bp_set', addr=address, type='exec')
                    sends = 0
                    for _ in range(100):
                        m.call('emu_run_for', ms=100)
                        state = campaign.state()
                        pc = state['registers']['pc']['dec']
                        if campaign.page_now() == 0:
                            if pc == getch:
                                break
                            if pc == batch:
                                sends += 1
                        if pc in (getch, batch):
                            m.call('debug_step', mode='into')
                    else:
                        raise RuntimeError(('render timeout', name, i))
                    assert campaign.cursor() == first+i+1
                    events.append(dict(decoded=decoded['cpu_ticks'], ready=state['cpu_ticks'],
                                       sends=sends, first=campaign.read(
                                           campaign.page, campaign.n['state.panelA_firstFile']+campaign.panel(), 1)[0]))
            finally:
                m.call('kbd_send', keys=['down'], action='release')
            assert events[-1]['first'] > events[0]['first'], 'viewport never scrolled'
            m.screenshot(folder / f'{name}.png')
            row = dict(profile=name, startup_ticks=ticks, startup_sends=startup_sends,
                       navigation_ticks=events[-1]['ready']-start, events=events)
            report['rows'].append(row)
            (folder/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
            print(name, ticks, sum(e['sends'] for e in events), flush=True)
            if bank and args.phase == 'after':
                row['cli_return_ticks'] = campaign.cli_return()
                cli = profile['symbols']('CLI')
                campaign.type('C:/DOS/CLI')
                m.call('bp_clear')
                m.call('bp_set', addr=cli['cliPrompt'], type='exec')
                campaign.tap('enter')
                campaign.wait(cli['cliPrompt'], 0)
                campaign.free()
                for key in 'exit':
                    campaign.tap(key)
                key_entry = profile['symbols'](profile['graphics_name']())['ngKey']
                m.call('bp_clear')
                m.call('bp_set', addr=key_entry, type='exec')
                campaign.tap('enter')
                campaign.wait(key_entry, 0)
                row['external_cli_return'] = True
                campaign.type('FONT 6x8/Packed')
                m.call('bp_clear')
                m.call('bp_set', addr=key_entry, type='exec')
                campaign.tap('enter')
                campaign.wait(key_entry, 0)
                assert campaign.read(0, campaign.b['FONT_ENGINE_HEADER']+7, 1) == b'\x01'
                row['font_reload'] = True
                (folder/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
                print('BANK return from CLI and FONT reload PASS', flush=True)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
