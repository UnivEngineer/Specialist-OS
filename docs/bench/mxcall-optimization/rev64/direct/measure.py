from pathlib import Path
import hashlib, json, os, runpy, struct

root=Path(__file__).resolve().parents[5]
out=Path(__file__).resolve().parent
h=runpy.run_path(str(root/'scripts/emu80/benchmark-disks.py'))
rt=Path(os.environ['LOCALAPPDATA'])/'Specialist-OS/emu80/19781'
h['Matrix'].__init__.__globals__.update(OUT=out,RUNTIME=rt)
x=h['Matrix']()
m=x.m
b=runpy.run_path(str(root/'scripts/emu80/benchmark-mxcall.py'))['symbols'](root/'bin/BIOS.sym')
k=h['symbols'](root/'bin/banked/KERNEL.sym')
t=h['symbols'](root/'bin/banked/TRANSPORT.sym')
word=struct.Struct('<H').pack
image=root/'images/C-Flash-512k-MMX-System-Emu80.bin'
rows=[]
stub=0x80c0
data=0x8060
def page(n):
    m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))
def run(bank,op,regs,inline):
    page(bank)
    code=b'\xcd'+word(b['mx_callRegs'] if inline else op)
    if inline:
        code+=word(op)
    m.write(stub,code)
    stop=stub+len(code)
    m.stop_at(stop)
    m.call('regs_set',pc=stub,sp=0x8650,**regs)
    before=m.call('cpu_state')['cpu_ticks']
    st=m.until(stop,10,page=bank)
    state={key:st['registers'][key]['dec'] for key in ('af','bc','de','hl','sp','pc')}
    assert state.pop('pc')==stop and state['sp']==0x8650
    return st['cpu_ticks']-before,state
def store():
    (out/'result.json').write_text(json.dumps({'system_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),
        'cpu':info,'font':'6X8/ROTATE','rows':rows},indent=2)+'\n')
try:
    for attempt in range(3):
        try:
            x.start('MAIN direct historical BIOS',system=image)
            break
        except (AssertionError,RuntimeError):
            x.stop()
            if attempt==2:raise
    m.call('debug_break')
    page(0)
    info=m.call('sys_info')['cpu']
    assert info['speed_factor']==32 and not info['full_throttle']
    assert m.read(0xd827,1)==b'\0'
    font=m.read(b['FONT_ENGINE_BASE'],128)
    win=(root/'bin/banked/WINDOW.bin').read_bytes()
    cursor=b['bios_vars.cursorY']
    cases=[
        ('UPPERCASE','upperCase','q',0x7193,0x1234,0x2345,0x3451),
        ('SUB_HL_DE','sub_hl_de','profile',0x7193,0x1234,0x2345,0x3451),
        ('GET_CURSOR_POS','getCursorPos','profile',0x7193,0x1234,0x2345,0x3451),
        ('SET_CURSOR_POS','setCursorPos','xy=100a',0xa5d7,0x100a,0x2345,0x3451),
        ('PRINT_CHAR','printChar','Q',0x7193,0x1234,0x2345,0x3451),
        ('PRINT_STRING_Z','printString','X+NUL',0x7193,data,0x2345,0x3451),
    ]
    for letter in (0x39,0x41,0xa3,0xa4,0xc0,0xe0):
        cases.append(('UPPERCASE','upperCase',f'{letter:02x}',letter<<8|0x93,0x1234,0x2345,0x3451))
    cases.append(('SUB_HL_DE','sub_hl_de','no-borrow',0x7193,0x2345,0x1234,0x3451))
    for op,legacy,label,af,hl,de,bc in cases:
        regs=dict(af=af,hl=hl,de=de,bc=bc)
        pair=[]
        for inline in (False,True):
            page(0)
            m.write(0xffc0,win)
            m.write(cursor,word(0x100a))
            m.write(data,b'X\0')
            expected_head=m.read(b['FONT_ENGINE_BASE'],128)
            addr=b['MXOS_BIOS_'+op] if inline else b['bios_'+legacy]
            ticks,result=run(0,addr,regs,inline)
            pair.append((ticks,result,m.read(cursor,2).hex()))
            assert expected_head==font
        assert pair[0][1:]==pair[1][1:],(op,label,pair)
        row=dict(group='BIOS',op=op,label=label,entry=b['bios_'+legacy],input=regs,result=pair[0][1],
                 cursor_out=pair[0][2],direct=pair[0][0],inline=pair[1][0])
        rows.append(row)
        store()
        print(op,label,row['direct'],row['inline'],flush=True)
    regs=dict(af=0x7193,hl=0x1234,de=0x2345,bc=0x3451)
    for err in (0,0xa5):
        pair=[]
        for inline in (False,True):
            page(1)
            m.write(k['kdBusy'],b'\0')
            m.write(k['v_dosError'],bytes([err]))
            ticks,result=run(1,k['MXOS_DOS_GET_ERROR'] if inline else k['dosGetError'],regs,inline)
            pair.append((ticks,result))
        assert pair[0][1]==pair[1][1]
        row=dict(group='DOS',op='GET_ERROR',label=f'{err:02x}',entry=k['dosGetError'],input=regs,
                 result=pair[0][1],direct=pair[0][0],inline=pair[1][0])
        rows.append(row)
        store()
        print('DOS',row['label'],row['direct'],row['inline'],flush=True)
finally:
    x.stop()
