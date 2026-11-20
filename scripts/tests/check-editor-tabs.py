"""8080 literal Tab, expanded raw slices and byte/visual caret navigation."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'))
S=g['S'];T=g['T']

def wide(m,name):return number(m,name)+(m[S[name+'Hi']]<<16)
def position(m,offset):
    word(m,'v_column',offset)
    return run(m,'editorPositionCursor')

for font in ['Rotate','Fast8','Cpm4']:
    for raw in [b'',b'\t',b'\t\tX',b'abc\tD\t',b'1234567\tX',b'A'*63+b'\tLAST',b'\tA\tB'*30]:
        payload=raw+b'\rNEXT';m=fixture(payload,font)
        for gap in [0,len(raw)//2,len(raw)]:
            m=run(m,'gapMove',hl=T+gap)
            for left,start,width in [(0,0,48),(0,3,17),(8,0,48),(48,8,8),(248,0,48)]:
                word(m,'v_leftColumn',left)
                # The ordinary runner doesn't set A, so use the slice renderer
                # for nonzero starts; it reads editorSliceStart explicitly.
                m[S['editorSliceStart']]=start;m[S['editorSliceWidth']]=width
                m=run(m,'drawTextSlice',hl=T)
                actual=raw.expandtabs(8)[left+start:left+start+width]
                assert m[S['editorRowBuffer']:S['editorRowBuffer']+width]==actual.ljust(width,b' '),(font,raw,left,start)
                assert document(m)==payload
        for offset in range(len(raw)+1):
            m=position(m,offset)
            expected=len(raw[:offset].expandtabs(8))
            assert wide(m,'v_visualColumn')==expected,(font,raw,offset)
            assert m[S['bios_vars.cursorX']]==(expected-wide(m,'v_leftColumn'))*m[S['v_charStep']]

    m=fixture(b'ABCD\rNEXT',font);m=position(m,2)
    m=run(m,'keyHandler_Tab');assert document(m)==b'AB\tCD\rNEXT'
    assert number(m,'v_column')==3
    m=run(m,'editorPositionCursor');assert wide(m,'v_visualColumn')==8
    m=run(m,'keyHandler_Left');assert number(m,'v_column')==2 and wide(m,'v_visualColumn')==2
    m=run(m,'keyHandler_Right');assert number(m,'v_column')==3 and wide(m,'v_visualColumn')==8
    m=run(m,'keyHandler_Backspace');assert document(m)==b'ABCD\rNEXT'
    m=run(m,'editorUndoLine');assert document(m)==b'ABCD\rNEXT'
    m=run(m,'keyHandler_Tab');m=run(m,'keyHandler_Enter')
    assert document(m)==b'AB\t\rCD\rNEXT'
    m=run(m,'keyHandler_Backspace');assert document(m)==b'AB\tCD\rNEXT'
    m=position(m,2);m=run(m,'keyHandler_Del');assert document(m)==b'ABCD\rNEXT'
    m=run(m,'editorUndoLine');assert document(m)==b'AB\tCD\rNEXT'
    # OVR writes the literal byte too, with no document length change.
    m=fixture(b'ABC\r',font);m=run(m,'editorToggleInsert');m=position(m,1)
    m=run(m,'keyHandler_Tab');assert document(m)==b'A\tC\r'

    # Preserve the visual goal through short lines and snap before a Tab
    # when a requested visual cell is inside its expansion.
    m=fixture(b'123456\r\tX\rshort\r123456789\r',font);m=position(m,6)
    m=run(m,'keyHandler_Down');m=run(m,'editorPositionCursor')
    assert number(m,'v_column')==0 and wide(m,'v_goalColumn')==6
    m=run(m,'keyHandler_Down');m=run(m,'editorPositionCursor')
    assert number(m,'v_column')==5
    m=run(m,'keyHandler_Down');m=run(m,'editorPositionCursor')
    assert number(m,'v_column')==6
    m=run(m,'keyHandler_Up');m=run(m,'keyHandler_Up');m=run(m,'keyHandler_Up')
    assert number(m,'v_column')==6
    m=fixture(b'ABC\t \tDEF\r',font)
    m=run(m,'moveNextWord');assert number(m,'v_column')==6
    m=run(m,'keyHandler_End');m=run(m,'movePreviousWord');assert number(m,'v_column')==6
    m=run(m,'movePreviousWord');assert number(m,'v_column')==0

    # Bitmap reuse/cache must agree with full redraw, with tabs before the
    # viewport, in selected rows and at clipped edges.
    payload=b''.join((b'\t'+str(i).encode()+b'\tTEXT\t'*12+b'\r') for i in range(32))
    m=fixture(payload,font)
    word(m,'v_selectionBegin',T);word(m,'v_selectionEnd',T+len(payload)//2)
    m=run(m,'redrawPreservingCursor')
    for offset in [8,9,18,2,0]:
        m=position(m,offset)
        full=run(m,'redrawPreservingCursor')
        assert m[0x9000:0xc000]==full[0x9000:0xc000],(font,'horizontal',offset)
        left=wide(m,'v_leftColumn');cols=m[S['v_columns']]
        want=[len(row.expandtabs(8)[left:left+cols]) for row in payload.split(b'\r')[:23]]
        assert list(m[S['editorRowLengths']:S['editorRowLengths']+23])==want
    for key in ['editorPageDown','editorPageUp']:
        m=run(m,key);full=run(m,'redrawPreservingCursor')
        assert m[0x9000:0xc000]==full[0x9000:0xc000],(font,key)

# More than 65535 visual cells: 24-bit scrolling, goal and fixed 6-cell Col.
long=b'\t'*9000+b'Z'
m=fixture(long+b'\rshort\r'+long)
m=position(m,len(long));m=run(m,'updateStatusBar')
assert wide(m,'v_visualColumn')==72001
assert m[S['editorNumberBuffer']+2:S['editorNumberBuffer']+6]==b'####'
assert wide(m,'v_leftColumn')>65535
for key,offset in [('keyHandler_Down',5),('keyHandler_Down',9001),('keyHandler_Up',5),('keyHandler_Up',9001)]:
    m=run(m,key);m=run(m,'editorPositionCursor')
    assert number(m,'v_column')==offset
m=run(m,'keyHandler_Home');m=run(m,'editorPositionCursor')
assert wide(m,'v_leftColumn')==0
m=run(m,'updateStatusBar');assert m[S['editorNumberBuffer']+2:S['editorNumberBuffer']+6]==b'   1'
m=position(m,8192);m=run(m,'updateStatusBar')
assert m[S['editorNumberBuffer']+2:S['editorNumberBuffer']+6]==b'####'
for offset in [8193,8200,8199,8192,8191]:
    m=position(m,offset)
    assert wide(m,'v_visualColumn')==8*offset
    full=run(m,'redrawPreservingCursor')
    assert m[0x9000:0xc000]==full[0x9000:0xc000],('24-bit bitmap shift',offset)

# Page commands preserve the same goal when landing inside a Tab.
rows=[b'1234567']*21+[b'\tQ']*21+[b'1234567']*21
m=fixture(b'\r'.join(rows));m=position(m,6)
for key,line,offset in [('editorPageDown',22,0),('editorPageDown',43,6),('editorPageUp',22,0),('editorPageUp',1,6)]:
    m=run(m,key);m=run(m,'editorPositionCursor')
    assert number(m,'v_lineNumber')==line and number(m,'v_column')==offset
# Format all six decimal places across 16-bit and decimal boundaries.
for value in [0,9,65535,65536,99999,100000,131072,210000]:
    m=fixture(b'X')
    at=0x8d00;S['tabFormatProbe']=at
    code=b'\x3e'+bytes([value>>16])+b'\x21'+g['W'](value&65535)+b'\xcd'+g['W'](S['editorFormatColumn'])+b'\xc9'
    m[at:at+len(code)]=code
    m=run(m,'tabFormatProbe')
    assert m[S['editorNumberBuffer']:S['editorNumberBuffer']+6]==f'{value:6d}'.encode()
print('EDITOR TABS PASS' ,g['run'].__globals__['count'])
