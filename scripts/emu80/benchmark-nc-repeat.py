"""Measure held panel arrows and actual graphics packets in Emu80."""
import argparse
import hashlib
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
    parser.add_argument('--disk', type=Path, default=ROOT/'images/H-Flash-2M-Games.bin')
    parser.add_argument('--counters', action='store_true', help='Record syscall counters in a separate run')
    parser.add_argument('--repeat', choices=('standard', 'application'), default='standard',
                        help='Use KEYBANK defaults for the panel arrows, or measure the NC application settings')
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
    report = {'cpu_hz': 2_000_000, 'profiles': {}, 'sha256': {
        name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
        for name in ('bin/NC.i80', 'bin/NC_CORE.i80', f"bin/{profile['graphics_name']()}.i80",
                     'bin/BIOS.sys', 'bin/DOS.sys', 'bin/MEM.sys',
                     'images/C-Flash-512k-MMX-System-Emu80.bin')}}
    report['sha256']['H:'] = hashlib.sha256(args.disk.read_bytes()).hexdigest()
    report_name = 'ui-counters.json' if args.counters else 'ui-results.json'

    def save():
        (folder/report_name).write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    try:
        matrix.start('nc-repeat-'+args.phase, disk=args.disk)
        report['cpu'] = m.call('sys_info')['cpu']
        campaign = profile['Campaign'](m)
        counters = None
        if args.counters:
            probe = runpy.run_path(str(ROOT/'scripts/emu80/ui-counters.py'))['UiCounters']
            counters = probe(m, campaign.t, campaign.k)
        assert campaign.read(0, 0, 4) == (ROOT/'bin/NC.i80').read_bytes()[:4]
        report['profiles']['MAIN'] = {'events': [], 'repeat_config': {
            'driver': 'BIOS GETCH', 'antiBsDelay': campaign.read(0, campaign.b['bios_upper.antiBsDelay'], 1)[0],
            'cursorDelay': campaign.read(0, campaign.b['bios_upper.cursorDelay'], 1)[0]}}
        measure(campaign, report['profiles']['MAIN'], save, folder, counters=counters)
        campaign.type('C:/DOS/NC.COM')
        m.stop_at(campaign.k['keExternal'])
        campaign.tap('enter')
        started = campaign.wait(campaign.k['keExternal'], 1)['cpu_ticks']
        n = profile['symbols']('NC_CORE')
        body = (ROOT/'bin/NC_CORE.i80').read_bytes()
        def is_nc_core(page):
            return page > 1 and campaign.read(page, 0x100, 12) == body[:12]

        launch_metrics = {}
        if counters:
            state, launch_metrics = counters.until(n['ncBankInputPoll'], None,
                                                    target_test=is_nc_core,
                                                    graphics_submit=n['cqDeliverDll'])
            page = campaign.page_now()
        else:
            m.stop_at(n['ncBankInputPoll'])
            for _ in range(150):
                m.call('emu_run_for', ms=1000)
                state = campaign.state()
                page = campaign.page_now()
                if state['registers']['pc']['dec'] == n['ncBankInputPoll']:
                    if is_nc_core(page):
                        break
                    m.call('debug_step', mode='into')
            else:
                raise RuntimeError('EXEC NC did not reach keyboard polling')
        keys = campaign.read(1, campaign.k['residentKeys'], 16)
        slot = next(i for i in range(8) if keys[2*i:2*i+2] == b'NC')
        campaign.page = campaign.read(1, campaign.k['MODULE_TABLE']+
                                      slot*campaign.k['MODULE_RECORD_SIZE']+3, 1)[0]
        assert campaign.page == page
        campaign.n = n
        handle = int.from_bytes(campaign.read(page, n['ncGfxHandle'], 2), 'little')
        gfx = profile['graphics_symbols'](lambda at, size: campaign.read(1, at, size), campaign.k, handle)
        repeat_config = configure_exec_repeat(campaign, args.repeat)
        report['profiles']['EXEC'] = {'launch_ticks': state['cpu_ticks']-started, 'events': [],
                                      'repeat_config': repeat_config}
        if launch_metrics:report['profiles']['EXEC']['launch_counters'] = launch_metrics
        measure(campaign, report['profiles']['EXEC'], save, folder, gfx, counters)
    finally:
        matrix.stop()


def configure_exec_repeat(campaign, mode):
    """Изменить только два операнда в RAM загруженного NC, сохранив такты его обработчика."""
    n, m = campaign.n, campaign.m
    standard = [n['KEY_REPEAT_FIRST'], n['KEY_REPEAT_NEXT']]
    if 'ncBankSetRepeat' not in n:
        return dict(driver='KEYBANK', mode=mode, firstNext=standard, patched=[])
    assert campaign.page_now() == campaign.page
    target = n['ncBankSetRepeat']
    template = bytes((0x3e, n['NC_PANEL_REPEAT_FIRST'], 0xc2)) + target.to_bytes(2, 'little')
    template += bytes((0x3e, n['NC_PANEL_REPEAT_NEXT'], 0x32)) + n['ncInputState.repeatCount'].to_bytes(2, 'little')
    start = target - 7
    assert m.read(start, len(template)) == template, 'Unexpected NC panel repeat code'
    selected = bytearray(template)
    patched = []
    if mode == 'standard':
        for offset, value in zip((1, 6), standard):
            if selected[offset] != value:
                patched.append(dict(address=start+offset, before=selected[offset], after=value))
                m.write(start+offset, bytes([value]))
                selected[offset] = value
    assert m.read(start, len(template)) == selected
    return dict(driver='KEYBANK', mode=mode, firstNext=[selected[1], selected[6]], patched=patched)


def measure(campaign, report, save, folder, gfx=None, counters=None):
    m, n, page = campaign.m, campaign.n, campaign.page
    poll = n['ncBankInputPoll'] if gfx else campaign.b['loc_C23C']
    decode = n['ncBankInputKey'] if gfx else campaign.b['loc_C311']

    def ready(trace=False):
        if counters:
            return counters.until(poll if gfx else campaign.b['getch2'], page,
                                  graphics_submit=n['cqDeliverDll'] if gfx else None)
        if gfx:
            end, packets, commands, transfer = wait_for_bank_poll(campaign, gfx['ngRecord'] if trace else None)
            return end, dict(packets=packets, commands=commands, route_and_copy_ticks=transfer)
        return campaign.wait(campaign.b['getch2'], 0), dict(packets=[],commands=[],route_and_copy_ticks=None)

    campaign.free()
    if campaign.panel() == 0:
        campaign.tap('tab')
        campaign.free()
    events = report['events']
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
        campaign.wait(poll, page)
        first_cursor = campaign.cursor()
        top_address = n['state.panelA_firstFile'] + campaign.panel()
        first_top = campaign.read(page, top_address, 1)[0]
        m.call('kbd_send', keys=[arrow], action='press')
        samples = []
        try:
            for index in range(1 if counters else count):
                key = campaign.wait(decode, page)
                assert key['registers']['af']['dec'] >> 8 == code
                end, metrics = ready(anchor == 'home' and arrow == 'down')
                cursor = campaign.cursor()
                assert cursor == first_cursor+(index+1)*stride,(arrow,index,cursor,first_cursor)
                top = campaign.read(page, top_address, 1)[0]
                sample = dict(event=key['cpu_ticks'],ready=end['cpu_ticks'],scroll=top,**metrics)
                if gfx:
                    sample['repeatCount'] = campaign.read(page, n['ncInputState.repeatCount'], 1)[0]
                    if report['repeat_config']['mode'] == 'standard':
                        assert sample['repeatCount'] == report['repeat_config']['firstNext'][int(index > 0)]
                samples.append(sample)
        finally:
            m.call('kbd_send', keys=[arrow], action='release')
        if anchor == 'scroll':
            assert samples[-1]['scroll'] > first_top
        events.append(dict(anchor=anchor,arrow=arrow,samples=samples))
        save()
        print(page, anchor, arrow, [s['ready']-s['event'] for s in samples], flush=True)

    for letter, code, label in (('tab', 9, 'switch_panel'), ('tab', 9, 'switch_back'),
                                ('f3', 2, 'volume_info'), ('f3', 2, 'restore_panel'),
                                ('f7', 6, 'mkdir_dialog'), ('esc', 27, 'dismiss_dialog')):
        campaign.free()
        campaign.wait(poll, page)
        m.call('kbd_send', keys=[letter], action='press')
        try:
            key = campaign.wait(decode, page)
            assert key['registers']['af']['dec'] >> 8 == code
        finally:
            m.call('kbd_send', keys=[letter], action='release')
        end, metrics = ready()
        if label == 'volume_info':
            assert campaign.read(page, n['state.panelA_info'], 1) == b'\x01'
            assert campaign.read(page, n['pipHasVolume'], 1) == b'\x01'
        if label in ('mkdir_dialog', 'dismiss_dialog'):
            assert campaign.read(page, n['ncDialogActive'], 1) == bytes([label == 'mkdir_dialog'])
        events.append(dict(operation=label, cycles=end['cpu_ticks']-key['cpu_ticks'], **metrics))
        if not counters:m.screenshot(folder/f'{page}-{label}.png')
        save()


def wait_for_bank_poll(campaign, receive=None):
    m = campaign.m
    poll = campaign.n['ncBankInputPoll']
    submit = campaign.n['cqDeliverDll']
    m.call('bp_clear')
    for address in (poll, submit):
        m.call('bp_set', addr=address, type='exec')
    packets = []
    commands = []
    transfer = None
    started = None
    for _ in range(100):
        m.call('emu_run_for', ms=1000)
        state = campaign.state()
        pc = state['registers']['pc']['dec']
        if pc == receive and campaign.page_now() == 0:
            transfer = state['cpu_ticks'] - started
            m.call('bp_clear')
            for address in (poll, submit):
                m.call('bp_set', addr=address, type='exec')
            receive = None
            m.call('debug_step', mode='into')
        elif pc in (poll, submit):
            if campaign.page_now() == campaign.page:
                if pc == poll:
                    return state, packets, commands, transfer
                size = state['registers']['bc']['dec']
                address = state['registers']['hl']['dec']
                packet = campaign.read(campaign.page, address, size)
                packets.append(size)
                header = campaign.n['GFX_RECORD_HEADER']
                pos = count = 0
                while pos < size:
                    assert pos+header <= size and packet[pos], (pos, packet.hex())
                    payload = int.from_bytes(packet[pos+header-2:pos+header], 'little')
                    pos += header+payload
                    count += 1
                assert pos == size, (pos, packet.hex())
                commands.append(count)
                if receive is not None:
                    started = state['cpu_ticks']
                    m.call('bp_set', addr=receive, type='exec')
            m.call('debug_step', mode='into')
        elif pc == receive:
            m.call('debug_step', mode='into')
    raise RuntimeError('NC_BANK did not return to keyboard polling')


if __name__ == '__main__':
    main()
