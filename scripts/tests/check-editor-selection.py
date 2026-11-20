"""8080: byte selections, multiline copy/move/delete and exact pixel highlight."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2];g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'));S=g['S'];T=g['T']
payload=b'AB\tCDE\rFGHI\r\rJKLM\r'
for font in ['Rotate','Fast8','Cpm4']:
 for start,end in [(0,1),(1,4),(2,3),(4,10),(6,7),(7,len(payload)),(0,len(payload))]:
  for gap in [0,3,9,len(payload)]:
   for pos in sorted(set([0,start,end,len(payload),len(payload)-1])):
    for operation in ['editorCopySelection','editorMoveSelection']:
     m=fixture(payload,font);m=run(m,'gapMove',hl=T+gap)
     word(m,'v_selectionBegin',T+start);word(m,'v_selectionEnd',T+end)
     word(m,'editorBlockResult',T+pos);m=run(m,'editorBlockReposition')
     m=run(m,operation)
     if start<pos<end or operation=='editorMoveSelection' and pos in (start,end):want=payload;caret=pos
     elif operation=='editorCopySelection':want=payload[:pos]+payload[start:end]+payload[pos:];caret=pos+end-start
     elif pos<start:want=payload[:pos]+payload[start:end]+payload[pos:start]+payload[end:];caret=pos+end-start
     else:want=payload[:start]+payload[end:pos]+payload[start:end]+payload[pos:];caret=pos
     assert document(m)==want,(font,start,end,pos,gap,operation,document(m),want)
     assert number(m,'v_currentLine')+number(m,'v_column')==T+caret,(operation,start,end,pos)
   for operation in ['editorDeleteSelection','keyHandler_Del','keyHandler_Backspace']:
    m=fixture(payload,font);m=run(m,'gapMove',hl=T+gap)
    word(m,'v_selectionBegin',T+start);word(m,'v_selectionEnd',T+end)
    m=run(m,operation);assert document(m)==payload[:start]+payload[end:],(operation,start,end,gap)
    assert number(m,'v_currentLine')+number(m,'v_column')==T+start
    assert not number(m,'v_selectionBegin')
 # Editing before a frozen range updates its exact pixel positions.
 for overwrite in [False,True]:
  m=fixture(b'AB\tCDEFG\rNEXT',font);m[S['v_tabWidth']]=8
  word(m,'v_selectionBegin',T+4);word(m,'v_selectionEnd',T+7)
  word(m,'v_column',2 if overwrite else 0);m[S['v_insertMode']]=0 if overwrite else 255
  m=run(m,'redrawPreservingCursor');m=run(m,'keyHandler_Space',bc=ord('X'))
  full=run(m,'redrawPreservingCursor');assert m[0x9000:0xc000]==full[0x9000:0xc000],(font,'edit-before-range',overwrite)
 # An empty active mark must not overwrite the inserted key with toolbar colors.
 m=fixture(b'ABC',font);m=run(m,'editorToggleMark');m=run(m,'keyHandler_Space',bc=ord('Z'))
 assert document(m)==b'ZABC' and not m[S['editorMarkActive']]
 # Undo clears a subsequently selected byte range before resizing the row.
 m=fixture(b'ABC',font);m=run(m,'keyHandler_Space',bc=ord('X'))
 word(m,'v_selectionBegin',T+1);word(m,'v_selectionEnd',T+2)
 m=run(m,'editorUndoLine');assert document(m)==b'ABC' and not number(m,'v_selectionBegin')
 # Forward and reverse marking start at the byte caret, not the line start.
 m=fixture(b'abcdef\rNEXT',font);word(m,'v_column',2);m=run(m,'editorToggleMark')
 assert not number(m,'v_selectionBegin')
 m=run(m,'keyHandler_Right');m=run(m,'editorUpdateMark')
 assert (number(m,'v_selectionBegin'),number(m,'v_selectionEnd'))==(T+2,T+3)
 m=run(m,'keyHandler_Left');m=run(m,'keyHandler_Left');m=run(m,'editorUpdateMark')
 assert (number(m,'v_selectionBegin'),number(m,'v_selectionEnd'))==(T+1,T+2)
 # Compare all VRAM bits: only selected cells (including expanded tabs) invert.
 for tab in [2,4,6,8]:
  for left in [0,8,16]:
   line=b'AB\tCDEFGHIJKLMNOPQRST'
   positions=[0]
   for c in line:positions.append((positions[-1]//tab+1)*tab if c==9 else positions[-1]+1)
   for start,end in [(0,1),(1,4),(2,3),(4,10),(5,len(line)),(0,len(line)+1)]:
    base=fixture(line+b'\r',font);base[S['v_tabWidth']]=tab;word(base,'v_leftColumn',left)
    base=run(base,'redrawPreservingCursor');m=bytearray(base)
    word(m,'v_selectionBegin',T+start);word(m,'v_selectionEnd',T+end)
    m=run(m,'redrawPreservingCursor');width=m[S['v_charStep']]*2;cols=m[S['v_columns']]
    a=max(0,min(cols,positions[start]-left));b=cols if end>len(line) else max(0,min(cols,positions[end]-left))
    want=bytearray(base[0x9000:0xc000])
    for x in range(a*width,b*width):
     for y in range(S['EDITOR_FIRST_Y']-7,S['EDITOR_FIRST_Y']+1):want[(x//8)*256+y]^=0x80>>(x%8)
    assert m[0x9000:0xc000]==want,(font,tab,left,start,end,a,b)
    word(m,'v_selectionBegin',0);m=run(m,'redrawPreservingCursor')
    assert m[0x9000:0xc000]==base[0x9000:0xc000],(font,'clear',tab,left,start,end)
print('EDITOR BYTE SELECTION PASS',g['run'].__globals__['count'])
