"""8080: all byte values are document data, including across a gap."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,number,word=(g[k] for k in ('fixture','run','document','number','word'))
S=g['S'];T=g['T']
for original in [bytes(range(256)), b'\x00A\x00', b'\xff', b'']:
    m=run(fixture(original),'scanDocument',end=S['clearAndRedrawDocument'])
    assert document(m)==original
m=fixture(b'A\xffB\rC\xffD')
m=run(m,'beginLineEdit')
u=number(m,'v_undoStart');assert m[u:u+3]==b'A\xffB'
m=run(m,'keyHandler_Space',bc=255)
assert document(m)==b'\xffA\xffB\rC\xffD'
m=run(m,'editorUndoLine');assert document(m)==b'A\xffB\rC\xffD'
m=run(m,'resizeTextRange',hl=T+4,de=T+5)
assert document(m)[5:]==b'C\xffD'
m=run(m,'resizeTextRange',hl=T+5,de=T+4)
assert document(m)==b'A\xffB\rC\xffD'
# Exact-length file reads append the NUL without filtering FF.
m=fixture(b'A\xffB\rC\xffD')
m[S['fileReadDescriptor']:S['fileReadDescriptor']+2]=b'\xb7\xc9'
word(m,'fileDescSize',7)
m=run(m,'loadTextFile',de=T)
assert m[T:T+8]==b'A\xffB\rC\xffD\0'
assert number(m,'v_textEnd')==T+7

# NUL at the start, middle and end; CRLF can cross the physical gap.
payload=b'\0A\0\r\nB\n\rZ\0'
for offset in range(len(payload)+1):
    m=run(fixture(payload),'gapMove',hl=T+offset)
    m=run(m,'drawTextLine',hl=T)
    assert m[S['editorRowBuffer']:S['editorRowBuffer']+3]==b' A '
    for at in (5,7,8):
        m=run(m,'keyHandler_Down')
        assert number(m,'v_currentLine')==T+at
    for at in (7,5,0):
        m=run(m,'keyHandler_Up')
        assert number(m,'v_currentLine')==T+at
    word(m,'v_column',3)
    m=run(m,'keyHandler_Del')
    assert document(m)==payload[:3]+payload[5:]
    assert number(m,'editorBarV.total')==3

# Search past NUL, overwrite a NUL byte, and restore the raw line.
m=fixture(b'A\0B\r\nC')
m[S['v_searchText']:S['v_searchText']+2]=b'B\0'
m=run(m,'editorFindNext')
assert number(m,'v_column')==2
word(m,'v_column',1)
m=run(m,'editorToggleInsert')
m=run(m,'keyHandler_Space',bc=88)
assert document(m)==b'AXB\r\nC'
m=run(m,'editorUndoLine')
assert document(m)==b'A\0B\r\nC'

# Removing the only byte between two different separators creates CRLF.
m=run(fixture(b'A\rX\nB'),'keyHandler_Down')
m=run(m,'keyHandler_Del')
assert document(m)==b'A\r\nB' and number(m,'editorBarV.total')==2
assert number(m,'v_currentLine')==T+3
m=run(m,'editorUndoLine')
assert document(m)==b'A\rX\nB' and number(m,'editorBarV.total')==3
m=fixture(b'A\nB')
word(m,'v_column',1)
m=run(m,'keyHandler_Enter')
assert document(m)==b'A\n\nB' and number(m,'editorBarV.total')==3

# Marked blocks may split or create a CRLF at either splice.
for start,end in [(1,2),(2,3),(3,4),(2,5),(1,5),(0,7)]:
    original=b'A\r\nX\r\nB'
    m=fixture(original)
    word(m,'v_selectionBegin',T+start)
    word(m,'v_selectionEnd',T+end)
    m=run(m,'editorDeleteSelection')
    expected=original[:start]+original[end:]
    assert document(m)==expected
    assert number(m,'editorBarV.total')==1+expected.count(13)+expected.count(10)-expected.count(b'\r\n')
original=b'A\rX\nB\r\nC\0'
for entry,start,end,line,column in [
    ('editorCopySelection',3,4,2,0),  # insert LF after CR
    ('editorCopySelection',1,2,2,1),  # insert CR before LF
    ('editorMoveSelection',2,3,7,2),  # removal joins a pair; append at EOF
    ('editorMoveSelection',7,9,0,0),  # move NUL from the suffix to the beginning
]:
    m=fixture(original)
    word(m,'v_currentLine',T+line)
    word(m,'v_column',column)
    word(m,'v_selectionBegin',T+start)
    word(m,'v_selectionEnd',T+end)
    m=run(m,entry)
    target=line+column
    block=original[start:end]
    expected=original
    if entry=='editorMoveSelection':
        expected=expected[:start]+expected[end:]
        if target>=end:target-=len(block)
    expected=expected[:target]+block+expected[target:]
    assert document(m)==expected
    assert number(m,'editorBarV.total')==1+expected.count(13)+expected.count(10)-expected.count(b'\r\n')
print('EDITOR NUL PASS',g['run'].__globals__['count'])
