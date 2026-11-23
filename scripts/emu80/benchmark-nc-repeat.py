"""Measure held panel arrows and actual graphics packets in Emu80."""
import argparse
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'docs/nc/keyboard-repeat-2026-11-18'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=['before', 'after'])
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
    parser.add_argument('--output', type=Path, default=OUT)
    args = parser.parse_args()
    folder = args.output / args.phase
    folder.mkdir(parents=True, exist_ok=True)
    tools = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    profile = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-profiles.py'))
    profile['symbols'].__globals__.update(SNAP=ROOT, OUT=folder)
    tools['Matrix'].__init__.__globals__.update(
        RUNTIME=args.runtime.resolve(), OUT=folder,
        mcp_owner=lambda: tools['shell'](
            f'$p=Get-NetTCPConnection -LocalPort {args.port} -State Listen -ErrorAction SilentlyContinue; '
            'if($p){$p[0].OwningProcess};exit 0'))
    matrix = tools['Matrix']()
    m = matrix.m
    m.url = f'http://127.0.0.1:{args.port}/mcp'
    try:
        matrix.start('nc-repeat-'+args.phase)
        campaign = profile['Campaign'](m)
        campaign.free()
        if campaign.panel() == 0:
            campaign.tap('tab')
        campaign.tap('home')
        campaign.free()
        campaign.wait(campaign.b['loc_C23C'], 0)
        samples = []
        first_cursor = campaign.cursor()
        m.call('kbd_send', keys=['down'], action='press')
        try:
            for index in range(9):
                key = campaign.wait(campaign.b['loc_C311'], 0)
                assert key['registers']['af']['dec'] >> 8 == 0x1a
                ready = campaign.wait(campaign.b['getch2'], 0)
                assert campaign.cursor() == first_cursor + index + 1
                samples.append(dict(event=key['cpu_ticks'], ready=ready['cpu_ticks']))
        finally:
            m.call('kbd_send', keys=['down'], action='release')
        (folder/'main.json').write_text(json.dumps(samples, indent=2)+'\n', encoding='utf-8')
        print('MAIN', samples, flush=True)
        for attempt in range(3):
            try:
                campaign.type('C:/DOS/NC.COM')
                break
            except AssertionError:
                if attempt == 2:
                    raise
        campaign.tap('enter')
        m.call('emu_run_for', ms=6500)
        keys = campaign.read(1, campaign.k['residentKeys'], 16)
        slot = next(i for i in range(8) if keys[2*i:2*i+2] == b'NC')
        campaign.page = campaign.read(1, campaign.k['MODULE_TABLE']+
                                      slot*campaign.k['MODULE_RECORD_SIZE']+3, 1)[0]
        n = campaign.n = profile['symbols']('NC_CORE')
        gfx = profile['symbols'](profile['graphics_name']())
        campaign.free()
        if campaign.panel() == 0:
            campaign.tap('tab')
            campaign.free()
        events = []
        visible = campaign.read(campaign.page,n['ncVisibleCount'],1)[0]
        for anchor, arrow, code, stride, count in (
            ('home', 'down', 0x1a, 1, 9),
            ('scroll', 'down', 0x1a, 1, 9),
            ('home', 'right', 0x18, n['FILE_LIST_SIZE']//2, 3),
            ('end', 'left', 0x08, -n['FILE_LIST_SIZE']//2, 3),
        ):
            campaign.free()
            campaign.tap('end' if anchor == 'end' else 'home')
            campaign.free()
            if anchor == 'scroll':
                for _ in range(visible-1):
                    campaign.tap('down')
            campaign.free()
            campaign.wait(n['ncBankInputPoll'], campaign.page)
            first_cursor = campaign.cursor()
            m.call('kbd_send', keys=[arrow], action='press')
            samples = []
            try:
                for index in range(count):
                    key = campaign.wait(n['ncBankInputKey'], campaign.page)
                    assert key['registers']['af']['dec'] >> 8 == code
                    ready, packets, transfer = wait_for_bank_poll(campaign, gfx['ngRecord'] if anchor == 'home' and arrow == 'down' else None)
                    cursor = campaign.cursor()
                    assert cursor == first_cursor+(index+1)*stride,(arrow,index,cursor,first_cursor)
                    top = campaign.read(campaign.page,n['state.panelA_firstFile']+campaign.panel(),1)[0]
                    samples.append(dict(event=key['cpu_ticks'],ready=ready['cpu_ticks'],scroll=top,
                                        packets=packets, route_and_copy_ticks=transfer))
            finally:
                m.call('kbd_send', keys=[arrow], action='release')
            events.append(dict(anchor=anchor,arrow=arrow,samples=samples))
            (folder/'results.json').write_text(json.dumps(events,indent=2)+'\n',encoding='utf-8')
            print(anchor, arrow, samples, flush=True)
        campaign.free()
        campaign.wait(n['ncBankInputPoll'], campaign.page)
        m.call('kbd_send', keys=['f3'], action='press')
        try:
            key = campaign.wait(n['ncBankInputKey'], campaign.page)
            assert key['registers']['af']['dec'] >> 8 == 2
        finally:
            m.call('kbd_send', keys=['f3'], action='release')
        _, packets, _ = wait_for_bank_poll(campaign)
        assert campaign.read(campaign.page, n['state.panelA_info'], 1) == b'\x01'
        assert campaign.read(campaign.page, n['pipHasVolume'], 1) == b'\x01'
        m.screenshot(folder/'info.png')
        print('F3 packets', packets, flush=True)
        (folder/'info.json').write_text(json.dumps(packets)+'\n', encoding='utf-8')
    finally:
        matrix.stop()


def wait_for_bank_poll(campaign, receive=None):
    m = campaign.m
    poll = campaign.n['ncBankInputPoll']
    submit = campaign.n['cqDeliverDll']
    m.call('bp_clear')
    for address in (poll, submit):
        m.call('bp_set', addr=address, type='exec')
    packets = []
    transfer = None
    started = None
    for _ in range(100):
        m.call('emu_run_for', ms=1000)
        state = campaign.state()
        pc = state['registers']['pc']['dec']
        if pc == receive and campaign.page_now() == 0:
            transfer = state['cpu_ticks'] - started
            m.call('bp_clear')
            m.call('bp_set', addr=poll, type='exec')
            receive = None
            m.call('debug_step', mode='into')
        elif pc in (poll, submit):
            if campaign.page_now() == campaign.page:
                if pc == poll:
                    return state, packets, transfer
                used = campaign.read(campaign.page, campaign.n['cqUsed'], 2)
                packets.append(int.from_bytes(used, 'little'))
                if receive is not None:
                    started = state['cpu_ticks']
                    m.call('bp_set', addr=receive, type='exec')
            m.call('debug_step', mode='into')
        elif pc == receive:
            m.call('debug_step', mode='into')
    raise RuntimeError('NC_BANK did not return to keyboard polling')


if __name__ == '__main__':
    main()
