"""Visible Emu80: multi-engine selection, append report, and hooked-error exit."""
from pathlib import Path
import runpy,re,sys,time,json

R=Path(__file__).resolve().parents[2]
failure='--fail-bank' in sys.argv
# Reuse exactly the same NC keyboard harness and independent bitmap oracle.
context={'__file__':str(R/'source-apps/fontbench/check-grid.py')}
source=(R/'source-apps/fontbench/check-grid.py').read_text(encoding='utf-8')
exec(source[:source.index('try:\n    x.start')],context)
m=context['m'];x=context['x'];s=context['s'];n=context['n'];until=context['until']
O=R/'docs/bench/engine-grid'/('error' if failure else 'selection');O.mkdir(parents=True,exist_ok=True)
context['O']=O
context['h']['Matrix'].__init__.__globals__['OUT']=O
font=(R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
f4=(R/'bin/FONTBEN.font4.bin').read_bytes()
font4=bytes((f4[(c//2)*8+y]>>(0 if c&1 else 4))&15 for c in range(256) for y in range(8))
def check_restored():
    assert [m.read(a,2) for a in context['slotaddrs']]==initial_slots,'ABI restore'
rows=[]
disk=O/'bench-h.bin'
disk.write_bytes(b''.join((R/f'images/H-Flash-2M-Games-chip{i}.bin').read_bytes() for i in range(1,5)))
try:
    x.start('engine-selection',system=R/'images/C-Flash-512k-MMX-System-Emu80.bin',disk=disk)
    initial_slots=[m.read(a,2) for a in context['slotaddrs']]
    m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
    cmd='H:/Bench/FontBen/FontBen.com -E SCALAR12 -P ON -N -Q' if failure else 'H:/Bench/FontBen/FontBen.com -E SCALAR12 -E 4DUP -P ON -Q'
    keys={':':'colon','/':'slash','.':'period',' ':'space','-':'minus'}
    for c in cmd:m.tap(keys.get(c,c.lower()))
    assert m.read(n['vars.cmdLine'],len(cmd)).upper()==cmd.upper().encode()
    m.screenshot(O/'command.png');m.stop_at(s['gridPreNext']);m.tap('enter')
    m.until(s['gridPreNext'],120,page=0)
    initial_blocks,initial_modules=context['resources']()
    if failure:
        until(s['gridBankRun'])
        assert m.read(s['engineHooked'],1)==b'\1'
        m.write(s['gridBankPath'],b'Z') # An unavailable disk, after hooks exist.
        m.call('bp_clear');m.stop_at(s['gridError']);m.until(s['gridError'],120,page=0)
        m.screenshot(O/'before-cleanup.png')
        m.call('bp_clear');m.call('debug_step',mode='into');until(s['bench_restore'])
    else:
        until(s['gridMeasured'])
        corpus=m.read(s['gridText'],s['GRID_TEXT_BYTES'])
        stages=2*s['GRID_FULL_STAGES']
        for stage in range(stages):
            record=s['engineRecords']+m.read(s['engineId'],1)[0]*8
            name=int.from_bytes(m.read(record,2),'little')
            name=m.read(name,16).split(b'\0')[0].decode()
            context['engine']=name;context['font']=font4 if name=='4DUP' else font
            kind=m.read(s['gridKind'],1)[0]
            expected,cursor=context['oracle'](corpus,kind in (1,3,4))
            got=m.read(0x9000,12288)
            assert all(got[i]==expected[i] for i in range(12288) if i%256<240),(stage,name,'pixels')
            assert m.read(s['bios_vars.cursorY'],2)==cursor,(stage,name,'cursor')
            assert m.read(s['gridPre'],1)==b'\1'
            rows.append(dict(engine=name,kind=kind,route=m.read(s['gridRoute'],1)[0],ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')))
            print(stage+1,rows[-1],flush=True)
            if stage+1<stages:
                m.call('debug_step',mode='into');until(s['gridMeasured'])
        m.call('bp_clear');until(s['bench_restore'])
        assert {r['engine'] for r in rows}=={'SCALAR12','4DUP'}
    check_restored()
    blocks,modules=context['resources']()
    assert blocks==initial_blocks and modules==initial_modules,('leaks',blocks^initial_blocks,modules^initial_modules)
    m.call('bp_clear');m.call('debug_step',mode='into');m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120,page=0)
    m.screenshot(O/'returned.png')
    if not failure:
        report=context['read_report'](4096)
        assert report.count(b'SCALAR12\r\nFontBen')==1 and report.count(b'4DUP\r\nFontBen')==1,report
        (O/'FONTBEN.TXT').write_bytes(report)
    (O/'results.json').write_text(json.dumps(dict(bitmap='PASS',restore='PASS',memory='PASS',injectedFailure=failure,rows=rows),indent=2)+'\n')
    print('ERROR CLEANUP PASS' if failure else 'SELECTION/APPEND PASS',flush=True)
finally:x.stop()
