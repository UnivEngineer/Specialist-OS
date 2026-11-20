"""8080 gap invariants, distant edits, F8 relocation and unchanged navigation."""
from pathlib import Path
import random, runpy

R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'))
S=g['S'];T=g['T']

def check(m,expected):
    assert document(m)==expected
    lo=number(m,'v_gapLo');hi=number(m,'v_gapHi');end=number(m,'v_textEnd')
    limit=number(m,'v_textLimitLo');u=number(m,'v_undoLength')
    assert T<=lo<=end and lo<=hi<limit and end+hi-lo==limit-1
    assert m[limit-1]==0
    assert not u or number(m,'v_undoStart')==hi-u
    assert hi-lo>=u

text=b'A\xffB\r'+bytes(range(32,127))*3+b'\rLAST\r'
m=fixture(text);rng=random.Random(8080)
for i in range(100):
    pos=rng.randrange(len(text)+1)
    if i%3 or pos==len(text):
        c=rng.choice([13,32,65,255])
        m=run(m,'resizeTextRange',hl=T+pos,de=T+pos+1)
        m=run(m,'gapWrite',hl=T+pos,bc=c)
        text=text[:pos]+bytes([c])+text[pos:]
    else:
        count=min(rng.randrange(1,8),len(text)-pos)
        m=run(m,'resizeTextRange',hl=T+pos+count,de=T+pos)
        text=text[:pos]+text[pos+count:]
    check(m,text)
    # Read/scan forward and backward across the gap, including FF and CR.
    if i%10==0:
        for offset in [0,pos,len(text)]:
            out=run(m,'nextLine',hl=T+offset)
            assert document(out)==text
        painted=run(m,'redrawPreservingCursor')
        flat=run(m,'gapFlatten');flat=run(flat,'redrawPreservingCursor')
        assert painted[0x9000:0xc000]==flat[0x9000:0xc000]

# Retain an entire line's snapshot while moving the gap in both directions,
# including chunks smaller than the snapshot and overlapping BIOS copies.
original=bytes(range(33,123))+b'\rTAIL'
m=fixture(original)
word(m,'v_textLimitLo',T+len(original)+100)
m=run(m,'gapInitialize')
word(m,'v_column',45)
m=run(m,'keyHandler_Space',bc=ord('X'));expected=original[:45]+b'X'+original[45:]
for pos in [0,90,2,88,20,60]:
    word(m,'v_column',pos)
    m=run(m,'keyHandler_Space',bc=ord('Z'))
    expected=expected[:pos]+b'Z'+expected[pos:]
    check(m,expected)
m=run(m,'editorUndoLine');check(m,original)
assert number(m,'v_column')==45

# Navigation never copies the document, even across a full-file distance.
text=b'LINE\r'*900+b'END';m=fixture(text)
word(m,'v_column',2);m=run(m,'keyHandler_Space',bc=ord('X'))
stored=bytes(m[T:0x8000]);lo=number(m,'v_gapLo');hi=number(m,'v_gapHi')
for key in ['keyHandler_DocumentEnd','keyHandler_DocumentHome','editorPageDown','editorPageUp']:
    m=run(m,key)
    assert bytes(m[T:0x8000])==stored
    assert (number(m,'v_gapLo'),number(m,'v_gapHi'))==(lo,hi)
    assert m[S['v_lineDirty']]
    if key=='keyHandler_DocumentEnd':assert number(m,'v_lineNumber')==901
    if key=='keyHandler_DocumentHome':assert number(m,'v_lineNumber')==1
    if key=='editorPageDown':assert number(m,'v_lineNumber')==22

# Search must cross the physical gap as if it were contiguous text.
m=fixture(b'FIRST\rBEFORE-NEEDLE-AFTER\rLAST')
m=run(m,'gapMove',hl=T+15)
m[S['v_searchText']:S['v_searchText']+7]=b'NEEDLE\0'
m=run(m,'editorFindNext')
assert number(m,'v_currentLine')==T+6

# Selected blocks can lie before or after an insertion point, across either
# physical span. Moves also delete the original after copying it.
for key,start,end,current,expected in [
    ('editorCopySelection',2,4,6,b'A\rB\rC\rB\rD\r'),
    ('editorMoveSelection',2,4,6,b'A\rC\rB\rD\r'),
    ('editorCopySelection',4,6,0,b'C\rA\rB\rC\rD\r'),
    ('editorMoveSelection',4,6,0,b'C\rA\rB\rD\r'),
]:
    m=fixture(b'A\rB\rC\rD\r');m=run(m,'gapMove',hl=T+3)
    word(m,'v_selectionBegin',T+start);word(m,'v_selectionEnd',T+end)
    word(m,'v_currentLine',T+current);word(m,'v_lineNumber',current//2+1)
    m=run(m,key);check(m,expected)

# Zero free bytes: deletion is still possible and creates a usable gap.
m=fixture(b'ABCDE')
word(m,'v_textLimitLo',T+6)
m=run(m,'gapInitialize');m=run(m,'resizeTextRange',hl=T+3,de=T+2)
check(m,b'ABDE')
print('EDITOR GAP PASS',g['run'].__globals__['count'])
