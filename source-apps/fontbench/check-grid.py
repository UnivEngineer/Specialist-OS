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
bsy=sy(R/'bin/BIOS.sym')
s=sy(R/'bin/FONTCOM.sym'); n=sy(R/'bin/NC.sym'); t=sy(R/'bin/banked/TRANSPORT.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/bios-font-20261109')
x=h['Matrix'](); m=x.m
original_call=m.call
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
    width=4 if engine.startswith('4') else 8 if engine=='8X8' else 6
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
font=(R/'assets/system-rom/FONT.FNT').read_bytes()
assert len(font)==2048
if engine.startswith('4'):
    source=(R/'bin/FONTBEN.font4.bin').read_bytes()
    font=bytes((source[(c//2)*8+y]>>(0 if c&1 else 4))&15 for c in range(256) for y in range(8))
results=[]
ui_only='--ui-only' in sys.argv
try:
    x.start('native-grid',system=R/'images/SystemRom_emu80_writable.bin')
    m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
    cmd='A:/Bench/FontBen/FontBen.com'+(' -E '+engine if engine!='BIOS' else '')+(' -P '+preOnly if preOnly!='BOTH' else '')+(' -I' if inverse else '')+(' -S' if scalarOnly else '')
    keys={':':'colon','/':'slash','.':'period',' ':'space','-':'minus','=':'equals'}
    for c in cmd:m.tap(keys.get(c,c.lower()))
    assert m.read(n['vars.cmdLine'],len(cmd)).upper()==cmd.upper().encode(),m.read(n['vars.cmdLine'],len(cmd))
    m.screenshot(O/'command.png');m.tap('enter')
    until(s['gridMeasured'])
    abi_before=(R/'bin/BIOS.sys').read_bytes()[0x800:0x8F6]
    slotaddrs=[int.from_bytes(m.read(s['hookSlots']+i*2,2),'little') for i in range(13)]
    originalslots=[(R/'bin/BIOS.sys').read_bytes()[a-0xC000:a-0xC000+2] for a in slotaddrs]
    corpus=m.read(s['gridText'],1153)
    assert corpus[-1]==0 and all(corpus[i*48+46:i*48+48]==b'\r\n' for i in range(24))
    references=[oracle(corpus[:-1],raw) for raw in (False,True)]
    for stage in range(STAGES):
        pre,kind,route=(m.read(s[k],1)[0] for k in ('gridPre','gridKind','gridRoute'))
        expected,cursor=references[kind in (1,3,4)]
        actual=m.read(0x9000,12288)
        diffs=[i for i in range(12288) if i%256<240 and actual[i]!=expected[i]]
        assert not diffs,(stage,pre,kind,route,'bitmap',diffs[:12],[(actual[i],expected[i]) for i in diffs[:12]])
        assert m.read(s['bios_vars.cursorY'],2)==cursor,(stage,'cursor',m.read(s['bios_vars.cursorY'],2).hex(),cursor.hex())
        expectedslots=originalslots if engine=='BIOS' else [m.read(s['hookTargets']+i*2,2) for i in range(13)]
        assert [m.read(a,2) for a in slotaddrs]==expectedslots,('hook slots',stage)
        ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')
        assert 0<ticks<65536,(stage,'timer',ticks)
        results.append(dict(pre=pre,kind=kind,route=route,ticks=ticks,ms=round(ticks*.128,3)))
        print(stage+1,results[-1],flush=True)
        if ui_only:
            m.call('debug_step',mode='into');until(s['gridProgressDone'])
            m.screenshot(O/f'progress-{stage+1}.png')
            if stage==9:
                print('NATIVE GRID UI PASS: MAIN, BANK CMD/TEXT/Win20/Far256/Own',flush=True)
                raise SystemExit(0)
        elif stage in (0,9,18,35):m.screenshot(O/f'stage-{stage+1}.png')
        if stage<STAGES-1:
            m.call('debug_step',mode='into');until(s['gridMeasured'])
    m.call('bp_clear');until(s['gridNextKey'])
    assert m.read(s['gridStage'],1)==bytes([STAGES])
    assert m.read(0xC800,0xF6)==abi_before,'BIOS entry modified'
    assert [m.read(a,2) for a in slotaddrs]==originalslots,'MXOS_CALL slots not restored'
    assert m.read(bsy['fast6.preBase'],2)==b'\0\0','prerender not disabled'
    kernel=sy(R/'bin/banked/KERNEL.sym')
    try:
        m.write(0xFFFD,b'\0')
        allocations=m.read(kernel['MEM_ALLOC_TABLE'],32*6)
        modules=m.read(kernel['MODULE_TABLE'],kernel['MODULE_MAX']*kernel['MODULE_RECORD_SIZE'])
    finally:m.write(0xFFFC,b'\0')
    assert all(allocations[i]==255 for i in range(0,len(allocations),6)),'allocation leak'
    assert all(modules[i:i+2]==b'\0\0' for i in range(0,len(modules),kernel['MODULE_RECORD_SIZE'])),'module leak'
    assert m.read(s['gridHandle']+42,1)==b'\0','unclosed report'
    size=int.from_bytes(m.read(s['gridWrite'],2),'little')-0x7C00
    assert 0<size<0x900
    report=m.read(0x7C00,size)
    (O/'FONTBEN.TXT').write_bytes(report)
    m.screenshot(O/'table.png')
    m.call('bp_clear');m.call('emu_run_for',ms=250);m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],120,page=0)
    m.screenshot(O/'returned.png')
    def api(af,hl):
        m.write(0x8C00,b'\xcd'+s['bios_fileHandle'].to_bytes(2,'little')+b'\xc3\x06\x8c')
        m.stop_at(0x8C06);m.call('regs_set',pc=0x8C00,sp=0x8E00,af=af,hl=hl,bc=0,de=0)
        state=m.until(0x8C06,120,page=0)
        assert not state['flags']['C'],('file ABI',state)
    obj=bytearray(64);obj[:11]=b'FONTBEN TXT';obj[40:42]=bytes([1,1])
    m.write(0x8B20,obj);api(0,0x8B20)
    actual=bytearray()
    while True:
        m.write(0x8B20+49,(0x7A00).to_bytes(2,'little')+(256).to_bytes(2,'little'))
        api(0x100,0x8B20)
        count=int.from_bytes(m.read(0x8B20+51,2),'little')
        if not count:break
        actual+=m.read(0x7A00,count)
        assert len(actual)<=len(report)
    api(0x500,0x8B20)
    assert actual==report,'RAM report readback'
    # MAIN has a real disk driver in F8C0h-F9FFh, not the bank I/O template.
    for operation in ('mx_put32','mx_get32'):
        m.write(0x8C00,b'\xcd'+s[operation].to_bytes(2,'little')+b'\xc3\x06\x8c')
        m.stop_at(0x8C06);m.call('regs_set',pc=0x8C00,sp=0x8E00,af=0,hl=0x7A01,bc=0xABCD,de=0xEF01)
        state=m.until(0x8C06,120,page=0)
        assert not state['flags']['C'],operation
        assert m.read(0x7A01,4)==bytes.fromhex('01efcdab'),operation
        if operation=='mx_get32':
            assert state['registers']['bc']['dec']==0xABCD and state['registers']['de']['dec']==0xEF01


    (O/'results.json').write_text(json.dumps(dict(bitmap='PASS',stages=STAGES,results=results,reportBytes=size,fileReadback='PASS',allocations='no leaks',modules='unloaded',prerender='disabled',dwordMainDriver='PASS',returnedToNC=True,cpuHz=2000000,pitHz=2000000,binarySha256={f:hashlib.sha256((R/f).read_bytes()).hexdigest() for f in ('bin/BIOS.sys','bin/IO.sys','bin/MEM.sys','bin/FONTCOM.i80','bin/FONTBANK.i80','images/SystemRom_emu80_writable.bin')}),indent=2)+'\n')
    print(engine,'GRID PASS',flush=True)
finally:x.stop()
