"""Native FontBen grid: NC keyboard launch, 36 framebuffer oracles, RAM report."""
from pathlib import Path
import json,re,runpy,time,urllib.error,sys,hashlib
R=Path(__file__).resolve().parents[2]
engine=sys.argv[sys.argv.index('--engine')+1].upper() if '--engine' in sys.argv else 'BIOS'
preOnly=sys.argv[sys.argv.index('--pre')+1].upper() if '--pre' in sys.argv else 'BOTH'
inverse='--inverse' in sys.argv
assert not inverse or engine=='BIOS'
scalarOnly='--scalar' in sys.argv
STAGES=(12 if preOnly=='BOTH' else 6) if scalarOnly else (36 if preOnly=='BOTH' else 18)
O=R/(('docs/bench/native-grid-inverse' if inverse else 'docs/bench/native-grid') if engine=='BIOS' else 'docs/bench/engine-grid/'+engine.lower())
if scalarOnly: O=R/('docs/bench/scalar-grid/'+engine.lower())
if '--out' in sys.argv: O=Path(sys.argv[sys.argv.index('--out')+1]).resolve()
O.mkdir(parents=True,exist_ok=True)
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',f.read_text(),re.M)}
s=sy(R/'bin/FONTCOM.sym'); n=sy(R/'bin/NC.sym'); t=sy(R/'bin/banked/TRANSPORT.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/bios-font-20261109')
x=h['Matrix'](); m=x.m
original_call=m.call
k=sy(R/'bin/banked/KERNEL.sym')
controller=(R/'bin/FONTCOM.i80').read_bytes()
hook_table=s['hookSlots']
slotaddrs=[int.from_bytes(controller[pos:pos+2],'little') for pos in range(hook_table,hook_table+2*s['HOOK_SLOTS'],2)]

def resources():
    m.write(0xFFFD,b'\0')
    try:
        blocks=m.read(k['MEM_ALLOC_TABLE'],k['MEM_ALLOC_RECORDS']*6)
        size=k['MODULE_RECORD_SIZE']
        modules=m.read(k['MODULE_TABLE'],k['MODULE_MAX']*size)
    finally:
        m.write(0xFFFC,b'\0')
    return ({blocks[i:i+6] for i in range(0,len(blocks),6) if blocks[i]!=255},
            {modules[i:i+2] for i in range(0,len(modules),size) if modules[i:i+2]!=bytes(2)})

def read_report(limit):
    handle,buffer,stub=0x8B20,0x7A00,0x8C00
    def file_call(operation):
        m.stop_at(stub+5)
        m.call('regs_set',pc=stub,sp=0x8E00,af=operation<<8,hl=handle,bc=0,de=0)
        state=m.until(stub+5,120,page=0)
        assert not state['flags']['C'],('FILEHANDLE',operation,state)
    m.write(stub,b'\xcd'+s['mx_callRegs'].to_bytes(2,'little')+s['MXOS_DOS_FILEHANDLE'].to_bytes(2,'little'))
    obj=bytearray(64)
    obj[:11]=b'FONTBEN TXT'
    obj[s['FH_DRIVE']]=1
    obj[s['FH_MODE']]=s['FILE_MODE_READ']
    m.write(handle,obj)
    file_call(s['FILE_OP_OPEN'])
    data=bytearray()
    try:
        while True:
            m.write(handle+s['FH_BUFFER'],buffer.to_bytes(2,'little')+(256).to_bytes(2,'little'))
            file_call(s['FILE_OP_READ'])
            count=int.from_bytes(m.read(handle+s['FH_COUNT'],2),'little')
            if not count: return bytes(data)
            data+=m.read(buffer,count)
            assert len(data)<=limit
    finally:
        file_call(s['FILE_OP_CLOSE'])

def reliable_call(name,**kwargs):
    for attempt in range(60):
        try:return original_call(name,**kwargs)
        except urllib.error.URLError as e:
            if getattr(e.reason,'winerror',None)!=10055:raise
            time.sleep(1)
    raise RuntimeError('MCP socket allocation failed repeatedly')
m.call=reliable_call

def until(pc):
    m.stop_at(pc);m.call('bp_set',addr=s['gridError'],type='exec');m.call('emu_run')
    end=time.monotonic()+180
    while time.monotonic()<end:
        q=m.call('cpu_state')
        if q['registers']['pc']['dec']==s['gridError'] and m.read(t['bankPage'],1)==b'\0':
            raise AssertionError(('FontBen error',q,m.read(s['gridControl'],14).hex(),m.read(s['gridStage'],1).hex()))
        if q['registers']['pc']['dec']==pc:
            if m.read(t['bankPage'],1)==b'\0':return q
            m.call('debug_step',mode='into');m.call('emu_run')
        time.sleep(.1)
    raise AssertionError(('timeout',hex(pc),q,m.read(s['gridStage'],1).hex(),m.read(s['gridControl'],14).hex()))

def oracle(text,raw):
    image=bytearray(12288);xp=0;y=7
    width=4 if engine.startswith('4') else 8 if engine in ('ROW8','WIN8') else 6
    for c in text:
        if not raw and c==13:xp=0;continue
        if not raw and c==10:y+=10;continue
        for row in range(8):
            bits=font[c*8+row] ^ (((1<<width)-1) if inverse else 0)
            for bit in range(width):
                px=xp+bit
                at=(px//8)*256+y-7+row
                mask=128>>(px%8)
                image[at]=(image[at]&~mask)|(mask if bits&(1<<(width-1-bit)) else 0)
        xp+=width
        if xp>=384:xp=0;y+=10
    return image,bytes([y,xp//2])
font=(R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
if engine=='WIN8':
    font=(R/'source/monitor_win/originals/monitor_win.rom').read_bytes()[0x1800:0x2000]
elif engine=='ROW8':
    font=(R/'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()
assert len(font)==2048
if engine.startswith('4'):
    source=(R/'bin/FONTBEN.font4.bin').read_bytes()
    font=bytes((source[(c//2)*8+y]>>(0 if c&1 else 4))&15 for c in range(256) for y in range(8))
results=[]
ui_only='--ui-only' in sys.argv
disk=O/'bench-h.bin'
disk.write_bytes(b''.join((R/f'images/H-Flash-2M-Games-chip{i}.bin').read_bytes() for i in range(1,5)))
try:
    x.start('native-grid',system=R/'images/C-Flash-512k-MMX-System-Emu80.bin',disk=disk)
    initial_pre=m.read(s['FONT_ENGINE_HEADER']+12,2)
    abi_before=m.read(0xC800,0xF6)
    originalslots=[m.read(a,2) for a in slotaddrs]
    m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
    cmd='H:/Bench/FontBen/FontBen.com'+(' -E '+engine if engine!='BIOS' else '')+(' -P '+preOnly if preOnly!='BOTH' else '')+(' -I' if inverse else '')+(' -S' if scalarOnly else '')
    keys={':':'colon','/':'slash','.':'period',' ':'space','-':'minus','=':'equals'}
    for c in cmd:m.tap(keys.get(c,c.lower()))
    assert m.read(n['vars.cmdLine'],len(cmd)).upper()==cmd.upper().encode(),m.read(n['vars.cmdLine'],len(cmd))
    m.screenshot(O/'command.png');m.stop_at(s['gridPreNext']);m.tap('enter')
    m.until(s['gridPreNext'],120,page=0)
    initial_blocks,initial_modules=resources()
    until(s['gridMeasured'])
    corpus=m.read(s['gridText'],s['GRID_TEXT_BYTES']+1)
    assert corpus[-1]==0 and all(corpus[i*s['GRID_STRIDE']+s['GRID_GLYPHS']:(i+1)*s['GRID_STRIDE']]==b'\r\n' for i in range(s['GRID_ROWS']))
    references=[oracle(corpus[:-1],raw) for raw in (False,True)]
    for stage in range(STAGES):
        pre,kind,route=(m.read(s[k],1)[0] for k in ('gridPre','gridKind','gridRoute'))
        expected,cursor=references[kind in (1,3,4)]
        actual=m.read(0x9000,12288)
        diffs=[i for i in range(12288) if i%256<240 and actual[i]!=expected[i]]
        assert not diffs,(stage,pre,kind,route,'bitmap',diffs[:12],[(actual[i],expected[i]) for i in diffs[:12]])
        assert m.read(s['bios_vars.cursorY'],2)==cursor,(stage,'cursor',m.read(s['bios_vars.cursorY'],2).hex(),cursor.hex())
        targets=s['directTargets'] if m.read(s['engineHistoricalMode'],1)[0]&128 else s['hookTargets']
        expectedslots=originalslots if engine=='BIOS' else [m.read(targets+i*2,2) for i in range(s['HOOK_SLOTS'])]
        assert [m.read(a,2) for a in slotaddrs]==expectedslots,('hook slots',stage)
        ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')
        assert 0<ticks<65536,(stage,'timer',ticks)
        results.append(dict(pre=pre,kind=kind,route=route,ticks=ticks,ms=round(ticks*.128,3)))
        print(stage+1,results[-1],flush=True)
        if ui_only:
            m.call('debug_step',mode='into');until(s['gridProgressDone'])
            m.screenshot(O/f'progress-{stage+1}.png')
            if stage==9:
                print('NATIVE GRID UI PASS: MAIN, BANK CMD/TEXT/Win32/Far256/Own',flush=True)
                raise SystemExit(0)
        elif stage in (0,9,18,35):m.screenshot(O/f'stage-{stage+1}.png')
        if stage<STAGES-1:
            m.call('debug_step',mode='into');until(s['gridMeasured'])
    m.call('bp_clear');until(s['gridNextKey'])
    assert m.read(s['gridStage'],1)==bytes([STAGES])
    assert m.read(0xC800,0xF6)==abi_before,'BIOS entry modified'
    assert [m.read(a,2) for a in slotaddrs]==originalslots,'MXOS_CALL slots not restored'
    assert m.read(s['FONT_ENGINE_HEADER']+12,2)==initial_pre,'prerender changed'
    blocks,modules=resources()
    assert blocks==initial_blocks and modules==initial_modules,('leaks',blocks^initial_blocks,modules^initial_modules)
    assert m.read(s['gridHandle']+s['FH_STATE'],1)==b'\0','unclosed report'
    report_start=int.from_bytes(m.read(s['gridControl']+8,2),'little')
    size=int.from_bytes(m.read(s['gridWrite'],2),'little')-report_start
    assert 0<size<0x900
    report=m.read(report_start,size)
    (O/'FONTBEN.TXT').write_bytes(report)
    m.screenshot(O/'table.png')
    m.call('bp_clear');m.call('emu_run_for',ms=250);m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],120,page=0)
    m.screenshot(O/'returned.png')
    assert read_report(size)==report,'RAM report readback'
    (O/'results.json').write_text(json.dumps(dict(bitmap='PASS',stages=STAGES,results=results,reportBytes=size,fileReadback='PASS',allocations='no leaks',modules='unloaded',prerenderBase=int.from_bytes(initial_pre,'little'),returnedToNC=True,cpuHz=2000000,pitHz=2000000,binarySha256={f:hashlib.sha256((R/f).read_bytes()).hexdigest() for f in ('bin/BIOS.sys','bin/IO.sys','bin/MEM.sys','bin/FONTCOM.i80','bin/FONTGRID.i80','images/C-Flash-512k-MMX-System-Emu80.bin')}),indent=2)+'\n')
    print(engine,'GRID PASS',flush=True)
finally:x.stop()
