"""Byte view on the assembled 8080 binary with the three actual font engines."""
from pathlib import Path
import re
import runpy

ROOT = Path(__file__).resolve().parents[2]
layout = runpy.run_path(str(ROOT/'scripts/tests/check-editor-layout.py'))
fixture, run, document, word, number = (layout[k] for k in ('fixture','run','document','word','number'))
S, T = layout['S'], layout['T']
checksum = runpy.run_path(str(ROOT/'scripts/images/executable.py'))['checksum']
ROW = S['hexRow']
pseudo = S['HEX_RENDER_PSEUDO']

def render(m, entry):
    return run(m, entry, end=S['hexWait'])

def field(m, offset, n):
    return bytes(m[ROW+offset:ROW+offset+n])

def four(value):
    return f'{value:04X}'.encode()

def columns(font, text_mode):
    if font=='Cpm4':
        return [7+14*(i//4)+3*(i%4) for i in range(16)],63,70,[5,19,33,47,61,68]
    if font=='Fast8':
        return list(range(7,29,3)),32,39,[5,30,37]
    if text_mode:
        return [6+5*(i//2)+2*(i%2) for i in range(16)],0,46,[5]
    return list(range(7,55,3)),57,0,[5,55]

for font, across in [('Rotate',16), ('Fast8',8), ('Cpm4',16)]:
    for length in (0,1,8,15,16,17,255,256,257,769):
        payload = (bytes(range(256))*4)[:length]
        m = run(fixture(payload,font),'gapMove',hl=T+length//2)
        # MAIN reuses the ordinary editor's visible-width table on entry.
        at=S['editorPageWidths']
        m[at:at+3*S['EDITOR_ROWS']]=bytes((37*i+1)&255 for i in range(3*S['EDITOR_ROWS']))
        m[S['hexActive']] = 1
        m = render(m,'hexView')
        assert m[S['hexAcross']] == across
        rows = length//across + 1 + (length+255)//256
        assert number(m,'hexBar.total') == rows,(font,length,number(m,'hexBar.total'),rows)
        m = render(m,'hexLast')
        assert number(m,'hexCaret') == length
        assert number(m,'hexTop') == max(0,rows-number(m,'hexBar.visible')),(font,length,'top')
        assert document(m) == payload
        assert number(m,'v_gapLo')==T+length//2
        for start in range(0,length,256):
            block = payload[start:start+256]
            slots,col,txt,rules = columns(font,False)
            summary = start//across + start//256 + (len(block)+across-1)//across
            m=run(m,'hexDecodeRow',hl=summary)
            assert m[S['hexSummary']] and number(m,'hexOffset')==start
            m=run(m,'hexFormatBlock')
            assert field(m,col,4)==four(checksum(block))
            assert field(m,col+4,1)==(b' ' if font=='Rotate' else b'\x80')
            for divider in rules:
                assert field(m,divider,1)==(b'\x8a' if pseudo else b'\x80')
            for offset in sorted({0, ((len(block)-1)//16)*16}):
                for half in range(16//across):
                    pos = offset+half*across
                    if pos>=len(block):
                        continue
                    word(m,'hexOffset',start+pos)
                    m=run(run(m,'hexSetCount'),'hexSelectLine')
                    m = run(m,'hexFormatData')
                    assert field(m,0,4) == (four(start+offset) if not half else b'    ')
                    for i,b in enumerate(block[pos:pos+across]):
                        assert field(m,slots[i],2) == f'{b:02X}'.encode()
                        if i and slots[i]==slots[i-1]+3:
                            assert field(m,slots[i]-1,1) == b' '
                    for divider in rules:
                        assert field(m,divider,1)==(b'\x81' if pseudo else b' ')
                    if txt:
                        raw=block[pos:pos+across]
                        filtered=bytes(b if 32<=b<127 or b>=192 or b in (163,179) else 46 for b in raw)
                        assert field(m,txt,across)==filtered.ljust(across)
                    if across==16 or half or len(block)-offset<=8:
                        assert field(m,col,4) == four(checksum(block[offset:offset+16]))
        if length!=769:
            continue
        if font=='Rotate':
            # The live address in the ruler must not overwrite its CS label.
            def cs_caption(frame):
                return [frame[0x9000+(x//8)*256+y] & (0x80>>(x%8))
                        for x in range(57*6,59*6) for y in range(1,9)]
            before=cs_caption(m)
            probe=m.copy()
            word(probe,'hexCaret',0x1234)
            probe=run(probe,'hexDrawAddress')
            assert cs_caption(probe)==before
        for filtered in (0,1,255):
            m[S['hexFiltered']]=filtered
            if font=='Rotate':
                m[S['hexMode6']]=1
            m = render(m,'hexGeometry')
            slots,cs,right,rules=columns(font,True)
            assert m[S['hexTextCol']]==right
            assert right+across <= 376//(m[S['v_charStep']]*2)
            for pos in range(0,256,across):
                word(m,'hexOffset',pos)
                m=run(run(m,'hexSetCount'),'hexSelectLine')
                m=run(m,'hexFormatData')
                raw=payload[pos:pos+across]
                for i,b in enumerate(raw):
                    assert field(m,slots[i],2)==f'{b:02X}'.encode()
                    if i and slots[i]==slots[i-1]+3:
                        assert field(m,slots[i]-1,1)==b' '
                for divider in rules:
                    assert field(m,divider,1)==(b'\x81' if pseudo else b' ')
                expect=bytes(b if not filtered or 32<=b<127 or b>=192 or b in (163,179) else 46 for b in raw)
                if filtered==255:
                    expect=bytes((b|128 if b>=96 else b) if 32<=b<128 else 46 for b in raw)
                assert field(m,right,across)==expect,(font,filtered,pos)
        # Pixel highlight matches both byte columns and raw glyph cells.
        m=render(m,'hexFirst')
        before=bytes(m[0x9000:0xc000])
        word(m,'v_selectionBegin',T+6);word(m,'v_selectionEnd',T+11)
        m=render(m,'hexRepaint')
        width=2*m[S['v_charStep']]
        def pixel(data, x, y):
            return bool(data[0x9000+(x//8)*256+y] & (0x80>>(x%8)))
        if font=='Rotate':
            y=m[S['hexFirstY']]-7
            probe=m.copy()
            word(probe,'v_selectionBegin',0)
            word(probe,'v_selectionEnd',0)
            raw=render(probe,'hexRepaint')
            separators=[slots[i]-1 for i in range(2,16,2)]+[right-1]
            for dy in range(8):
                for cell in separators:
                    assert not any(pixel(raw,x,y+dy) for x in range(6*cell,6*(cell+1)))
            assert raw[S['bios_vars.cursorY']+1] == 3*slots[0]
        for idx in range(16):
            for char in (False,True):
                slot=idx%across
                col=right+slot if char else slots[slot]
                x=col*width
                y=m[S['hexFirstY']]-7+10*(idx//across)
                bit=0x80>>(x%8)
                for dy in range(8):
                    addr=(x//8)*256+y+dy
                    assert bool((m[0x9000+addr]^before[addr])&bit)==(6<=idx<11),(font,idx,char)
# Dispatch one key and then Shift+F9 to return from the HEX loop.
# All deletion keys and ordinary input preserve even an active byte range.
m=fixture(b'A\0B\r\nC')
m[S['hexActive']]=1
m=render(m,'hexView')
word(m,'v_selectionBegin',T+1)
word(m,'v_selectionEnd',T+4)
for code,shift in ((0x7f,False),(0x7f,True),(7,False),(ord('G'),False)):
    m[0xffe1]=0 if shift else 2
    # The second BIOS call enters with Shift held and returns F9.
    stub=0x8c70
    m[S['bios_getch']:S['bios_getch']+3]=b'\xc3'+stub.to_bytes(2,'little')
    tail=stub+12
    m[stub:stub+12]=b'\x21'+tail.to_bytes(2,'little')+b'\x22'+(S['bios_getch']+1).to_bytes(2,'little')+bytes([0x3e,code,0xb7,0xc9,0,0])
    m[tail:tail+8]=b'\xaf\x32\xe1\xff\x3e\x8a\xb7\xc9'
    m=run(m,'hexWait')
    assert document(m)==b'A\0B\r\nC'
    assert number(m,'v_selectionBegin')==T+1 and number(m,'v_selectionEnd')==T+4
    assert not m[S['v_documentDirty']]

# Text projects the LF of a CRLF onto the preceding CR until the caret
# moves or text is edited. A nonmoving command keeps the original offset.
m=fixture(b'A\r\nB')
word(m,'v_column',1)
word(m,'hexCaret',2)
m[S['hexRestoreLF']]=1
m=run(m,'hexCheckTextCaret')
assert m[S['hexRestoreLF']]==1
m=run(m,'keyHandler_Left')
m=run(m,'hexCheckTextCaret')
assert not m[S['hexRestoreLF']]
m[S['hexRestoreLF']]=1
m=run(m,'editorSetDirty')
assert not m[S['hexRestoreLF']] and m[S['v_documentDirty']]
print('HEX VIEW PASS: raw gap, EOF, row/block checksums, glyphs, selection, three fonts')
# A real Down at the viewport edge crosses a physical checksum-block boundary.
# Verify both scrolling directions and page overlap against fresh rasters.
import subprocess
ticks='D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
def latest_cycles():
    return int(subprocess.check_output([ticks,'-m8080','-pc','8c20','-end',f"{S['hexWait']:x}",
                                        '-counter','20000000',str(layout['O']/'in.bin')]))

for font,across in [('Rotate',16),('Fast8',8),('Cpm4',16)]:
    m=fixture(bytes(range(256))*12,font)
    m[S['hexActive']]=1
    m=render(m,'hexView')
    print(font,'first page T',latest_cycles())
    # A marker in the right margin of the last retained row distinguishes a
    # bitmap move from redrawing identical glyphs. PageUp must move it back.
    marker = 0x9000 + 46*256 + S['EDITOR_TEXT_TOP'] + S['EDITOR_HEIGHT']-5
    probe=m.copy()
    probe[marker]=0xA5
    probe=render(probe,'hexPageDown')
    print(font,'page down T',latest_cycles())
    assert probe[marker-10*number(probe,'hexTop')]==0xA5
    probe=render(probe,'hexPageUp')
    assert probe[marker]==0xA5
    # All F6 transitions must match a fresh raster. Charset-only changes
    # preserve the address and byte digit bitmap from the preceding frame.
    word(m,'v_selectionBegin',T+7)
    word(m,'v_selectionEnd',T+319)
    m=render(m,'hexRepaint')
    for _ in range(4):
        entry='hexFilterKey'
        y=m[S['hexFirstY']]-4
        # Protect part of the address, the byte digits and an ASCII character
        # identical in all three charsets (offset 0x42, at its own screen row).
        layout_changes=font=='Rotate' and m[S['hexMode6']] in (0,3)
        marks=[] if layout_changes else [0x9000+y, 0x9000+8*256+y]
        if not layout_changes and m[S['hexTextCol']]:
            x=(m[S['hexTextCol']]+2)*2*m[S['v_charStep']]
            marks.append(0x9000+(x//8)*256+y+(0x42//m[S['hexAcross']])*10)
        saved=[m[at] for at in marks]
        for at in marks:m[at]^=1
        m=render(m,entry)
        for at,old in zip(marks,saved):
            assert m[at]==old^1,(font,entry,'overwrote unchanged field',hex(at))
            m[at]=old
        reference=render(m,'hexRepaint')
        assert m[0x9000:0xc000]==reference[0x9000:0xc000],(font,entry,'partial update',next(i for i in range(0x9000,0xc000) if m[i]!=reference[i]))
    word(m,'v_selectionBegin',0)
    word(m,'v_selectionEnd',0)
    m=render(m,'hexFirst')
    clean=render(m,'hexRepaint')
    mark_y=clean[S['hexFirstY']]-4
    marker=0x9000+mark_y
    clean[marker]^=0x80
    m=render(clean,'hexMark')
    assert m[marker]==clean[marker] and m[S['editorMarkActive']]
    m=render(m,'hexRight')
    assert m[S['hexNibble']]==1 and not number(m,'v_selectionBegin')
    m=render(m,'hexRight')
    assert number(m,'v_selectionEnd')==T+1
    m[marker]=clean[marker]
    m=render(m,'hexMark')
    assert m[marker]==clean[marker] and not m[S['editorMarkActive']]
    m=render(m,'hexMark')
    assert m[marker]==clean[marker] and not number(m,'v_selectionBegin')
    content_y=clean[S['hexBar.y']]
    assert all(m[0x9000+x*256+content_y:0x9000+x*256+238]
               == clean[0x9000+x*256+content_y:0x9000+x*256+238] for x in range(48))
    if font=='Fast8':
        for start in (5, 0xFD, 0x105):
            word(m,'hexCaret',start)
            m=run(m,'hexSetGoal')
            m=render(m,'hexDown')
            assert number(m,'hexCaret')==start+8
            m=render(m,'hexUp')
            assert number(m,'hexCaret')==start
    for marked in (False,True):
        for at in (0x1e0,0x1f0):
            row=at//across+at//256
            word(m,'hexTop',row-number(m,'hexBar.visible')+1);word(m,'hexCaret',at)
            if marked:
                word(m,'editorMarkAnchor',T+at-3)
                m[S['editorMarkActive']]=1
            m=render(m,'hexRepaint')
            for entry in ('hexDown','hexPageDown','hexPageUp','hexFirst','hexLast','hexUp'):
                m=render(m,entry)
                if entry=='hexDown' and not marked:
                    print(font,hex(at),'Down T',latest_cycles())
                reference=render(m,'hexRepaint')
                assert m[0x9000:0xc000]==reference[0x9000:0xc000],(font,entry,at,marked)
    # Returning to text with plain F9 and with Shift+F9 works on every font.
    for shift in (0,2):
        m[0xffe1]=shift
        m[S['bios_getch']:S['bios_getch']+4]=b'\x3e\x8a\xb7\xc9'
        run(m,'hexWait')
print('HEX SCROLL AND KEYS PASS')

# Geometry and navigation include the header in the editable file coordinate space.
executable=runpy.run_path(str(ROOT/'scripts/images/executable.py'))
body=bytes(range(256))+b'payload'
for kind,font in zip(('com','exe','dll','fnt','rks'),('Rotate','Fast8','Cpm4','Rotate','Fast8')):
    packed=executable['encode'](executable['Image'](body,0x8123,0x8123),kind)
    base=4 if kind=='rks' else 32
    for damaged in (False,True):
        data=bytearray(packed)
        if damaged:
            data[base+50]^=1
            if base==32:
                data[30]^=1       # bad header CS permits inspection and repair
        m=run(fixture(data,font),'gapMove',hl=T+base+127)
        name=S['FILE_NAME_HISTORY']
        filename=f'File.{kind.title()}\0'.encode()
        m[name:name+len(filename)]=filename
        m[S['hexActive']]=1
        word(m,'hexCaret',base+99)
        auto=run(m,'editorDetectHex')
        assert auto[S['editorPendingHex']]==(2 if kind=='rks' else 1)
        auto=render(auto,'hexView')
        assert not auto[S['editorPendingHex']] and number(auto,'hexBase')==base
        m=render(m,'hexView')
        # The margin below the text rows must survive F7, unlike a clear.
        marker=0x9000+242
        m[marker]^=0x80
        saved=m[marker]
        m=render(m,'hexContainerKey')
        assert m[marker]==saved
        m[marker]^=0x80
        fresh=render(m,'hexRepaint')
        assert m[0x9000:0xc000]==fresh[0x9000:0xc000], (kind,'F7 into EXEC')
        assert number(m,'hexSize')==base+len(body)
        assert number(m,'hexBase')==base and number(m,'hexLoad')==0x8123
        assert number(m,'hexCaret')==base+99 and document(m)==data
        assert m[S['hexContainerWarn']]==ord('?')
        section=m[S['hexHeaderRows']]
        assert section==(base+m[S['hexAcross']]-1)//m[S['hexAcross']]+1
        m=run(m,'hexDecodeRow',hl=section-1)
        assert m[S['hexSummary']]==2
        for row,address in [(0,b'0000'),(section,b'8120')]:
            m=run(m,'hexDecodeRow',hl=row)
            m=run(run(m,'hexSetCount'),'hexSelectLine')
            m=run(m,'hexFormatData')
            assert field(m,0,4)==address,(kind,row,field(m,0,4))
            if row==section:
                slots=columns(font,False)[0]
                assert [field(m,c,2) for c in slots[:4]]==[b'  ',b'  ',b'  ',b'00']
        m=render(m,'hexFirst')
        m=render(m,'hexEnd')
        assert number(m,'hexCaret')==min(base,m[S['hexAcross']])-1
        for _ in range(section-1):
            m=render(m,'hexDown')
        assert number(m,'hexCaret')==base+min(base,m[S['hexAcross']])-1-3
        m=render(m,'hexUp')
        assert number(m,'hexCaret')==base-1
        m=render(m,'hexRight')
        assert number(m,'hexCaret')==base-1 and m[S['hexNibble']]==1
        m=render(m,'hexRight')
        assert number(m,'hexCaret')==base
        marker=0x9000+46*256+m[S['hexFirstY']]-4
        expected_pixel=m[marker]^1
        m[marker]=expected_pixel
        m=render(m,'hexRecalcContainer')
        assert m[marker]==expected_pixel,(kind,'F8 repainted an unrelated cell')
        if kind=='rks':
            m=run(m,'hexDecodeRow',hl=number(m,'hexTrailerRow'))
            assert m[S['hexSummary']]==3
            m=run(m,'hexFormatBlock')
            assert field(m,0,6)==b'RKS CS'
            assert field(m,columns(font,False)[1],4)==four(checksum(data[base:base+len(body)]))
        assert m[S['hexContainerWarn']]==ord(' ')
        assert bool(m[S['v_documentDirty']])==damaged
        regenerated=executable['encode'](executable['Image'](bytes(data[base:base+len(body)]),0x8123,0x8123),kind)
        assert document(m)==regenerated,(kind,damaged)
        m=render(m,'hexContainerKey')
        assert not number(m,'hexBase') and number(m,'hexCaret')==base

# A short RKS has different numbers of rows with and without the header.
for font in ('Rotate','Fast8','Cpm4'):
    data=executable['encode'](executable['Image'](b'\x01\x02',0x8123,0x8123),'rks')
    m=fixture(data,font)
    name=S['FILE_NAME_HISTORY']
    m[name:name+9]=b'File.RKS\0'
    m[S['hexActive']]=1
    m=render(m,'hexView')
    for _ in range(2):
        m=render(m,'hexContainerKey')
        fresh=render(m,'hexRepaint')
        assert m[0x9000:0xc000]==fresh[0x9000:0xc000], (font,'F7 short page')

# A recognized suffix selects HEX, even when the body is a legacy raw COM.
m=fixture(b'raw')
name=S['FILE_NAME_HISTORY']
m[name:name+13]=b'D:\\RAW.COM\0\0\0'
m[S['hexActive']]=1
assert run(m,'editorDetectHex')[S['editorPendingHex']]==1
m=render(run(m,'editorDetectHex'),'hexView')
assert not m[S['hexViewType']] and document(m)==b'raw'
print('HEX CONTAINERS PASS')

# Render first, then check incremental painting against a fresh page after
# each nibble, including the 8x8 half of a row that holds its checksum.
def digit(m, key):
    return run(m,'hexInputDigit',a=ord(key),end=S['hexWait'])

for font,text_mode in [('Cpm4',False),('Rotate',False),('Rotate',True),('Fast8',False)]:
    data=bytes(range(256))+b'abc'
    for offset in (0,15,241,255,258):
        m=run(fixture(data,font),'gapMove',hl=T+offset+1)
        m[S['hexActive']]=1
        if text_mode:
            m[S['hexMode6']]=1
        word(m,'hexCaret',offset)
        word(m,'v_undoLength',1)
        m[S['v_lineDirty']]=1
        m=render(m,'hexView')
        high_x=m[S['bios_vars.cursorY']+1]
        for key,byte,caret,nibble in [('a',0xA0|(data[offset]&15),offset,1),('5',0xA5,offset+1,0)]:
            y=m[S['bios_vars.cursorY']]-4
            marker=0x9000+47*256+y
            old=m[marker]
            if offset in (0,15):
                m[marker]^=1
            m=digit(m,key)
            if offset in (0,15):
                assert m[marker]==old^1,(font,offset,key,'unrelated pixels were touched')
                m[marker]=old
            assert document(m)==data[:offset]+bytes([byte])+data[offset+1:],(font,offset,key)
            assert number(m,'hexCaret')==caret and m[S['hexNibble']]==nibble
            if nibble:
                assert m[S['bios_vars.cursorY']+1] == high_x+m[S['v_charStep']]
            assert m[S['v_documentDirty']] and not number(m,'v_undoLength')
            assert not m[S['v_lineDirty']]
            fresh=render(m,'hexRepaint')
            assert m[0x9000:0xc000]==fresh[0x9000:0xc000],(font,offset,key,'incremental raster',[(hex(i),m[i],fresh[i]) for i in range(0x9000,0xc000) if m[i]!=fresh[i]][:10])
        if offset==0:
            m=render(m,'hexFirst')
            m=digit(m,'3')
            m=render(m,'hexRight')
            assert m[S['hexNibble']]==0 and document(m)[0]==0x35
            word(m,'v_selectionBegin',T+1)
            word(m,'v_selectionEnd',T+2)
            m=digit(m,'9')
            assert document(m)[1]==1 and m[S['hexNibble']]==0
            m=render(m,'hexLast')
            assert document(digit(m,'F'))==document(m)

data=b'X\r\nY\rZ'
for gap in range(len(data)+1):
    m=run(fixture(data),'gapMove',hl=T+gap)
    m[S['hexActive']]=1
    word(m,'hexCaret',1)
    m=digit(render(m,'hexView'),'1')   # 0Dh becomes 1Dh, leaving the single LF.
    m=render(m,'hexRight')
    m=digit(m,'2')                   # 0Ah becomes 2Ah, removing the first break.
    m=run(m,'hexRestoreTextGeometry')
    assert document(m)==b'X\x1d\x2aY\rZ'
    assert number(m,'editorBarV.total')==len(re.findall(rb'\r\n|[\r\n]',document(m)))+1
    assert number(m,'v_gapLo')==T+gap

body=bytes(range(256))*3
program=executable['encode'](executable['Image'](body,load=0x5023,entry=0x5023,flags=1),'com')
for offset in (0,255,256,767):
    m=run(fixture(program),'gapMove',hl=T+32+offset+1)
    m[S['hexActive']]=1
    m=render(render(m,'hexView'),'hexContainerKey')
    word(m,'hexCaret',6)
    m=digit(render(m,'hexRepaint'),'1')
    assert number(m,'hexLoad')==0x5013
    assert document(m)[6]==0x13
    m=digit(m,'D')
    assert number(m,'hexLoad')==0x501D and number(m,'hexCaret')==7
    assert document(m)[6]==0x1D
    header=document(m)[:32]
    m=run(m,'hexHorizontal',hl=32+offset,end=S['hexWait'])
    for key,byte in [('0',body[offset]&15),('7',7)]:
        m=digit(m,key)
        result=body[:offset]+bytes([byte])+body[offset+1:]
        assert document(m)==header+result
        assert m[S['hexContainerWarn']]==ord('?')
    m=render(m,'hexRecalcContainer')
    expected=bytearray(header)
    expected[12:14]=checksum(result).to_bytes(2,'little')
    expected[30:32]=checksum(expected[:30]).to_bytes(2,'little')
    assert document(m)==expected+result,(offset,'recomputed CS in place')
    assert m[S['hexContainerWarn']]==ord(' ')
# A malformed length must be caught before either checksum field is written.
m=fixture(program)
m[S['hexActive']]=1
m=render(render(m,'hexView'),'hexContainerKey')
word(m,'hexCaret',10)
m=digit(render(m,'hexRepaint'),'1')
before=document(m)
m=run(m,'hexRecalcContainer',end=S['hexContainerInvalid'])
assert document(m)==before
print('HEX NIBBLES PASS')

# The first 8x8 half is skipped when the load address leaves it empty.
# The first divider covers the populated bytes up to the end of the first displayed 256-byte block.
for font in ('Rotate','Fast8','Cpm4'):
    packed=executable['encode'](executable['Image'](body,0x812D,0x812D),'com')
    m=fixture(packed,font)
    m[S['hexActive']]=1
    m=render(render(m,'hexView'),'hexContainerKey')
    section=m[S['hexHeaderRows']]
    across=m[S['hexAcross']]
    skipped=int(across==8)
    m=run(m,'hexDecodeRow',hl=section)
    m=run(run(m,'hexSetCount'),'hexSelectLine')
    m=run(m,'hexFormatData')
    slots,col,_,_=columns(font,False)
    assert field(m,0,4)==b'8120'
    assert field(m,slots[13%across],2)==b'00'
    divider=section+256//across-skipped
    m=run(m,'hexDecodeRow',hl=divider)
    assert m[S['hexSummary']]==1
    m=run(m,'hexFormatBlock')
    assert field(m,col,4)==four(checksum(body[:243]))
    m=run(m,'hexDecodeRow',hl=divider+1)
    assert number(m,'hexOffset')==32+243
    m=run(run(m,'hexSetCount'),'hexSelectLine')
    m=run(m,'hexFormatData')
    assert field(m,0,4)==b'8220'

# Modal capture uses the free gap on MAIN and preserves its document and sentinel.
m=fixture(b'Modal background')
m[S['hexActive']]=1
m=render(m,'hexView')
m=run(m,'editorGeometry')
before=bytes(m[0x9000:0xc000])
m=run(m,'editorModalBegin',hl=S['editorFileDescriptor'])
assert m[S['hexUnderlay']] and bytes(m[0x9000:0xc000])!=before
m=run(m,'editorCloseModal')
assert bytes(m[0x9000:0xc000])==before
assert document(m)==b'Modal background' and m[number(m,'v_gapLo')]==0
print('HEX MODAL UNDERLAY PASS')

# Nibble movement is independent of edits and keeps the byte selection unit.
for font in ('Rotate','Fast8','Cpm4'):
    m=fixture(bytes([0xAB])*17,font)
    m[S['hexActive']]=1
    m=render(m,'hexView')
    x=m[S['bios_vars.cursorX']]
    m=render(m,'hexRight')
    assert number(m,'hexCaret')==0 and m[S['hexNibble']]==1
    assert m[S['bios_vars.cursorX']]==x+m[S['v_charStep']]
    m=render(m,'hexDown')
    assert number(m,'hexCaret')==m[S['hexAcross']] and m[S['hexNibble']]==1
    m=render(m,'hexUp')
    assert number(m,'hexCaret')==0 and m[S['hexNibble']]==1
    m=digit(m,'2')
    assert document(m)[0]==0xA2 and number(m,'hexCaret')==1 and m[S['hexNibble']]==0
    m=render(m,'hexLeft')
    assert number(m,'hexCaret')==0 and m[S['hexNibble']]==1
    m=render(m,'hexLast')
    m=render(m,'hexRight')
    assert number(m,'hexCaret')==17 and m[S['hexNibble']]==0
    m=render(m,'hexLeft')
    assert number(m,'hexCaret')==16 and m[S['hexNibble']]==1
print('HEX NIBBLE NAVIGATION PASS')

normal=render(fixture(b'Hex toolbar'),'hexView')
normal[S['hexActive']]=1
normal=run(normal,'editorToolbar')
shifted=normal.copy()
shifted[S['editorShiftState']]=1
shifted=run(shifted,'editorToolbar')
def toolbar(m, columns):
    return b''.join(m[x*256+247:(x+1)*256] for x in columns)
assert toolbar(shifted,range(0xa8,0xb9))==b'\xff'*(17*9)
assert toolbar(shifted,range(0xb9,0xc0))==toolbar(normal,range(0xb9,0xc0))

payload=b'X\r\nY\x80ZQ'
m=run(fixture(payload),'gapMove',hl=T+2)
m[S['hexActive']]=1
m=render(m,'hexView')
word(m,'v_selectionBegin',T+1)
word(m,'v_selectionEnd',T+4)
for pos in (0,1,4,len(payload)):
    probe=m.copy()
    word(probe,'hexCaret',pos)
    probe=render(probe,'hexCopyBlock')
    assert document(probe)==payload[:pos]+payload[1:4]+payload[pos:],pos
    assert number(probe,'hexCaret')==pos+3 and probe[S['v_documentDirty']]
    assert number(probe,'v_selectionBegin')==T+(4 if pos<=1 else 1)
    assert number(probe,'v_selectionEnd')==T+(7 if pos<=1 else 4)
    probe=run(probe,'hexRestoreTextGeometry')
    assert number(probe,'editorBarV.total')==len(re.findall(rb'\r\n|[\r\n]',document(probe)))+1

for pos in (0,2,4):
    probe=m.copy()
    word(probe,'hexCaret',pos)
    probe=render(probe,'hexOverwriteBlock')
    assert document(probe)==payload[:pos]+payload[1:4]+payload[pos+3:],pos
    assert number(probe,'hexCaret')==pos+3 and probe[S['v_documentDirty']]
    assert number(probe,'v_selectionBegin')==T+1 and number(probe,'v_selectionEnd')==T+4

for pos in (0,2,len(payload)):
    probe=m.copy()
    word(probe,'hexCaret',pos)
    probe=run(probe,'hexInsertZeroCount',hl=4,end=S['hexWait'])
    assert document(probe)==payload[:pos]+bytes(4)+payload[pos:]
    assert number(probe,'hexCaret')==pos+4 and probe[S['v_documentDirty']]
probe=run(m,'hexFillValue',hl=0x1234,end=S['hexWait'])
assert document(probe)==b'X\x34\x12\x34\x80ZQ'
assert number(probe,'v_selectionBegin')==T+1 and number(probe,'v_selectionEnd')==T+4
assert document(run(m,'hexInsertZeroCount',hl=0,end=S['hexWait']))==payload

for command,pos in [('hexCopyBlock',2),('hexOverwriteBlock',5),('hexCopyBlock',0),('hexFillWord',0)]:
    probe=m.copy()
    word(probe,'hexCaret',pos)
    if pos==0:
        word(probe,'v_selectionEnd',number(probe,'v_selectionBegin'))
    probe=run(probe,command,end=S['hexBlockError'])
    assert document(probe)==payload and not probe[S['v_documentDirty']]
free=number(m,'v_gapHi')-number(m,'v_gapLo')
probe=run(m,'hexInsertZeroCount',hl=free+1,end=S['hexBlockError'])
assert document(probe)==payload and not probe[S['v_documentDirty']]
probe=run(m,'hexInsertZeroCount',hl=free,end=S['hexWait'])
assert document(probe)==bytes(free)+payload

payload=bytes(range(254))
probe=fixture(payload)
probe[S['hexActive']]=1
probe=render(probe,'hexView')
word(probe,'hexCaret',252)
probe=run(probe,'hexInsertZeroCount',hl=4,end=S['hexWait'])
expected=payload[:252]+bytes(4)+payload[252:]
assert document(probe)==expected
assert probe[0x9000:0xc000]==render(probe,'hexRepaint')[0x9000:0xc000]
for row,block in ((16,expected[:256]),(18,expected[256:])):
    summary=run(run(probe,'hexDecodeRow',hl=row),'hexFormatBlock')
    assert summary[S['hexSummary']]==1 and field(summary,57,4)==four(checksum(block))

container=executable['encode'](executable['Image'](b'X\r\nY\x80ZQ',load=0x5023,entry=0x5023,flags=1),'com')
probe=fixture(container)
probe[S['hexActive']]=1
probe=render(render(probe,'hexView'),'hexContainerKey')
word(probe,'v_selectionBegin',T+33)
word(probe,'v_selectionEnd',T+36)
for command in ('hexCopyBlock','hexInsertZeros'):
    rejected=run(probe,command,end=S['hexBlockError'])
    assert document(rejected)==container and not rejected[S['v_documentDirty']]
probe=run(probe,'hexFillValue',hl=0x1234,end=S['hexWait'])
assert document(probe)==container[:33]+b'\x34\x12\x34'+container[36:]
assert probe[S['hexViewType']]==1 and probe[S['hexContainerWarn']]==ord('?')
probe=render(probe,'hexRecalcContainer')
assert probe[S['hexContainerWarn']]==ord(' ')
print('HEX BLOCK EDIT PASS: copy, overlap, padding, word fill, bounds, EXEC checksums')
