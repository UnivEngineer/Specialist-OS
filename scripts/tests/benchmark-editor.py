"""8080 timings on the unchanged Elite corpus; separate buffer and UI costs."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--variant',required=True)
p.add_argument('--binary-dir',type=Path,default=R/'bin')
p.add_argument('--editor-base',default='EDIT',help='Use E only for the frozen pre-rename benchmark fixture')
p.add_argument('--report',type=Path,default=R/'docs/editor/performance-2026-11-17')
p.add_argument('--trace-row',action='store_true')
p.add_argument('--trace-page',action='store_true')
a=p.parse_args();O=a.report.resolve()/a.variant;O.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(a.binary_dir/(a.editor_base+'.sym')).read_text(),re.M)}
W=lambda v:int(v).to_bytes(2,'little')
body=(a.binary_dir/(a.editor_base+'.i80')).read_bytes();T=S['EDITOR_TEXT_BEGIN']
elite=(R/'assets/games/Elite.txt').read_bytes()
counter=0

def word(m,name,v):m[S[name]:S[name]+2]=W(v)
def val(m,name):return int.from_bytes(m[S[name]:S[name]+2],'little')
def address(entry):
    if not isinstance(entry,str):return entry
    if entry in S:return S[entry]
    # Only archived benchmark binaries keep the historical private labels.
    legacy={'editorPageUp':'keyHandler_F2','editorPageDown':'keyHandler_F3','editorUndoLine':'keyHandler_F8'}
    return S[legacy[entry]]

def call(m,entry,hl=0,de=0,bc=0,trace=False):
    global counter
    m=bytearray(m)
    code=b'\x31'+W(S['STACK_ADDR'])+b'\x21'+W(hl)+b'\x11'+W(de)+b'\x01'+W(bc)+b'\xcd'+W(address(entry))+b'\xc3'+W(0x8cff)
    m[0x8c20:0x8c20+len(code)]=code
    (O/'input.bin').write_bytes(m)
    cmd=['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c20','-end','8cff','-counter','100000000','-output',str(O/'output.bin')]
    if trace:cmd+=['-trace']
    out=subprocess.run(cmd+[str(O/'input.bin')],check=True,capture_output=True,timeout=30)
    data=(O/'output.bin').read_bytes();assert data[65542:65544]==W(0x8cff),(entry,data[65536:].hex())
    counter+=1
    if trace:(O/('page.trace' if trace=='page' else 'row.trace')).write_bytes(out.stdout)
    return bytearray(data[:65536]),int(out.stdout.strip().splitlines()[-1])

def fixture(font,full=False):
    m=bytearray(65536);m[:len(body)]=body
    for at,name in [(0xc000,'BIOS.sys'),(0xf000,'IO.main.bin'),(0xd800,'fonts/'+font+'.i80')]:
        b=(R/'bin'/name).read_bytes();m[at:at+len(b)]=b
    lower={'Rotate':0,'Packed6K':4096,'Fast12K':10240}[font]
    limit=0x8c00-lower
    if lower:
        b=(R/'bin/fonts'/f'{font}.pre').read_bytes();assert len(b)==lower
        m[limit:limit+lower]=b
    m,_=call(m,S['FONT_ENGINE_INIT'],de=limit if lower else 0)
    payload=elite if full else elite[:min(16000,0x6400-T-129)].rsplit(b'\r',1)[0]+b'\r'
    assert T+len(payload)+1<limit,(font,len(payload),T,limit)
    m[T:T+len(payload)+1]=payload+b'\0'
    for name,v in [('v_textBeginLo',T),('v_textEnd',T+len(payload)),('v_currentLine',T),('v_topLine',T),('v_lineNumber',1),('v_textLimitLo',limit),('bios_vars.cursorY',S['EDITOR_FIRST_Y'])]:word(m,name,v)
    if 'v_fullLimit' in S:word(m,'v_fullLimit',limit)
    m[S['IO_KEYB_B']]=2
    if 'gapInitialize' in S:m,_=call(m,'gapInitialize')
    m,_=call(m,'editorGeometry')
    return m,payload

def document(m):
    end=val(m,'v_textEnd')
    if 'v_gapLo' not in S:return bytes(m[T:end])
    lo=val(m,'v_gapLo');hi=val(m,'v_gapHi')
    return bytes(m[T:lo]+m[hi:hi+end-lo])

rows=[]
def record(font,operation,cycles,**kw):
    row=dict(font=font,operation=operation,cycles=cycles,ms=round(cycles/2000,3),**kw);rows.append(row)
    print(json.dumps(row),flush=True)

# Full file fits Rotate; renderer tables reduce the document's actual memtop.
m,payload=fixture('Rotate',True)
for offset in [0,len(payload)//2,len(payload)]:
    for change in [1,-1]:
        src=T+offset+(change<0);dst=T+offset+(change>0)
        if src>val(m,'v_textEnd'):continue
        out,cycles=call(m,'resizeTextRange',hl=src,de=dst)
        expected=payload[:offset]+(b'\x00'+payload[offset:] if change>0 else payload[offset+1:])
        # Insertion leaves a hole: only transferred bytes and terminal NUL matter.
        if change>0:assert document(out)[:offset]==payload[:offset] and document(out)[offset+1:]==payload[offset:]
        else:assert document(out)==expected
        assert val(out,'v_textEnd')==T+len(payload)+change
        record('Rotate','tail_insert' if change>0 else 'tail_delete',cycles,offset=offset,moved=len(payload)-offset+1-(change<0),fileBytes=len(payload))
m,_=call(m,'redrawPreservingCursor')
for operation,bc in [('keyHandler_Space',ord('X')),('keyHandler_Del',0)]:
    out,cycles=call(m,operation,bc=bc)
    expected=b'X'+payload if operation=='keyHandler_Space' else payload[1:]
    assert document(out)==expected
    record('Rotate','full_'+operation,cycles,fileBytes=len(payload))
    again,cycles=call(out,operation,bc=bc)
    expected2=b'XX'+payload if operation=='keyHandler_Space' else payload[2:]
    assert document(again)==expected2
    record('Rotate','repeat_'+operation,cycles,fileBytes=len(payload))

for font in ['Rotate','Packed6K','Fast12K']:
    m,payload=fixture(font)
    m,cycles=call(m,'redrawPreservingCursor')
    record(font,'initial_page',cycles,fileBytes=len(payload))
    assert int.from_bytes(m[S['bios_printStringRawLim']+1:S['bios_printStringRawLim']+3],'little')>=0xd800
    for i in range(3):
        m,cycles=call(m,'editorPageDown',trace='page' if a.trace_page and font=='Fast12K' and i==0 else False);record(font,'page_down',cycles,page=i+1,fileBytes=len(payload))
    for i in range(3):
        m,cycles=call(m,'editorPageUp');record(font,'page_up',cycles,page=3-i,fileBytes=len(payload))
    out,cycles=call(m,'drawTextLine',hl=T,trace=a.trace_row and font=='Packed6K')
    record(font,'first_row',cycles)
    word(m,'v_column',8)
    out,cycles=call(m,'keyHandler_Space',bc=ord('X'));record(font,'insert_with_line_redraw',cycles,fileBytes=len(payload))
    assert document(out)==payload[:8]+b'X'+payload[8:]

report=dict(cpu='z88dk-ticks Intel 8080',cpuHz=2000000,variant=a.variant,editorTextBegin=T,
            input=dict(path='assets/games/Elite.txt',bytes=len(elite),sha256=hashlib.sha256(elite).hexdigest()),
            hashes={name:hashlib.sha256((R/'bin'/name).read_bytes()).hexdigest() for name in ['BIOS.sys','IO.main.bin','fonts/Rotate.i80','fonts/Packed6K.i80','fonts/Packed6K.pre','fonts/Fast12K.i80','fonts/Fast12K.pre']},
            editorSha256=hashlib.sha256(body).hexdigest(),results=rows,calls=counter,
            scope='Tail: full Elite; page render: same complete-line prefix bounded by 16000 bytes and current Fast12K capacity, fitting all three fonts. Calls exclude keyboard wait and status fields; insertion includes initial F8 line snapshot and row redraw.')
(O/'results.json').write_text(json.dumps(report,indent=2)+'\n')
