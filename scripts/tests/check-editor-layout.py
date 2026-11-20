"""8080 regression: full lines, font geometry, preserved newlines and undo."""
from pathlib import Path
import re, subprocess

R = Path(__file__).resolve().parents[2]
O = R / '.local/editor-layout-check'
O.mkdir(parents=True, exist_ok=True)
S = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (R/'bin/EDIT.sym').read_text(), re.M)}
W = lambda n: n.to_bytes(2, 'little')
T = int.from_bytes((R/'bin/EDIT.i80').read_bytes()[S['v_textBeginLo']:S['v_textBeginLo']+2], 'little')
count = 0

def word(m, name, value):
    m[S[name]:S[name]+2] = W(value)

def number(m, name):
    return int.from_bytes(m[S[name]:S[name]+2], 'little')

def fixture(payload, font='Rotate'):
    m = bytearray(65536)
    for name, address in [('EDIT.i80', 0), ('BIOS.sys', 0xc000), ('IO.main.bin', 0xf000), ('fonts/'+font+'.i80', 0xd800)]:
        data = (R/'bin'/name).read_bytes()
        m[address:address+len(data)] = data
    word(m, 'bios_upper.charGen', S['FONT_ADDR'])
    path = {'Rotate':'6x8/KOI8.pf', 'Fast8':'8x8/Win8.pf', 'Cpm4':'4x8/Cpm4.pf'}[font]
    raw = (R/'assets/system-rom/fonts'/path).read_bytes()
    m[S['FONT_ADDR']:S['FONT_ADDR']+2048] = bytes(raw[c*8+y] for y in range(8) for c in range(256))
    word(m, 'v_textLimitLo', 0x8000)
    word(m, 'v_topLine', T)
    word(m, 'v_currentLine', T)
    word(m, 'v_textEnd', T+len(payload))
    word(m, 'v_lineNumber', 1)
    word(m, 'editorBarV.total', len(re.findall(rb'\r\n|[\r\n]',payload))+1)
    word(m, 'bios_vars.cursorY', S['EDITOR_FIRST_Y'])
    m[T:T+len(payload)+1] = payload+b'\0'
    m[0xffe1] = 2
    m[S['editorInterfaceReady']] = 1
    m[S['v_tabWidth']] = 8
    return run(run(m, 'gapInitialize'), 'editorGeometry')

def run(m, entry, hl=0, de=0, bc=0, end=0x8c40, a=None):
    global count
    m = bytearray(m[:65536])
    stub = b'\x31'+W(S['STACK_ADDR'])+b'\x21'+W(hl)+b'\x11'+W(de)+b'\x01'+W(bc)+(bytes([0x3e,a]) if a is not None else b'')+b'\xcd'+W(S[entry])+b'\xc3'+W(0x8c40)
    m[0x8c20:0x8c20+len(stub)] = stub
    (O/'in.bin').write_bytes(m)
    subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', '8c20', '-end', f'{end:x}', '-counter', '20000000', '-output', str(O/'out.bin'), str(O/'in.bin')], capture_output=True, check=True, timeout=20)
    data = (O/'out.bin').read_bytes()
    assert data[65542:65544] == W(end), (entry, data[65536:].hex())
    count += 1
    return bytearray(data[:65536])

def document(m):
    lo=number(m,'v_gapLo');hi=number(m,'v_gapHi');end=number(m,'v_textEnd')
    return bytes(m[T:lo]+m[hi:hi+end-lo])

def scrolling_checks():
    def lengths(m):
        visible=document(m)[number(m,'v_topLine')-T:].split(b'\r')
        left=number(m,'v_leftColumn');width=m[S['v_columns']]
        want=[];at=number(m,'v_topLine');end=number(m,'v_selectionEnd');start=number(m,'v_selectionBegin')
        for row in visible[:23]:
            size=len(row[left:left+width])
            if start and start<at+len(row)+1 and end>=at+len(row)+1:size=width
            want.append(size);at+=len(row)+1
        want += [0]*(23-len(want))
        assert list(m[S['editorRowLengths']:S['editorRowLengths']+23])==want
    # Compare optimized paths with full redraw using real glyphs and selection.
    lines = [((f'{i:02d}: ' + 'Abc123XYZ '*24).encode() if i%3 == 0 else b'short' if i%3 == 1 else b'')+b'\r' for i in range(32)]
    payload = b''.join(lines)
    offsets = [T+sum(map(len,lines[:i])) for i in range(32)]
    for font in ['Rotate','Fast8','Cpm4']:
        m = fixture(payload,font)
        word(m,'v_selectionBegin',offsets[2]);word(m,'v_selectionEnd',offsets[28])
        m = run(m,'redrawPreservingCursor')
        for name,top,current,y in [('keyHandler_Down',0,22,S['EDITOR_LAST_Y']),('keyHandler_Up',5,5,S['EDITOR_FIRST_Y'])]:
            word(m,'v_topLine',offsets[top]);word(m,'v_currentLine',offsets[current])
            word(m,'v_lineNumber',current+1);word(m,'bios_vars.cursorY',y)
            m = run(m,'redrawPreservingCursor')
            m = run(m,name)
            full = run(m,'redrawPreservingCursor')
            assert m[0x9000:0xc000] == full[0x9000:0xc000],(font,name)
            lengths(m)
        word(m,'v_topLine',T);word(m,'v_currentLine',T)
        word(m,'bios_vars.cursorY',S['EDITOR_FIRST_Y'])
        m = run(m,'redrawPreservingCursor')
        for column in [m[S['v_columns']],7,200,0]:
            word(m,'v_column',column)
            m = run(m,'editorPositionCursor')
            full = run(m,'redrawPreservingCursor')
            assert m[0x9000:0xc000] == full[0x9000:0xc000],(font,'horizontal',column)
            lengths(m)
        m = run(m,'drawStatusBar')
        # A marker in the static caption must survive a numeric refresh.
        m[0x9001] ^= 0x55
        old = bytes(m[0x9000:0xc000])
        word(m,'v_column',65534)
        m = run(m,'updateStatusBar')
        first=(m[S['editorStatusColPos']]+4)*m[S['v_charStep']]//4
        last=first+m[S['editorStatusNumBytes']]
        for x in range(48):
            for y in range(256):
                if first <= x < last and 1 <= y <= 8:continue
                assert m[0x9000+x*256+y] == old[x*256+y],(font,'status',x,y)
        unchanged=run(m,'updateStatusBar')
        assert unchanged[0x9000:0xc000]==m[0x9000:0xc000]
        # Replace long selected rows with a short page ending mid-screen.
        m=fixture(payload,font)
        word(m,'v_selectionBegin',T);word(m,'v_selectionEnd',offsets[28])
        m=run(m,'redrawPreservingCursor')
        word(m,'v_selectionBegin',0)
        word(m,'v_topLine',offsets[-2]);word(m,'v_currentLine',offsets[-2])
        painted=run(m,'redrawPreservingCursor')
        clean=run(m,'clearTextViewport')
        clean=run(clean,'redrawPreservingCursor')
        assert painted[0x9000:0xc000]==clean[0x9000:0xc000],(font,'short page over old text')
        # A marker beyond both old/new lengths proves the renderer doesn't
        # repaint the physical width when shortening a line.
        m=fixture(b'ABCDE\r',font)
        m=run(m,'redrawPreservingCursor')
        marker=0x9000+30*256+S['EDITOR_FIRST_Y']
        m[marker]=0xa5
        m[T:T+6]=b'X\r\0   '
        word(m,'v_textEnd',T+2)
        m=run(m,'gapInitialize')
        m=run(m,'redrawPreservingCursor')
        assert m[marker]==0xa5,(font,'unnecessary tail repaint')
        assert m[S['editorRowLengths']]==1
        # Paging must match a fresh render, including the two retained rows,
        # all cursor rows, selection and a shifted horizontal window.
        for cursor_row in [0,12,22]:
            m=fixture(payload,font)
            word(m,'v_selectionBegin',offsets[2]);word(m,'v_selectionEnd',offsets[28])
            word(m,'v_currentLine',offsets[cursor_row])
            word(m,'v_lineNumber',cursor_row+1)
            word(m,'v_leftColumn',8)
            word(m,'bios_vars.cursorY',S['EDITOR_FIRST_Y']+cursor_row*10)
            m=run(m,'redrawPreservingCursor')
            for key in ['editorPageDown','editorPageUp','editorPageUp','editorPageDown']:
                m=run(m,key)
                expected=run(m,'clearTextViewport')
                expected=run(expected,'redrawPreservingCursor')
                assert m[0x9000:0xc000]==expected[0x9000:0xc000],(font,key,cursor_row,'page overlap')
                lengths(m)
    # SDK memmove: both axes, overlap, full height and untouched cell gaps.
    for sx,sy,dx,dy,width,height in [(0,10,0,20,48,220),(0,20,0,10,48,220),(0,10,6,10,42,230),(6,10,0,10,42,230),(0,0,1,6,47,250),(1,6,0,0,47,250),(0,30,3,20,40,200)]:
        m=fixture(b'\r')
        m[0x9000:0xc000]=bytes((x*31+y*17)&255 for x in range(48) for y in range(256))
        expected=bytearray(m)
        for x in range(width):
            for y in range(height):
                if y%10 in (0,9):continue
                expected[0x9000+(dx+x)*256+dy+y]=m[0x9000+(sx+x)*256+sy+y]
        out=run(m,'uiCopyTextRect',hl=0x9000+sx*256+sy,bc=0x9000+dx*256+dy,de=width*256+height)
        assert out[0x9000:0xc000]==expected[0x9000:0xc000],(sx,sy,dx,dy,width,height)


def editing_keys():
    for font in ['Rotate','Fast8','Cpm4']:
        # Default INS, explicit OVR and switching back.
        m=fixture(b'ABC\rNEXT',font)
        assert m[S['v_insertMode']]==255
        word(m,'v_column',1)
        m=run(m,'keyHandler_Space',bc=ord('Z'))
        assert document(m)==b'AZBC\rNEXT' and m[S['v_documentDirty']]
        m=run(m,'editorUndoLine')
        m=run(m,'editorToggleInsert')
        assert m[S['v_insertMode']]==0
        m=run(m,'keyHandler_Space',bc=ord('Z'))
        assert document(m)==b'AZC\rNEXT'
        m=run(m,'editorToggleInsert');assert m[S['v_insertMode']]==255
        # Right crosses CR, left returns to the previous physical line end.
        m=fixture(b'ABC\rDE\r',font)
        m=run(m,'keyHandler_End');m=run(m,'moveRightWithinLine')
        assert number(m,'v_currentLine')==T+4 and number(m,'v_column')==0
        m=run(m,'keyHandler_Left')
        assert number(m,'v_currentLine')==T and number(m,'v_column')==3
        assert document(m)==b'ABC\rDE\r' and not m[S['v_documentDirty']]
        # Backspace removes the preceding character; Delete removes the next.
        word(m,'v_column',2)
        m=run(m,'keyHandler_Backspace')
        assert document(m)==b'AC\rDE\r' and number(m,'v_column')==1
        m=run(m,'keyHandler_Del')
        assert document(m)==b'A\rDE\r'
        m=run(m,'keyHandler_Del')
        assert document(m)==b'ADE\r' and number(m,'v_column')==1
        # Splitting at every position and Backspace at the new line's start
        # is a lossless round trip, including final NUL and empty lines.
        for text in [b'ABC',b'ABC\rNEXT',b'',b'\r',b'  ABC  \r']:
            for col in range(len(text.split(b'\r')[0])+1):
                m=fixture(text,font);word(m,'v_column',col)
                m=run(m,'keyHandler_Enter')
                assert document(m)==text[:col]+b'\r'+text[col:],(font,text,col)
                assert number(m,'v_column')==0
                m=run(m,'keyHandler_Backspace')
                assert document(m)==text and number(m,'v_column')==col,(font,text,col)
        # Empty final line after a terminator is a real cursor position.
        m=fixture(b'ABC\r',font)
        m=run(m,'keyHandler_DocumentEnd')
        assert number(m,'v_currentLine')==T+4 and number(m,'v_column')==0
        m=run(m,'moveRightWithinLine');assert number(m,'v_currentLine')==T+4
        m=run(m,'keyHandler_DocumentHome')
        assert number(m,'v_currentLine')==T and number(m,'v_column')==0
        m=run(m,'keyHandler_Backspace');assert document(m)==b'ABC\r'
        # Last page ends on the actual last line/column; Home returns to 1:1.
        text=b''.join(b'LINE\r' for _ in range(40))+b'LAST'
        m=fixture(text,font);m=run(m,'keyHandler_DocumentEnd')
        assert number(m,'v_currentLine')==T+200 and number(m,'v_column')==4
        assert number(m,'v_lineNumber')==41
        assert m[S['bios_vars.cursorY']]==S['EDITOR_LAST_Y']
        m=run(m,'keyHandler_DocumentHome')
        assert number(m,'v_lineNumber')==1 and number(m,'v_column')==0
        # CTRL from BIOS must be dispatched before ordinary Home/End.
        for key,target in [(12,'keyHandler_DocumentHome'),(10,'keyHandler_DocumentEnd')]:
            probe=bytearray(m)
            at=S['bios_getch'];probe[at:at+4]=bytes([0x3e,key,0x37,0xc9])
            run(probe,'keyInput',end=S[target])
    assert not any(name in S for name in ['keyHandler_Esc_S','keyHandler_Esc_J','readTapeByte'])
    # Startup errors use the console; errors within an active editor use
    # the shared modal. Stop before getch so the CPU fixture needs no input.
    m=fixture(b'ABC\r')
    run(m,'editorShowFileTooLarge',end=S['editorErrorWait'])
    m[S['editorInterfaceReady']]=0
    run(m,'editorShowFileTooLarge',end=S['editorStartupErrorWait'])


def packet_alignment():
    for font,columns in [('Rotate',64),('Fast8',48),('Cpm4',96)]:
        for size in [0,1,2,3,4,5,columns-1,columns]:
            for selected in [False,True]:
                m=fixture(b'A'*size+b'\r',font)
                if selected:
                    word(m,'v_selectionBegin',T);word(m,'v_selectionEnd',T+size+1)
                m[S['editorRowLengths']]=columns
                public=S['bios_printStringRawLim']
                target=int.from_bytes(m[public+1:public+3],'little')
                # Check the actual API calls: every length is a multiple of
                # four and every X begins on a whole VRAM byte (X/2 & 3=0).
                code=b'\xf5\x79\xe6\x03\xc2'+W(0x8d30)+b'\x3a'+W(S['bios_vars.cursorX'])+b'\xe6\x03\xc2'+W(0x8d30)+b'\xf1\xc3'+W(target)
                m[0x8d00:0x8d00+len(code)]=code
                m[0x8d30:0x8d37]=b'\xf1\x3e\x01\x32\x80\x8d\xc9'
                m[0x8d80]=0;m[public+1:public+3]=W(0x8d00)
                m=run(m,'drawTextLine',hl=T)
                assert m[0x8d80]==0,(font,size,selected,'unaligned raw packet')
                assert document(m)==b'A'*size+b'\r'
                assert m[S['editorRowLengths']]==(columns if selected else size)


def console_startup():
    m=fixture(b'ABC\r')
    m[S['bios_upper.escMode']]=255
    word(m,'bios_vars.cursorY',100)
    m[0x9000]=0xa5;m[0x90c8]=0x5a
    m=run(m,'editorLoadingBanner')
    assert m[0x9000]==0xa5 and m[0x90c8]==0x5a,'banner cleared console'
    assert number(m,'bios_vars.cursorY')==120
    m[S['editorInterfaceReady']]=0
    m=run(m,'editorShowFileTooLarge',end=S['editorStartupErrorWait'])
    assert number(m,'bios_vars.cursorY')==140,'extra blank line before error'
    assert m[0x9000]==0xa5 and m[0x90c8]==0x5a


def document_line_counts():
    def check(m, expected):
        assert document(m) == expected
        assert number(m, 'editorBarV.total') == len(re.findall(rb'\r\n|[\r\n]',expected))+1

    m=fixture(b'A\r\n\nB\rC')
    m=run(m,'editorPrepareDocument')
    check(m,b'A\r\n\nB\rC')
    m=fixture(b'')
    check(run(m,'editorPrepareDocument'),b'')

    original=b'X\r'*22+b'AB'
    m=fixture(original)
    m=run(m,'editorSetScrollLayout')
    assert m[S['editorCopyWidth']]==48
    m=run(m,'keyHandler_Enter')
    check(m,b'\r'+original)
    m=run(m,'editorApplyScrollLayout')
    assert m[S['editorCopyWidth']]==47
    m=run(m,'keyHandler_Backspace')
    check(m,original)
    m=run(m,'editorApplyScrollLayout')
    assert m[S['editorCopyWidth']]==48
    word(m,'editorBarV.position',0)
    m=run(m,'keyHandler_Space',bc=ord('Q'))
    m=run(m,'editorUndoLine')
    check(m,original)
    assert number(m,'editorBarV.position')==0

    original=b'AA\rBB\r\rCC'
    for entry,expected in [
        ('editorCopySelection',b'AA\rBB\r\rCCA\rBB\r'),
        ('editorMoveSelection',b'A\rCCA\rBB\r'),
        ('editorDeleteSelection',b'A\rCC'),
    ]:
        m=fixture(original)
        m=run(m,'gapMove',hl=T+4)
        word(m,'v_currentLine',T+7);word(m,'v_column',2)
        word(m,'v_selectionBegin',T+1);word(m,'v_selectionEnd',T+6)
        check(run(m,entry),expected)

    m=fixture(b'AB')
    m=run(m,'editorAddLines',bc=65535)
    assert number(m,'editorBarV.total')==65535
    m=run(m,'editorSubtractLines',bc=65535)
    assert number(m,'editorBarV.total')==1
    m=run(m,'editorSubtractLines',bc=9)
    assert number(m,'editorBarV.total')==1


def document_scrollbars():
    # The horizontal track uses the row below the last glyph. The vertical
    # track occupies the final screen byte and is excluded from the text copy.
    vtrack=0xbf00+S['EDITOR_TEXT_TOP']
    htrack=0x9000+S['EDITOR_BAR_Y']
    hthumb=htrack+1
    hline=htrack+4
    for font,full_columns in [('Cpm4',96),('Rotate',64),('Fast8',48)]:
        m=fixture(b'Short',font)
        m=run(m,'editorSetScrollLayout');m=run(m,'editorDrawDocumentBars')
        assert m[S['v_columns']]==full_columns
        assert not m[S['editorHShown']] and m[vtrack]==0
        m=fixture(b'X\r'*23,font)
        m=run(m,'editorSetScrollLayout');m=run(m,'editorDrawDocumentBars')
        assert number(m,'editorBarV.total')==24
        assert m[S['v_columns']]==376//(m[S['v_charStep']]*2)
        assert m[vtrack]==0x7f and m[vtrack+S['EDITOR_BAR_Y']-S['EDITOR_TEXT_TOP']-1]==0x08
        word(m,'v_lineNumber',2)
        m=run(m,'editorDrawDocumentBars')
        assert number(m,'editorBarV.position')==1 and m[vtrack]==0x08

        row=b'\t'*30
        m=fixture(row+b'\rShort',font)
        m=run(m,'editorSetScrollLayout');m=run(m,'redrawPreservingCursor');m=run(m,'editorDrawDocumentBars')
        assert m[S['editorHShown']] and m[hthumb]==0xff and m[hthumb+47*256]==0 and m[hline+47*256]==0xff
        word(m,'v_leftColumn',(241-full_columns+7)&~7)
        m=run(m,'editorDrawDocumentBars')
        assert m[hthumb]==0 and m[hline]==0xff and m[hthumb+47*256]==0xff
        word(m,'v_currentLine',T+len(row)+1)
        word(m,'v_leftColumn',0)
        m=run(m,'editorDrawDocumentBars')
        assert m[S['editorHShown']], 'A wide visible row still needs a scrollbar'
        word(m,'v_topLine',T+len(row)+1)
        m=run(m,'redrawPreservingCursor');m=run(m,'editorDrawDocumentBars')
        assert not m[S['editorHShown']]
        assert all(m[htrack+column*256:htrack+column*256+8]==bytes(8) for column in range(48))

    # Rotate has room for 62 glyphs. The last four pixels before the bar
    # remain clear after bitblt in either direction.
    m=fixture((b'X'*70+b'\r')*24)
    m=run(m,'editorSetScrollLayout');m=run(m,'redrawPreservingCursor')
    row=S['EDITOR_FIRST_Y']
    assert m[S['v_columns']]==62
    assert any(m[0xbe00+y]&0xf0 for y in range(row-7,row+1))
    for column in (62,0):
        word(m,'v_column',column)
        m=run(m,'editorPositionCursor')
        full=run(m,'redrawPreservingCursor')
        assert all(m[0xbe00+y]&0x0f==0 for y in range(row-7,row+1))
        for byte_column in range(47):
            start=0x9000+byte_column*256+S['EDITOR_TEXT_TOP']
            end=start+S['EDITOR_HEIGHT']-1
            assert m[start:end]==full[start:end],(column,byte_column)


def main():
    for original in [b'A\rB\nC\r\nD\n\rE', b'\xff\n', b'']:
        m = run(fixture(original), 'scanDocument', end=S['clearAndRedrawDocument'])
        assert document(m) == original, document(m)
    for font, cols in [('Rotate', 64), ('Fast8', 48), ('Cpm4', 96)]:
        blank = fixture(b'\rNEXT\r', font)
        word(blank, 'v_column', cols-1)
        blank = run(blank, 'editorPositionCursor')
        blank = run(blank, 'keyHandler_Space', bc=ord('X'))
        assert document(blank) == b'X\rNEXT\r'
        blank = run(blank, 'editorUndoLine')
        assert document(blank) == b'\rNEXT\r'
        payload = b'A'*400+b'\rSECOND\r'
        m = fixture(payload, font)
        assert m[S['v_columns']] == cols
        for column in [0, cols-1, cols, 255, 399, 400]:
            word(m, 'v_column', column)
            m = run(m, 'editorPositionCursor')
            assert number(m, 'v_column') == column
            assert m[S['bios_vars.cursorX']] == (column-number(m, 'v_leftColumn'))*(192//cols)
            assert document(m) == payload
        # A full bottom row must neither scroll nor change the row baseline.
        word(m, 'v_leftColumn', 0)
        word(m, 'bios_vars.cursorY', S['EDITOR_LAST_Y'])
        for x in range(48):
            m[0x9000+x*256+243:0x9000+x*256+256] = bytes([0x59])*13
        m = run(m, 'drawTextLine', hl=T)
        assert m[S['bios_vars.cursorY']] == S['EDITOR_LAST_Y']
        for x in range(48):
            assert m[0x9000+x*256+243:0x9000+x*256+256] == bytes([0x59])*13
        word(m, 'v_column', 300)
        m[S['v_insertMode']] = 1
        m = run(m, 'keyHandler_Space', bc=ord('Z'))
        assert document(m) == payload[:300]+b'Z'+payload[300:]
        m = run(m, 'keyHandler_Del')
        assert document(m) == payload[:300]+b'Z'+payload[301:]
        m = run(m, 'editorUndoLine')
        assert document(m) == payload
        assert number(m, 'v_column') == 300
        # Moving without editing cannot trim or truncate anything.
        m = run(m, 'keyHandler_End')
        m = run(m, 'keyHandler_Down')
        assert document(m) == payload
        assert number(m, 'v_currentLine') == T+401
        # Toolbar uses installed font and preserves BIOS cursor/inverse.
        word(m, 'bios_vars.cursorY', 0x1812)
        word(m, 'bios_vars.inverse', 0xffff)
        for x in range(48):
            m[0x9000+x*256:0x9000+x*256+247] = bytes([x+1])*247
        before = bytes(m[0x9000:0xc000])
        m = run(m, 'editorToolbar')
        assert number(m, 'bios_vars.cursorY') == 0x1812
        assert number(m, 'bios_vars.inverse') == 0xffff
        for x in range(48):
            assert m[0x9000+x*256:0x9000+x*256+247] == before[x*256:x*256+247], (font, 'toolbar scrolled other rows')
    for payload, expected in [(b'ABC\rNEXT\r', b'\rABC\rNEXT\r'), (b'ABC', b'\rABC'), (b'  ABC\r', b'\r  ABC\r')]:
        m = run(fixture(payload), 'keyHandler_Enter')
        assert document(m) == expected, (payload, document(m))
    m = fixture(b'A'*600+b'\rB'*3)
    word(m, 'v_column', 300)
    m = run(m, 'keyHandler_Enter')
    assert document(m).startswith(b'A'*300+b'\r'+b'A'*300+b'\r')
    m = run(m, 'keyHandler_Backspace')
    assert document(m).startswith(b'A'*600+b'\r')
    payload = b'ABC  \rDE\rFG\r'
    m = fixture(payload)
    m = run(m, 'keyHandler_End')
    assert number(m, 'v_column') == 5 and document(m) == payload
    word(m, 'v_selectionBegin', T)
    word(m, 'v_selectionEnd', T+9)
    word(m, 'v_currentLine', T+9)
    word(m, 'v_lineNumber', 3)
    m = run(m, 'editorCopySelection')
    assert document(m) == payload[:11]+payload[:9]+payload[11:]
    # All 23 display rows, vertical scrolling and paging stay inside the text area.
    payload = b''.join(f'{i:02d}\r'.encode() for i in range(30))
    m = fixture(payload)
    for _ in range(25):
        m = run(m, 'keyHandler_Down')
    assert number(m, 'v_currentLine') == T+75
    assert number(m, 'v_topLine') == T+9 and m[S['bios_vars.cursorY']] == S['EDITOR_LAST_Y']
    m = run(m, 'editorPageUp')
    assert number(m, 'v_topLine') == T and number(m, 'v_currentLine') == T+66
    m = run(m, 'editorPageDown')
    assert number(m, 'v_currentLine') == T+90
    # Refuse an undo snapshot that would overlap the document; do not lose data.
    m = fixture(b'A'*1000)
    word(m, 'v_textLimitLo', T+1500)
    m = run(m, 'gapInitialize')
    original = document(m)
    m = run(m, 'beginLineEdit')
    assert not m[S['v_lineDirty']] and document(m) == original
    scrolling_checks()
    document_line_counts()
    document_scrollbars()
    editing_keys()
    packet_alignment()
    console_startup()
    print('EDITOR LAYOUT PASS', count)

if __name__ == '__main__':
    main()
