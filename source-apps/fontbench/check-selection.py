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
font=(R/'assets/system-rom/FONT.FNT').read_bytes()
f4=(R/'bin/FONTBEN.font4.bin').read_bytes()
font4=bytes((f4[(c//2)*8+y]>>(0 if c&1 else 4))&15 for c in range(256) for y in range(8))
baseline=(R/'bin/BIOS.sys').read_bytes()
def check_restored():
    slots=[int.from_bytes(m.read(s['hookSlots']+i*2,2),'little') for i in range(13)]
    assert all(m.read(a,2)==baseline[a-0xC000:a-0xC000+2] for a in slots),'ABI restore'
def check_memory():
    k=context['sy'](R/'bin/banked/KERNEL.sym')
    try:
        m.write(0xFFFD,b'\0')
        blocks=m.read(k['MEM_ALLOC_TABLE'],32*6)
        modules=m.read(k['MODULE_TABLE'],k['MODULE_MAX']*k['MODULE_RECORD_SIZE'])
    finally:m.write(0xFFFC,b'\0')
    assert all(blocks[i]==255 for i in range(0,len(blocks),6)),'block leak'
    assert all(modules[i:i+2]==b'\0\0' for i in range(0,len(modules),k['MODULE_RECORD_SIZE'])),'module leak'
rows=[]
try:
    x.start('engine-selection',system=R/'images/SystemRom_emu80_writable.bin')
    m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
    cmd='A:/Bench/FontBen/FontBen.com -E FULL -P ON -N -Q' if failure else 'A:/Bench/FontBen/FontBen.com -E FULL -E 4DUP -P ON -Q'
    keys={':':'colon','/':'slash','.':'period',' ':'space','-':'minus'}
    for c in cmd:m.tap(keys.get(c,c.lower()))
    assert m.read(n['vars.cmdLine'],len(cmd)).upper()==cmd.upper().encode()
    m.screenshot(O/'command.png');m.tap('enter')
    if failure:
        until(s['gridBankRun'])
        assert m.read(s['engineHooked'],1)==b'\1'
        m.write(s['gridBankPath'],b'Z') # An unavailable disk, after hooks exist.
        m.call('bp_clear');m.stop_at(s['gridError']);m.until(s['gridError'],120,page=0)
        m.screenshot(O/'before-cleanup.png')
        m.call('bp_clear');m.call('debug_step',mode='into');until(s['bench_restore'])
    else:
        until(s['gridMeasured'])
        corpus=m.read(s['gridText'],1152)
        for stage in range(36):
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
            if stage<35:
                m.call('debug_step',mode='into');until(s['gridMeasured'])
        m.call('bp_clear');until(s['bench_restore'])
        assert {r['engine'] for r in rows}=={'FULL','4DUP'}
    check_restored();check_memory()
    m.call('bp_clear');m.call('debug_step',mode='into');m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120,page=0)
    m.screenshot(O/'returned.png')
    if not failure:
        def api(af):
            m.write(0x8C00,b'\xcd'+s['bios_fileHandle'].to_bytes(2,'little')+b'\xc3\x06\x8c')
            m.stop_at(0x8C06);m.call('regs_set',pc=0x8C00,sp=0x8E00,af=af,hl=0x8B20,bc=0,de=0)
            assert not m.until(0x8C06,120,page=0)['flags']['C']
        obj=bytearray(64);obj[:11]=b'FONTBEN TXT';obj[40:42]=bytes([1,1]);m.write(0x8B20,obj);api(0)
        report=bytearray()
        while True:
            m.write(0x8B20+49,(0x7A00).to_bytes(2,'little')+(256).to_bytes(2,'little'))
            api(0x100);count=int.from_bytes(m.read(0x8B20+51,2),'little')
            if not count:break
            report+=m.read(0x7A00,count)
            assert len(report)<4096
        api(0x500)
        assert report.count(b'FULL\r\nFontBen')==1 and report.count(b'4DUP\r\nFontBen')==1,report
        (O/'FONTBEN.TXT').write_bytes(report)
    (O/'results.json').write_text(json.dumps(dict(bitmap='PASS',restore='PASS',memory='PASS',injectedFailure=failure,rows=rows),indent=2)+'\n')
    print('ERROR CLEANUP PASS' if failure else 'SELECTION/APPEND PASS',flush=True)
finally:x.stop()
