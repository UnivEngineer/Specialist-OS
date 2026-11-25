"""Одно удержание Down, десять шагов в панели NC без прокрутки."""
import argparse
import json
import os
from pathlib import Path
import runpy
import statistics
import sys
import time

CPU_HZ = 2_000_000
STEPS = 10
F806 = 0xF806
SKIP = 1_000_000


def metrics(ticks):
    return dict(ticks=ticks, ms=ticks*1000/CPU_HZ,
                steps_per_second=CPU_HZ/ticks if ticks else None)


def measure(campaign, report, save, folder, gfx=None, counters=None):
    m, n, page = campaign.m, campaign.n, campaign.page
    poll = n['ncBankInputPoll'] if gfx else campaign.b['loc_C23C']
    ready = poll if gfx else campaign.b['getch2']
    decode = n['ncBankInputKey'] if gfx else campaign.b['loc_C311']
    cpu = m.call('sys_info')['cpu']
    assert cpu['speed_factor'] == 32 and not cpu['full_throttle'] and cpu['frequency_hz'] == CPU_HZ

    def counted_wait(target):
        m.call('bp_clear')
        m.call('bp_set', addr=target, type='exec')
        m.call('bp_set', addr=F806, type='exec', skipCount=SKIP)
        for _ in range(20):
            m.call('emu_run_for', ms=1000)
            state = campaign.state()
            if state['registers']['pc']['dec'] != target:
                continue
            if campaign.page_now() == page:
                break
            m.call('debug_step', mode='into')
        else:
            raise RuntimeError((target, page, state))
        counter, = [bp for bp in m.call('bp_list')['breakpoints']
                    if int(bp['addr'],16) == F806 and bp['type'] == 'exec']
        hits = counter['hitCount']
        assert counter['remaining'] == SKIP-hits
        return state, hits

    campaign.free()
    if campaign.panel() == 0:
        campaign.tap('tab')
    campaign.free()
    campaign.tap('home')
    campaign.free()
    campaign.wait(poll, page)
    panel = campaign.panel()
    assert panel == 1
    top_address = n['state.panelA_firstFile'] + panel
    visible = campaign.read(page,n['ncVisibleCount'],1)[0]
    files = campaign.read(page,n['state.panelA_filesCnt']+panel,1)[0]
    initial_cursor = campaign.cursor()
    top = campaign.read(page,top_address,1)[0]
    assert initial_cursor == top and initial_cursor+STEPS < min(top+visible, files)
    m.screenshot(folder/f'{page}-before-down.png')
    before = campaign.state()['cpu_ticks']
    host_start = time.perf_counter_ns()
    m.call('kbd_send', keys=['down'], action='press')
    samples = []
    try:
        press = campaign.state()['cpu_ticks']
        assert press == before
        last_decode = last_ready = press
        for index in range(1, STEPS+1):
            key, f806_wait = counted_wait(decode)
            assert key['registers']['af']['dec'] >> 8 == 0x1a
            end, f806_work = counted_wait(ready)
            cursor = campaign.cursor()
            current_top = campaign.read(page,top_address,1)[0]
            assert cursor == initial_cursor+index and current_top == top, (index,cursor,current_top)
            key_tick, ready_tick = key['cpu_ticks'],end['cpu_ticks']
            assert key_tick > last_ready and ready_tick > key_tick
            samples.append(dict(step=index,cursor=cursor,top=current_top,
                                decode_tick=key_tick,ready_tick=ready_tick,
                                decode_interval_ticks=key_tick-last_decode,
                                waiting_ticks=key_tick-last_ready,
                                handler_ticks=ready_tick-key_tick,
                                F806_wait=f806_wait,F806_handler=f806_work))
            last_decode, last_ready = key_tick, ready_tick
    finally:
        m.call('kbd_send', keys=['down'], action='release')
        host_end = time.perf_counter_ns()
    release = campaign.state()['cpu_ticks']
    assert release == last_ready
    m.screenshot(folder/f'{page}-after-down.png')
    steady = samples[2:]
    median = lambda field: statistics.median(item[field] for item in steady)
    report.update(page=page, panel=panel, files=files, visible=visible, initial_cursor=initial_cursor,
                  initial_top=top, press_tick=press, release_tick=release,
                  hold_ms_guest=(release-press)*1000/CPU_HZ,
                  hold_ms_host=(host_end-host_start)/1_000_000,
                  press_to_first=metrics(samples[0]['decode_interval_ticks']),
                  first_repeat=metrics(samples[1]['decode_interval_ticks']),
                  steady_decode_interval=metrics(median('decode_interval_ticks')),
                  steady_wait=metrics(median('waiting_ticks')),
                  steady_handler=metrics(median('handler_ticks')),
                  F806_wait_total=sum(item['F806_wait'] for item in samples),
                  F806_handler_total=sum(item['F806_handler'] for item in samples),
                  samples=samples)
    del report['events']
    save()
    print(page, 'press',report['press_to_first'],'first_repeat',report['first_repeat'],
          'steady',report['steady_decode_interval'],'work',report['steady_handler'],
          'F806_wait',report['F806_wait_total'],flush=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('phase', choices=('before','after'))
    p.add_argument('--checkout', type=Path, required=True)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--disk', type=Path, required=True)
    p.add_argument('--out', type=Path, default=Path(__file__).resolve().parent)
    a = p.parse_args()
    controller = runpy.run_path(str(a.checkout.resolve()/'scripts/emu80/benchmark-nc-repeat.py'))
    controller['main'].__globals__['measure'] = measure
    os.environ['MXOS_EMU80_SPEED'] = '32'
    sys.argv = [sys.argv[0],a.phase,'--port','19782','--runtime',str(a.runtime),
                '--disk',str(a.disk),'--output',str(a.out)]
    controller['main']()


if __name__ == '__main__':
    main()
